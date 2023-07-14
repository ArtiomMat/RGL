#pragma once

// Artiom's 3D software renderer.

#include <math.h>

#define INLINE inline __attribute__((always_inline))

#define D3D_PI 3.141593f
// The parameter that is added to qnormalize
#define D3D_QNADDER (-0.5f)

#define D3D_DtoR(X) (X*D3D_PI/180)

typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned int UINT;

typedef float D3D_VEC[3];
typedef UINT D3D_TRI[3];

typedef struct {
	USHORT w, h;
	USHORT frames;
	UCHAR* pixels;
} D3D_IMG;

// The base of most physical structs in D3D
typedef struct {
	D3D_VEC offset;
	D3D_VEC angles; // In radians
} D3D_OBJ;

typedef struct D3D_MODEL {
	struct D3D_MODEL *next;
	D3D_OBJ obj;
	D3D_VEC* vecs; // The points that comprise the mf
	D3D_TRI* tris; // The indices to the triangles
	char* triculls; // Wheter these triangles are culled or not.
	D3D_IMG* img;
	UINT vecsn, trisn;
} D3D_MODEL;

typedef struct {
	D3D_OBJ obj;
	float fov; // In radians!!!
	float near; // The near plane's distance from the camera
	float far;
	UCHAR exposure; // 0 means void, 255 means total white-ness.
	UCHAR fog; // 0 no fog, 255 means total white.
} D3D_CAMERA;

typedef struct D3D_LIGHT {
	struct D3D_LIGHT *next;
	D3D_OBJ obj;
	float size;
	float strength;
} D3D_LIGHT;

extern D3D_CAMERA D3D_cam;
extern D3D_LIGHT* D3D_lights;
extern D3D_MODEL* D3D_models;

float D3D_Q_rsqrt( float number );

INLINE float D3D_dot(D3D_VEC a, D3D_VEC b) {
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

INLINE float D3D_magnitude(D3D_VEC in) {
	return sqrtf(D3D_dot(in, in));
}

// Uses quake's rsqrt, calculates 1/magnitude.
INLINE float D3D_rmagnitude(D3D_VEC in) {
	return D3D_Q_rsqrt(D3D_dot(in, in));
}

INLINE void D3D_vecdiv(D3D_VEC a, D3D_VEC b, D3D_VEC out) {
	out[0] = a[0] / b[0];
	out[1] = a[1] / b[1];
	out[2] = a[2] / b[2];
}
INLINE void D3D_vecsub(D3D_VEC a, D3D_VEC b, D3D_VEC out) {
	out[0] = a[0] - b[0];
	out[1] = a[1] - b[1];
	out[2] = a[2] - b[2];
}

INLINE float D3D_normalize(D3D_VEC in, D3D_VEC out) {
	float rmag = D3D_rmagnitude(in);
	out[0] = in[0]*rmag;
	out[1] = in[1]*rmag;
	out[2] = in[2]*rmag;
}

INLINE float D3D_copyvec(D3D_VEC a, D3D_VEC into) {
	into[0] = a[0];
	into[1] = a[1];
	into[2] = a[2];
}

INLINE float D3D_cross(D3D_VEC a, D3D_VEC b) {
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

// Signal that the camera was rotated
void D3D_rotcam();

// Move a vector relative to it's angles
// Currently only works with the object's z angle, since I need to figure out stuff.
void D3D_relmove(D3D_VEC v, D3D_VEC angles, D3D_VEC out);
// Rotates around 0,0,0.
// You can rotate around x,y,z by subtracting the vectors and then adding it back.
void D3D_rotate(D3D_VEC v, D3D_VEC angles, D3D_VEC out);

void D3D_init(UCHAR* data, int width, int height);
void D3D_draw();

// vec is both input and output
// [0] = x on screen, 0 to 1
// [1] = y on screen, 0 to 1
// [2] = depth
void D3D_rastervec(D3D_VEC vec, D3D_VEC out);
void D3D_drawpoint(D3D_VEC vec);

void D3D_free();

void D3D_addobj(D3D_OBJ* obj);


#undef INLINE
