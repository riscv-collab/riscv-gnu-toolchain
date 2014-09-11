# Artisanally coded in California by A. Shell Waterman

#include <sysdep.h>
#include <sys/asm.h>

#if BYTE_ORDER != LITTLE_ENDIAN
# error
#endif

ENTRY(strcmp)
  or    v1, a0, a1
  li    t2, -1
  and   v1, v1, SZREG-1
  li    v0, 0
  bnez  v1, .Lmisaligned

#if SZREG == 4
  li t3, 0x7f7f7f7f
#else
  ld t3, mask
#endif

  .macro check_one_word i n
    REG_L a2, \i*SZREG(a0)
    REG_L a3, \i*SZREG(a1)

    and   t0, a2, t3
    or    t1, a2, t3
    add   t0, t0, t3
    or    t0, t0, t1

    bne   t0, t2, .Lnull\i
    .if \i+1-\n
      bne   a2, a3, .Lmismatch
    .else
      add   a0, a0, \n*SZREG
      add   a1, a1, \n*SZREG
      beq   a2, a3, .Lloop
      # fall through to .Lmismatch
    .endif
  .endm

  .macro foundnull i n
    .ifne \i
      .Lnull\i:
      add   a0, a0, \i*SZREG
      add   a1, a1, \i*SZREG
      .ifeq \i-1
        .Lnull0:
      .endif
      bne   a2, a3, .Lmisaligned
      ret
    .endif
  .endm

.Lloop:
  # examine full words
  check_one_word 0 4
  check_one_word 1 4
  check_one_word 2 4
  check_one_word 3 4
  # backwards branch to .Lloop contained above

.Lmismatch:
  # words don't match, but a2 has no null byte.
#ifdef __riscv64
  sll   a0, a2, 48
  sll   a1, a3, 48
  bne   a0, a1, .Lmismatch_upper
  sll   a0, a2, 32
  sll   a1, a3, 32
  bne   a0, a1, .Lmismatch_upper
#endif
  sll   a0, a2, 16
  sll   a1, a3, 16
  bne   a0, a1, .Lmismatch_upper

  srl   a0, a2, 8*SZREG-16
  srl   a1, a3, 8*SZREG-16
  sub   v0, a0, a1
  and   v1, v0, 0xff
  bnez  v1, 1f
  ret

.Lmismatch_upper:
  srl   a0, a0, 8*SZREG-16
  srl   a1, a1, 8*SZREG-16
  sub   v0, a0, a1
  and   v1, v0, 0xff
  bnez  v1, 1f
  ret

1:and   a0, a0, 0xff
  and   a1, a1, 0xff
  sub   v0, a0, a1
  ret

.Lmisaligned:
  # misaligned
  lbu   v0, 0(a0)
  lbu   v1, 0(a1)
  add   a0, a0, 1
  add   a1, a1, 1
  bne   v0, v1, 1f
  bnez  v0, .Lmisaligned

1:
  sub   v0, v0, v1
  ret

  # cases in which a null byte was detected
  foundnull 0, 4
  foundnull 1, 4
  foundnull 2, 4
  foundnull 3, 4

END(strcmp)

weak_alias(strcmp, __GI_strcmp)

#if SZREG == 8
#ifdef __PIC__
.section .rodata.cst8,"aM",@progbits,8
#else
.section .srodata.cst8,"aM",@progbits,8
#endif
.align 3
mask: .8byte 0x7f7f7f7f7f7f7f7f
#endif
