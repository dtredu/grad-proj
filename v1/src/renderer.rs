//! Render pipeline management and frame rendering.
//!
//! The [`Renderer`] struct coordinates the WGPU surface, depth texture, egui
//! integration, and the 3D [`Scene`] to produce a complete frame.
//!
//! ## Rendering Steps
//!
//! Each frame performs the following operations:
//!
//! 1. Update egui with the current input and run the UI.
//! 2. Tessellate the egui shapes into paint jobs.
//! 3. Begin a render pass with depth and color attachments.
//! 4. Update the scene uniforms (MVP matrix, simulation parameters).
//! 5. Render the 3D scene (quad) via [`Scene::render()`].
//! 6. Render the egui paint jobs on top.
//! 7. Submit the command encoder and present the surface texture.

use crate::camera::Camera;
use crate::gpu::Gpu;
use crate::scene::Scene;
use crate::simulation_params::SimulationParams;

/// Encapsulates the full rendering pipeline, including egui rendering.
pub struct Renderer {
    gpu: Gpu,
    depth_texture_view: wgpu::TextureView,
    egui_renderer: egui_wgpu::Renderer,
    scene: Scene,
}

impl Renderer {
    /// The depth format used for the depth-stencil attachment.
    pub const DEPTH_FORMAT: wgpu::TextureFormat = wgpu::TextureFormat::Depth32Float;

    /// Creates a new renderer for the given window.
    ///
    /// # Arguments
    ///
    /// * `window` - The window target for the swap chain.
    /// * `width` - Initial swap chain width.
    /// * `height` - Initial swap chain height.
    ///
    /// # Returns
    ///
    /// A fully initialized [`Renderer`].
    pub async fn new(
        window: impl Into<wgpu::SurfaceTarget<'static>>,
        width: u32,
        height: u32,
    ) -> Self {
        let gpu = Gpu::new_async(window, width, height).await;
        let depth_texture_view = gpu.create_depth_texture(width, height);

        let egui_renderer = egui_wgpu::Renderer::new(
            &gpu.device,
            gpu.surface_config.format,
            egui_wgpu::RendererOptions {
                depth_stencil_format: Some(Self::DEPTH_FORMAT),
                msaa_samples: 1,
                ..Default::default()
            },
        );

        let scene = Scene::new(&gpu.device, &gpu.queue, gpu.surface_format);

        Self {
            gpu,
            depth_texture_view,
            egui_renderer,
            scene,
        }
    }

    /// Resizes the swap chain and depth texture to new dimensions.
    ///
    /// # Arguments
    ///
    /// * `width` - New width in pixels.
    /// * `height` - New height in pixels.
    pub fn resize(&mut self, width: u32, height: u32) {
        self.gpu.resize(width, height);
        self.depth_texture_view = self.gpu.create_depth_texture(width, height);
    }

    /// Loads a PNG texture from bytes and replaces the current texture.
    ///
    /// # Arguments
    ///
    /// * `png_bytes` - Raw PNG file data.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success, or an error string on failure.
    pub fn load_texture(&mut self, png_bytes: &[u8]) -> Result<(), String> {
        let img = image::load_from_memory(png_bytes)
            .map_err(|e| format!("Failed to load image: {}", e))?;
        let rgba = img.to_rgba8();
        let (width, height) = rgba.dimensions();
        let pixels = rgba.into_raw();

        let texture = self.gpu.device.create_texture(&wgpu::TextureDescriptor {
            label: Some("Texture"),
            size: wgpu::Extent3d {
                width,
                height,
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8UnormSrgb,
            usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
            view_formats: &[],
        });

        self.gpu.queue.write_texture(
            wgpu::TexelCopyTextureInfo {
                texture: &texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            &pixels,
            wgpu::TexelCopyBufferLayout {
                offset: 0,
                bytes_per_row: Some(4 * width),
                rows_per_image: Some(height),
            },
            wgpu::Extent3d {
                width,
                height,
                depth_or_array_layers: 1,
            },
        );

        let texture_view = texture.create_view(&wgpu::TextureViewDescriptor::default());
        let sampler = self.gpu.device.create_sampler(&wgpu::SamplerDescriptor {
            address_mode_u: wgpu::AddressMode::ClampToEdge,
            address_mode_v: wgpu::AddressMode::ClampToEdge,
            address_mode_w: wgpu::AddressMode::ClampToEdge,
            mag_filter: wgpu::FilterMode::Linear,
            min_filter: wgpu::FilterMode::Linear,
            mipmap_filter: wgpu::MipmapFilterMode::Linear,
            ..Default::default()
        });

        self.scene
            .update_texture(&self.gpu.device, texture_view, sampler);
        Ok(())
    }

    /// Renders a complete frame.
    ///
    /// This method performs the full UI and 3D rendering pipeline as described
    /// in the module-level documentation.
    ///
    /// # Arguments
    ///
    /// * `screen_descriptor` - egui screen descriptor for the current window.
    /// * `paint_jobs` - Tessellated egui shapes.
    /// * `textures_delta` - egui texture updates.
    /// * `_delta_time` - Time since the last frame (unused but kept for
    ///   forward compatibility).
    /// * `camera` - Current camera state.
    /// * `sim_params` - Current simulation parameters.
    pub fn render_frame(
        &mut self,
        screen_descriptor: egui_wgpu::ScreenDescriptor,
        paint_jobs: Vec<egui::epaint::ClippedPrimitive>,
        textures_delta: egui::TexturesDelta,
        _delta_time: web_time::Duration,
        camera: &Camera,
        sim_params: SimulationParams,
    ) {
        for (id, image_delta) in &textures_delta.set {
            self.egui_renderer
                .update_texture(&self.gpu.device, &self.gpu.queue, *id, image_delta);
        }

        for id in &textures_delta.free {
            self.egui_renderer.free_texture(id);
        }

        self.scene
            .sim_params
            .update_sim_params(&self.gpu.queue, sim_params);
        self.scene
            .update(&self.gpu.queue, self.gpu.aspect_ratio(), camera, sim_params);

        let mut encoder = self
            .gpu
            .device
            .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                label: Some("Render Encoder"),
            });

        self.egui_renderer.update_buffers(
            &self.gpu.device,
            &self.gpu.queue,
            &mut encoder,
            &paint_jobs,
            &screen_descriptor,
        );

        let surface_texture = match self.gpu.surface.get_current_texture() {
            wgpu::CurrentSurfaceTexture::Success(frame) => frame,
            wgpu::CurrentSurfaceTexture::Suboptimal(frame) => frame,
            wgpu::CurrentSurfaceTexture::Outdated => {
                self.gpu
                    .surface
                    .configure(&self.gpu.device, &self.gpu.surface_config);
                match self.gpu.surface.get_current_texture() {
                    wgpu::CurrentSurfaceTexture::Success(frame) => frame,
                    wgpu::CurrentSurfaceTexture::Suboptimal(frame) => frame,
                    other => {
                        panic!("Failed to get surface texture after reconfiguration: {other:?}")
                    }
                }
            }
            other => panic!("Failed to get surface texture: {other:?}"),
        };

        let surface_texture_view = surface_texture
            .texture
            .create_view(&wgpu::TextureViewDescriptor {
                label: wgpu::Label::default(),
                aspect: wgpu::TextureAspect::default(),
                format: Some(self.gpu.surface_format),
                dimension: None,
                base_mip_level: 0,
                mip_level_count: None,
                base_array_layer: 0,
                array_layer_count: None,
                usage: None,
            });

        encoder.insert_debug_marker("Render scene");

        {
            let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("Render Pass"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &surface_texture_view,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color {
                            r: 0.1,
                            g: 0.1,
                            b: 0.1,
                            a: 1.0,
                        }),
                        store: wgpu::StoreOp::Store,
                    },
                    depth_slice: None,
                })],
                depth_stencil_attachment: Some(wgpu::RenderPassDepthStencilAttachment {
                    view: &self.depth_texture_view,
                    depth_ops: Some(wgpu::Operations {
                        load: wgpu::LoadOp::Clear(1.0),
                        store: wgpu::StoreOp::Store,
                    }),
                    stencil_ops: None,
                }),
                timestamp_writes: None,
                occlusion_query_set: None,
                multiview_mask: None,
            });
            self.scene.render(&mut render_pass);

            self.egui_renderer.render(
                &mut render_pass.forget_lifetime(),
                &paint_jobs,
                &screen_descriptor,
            );
        }

        self.gpu.queue.submit(std::iter::once(encoder.finish()));
        surface_texture.present();
    }
}


