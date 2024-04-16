# Test that unsigned widening multiplication of 32-bit operands to produce a
# 64-bit result is simulated correctly, when using 32-bit or F5series hardware
# multiply functionality.
# 0xffff fffc * 0x2 = 0x1 ffff fff8
# mach: msp430

# 32-bit hwmult register addresses
.set MPY32L,	0x0140
.set MPY32H,	0x0142
.set OP2L,	0x0150
.set OP2H,	0x0152
.set RES0,	0x0154
.set RES1,	0x0156
.set RES2,	0x0158
.set RES3,	0x015A

# F5series hwmult register addresses
.set MPY32L_F5,		0x04D0
.set MPY32H_F5,		0x04D2
.set OP2L_F5,		0x04E0
.set OP2H_F5,		0x04E2
.set RES0_F5,		0x04E4
.set RES1_F5,		0x04E6
.set RES2_F5,		0x04E8
.set RES3_F5,		0x04EA

.include "testutils.inc"

	start

	; Test 32bit hwmult
	MOV.W	#2, &MPY32L		; Load operand 1 Low into multiplier
	MOV.W	#0, &MPY32H		; Load operand 1 High into multiplier
	MOV.W	#-4, &OP2L		; Load operand 2 Low into multiplier
	MOV.W	#-1, &OP2H		; Load operand 2 High, trigger MPY

	CMP.W	#-8, &RES0	{ JNE	.L5
	CMP.W	#-1, &RES1	{ JNE	.L5
	CMP.W	#1, &RES2	{ JNE	.L5
	CMP.W	#0, &RES3	{ JNE	.L5

	; Test f5series hwmult
	MOV.W	#2, &MPY32L_F5
	MOV.W	#0, &MPY32H_F5
	MOV.W	#-4, &OP2L_F5
	MOV.W	#-1, &OP2H_F5

	CMP.W	#-8, &RES0_F5	{ JNE	.L5
	CMP.W	#-1, &RES1_F5	{ JNE	.L5
	CMP.W	#1, &RES2_F5	{ JNE	.L5
	CMP.W	#0, &RES3_F5	{ JEQ	.L6
.L5:
	fail
.L6:
	pass
