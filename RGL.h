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

enum {
  RGL_VERTEXSHADER,
  RGL_FRAGMENTSHADER,
};

typedef struct {
} RGL_SKIN;

typedef struct {
  RGL_VEC* vecs;
  RGL_TRI* tris;
  struct {
    UCHAR* pixels;
    USHORT width, height, framesn;
  } skin;
  UINT program;
  UINT trisn;
  UINT vecsn;
} RGL_MODEL;

typedef struct {
  RGL_MODEL* model;
  RGL_VEC offset;
  RGL_VEC angles;
  USHORT framei;
  UCHAR flags;
} RGL_OBJ;

EXTERN UINT RGL_width, RGL_height;
EXTERN UCHAR RGL_colors[256][3];

EXTERN UINT RGL_mousex, RGL_mousey;

// General RGL
int RGL_init(UCHAR bpp, UCHAR vsync, int width, int height);
void RGL_refresh();
void RGL_free();
// Shaders
UINT RGL_loadshader(const char* fp, UINT type);
void RGL_freeshader(UINT shader);
// Program
UINT RGL_initprogram(UINT vertshader, UINT fragshader);
UINT RGL_loadprogram(const char* fp);
void RGL_saveprogram(UINT program, const char* fp);
void RGL_freeprogram(UINT program);
// RGL_MODEL
void RGL_loadmodel(UINT program);
