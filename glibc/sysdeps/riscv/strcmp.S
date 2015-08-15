# Artisanally coded in California by A. Shell Waterman

#include <sysdep.h>
#include <sys/asm.h>

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
# error
#endif

ENTRY(strcmp)
  or    a4, a0, a1
  li    t2, -1
  and   a4, a4, SZREG-1
  bnez  a4, .Lmisaligned

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
      li    a0, 0
      ret
    .endif
  .endm

.Lloop:
  # examine full words
  check_one_word 0 3
  check_one_word 1 3
  check_one_word 2 3
  # backwards branch to .Lloop contained above

.Lmismatch:
  # words don't match, but a2 has no null byte.
#ifdef __riscv64
  sll   a4, a2, 48
  sll   a5, a3, 48
  bne   a4, a5, .Lmismatch_upper
  sll   a4, a2, 32
  sll   a5, a3, 32
  bne   a4, a5, .Lmismatch_upper
#endif
  sll   a4, a2, 16
  sll   a5, a3, 16
  bne   a4, a5, .Lmismatch_upper

  srl   a4, a2, 8*SZREG-16
  srl   a5, a3, 8*SZREG-16
  sub   a0, a4, a5
  and   a1, a0, 0xff
  bnez  a1, 1f
  ret

.Lmismatch_upper:
  srl   a4, a4, 8*SZREG-16
  srl   a5, a5, 8*SZREG-16
  sub   a0, a4, a5
  and   a1, a0, 0xff
  bnez  a1, 1f
  ret

1:and   a4, a4, 0xff
  and   a5, a5, 0xff
  sub   a0, a4, a5
  ret

.Lmisaligned:
  # misaligned
  lbu   a2, 0(a0)
  lbu   a3, 0(a1)
  add   a0, a0, 1
  add   a1, a1, 1
  bne   a2, a3, 1f
  bnez  a2, .Lmisaligned

1:
  sub   a0, a2, a3
  ret

  # cases in which a null byte was detected
  foundnull 0, 3
  foundnull 1, 3
  foundnull 2, 3

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
