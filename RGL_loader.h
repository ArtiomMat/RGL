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

EXTERN void (*rglDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);

EXTERN GLint (*rglGetUniformLocation)(GLuint program, const GLchar *name);
EXTERN void (*rglUniform1f)(GLint location, GLfloat v0);
EXTERN void (*rglUniform2f)(GLint location, GLfloat v0, GLfloat v1);
EXTERN void (*rglUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
EXTERN void (*rglUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
EXTERN void (*rglUniform1i)(GLint location, GLint v0);
EXTERN void (*rglUniform2i)(GLint location, GLint v0, GLint v1);
EXTERN void (*rglUniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
EXTERN void (*rglUniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);

EXTERN void (*rglGetUniformfv)(GLuint program, GLint location, GLfloat* params);

EXTERN void (*rglUniform1fv)(GLint location, GLsizei count, const GLfloat* value);
EXTERN void (*rglUniform2fv)(GLint location, GLsizei count, const GLfloat* value);
EXTERN void (*rglUniform3fv)(GLint location, GLsizei count, const GLfloat* value);
EXTERN void (*rglUniform4fv)(GLint location, GLsizei count, const GLfloat* value);
EXTERN void (*rglUniform1iv)(GLint location, GLsizei count, const GLint* value);
EXTERN void (*rglUniform2iv)(GLint location, GLsizei count, const GLint* value);
EXTERN void (*rglUniform3iv)(GLint location, GLsizei count, const GLint* value);
EXTERN void (*rglUniform4iv)(GLint location, GLsizei count, const GLint* value);

EXTERN void (*rglGenerateMipmap) (GLenum target);

EXTERN void (*rglUniformBlockBinding) (GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
EXTERN GLint (*rglGetUniformBlockIndex) (GLuint program, const GLchar *uniformBlockName);
EXTERN void (*rglGetActiveUniformBlockiv) (GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params);
EXTERN void (*rglGetActiveUniformBlockName) (GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName);
EXTERN void (*rglBindBufferBase) (GLenum target, GLuint index, GLuint buffer);
EXTERN void (*rglGetUniformIndices) (GLuint program, GLsizei uniformCount, const GLchar **uniformNames, GLuint *uniformIndices);
EXTERN void (*rglGetActiveUniformsiv) (GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params);
EXTERN void (*rglGetActiveUniformName) (GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName);
EXTERN void (*rglGetActiveUniform) (GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
EXTERN void (*rglBufferSubData) (GLenum target, GLintptr offset, GLsizeiptr size, const void * data);