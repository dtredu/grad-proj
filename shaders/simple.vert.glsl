#version 450

vec2 positions[6] = vec2[] (
    vec2( 0.0, -0.75),
    vec2( 0.5,  0.75),
    vec2(-0.5,  0.75),
    vec2( 0.5,  0.75),
    vec2(-0.5,  0.75),
    vec2( 0.0,  0.9)
);

void main() {
  gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
