#version 450

layout (location = 0) out vec4 outColor;

vec2 imageSize = vec2(1000.0, 1000.0);

void main() {
    vec2 uv = gl_FragCoord.xy / imageSize;
    outColor = vec4(uv, 0.0, 1.0);

}
