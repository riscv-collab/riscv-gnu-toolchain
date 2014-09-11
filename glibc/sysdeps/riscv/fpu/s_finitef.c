#include "math.h"
#include "fpu_control.h"

int __finitef(float x)
{
  return _FCLASS(x) & ~(_FCLASS_INF | _FCLASS_NAN);
}
hidden_def (__finitef)
weak_alias (__finitef, finitef)
