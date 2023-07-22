#pragma once

#ifdef RGL_SRC
  #define EXTERN 
#else
  #define EXTERN extern
#endif

#define RGL_LIGHTSN 16

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

typedef UINT RGL_TEXTURE, RGL_PROGRAM, RGL_SHADER;

enum {
  RGL_VERTEXSHADER,
  RGL_FRAGMENTSHADER,
};

typedef struct {
  UINT type;
  union {
    struct {
      UINT code;
    } key;

  };
} RGL_INPUT;

typedef struct {
  struct {
    RGL_VEC offset;
    float padding1; // Padding to align angles to a 16-byte boundary
    RGL_VEC angles;
    // float padding2; // Padding to align subsequent members
    float p_near;
    float p_far;
    // So inverse of 2*d_max, on both y and x.
    // Since we are ratio aware!
    float rdx_max;
    float rdy_max;
  } info;
  float fov;
  RGL_PROGRAM program;
  UINT ubo; // What contains shader information about the camera
  UINT colorsubo; // What contains shader information about the camera
} RGL_EYEDATA, *RGL_EYE;

// A model can be played around with via CPU, since it's stored in RAM, and you don't have to update it, unlike an RGL_BODY.
typedef struct {
  // OpenGL stuff
  RGL_TEXTURE to; // texture object
  UINT vao; // vertex array object
  UINT vbo; // vertex buffer object
  UINT fbo; // face/element buffer object
  UINT facesn;
} RGL_MODELDATA, *RGL_MODEL;

// Just like a model, it is stored in RAM so it can be played around with via CPU.
// A body is an instance of a model, it exists in the rendering context and has an offset and other realtime attributes.
typedef struct {
  RGL_MODEL model;
  RGL_VEC offset;
  RGL_VEC angles;
  UCHAR flags;
} RGL_BODYDATA, *RGL_BODY;

typedef struct {
  RGL_VEC offset;
  float align0;
  RGL_VEC angles;
  RGL_VEC color;
  float strength;
  float size;
  int type;
} RGL_LIGHTDATA, *RGL_LIGHT;

EXTERN UINT RGL_width, RGL_height;
// Includes alpha
EXTERN float RGL_colors[256*4];

EXTERN UINT RGL_mousex, RGL_mousey;

EXTERN RGL_EYE RGL_usedeye;

// Terminated with (RGL_LIGHT)0, since RGL_LIGHT is a pointer.
EXTERN RGL_LIGHT RGL_lights[RGL_LIGHTSN];

// General RGL
// I don't recommend using frame capping if you vsync.
int RGL_init(UCHAR vsync, int width, int height);
void RGL_settitle(const char* title);
int RGL_loadcolors(const char* fp);
void RGL_setcursor(char shown, char captured);
void RGL_begin();
void RGL_drawbody(RGL_BODY body);
// Note, for drawing you must create a eye, optionally if you create multiple eyes, you can change RGL_usedeye, but the first eye you create is set automatically.
void RGL_drawbodies(RGL_BODY* bodies, UINT _i, UINT n);
// Call after draw calls.
void RGL_end();
void RGL_free();

// RGL_SHADER
RGL_SHADER RGL_loadshader(const char* fp, UINT type);
void RGL_freeshader(RGL_SHADER shader);

// RGL_TEXTURE
RGL_TEXTURE RGL_loadtexture(const char* fp);
void RGL_freetexture(RGL_TEXTURE texture);

// RGL_PROGRAM
RGL_PROGRAM RGL_initprogram(RGL_SHADER vertshader, RGL_SHADER fragshader);
RGL_PROGRAM RGL_loadprogram(const char* fp);
// For caching the program in the disk.
void RGL_saveprogram(RGL_PROGRAM program, const char* fp);
void RGL_freeprogram(RGL_PROGRAM program);

// RGL_EYE
// fov is in radians.
RGL_EYE RGL_initeye(RGL_PROGRAM program, float fov);
void RGL_freeeye(RGL_EYE eye);

// RGL_MODEL
// Free all these mfs after calling, since now the GPU has them copied.
// Initializes opengl part of the model
// vbodata contains data of both the model vertices and the texture's vertices:
// X Y Z U V ...
// verticesn is the number of vbo elements there are.
// fbodata simply contains data on the triangle array:
// T0 T1 T2 ...
// facesn is the number of those triplets that make triangles.
// texture must be in R G B format, for now...
// texturew/h is pretty self explanitory.
RGL_MODEL RGL_initmodel(float* vbodata, UINT verticesn, UINT* fbodata, UINT facesn, RGL_TEXTURE texture);
// You can set program to 0 for the default retro program.
RGL_MODEL RGL_loadmodel(const char* fp,  RGL_TEXTURE texture);
void RGL_freemodel(RGL_MODEL model);
// RGL_BODY
RGL_BODY RGL_initbody(RGL_MODEL model, UCHAR flags);
void RGL_freebody(RGL_BODY body);
