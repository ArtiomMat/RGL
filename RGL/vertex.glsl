// This is the basic RGL vertex shader, you can copy anything from it for your own shader needs.
// These layout things

#version 400 core

layout (location = 0) in vec3 vert;

void main() {
    gl_Position = vec4(vert.x, vert.y, vert.z, 1.0);
}
