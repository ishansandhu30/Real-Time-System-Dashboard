#version 330 core

uniform vec4 u_color;
uniform float u_lineWidth;
uniform float u_antiAlias;

in vec2 v_position;
out vec4 fragColor;

void main() {
    // Apply the uniform color with optional anti-aliasing alpha modulation.
    // When anti-aliasing is enabled (u_antiAlias > 0), edge pixels are
    // softened by the built-in GL_LINE_SMOOTH. This shader simply passes
    // through the color; the smoothing is handled at the rasterizer level.
    fragColor = u_color;
}
