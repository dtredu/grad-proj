extern crate app_core;

use app_core::app::App;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = std::env::args().collect();
    let texture_path = args.get(1).cloned();

    let event_loop = winit::event_loop::EventLoop::builder().build()?;
    event_loop.set_control_flow(winit::event_loop::ControlFlow::Poll);
    let mut application = App::default();
    if let Some(path) = texture_path {
        application.load_texture_at_startup(&path);
    }
    event_loop.run_app(&mut application)?;
    Ok(())
}
