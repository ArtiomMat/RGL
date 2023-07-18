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
  // TODO: Make it cached as inverse, multiplication by inverse is faster than finding it lol.
  // So eye_rd_max
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

// /*
//   Around y axis: coord(x,z)
//   Around x axis: coord(y,z)
//   Around z axis: coord(y,x)
// */
// // This works ok for the eye.
// vec2 eyerotate(float angle, vec2 coord) {
//   // sign fixes the issue of z becoming positive.
//   // I believe it has to do with the fact length is positive and the math here has no way of ever knowing if the z is supposed to stay negative or not.
//   // TODO: KEEP IN MIND, THIS BREAKS ROTATION ON THE XY PLANE, MOST LIKELY?
//   float mag = sign(coord.y) * length(coord);
  
//   angle = angle + atan(coord.x/coord.y);

//   coord.x = sin(angle) * mag;
//   coord.y = cos(angle) * mag;

//   return coord;
// }

// This is euler transforms.
vec3 rotate(vec3 a, vec3 p) {
  vec3 ret;
  
  // Around X
  ret.x = p.x;
  ret.y = p.y * cos(a.x) - p.z * sin(a.x);
  ret.z = p.y * sin(a.x) + p.z * cos(a.x);

  // Around Y
  p.x = ret.x;
  p.z = ret.z;
  ret.x = p.x * cos(a.y) + p.z * sin(a.y);
  ret.z = -p.x * sin(a.y) + p.z * cos(a.y);

  // Around Z
  p.x = ret.x;
  p.y = ret.y;
  ret.x = p.x * cos(a.z) - p.y * sin(a.z);
  ret.y = p.x * sin(a.z) + p.y * cos(a.z);

  return ret;
}

void main() {
  texturecoord = i_texturevert; // For the fragment shader

  // Rotate around model(self)
  vec3 finale = rotate(RGL_angles, vert);
  // Rotate around camera
  finale += RGL_offset-eye_offset;
  finale = rotate(-eye_angles, finale);
  
  if (finale.z > 0) {
    float depth = (finale.z-eye_p_near)/(eye_p_far-eye_p_near); // Depth is just normalized z
    float dx = getd(finale.z, finale.x);
    float dy = getd(finale.z, finale.y);

    gl_Position = vec4(normalize_d(dx, eye_d_max), normalize_d(dy, eye_d_max), depth, 1.0);
  }
  else {
    gl_Position = vec4(2,2,-2,1.0);
  }

  // gl_Position = vec4(vert.x+eye_offset.x, vert.y+eye_offset.y, depth, 1.0);
}
