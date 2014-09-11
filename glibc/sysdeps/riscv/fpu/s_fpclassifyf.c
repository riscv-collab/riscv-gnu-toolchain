#include <math.h>
#include "fpu_control.h"
#include "math_private.h"

int __fpclassifyf (float x)
{
  int cls = _FCLASS(x);
  if (__builtin_expect(cls & _FCLASS_NORM, _FCLASS_NORM))
    return FP_NORMAL;
  if (__builtin_expect(cls & _FCLASS_ZERO, _FCLASS_ZERO))
    return FP_ZERO;
  if (__builtin_expect(cls & _FCLASS_SUBNORM, _FCLASS_SUBNORM))
    return FP_SUBNORMAL;
  if (__builtin_expect(cls & _FCLASS_INF, _FCLASS_INF))
    return FP_INFINITE;
  return FP_NAN;
}
libm_hidden_def (__fpclassifyf)
