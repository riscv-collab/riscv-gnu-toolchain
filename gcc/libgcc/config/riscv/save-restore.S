  .text

  .globl __riscv_save_12
  .globl __riscv_save_11
  .globl __riscv_save_10
  .globl __riscv_save_9
  .globl __riscv_save_8
  .globl __riscv_save_7
  .globl __riscv_save_6
  .globl __riscv_save_5
  .globl __riscv_save_4
  .globl __riscv_save_3
  .globl __riscv_save_2
  .globl __riscv_save_1
  .globl __riscv_save_0

  .globl __riscv_restore_12
  .globl __riscv_restore_11
  .globl __riscv_restore_10
  .globl __riscv_restore_9
  .globl __riscv_restore_8
  .globl __riscv_restore_7
  .globl __riscv_restore_6
  .globl __riscv_restore_5
  .globl __riscv_restore_4
  .globl __riscv_restore_3
  .globl __riscv_restore_2
  .globl __riscv_restore_1
  .globl __riscv_restore_0

#ifdef __riscv64

__riscv_save_12:
  addi sp, sp, -112
  li t1, 0
  sd s11, 8(sp)
  j .Ls10

__riscv_save_11:
__riscv_save_10:
  addi sp, sp, -112
  li t1, -16
.Ls10:
  sd s10, 16(sp)
  sd s9, 24(sp)
  j .Ls8

__riscv_save_9:
__riscv_save_8:
  addi sp, sp, -112
  li t1, -32
.Ls8:
  sd s8, 32(sp)
  sd s7, 40(sp)
  j .Ls6

__riscv_save_7:
__riscv_save_6:
  addi sp, sp, -112
  li t1, -48
.Ls6:
  sd s6, 48(sp)
  sd s5, 56(sp)
  j .Ls4

__riscv_save_5:
__riscv_save_4:
  addi sp, sp, -112
  li t1, -64
.Ls4:
  sd s4, 64(sp)
  sd s3, 72(sp)
  j .Ls2

__riscv_save_3:
__riscv_save_2:
  addi sp, sp, -112
  li t1, -80
.Ls2:
  sd s2, 80(sp)
  sd s1, 88(sp)
  sd s0, 96(sp)
  sd ra, 104(sp)
  sub sp, sp, t1
  jr t0

__riscv_save_1:
__riscv_save_0:
  addi sp, sp, -16
  sd s0, 0(sp)
  sd ra, 8(sp)
  jr t0

__riscv_restore_12:
  ld s11, 8(sp)
  addi sp, sp, 16

__riscv_restore_11:
__riscv_restore_10:
  ld s10, 0(sp)
  ld s9, 8(sp)
  addi sp, sp, 16

__riscv_restore_9:
__riscv_restore_8:
  ld s8, 0(sp)
  ld s7, 8(sp)
  addi sp, sp, 16

__riscv_restore_7:
__riscv_restore_6:
  ld s6, 0(sp)
  ld s5, 8(sp)
  addi sp, sp, 16

__riscv_restore_5:
__riscv_restore_4:
  ld s4, 0(sp)
  ld s3, 8(sp)
  addi sp, sp, 16

__riscv_restore_3:
__riscv_restore_2:
  ld s2, 0(sp)
  ld s1, 8(sp)
  addi sp, sp, 16

__riscv_restore_1:
__riscv_restore_0:
  ld s0, 0(sp)
  ld ra, 8(sp)
  addi sp, sp, 16
  ret

#else

__riscv_save_12:
  addi sp, sp, -64
  li t1, 0
  sw s11, 12(sp)
  j .Ls10

__riscv_save_11:
__riscv_save_10:
__riscv_save_9:
__riscv_save_8:
  addi sp, sp, -64
  li t1, -16
.Ls10:
  sw s10, 16(sp)
  sw s9, 20(sp)
  sw s8, 24(sp)
  sw s7, 28(sp)
  j .Ls6

__riscv_save_7:
__riscv_save_6:
__riscv_save_5:
__riscv_save_4:
  addi sp, sp, -64
  li t1, -32
.Ls6:
  sw s6, 32(sp)
  sw s5, 36(sp)
  sw s4, 40(sp)
  sw s3, 44(sp)
  sw s2, 48(sp)
  sw s1, 52(sp)
  sw s0, 56(sp)
  sw ra, 60(sp)
  sub sp, sp, t1
  jr t0

__riscv_save_3:
__riscv_save_2:
__riscv_save_1:
__riscv_save_0:
  addi sp, sp, -16
  sw s2, 0(sp)
  sw s1, 4(sp)
  sw s0, 8(sp)
  sw ra, 12(sp)
  jr t0

__riscv_restore_12:
  lw s11, 12(sp)
  addi sp, sp, 16

__riscv_restore_11:
__riscv_restore_10:
__riscv_restore_9:
__riscv_restore_8:
  lw s10, 0(sp)
  lw s9, 4(sp)
  lw s8, 8(sp)
  lw s7, 12(sp)
  addi sp, sp, 16

__riscv_restore_7:
__riscv_restore_6:
__riscv_restore_5:
__riscv_restore_4:
  lw s6, 0(sp)
  lw s5, 4(sp)
  lw s4, 8(sp)
  lw s3, 12(sp)
  addi sp, sp, 16

__riscv_restore_3:
__riscv_restore_2:
__riscv_restore_1:
__riscv_restore_0:
  lw s2, 0(sp)
  lw s1, 4(sp)
  lw s0, 8(sp)
  lw ra, 12(sp)
  addi sp, sp, 16
  ret

#endif
