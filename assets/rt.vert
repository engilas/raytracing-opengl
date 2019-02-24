#version 330

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex;

uniform mat4 matrix;

out vec2 texcoord;

void main() {
    texcoord = tex;
    gl_Position = matrix * vec4(pos,1.0);
}
