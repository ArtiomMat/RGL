// This is the basic RGL vertex shader, you can copy anything from it for your own shader needs.
// You may notice that the math is not exactly your classic mat4 shit, nono, I made a faster method(I want to believe that, probably slightly slower).

#version 430 core

layout (location = 0) in vec3 vert;
layout (location = 1) in vec2 i_texturevert;

layout(std140) uniform RGL_eye {
  vec3 eye_offset;
  vec3 eye_angles;
  float eye_p_near;
  float eye_p_far;
  float eye_d_max;
};

uniform vec3 RGL_offset;
uniform vec3 RGL_angles;

out vec2 texturecoord;

// TODO: Use ratio to avoid stretching!
// h being either x or y, depending on which d we are getting.
float getd(float z, float h) {
  return h*eye_p_near/(z);
}

// There are two d_max so it's a parameter
float normalize_d(float d, float d_max) {
  return d/(2*d_max);
  // return d/(d_max);
}

/*
  Around y axis: coord(x,z)
  Around x axis: coord(y,z)
  Around z axis: coord(y,x)
*/
vec2 rotate(float angle, vec2 coord) {
  float mag = length(coord);
  if (coord.x != 0)
    angle = angle + atan(coord.x/coord.y);
  coord.x = sin(angle) * mag;
  coord.y = cos(angle) * mag;

  return coord;
}

void main() {
  texturecoord = i_texturevert; // For the fragment shader

  vec3 finale = vert+RGL_offset-eye_offset;
  // finale.xy = rotate(eye_angles.z, finale.xy);
  finale.xz = rotate(eye_angles.y, finale.xz);
  finale.yz = rotate(eye_angles.x, finale.yz);
  
  float dx = getd(finale.z, finale.x);
  float dy = getd(finale.z, finale.y);

  float depth = (finale.z-eye_p_near)/(eye_p_far-eye_p_near); // Depth is just normalized z
  
  // Fixes an interesting phenomena where vertices in the back glitch into the front of the screen, unsure if it is me, or OpenGL, obviously it's me.
  if (depth < 0) {
    depth = -2;
    dx = dy = 2;
  }

  gl_Position = vec4(normalize_d(dx, eye_d_max), normalize_d(dy, eye_d_max), depth, 1.0);

  // gl_Position = vec4(vert.x+eye_offset.x, vert.y+eye_offset.y, depth, 1.0);
}
