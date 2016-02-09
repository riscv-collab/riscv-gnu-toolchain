  .text
  .align 2

#ifndef __riscv64
/* Our RV64 64-bit routines are equivalent to our RV32 32-bit routines.  */
# define __multi3 __muldi3
#endif

  .globl __multi3
__multi3:

#ifndef __riscv64
/* Our RV64 64-bit routines are equivalent to our RV32 32-bit routines.  */
# define __muldi3 __mulsi3
#endif

/* We rely on the fact that __muldi3 doesn't clobber the t-registers.  */

  mv  t0, ra
  mv  t5, a0
  mv  a0, a1
  mv  t6, a3
  mv  a1, t5
  mv  a4, a2
  li  a5, 0
  li  t2, 0
  li  t4, 0
.L1:
  add  a6, t2, a1
  andi t3, a4, 1
  slli a7, a5, 1
  slti t1, a1, 0
  srli a4, a4, 1
  add  a5, t4, a5
  beqz t3, .L2
  sltu t3, a6, t2
  mv   t2, a6
  add  t4, t3, a5
.L2:
  slli a1, a1, 1
  or   a5, t1, a7
  bnez a4, .L1
  beqz a0, .L3
  mv   a1, a2
  call __muldi3
  add  t4, t4, a0
.L3:
  beqz t6, .L4
  mv   a1, t6
  mv   a0, t5
  call  __muldi3
  add  t4, t4, a0
.L4:
  mv  a0, t2
  mv  a1, t4
  jr  t0
