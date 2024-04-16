# mips r6 test for forbidden slot behaviour
# mach: mips32r6 mips64r6
# as:   -mabi=eabi
# ld:   -N -Ttext=0x80010000
# output: *\\nReservedInstruction at PC = *\\nprogram stopped with signal 4 (Illegal instruction).\\n
# xerror:

  .include "testutils.inc"

  setup

  .set noreorder

  .ent DIAG
DIAG:

  writemsg "[1] Test if FS is ignored when branch is taken"
  li $4, 0
  beqzalc $4, L1
  bc L2

L2:
  fail

L1:
  writemsg "[2] Test if FS is used when branch is not taken"
  li $4, 1
  blezc $4, L3
  addiu $4, $4, 1
  li $2, 2
  beq $4, $2, L4

L3:
  nop
  fail

L4:
  writemsg "[3] Test if FS causes an error when it contains a branch"
  li $4, 3
  beqzalc $4, L6
  bc L5

L5:
  nop
  fail

L6:
  #There is no passing condition here, all routes to the end indicate failure
  fail

  .end DIAG
