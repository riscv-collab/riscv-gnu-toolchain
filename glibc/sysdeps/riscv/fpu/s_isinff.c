#include "math.h"
#include "fpu_control.h"

int __isinff(float x)
{
  int cls = _FCLASS(x);
  return -((cls & _FCLASS_MINF) ? 1 : 0) | ((cls & _FCLASS_PINF) ? 1 : 0);
}
hidden_def (__isinff)
weak_alias (__isinff, isinff)
