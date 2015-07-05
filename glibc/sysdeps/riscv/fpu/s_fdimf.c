#include <errno.h>
#include <math.h>
#include "fpu_control.h"

float __fdimf (float x, float y)
{
  float diff = x - y;
  
  if (x <= y)
    return 0.0f;

#ifdef __riscv_soft_float
  if (isinf(diff))
    errno = ERANGE;
#else
  if (__builtin_expect(_FCLASS(diff) & _FCLASS_INF, 0))
    errno = ERANGE;
#endif

  return diff;
}
weak_alias (__fdimf, fdimf)
