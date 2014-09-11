#include "math.h"

float __fabsf(float x)
{
  float res;
  asm ("fabs.s %0, %1" : "=f"(res) : "f"(x));
  return res;
}
weak_alias (__fabsf, fabsf)
