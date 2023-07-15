#pragma once

#ifdef RGL_SRC
  #define EXTERN 
#else
  #define EXTERN extern
#endif

/*
	RGL - Retro Graphics Library
	This library mimics retro graphics.
	
	It also provides the input from the user's keyboard and shit, since that's Windows's gist.
*/

typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned int UINT;

typedef float RGL_VEC[3];
typedef UINT RGL_TRI[3];

typedef UINT RGL_PROGRAM, RGL_SHADER;

enum {
  RGL_VERTEXSHADER,
  RGL_FRAGMENTSHADER,
};

// A model can be played around with via CPU, since it's stored in RAM, and you don't have to update it, unlike an RGL_BODY.
typedef struct {
  float* vertices;
  UINT* indices;
  struct {
    UCHAR* pixels;
    USHORT width, height, framesn;
  } skin;
  UINT indicesn;
  UINT verticesn;

  // OpenGL stuff
  RGL_PROGRAM program;
  UINT vao;
  UINT vertex_bo;
  UINT index_bo;
} RGL_MODELDATA, *RGL_MODEL;

// Just like a model, it is stored in RAM so it can be played around with via CPU.
// A body is an instance of a model, it exists in the rendering context and has an offset and other realtime attributes.
typedef struct {
  RGL_MODEL* model;
  RGL_VEC offset;
  RGL_VEC angles;
  // USHORT framei;
  UCHAR flags;
} RGL_BODYDATA, *RGL_BODY;

EXTERN UINT RGL_width, RGL_height;
EXTERN UCHAR RGL_colors[256][3];

EXTERN UINT RGL_mousex, RGL_mousey;

// General RGL
int RGL_init(UCHAR bpp, UCHAR vsync, int width, int height);
void RGL_begin(char doclear);
void RGL_drawmodels(RGL_MODEL* models, UINT _i, UINT n);
void RGL_end();
void RGL_free();
// Shaders
RGL_SHADER RGL_loadshader(const char* fp, UINT type);
void RGL_freeshader(RGL_SHADER shader);
// Program
RGL_PROGRAM RGL_initprogram(RGL_SHADER vertshader, RGL_SHADER fragshader);
RGL_PROGRAM RGL_loadprogram(const char* fp);
// For caching the program in the disk.
void RGL_saveprogram(RGL_PROGRAM program, const char* fp);
void RGL_freeprogram(RGL_PROGRAM program);
// RGL_MODEL
// You can set program to 0 for the default retro program.
RGL_MODEL RGL_loadmodel(const char* fp, RGL_PROGRAM program);
void RGL_freemodel(RGL_MODEL model, RGL_PROGRAM program);
// RGL_BODY
RGL_BODY RGL_initbody(RGL_MODEL model, UCHAR flags);
void RGL_freebody(RGL_BODY program);
