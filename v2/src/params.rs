use nalgebra_glm::Vec3;

#[derive(Clone, Copy)]
pub struct CameraParams {
    pub width: u32,
    pub height: u32,
    pub frame_rate: f64,
    pub row_time: f64,
    pub exposure_time: f64,
}

impl Default for CameraParams {
    fn default() -> Self {
        Self::new()
    }
}

impl CameraParams {
    pub fn new() -> Self {
        Self {
            width: 1280,
            height: 720,
            frame_rate: 60.0,
            row_time: 1e-5,
            exposure_time: 0.01,
        }
    }

    pub fn reset(&mut self) {
        *self = Self::new();
    }
}

#[derive(Clone, Copy)]
pub struct DisplayParams {
    pub width: u32,
    pub height: u32,
    pub scanline_time: f64,
    pub pixel_time: f64,
    pub pwm_freq: f64,
    pub pwm_duty: f64,
}

impl Default for DisplayParams {
    fn default() -> Self {
        Self::new()
    }
}

impl DisplayParams {
    pub fn new() -> Self {
        Self {
            width: 1280,
            height: 720,
            scanline_time: 1e-5,
            pixel_time: 1e-6,
            pwm_freq: 1000.0,
            pwm_duty: 0.5,
        }
    }

    pub fn reset(&mut self) {
        *self = Self::new();
    }
}

#[derive(Clone, Copy)]
pub struct SceneParams {
    pub distance: f64,
    pub display_roll: f64,
    pub camera_yaw: f64,
    pub camera_pitch: f64,
    pub camera_fov: f64,
}

impl Default for SceneParams {
    fn default() -> Self {
        Self::new()
    }
}

impl SceneParams {
    pub fn new() -> Self {
        Self {
            distance: 2.5,
            display_roll: 0.0,
            camera_yaw: 0.0,
            camera_pitch: 0.0,
            camera_fov: 45.0,
        }
    }

    pub fn reset(&mut self) {
        self.distance = 2.5;
        self.display_roll = 0.0;
        self.camera_yaw = 0.0;
        self.camera_pitch = 0.0;
        self.camera_fov = 45.0;
    }

    pub fn view_matrix(&self) -> nalgebra_glm::Mat4 {
        let eye = self.eye();
        nalgebra_glm::look_at_lh(&eye, &Vec3::zeros(), &Vec3::y())
    }

    pub fn model_rotation(&self) -> nalgebra_glm::Mat4 {
        nalgebra_glm::rotate_z(&nalgebra_glm::Mat4::identity(), self.display_roll as f32)
    }

    pub fn eye(&self) -> nalgebra_glm::Vec3 {
        let clamped_pitch = self.camera_pitch.clamp(-1.5, 1.5) as f32;
        let yaw = self.camera_yaw as f32;
        let dist = self.distance as f32;
        nalgebra_glm::vec3(
            dist * clamped_pitch.cos() * yaw.sin(),
            dist * clamped_pitch.sin(),
            dist * clamped_pitch.cos() * yaw.cos(),
        )
    }
}