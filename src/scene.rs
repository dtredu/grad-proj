//! 3D scene containing the display quad, rendering pipeline, and billboarding.
//!
//! The [`Scene`] struct holds all GPU resources needed to render the display:
//! vertex and index buffers, uniform buffers, bind groups, and the render
//! pipeline. It also performs the billboarding rotation each frame so that the
//! display quad always faces the camera.
//!
//! ## Billboarding
//!
//! The display quad is rotated so its normal points at the camera while its
//! up direction aligns with the camera's up (taking roll into account). This
//! eliminates perspective foreshortening and keeps display rows horizontal on
//! screen, making artifact patterns easier to analyze.
//!
//! See [`docs/camera.md`](docs/camera.md) for the mathematical details.

use crate::camera::Camera;
use crate::shader::SHADER_SOURCE;
use crate::simulation_params::SimulationParams;
use crate::uniform::UniformBinding;
use crate::vertex::{Vertex, INDICES, VERTICES};

/// Default white 1×1 texture used before a custom texture is loaded.
const DEFAULT_TEXTURE_DATA: [u8; 4] = [255, 255, 255, 255];

/// A textured quad representing the display, together with its render pipeline
/// and associated GPU resources.
pub struct Scene {
    /// Vertex buffer holding the four corners of the display quad.
    pub vertex_buffer: wgpu::Buffer,
    /// Index buffer for rendering the quad as two triangles.
    pub index_buffer: wgpu::Buffer,
    /// Uniform binding for the model‑view‑projection matrix.
    pub uniform: UniformBinding,
    /// Uniform binding for simulation parameters.
    pub sim_params: UniformBinding,
    /// The render pipeline that draws the textured quad.
    pub pipeline: wgpu::RenderPipeline,
    /// Optional bind group for the texture (or its sampler).
    ///
    /// Contains the default white texture until [`update_texture()`] is
    /// called with a new image.
    pub texture_bind_group: Option<wgpu::BindGroup>,
    /// Bind group layout used for texture binding.
    ///
    /// This is needed when updating the texture to create a new bind group.
    pub bind_group_layout_for_texture: wgpu::BindGroupLayout,
}
impl Scene {
    /// Reusable epsilon for singularity detection when computing billboard basis.
    const SINGULARITY_EPSILON: f32 = 1e-6;

    ///
    /// # Arguments
    ///
    /// * `device` - The WGPU device used to create GPU resources.
    /// * `queue` - The WGPU queue used for buffer initialization.
    /// * `surface_format` - The swap‑chain texture format.
    ///
    /// # Returns
    ///
    /// A fully initialized [`Scene`].
    pub fn new(
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        surface_format: wgpu::TextureFormat,
    ) -> Self {
        let vertex_buffer = wgpu::util::DeviceExt::create_buffer_init(
            device,
            &wgpu::util::BufferInitDescriptor {
                label: Some("Vertex Buffer"),
                contents: bytemuck::cast_slice(&VERTICES),
                usage: wgpu::BufferUsages::VERTEX,
            },
        );
        let index_buffer = wgpu::util::DeviceExt::create_buffer_init(
            device,
            &wgpu::util::BufferInitDescriptor {
                label: Some("Index Buffer"),
                contents: bytemuck::cast_slice(&INDICES),
                usage: wgpu::BufferUsages::INDEX,
            },
        );
        let uniform = UniformBinding::new(device);
        let sim_params = UniformBinding::new_sim_params(device, queue);

        let bind_group_layout_for_texture =
            device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
                entries: &[
                    wgpu::BindGroupLayoutEntry {
                        binding: 1,
                        visibility: wgpu::ShaderStages::FRAGMENT,
                        ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                        count: None,
                    },
                    wgpu::BindGroupLayoutEntry {
                        binding: 2,
                        visibility: wgpu::ShaderStages::FRAGMENT,
                        ty: wgpu::BindingType::Texture {
                            sample_type: wgpu::TextureSampleType::Float { filterable: true },
                            view_dimension: wgpu::TextureViewDimension::D2,
                            multisampled: false,
                        },
                        count: None,
                    },
                ],
                label: Some("texture_bind_group_layout"),
            });

        let pipeline = Self::create_pipeline(
            device,
            surface_format,
            &uniform,
            &sim_params.bind_group_layout,
            &bind_group_layout_for_texture,
        );

        let default_texture = device.create_texture(&wgpu::TextureDescriptor {
            label: Some("Default Texture"),
            size: wgpu::Extent3d {
                width: 1,
                height: 1,
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8UnormSrgb,
            usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
            view_formats: &[],
        });
        queue.write_texture(
            wgpu::TexelCopyTextureInfo {
                texture: &default_texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            &DEFAULT_TEXTURE_DATA,
            wgpu::TexelCopyBufferLayout {
                offset: 0,
                bytes_per_row: Some(4),
                rows_per_image: Some(1),
            },
            wgpu::Extent3d {
                width: 1,
                height: 1,
                depth_or_array_layers: 1,
            },
        );
        let default_texture_view =
            default_texture.create_view(&wgpu::TextureViewDescriptor::default());
        let default_sampler = device.create_sampler(&wgpu::SamplerDescriptor {
            address_mode_u: wgpu::AddressMode::ClampToEdge,
            address_mode_v: wgpu::AddressMode::ClampToEdge,
            address_mode_w: wgpu::AddressMode::ClampToEdge,
            mag_filter: wgpu::FilterMode::Linear,
            min_filter: wgpu::FilterMode::Linear,
            mipmap_filter: wgpu::MipmapFilterMode::Linear,
            ..Default::default()
        });

        let texture_bind_group = Some(device.create_bind_group(&wgpu::BindGroupDescriptor {
            layout: &bind_group_layout_for_texture,
            entries: &[
                wgpu::BindGroupEntry {
                    binding: 1,
                    resource: wgpu::BindingResource::Sampler(&default_sampler),
                },
                wgpu::BindGroupEntry {
                    binding: 2,
                    resource: wgpu::BindingResource::TextureView(&default_texture_view),
                },
            ],
            label: Some("texture_bind_group"),
        }));

        Self {
            uniform,
            sim_params,
            pipeline,
            vertex_buffer,
            index_buffer,
            texture_bind_group,
            bind_group_layout_for_texture,
        }
    }

    /// Replaces the current texture with a new one.
    ///
    /// This also creates a new sampler (linear filtering, clamp‑to‑edge).
    ///
    /// # Arguments
    ///
    /// * `device` - The WGPU device used to create the texture resources.
    /// * `texture_view` - View of the new texture.
    /// * `sampler` - Sampler for the texture.
    pub fn update_texture(
        &mut self,
        device: &wgpu::Device,
        texture_view: wgpu::TextureView,
        sampler: wgpu::Sampler,
    ) {
        self.texture_bind_group = Some(device.create_bind_group(&wgpu::BindGroupDescriptor {
            layout: &self.bind_group_layout_for_texture,
            entries: &[
                wgpu::BindGroupEntry {
                    binding: 1,
                    resource: wgpu::BindingResource::Sampler(&sampler),
                },
                wgpu::BindGroupEntry {
                    binding: 2,
                    resource: wgpu::BindingResource::TextureView(&texture_view),
                },
            ],
            label: Some("texture_bind_group"),
        }));
    }

    /// Draws the display quad using the given render pass.
    ///
    /// This sets the pipeline, bind groups, vertex/index buffers, and issues
    /// the draw call. The caller is responsible for configuring the render
    /// pass (color/depth attachments, etc.).
    ///
    /// # Arguments
    ///
    /// * `renderpass` - A render pass that has already begun.
    pub fn render<'rpass>(&'rpass self, renderpass: &mut wgpu::RenderPass<'rpass>) {
        renderpass.set_pipeline(&self.pipeline);
        renderpass.set_bind_group(0, &self.uniform.bind_group, &[]);
        renderpass.set_bind_group(1, &self.sim_params.bind_group, &[]);
        if let Some(ref texture_bind_group) = self.texture_bind_group {
            renderpass.set_bind_group(2, texture_bind_group, &[]);
        }

        renderpass.set_vertex_buffer(0, self.vertex_buffer.slice(..));
        renderpass.set_index_buffer(self.index_buffer.slice(..), wgpu::IndexFormat::Uint32);

        renderpass.draw_indexed(0..(INDICES.len() as _), 0, 0..INDICES.len() as u32);
    }

    /// Updates the MVP uniform and simulation parameters.
    ///
    /// This should be called once per frame before rendering. It computes the
    /// billboard rotation matrix so the display always faces the camera.
    ///
    /// # Arguments
    ///
    /// * `queue` - The WGPU queue used to write uniform buffers.
    /// * `aspect_ratio` - Current viewport aspect ratio (width / height).
    /// * `camera` - The camera used for view/billboard calculations.
    /// * `sim_params` - Current simulation parameters.
    pub fn update(
        &mut self,
        queue: &wgpu::Queue,
        aspect_ratio: f32,
        camera: &Camera,
        sim_params: SimulationParams,
    ) {
        let projection =
            nalgebra_glm::perspective_lh_zo(aspect_ratio, 45_f32.to_radians(), 0.1, 100.0);
        let view = camera.view_matrix();

        // Billboard rotation: make the quad always face the camera while keeping
        // its up aligned with camera's up (including roll).
        let eye = camera.eye_position();
        let forward = nalgebra_glm::normalize(&eye);
        let camera_up = nalgebra_glm::rotate_z_vec3(&nalgebra_glm::Vec3::y(), camera.roll);

        // Compute right vector, handling the singularity when forward is parallel
        // to camera_up.
        let mut right = nalgebra_glm::cross(&camera_up, &forward);
        let right_len = right.norm();
        if right_len < Self::SINGULARITY_EPSILON {
            // forward is nearly parallel to camera_up, use fallback
            right = nalgebra_glm::cross(&nalgebra_glm::Vec3::x(), &forward);
            let right_len2 = right.norm();
            if right_len2 < Self::SINGULARITY_EPSILON {
                right = nalgebra_glm::cross(&nalgebra_glm::Vec3::y(), &forward);
            }
            right = nalgebra_glm::normalize(&right);
        } else {
            right = nalgebra_glm::normalize(&right);
        }

        let up = nalgebra_glm::normalize(&nalgebra_glm::cross(&forward, &right));

        // Build rotation matrix with columns: right, up, forward
        let model = nalgebra_glm::Mat4::new(
            right.x, right.y, right.z, 0.0,
            up.x, up.y, up.z, 0.0,
            forward.x, forward.y, forward.z, 0.0,
            0.0, 0.0, 0.0, 1.0,
        );

        self.uniform.update_buffer(
            queue,
            0,
            crate::vertex::UniformBuffer {
                mvp: projection * view * model,
            },
        );
        self.sim_params.update_sim_params(queue, sim_params);
    }

    /// Creates the render pipeline.
    ///
    /// This is a private helper that configures the vertex/fragment stages,
    /// bind group layouts, depth testing, and primitive state.
    ///
    /// # Arguments
    ///
    /// * `device` - The WGPU device.
    /// * `surface_format` - The swap‑chain texture format.
    /// * `uniform` - The MVP uniform binding (for layout).
    /// * `sim_params_bind_group_layout` - Layout for simulation params.
    /// * `texture_bind_group_layout` - Layout for texture/sampler.
    ///
    /// # Returns
    ///
    /// A configured [`wgpu::RenderPipeline`].
    fn create_pipeline(
        device: &wgpu::Device,
        surface_format: wgpu::TextureFormat,
        uniform: &UniformBinding,
        sim_params_bind_group_layout: &wgpu::BindGroupLayout,
        texture_bind_group_layout: &wgpu::BindGroupLayout,
    ) -> wgpu::RenderPipeline {
        let shader_module = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: None,
            source: wgpu::ShaderSource::Wgsl(std::borrow::Cow::Borrowed(SHADER_SOURCE)),
        });

        let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: None,
            bind_group_layouts: &[
                Some(&uniform.bind_group_layout),
                Some(sim_params_bind_group_layout),
                Some(texture_bind_group_layout),
            ],
            immediate_size: 0,
        });

        device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: None,
            layout: Some(&pipeline_layout),
            vertex: wgpu::VertexState {
                module: &shader_module,
                entry_point: Some("vertex_main"),
                buffers: &[Vertex::description(&Vertex::vertex_attributes())],
                compilation_options: Default::default(),
            },
            primitive: wgpu::PrimitiveState {
                topology: wgpu::PrimitiveTopology::TriangleList,
                strip_index_format: None,
                front_face: wgpu::FrontFace::Cw,
                cull_mode: None,
                polygon_mode: wgpu::PolygonMode::Fill,
                conservative: false,
                unclipped_depth: false,
            },
            depth_stencil: Some(wgpu::DepthStencilState {
                format: crate::renderer::Renderer::DEPTH_FORMAT,
                depth_write_enabled: Some(true),
                depth_compare: Some(wgpu::CompareFunction::Less),
                stencil: wgpu::StencilState::default(),
                bias: wgpu::DepthBiasState::default(),
            }),
            multisample: wgpu::MultisampleState {
                count: 1,
                mask: !0,
                alpha_to_coverage_enabled: false,
            },
            fragment: Some(wgpu::FragmentState {
                module: &shader_module,
                entry_point: Some("fragment_main"),
                targets: &[Some(wgpu::ColorTargetState {
                    format: surface_format,
                    blend: Some(wgpu::BlendState::ALPHA_BLENDING),
                    write_mask: wgpu::ColorWrites::ALL,
                })],
                compilation_options: Default::default(),
            }),
            multiview_mask: None,
            cache: None,
        })
    }
}
