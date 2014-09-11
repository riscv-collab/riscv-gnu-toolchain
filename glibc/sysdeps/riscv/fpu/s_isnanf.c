#include "math.h"
#include "fpu_control.h"

int __isnanf(float x)
{
  return _FCLASS(x) & _FCLASS_NAN;
}
hidden_def (__isnanf)
weak_alias (__isnanf, isnanf)
