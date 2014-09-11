#include <features.h>
#undef __USE_EXTERN_INLINES
#include <math.h>
#include <stdint.h>
#include "math_private.h"

int __signbitf (float x)
{
  int32_t hx;
  GET_FLOAT_WORD (hx, x);
  return hx < 0;
}
