#version 430 core

out vec4 color;

in vec2 texturecoord;
in vec3 highlight;

uniform sampler2D RGL_texture;

layout(std140) uniform RGL_palette {
  vec4 RGL_colors[256];
};

// TODO: The problem, is that I think it's better to do it in post processing rather that with every object, perhaps it will even be faster?
void main() {
  // color = vec4(1,1,1,1);
  // Since RGM is just OBJ but without the unordered shit, I guess the reversing of the coordnates has something to do with the fact that in obj the y=0 is the opposite.
  color = vec4(highlight,1) * texture(RGL_texture, vec2(texturecoord.x, -texturecoord.y));
  
  int besti = 0;
  float dst = 100.0;
  
  for (int i = 0; i < 256; i++) {
    float curdst = distance(RGL_colors[i], color);
    if (curdst < dst) {
      besti = i;
      dst = curdst;
    }
  }
  
  color = RGL_colors[besti];
}