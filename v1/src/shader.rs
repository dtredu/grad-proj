pub const SHADER_SOURCE: &str = "
struct Uniform {
    mvp: mat4x4<f32>,
};

struct SimulationParams {
    display_h_active: f32,
    display_v_active: f32,
    display_h_blank: f32,
    display_v_blank: f32,
    display_freq_pixel: f32,
    display_freq_scanline: f32,
    display_freq_frame: f32,
    display_emission_type: i32,
    display_pwm_freq: f32,
    display_pwm_duty: f32,
    display_start_offset: f32,
    camera_width: f32,
    camera_height: f32,
    camera_shutter_type: i32,
    camera_fps: f32,
    camera_readout_duration: f32,
    camera_scanline_duration: f32,
    camera_start_offset: f32,
    simulation_time: f32,
    paused: i32,
    pad0: f32,
    pad1: f32,
    pad2: f32,
};

@group(0) @binding(0)
var<uniform> ubo: Uniform;

@group(1) @binding(0)
var<uniform> sim_params: SimulationParams;

@group(2) @binding(1)
var texture_sampler: sampler;
@group(2) @binding(2)
var texture: texture_2d<f32>;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) tex_coords: vec2<f32>,
};
struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) tex_coords: vec2<f32>,
    @location(1) @interpolate(flat) vertex_index: u32,
    @location(2) clip_xy: vec2<f32>,
    @location(3) clip_w: f32,
};

@vertex
fn vertex_main(vert: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = ubo.mvp * vec4<f32>(vert.position, 1.0);
    out.tex_coords = vert.tex_coords;
    out.clip_xy = out.position.xy;
    out.clip_w = out.position.w;
    return out;
};

fn ndc_y_to_viewport_y(ndc_y: f32) -> f32 {
    return 0.5 - 0.5 * ndc_y;
}

fn compute_pwm_brightness(display_time: f32, row: f32, emission_type: i32) -> f32 {
    let pwm_period = 1.0 / sim_params.display_pwm_freq;

    let scanline_freq = sim_params.display_freq_pixel / (sim_params.display_h_active + sim_params.display_h_blank);
    let row_offset = select(0.0, row * (1.0 / scanline_freq), emission_type == 0);

    let display_time_for_row = display_time + row_offset;
    let phase = display_time_for_row * sim_params.display_pwm_freq;
    let phase_fract = phase - floor(phase);

    return select(1.0, 0.0, phase_fract < sim_params.display_pwm_duty);
}

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let base_color = textureSample(texture, texture_sampler, in.tex_coords);

    let display_row = in.tex_coords.y * sim_params.display_v_active;

    let ndc_y = in.clip_xy.y / in.clip_w;
    let viewport_y = ndc_y_to_viewport_y(ndc_y);

    let t = sim_params.simulation_time;
    let camera_frame_start = floor((t - sim_params.camera_start_offset) * sim_params.camera_fps) / sim_params.camera_fps;

    let camera_scanline_j = viewport_y * sim_params.camera_height;

    let capture_time = select(
        camera_frame_start,
        camera_frame_start + camera_scanline_j * sim_params.camera_scanline_duration,
        sim_params.camera_shutter_type == 0
    );

    let display_elapsed = capture_time - sim_params.display_start_offset;

    let brightness = compute_pwm_brightness(display_elapsed, display_row, sim_params.display_emission_type);

    return vec4<f32>(base_color.rgb * brightness, base_color.a);
}
";
