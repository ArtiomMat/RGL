#include "RGL.h"
#include "TM.h"

// Secret, shhhh....
void RGL_initmodel(RGL_MODEL modelptr);

int main() {
  TM_init(30);
  RGL_init(16, 0, 400, 400);
  
  // RGL_SHADER vert = RGL_loadshader("RGL/vertex.glsl", RGL_VERTEXSHADER);
  // RGL_SHADER frag = RGL_loadshader("RGL/fragment.glsl", RGL_FRAGMENTSHADER);

  // RGL_PROGRAM prog = RGL_initprogram(vert, frag);
  // RGL_saveprogram(prog, "RGL/program.glpb");

  RGL_PROGRAM prog = RGL_loadprogram("RGL/program.glpb");

  RGL_MODEL model = malloc(sizeof(*model));
  model->verticesn = 3;
  float v[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
  };
  model->vertices = v;
  model->program = prog;
  RGL_initmodel(model);

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
