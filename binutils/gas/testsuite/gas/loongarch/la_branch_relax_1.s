# Instruction and Relocation generating tests

.L1:
  .fill 0x123456, 4, 0x0

# R_LARCH_B16
  beq $r12, $r13, .L1
  bne $r12, $r13, .L1

  blt $r12, $r13, .L1
  bgt $r12, $r13, .L1

  bltz $r12, .L1
  bgtz $r12, .L1

  ble $r12, $r13, .L1
  bge $r12, $r13, .L1

  blez $r12, .L1
  bgez $r12, .L1

  bltu $r12, $r13, .L1
  bgtu $r12, $r13, .L1

  bleu $r12, $r13, .L1
  bgeu $r12, $r13, .L1

# R_LARCH_B21
  beqz $r12, .L1
  bnez $r12, .L1

  bceqz $fcc0, .L1
  bcnez $fcc0, .L1
