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
