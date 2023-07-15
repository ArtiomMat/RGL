#include "RGL.h"
#include "TM.h"

int main() {
  TM_init(30);
  RGL_init(16, 0, 400, 400);
  
  UINT shader = RGL_loadshader("vertex.glsl", RGL_VERTEXSHADER);
  RGL_initprogram(11, 69);

  TM_initwait();
  while (1) {
    RGL_refresh();
    TM_wait();
    // puts("OK");
  }
  return 0;
}
