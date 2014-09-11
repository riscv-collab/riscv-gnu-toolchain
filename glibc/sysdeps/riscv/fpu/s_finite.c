#include "math.h"
#include "fpu_control.h"

int __finite(double x)
{
  return _FCLASS(x) & ~(_FCLASS_INF | _FCLASS_NAN);
}
hidden_def (__finite)
weak_alias (__finite, finite)
