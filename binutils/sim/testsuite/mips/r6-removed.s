# Tests the instructions removed in R6 are correctly invalidated
# mach: mips32r6 mips64r6
# as:   -mabi=eabi
# ld:   -N -Ttext=0x80010000
# output: ReservedInstruction at PC = *\nprogram stopped with signal 4 (Illegal instruction).\n
# xerror:

  .include "testutils.inc"
  .include "r6-removed.inc"

  setup

  .set noreorder
  .ent DIAG
DIAG:
  removed_instr
  fail
  .end DIAG
