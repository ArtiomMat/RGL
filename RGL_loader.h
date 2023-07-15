#pragma once

#include <GL/glcorearb.h>
#include <GL/gl.h>
#include <GL/glext.h>

#ifdef RGL_SRC
  #define EXTERN 
#else
  #define EXTERN extern
#endif

void RGL_loadgl();

EXTERN GLuint (*rglCreateShader) (GLenum shaderType);
EXTERN void (*rglShaderSource) (GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
EXTERN void (*rglCompileShader) (GLuint shader);
EXTERN void (*rglAttachShader) (GLuint program, GLuint shader);
EXTERN void (*rglLinkProgram) (GLuint program);
EXTERN void (*rglUseProgram) (GLuint program);
EXTERN void (*rglDeleteShader) (GLuint shader);
EXTERN void (*rglGetShaderiv) (GLuint shader, GLenum pname, GLint *params);
EXTERN void (*rglGetShaderInfoLog) (GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
EXTERN void (*rglGenBuffers) (GLsizei n, GLuint *buffers);
EXTERN void (*rglBindBuffer) (GLenum target, GLuint buffer);
EXTERN void (*rglBufferData) (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
EXTERN void (*rglEnableVertexAttribArray) (GLuint index);
EXTERN void (*rglVertexAttribPointer) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
EXTERN void (*rglGenVertexArrays) (GLsizei n, GLuint *arrays);
EXTERN void (*rglBindVertexArray) (GLuint array);
EXTERN void (*rglDrawArrays) (GLenum mode, GLint first, GLsizei count);
EXTERN void (*rglDeleteBuffers) (GLsizei n, const GLuint *buffers);
EXTERN void (*rglDeleteVertexArrays) (GLsizei n, const GLuint *arrays);
EXTERN GLuint (*rglCreateProgram) ();
EXTERN void (*rglDeleteProgram) (GLuint program);
EXTERN void (*rglGetProgramiv) (GLuint program, GLenum pname, GLint *params);

EXTERN void (*rglGetProgramBinary) (GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
EXTERN void (*rglProgramBinary) (GLuint program, GLenum binaryFormat, const void *binary, GLsizei length);
EXTERN void (*rglProgramParameteri) (GLuint program, GLenum pname, GLint value);
EXTERN void (*rglValidateProgram) ( 	GLuint program);