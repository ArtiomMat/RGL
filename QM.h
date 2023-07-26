#pragma once

#include <math.h>

// Quick Math library. sacrifices Nasa level accuracy for the best speed possibly possible.

/**
 * QM standard:
 * If there is scalar output that requires no pointers it will be via the return value of the function.
 * If there is multi dimentional output via pointers, it is through a parameter.
 * Note: QM_V is an array(passed as pointer to functions), not a struct.
*/

#define QM_INLINE inline __attribute__((always_inline))

#define QM_PI 3.14159f

typedef float QM_V[3];

float QM_rsqrt(float x);
float QM_cos(float x);
float QM_sin(float x);
float QM_cos(float x);

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
void QM_cross(QM_V a, QM_V b, QM_V out);
QM_INLINE float QM_magnitude(QM_V a) {
  return sqrtf(QM_dot(a, a));
}
float QM_normalize(QM_V x);
void QM_rotate(QM_V a, QM_V angles, QM_V out);
