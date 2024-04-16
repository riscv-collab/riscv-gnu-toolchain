# mips r6 tests (non FPU)
# mach:  mips32r6 mips64r6
# as:    -mabi=eabi
# ld:    -N -Ttext=0x80010000
# output: *\\npass\\n

  .include "testutils.inc"
  .include "utils-r6.inc"

  setup

  .data
dval1:  .word 0xabcd1234
dval2: .word 0x1234eeff
  .fill 248,1,0
dval3:	.word 0x55555555
  .fill  260,1,0
dval4:	.word 0xaaaaaaaa
  .text

  .set noreorder

  .ent DIAG
DIAG:

  writemsg "[1] Test MUL"
  r6ck_2r mul, 7, 9, 63
  r6ck_2r mul, -7, -9, 63
  r6ck_2r mul, 61, -11, -671
  r6ck_2r mul, 1001, 1234, 1235234
  r6ck_2r mul, 123456789, 999999, 0x7eb1e22b
  r6ck_2r mul, 0xaaaabbbb, 0xccccdddd, 0x56787f6f

  writemsg "[2] Test MUH"
  r6ck_2r muh, 61, -11, 0xffffffff
  r6ck_2r muh, 1001, 1234, 0
  r6ck_2r muh, 123456789, 999999, 0x7048
  r6ck_2r muh, 0xaaaabbbb, 0xccccdddd, 0x111107f7

  writemsg "[3] Test MULU"
  r6ck_2r mulu, 7, 9, 63
  r6ck_2r mulu, -7, -9, 63
  r6ck_2r mulu, 61, -11, -671
  r6ck_2r mulu, 1001, 1234, 1235234
  r6ck_2r mulu, 123456789, 999999, 0x7eb1e22b
  r6ck_2r mulu, 0xaaaabbbb, 0xccccdddd, 0x56787f6f

  writemsg "[4] Test MUHU"
  r6ck_2r muhu, 1001, 1234, 0
  r6ck_2r muhu, 123456789, 999999, 0x7048
  r6ck_2r muhu, 0xaaaabbbb, 0xccccdddd, 0x8888a18f
  r6ck_2r muhu, 0xaaaabbbb, 0xccccdddd, 0x8888a18f

  writemsg "[5] Test DIV"
  r6ck_2r div, 10001, 10, 1000
  r6ck_2r div, -123456, 560, -220
  r6ck_2r div, 9, 100, 0

  writemsg "[6] Test MOD"
  r6ck_2r mod, 10001, 10, 1
  r6ck_2r mod, -123456, 560, 0xffffff00
  r6ck_2r mod, 9, 100, 9

  writemsg "[7] Test DIVU"
  r6ck_2r divu, 10001, 10, 1000
  r6ck_2r divu, -123456, 560, 0x750674
  r6ck_2r divu, 9, 100, 0
  r6ck_2r divu, 0xaaaabbbb, 3, 0x38e393e9

  writemsg "[8] Test MODU"
  r6ck_2r modu, 10001, 10, 1
  r6ck_2r modu, -123456, 560, 0
  r6ck_2r modu, 9, 100, 9
  r6ck_2r modu, 0xaaaabbbb, 5, 4

  writemsg "[9] Test LSA"
  r6ck_2r1i lsa, 1, 2, 2, 6
  r6ck_2r1i lsa, 0x8000, 0xa000, 1, 0x1a000
  r6ck_2r1i lsa, 0x82, 0x2000068, 4, 0x2000888

  writemsg "[10] Test AUI"
  r6ck_1r1i aui, 0x0000c0de, 0xdead, 0xdeadc0de
  r6ck_1r1i aui, 0x00005678, 0x1234, 0x12345678
  r6ck_1r1i aui, 0x0000eeff, 0xabab, 0xababeeff

  writemsg "[11] Test SELEQZ"
  r6ck_2r seleqz, 0x1234, 0, 0x1234
  r6ck_2r seleqz, 0x1234, 4, 0
  r6ck_2r seleqz, 0x80010001, 0, 0x80010001

  writemsg "[12] Test SELNEZ"
  r6ck_2r selnez, 0x1234, 0, 0
  r6ck_2r selnez, 0x1234, 1, 0x1234
  r6ck_2r selnez, 0x80010001, 0xffffffff, 0x80010001

  writemsg "[13] Test ALIGN"
  r6ck_2r1i align, 0xaabbccdd, 0xeeff0011, 1, 0xff0011aa
  r6ck_2r1i align, 0xaabbccdd, 0xeeff0011, 3, 0x11aabbcc

  writemsg "[14] Test BITSWAP"
  r6ck_1r bitswap, 0xaabbccdd, 0x55dd33bb
  r6ck_1r bitswap, 0x11884422, 0x88112244

  writemsg "[15] Test CLZ"
  r6ck_1r clz, 0x00012340, 15
  r6ck_1r clz, 0x80012340, 0
  r6ck_1r clz, 0x40012340, 1

  writemsg "[16] Test CLO"
  r6ck_1r clo, 0x00123050, 0
  r6ck_1r clo, 0xff123050, 8
  r6ck_1r clo, 0x8f123050, 1

  writemsg "[17] Test ADDIUPC"
  jal GetPC
  nop
  addiu $4, $6, 8
  addiupc $5, 4
  fp_assert $4, $5

  writemsg "[18] Test AUIPC"
  jal GetPC
  nop
  addiu $4, $6, 8
  aui $4, $4, 8
  auipc $5, 8
  fp_assert $4, $5

  writemsg "[19] Test ALUIPC"
  jal GetPC
  nop
  addiu $4, $6, 16
  aui $4, $4, 8
  li $7, 0xffff0000
  and $4, $4, $7
  aluipc $5, 8
  fp_assert $4, $5

  writemsg "[20] Test LWPC"
  lw $5, dval1
  lwpc $4, dval1
  fp_assert $4, $5
  lw $5, dval2
  lwpc $4, dval2
  fp_assert $4, $5

  writemsg "[21] Test LL"
  lw $5, dval2
  la $3, dval3
  ll $4, -252($3)
  fp_assert $4, $5

  writemsg "[22] Test SC"
  ll $4, -252($3)
  li $4, 0xafaf
  sc $4, -252($3)
  lw $5, dval2
  li $4, 0xafaf
  fp_assert $4, $5

  pass

  .end DIAG
