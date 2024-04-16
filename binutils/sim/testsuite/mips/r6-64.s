# mips64 specific r6 tests (non FPU)
# mach:  mips64r6
# as:    -mabi=eabi
# ld:    -N -Ttext=0x80010000 -Tdata=0x80020000
# output: *\\npass\\n

  .include "testutils.inc"
  .include "utils-r6.inc"

  .data
d0:   .dword 0
dval: .dword 0xaa55bb66cc77dd88
d1:   .dword 0xaaaabbbbccccdddd
d2:   .dword 256
dlo:  .dword 0xaabbbbccccdddd00
dhi:  .dword 0xffffffffffffffaa
dhiu: .dword 0x00000000000000aa
d3:   .dword 0xffaaaabbbbccccde
d4:   .dword 0xffffffffffffffdd
d5:   .dword 0x00000000000000dd
d6:   .dword 0x00aaaabbbbccccdd
d7:   .dword 0xeeeeffff00001111
d8:   .dword 0xbbccccddddeeeeff
d9:   .dword 0x000000ddaaaabbbb
d10:  .dword 0x5555dddd3333bbbb
d11:  .dword 0x9999999999999999
d12:  .dword 56
d13:  .dword 8
d14:  .dword 57
d15:  .dword 0x000000ddaaaac98b
d16:  .dword 0xffffffffdead00dd
d17:  .dword 0xffffffffc0de0000
d18:  .dword 0x0000123400000000
d19:  .dword 0xffffabcddead00dd
d20:  .dword 0xc0de000000000000
d21:  .dword 0x8000abcddead00dd
dmask:.dword 0xffffffffffff0000
dval1: .word 0x1234abcd
dval2: .word 0xffee0000
dval3:	.dword 0xffffffffffffffff
  .fill 240,1,0
dval4:	.dword 0x5555555555555555
  .fill  264,1,0
dval5:	.dword 0xaaaaaaaaaaaaaaaa

  .text

  setup

  .set noreorder

  .ent DIAG
DIAG:

  writemsg "[1] Test DMUL"
  r6ck_2r dmul, 6, 5, 30
  r6ck_2r dmul, -7, 9, -63
  r6ck_2r dmul, -1, 1, -1
  r6ck_2dr dmul, d1, d2, dlo

  writemsg "[2] Test DMUH"
  r6ck_2r dmuh, 6, 5, 0
  r6ck_2r dmuh, -7, 9, 0xffffffffffffffff
  r6ck_2r dmuh, -1, 1, -1
  r6ck_2dr dmuh, d1, d2, dhi

  writemsg "[3] Test DMULU"
  r6ck_2r dmulu, 12, 10, 120
  r6ck_2r dmulu, -1, 1, -1
  r6ck_2dr dmulu, d1, d2, dlo

  writemsg "[4] Test DMUHU"
  r6ck_2r dmuhu, 12, 10, 0
  r6ck_2r dmuhu, -1, 1, 0
  r6ck_2dr dmuhu, d1, d2, dhiu

  writemsg "[5] Test DDIV"
  r6ck_2r ddiv, 10001, 10, 1000
  r6ck_2r ddiv, -123456, 560, -220
  r6ck_2dr ddiv, d1, d2, d3

  writemsg "[6] Test DMOD"
  r6ck_2r dmod, 10001, 10, 1
  r6ck_2r dmod, -123456, 560, 0xffffffffffffff00
  r6ck_2dr dmod, d1, d2, d4

  writemsg "[7] Test DDIVU"
  r6ck_2r ddivu, 9, 100, 0
  r6ck_2dr ddivu, d1, d2, d6

  writemsg "[8] Test DMODU"
  r6ck_2r dmodu, 9, 100, 9
  r6ck_2dr dmodu, d1, d2, d5

  writemsg "[9] Test DALIGN"
  r6ck_2dr1i dalign, d7, d1, 3, d8
  r6ck_2dr1i dalign, d1, d5, 4, d9

  writemsg "[10] Test DBITSWAP"
  r6ck_1dr dbitswap, d1, d10
  r6ck_1dr dbitswap, d11, d11

  writemsg "[11] Test DCLZ"
  r6ck_1dr dclz, d5, d12
  r6ck_1dr dclz, d6, d13

  writemsg "[12] Test DCLO"
  r6ck_1dr dclo, d5, d0
  r6ck_1dr dclo, dhi, d14

  writemsg "[13] Test DLSA"
  r6ck_2r1i dlsa, 0x82, 0x2000068, 4, 0x2000888
  r6ck_2dr1i dlsa, d5, d9, 4, d15

  writemsg "[14] Test DAUI"
  r6ck_1dr1i daui, d5, 0xdead, d16
  r6ck_1dr1i daui, d0, 0xc0de, d17

  writemsg "[15] Test DAHI"
  r6ck_0dr1i dahi, d0, 0x1234, d18
  r6ck_0dr1i dahi, d16, 0xabce, d19

  writemsg "[16] Test DATI"
  r6ck_0dr1i dati, d0, 0xc0de, d20
  r6ck_0dr1i dati, d19, 0x8001, d21

  writemsg "[17] Test LDPC"
  ld $5, dval
  nop
  ldpc $4, dval
  fp_assert $4, $5

  writemsg "[18] Test LWUPC"
  lwu $5, dval1
  lwupc $4, dval1
  fp_assert $4, $5
  lwu $5, dval2
  lwupc $4, dval2
  fp_assert $4, $5

  writemsg "[19] Test LLD"
  ld $5, dval3
  dla $3, dval4
  lld $4, -248($3)
  fp_assert $4, $5

  writemsg "[20] Test SCD"
  lld $4, -248($3)
  dli $4, 0xafaf
  scd $4, -248($3)
  ld $5, dval3
  dli $4, 0xafaf
  fp_assert $4, $5

  pass

  .end DIAG
