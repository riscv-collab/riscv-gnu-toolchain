#include <math.h>

float __fminf (float x, float y)
{
  float res;
  asm ("fmin.s %0, %1, %2" : "=f"(res) : "f"(x), "f"(y));
  return res;
}
weak_alias (__fminf, fminf)
