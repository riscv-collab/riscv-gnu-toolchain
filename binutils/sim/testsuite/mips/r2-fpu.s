# mips r2 fpu tests
# mach:   mips32r2 mips64r2
# as:     -mabi=eabi
# ld:     -N -Ttext=0x80010000
# output: *\\npass\\n

  .include "testutils.inc"

  setup

  .set noreorder

  .ent DIAG

DIAG:
  writemsg "[1] Test qNaN format is 754-1985"
  li $6, 0x7fbfffff
  mtc1 $0, $f2
  mtc1 $0, $f4
  div.s $f6, $f2, $f4
  mfc1 $8, $f6
  beq $8, $6, L1
  nop
  fail

  L1:
  #TODO: More tests?

  pass

  .end DIAG
