#pragma once

#include <math.h>

// Quick Math library - sacrifices Nasa level accuracy for the best speed possibly possible.

/**
 * QM standard:
 * If there is scalar output that requires no pointers it will be via the return value of the function.
 * If there is multi dimentional output via pointers, it is through a parameter.
 * Note: QM_V is an array(passed as pointer to functions), not a struct.
*/

#define QM_INLINE inline __attribute__((always_inline))

#define QM_PI 3.14159f

typedef float QM_V[3];

// QM initializes a table to calculate cos and sin and tan, the table's range is between 0 and 2pi, the size is essentially the resolution.
void QM_init(int trigtablesize);

float QM_rsqrt(float x);
float QM_sin(float x);
QM_INLINE float QM_cos(float x) {
  QM_sin(x+QM_PI/2);
}
float QM_tan(float x);

QM_INLINE void QM_setv(float what, QM_V out) {
  out[0] = what;
  out[1] = what;
  out[2] = what;
}
QM_INLINE void QM_copyv(QM_V what, QM_V out){
  out[0] = what[0];
  out[1] = what[1];
  out[2] = what[2];
}

QM_INLINE float QM_dot(QM_V a, QM_V b) {
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}
QM_INLINE void QM_cross(QM_V a, QM_V b, QM_V out) {
  out[0] = a[1] * b[2] - a[2] * b[1];
  out[1] = a[2] * b[0] - a[0] * b[2];
  out[2] = a[0] * b[1] - a[1] * b[0];
}
// For normalization use QM_normalize
QM_INLINE float QM_magnitude(QM_V a) {
  return sqrtf(QM_dot(a, a));
}
QM_INLINE void QM_normalize(QM_V x, QM_V out) {
  float rmag = QM_rsqrt(QM_dot(x,x));
  out[0] = x[0]/rmag;
  out[1] = x[1]/rmag;
  out[2] = x[2]/rmag;
}
// a is angles, p is the point.
QM_INLINE void QM_rotate(QM_V p, QM_V a, QM_V out) {
  // Around Z
  out[0] = p[0] * QM_cos(a[2]) - p[1] * QM_sin(a[2]);
  out[1] = p[0] * QM_sin(a[2]) + p[1] * QM_cos(a[2]);

  // Around Y
  p[0] = out[0];
  out[0] =  p[0] * QM_cos(a[1]) + p[2] * QM_sin(a[1]);
  out[2] = -p[0] * QM_sin(a[1]) + p[2] * QM_cos(a[1]);

  // Around X
  p[2] = out[2];
  p[1] = out[1];
  out[1] = p[1] * QM_cos(a[0]) - p[2] * QM_sin(a[0]);
  out[2] = p[1] * QM_sin(a[0]) + p[2] * QM_cos(a[0]);
}
