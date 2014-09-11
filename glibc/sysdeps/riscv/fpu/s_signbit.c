#include <features.h>
#undef __USE_EXTERN_INLINES
#include <math.h>
#include <stdint.h>
#include "math_private.h"

int __signbit (double x)
{
#ifdef __riscv64
  int64_t hx;
  EXTRACT_WORDS64 (hx, x);
  return hx < 0;
#else
  int32_t hx;
  GET_HIGH_WORD (hx, x);
  return hx < 0;
#endif
}
