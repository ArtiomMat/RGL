#version 430 core

out vec4 color;

in vec2 texturecoord;
in float highlight;

uniform sampler2D RGL_texture;

layout(std140) uniform RGL_eye {
  vec4 RGL_colors[256];
};

void main() {
  color = texture(RGL_texture, texturecoord);
  color *= vec4(highlight, highlight/3, highlight/3, 1);
  
  int besti = 0;
  float dst = 100.0;
  
  for (int i = 0; i < 256; i++) {
    vec4 curdst = distance(RGL_colors[i], color);
    if (curdst < dst) {
      besti = i;
      dst = curdst;
    }
  }
  
  color = RGL_colors[besti];
}