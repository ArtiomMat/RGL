#include <windows.h>
#include <stdio.h>

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

  if (!module){
    puts("FATAL: RGL failed to load opengl32.dll.");
    exit(1);
  }

  rglCreateShader = load("glCreateShader");
  if (!rglCreateShader) {
    puts("FATAL: Loading OpenGL functions failed.");
    exit(1);
  }
  rglShaderSource = load("glShaderSource");
  rglCompileShader = load("glCompileShader");
  rglAttachShader = load("glAttachShader");
  rglLinkProgram = load("glLinkProgram");
  rglUseProgram = load("glUseProgram");
  rglDeleteShader = load("glDeleteShader");
  rglGetShaderiv = load("glGetShaderiv");
  rglGetShaderInfoLog = load("glGetShaderInfoLog");
  rglGenBuffers = load("glGenBuffers");
  rglBindBuffer = load("glBindBuffer");
  rglBufferData = load("glBufferData");
  rglEnableVertexAttribArray = load("glEnableVertexAttribArray");
  rglVertexAttribPointer = load("glVertexAttribPointer");
  rglGenVertexArrays = load("glGenVertexArrays");
  rglBindVertexArray = load("glBindVertexArray");
  rglDrawArrays = load("glDrawArrays");
  rglDeleteBuffers = load("glDeleteBuffers");
  rglDeleteVertexArrays = load("glDeleteVertexArrays");
  rglCreateProgram = load("glCreateProgram");
  rglDeleteProgram = load("glDeleteProgram");
  rglGetProgramiv = load("glGetProgramiv");
  rglGetProgramBinary = load("glGetProgramBinary");
  rglProgramBinary = load("glProgramBinary");
  rglProgramParameteri = load("glProgramParameteri");
  rglValidateProgram = load("glValidateProgram");
  rglDrawElements = load("glDrawElements");
  rglUniform1f = load("glUniform1f");

  rglUniform2f = load("glUniform2f");
  rglUniform3f = load("glUniform3f");
  rglUniform4f = load("glUniform4f");
  rglUniform1i = load("glUniform1i");
  rglUniform2i = load("glUniform2i");
  rglUniform3i = load("glUniform3i");
  rglUniform4i = load("glUniform4i");

  rglUniform1fv = load("glUniform1fv");
  rglUniform2fv = load("glUniform2fv");
  rglUniform3fv = load("glUniform3fv");
  rglUniform4fv = load("glUniform4fv");
  rglUniform1iv = load("glUniform1iv");
  rglUniform2iv = load("glUniform2iv");
  rglUniform3iv = load("glUniform3iv");
  rglUniform4iv = load("glUniform4iv");

  rglGetUniformfv = load("glGetUniformfv");
  
  rglGenerateMipmap = load("glGenerateMipmap");
  rglGetUniformLocation = load("glGetUniformLocation");

  rglUniformBlockBinding = load("glUniformBlockBinding");
  rglGetUniformBlockIndex = load("glGetUniformBlockIndex");
  rglGetActiveUniformBlockiv = load("glGetActiveUniformBlockiv");
  rglGetActiveUniformBlockName = load("glGetActiveUniformBlockName");
  rglBindBufferBase = load("glBindBufferBase");
  rglGetUniformIndices = load("glGetUniformIndices");
  rglGetActiveUniformsiv = load("glGetActiveUniformsiv");
  rglGetActiveUniformName = load("glGetActiveUniformName");
  rglGetActiveUniform = load("glGetActiveUniform");
  rglBufferSubData = load("glBufferSubData");

  #ifdef _WIN32
    rglSwapInterval = load("wglSwapIntervalEXT");
  #endif
}
