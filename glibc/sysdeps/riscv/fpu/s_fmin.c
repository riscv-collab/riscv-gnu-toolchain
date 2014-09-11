#include <math.h>

double __fmin (double x, double y)
{
  double res;
  asm ("fmin.d %0, %1, %2" : "=f"(res) : "f"(x), "f"(y));
  return res;
}
weak_alias (__fmin, fmin)
