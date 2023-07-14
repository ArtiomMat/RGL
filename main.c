#include "RGL.h"
#include "TM.h"

int main() {
  TM_init(30);
  RGL_init(16, 0, 400, 400);
  
  TM_initwait();
  while (1) {
    RGL_refresh();
    TM_wait();
    // puts("OK");
  }
  return 0;
}
