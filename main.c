#include "RGL.h"
#include "TM.h"

int main() {
  TM_init(30);
  RGL_init(0, 640, 400);
  RGL_settitle("Momento Mori");

  RGL_setcursor(1);
  
  RGL_SHADER vert = RGL_loadshader("RGL/vertex.glsl", RGL_VERTEXSHADER);
  RGL_SHADER frag = RGL_loadshader("RGL/fragment.glsl", RGL_FRAGMENTSHADER);

  RGL_PROGRAM prog = RGL_initprogram(vert, frag);
  // RGL_saveprogram(prog, "RGL/program.glpb");

  // RGL_PROGRAM prog = RGL_loadprogram("RGL/program.glpb");

  RGL_EYE eye = RGL_initeye(prog, 1.3);
  RGL_loadcolors(eye, "cranes.rgc");

  float vertices[] = {
    // positions          // texture coords
     0.5f,  0.5f, 0.0f,  0.5f,  0.5f, 0.0f,   1.0f, 1.0f,   // top right
     0.5f, -0.5f, 0.0f,  0.5f,  0.5f, 0.0f,   1.0f, 0.0f,   // bottom right
    -0.5f, -0.5f, 0.0f,  0.5f,  0.5f, 0.0f,   0.0f, 0.0f,   // bottom left
    -0.5f,  0.5f, 0.0f,  0.5f,  0.5f, 0.0f,   0.0f, 1.0f    // top left 
    -0.5f,  0.7f, 0.1f,  0.5f,  0.5f, 0.0f,   0.0f, 1.0f    // top left 
  };
  UINT indices[] = {  // note that we start from 0!
    0, 1, 3,   // first triangle
    1, 2, 3,    // second triangle
    2, 3, 4    // second triangle
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

  // RGL_MODEL model = RGL_initmodel(vertices, 5, indices, 3, texturedata, 4, 4);
  RGL_MODEL model = RGL_loadmodel("untitled.rgm",RGL_loadtexture("mori.rgt", 1));
  RGL_MODEL model2 = RGL_loadmodel("frank.rgm", RGL_loadtexture("frank.rgt", 1));

  RGL_BODY bodies[] = {RGL_initbody(model2, 0), RGL_initbody(model, 0), RGL_initbody(model, 0)};
  bodies[0]->offset[2] += 5.5f;
  bodies[0]->offset[1] -= 5.5f;
  bodies[1]->offset[0] += 1.0f;
  bodies[1]->offset[2] += 2.0f;
  bodies[2]->offset[1] += 2.0f;
  bodies[2]->offset[2] += 2.0f;
  // bodies[0]->angles[1] -= 3.141/2;

  // eye->info.angles[1] -= 1;
  int i = 0;

  // TODO: Notice when we render all the shit, the hand renders only 4 vertices, just like the quads *big brain*(*smol brain* because I am the one who fucked it up).
  TM_initwait();
  while (1) {
    i++;
    // eye->info.angles[1] += 0.01;

    // bodies[1]->angles[1] -= 0.3;
    // bodies[1]->angles[2] += 0.2;
    // bodies[2]->angles[1] += 0.1;

    RGL_begin();
      RGL_drawbody(bodies[0]);
    RGL_end();

    TM_wait();
    // puts("OK");
  }
  return 0;
}
