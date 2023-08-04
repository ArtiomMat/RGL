// This is the basic RGL vertex shader, you can copy anything from it for your own shader needs.
// You may notice that the math is not exactly your classic mat4 shit, nono, I made a faster method(I want to believe that, probably slightly slower).

#version 430 core

layout (location = 0) in vec3 in_v;
layout (location = 1) in vec3 in_n;
layout (location = 2) in vec2 in_t;

layout(std140) uniform eyeinfo {
  vec3 offset;
  vec3 angles;
  float p_near;
  float p_far;
  // r is for inverse, so it's 1/2*dx/y_max^.
  float rdx_max;
  float rdy_max;
} eye;

layout(std140) uniform suninfo {
  vec3 sundir; // DIRECTION OF THE SUUUUUUUUUUUUUUUN
  vec3 suncolor;
  int lightsn;
};

struct lightdata {
  vec3 offset;
  vec3 color;
};
layout(std140) uniform lightsinfo {
  lightdata lights[16];
};

layout(std140) uniform bodyinfo {
  vec3 body_offset;
  vec3 body_angles;
  int body_flags;
};

out float f_depth;
out vec2 f_t;
out vec3 f_highlight;

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
  
  // Around Z
  ret.x = p.x * cos(a.z) - p.y * sin(a.z);
  ret.y = p.x * sin(a.z) + p.y * cos(a.z);

  // Around Y
  p.x = ret.x;
  ret.x = p.x * cos(a.y) + p.z * sin(a.y);
  ret.z = -p.x * sin(a.y) + p.z * cos(a.y);

  // Around X
  p.z = ret.z;
  p.y = ret.y;
  ret.y = p.y * cos(a.x) - p.z * sin(a.x);
  ret.z = p.y * sin(a.x) + p.z * cos(a.x);

  return ret;
}

void main() {
  f_t = in_t; // For the fragment shader

  // Rotate around model(self)
  vec3 finale = rotate(body_angles, in_v);
  finale += body_offset;
  
  // Calculate light stuff
  // vec3 RGL_light = vec3(-4, 4, 5.5);
  // RGL_light -= body_offset;
  // RGL_light = rotate(-body_angles, RGL_light);
  // vec3 L = normalize(RGL_light-in_v);
  // f_highlight = vec3(dot(in_n, L));

  // If the objcet is unlit just make lighting 1
  if ((body_flags & (1<<1)) == (1<<1)) {
    f_highlight = vec3(1);
  }
  else {
    f_highlight = vec3(0);

    for (int i = 0; i < lightsn; i++) {
      vec3 offset = lights[i].offset;
      offset -= body_offset;
      offset = rotate(-body_angles, offset); // FIXME: Probably broken, the idea is though Instead of rotating the normals, the light can be rotated!
      vec3 dst = offset-in_v;
      vec3 L = normalize(dst); // We don't use finale since it's rotated, and if we use finale it's just a double rotation.
      float len = length(dst);
      f_highlight += vec3(dot(in_n, L)) * lights[i].color / (len*len);
    }

    vec3 new_sundir = normalize(sundir);
    new_sundir = rotate(-body_angles, new_sundir);
    // Finally the sun calculation
    f_highlight += (dot(in_n, normalize(new_sundir))) * suncolor;
  }

  // Shift the model by the camera's offset
  finale -= eye.offset;

  // Eye offset is now 0,0,0, since we moved the mf, so it's just -finale
  // TODO: Apply camera rotation, since when the object rotates, nothing changes.
  vec3 dirtocam = normalize(-finale);
  // float edn = dot(dirtocam, in_n); // Eye Dot Normal, is used to avoid rendering useless faces.
  float edn = 1;

  // Rotate the model around the camera
  finale = rotate(-eye.angles, finale);

  // Lock the vertex to a grid.
  ivec3 ifinale = ivec3(gridsize*finale);
  finale = vec3(ifinale)/gridsize;

  // Note depth in OpenGL, very weirdly is from -1 to 1
  float depth;
  if (edn < 0) // edn is only part of the whole face culling system.
    depth = 2;
  else {
    depth = 2*(finale.z-eye.p_near)/(eye.p_far-eye.p_near)-1; // Depth is just normalized z
    f_depth = depth;
  }

  float dx = getd(finale.z, finale.x);
  float dy = getd(finale.z, finale.y);
  gl_Position = vec4(normalize_d(dx, eye.rdx_max), normalize_d(dy, eye.rdy_max), depth, 1.0);
}
