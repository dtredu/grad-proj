# Refactoring Plan: 3D PNG Texture Viewer Engine

## Overview
Transform the existing wgpu cross-platform demonstration into a focused 3D graphics engine for viewing PNG image textures from different angles and rotations.

## Current State Analysis

### Existing Features
- wgpu 29 + winit 0.30 + egui 0.34 stack
- Basic triangle rendering with colored vertices
- Static camera with auto-rotation animation
- egui overlay UI with File/Edit menus
- Unused cube vertex/index data

### Limitations
- No texture support (vertex colors only)
- No user camera control
- No image loading capability
- Overly complex for a texture viewer

---

## Phase 1: Dependencies

### 1.1 Add Required Crates
```toml
image = "0.25"  # PNG loading and image manipulation
```

### 1.2 Verify Existing Dependencies
- `bytemuck` - for vertex buffer casting ✓
- `nalgebra-glm` - for matrix math ✓
- `wgpu` - for GPU operations ✓

---

## Phase 2: Texture Loading System

### 2.1 Create TextureLoader Module
- Implement asynchronous texture loading from PNG files
- Convert PNG data to `wgpu::Texture` with proper format
- Handle texture format conversion (RGBA8UnormSrgb)
- Create sampler with LINEAR filtering for smooth viewing

### 2.2 Add Texture Loading Method to Renderer
```rust
pub fn load_texture(&mut self, png_bytes: &[u8]) -> Result<(), Error>
```

---

## Phase 3: Vertex Format Update

### 3.1 Modify Vertex Struct
```rust
#[repr(C)]
#[derive(Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Vertex {
    position: [f32; 3],  // x, y, z
    tex_coords: [f32; 2], // u, v
}
```

### 3.2 Create Quad Geometry
- 4 vertices forming a plane in XY (Z=0)
- Triangle strip: 0, 1, 2, 2, 3, 0
- UV coordinates: (0,0) bottom-left to (1,1) top-right

### 3.3 Update Vertex Attributes
- Location 0: position (Float32x3)
- Location 1: tex_coords (Float32x2)

---

## Phase 4: Shader Updates

### 4.1 Update Vertex Shader
```wgsl
struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) tex_coords: vec2<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) tex_coords: vec2<f32>,
};
```

### 4.2 Update Fragment Shader
```wgsl
@group(0) @binding(1)
var texture_sampler: sampler;
@group(0) @binding(2)
var texture: texture_2d<f32>;

@fragment
fn fragment_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return textureSample(texture, texture_sampler, in.tex_coords);
}
```

### 4.3 Update Bind Group Layout
- Binding 0: Uniform (MVP matrix) - VERTEX
- Binding 1: Sampler - FRAGMENT
- Binding 2: Texture - FRAGMENT

---

## Phase 5: Camera System

### 5.1 Camera State
```rust
pub struct Camera {
    pub yaw: f32,       // horizontal rotation
    pub pitch: f32,     // vertical rotation
    pub distance: f32,  // zoom distance
    pub target: Vec3,   // look at point
}
```

### 5.2 Mouse Controls
- Left drag: rotate yaw/pitch
- Scroll wheel: adjust distance
- Right drag: pan target (optional)

### 5.3 View Matrix Calculation
```rust
let eye = calculate_eye_position(yaw, pitch, distance);
let view = look_at_lh(eye, target, up);
```

### 5.4 Event Handling Updates
- `WindowEvent::MouseMotion` - track drag rotation
- `WindowEvent::MouseWheel` - adjust zoom
- Mouse button press/release tracking

---

## Phase 6: UI Simplification

### 6.1 Replace Menu Bar
- Remove File/Edit menus
- Add simple control panel:
  - Load Texture button (file dialog)
  - Reset View button
  - Instructions text

### 6.2 File Dialog Integration
- Use `rfd` or similar crate for file selection
- Filter for PNG files only
- Load selected file into texture

---

## Phase 7: Cleanup and Optimization

### 7.1 Remove Unused Code
- Delete `CUBE_VERTICES` and `CUBE_INDICES`
- Delete `GREEN_CUBE_VERTICES`
- Remove `sky.wgsl` and `grid.wgsl` references
- Remove unused GUI panels (Scene Tree, Inspector, Console)

### 7.2 Simplify App State
- Remove `gui_state` if not needed for texture loading
- Keep minimal egui for file dialog and controls

### 7.3 Update Window Title
- Change to "3D Texture Viewer"

---

## Implementation Order

1. **Add `image` crate to Cargo.toml**
2. **Create quad geometry with UVs**
3. **Update vertex shader for texture coords**
4. **Add texture/sampler bindings to shader**
5. **Create texture loading system**
6. **Implement camera controls**
7. **Add file loading UI**
8. **Clean up unused code**
9. **Test with sample PNG**

---

## File Changes Summary

| File | Action |
|------|--------|
| `Cargo.toml` | Add `image` and `rfd` dependencies |
| `src/lib.rs` | Major refactoring |
| `src/main.rs` | Minor title update |
| `src/sky.wgsl` | Deleted (unused) |
| `src/grid.wgsl` | Deleted (unused) |

---

## Testing Criteria

- [x] PNG image loads and displays on quad
- [x] Mouse drag rotates view smoothly
- [x] Scroll wheel zooms in/out
- [x] Load button opens file dialog
- [x] Reset button restores default view
- [x] No console errors or warnings

## Status: COMPLETED