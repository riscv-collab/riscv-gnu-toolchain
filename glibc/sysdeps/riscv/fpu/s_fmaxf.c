#include <math.h>

float __fmaxf (float x, float y)
{
  float res;
  asm ("fmax.s %0, %1, %2" : "=f"(res) : "f"(x), "f"(y));
  return res;
}
weak_alias (__fmaxf, fmaxf)
