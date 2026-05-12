//! GPU uniform buffer management.
//!
//! This module provides [`UniformBinding`], a helper for creating and updating
//! uniform buffers used in the WGPU render pipeline. It encapsulates the buffer,
//! bind group, and bind group layout needed to expose data to shaders.

use wgpu::util::DeviceExt;

use crate::vertex::UniformBuffer;

/// A uniform buffer together with its bind group and layout.
///
/// This structure manages a GPU buffer that can be updated with new data each
/// frame, and provides the bind group used in the render pipeline to make the
/// buffer accessible to shaders.
pub struct UniformBinding {
    /// The GPU buffer holding the uniform data.
    pub buffer: wgpu::Buffer,
    /// The bind group used to bind this buffer in the render pipeline.
    pub bind_group: wgpu::BindGroup,
    /// The bind group layout that describes the buffer binding.
    pub bind_group_layout: wgpu::BindGroupLayout,
}

impl UniformBinding {
    /// Creates a new uniform binding for the MVP (model‑view‑projection) matrix.
    ///
    /// The buffer is initialized with an identity matrix and is usable in the
    /// vertex shader as a uniform buffer (group 0, binding 0).
    ///
    /// # Arguments
    ///
    /// * `device` - The WGPU device used to create GPU resources.
    pub fn new(device: &wgpu::Device) -> Self {
        let buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
            label: Some("Uniform Buffer"),
            contents: bytemuck::cast_slice(&[UniformBuffer::default()]),
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
        });

        let bind_group_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            entries: &[wgpu::BindGroupLayoutEntry {
                binding: 0,
                visibility: wgpu::ShaderStages::VERTEX,
                ty: wgpu::BindingType::Buffer {
                    ty: wgpu::BufferBindingType::Uniform,
                    has_dynamic_offset: false,
                    min_binding_size: None,
                },
                count: None,
            }],
            label: Some("uniform_bind_group_layout"),
        });

        let bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            layout: &bind_group_layout,
            entries: &[wgpu::BindGroupEntry {
                binding: 0,
                resource: buffer.as_entire_binding(),
            }],
            label: Some("uniform_bind_group"),
        });

        Self {
            buffer,
            bind_group,
            bind_group_layout,
        }
    }

    /// Updates the uniform buffer with new data.
    ///
    /// # Arguments
    ///
    /// * `queue` - The WGPU queue used to submit the write command.
    /// * `offset` - Byte offset within the buffer to write to (typically 0).
    /// * `uniform_buffer` - The new uniform data to upload.
    pub fn update_buffer(
        &mut self,
        queue: &wgpu::Queue,
        offset: wgpu::BufferAddress,
        uniform_buffer: UniformBuffer,
    ) {
        queue.write_buffer(
            &self.buffer,
            offset,
            bytemuck::cast_slice(&[uniform_buffer]),
        )
    }

    /// Creates a uniform binding for simulation parameters.
    ///
    /// This is similar to [`new()`](UniformBinding::new) but intended for the
    /// fragment shader's simulation parameters (group 1, binding 0). The
    /// buffer is initialized with default simulation parameters.
    ///
    /// # Arguments
    ///
    /// * `device` - The WGPU device used to create GPU resources.
    /// * `_queue` - The WGPU queue (currently unused but kept for symmetry).
    pub fn new_sim_params(device: &wgpu::Device, _queue: &wgpu::Queue) -> Self {
        use crate::simulation_params::SimulationParams;

        let buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
            label: Some("Sim Params Uniform Buffer"),
            contents: bytemuck::cast_slice(&[SimulationParams::default()]),
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
        });

        let bind_group_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            entries: &[wgpu::BindGroupLayoutEntry {
                binding: 0,
                visibility: wgpu::ShaderStages::FRAGMENT,
                ty: wgpu::BindingType::Buffer {
                    ty: wgpu::BufferBindingType::Uniform,
                    has_dynamic_offset: false,
                    min_binding_size: None,
                },
                count: None,
            }],
            label: Some("sim_params_bind_group_layout"),
        });

        let bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            layout: &bind_group_layout,
            entries: &[wgpu::BindGroupEntry {
                binding: 0,
                resource: buffer.as_entire_binding(),
            }],
            label: Some("sim_params_bind_group"),
        });

        Self {
            buffer,
            bind_group,
            bind_group_layout,
        }
    }

    /// Updates the simulation parameters buffer.
    ///
    /// # Arguments
    ///
    /// * `queue` - The WGPU queue used to submit the write command.
    /// * `sim_params` - The simulation parameters to upload.
    pub fn update_sim_params(
        &mut self,
        queue: &wgpu::Queue,
        sim_params: crate::simulation_params::SimulationParams,
    ) {
        queue.write_buffer(
            &self.buffer,
            0,
            bytemuck::cast_slice(&[sim_params]),
        )
    }
}
