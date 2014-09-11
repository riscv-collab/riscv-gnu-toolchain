#include <math.h>
#include <fenv.h>
#include <ieee754.h>

float __fmaf (float x, float y, float z)
{
  float out;
  asm volatile ("fmadd.s %0, %1, %2, %3" : "=f"(out) : "f"(x), "f"(y), "f"(z));
  return out;
}
weak_alias (__fmaf, fmaf)
