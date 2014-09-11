#include "math.h"

float __copysignf(float x, float y)
{
  float res;
  asm ("fsgnj.s %0, %1, %2" : "=f"(res) : "f"(x), "f"(y));
  return res;
}
weak_alias (__copysignf, copysignf)
