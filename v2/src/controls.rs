use winit::event::{ElementState, MouseButton, WindowEvent};

use crate::params::SceneParams;

#[derive(Default)]
pub struct Controls {
    lmb_is_rotating: bool,
    lmb_last_cursor_pos: Option<(f64, f64)>,
    rmb_is_rotating: bool,
    rmb_last_cursor_pos: Option<(f64, f64)>,
}

impl Controls {
    pub fn new() -> Self {
        Self {
            lmb_is_rotating: false,
            lmb_last_cursor_pos: None,
            rmb_is_rotating: false,
            rmb_last_cursor_pos: None,
        }
    }

    /// Process a window event. Returns `true` if the application should exit.
    pub fn process_event(
        &mut self,
        event: &WindowEvent,
        params: &mut SceneParams,
        egui_visible: &mut bool,
    ) -> bool {
        match event {
            WindowEvent::MouseInput { state, button, .. } => match button {
                MouseButton::Left => {
                    if *state == ElementState::Pressed {
                        self.lmb_is_rotating = true;
                        self.lmb_last_cursor_pos = None;
                    } else {
                        self.lmb_is_rotating = false;
                    }
                    false
                }
                MouseButton::Right => {
                    if *state == ElementState::Pressed {
                        self.rmb_is_rotating = true;
                        self.rmb_last_cursor_pos = None;
                    } else {
                        self.rmb_is_rotating = false;
                    }
                    false
                }
                _ => false,
            },
            winit::event::WindowEvent::CursorMoved { position, .. } => {
                if self.lmb_is_rotating {
                    if let Some(last_pos) = self.lmb_last_cursor_pos {
                        let delta_x = (position.x - last_pos.0) as f64;
                        let delta_y = (position.y - last_pos.1) as f64;
                        let sensitivity = 0.005;
                        params.camera_yaw += delta_x * sensitivity;
                        params.camera_pitch =
                            (params.camera_pitch + delta_y * sensitivity).clamp(-1.5, 1.5);
                    }
                    self.lmb_last_cursor_pos = Some((position.x, position.y));
                } else {
                    self.lmb_last_cursor_pos = Some((position.x, position.y));
                }

                if self.rmb_is_rotating {
                    if let Some(last_pos) = self.rmb_last_cursor_pos {
                        let delta_x = (position.x - last_pos.0) as f64;
                        let sensitivity = 0.01;
                        params.display_roll += delta_x * sensitivity;
                    }
                    self.rmb_last_cursor_pos = Some((position.x, position.y));
                } else {
                    self.rmb_last_cursor_pos = Some((position.x, position.y));
                }

                false
            }
            winit::event::WindowEvent::KeyboardInput {
                event:
                    winit::event::KeyEvent {
                        physical_key: winit::keyboard::PhysicalKey::Code(key_code),
                        state,
                        ..
                    },
                ..
            } => {
                if *state == winit::event::ElementState::Pressed {
                    let step = 0.1;
                    match key_code {
                        winit::keyboard::KeyCode::KeyG => {
                            *egui_visible = !*egui_visible;
                        }
                        winit::keyboard::KeyCode::KeyA | winit::keyboard::KeyCode::ArrowLeft => {
                            params.camera_yaw -= step;
                        }
                        winit::keyboard::KeyCode::KeyD | winit::keyboard::KeyCode::ArrowRight => {
                            params.camera_yaw += step;
                        }
                        winit::keyboard::KeyCode::KeyW | winit::keyboard::KeyCode::ArrowUp => {
                            params.camera_pitch = (params.camera_pitch - step).max(-1.5);
                        }
                        winit::keyboard::KeyCode::KeyS | winit::keyboard::KeyCode::ArrowDown => {
                            params.camera_pitch = (params.camera_pitch + step).min(1.5);
                        }
                        winit::keyboard::KeyCode::KeyC => {
                            params.distance = (params.distance - 0.5).max(0.5);
                        }
                        winit::keyboard::KeyCode::KeyZ => {
                            params.distance = (params.distance + 0.5).min(20.0);
                        }
                        winit::keyboard::KeyCode::KeyQ => {
                            params.display_roll -= step;
                        }
                        winit::keyboard::KeyCode::KeyE => {
                            params.display_roll += step;
                        }
                        _ => {}
                    }
                }
                false
            }
            winit::event::WindowEvent::MouseWheel { delta, .. } => {
                let delta = match delta {
                    winit::event::MouseScrollDelta::LineDelta(_, y) => *y as f64,
                    winit::event::MouseScrollDelta::PixelDelta(p) => p.y as f64 * 0.01,
                };
                params.distance = (params.distance - delta * 0.5).max(0.5).min(20.0);
                false
            }
            _ => false,
        }
    }
}
