#include "math.h"

double __fabs(double x)
{
  double res;
  asm ("fabs.d %0, %1" : "=f"(res) : "f"(x));
  return res;
}
weak_alias (__fabs, fabs)
