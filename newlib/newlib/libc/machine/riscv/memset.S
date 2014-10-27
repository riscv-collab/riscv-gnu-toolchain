# memset, optimized for RISC-V
# Andrew Waterman
# 2012/10/15

.text
.global memset
memset:
  li a6, 15
  move a4, a0
  bleu a2, a6, .Ltiny
  and a5, a4, 15
  bnez a5, .Lmisaligned

.Laligned:
  bnez a1, .Lwordify

.Lwordified:
  and a3, a2, ~15
  and a2, a2, 15
  add a3, a3, a4

#ifdef __riscv64
1:sd a1, 0(a4)
  sd a1, 8(a4)
#else
1:sw a1, 0(a4)
  sw a1, 4(a4)
  sw a1, 8(a4)
  sw a1, 12(a4)
#endif
  add a4, a4, 16
  bltu a4, a3, 1b

  bnez a2, .Ltiny
  ret

.Ltiny:
  sub a3, a6, a2
  sll a3, a3, 2
1:auipc t0, %pcrel_hi(.Ltable)
  add a3, a3, t0
.option push
.option norvc
.Ltable_misaligned:
  jr a3, %pcrel_lo(1b)
.Ltable:
  sb a1,14(a4)
  sb a1,13(a4)
  sb a1,12(a4)
  sb a1,11(a4)
  sb a1,10(a4)
  sb a1, 9(a4)
  sb a1, 8(a4)
  sb a1, 7(a4)
  sb a1, 6(a4)
  sb a1, 5(a4)
  sb a1, 4(a4)
  sb a1, 3(a4)
  sb a1, 2(a4)
  sb a1, 1(a4)
  sb a1, 0(a4)
.option pop
  ret

.Lwordify:
  and a1, a1, 0xFF
  sll a3, a1, 8
  or  a1, a1, a3
  sll a3, a1, 16
  or  a1, a1, a3
#ifdef __riscv64
  sll a3, a1, 32
  or  a1, a1, a3
#endif
  j .Lwordified

.Lmisaligned:
  sll a3, a5, 2
1:auipc t0, %pcrel_hi(.Ltable_misaligned)
  add a3, a3, t0
  mv t0, ra
  jalr a3, %pcrel_lo(1b)
  mv ra, t0

  add a5, a5, -16
  sub a4, a4, a5
  add a2, a2, a5
  bleu a2, a6, .Ltiny
  j .Laligned
