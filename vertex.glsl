#version 330 core
layout (location = 0) in vec3 vert;

void main() {
    gl_Position = vec4(vert.x, vert.y, vert.z, 1.0);
}