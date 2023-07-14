#include "D3D.h"
#include <stdlib.h>

#include <math.h>

// TODO: Try to make the near plane circle shaped omg.

D3D_CAMERA D3D_cam;
D3D_LIGHT* D3D_lights;
D3D_MODEL* D3D_models;

static char rotcam = 0;

static UCHAR* data;
static int w, h;

static float ratio;
static float maxdy, maxdz;

#define TABLE_SIZE 256
#define TABLE_MASK (TABLE_SIZE - 1)

float cosTable[TABLE_SIZE];
float sinTable[TABLE_SIZE];

static inline void putpx(UCHAR c, int x, int y) {
  data[x+y*w] = c;
}

static inline void setvec(D3D_VEC v, float x, float y, float z) {
  v[0] = x;
  v[1] = y;
  v[2] = z;
}

float D3D_Q_rsqrt( float number ) {
  long i;
  float x2, y;
  const float threehalfs = 1.5F;

  x2 = number * 0.5F;
  y  = number;
  i  = * ( long * ) &y;                       // evil floating point bit level hacking
  i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
  y  = * ( float * ) &i;
  y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
  // y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

  return y;
}

void D3D_init(UCHAR* _data, int width, int height) {
  data = _data;
  w = width;
  h = height;

  D3D_lights = NULL;
  D3D_models = NULL;

  setvec(D3D_cam.obj.offset, 0,0,0);
  setvec(D3D_cam.obj.angles, 0,0,0);
  D3D_cam.fov = D3D_PI/2;
  D3D_cam.far = 100;
  D3D_cam.near = 0.1f;
  D3D_cam.exposure = 127;
  D3D_cam.fog = 0;

  ratio = w/h;

  float tg = tanf(D3D_cam.fov/2) * D3D_cam.near;

  maxdy = (ratio*tg);
  maxdz = (tg);

  for (int i = 0; i < TABLE_SIZE; ++i) {
      float angle = (float)i * 2.0f * D3D_PI / TABLE_SIZE;
      cosTable[i] = cos(angle);
      sinTable[i] = sin(angle);
  }
}


// https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle
// TODO: Understand this.
static float sign (D3D_VEC p1, D3D_VEC p2, D3D_VEC p3) {
    return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1]);
}

// Triangle consists of v1 v2 v3.
static int intri (D3D_VEC pt, D3D_VEC v1, D3D_VEC v2, D3D_VEC v3) {
  float d1, d2, d3;
  int has_neg, has_pos;

  d1 = sign(pt, v1, v2);
  d2 = sign(pt, v2, v3);
  d3 = sign(pt, v3, v1);

  has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
  has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

  return !(has_neg && has_pos);
}

// dst is not the distance of the point but the distance only on the x axis.
// height is how far it is from that ideal dst center point.
// Essentially D=sqrt(height^2+dst^2)
// Rasterizes a point on the near plane from 0 to 1 in a single dimention from a 2 dimentional reference.
// So you use it once to calculate the point from the dst=x height=y reference onto the x dimention in the near plane.
// and once the dst=x dst=z for the y dimention.

/*
  So this is a little confusing, i am just dumb.
  in 3D x is the depth, y is the right left, z is the up down.
  in 2D x is the right left, y is the up down.
  
  This rasterizes a point on a single dimention
  xdst: always just the distance of the point on the x dimention.
  height: either z or y, depends on what dimention you put the
  onx: whether we
*/
static float two_d_rasterize(float xdst, float height, char onz) {
  // d is the location of the point on the near plane
  // d=0 is left, d=1 is right, d=0.5 is center
  // The point must not go outside the near plane, ofc, so we calculate the size of the plane.

  float maxd = onz?maxdz:maxdy;
  // This is to avoid squishing
  float dfactor = onz?1:ratio;

  // d/n = y/x
  float d = D3D_cam.near * dfactor * (height/xdst);
  d /= 2*maxd;

  d += 0.5; // Shift it to be from left to right, ideally.

  return d;
}

// I tried to understand rotation for 2 days myself, failed unfortunately, so gonna use the average euler angles method.
void D3D_rotate(D3D_VEC v, D3D_VEC angles, D3D_VEC out) {
    float px = v[0];
    float py = v[1];
    float pz = v[2];
    float cx = angles[0];
    float cy = angles[1];
    float cz = angles[2];

    // Compute the differences
    float dx = px - cx;
    float dy = py - cy;
    float dz = pz - cz;

    // Calculate table indices
    int indexX = (int)(cx * TABLE_SIZE / (2.0f * D3D_PI)) & TABLE_MASK;
    int indexY = (int)(cy * TABLE_SIZE / (2.0f * D3D_PI)) & TABLE_MASK;
    int indexZ = (int)(cz * TABLE_SIZE / (2.0f * D3D_PI)) & TABLE_MASK;

    // Apply rotation transformations using table lookups
    out[0] = dx * cosTable[indexY] * cosTable[indexZ] - dy * cosTable[indexY] * sinTable[indexZ] + dz * sinTable[indexY] + cx;

    out[1] = dx * (cosTable[indexX] * sinTable[indexZ] + sinTable[indexX] * sinTable[indexY] * cosTable[indexZ]) + dy * (cosTable[indexX] * cosTable[indexZ] - sinTable[indexX] * sinTable[indexY] * sinTable[indexZ]) - dz * sinTable[indexX] * cosTable[indexY] + cy;

    out[2] = dx * (sinTable[indexX] * sinTable[indexZ] - cosTable[indexX] * sinTable[indexY] * cosTable[indexZ]) + dy * (sinTable[indexX] * cosTable[indexZ] + cosTable[indexX] * sinTable[indexY] * sinTable[indexZ]) + dz * cosTable[indexX] * cosTable[indexY] + cz;
}


void D3D_rastervec(D3D_VEC in, D3D_VEC out) {
  D3D_vecsub(in, D3D_cam.obj.offset, out);

  // FIXME: SECONDARY BOTTLENECK
  // if (rotcam)
  D3D_rotate(out, D3D_cam.obj.angles, out);

  // If behind near or farther than far plane ofc it wont work.
  if (in[0] < D3D_cam.near) {
    out[0] = out[1] = out[2] = -1;
    return;
  }

  // FIXME: PRIMARY BOTTLENECK
  float fx = two_d_rasterize(out[0], out[1], 1);
  float fy = two_d_rasterize(out[0], out[2], 0);

  out[2] = (in[0]-D3D_cam.near)/(D3D_cam.far-D3D_cam.near);
  out[1] = fy;
  out[0] = fx;
}

int testinview2d(D3D_VEC p) {
  if (p[0] < .0f || p[0] > 1.0f)
    return 0;
  
  if (p[1] < .0f || p[1] > 1.0f)
    return 0;

  if (p[2] < .0f)
    return 0;
  
  return 1;
}

int testinview3d(D3D_VEC p) {
  if (p[0] < D3D_cam.near || p[0] > D3D_cam.far)
    return 0;
  
  return 1;
}

void D3D_drawpoint(D3D_VEC vec) {
  D3D_VEC p;
  D3D_rastervec(vec, p);

  if (testinview2d(p))
    putpx(2, p[0]*w, p[1]*h);
}

static int drawmodel(D3D_VEC cursor, D3D_MODEL* mdl) {
  D3D_VEC a, b, c;
  for (int ti = 0; ti < mdl->trisn; ti++) {
    // FIXME: BOTTLENECK
    D3D_rastervec(mdl->vecs[mdl->tris[ti][0]], a);
    D3D_rastervec(mdl->vecs[mdl->tris[ti][1]], b);
    D3D_rastervec(mdl->vecs[mdl->tris[ti][2]], c);

    // NOT THE BOTTOLENECK AT ALL!
    int inside = intri(cursor, a, b, c);

    if (inside)
      return inside;
  }
  return 0;
}

void D3D_draw() {
  // maxdz = tanf(D3D_cam.fov/2) * D3D_cam.near * (h/w);

  // float jmpx = 1/w, jmpy = 1;
  D3D_VEC cursor = {0,0,0};

  for (float x = 0; x < w; x++) {
    for (float y = 0; y < h; y++) {
      for (D3D_MODEL* mdl = D3D_models; mdl; mdl = mdl->next) {
        cursor[0] = x/w;
        cursor[1] = y/h;
        // printf("%f\n", y);
        if (drawmodel(cursor, mdl))
          putpx(rand()%8, x, y);
      }
    }
  }
}