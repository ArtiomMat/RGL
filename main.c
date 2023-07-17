#include "RGL.h"
#include "TM.h"

int main() {
  TM_init(12);
  RGL_init(16, 0, 400, 400);
  
  RGL_SHADER vert = RGL_loadshader("RGL/vertex.glsl", RGL_VERTEXSHADER);
  RGL_SHADER frag = RGL_loadshader("RGL/fragment.glsl", RGL_FRAGMENTSHADER);

  RGL_PROGRAM prog = RGL_initprogram(vert, frag);
  // RGL_saveprogram(prog, "RGL/program.glpb");

  // RGL_PROGRAM prog = RGL_loadprogram("RGL/program.glpb");

  RGL_EYE eye = RGL_initeye(prog, 1.7);

  float vertices[] = {
    // positions          // texture coords
     0.5f,  0.5f, -0.5f,   1.0f, 1.0f,   // top right
     0.5f, -0.5f, 0.0f,   1.0f, 0.0f,   // bottom right
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,   // bottom left
    -0.5f,  0.5f, 0.0f,   0.0f, 1.0f    // top left 
  };
  UINT indices[] = {  // note that we start from 0!
    0, 1, 3,   // first triangle
    1, 2, 3    // second triangle
  };  
  UCHAR texturedata[] = {
    255,0  ,0  ,    255,255,0  ,    255,0  ,0  ,    255,255,0  ,

    255,255,0  ,    255,0  ,0  ,    255,255,0  ,    255,0  ,0  ,

    255,0  ,0  ,    255,255,0  ,    255,0  ,0  ,    255,255,0  ,

    255,255,0  ,    255,0  ,0  ,    255,255,0  ,    255,0  ,0  ,
  };
  UCHAR texturedata2[] = {
    0  ,0  ,255  ,    0  ,255,255  ,    0  ,0  ,255  ,    0  ,255,255  ,

    0  ,255,255  ,    0  ,0  ,255  ,    0  ,255,255  ,    0  ,0  ,255  ,

    0  ,0  ,255  ,    0  ,255,255  ,    0  ,0  ,255  ,    0  ,255,255  ,

    0  ,255,255  ,    0  ,0  ,255  ,    0  ,255,255  ,    0  ,0  ,255  ,
  };

  RGL_MODEL model = RGL_initmodel(vertices, 4, indices, 2, texturedata, 4, 4);
  RGL_MODEL model2 = RGL_initmodel(vertices, 4, indices, 2, texturedata2, 4, 4);

  eye->info.offset[2] = -5;

  RGL_BODY bodies[] = {RGL_initbody(model, 0), RGL_initbody(model2, 0), RGL_initbody(model, 0)};
  bodies[0]->offset[2] += 15.5f;
  bodies[1]->offset[0] += 5.0f;
  bodies[2]->offset[1] += 2.0f;
  bodies[2]->offset[2] += 2.0f;

  int i = 0;

  TM_initwait();
  while (1) {
    
    // bodies[0]->offset[2] += 0.2f;
    // bodies[2]->offset[2] += 0.1f;
    // bodies[1]->offset[1] -= 0.2f;
    // bodies[2]->offset[2] += 0.2f;

    RGL_begin(1);
      RGL_drawbodies(bodies, 0, 3);
    RGL_end();

    TM_wait();
    // puts("OK");
  }
  return 0;
}
