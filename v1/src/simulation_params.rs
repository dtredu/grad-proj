//! Simulation timing parameters for the PWM display and camera.
//!
//! This module defines [`SimulationParams`], which holds all configurable timing
//! parameters for the display (resolution, blanking, pixel clock, PWM) and
//! camera (resolution, frame rate, shutter timing). It also computes derived
//! frequencies and provides default values.
//!
//! ## Timing Model
//!
//! See [`docs/simulation.md`](docs/simulation.md) for the full mathematical
//! model. In brief:
//!
//! - **Display row timing**: In progressive mode each row starts its PWM cycle
//!   after `row * (1 / scanline_frequency)`.
//! - **Capture timing**: Rolling shutter captures scanlines sequentially;
//!   global shutter captures all at once.
//! - **Brightness**: Evaluated from the PWM phase at the capture time for each
//!   pixel.

use bytemuck::{Pod, Zeroable};

/// All configurable timing parameters for the display and camera.
///
/// This struct is uploaded to the fragment shader as a uniform buffer
/// (group 1, binding 0). It controls the simulated artifact patterns.
///
/// # Layout
///
/// Fields are arranged to match the `SimulationParams` struct in the WGSL
/// shader. Padding fields (`pad0`, `pad1`, `pad2`) ensure proper alignment.
#[repr(C)]
#[derive(Copy, Clone, Debug, Pod, Zeroable)]
pub struct SimulationParams {
    /// Active horizontal resolution, in pixels.
    pub display_h_active: f32,
    /// Active vertical resolution, in pixels.
    pub display_v_active: f32,
    /// Horizontal blanking interval, in pixels.
    pub display_h_blank: f32,
    /// Vertical blanking interval, in pixels.
    pub display_v_blank: f32,
    /// Pixel clock frequency (Hz).
    pub display_freq_pixel: f32,
    /// Derived scanline frequency (Hz).
    ///
    /// Computed as `display_freq_pixel / (display_h_active + display_h_blank)`.
    pub display_freq_scanline: f32,
    /// Derived frame frequency (Hz).
    ///
    /// Computed as `display_freq_scanline / (display_v_active + display_v_blank)`.
    pub display_freq_frame: f32,
    /// Display emission type: 0 = progressive, 1 = simultaneous (global).
    pub display_emission_type: i32,
    /// PWM frequency (Hz).
    pub display_pwm_freq: f32,
    /// PWM duty cycle in [0.0, 1.0].
    pub display_pwm_duty: f32,
    /// Display start offset (seconds) relative to global simulation time.
    pub display_start_offset: f32,
    /// Camera horizontal resolution, in pixels.
    pub camera_width: f32,
    /// Camera vertical resolution, in pixels.
    pub camera_height: f32,
    /// Camera shutter type: 0 = rolling, 1 = global.
    pub camera_shutter_type: i32,
    /// Camera frame rate (Hz).
    pub camera_fps: f32,
    /// Total time to read out one frame (seconds).
    ///
    /// This represents the effective **exposure time** for the rolling shutter model,
    /// assuming continuous readout with no gaps between rows.
    ///
    /// # Model Details
    ///
    /// In this simulation, each scanline (row) is captured instantaneously at a
    /// specific time during the frame interval. The time between successive scanline
    /// captures is constant and equal to:
    ///
    /// ```text
    /// camera_scanline_duration = camera_readout_duration / camera_height
    /// ```
    ///
    /// Because `camera_readout_duration` equals the frame exposure time (shutter open
    /// for the entire frame), this implies:
    /// - Exposure duty cycle = 1.0 (100% of frame time is exposure)
    /// - No dead time between row exposures
    /// - Each row is exposed for the full frame duration
    ///
    /// This is a simplified but valid model that produces clear rolling-shutter
    /// artifacts. For more complex scenarios (shorter exposure, gaps between rows),
    /// one could introduce additional `camera_frame_exposure_duty` and
    /// `camera_scanline_exposure_duty` parameters.
    ///
    /// # Relation to SPEC-NEW
    ///
    /// In SPEC-NEW terminology, this parameter corresponds to:
    /// `camera_frame_exposure_time` with an implicit `camera_frame_exposure_duty = 1.0`.
    /// The per-scanline time is then `camera_scanline_time` (called
    /// `camera_scanline_duration` here).
    pub camera_readout_duration: f32,
    /// Time per camera scanline (seconds).
    ///
    /// This is the temporal spacing between successive scanline captures during
    /// rolling-shutter exposure. Equivalent to SPEC-NEW's `camera_scanline_time`
    /// (line 135).
    ///
    /// In the simulation, this value is derived as:
    /// `camera_readout_duration / camera_height` when `camera_frame_exposure_duty = 1.0`.
    /// For a more general model with exposure duty < 1.0, this would be:
    /// `camera_frame_time * camera_frame_exposure_duty / camera_height`.
    ///
    /// # Shader Usage
    ///
    /// In `shader.rs`, this parameter computes the capture time for each scanline `j`:
    /// ```wgsl
    /// capture_time = camera_frame_start + f32(j) * camera_scanline_duration;
    /// ```
    pub camera_scanline_duration: f32,
    /// Camera start offset (seconds).
    ///
    /// Equivalent to SPEC-NEW's `camera_start_offset`. This offset is subtracted
    /// from the simulation time when computing the start of the current camera
    /// frame (see `shader.rs`, line 92).
    pub camera_start_offset: f32,
    /// Current simulation time (seconds).
    pub simulation_time: f32,
    /// Whether the simulation is paused: 0 = running, 1 = paused.
    pub paused: i32,
    /// Padding for std140 alignment.
    pub pad0: f32,
    /// Padding for std140 alignment.
    pub pad1: f32,
    /// Padding for std140 alignment.
    pub pad2: f32,
}

impl SimulationParams {
    /// Recomputes derived frequencies from the current parameters.
    ///
    /// Updates `display_freq_scanline` and `display_freq_frame` to be
    /// consistent with the pixel clock and active/blanking intervals.
    /// Also recomputes `camera_scanline_duration` from `camera_readout_duration`
    /// and `camera_height`.
    ///
    /// This should be called whenever `display_h_active`, `display_v_active`, `display_h_blank`,
    /// `display_v_blank`, `display_freq_pixel`, `camera_readout_duration`, or `camera_height` changes.
    pub fn compute_derived(&mut self) {
        let h_total = self.display_h_active + self.display_h_blank;
        let v_total = self.display_v_active + self.display_v_blank;
        
        if h_total > 0.0 && v_total > 0.0 && self.display_freq_pixel > 0.0 {
            // Derive from pixel clock (master)
            self.display_freq_scanline = self.display_freq_pixel / h_total;
            self.display_freq_frame = self.display_freq_scanline / v_total;

            // Debug assertion: verify SPEC-NEW invariant
            #[cfg(debug_assertions)]
            {
                let pixel_freq_check = self.display_freq_frame * v_total * h_total;
                let relative_error = (pixel_freq_check - self.display_freq_pixel).abs() / self.display_freq_pixel;
                assert!(
                    relative_error < 0.001,
                    "Frequency consistency check failed: relative_error = {:.6}",
                    relative_error
                );
            }
        } else {
            self.display_freq_scanline = 0.0;
            self.display_freq_frame = 0.0;
        }
        
        // Recompute camera scanline duration from readout duration and height.
        // This maintains the relationship: camera_scanline_duration = camera_readout_duration / camera_height
        // which corresponds to SPEC-NEW's camera_scanline_time = camera_frame_exposure_time / camera_height
        // under the assumption of 100% exposure duty (camera_frame_exposure_duty = 1.0).
        if self.camera_height > 0.0 {
            self.camera_scanline_duration = self.camera_readout_duration / self.camera_height;
        } else {
            self.camera_scanline_duration = 0.0;
        }
    }

    /// Returns a default parameter set for a Full HD display.
    ///
    /// Defaults correspond to a typical 1920×1080 display with 160/89
    /// blanking, 148.5 MHz pixel clock, 120 Hz PWM at 50% duty cycle,
    /// and a 30 fps rolling‑shutter camera.
    pub fn default_fullhd() -> Self {
        let h_active = 1920.0;
        let v_active = 1080.0;
        let h_blank = 160.0;
        let v_blank = 89.0;
        let pixel_freq = 148.5e6;
        let scanline_freq = pixel_freq / (h_active + h_blank);
        let frame_freq = scanline_freq / (v_active + v_blank);
        Self {
            display_h_active: h_active,
            display_v_active: v_active,
            display_h_blank: h_blank,
            display_v_blank: v_blank,
            display_freq_pixel: pixel_freq,
            display_freq_scanline: scanline_freq,
            display_freq_frame: frame_freq,
            display_emission_type: 0,
            display_pwm_freq: 120.0,
            display_pwm_duty: 0.5,
            display_start_offset: 0.0,
            camera_width: 3840.0,
            camera_height: 2160.0,
            camera_shutter_type: 0,
            camera_fps: 30.0,
            camera_readout_duration: 1.0 / 30.0,
            camera_scanline_duration: (1.0 / 30.0) / 2160.0,
            camera_start_offset: 0.0,
            simulation_time: 0.0,
            paused: 0,
            pad0: 0.0,
            pad1: 0.0,
            pad2: 0.0,
        }
    }

    /// Returns a default parameter set with all zeros.
    pub fn default() -> Self {
        Self::default_fullhd()
    }
}
