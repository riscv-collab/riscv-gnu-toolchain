#include <math.h>

float __fmaxf (float x, float y)
{
#ifdef __riscv_soft_float
  if (isnan(x))
    return y;
  if (isnan(y))
    return x;
  return (x > y) ? x : y;
#else
  float res;
  asm ("fmax.s %0, %1, %2" : "=f"(res) : "f"(x), "f"(y));
  return res;
#endif
}
weak_alias (__fmaxf, fmaxf)
