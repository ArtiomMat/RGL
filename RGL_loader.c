#include <windows.h>

#include "RGL_loader.h"

static HMODULE module;

static void* load(const char* name) {
  void *p = (void *)wglGetProcAddress(name);
  
  if(p == 0 ||
    (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) ||
    (p == (void*)-1) )
      p = (void *)GetProcAddress(module, name);

  return p;
}

void RGL_loadgl() {
  module = LoadLibraryA("opengl32.dll");

  glCreateShader = load("glCreateShader");
}
