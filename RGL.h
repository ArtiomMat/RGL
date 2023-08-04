#pragma once

// Retro Graphics Library - Hardware accelerated graphics library that mimics old games.

#ifdef RGL_SRC
  #define EXTERN 
#else
  #define EXTERN extern
#endif

// THIS MUST MATCH THE LIGHT NUMBER IN THE VERTEX SHADER!
#define RGL_MAXLIGHTSN 16

enum {
  // Culling of individaul triangles is always a thing, but entire body culling is disabled by default, since maybe the body is a map.
  // Culling happens based on the body's offset to the camera, done on the CPU.
  RGL_BODYFLCULLED = 1<<0,
  // Mainly useful for skyboxes, they wont be affected by lights and can safely just hover above the model.
  RGL_BODYFLUNLIT = 1<<1,
};

/*
	RGL - Retro Graphics Library
	This library mimics retro graphics.
	
	It also provides the input from the user's keyboard and shit, since that's Windows's gist.
*/

typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned int UINT;

typedef float RGL_VEC[3];

// A GPU object, hence you cannot modify it in your program, not without functions.
typedef UINT RGL_TEXTURE, RGL_OLDPROGRAM, RGL_SHADER, RGL_COLORS;

// The binding indieces of all the UBOs a program in RGL has.
enum {
  RGL_EYEUBOINDEX,
  RGL_LIGHTSUBOINDEX,
  RGL_SUNUBOINDEX,
  RGL_COLORSUBOINDEX,
  RGL_BODYUBOINDEX,
  RGL_UBOSN,
};

typedef struct {
  UINT p; // OpenGL program object
  UINT ubos[RGL_UBOSN];
} RGL_PROGRAMDATA, *RGL_PROGRAM;

enum {
  RGL_VERTEXSHADER,
  RGL_FRAGMENTSHADER,
};

// These are used within an eye, and stored withing an eye.
// Currently only point light
typedef struct {
  RGL_VEC offset;
  float _padding1;
  RGL_VEC color;
  float _padding2;
} RGL_LIGHTDATA, *RGL_LIGHT;

typedef struct {
  struct RGL_EYEINFO {
    RGL_VEC offset;
    float _padding1; // Padding to align angles to a 16-byte boundary, caused quite a bit of issues until I figured it out.
    RGL_VEC angles;
    float p_near;
    float p_far;
    // So inverse of d_max, on both y and x.
    // Since we are ratio aware!
    float rdx_max;
    float rdy_max;
  } info;
  struct RGL_SUNINFO {
    RGL_VEC sundir;
    float _padding1;
    RGL_VEC suncolor;
    int lightsn;
  } sun;
  // Terminated with (RGL_LIGHT)0, it's a pointer so it's valid.
  RGL_LIGHT lights[RGL_MAXLIGHTSN];
  float fov;
  RGL_PROGRAM program;
} RGL_EYEDATA, *RGL_EYE;

// A model can be played around with via CPU, since it's stored in RAM, and you don't have to update it, unlike an RGL_BODY.
typedef struct {
  // OpenGL stuff
  RGL_TEXTURE to; // texture object
  UINT vao; // vertex array object
  UINT vbo; // vertex buffer object
  UINT fbo; // face/element buffer object
  UINT facesn;
  struct RGL_ANIMATION {
    struct RGL_FRAME {
      UINT index; // Index of the vertex buffer
      UINT time; // In ms
    }* frames;
    UINT framesn;
  }* animations;
  UINT animationsn;
} RGL_MODELDATA, *RGL_MODEL;

// Just like a model, it is stored in RAM so it can be played around with via CPU.
// A body is an instance of a model, it exists in the rendering context and has an offset and other realtime attributes.
typedef struct {
  RGL_MODEL model;
  // The reason it's in a struct it needs to be passed over to OpenGL as a UBO
  struct RGL_BODYINFO {
    RGL_VEC offset;
    float _padding1;
    RGL_VEC angles;
    int flags;
  } info;
} RGL_BODYDATA, *RGL_BODY;

enum {
  RGL_KMOUSEL,
  RGL_KMOUSEM,
  RGL_KMOUSER,
  
  RGL_KWINDOWEXIT, // Only has down, no up.

  RGL_KCONTROL,
  RGL_KCAPTIAL,
  RGL_KTAB,
  RGL_KMETA,
  RGL_KENTER,
  RGL_KBACKSPACE,
  RGL_KSPACE,
  RGL_KSHIFT,
  RGL_KESCAPE,

  RGL_KUP,
  RGL_KDOWN,
  RGL_KRIGHT,
  RGL_KLEFT,
};

enum {
  RGL_MMOUSE, // x and y are the delta of the mouse.
};

EXTERN UINT RGL_width, RGL_height;

EXTERN int RGL_mousex, RGL_mousey;

EXTERN RGL_EYE RGL_usedeye;

// Keep in mind that RGL will send down signals even if the key was not pressed this frame, but is still held down, this is not very useful as WinAPI for example has a pause in these cases and the behaviour is not very useful, so it's better to have logic in your program that ignores additional down signals until an up signal, and repeat.
EXTERN void (*RGL_keycb) (int key, int down);
EXTERN void (*RGL_movecb) (int what, int x, int y);

// General RGL
// I don't recommend using frame capping if you vsync.
int RGL_init(UCHAR vsync, int width, int height);
void RGL_settitle(const char* title);
void RGL_setcursor(char captured);
void RGL_begin();
// This can be used to have multiple layers of rendering. Intention is for GUI, or viewmodels.
void RGL_resetdepth();
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
// mipmap is 0 for not using mipmaps, 1 for using
RGL_TEXTURE RGL_loadtexture(const char* fp, char mipmap);
void RGL_freetexture(RGL_TEXTURE texture);

// RGL_PROGRAM
RGL_PROGRAM RGL_initprogram(RGL_SHADER vertshader, RGL_SHADER fragshader);
RGL_PROGRAM RGL_loadprogram(const char* fp);
// For caching the program in the disk.
int RGL_saveprogram(RGL_PROGRAM program, const char* fp);
void RGL_freeprogram(RGL_PROGRAM program);
int RGL_loadcolors(RGL_PROGRAM program, const char* fp);

// RGL_LIGHT
// Position and color can be modifed within the struct itself.
RGL_LIGHT RGL_initlight(float strength);
void RGL_freelight(RGL_LIGHT light);

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
RGL_BODY RGL_initbody(RGL_MODEL model, int flags);
// Plays animation i at moment m.
void RGL_play(RGL_BODY body, UINT i, UINT m);
void RGL_freebody(RGL_BODY body);
