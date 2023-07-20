// This is the basic RGL vertex shader, you can copy anything from it for your own shader needs.
// You may notice that the math is not exactly your classic mat4 shit, nono, I made a faster method(I want to believe that, probably slightly slower).

#version 430 core

layout (location = 0) in vec3 vert;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 i_texturevert;

layout(std140) uniform RGL_eye {
  vec3 eye_offset;
  vec3 eye_angles;
  float eye_p_near;
  float eye_p_far;
  // r is for inverse, so it's 1/2*dx/y_max^.
  float eye_rdx_max;
  float eye_rdy_max;
};

uniform vec3 RGL_offset;
uniform vec3 RGL_angles;

out vec2 texturecoord;
out float highlight;

// h being either x or y, depending on which d we are getting.
float getd(float z, float h) {
  // So there is a very weird bug where vertices go crazy once they go outside of the view. Apparently, it has to do with points that are below the near plane on the z axis, and adding the following check and branching results, fixed it, from current observations.
  // My theory: We use a method where a triangle is inside another triangle and since they are below the z, this d/x=n/z formula breaks, as it does not account for the z being below the near plane. This results in the formula "thinking" that the triangle is in the view, since the x is within the vision plane, but it really is not. so thorwing d to like 100*x, will result in d/rd_max giving a result where the point is surely outside the view, we also make sure to include x, because of it's sign.
  // FIXME: a problem where one a vertex is thrown far away and it causes stretching of triangles. Perhaps create a formula for when the vertex is behind the vision plane.
  if (z > eye_p_near)
    return (h*eye_p_near)/z;
  return h; // NOTE: Changed from h*100, maybe it's not a good change, seems fine for now.
}

// There are two d_max so it's a parameter
float normalize_d(float d, float rd_max) {
  return d*(rd_max);
}

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
  // gl_Position = vec4(i_texturevert-0.5, 0.5, 1.0);
  // return;

  // Rotate around model(self)
  vec3 finale = rotate(RGL_angles, vert);
  finale += RGL_offset;
  
  // Calculate light stuff
  vec3 RGL_light = vec3(3, -7, -3);
  RGL_light -= RGL_offset;
  RGL_light = rotate(-RGL_angles, RGL_light);
  vec3 L = normalize(RGL_light-vert);
  highlight = dot(normal, L);

  // Rotate around camera
  finale -= eye_offset;
  finale = rotate(-eye_angles, finale);
  
  float depth = (finale.z-eye_p_near)/(eye_p_far-eye_p_near); // Depth is just normalized z
  float dx = getd(finale.z, finale.x);
  float dy = getd(finale.z, finale.y);
  gl_Position = vec4(normalize_d(dx, eye_rdx_max), normalize_d(dy, eye_rdy_max), depth, 1.0);
}
