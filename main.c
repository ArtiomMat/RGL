#include "RGL.h"
#include "TM.h"

int main() {
  TM_init(30);
  RGL_init(0, 640, 400);
  RGL_settitle("Momento Mori");
  
  RGL_SHADER vert = RGL_loadshader("RGL/vertex.glsl", RGL_VERTEXSHADER);
  RGL_SHADER frag = RGL_loadshader("RGL/fragment.glsl", RGL_FRAGMENTSHADER);

  RGL_PROGRAM prog = RGL_initprogram(vert, frag);
  // RGL_saveprogram(prog, "RGL/program.glpb");

  // RGL_PROGRAM prog = RGL_loadprogram("RGL/program.glpb");

  RGL_EYE eye = RGL_initeye(prog, 1.3);

  float vertices[] = {
    // positions          // texture coords
     0.5f,  0.5f, 0.0f,  0.5f,  0.5f, 0.0f,   1.0f, 1.0f,   // top right
     0.5f, -0.5f, 0.0f,  0.5f,  0.5f, 0.0f,   1.0f, 0.0f,   // bottom right
    -0.5f, -0.5f, 0.0f,  0.5f,  0.5f, 0.0f,   0.0f, 0.0f,   // bottom left
    -0.5f,  0.5f, 0.0f,  0.5f,  0.5f, 0.0f,   0.0f, 1.0f    // top left 
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
  RGL_MODEL model2 = RGL_loadmodel("mori.rgm", "mori.rgt");

  RGL_BODY bodies[] = {RGL_initbody(model2, 0), RGL_initbody(model2, 0), RGL_initbody(model2, 0)};
  bodies[0]->offset[2] += 7.5f;
  bodies[1]->offset[0] += 2.0f;
  bodies[2]->offset[1] += 2.0f;
  bodies[2]->offset[2] += 2.0f;

  // eye->info.angles[1] -= 1;
  int i = 0;


  TM_initwait();
  while (1) {
    
    // eye->info.angles[1] += 0.01;

    // bodies[1]->angles[1] -= 0.3;
    bodies[0]->angles[1] -= 0.05;
    // bodies[1]->angles[2] += 0.2;
    // bodies[2]->angles[1] += 0.1;

    RGL_drawbodies(bodies, 0, 1);
    RGL_refresh();

    TM_wait();
    // puts("OK");
  }
  return 0;
}
