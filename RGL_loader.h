#include <GL/glext.h>
#include <GL/glcorearb.h>
#include <GL/gl.h>

#ifdef RGL_SRC
  #define EXTERN 
#else
  #define EXTERN extern
#endif

void RGL_loadgl();

EXTERN GLuint (*glCreateShader) (GLenum shaderType);
EXTERN void (*glShaderSource) (GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
EXTERN void (*glCompileShader) (GLuint shader);
EXTERN void (*glAttachShader) (GLuint program, GLuint shader);
EXTERN void (*glLinkProgram) (GLuint program);
EXTERN void (*glUseProgram) (GLuint program);
EXTERN void (*glDeleteShader) (GLuint shader);
EXTERN void (*glGetShaderiv) (GLuint shader, GLenum pname, GLint *params);
EXTERN void (*glGetShaderInfoLog) (GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
EXTERN void (*glGenBuffers) (GLsizei n, GLuint *buffers);
EXTERN void (*glBindBuffer) (GLenum target, GLuint buffer);
EXTERN void (*glBufferData) (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
EXTERN void (*glEnableVertexAttribArray) (GLuint index);
EXTERN void (*glVertexAttribPointer) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
EXTERN void (*glGenVertexArrays) (GLsizei n, GLuint *arrays);
EXTERN void (*glBindVertexArray) (GLuint array);
EXTERN void (*glDrawArrays) (GLenum mode, GLint first, GLsizei count);
EXTERN void (*glDeleteBuffers) (GLsizei n, const GLuint *buffers);
EXTERN void (*glDeleteVertexArrays) (GLsizei n, const GLuint *arrays);
EXTERN GLuint (*glCreateProgram) ();
EXTERN void (*glDeleteProgram) (GLuint program);
EXTERN void (*glGetProgramiv) (GLuint program, GLenum pname, GLint *params);

EXTERN void (*glGetProgramBinary) (GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
EXTERN void (*glProgramBinary) (GLuint program, GLenum binaryFormat, const void *binary, GLsizei length);
EXTERN void (*glProgramParameteri) (GLuint program, GLenum pname, GLint value);
