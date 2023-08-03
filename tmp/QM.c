#include <stdlib.h>
#include <stdio.h>

#include "QM.h"

static int sintablesize;
// A value we use to normalize the index in the sine table
static int magicpi;

static float* sintable;

void QM_init(int _trigtablesize) {
  sintablesize = _trigtablesize;
  magicpi = sintablesize/(sintablesize*2*QM_PI);

  sintable = malloc(sizeof(float) * sintablesize);

  float fjump = 2*QM_PI/(sintablesize-1);
  float f = 0;
  for (int i = 0; i < sintablesize; i++) {
    sintable[i] = sinf(f);
    // printf("%f\n", trigtable[i]);
    f+=fjump;
  }
  printf("QM: Quick Math library initialized. Table is from %1.3f to %1.3f.\n", sintable[0], sintable[sintablesize-1]);
}

float QM_rsqrt(float x) {
  int i;
  float x2, y;
  const float threehalfs = 1.5f;

  x2 = x * 0.5f;
  y  = x;
  i  = *(int*) &y;
  i  = 0x5f3759df - ( i >> 1 );
  y  = *(float*) &i;
  y  = y * ( threehalfs - ( x2 * y * y ) ); 

  return y;
}

float QM_sin(float x) {
  x /= QM_PI*2; // Makes 2pi radians(full cycle) into 1 unit.
  int i = abs(sintablesize*x);
  float ret = sintable[i%sintablesize];
  // When x<0 it's just x>0 but reveresed, that's how sin works.
  if (x > 0)
    return ret;
  return -ret;
}

int main() {
  QM_init(256);

  float x = 0;
  for (unsigned i = 0; i < 99999999; i++) {
    float y = (rand()%1000000)/1000.0f;
    float res = sinf(y);
    x += res;
  }
  printf("%f\n", x);
  return 0;
}
