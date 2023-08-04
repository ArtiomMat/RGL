#include <stdio.h>
#include <stdlib.h>

#include "RGL.h"
#include "TM.h"
#include "UTL.h"

RGL_EYE eye;

int cursorstate = 1;

void rglkeycb(int key, int down) {
  if (key == 'W')
    eye->info.offset[2] += 0.1;
  else if (key == 'A')
    eye->info.offset[0] -= 0.1;
  else if (key == 'D')
    eye->info.offset[0] += 0.1;
  else if (key == 'S')
    eye->info.offset[2] -= 0.1;
  else if (key == RGL_KESCAPE && !down)
    RGL_setcursor(cursorstate=!cursorstate);
  else if (key == RGL_KWINDOWEXIT) {
    RGL_free();
    UTL_Free();
    exit(0);
  }
}

void rglmovecb(int what, int x, int y) {
  eye->info.angles[1] += x*0.0015f;
  eye->info.angles[0] += y*0.0015f;
}

int main() {
  UTL_Init();
  // UTL_MessageBox("Memento Mori question", "Hello", UTL_MBYesNoCancel);
  TM_init(30);
  if (!RGL_init(0, 600, 500))
    return 1;
  RGL_settitle("Momento Mori");

  RGL_keycb = rglkeycb;
  RGL_movecb = rglmovecb;
  
  RGL_SHADER vert = RGL_loadshader(UTL_RelPath("RGL/rgl_v.glsl"), RGL_VERTEXSHADER);
  RGL_SHADER frag = RGL_loadshader(UTL_RelPath("RGL/rgl_f.glsl"), RGL_FRAGMENTSHADER);

  RGL_PROGRAM prog = RGL_initprogram(vert, frag);
  RGL_loadcolors(prog, UTL_RelPath("RGL/cranes.rgc"));
  // RGL_saveprogram(prog, UTL_RelPath("RGL/program.glpb"));

  // RGL_OLDPROGRAM prog = RGL_loadprogram(UTL_RelPath("RGL/program.glpb"));

  eye = RGL_initeye(prog, 1.3);

  // RGL_MODEL model = RGL_initmodel(vertices, 5, indices, 3, texturedata, 4, 4);
  RGL_MODEL model = RGL_loadmodel(UTL_RelPath("RGL/untitled.rgm"),RGL_loadtexture(UTL_RelPath("RGL/mori.rgt"), 1));
  RGL_MODEL model2 = RGL_loadmodel(UTL_RelPath("RGL/aristocrat.rgm"), RGL_loadtexture(UTL_RelPath("RGL/aristocrat.rgt"), 1));

  RGL_BODY bodies[] = {RGL_initbody(model2, 0), RGL_initbody(model, RGL_BODYFLUNLIT), RGL_initbody(model, 0)};
  bodies[0]->info.offset[2] += 5.5f;
  bodies[0]->info.offset[1] -= 3.0f;
  bodies[1]->info.offset[0] += 1.0f;
  bodies[1]->info.offset[2] += 2.0f;
  bodies[2]->info.offset[1] += 2.0f;
  bodies[2]->info.offset[2] += 2.0f;
  bodies[0]->info.angles[1] -= 3.141;

  eye->sun.sundir[1] = 0.5;
  eye->sun.sundir[0] = -0.5;
  eye->sun.sundir[2] = 0.1;
  eye->sun.suncolor[0] = 1;
  eye->sun.suncolor[1] = 1;
  eye->sun.suncolor[2] = 1;

  // eye->info.angles[1] -= 1;
  int i = 0;

  RGL_setcursor(1);
  // TODO: Notice when we render all the shit, the hand renders only 4 vertices, just like the quads *big brain*(*smol brain* because I am the one who fucked it up).
  TM_initwait();
  while (1) {
    i++;

    RGL_begin();
      RGL_drawbody(bodies[1]);
      RGL_resetdepth();
      RGL_drawbody(bodies[0]);
    RGL_end();

    TM_wait();
    // puts("OK");
  }
  return 0;
}
