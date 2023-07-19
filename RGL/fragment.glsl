#version 430 core

out vec4 color;

in vec2 texturecoord;

uniform sampler2D RGL_texture;

void main() {
    // color = texture(RGL_texture, texturecoord);
    color = vec4(1,1,1,1);
}