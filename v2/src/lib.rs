mod controls;
mod gpu;
mod params;
mod renderer;
mod scene;
mod shader;

use std::sync::Arc;
use web_time::Instant;
use winit::{
    application::ApplicationHandler,
    dpi::PhysicalSize,
    event::WindowEvent,
    window::{Theme, Window},
};

pub use params::CameraParams;
pub use params::DisplayParams;
pub use controls::Controls;
pub use params::SceneParams;
pub use gpu::Gpu;
pub use renderer::Renderer;
pub use scene::{UniformBinding, UniformBuffer, Vertex};
pub use shader::SHADER_SOURCE;

#[derive(Default)]
pub struct App {
    texture_path: Option<String>,
    window: Option<Arc<Window>>,
    renderer: Option<Renderer>,
    gui_state: Option<egui_winit::State>,
    last_render_time: Option<Instant>,
    last_size: (u32, u32),
    initialized: bool,
    camera_params: CameraParams,
    display_params: DisplayParams,
    params: SceneParams,
    controls: Controls,
    egui_visible: bool,
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

        self.camera_params = CameraParams::new();
        self.display_params = DisplayParams::new();
        self.gui_state = Some(gui_state);
        self.last_render_time = Some(Instant::now());
        self.initialized = true;
        self.egui_visible = true;
    }

    fn window_event(
        &mut self,
        event_loop: &winit::event_loop::ActiveEventLoop,
        _window_id: winit::window::WindowId,
        event: winit::event::WindowEvent,
    ) {
        let (Some(gui_state), Some(renderer), Some(window), Some(last_render_time)) = (
            self.gui_state.as_mut(),
            self.renderer.as_mut(),
            self.window.as_ref(),
            self.last_render_time.as_mut(),
        ) else {
            return;
        };

        if self.controls.process_event(&event, &mut self.params, &mut self.egui_visible) {
            event_loop.exit();
            return;
        }

        if gui_state.on_window_event(window, &event).consumed {
            return;
        }

        match event {
            WindowEvent::ScaleFactorChanged { .. } => {
                #[cfg(not(target_arch = "wasm32"))]
                {
                    let scale_factor = window.scale_factor() as f32;
                    gui_state.egui_ctx().set_pixels_per_point(scale_factor);
                }
            }
            WindowEvent::Resized(PhysicalSize { width, height }) => {
                if width == 0 || height == 0 {
                    return;
                }
                renderer.resize(width, height);
                self.last_size = (width, height);
            }
            WindowEvent::CloseRequested => {
                event_loop.exit();
            }
            WindowEvent::RedrawRequested => {
                let now = Instant::now();
                let delta_time = now - *last_render_time;
                *last_render_time = now;

                let gui_input = gui_state.take_egui_input(window);

                let egui_winit::egui::FullOutput {
                    textures_delta,
                    shapes,
                    pixels_per_point,
                    platform_output,
                    ..
                } = gui_state.egui_ctx().run_ui(gui_input, |ui| {
                    if self.egui_visible {
                        egui::Panel::left("controls").show_inside(ui, |ui| {
                            ui.heading("Controls");
                            ui.separator();

                            // Camera Params
                            egui::CollapsingHeader::new("Camera Params").show(ui, |ui| {
                                ui.add(egui::DragValue::new(&mut self.camera_params.width).speed(1.0).prefix("Width: "));
                                ui.add(egui::DragValue::new(&mut self.camera_params.height).speed(1.0).prefix("Height: "));
                                ui.add(egui::DragValue::new(&mut self.camera_params.frame_rate).speed(1.0).prefix("Frame Rate: "));
                                ui.add(egui::DragValue::new(&mut self.camera_params.row_time).speed(1e-6).prefix("Row Time: "));
                                ui.add(egui::DragValue::new(&mut self.camera_params.exposure_time).speed(1e-4).prefix("Exposure: "));
                            });

                            // Display Params
                            egui::CollapsingHeader::new("Display Params").show(ui, |ui| {
                                ui.add(egui::DragValue::new(&mut self.display_params.width).speed(1.0).prefix("Width: "));
                                ui.add(egui::DragValue::new(&mut self.display_params.height).speed(1.0).prefix("Height: "));
                                ui.add(egui::DragValue::new(&mut self.display_params.scanline_time).speed(1e-6).prefix("Scanline Time: "));
                                ui.add(egui::DragValue::new(&mut self.display_params.pixel_time).speed(1e-7).prefix("Pixel Time: "));
                                ui.add(egui::DragValue::new(&mut self.display_params.pwm_freq).speed(1.0).prefix("PWM Freq: "));
                                ui.add(egui::DragValue::new(&mut self.display_params.pwm_duty).speed(0.01).prefix("PWM Duty: "));
                            });

                            // Scene Params
                            egui::CollapsingHeader::new("Scene Params").show(ui, |ui| {
                                ui.add(egui::DragValue::new(&mut self.params.camera_yaw).speed(0.01).prefix("Camera Yaw: "));
                                ui.add(egui::DragValue::new(&mut self.params.camera_pitch).speed(0.01).prefix("Camera Pitch: "));
                                ui.add(egui::DragValue::new(&mut self.params.distance).speed(0.1).prefix("Distance: "));
                                ui.add(egui::DragValue::new(&mut self.params.display_roll).speed(0.01).prefix("Display Roll: "));
                                ui.add(egui::DragValue::new(&mut self.params.camera_fov).speed(0.1).prefix("Camera FOV: "));
                            });

                            ui.separator();
                            if ui.button("Reset All").clicked() {
                                self.camera_params.reset();
                                self.display_params.reset();
                                self.params.reset();
                            }
                            ui.label("Drag with left mouse to rotate");
                            ui.label("RMB drag: adjust display roll");
                            ui.separator();
                            let eye = self.params.eye();
                            ui.label(format!("Eye: ({:.2}, {:.2}, {:.2})", eye.x, eye.y, eye.z));
                        });
                    }
                });

                gui_state.handle_platform_output(window, platform_output);
                let paint_jobs = gui_state.egui_ctx().tessellate(shapes, pixels_per_point);

                let screen_descriptor = {
                    let (width, height) = self.last_size;
                    if width == 0 || height == 0 {
                        return;
                    }
                    egui_wgpu::ScreenDescriptor {
                        size_in_pixels: [width, height],
                        pixels_per_point,
                    }
                };

                renderer.render_frame(
                    screen_descriptor,
                    paint_jobs,
                    textures_delta,
                    delta_time,
                    &self.params,
                );
            }
            _ => (),
        }

        window.request_redraw();
    }
}

impl App {
    pub fn load_texture_at_startup(&mut self, path: &str) {
        self.texture_path = Some(path.to_string());
    }
}