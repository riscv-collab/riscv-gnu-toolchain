#include <ieeefp.h>

#ifdef __riscv_hard_float
static void fssr(int value)
{
  asm volatile ("fssr %0" :: "r"(value));
}

static int frsr()
{
  int value;
  asm volatile ("frsr %0" : "=r"(value));
  return value;
}
#endif

fp_except fpgetmask(void)
{
  return 0;
}

fp_rnd fpgetround(void)
{
#ifdef __riscv_hard_float
  int rm = frsr() >> 5;
  return rm == 0 ? FP_RN : rm == 1 ? FP_RZ : rm == 2 ? FP_RM : FP_RP;
#else
  return FP_RZ;
#endif
}

fp_except fpgetsticky(void)
{
#ifdef __riscv_hard_float
  return frsr() & 0x1f;
#else
  return 0;
#endif
}

fp_except fpsetmask(fp_except mask)
{
  return -1;
}

fp_rnd fpsetround(fp_rnd rnd_dir)
{
#ifdef __riscv_hard_float
  int fsr = frsr();
  int rm = fsr >> 5;
  int new_rm = rnd_dir == FP_RN ? 0 : rnd_dir == FP_RZ ? 1 : rnd_dir == FP_RM ? 2 : 3;
  fssr(new_rm << 5 | fsr & 0x1f);
  return rm == 0 ? FP_RN : rm == 1 ? FP_RZ : rm == 2 ? FP_RM : FP_RP;
#else
  return -1;
#endif
}

fp_except fpsetsticky(fp_except sticky)
{
#ifdef __riscv_hard_float
  int fsr = frsr();
  fssr(sticky & 0x1f | fsr & ~0x1f);
  return fsr & 0x1f;
#else
  return -1;
#endif
}
