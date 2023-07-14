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

typedef struct {
  UCHAR* pixels;
  USHORT width, height, framesn;
} RGL_SKIN;

typedef struct {
  RGL_VEC* vecs;
  RGL_TRI* tris;
  RGL_SKIN* skin;
  UINT trisn;
  UINT vecsn;
} RGL_MODEL;

typedef struct {
  RGL_MODEL* model;
  USHORT framei;
  UCHAR flags;
} RGL_OBJ;

EXTERN UINT RGL_width, RGL_height;
EXTERN UCHAR RGL_colors[256][3];

EXTERN UINT RGL_mousex, RGL_mousey;

int RGL_init(UCHAR bpp, UCHAR vsync, int width, int height);
void RGL_refresh();
void RGL_free();
