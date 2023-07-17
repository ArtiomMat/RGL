// This is the basic RGL vertex shader, you can copy anything from it for your own shader needs.
// These layout things

#version 400 core

layout (location = 0) in vec3 vert;
layout (location = 1) in vec2 i_texturevert;

uniform vec3 RGL_offset;
uniform vec3 RGL_angles;

// Btw, fov does not matter in this algorithm, because it is part of d_max
uniform float RGL_p_near;
uniform float RGL_p_far;
uniform float RGL_d_max; // Precaulculated, because i mean, it's constant. is tan(fov/2)*p_near

out vec2 texturecoord;

// TODO: Use ratio to avoid stretching!
// h being either x or y, depending on which d we are getting.
float getd(float h) {
  return h*RGL_p_near/(vert.z+RGL_offset.z);
}

// There are two d_max so it's a parameter
float normalize_d(float d, float d_max) {
  return d/(2*d_max);
}

void main() {
  texturecoord = i_texturevert; // For the fragment shader

  float dx = getd(vert.x+RGL_offset.x);
  float dy = getd(vert.y+RGL_offset.y);

  float depth = (vert.z+RGL_offset.z)/RGL_p_far; // Depth is just normalized z

  gl_Position = vec4(normalize_d(dx, RGL_d_max), normalize_d(dy, RGL_d_max), depth, 1.0);
}
