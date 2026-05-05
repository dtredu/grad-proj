//! GPU device, surface, and swap chain management.
//!
//! This module encapsulates WGPU initialization and configuration, including
//! the surface, adapter, device, queue, and surface configuration. It also
//! provides utilities for resizing the swap chain and creating depth textures.
//!
//! The [`Gpu`] struct holds all GPU resources that need to survive across
//! frames but may change when the window is resized.

use wgpu::InstanceDescriptor;

/// The main GPU abstraction, wrapping the surface, device, and configuration.
///
/// This struct is created once during application startup and is recreated
/// (or resized) when the window dimensions change.
pub struct Gpu {
    /// The WGPU surface used for presenting frames.
    pub surface: wgpu::Surface<'static>,
    /// The logical GPU device.
    pub device: wgpu::Device,
    /// The queue used for submitting GPU commands.
    pub queue: wgpu::Queue,
    /// The surface configuration (size, format, present mode, etc.).
    pub surface_config: wgpu::SurfaceConfiguration,
    /// The surface texture format used for rendering.
    pub surface_format: wgpu::TextureFormat,
}

impl Gpu {
    /// Computes the current aspect ratio of the swap chain.
    ///
    /// Returns `width / height`, clamping height to 1 to avoid division by zero.
    pub fn aspect_ratio(&self) -> f32 {
        self.surface_config.width as f32 / self.surface_config.height.max(1) as f32
    }

    /// Resizes the surface to the given dimensions.
    ///
    /// This updates the internal configuration and reconfigures the surface
    /// with the new size. It should be called in response to window resize
    /// events.
    ///
    /// # Arguments
    ///
    /// * `width` - New width in pixels.
    /// * `height` - New height in pixels.
    pub fn resize(&mut self, width: u32, height: u32) {
        self.surface_config.width = width;
        self.surface_config.height = height;
        self.surface.configure(&self.device, &self.surface_config);
    }

    /// Creates a depth texture view for use as a depth-stencil attachment.
    ///
    /// The depth texture matches the current surface dimensions and uses the
    /// `Depth32Float` format.
    ///
    /// # Arguments
    ///
    /// * `width` - Width of the depth texture.
    /// * `height` - Height of the depth texture.
    ///
    /// # Returns
    ///
    /// A [`wgpu::TextureView`] that can be used as the `depth_stencil_attachment`
    /// in a render pass.
    pub fn create_depth_texture(&self, width: u32, height: u32) -> wgpu::TextureView {
        let texture = self.device.create_texture(&wgpu::TextureDescriptor {
            label: Some("Depth Texture"),
            size: wgpu::Extent3d {
                width,
                height,
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Depth32Float,
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::TEXTURE_BINDING,
            view_formats: &[],
        });
        texture.create_view(&wgpu::TextureViewDescriptor {
            label: None,
            format: Some(wgpu::TextureFormat::Depth32Float),
            dimension: Some(wgpu::TextureViewDimension::D2),
            aspect: wgpu::TextureAspect::All,
            base_mip_level: 0,
            base_array_layer: 0,
            array_layer_count: None,
            mip_level_count: None,
            usage: None,
        })
    }

    /// Asynchronously creates a new GPU instance from a window target.
    ///
    /// This is the main entry point for GPU initialization. It requests an
    /// adapter, creates a device and queue, configures the surface, and
    /// returns a fully initialized [`Gpu`].
    ///
    /// # Arguments
    ///
    /// * `window` - A window target that can be converted into a surface.
    /// * `width` - Initial swap chain width.
    /// * `height` - Initial swap chain height.
    ///
    /// # Returns
    ///
    /// A [`Gpu`] instance ready for rendering.
    pub async fn new_async(
        window: impl Into<wgpu::SurfaceTarget<'static>>,
        width: u32,
        height: u32,
    ) -> Self {
        let instance = wgpu::Instance::new(InstanceDescriptor::new_without_display_handle_from_env());
        let surface = instance.create_surface(window).unwrap();

        let adapter = instance.request_adapter(&wgpu::RequestAdapterOptions {
                power_preference: wgpu::PowerPreference::default(),
                compatible_surface: Some(&surface),
                force_fallback_adapter: false,
            })
            .await
            .expect("Failed to request adapter!");

        let (device, queue) = adapter.request_device(
                &wgpu::DeviceDescriptor {
                    label: Some("WGPU Device"),
                    required_features: wgpu::Features::default(),
                    required_limits: wgpu::Limits::default().using_resolution(adapter.limits()),
                    memory_hints: wgpu::MemoryHints::default(),
                    experimental_features: wgpu::ExperimentalFeatures::disabled(),
                    trace: wgpu::Trace::Off,
                },
            )
            .await
            .expect("Failed to request a device!");

        let surface_capabilities = surface.get_capabilities(&adapter);
        let surface_format = surface_capabilities.formats.iter().copied().find(|f| !f.is_srgb()).unwrap_or(surface_capabilities.formats[0]);

        let surface_config = wgpu::SurfaceConfiguration {
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
            format: surface_format,
            width,
            height,
            present_mode: surface_capabilities.present_modes[0],
            alpha_mode: surface_capabilities.alpha_modes[0],
            view_formats: vec![],
            desired_maximum_frame_latency: 2,
        };
        surface.configure(&device, &surface_config);

        Self {
            surface,
            device,
            queue,
            surface_config,
            surface_format,
        }
    }
}
