  .text
  .align 2

#ifndef __riscv64
/* Our RV64 64-bit routine is equivalent to our RV32 32-bit routine.  */
# define __muldi3 __mulsi3
#endif

  .globl __muldi3
__muldi3:
  mv     a2, a0
  li     a0, 0
.L1:
  andi   a3, a1, 1
  beqz   a3, .L2
  add    a0, a0, a2
.L2:
  srli   a1, a1, 1
  slli   a2, a2, 1
  bnez   a1, .L1
  ret
