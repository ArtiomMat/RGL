// This is the basic RGL vertex shader, you can copy anything from it for your own shader needs.
// You may notice that the math is not exactly your classic mat4 shit, nono, I made a faster method(I want to believe that, probably slightly slower).

#version 430 core

layout (location = 0) in vec3 vert;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 i_texturevert;

layout(std140) uniform RGL_eye {
  vec3 offset;
  vec3 angles;
  float p_near;
  float p_far;
  // r is for inverse, so it's 1/2*dx/y_max^.
  float rdx_max;
  float rdy_max;
} eye;

layout(std140) uniform RGL_sun {
  vec3 sundir; // DIRECTION OF THE SUUUUUUUUUUUUUUUN
  vec3 suncolor;
  int lightsn;
};

struct lightdata {
  vec3 offset;
  vec3 color;
};

layout(std140) uniform RGL_lights {
  lightdata lights[16];
};

uniform vec3 RGL_offset;
uniform vec3 RGL_angles;

out vec2 texturecoord;
out vec3 highlight;

const int gridsize = 32;

// h being either x or y, depending on which d we are getting.
float getd(float z, float h) {
  // So there is a very weird bug where vertices go crazy once they go outside of the view. Apparently, it has to do with points that are below the near plane on the z axis, and adding the following check and branching results, fixed it, from current observations.
  // My theory: We use a method where a triangle is inside another triangle and since they are below the z, this d/x=n/z formula breaks, as it does not account for the z being below the near plane. This results in the formula "thinking" that the triangle is in the view, since the x is within the vision plane, but it really is not. so thorwing d to like 100*x, will result in d/rd_max giving a result where the point is surely outside the view, we also make sure to include x, because of it's sign.
  if (z > eye.p_near)
    return (h*eye.p_near)/z;
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

  // Rotate around model(self)
  vec3 finale = rotate(RGL_angles, vert);
  finale += RGL_offset;
  
  // Calculate light stuff
  // vec3 RGL_light = vec3(-4, 4, 5.5);
  // RGL_light -= RGL_offset;
  // RGL_light = rotate(-RGL_angles, RGL_light);
  // vec3 L = normalize(RGL_light-vert);
  // highlight = vec3(dot(normal, L));

  highlight = dot(normal, normalize(sundir)) * suncolor;

  // Shift the model by the camera's offset
  finale -= eye.offset;

  // Eye offset is now 0,0,0, since we moved the mf, so it's just -finale
  vec3 dirtocam = normalize(-finale);
  float edn = dot(dirtocam, normal); // Eye Dot Normal, is used to avoid rendering useless faces.

  // Rotate the model around the camera
  finale = rotate(-eye.angles, finale);

  // Lock the vertex to a grid.
  ivec3 ifinale = ivec3(gridsize*finale);
  finale = vec3(ifinale)/gridsize;

  // Note depth in OpenGL, very weirdly is from -1 to 1
  float depth;
  if (edn < 0) // edn is only part of the whole face culling system.
    depth = 2;
  else
    depth = 2*(finale.z-eye.p_near)/(eye.p_far-eye.p_near)-1; // Depth is just normalized z

  
  float dx = getd(finale.z, finale.x);
  float dy = getd(finale.z, finale.y);
  gl_Position = vec4(normalize_d(dx, eye.rdx_max), normalize_d(dy, eye.rdy_max), depth, 1.0);
}
