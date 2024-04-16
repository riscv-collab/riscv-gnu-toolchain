# mips r6 branch tests (non FPU)
# mach: mips32r6 mips64r6
# as:   -mabi=eabi
# ld:   -N -Ttext=0x80010000
# output: *\\npass\\n

  .include "testutils.inc"
  .include "utils-r6.inc"

  setup

  .set noreorder

  .ent DIAG
DIAG:
  li $14, 0xffffffff
  li $13, 0x123
  li $12, 0x45
  li $7, 0x45
  li $8, 0xfffffffe
  li $9, 2147483647
  li $11, 0

  writemsg "[1] Test BOVC"
  bovc $12, $13, Lfail
  nop
  bovc $9, $13, L2
  nop
  fail

L2:
  writemsg "[2] Test BNVC"
  bnvc $9, $13, Lfail
  nop
  bnvc $12, $13, L3
  nop
  fail

L3:
  writemsg "[3] Test BEQC"
  beqc $12, $13, Lfail
  nop
  beqc $12, $7, L4
  nop
  fail

L4:
  writemsg "[4] Test BNEC"
  bnec $12, $7, Lfail
  nop
  bnec $12, $13, L5
  nop
  fail

L5:
  writemsg "[5] Test BLTC"
  bltc $13, $12, Lfail
  nop
  bltc $12, $13, L6
  nop
  fail

L6:
#  writemsg "[6] Test BLEC"
#  blec $13, $12, Lfail
#  nop
#  blec $7, $12, L7
#  nop
#  fail

L7:
  writemsg "[7] Test BGEC"
  bgec $12, $13, Lfail
  nop
  bgec $13, $12, L8
  nop
  fail

L8:
#  writemsg "[8] Test BGTC"
#  bgtc $12, $13, Lfail
#  nop
#  bgtc $13, $12, L9
#  nop
#  fail


L9:
  writemsg "[9] Test BLTUC"
  bltuc $14, $13, Lfail
  nop
  bltuc $8, $14, L10
  nop
  fail

L10:
#  writemsg "[10] Test BLEUC"
#  bleuc $14, $13, Lfail
#  nop
#  bleuc $8, $14, L11
#  nop
#  fail

L11:
  writemsg "[11] Test BGEUC"
  bgeuc $13, $14, Lfail
  nop
  bgeuc $14, $8, L12
  nop
  fail

L12:
#  writemsg "[12] Test BGTUC"
#  bgtuc $13, $14, Lfail
#  nop
#  bgtuc $14, $8, L13
#  nop
#  fail

L13:
  writemsg "[13] Test BLTZC"
  bltzc $13, Lfail
  nop
  bltzc $11, Lfail
  nop
  bltzc $14, L14
  nop
  fail

L14:
  writemsg "[14] Test BLEZC"
  blezc $13, Lfail
  nop
  blezc $11, L145
  nop
  fail
L145:
  blezc $14, L15
  nop
  fail

L15:
  writemsg "[15] Test BGEZC"
  bgezc $8, Lfail
  nop
  bgezc $11, L155
  nop
  fail
L155:
  bgezc $13, L16
  nop
  fail

L16:
  writemsg "[16] Test BGTZC"
  bgtzc $8, Lfail
  nop
  bgtzc $11, Lfail
  nop
  bgtzc $13, L17
  nop
  fail

  li $10, 0

L17:
  writemsg "[17] Test BLEZALC"
  blezalc $12, Lfail
  nop
  blezalc $11, Lret
  li $10, 1
  beqzc $10, L175
  nop
  fail
L175:
  blezalc $14, Lret
  li $10, 1
  beqzc $10, L18
  nop
  fail

L18:
  writemsg "[18] Test BGEZALC"
  bgezalc $14, Lfail
  nop
  bgezalc $11, Lret
  li $10, 1
  beqzc $10, L185
  nop
  fail
L185:
  bgezalc $12, Lret
  li $10, 1
  beqzc $10, L19
  nop
  fail

L19:
  writemsg "[19] Test BGTZALC"
  bgtzalc $14, Lfail
  nop
  bgtzalc $11, Lfail
  nop
  bgtzalc $12, Lret
  li $10, 1
  beqzc $10, L20
  nop
  fail

L20:
  writemsg "[20] Test BLTZALC"
  bltzalc $12, Lfail
  nop
  bltzalc $11, Lfail
  nop
  bltzalc $14, Lret
  li $10, 1
  beqzc $10, L21
  nop
  fail

L21:
  writemsg "[21] Test BC"
  bc L22
  fail

L22:
  writemsg "[22] Test BALC"
  balc Lret
  li $10, 1
  beqzc $10, L23
  nop
  fail

L23:
  writemsg "[23] Test JIC"
  jal GetPC
  nop
  jic $6, 4
  nop
  fail

L24:
  writemsg "[24] Test JIALC"
  li $10, 1
  jal GetPC
  nop
  jialc $6, 20
  nop
  beqzc $10, L25
  nop
  fail

LJIALCRET:
  li $10, 0
  jr $ra
  nop

L25:
  writemsg "[25] Test NAL"
  jal GetPC
  nop
  move $11, $6
  nal
  nop
  addiu $11, 12
  beqc $11, $31, L26
  nop
  fail

L26:
  writemsg "[26] Test BAL"
  balc Lret
  li $10, 1
  beqzc $10, Lend
  nop
  fail

Lend:
  pass

Lfail:
  fail

  .end DIAG

Lret:
  li $10, 0
  addiu $ra, 4
  jr $ra
  nop
