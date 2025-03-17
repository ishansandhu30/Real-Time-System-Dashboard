#version 330 core

layout(location = 0) in vec2 a_position;

uniform mat4 u_projection;
uniform mat4 u_modelView;

out vec2 v_position;

void main() {
    v_position = a_position;
    gl_Position = u_projection * u_modelView * vec4(a_position, 0.0, 1.0);
}
