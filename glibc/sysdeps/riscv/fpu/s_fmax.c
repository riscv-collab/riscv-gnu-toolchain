#include <math.h>

double __fmax (double x, double y)
{
  double res;
  asm ("fmax.d %0, %1, %2" : "=f"(res) : "f"(x), "f"(y));
  return res;
}
weak_alias (__fmax, fmax)
