#version 330 core

uniform vec4 u_color;
uniform float u_barValue;    // Normalized bar value (0.0 to 1.0)
uniform float u_maxValue;    // Maximum value for gradient computation

in vec2 v_position;
out vec4 fragColor;

void main() {
    // Render the bar with the uniform color.
    // A subtle vertical gradient can be applied by modulating alpha
    // based on the fragment's vertical position within the bar.
    // This gives bars a slight 3D depth effect.
    fragColor = u_color;
}
