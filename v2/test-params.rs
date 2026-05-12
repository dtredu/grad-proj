struct CameraParams {
    // dimensions
    pub width: u32,
    pub height: u32,

    // timing params
    pub frame_rate: f64,        // just the camera fps, defines the frame_time
    pub row_time: f64,          // time delta between start of each row exposure
    pub exposure_time: f64,     // how long each row is exposed to light
}

struct DisplayParams {
    // dimensions
    pub width: u32,
    pub height: u32,

    // timing params for PWM
    pub scanline_time: f64,     // time delta between start of each scanline (used only for PWM)
    pub pixel_time: f64,        // time delta between start of each pixel (used only for PWM)
    pub pwm_freq: f64,          // frequency of PWM pulse
    pub pwm_duty: f64,          // pwm duty fraction in 0.0f..=1.0f
}

struct SceneParams {
    pub distance: f64,          // distance between display and camera
    pub display_roll: f64,      // the display texture roll angle on XY plane (Z axis)
    pub camera_yaw: f64,        // camera yaw, that determines camera position angle in XZ plane (Y axis)
    pub camera_pitch: f64,      // camera pitch, that goes after `camera_yaw`, and determines camera
                                //   position angle in virtual YZ plane transformed by `camera_yaw`
    pub camera_fov: f64,        // camera fov angle (for width)
}

struct SimulationParams {
    camera: CameraParams,
    display: DisplayParams,
    scene: SceneParams,
}
