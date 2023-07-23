#version 430 core

out vec4 color;

in vec2 f_t;
in vec3 f_highlight;

uniform sampler2D RGL_texture;

layout(std140) uniform colorsinfo {
  vec4 colors[256];
};

// TODO: The problem, is that I think it's better to do it in post processing rather that with every object, perhaps it will even be faster?
void main() {
  // color = vec4(1,1,1,1);
  // Since RGM is just OBJ but without the unordered shit, I guess the reversing of the coordnates has something to do with the fact that in obj the y=0 is the opposite.
  color = vec4(f_highlight/2+0.5,1) * texture(RGL_texture, vec2(f_t.x, -f_t.y));
  
  int besti = 0;
  float dst = 100.0;
  
  for (int i = 0; i < 256; i++) {
    float curdst = distance(colors[i], color);
    if (curdst < dst) {
      besti = i;
      dst = curdst;
    }
  }
  
  color = colors[besti];
}