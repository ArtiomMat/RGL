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
  glShaderSource = load("glShaderSource");
  glCompileShader = load("glCompileShader");
  glAttachShader = load("glAttachShader");
  glLinkProgram = load("glLinkProgram");
  glUseProgram = load("glUseProgram");
  glDeleteShader = load("glDeleteShader");
  glGetShaderiv = load("glGetShaderiv");
  glGetShaderInfoLog = load("glGetShaderInfoLog");
  glGenBuffers = load("glGenBuffers");
  glBindBuffer = load("glBindBuffer");
  glBufferData = load("glBufferData");
  glEnableVertexAttribArray = load("glEnableVertexAttribArray");
  glVertexAttribPointer = load("glVertexAttribPointer");
  glGenVertexArrays = load("glGenVertexArrays");
  glBindVertexArray = load("glBindVertexArray");
  glDrawArrays = load("glDrawArrays");
  glDeleteBuffers = load("glDeleteBuffers");
  glDeleteVertexArrays = load("glDeleteVertexArrays");
  glCreateProgram = load("glCreateProgram");
  glDeleteProgram = load("glDeleteProgram");
  glGetProgramiv = load("glGetProgramiv");
  glGetProgramBinary = load("glGetProgramBinary");
  glProgramBinary = load("glProgramBinary");
  glProgramParameteri = load("glProgramParameteri");
}
