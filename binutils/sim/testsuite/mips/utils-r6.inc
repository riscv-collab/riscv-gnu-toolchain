  .macro fp_assert a, b
  beq \a, \b, 1f
  nop
  j _fail
  nop
1:
  .endm

  .macro r6ck_1r inst, a, ret
  li $4, \a
  li $6, \ret
  \inst $7, $4
  fp_assert $6, $7
  .endm

  .macro r6ck_1dr inst, a, ret
  ld $4, \a
  ld $6, \ret
  \inst $7, $4
  fp_assert $6, $7
  .endm

  .macro r6ck_2r inst, a, b, ret
  li $4, \a
  li $5, \b
  li $6, \ret
  \inst $7, $4, $5
  fp_assert $6, $7
  .endm

  .macro r6ck_2dr inst, a, b, ret
  ld $4, \a
  ld $5, \b
  ld $6, \ret
  \inst $7, $4, $5
  fp_assert $6, $7
  .endm

  .macro r6ck_2dr1i inst, a, b, imm, ret
  ld $4, \a
  ld $5, \b
  ld $6, \ret
  \inst $7, $4, $5, \imm
  fp_assert $6, $7
  .endm

  .macro r6ck_1r1i inst, a, imm, ret
  li $4, \a
  li $6, \ret
  \inst $7, $4, \imm
  fp_assert $6, $7
  .endm

  .macro r6ck_1dr1i inst, a, imm, ret
  ld $4, \a
  ld $6, \ret
  \inst $7, $4, \imm
  fp_assert $6, $7
  .endm

  .macro r6ck_0dr1i inst, a, imm, ret
  ld $4, \a
  ld $6, \ret
  \inst $4, $4, \imm
  fp_assert $6, $4
  .endm

  .macro r6ck_2r1i inst, a, b, imm, ret
  li $4, \a
  li $5, \b
  li $6, \ret
  \inst $7, $4, $5, \imm
  fp_assert $6, $7
  .endm

  .macro r6ck_3s inst, a, b, c, ret
  li $4, \a
  li $5, \b
  li $6, \c
  li $7, \ret
  mtc1 $4, $f2
  mtc1 $5, $f4
  mtc1 $6, $f6
  \inst $f2, $f4, $f6
  mfc1 $8, $f2
  fp_assert $7, $8
  .endm

  .macro r6ck_2s inst, a, b, ret
  li $4, \a
  li $5, \b
  li $6, \ret
  mtc1 $4, $f2
  mtc1 $5, $f4
  \inst $f2, $f4
  mfc1 $7, $f2
  fp_assert $6, $7
  .endm

  .macro r6ck_2d inst, a, b, ret
  .data
1: .dword \a
2: .dword \b
3: .dword \ret
  .text
  la $4, 1b
  la $5, 2b
  la $6, 3b
  ldc1 $f2, 0($4)
  ldc1 $f4, 0($5)
  lw $7, 0($6)
  lw $8, 4($6)
  \inst $f2, $f4

  #simulate dmfc1
  mfhc1 $9, $f2
  mfc1 $10, $f2
  fp_assert $7, $9
  fp_assert $8, $10
  .endm

  .macro r6ck_3d inst, a, b, c, ret
  .data
1: .dword \a
2: .dword \b
3: .dword \c
4: .dword \ret
  .text
  la $4, 1b
  la $5, 2b
  la $6, 3b
  la $2, 4b
  ldc1 $f2, 0($4)
  ldc1 $f4, 0($5)
  ldc1 $f6, 0($6)
  lw $7, 0($2)
  lw $8, 4($2)
  \inst $f2, $f4, $f6

  #simulate dmfc1
  mfhc1 $9, $f2
  mfc1 $10, $f2
  fp_assert $7, $9
  fp_assert $8, $10
  .endm

.text
GetPC:
  move $6, $ra
  jr $ra
