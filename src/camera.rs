//! Camera module
//!
//! This module provides the camera implementation for the 3D simulation viewer.
//! The camera uses spherical coordinates (yaw, pitch, distance) plus a roll angle
//! to allow intuitive navigation around the display quad. It also includes
//! billboarding logic that keeps the display facing the camera while respecting
//! camera roll, ensuring that display rows appear horizontal on screen.
//!
//! ## Coordinate System
//!
//! - **Eye position**: Computed from spherical coordinates in world space
//! - **View matrix**: Built using `look_at_lh` with a roll-adjusted up vector
//! - **Billboard rotation**: Each frame, the display quad is rotated so its normal
//!   points at the camera, eliminating perspective foreshortening
//!
//! ## References
//!
//! See [`docs/camera.md`](docs/camera.md) for detailed mathematical description.
//!
//! [`Camera`] is used by [`App`](crate::app::App) for view matrix computation and
//! by [`Scene`](crate::scene::Scene) for billboarding.

use nalgebra_glm::{Vec3, Mat4, look_at_lh, rotate_z_vec3};

/// The camera defining the viewer's position and orientation in world space.
///
/// The camera uses spherical coordinates:
/// - **Yaw** (θ): Rotation around the world Y‑axis (horizontal view direction).
/// - **Pitch** (φ): Rotation around the world X‑axis (vertical, clamped to
///   `[-1.5, 1.5]` radians to avoid gimbal lock).
/// - **Distance** (r): Radial distance from the display center (world origin).
/// - **Roll** (ψ): Rotation around the camera's view axis (Z‑axis in view space).
///
/// # Examples
///
/// ```
/// use pwm_simulation::camera::Camera;
///
/// let mut camera = Camera::new();
/// camera.yaw = 0.5;
/// camera.pitch = 0.2;
/// camera.distance = 3.0;
/// let view_matrix = camera.view_matrix();
/// ```
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Camera {
    /// Horizontal rotation around the world Y‑axis (radians).
    ///
    /// A yaw of 0 points along the -Z axis (into the screen). Increasing yaw
    /// rotates counter‑clockwise when viewed from above.
    pub yaw: f32,
    /// Vertical rotation around the world X‑axis (radians).
    ///
    /// Positive pitch points the camera upward. Clamped to `[-1.5, 1.5]` to
    /// avoid gimbal lock (looking straight up or down).
    pub pitch: f32,
    /// Distance from the display center (world origin).
    ///
    /// Must be positive. Controls zoom level.
    pub distance: f32,
    /// Roll angle around the camera's view axis (radians).
    ///
    /// Tilts the camera about its line of sight. This affects both the view
    /// matrix (via the up vector) and the billboard orientation of the display
    /// quad, so that the display texture rolls along with the camera.
    pub roll: f32,
}

impl Default for Camera {
    /// Returns a default camera positioned to point straight at the display
    /// (along the negative Z‑axis).
    fn default() -> Self {
        Self::new()
    }
}

impl Camera {
    /// Creates a new camera with default position and orientation.
    ///
    /// # Returns
    ///
    /// A [`Camera`] instance positioned to point straight at the display origin
    /// (along the negative Z‑axis).
    pub fn new() -> Self {
        Self {
            yaw: std::f32::consts::FRAC_PI_2, // 90°, -Z axis for straight-on view
            pitch: 0.0,
            distance: 2.5,
            roll: 0.0,
        }
    }

    /// Resets the camera to its default position and orientation.
    ///
    /// This is useful for returning to a known view after interactive
    /// navigation. The default view points straight at the display origin
    /// (along the negative Z‑axis).
    pub fn reset(&mut self) {
        self.yaw = std::f32::consts::FRAC_PI_2; // 90°, -Z axis for straight-on view
        self.pitch = 0.0;
        self.distance = 2.5;
        self.roll = 0.0;
    }

    /// Computes the eye (camera) position in world coordinates.
    ///
    /// Uses spherical‑to‑Cartesian conversion:
    ///
    /// ```text
    /// eye_x = r * cos(φ) * cos(θ)
    /// eye_y = r * sin(φ)
    /// eye_z = r * cos(φ) * sin(θ)
    /// ```
    ///
    /// where `r` is `distance`, `φ` is `pitch`, and `θ` is `yaw`.
    ///
    /// # Returns
    ///
    /// A 3‑dimensional vector representing the camera's position in world space.
    pub fn eye_position(&self) -> Vec3 {
        let pitch = self.pitch.clamp(-1.5, 1.5);
        Vec3::new(
            self.distance * pitch.cos() * self.yaw.cos(),
            self.distance * pitch.sin(),
            self.distance * pitch.cos() * self.yaw.sin(),
        )
    }

    /// Builds the view matrix for the camera.
    ///
    /// The view matrix is constructed using `look_at_lh` with:
    /// - **Eye**: The position returned by [`eye_position()`](Camera::eye_position).
    /// - **Target**: The world origin (center of the display).
    /// - **Up**: The world Y‑axis rotated by `roll` around the Z‑axis. This
    ///   ensures that rolling the camera tilts the entire view while keeping
    ///   the display centered.
    ///
    /// # Returns
    ///
    /// A 4×4 left‑handed view matrix.
    ///
    /// # See Also
    ///
    /// [`eye_position()`](Camera::eye_position) for the camera position computation.
    pub fn view_matrix(&self) -> Mat4 {
        let pitch = self.pitch.clamp(-1.5, 1.5);
        let eye = Vec3::new(
            self.distance * pitch.cos() * self.yaw.cos(),
            self.distance * pitch.sin(),
            self.distance * pitch.cos() * self.yaw.sin(),
        );
        let up = rotate_z_vec3(&Vec3::y(), self.roll);
        look_at_lh(&eye, &Vec3::zeros(), &up)
    }
}
