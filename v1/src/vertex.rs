//! Vertex and uniform buffer types for the 3D rendering pipeline.
//!
//! This module defines the data structures used for vertex attributes and
//! uniform buffers that are passed to the GPU. All types in this module
//! implement the necessary traits for GPU upload via `bytemuck`.

use nalgebra_glm::Mat4;

/// A single vertex with position and texture coordinates.
///
/// This struct is used to define the corners of the display quad. The
/// quad is rendered as two triangles covering the entire viewport.
#[repr(C)]
#[derive(Copy, Clone, Debug, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Vertex {
    /// 3D position of the vertex in model space.
    pub position: [f32; 3],
    /// 2D texture coordinates (U, V).
    ///
    /// The V coordinate is oriented such that 0.0 is at the top of the
    /// texture and 1.0 is at the bottom.
    pub tex_coords: [f32; 2],
}

impl Vertex {
    /// Returns the WGSL vertex attribute descriptions for this vertex type.
    ///
    /// This is used to configure the render pipeline's vertex buffer layout.
    ///
    /// # Returns
    ///
    /// A vector of [`wgpu::VertexAttribute`] describing the location and
    /// format of each vertex attribute.
    pub fn vertex_attributes() -> Vec<wgpu::VertexAttribute> {
        wgpu::vertex_attr_array![0 => Float32x3, 1 => Float32x2].to_vec()
    }

    /// Creates a vertex buffer layout descriptor for use in pipeline creation.
    ///
    /// # Arguments
    ///
    /// * `attributes` - The slice of vertex attribute descriptions, typically
    ///   obtained from [`vertex_attributes()`](Vertex::vertex_attributes).
    ///
    /// # Returns
    ///
    /// A [`wgpu::VertexBufferLayout`] that can be used in
    /// [`wgpu::RenderPipelineDescriptor`].
    pub fn description(attributes: &[wgpu::VertexAttribute]) -> wgpu::VertexBufferLayout<'_> {
        wgpu::VertexBufferLayout {
            array_stride: std::mem::size_of::<Vertex>() as wgpu::BufferAddress,
            step_mode: wgpu::VertexStepMode::Vertex,
            attributes,
        }
    }
}

/// The vertex buffer data for the full-screen display quad.
///
/// The quad covers the normalized device coordinate space from (-1, -1) to
/// (1, 1). Texture coordinates are arranged so that the V axis points
/// downward (top = 0.0, bottom = 1.0) to match typical image conventions.
pub const VERTICES: [Vertex; 4] = [
    Vertex {
        position: [-1.0, -1.0, 0.0],
        tex_coords: [0.0, 1.0],
    },
    Vertex {
        position: [1.0, -1.0, 0.0],
        tex_coords: [1.0, 1.0],
    },
    Vertex {
        position: [1.0, 1.0, 0.0],
        tex_coords: [1.0, 0.0],
    },
    Vertex {
        position: [-1.0, 1.0, 0.0],
        tex_coords: [0.0, 0.0],
    },
];

/// Indices for rendering the display quad as two triangles.
pub const INDICES: [u32; 6] = [0, 1, 2, 2, 3, 0];

/// Uniform buffer for the model‑view‑projection matrix.
///
/// This is uploaded to the vertex shader's `ubo` binding (group 0, binding 0)
/// each frame to transform the display quad.
#[repr(C)]
#[derive(Copy, Clone, Debug, bytemuck::Pod, bytemuck::Zeroable)]
pub struct UniformBuffer {
    /// The combined MVP matrix.
    pub mvp: Mat4,
}

impl Default for UniformBuffer {
    /// Returns an identity MVP matrix.
    fn default() -> Self {
        Self {
            mvp: Mat4::identity(),
        }
    }
}
