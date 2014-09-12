# memset, optimized for RISC-V
# Andrew Waterman
# 2012/10/15

.text
.global memset
memset:
  li a6, 7
  move v0, a0
  bleu a2, a6, .tiny
  and a5, a0, 7
  bnez a5, .misaligned

.aligned:
  bnez a1, .wordify

.wordified:
  add a3, a2, -16
  bltu a3, a2, .block16

.8to15:
#ifdef __riscv64
  sd a1, 0(a0)
#else
  sw a1, 0(a0)
  sw a1, 4(a0)
#endif
  add a0, a0, 8
  add a2, a2, -8
  bnez a2, .tiny
  ret

.block16:
  add a5, a0, a2
  add a3, a3, a0
#ifdef __riscv64
1:sd a1, 0(a0)
  sd a1, 8(a0)
#else
1:sw a1, 0(a0)
  sw a1, 4(a0)
  sw a1, 8(a0)
  sw a1, 12(a0)
#endif
  add a0, a0, 16
  bleu a0, a3, 1b

  bne a0, a5, .more
  ret
.more:
  sub a2, a5, a0
  bgtu a2, a6, .8to15
  /* Fall through */

.tiny:
  sub a3, a6, a2
  sll a3, a3, 2
.option push
.option norvc
  lui a4, %hi(1f)
  add a3, a3, a4
  jalr x0, a3, %lo(1f)

1:sb a1, 6(a0)
  sb a1, 5(a0)
  sb a1, 4(a0)
  sb a1, 3(a0)
  sb a1, 2(a0)
  sb a1, 1(a0)
  sb a1, 0(a0)
  ret
.option pop

.wordify:
  and a1, a1, 0xFF
  sll a3, a1, 8
  or  a1, a1, a3
  sll a3, a1, 16
  or  a1, a1, a3
#ifdef __riscv64
  sll a3, a1, 32
  or  a1, a1, a3
#endif
  j .wordified

.misaligned:
  sll a3, a5, 2
.option push
.option norvc
  lui a4, %hi(1f)
  add a3, a3, a4
1:jalr x0, a3, %lo(1b)

  sb a1, 6(a0)
  sb a1, 5(a0)
  sb a1, 4(a0)
  sb a1, 3(a0)
  sb a1, 2(a0)
  sb a1, 1(a0)
  sb a1, 0(a0)
  add a5, a5, -8
  sub a0, a0, a5
  add a2, a2, a5
  bleu a2, a6, .tiny
  j .aligned
.option pop
