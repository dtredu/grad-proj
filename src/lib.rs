//! PWM Progressive Emission + Rolling Shutter Simulation
//!
//! This library simulates the visual artifacts produced by PWM (Pulse Width
//! Modulation) progressive emission on AMOLED displays when captured by a camera
//! with rolling or global shutter. It demonstrates how different combinations
//! of display emission types and camera shutter types can produce characteristic
//! patterns such as flicker, banding, and other artifacts.
//!
//! ## Architecture
//!
//! The simulation is divided into the following modules:
//!
//! - [`app`]: Top-level application state and main event loop
//! - [`camera`]: Camera controls and view matrix computation
//! - [`gpu`]: GPU device, surface, and swap chain management
//! - [`vertex`]: Vertex definitions and uniform buffer types
//! - [`simulation_params`]: Timing parameters for display and camera
//! - [`shader`]: WGSL shader source code
//! - [`scene`]: 3D scene with billboarding and render pipeline
//! - [`uniform`]: GPU uniform buffer management
//! - [`renderer`]: Render pipeline coordination and frame rendering
//!
//! ## Usage
//!
//! For a native desktop application, see `src/main.rs`.
//! For web/WASM builds, this library can be used as a starting point.

pub mod app;
pub mod camera;
pub mod gpu;
pub mod renderer;
pub mod scene;
pub mod shader;
pub mod simulation_params;
pub mod uniform;
pub mod vertex;

pub use app::App;
pub use camera::Camera;
pub use gpu::Gpu;
pub use renderer::Renderer;
pub use scene::Scene;
pub use shader::SHADER_SOURCE;
pub use simulation_params::SimulationParams;
pub use uniform::UniformBinding;
pub use vertex::{Vertex, UniformBuffer, VERTICES, INDICES};
