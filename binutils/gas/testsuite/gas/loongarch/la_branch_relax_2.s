# Immediate boundary value tests

.L1:
  .fill 0x8000, 4, 0
  beq $r12, $r13, .L1 # min imm -0x20000
  beq $r12, $r13, .L1 # out of range
  beq $r12, $r13, .L2 # out of range
  beq $r12, $r13, .L2 # max imm 0x1fffc
  .fill 0x7ffe, 4, 0
.L2:
  .fill 0x100000, 4, 0
  beqz $r12, .L2 # min imm -0x400000
  beqz $r12, .L2 # out of range
  beqz $r12, .L3 # out of range
  beqz $r12, .L3 # max imm 0x3ffffc
  .fill 0xffffe, 4, 0
.L3:

# 0 imm
.L4:
  beq $r12, $r13, .L4
.L5:
  beqz $r12, .L5
