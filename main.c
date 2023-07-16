#include "RGL.h"
#include "TM.h"

int main() {
  TM_init(30);
  RGL_init(16, 0, 400, 400);
  
  RGL_SHADER vert = RGL_loadshader("RGL/vertex.glsl", RGL_VERTEXSHADER);
  RGL_SHADER frag = RGL_loadshader("RGL/fragment.glsl", RGL_FRAGMENTSHADER);

  RGL_PROGRAM prog = RGL_initprogram(vert, frag);
  // RGL_saveprogram(prog, "RGL/program.glpb");

  // RGL_PROGRAM prog = RGL_loadprogram("RGL/program.glpb");

  float vertices[] = {
    // positions          // texture coords
     0.5f,  0.5f, 0.0f,   1.0f, 1.0f,   // top right
     0.5f, -0.5f, 0.0f,   1.0f, 0.0f,   // bottom right
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,   // bottom left
    -0.5f,  0.5f, 0.0f,   0.0f, 1.0f    // top left 
  };
  UINT indices[] = {  // note that we start from 0!
    0, 1, 3,   // first triangle
    1, 2, 3    // second triangle
  };  
  UCHAR texturedata[] = {
    255,0  ,0  ,    0  ,255,0  ,    255,0  ,0  ,    0  ,255,0  ,

    0  ,255,0  ,    255,0  ,0  ,    0  ,255,0  ,    255,0  ,0  ,

    255,0  ,0  ,    0  ,255,0  ,    255,0  ,0  ,    0  ,255,0  ,

    0  ,255,0  ,    255,0  ,0  ,    0  ,255,0  ,    255,0  ,0  ,
  };

  RGL_MODEL model = RGL_initmodel(prog, vertices, 4, indices, 2, texturedata, 4, 4);

  TM_initwait();
  while (1) {
    RGL_begin(1);
      RGL_drawmodels(&model, 0, 1);
    RGL_end();

    RGL_end();
    TM_wait();
    // puts("OK");
  }
  return 0;
}
