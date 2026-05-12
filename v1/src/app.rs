//! Top-level application state and main event loop.
//!
//! The [`App`] struct owns the window, renderer, UI state, camera, simulation
//! parameters, and controls the main event loop. It implements
//! [`winit::application::ApplicationHandler`] to handle window lifecycle and
//! events.
//!
//! ## Responsibilities
//!
//! - Window creation and management via `resumed()` and `suspended()`
//! - Event handling (keyboard, mouse, resize)
//! - Camera control (orbit, zoom, roll) with UI interaction guard
//! - UI panel rendering via egui
//! - Simulation time update and pause/resume
//! - Delegating frame rendering to [`Renderer`]

use egui_winit::State as EguiState;
use winit::{
    application::ApplicationHandler,
    dpi::PhysicalSize,
    event::{ElementState, MouseButton, WindowEvent},
    window::{Theme, Window},
};

use std::sync::Arc;
use web_time::Instant;

use crate::camera::Camera;
use crate::renderer::Renderer;
use crate::simulation_params::SimulationParams;

/// The main application state.
///
/// This struct is created once and lives for the duration of the application.
/// It holds all resources that need to persist across frames and window
/// lifecycle events.
pub struct App {
    /// Optional path to a texture to load at startup (set via `--texture`).
    texture_path: Option<String>,
    /// The main window, created on first `resumed()` call.
    window: Option<Arc<Window>>,
    /// The renderer, created after the window is available.
    renderer: Option<Renderer>,
    /// egui state for input handling and UI rendering.
    gui_state: Option<EguiState>,
    /// Instant of the last render frame, used for delta-time calculation.
    last_render_time: Option<Instant>,
    /// Last known window size, for resize tracking.
    last_size: (u32, u32),
    /// Whether the window system has been initialized.
    initialized: bool,
    /// Camera for viewing the 3D scene.
    camera: Camera,
    /// Last cursor position for orbit dragging.
    last_cursor_pos: Option<(f64, f64)>,
    /// Whether the left mouse button is held for rotating.
    is_rotating: bool,
    /// Mutable simulation parameters shown in the UI.
    sim_params: SimulationParams,
    /// Whether the simulation time is paused.
    sim_paused: bool,
    /// Whether camera resolution is locked to viewport size.
    camera_matches_viewport: bool,
}

impl Default for App {
    /// Returns an application with default camera and simulation parameters.
    fn default() -> Self {
        Self {
            texture_path: None,
            window: None,
            renderer: None,
            gui_state: None,
            last_render_time: None,
            last_size: (0, 0),
            initialized: false,
            camera: Camera::new(),
            last_cursor_pos: None,
            is_rotating: false,
            sim_params: SimulationParams::default_fullhd(),
            sim_paused: false,
            camera_matches_viewport: false,
        }
    }
}

impl App {
    /// Sets a texture file to be loaded once the renderer is ready.
    ///
    /// # Arguments
    ///
    /// * `path` - Path to a PNG file relative to the current directory.
    pub fn load_texture_at_startup(&mut self, path: &str) {
        self.texture_path = Some(path.to_string());
    }
}

impl ApplicationHandler for App {
    fn suspended(&mut self, _event_loop: &winit::event_loop::ActiveEventLoop) {
        self.renderer = None;
        self.window = None;
    }

    fn resumed(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        if self.window.is_some() {
            return;
        }

        let mut attributes = Window::default_attributes();
        attributes = attributes.with_title("3D Texture Viewer");

        let Ok(window) = event_loop.create_window(attributes) else {
            return;
        };

        let window_handle = Arc::new(window);
        self.window = Some(window_handle.clone());

        let gui_context = egui::Context::default();

        #[cfg(not(target_arch = "wasm32"))]
        {
            let inner_size = window_handle.inner_size();
            self.last_size = (inner_size.width, inner_size.height);
        }

        let viewport_id = gui_context.viewport_id();
        let gui_state = egui_winit::State::new(
            gui_context,
            viewport_id,
            &window_handle,
            Some(window_handle.scale_factor() as _),
            Some(Theme::Dark),
            None,
        );

        let (width, height) = (
            window_handle.inner_size().width,
            window_handle.inner_size().height,
        );

        #[cfg(all(not(target_arch = "wasm32"), not(target_os = "android")))]
        {
            if !self.initialized {
                env_logger::init();
            }
            let mut renderer = pollster::block_on(async move {
                Renderer::new(window_handle.clone(), width, height).await
            });
            if let Some(ref path) = self.texture_path {
                if let Ok(bytes) = std::fs::read(path) {
                    let _ = renderer.load_texture(&bytes);
                }
            }
            self.renderer = Some(renderer);
        }

        self.gui_state = Some(gui_state);
        self.last_render_time = Some(Instant::now());
        self.initialized = true;
    }

    fn window_event(
        &mut self,
        event_loop: &winit::event_loop::ActiveEventLoop,
        _window_id: winit::window::WindowId,
        event: WindowEvent,
    ) {
        let (Some(gui_state), Some(renderer), Some(window), Some(last_render_time)) = (
            self.gui_state.as_mut(),
            self.renderer.as_mut(),
            self.window.as_ref(),
            self.last_render_time.as_mut(),
        ) else {
            return;
        };

        match &event {
            WindowEvent::MouseInput {
                state,
                button: MouseButton::Left,
                ..
            } => {
                if *state == ElementState::Pressed {
                    self.is_rotating = true;
                    self.last_cursor_pos = None;
                } else {
                    self.is_rotating = false;
                }
            }
            WindowEvent::CursorMoved { position, .. } => {
                let ui_ctx = gui_state.egui_ctx();
                let is_ui_interacting = ui_ctx.is_pointer_over_area() || ui_ctx.is_using_pointer();
                if self.is_rotating && !is_ui_interacting {
                    if let Some(last_pos) = self.last_cursor_pos {
                        let delta_x = (position.x - last_pos.0) as f32;
                        let delta_y = (position.y - last_pos.1) as f32;
                        let sensitivity = 0.005;
                        self.camera.yaw -= delta_x * sensitivity;
                        self.camera.pitch += delta_y * sensitivity;
                        self.camera.pitch = self.camera.pitch.clamp(-1.5, 1.5);
                    }
                    self.last_cursor_pos = Some((position.x, position.y));
                } else {
                    self.last_cursor_pos = Some((position.x, position.y));
                }
            }
            _ => {}
        }

        if gui_state.on_window_event(window, &event).consumed {
            return;
        }

        match &event {
            WindowEvent::KeyboardInput {
                event:
                    winit::event::KeyEvent {
                        physical_key: winit::keyboard::PhysicalKey::Code(key_code),
                        state,
                        ..
                    },
                ..
            } => {
                if matches!(key_code, winit::keyboard::KeyCode::Escape) {
                    event_loop.exit();
                }
                if *state == ElementState::Pressed {
                    let step = 0.1;
                    match key_code {
                        winit::keyboard::KeyCode::KeyA | winit::keyboard::KeyCode::ArrowLeft => {
                            self.camera.yaw += step;
                        }
                        winit::keyboard::KeyCode::KeyD | winit::keyboard::KeyCode::ArrowRight => {
                            self.camera.yaw -= step;
                        }
                        winit::keyboard::KeyCode::KeyW | winit::keyboard::KeyCode::ArrowUp => {
                            self.camera.pitch = (self.camera.pitch - step).max(-1.5);
                        }
                        winit::keyboard::KeyCode::KeyS | winit::keyboard::KeyCode::ArrowDown => {
                            self.camera.pitch = (self.camera.pitch + step).min(1.5);
                        }
                        winit::keyboard::KeyCode::KeyQ => {
                            self.camera.distance = (self.camera.distance - 0.5).max(0.5);
                        }
                        winit::keyboard::KeyCode::KeyE => {
                            self.camera.distance = (self.camera.distance + 0.5).min(20.0);
                        }
                        winit::keyboard::KeyCode::Space => {
                            self.sim_paused = !self.sim_paused;
                        }
                        winit::keyboard::KeyCode::KeyZ => {
                            self.camera.roll -= 0.1;
                        }
                        winit::keyboard::KeyCode::KeyC => {
                            self.camera.roll += 0.1;
                        }
                        _ => {}
                    }
                }
            }
            WindowEvent::MouseWheel { delta, .. } => {
                let delta = match delta {
                    winit::event::MouseScrollDelta::LineDelta(_, y) => y,
                    winit::event::MouseScrollDelta::PixelDelta(_p) => &0.01_f32,
                };
                let d = self.camera.distance - delta * 0.5;
                self.camera.distance = if d < 0.5 { 0.5 } else if d > 20.0 { 20.0 } else { d };
            }
            WindowEvent::ScaleFactorChanged { .. } => {
                #[cfg(not(target_arch = "wasm32"))]
                {
                    let scale_factor = window.scale_factor() as f32;
                    gui_state.egui_ctx().set_pixels_per_point(scale_factor);
                }
            }
            WindowEvent::Resized(PhysicalSize { width, height }) => {
                let width = *width;
                let height = *height;
                if width == 0 || height == 0 {
                    return;
                }
                renderer.resize(width, height);
                self.last_size = (width, height);
                
                 // If camera resolution is locked to viewport, update it
                 if self.camera_matches_viewport {
                    self.sim_params.camera_width = width as f32;
                    self.sim_params.camera_height = height as f32;
                }
            }
            WindowEvent::CloseRequested => {
                event_loop.exit();
            }
            WindowEvent::RedrawRequested => {
                let now = Instant::now();
                let delta_time = now - *last_render_time;
                *last_render_time = now;

                if !self.sim_paused {
                    self.sim_params.simulation_time += delta_time.as_secs_f32();
                }
                self.sim_params.paused = self.sim_paused as i32;

                let gui_input = gui_state.take_egui_input(window);

                let egui_winit::egui::FullOutput {
                    textures_delta,
                    shapes,
                    pixels_per_point,
                    platform_output,
                    ..
                 } = gui_state.egui_ctx().run_ui(gui_input, |ui| {

                    egui::SidePanel::left("pwm_sim")
                        .default_width(280.0)
                        .show_inside(ui, |ui| {
                            ui.heading("PWM Simulation");
                            ui.separator();

                            ui.horizontal(|ui| {
                                if ui.button("Reset View").clicked() {
                                    self.camera.reset();
                                }
                                ui.label("Drag with left mouse to rotate (Z/C to rotate camera)");
                            });
                            ui.separator();

                            ui.horizontal(|ui| {
                                if ui.button(if self.sim_paused { "Resume" } else { "Pause" }).clicked() {
                                    self.sim_paused = !self.sim_paused;
                                }
                                ui.label(format!("Sim Time: {:.1}s", self.sim_params.simulation_time));
                                ui.separator();
                                ui.label(format!("Pitch: {:.1}°  Yaw: {:.1}°  Roll: {:.1}°",
                                    self.camera.pitch.to_degrees(),
                                    self.camera.yaw.to_degrees(),
                                    self.camera.roll.to_degrees()));
                            });
                            ui.separator();

                            ui.collapsing("Display", |ui| {
                                let h_active_changed = ui
                                    .add(egui::Slider::new(&mut self.sim_params.display_h_active, 1.0..=4096.0).text("H Active"))
                                    .changed();

                                let v_active_changed = ui
                                    .add(egui::Slider::new(&mut self.sim_params.display_v_active, 1.0..=2160.0).text("V Active"))
                                    .changed();

                                let h_blank_changed = ui
                                    .add(egui::Slider::new(&mut self.sim_params.display_h_blank, 0.0..=1000.0).text("H Blank"))
                                    .changed();

                                let v_blank_changed = ui
                                    .add(egui::Slider::new(&mut self.sim_params.display_v_blank, 0.0..=1000.0).text("V Blank"))
                                    .changed();

                                // Recompute derived frequencies when base parameters change
                                if h_active_changed || v_active_changed || h_blank_changed || v_blank_changed {
                                    self.sim_params.compute_derived();
                                }

                                ui.horizontal(|ui| {
                                    ui.radio_value(&mut self.sim_params.display_emission_type, 0, "Progressive");
                                    ui.radio_value(&mut self.sim_params.display_emission_type, 1, "Simultaneous");
                                });
                                ui.add(egui::Slider::new(&mut self.sim_params.display_pwm_freq, 30.0..=2000.0).text("PWM Freq"));
                                ui.add(egui::Slider::new(&mut self.sim_params.display_pwm_duty, 0.0..=1.0).text("PWM Duty"));
                                ui.add(egui::Slider::new(&mut self.sim_params.display_start_offset, 0.0..=1.0).text("Display Offset"));

                                ui.separator();
                                // Use stored derived values (now guaranteed correct)
                                ui.label(format!("Pixel Freq: {:.2} MHz", self.sim_params.display_freq_pixel / 1.0e6));
                                ui.label(format!("Scanline Freq: {:.2} kHz", self.sim_params.display_freq_scanline / 1.0e3));
                                ui.label(format!("Frame Freq: {:.2} Hz", self.sim_params.display_freq_frame));
                            });
                            ui.collapsing("Camera", |ui| {
                                ui.add(egui::Slider::new(&mut self.sim_params.camera_width, 1.0..=4096.0).text("Width"));
                                ui.add(egui::Slider::new(&mut self.sim_params.camera_height, 1.0..=4096.0).text("Height"));
                                ui.checkbox(&mut self.camera_matches_viewport, "Lock to Viewport");
                                ui.horizontal(|ui| {
                                    ui.radio_value(&mut self.sim_params.camera_shutter_type, 0, "Rolling");
                                    ui.radio_value(&mut self.sim_params.camera_shutter_type, 1, "Global");
                                });
                                ui.add(egui::Slider::new(&mut self.sim_params.camera_fps, 1.0..=1000.0).text("FPS"));
                                let _readout_changed = ui.add(egui::Slider::new(&mut self.sim_params.camera_readout_duration, 0.0..=0.5).text("Readout Duration")).changed();
                                
                                // Compute camera_scanline_duration from readout_duration and height (assuming 100% exposure duty)
                                // This ensures the rolling-shutter timing is physically consistent with the frame rate
                                self.sim_params.camera_scanline_duration = if self.sim_params.camera_height > 0.0 {
                                    self.sim_params.camera_readout_duration / self.sim_params.camera_height
                                } else {
                                    0.0
                                };
                                ui.label(format!("Scanline Duration: {:.6}s (derived)", self.sim_params.camera_scanline_duration));
                                ui.add(egui::Slider::new(&mut self.sim_params.camera_start_offset, 0.0..=1.0).text("Camera Offset"));
                            });

                            ui.separator();
                            ui.label("Controls: Space=Pause, Z/C=camera rotate");
                        });
                  });

                gui_state.handle_platform_output(window, platform_output);
                let paint_jobs = gui_state.egui_ctx().tessellate(shapes, pixels_per_point);

                let screen_descriptor = egui_wgpu::ScreenDescriptor {
                    size_in_pixels: [self.last_size.0, self.last_size.1],
                    pixels_per_point,
                };

                renderer.render_frame(
                    screen_descriptor,
                    paint_jobs,
                    textures_delta,
                    delta_time,
                    &self.camera,
                    self.sim_params,
                );
            }
            _ => (),
        }

        window.request_redraw();
    }
}
