#include "math.h"
#include "fpu_control.h"

int __isnan(double x)
{
  return _FCLASS(x) & _FCLASS_NAN;
}
hidden_def (__isnan)
weak_alias (__isnan, isnan)
