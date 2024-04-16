# Blackfin testcase for register move instructions
# mach: bfin


	.include "testutils.inc"

	start

	.macro move reg0:req, reg1:req, clobber:req
	imm32 \reg0, 0x5555aaaa
	imm32 \reg1, 0x12345678
	imm32 \clobber, 0x12345678
	\reg0 = \reg1;
	CC = \reg0 == \clobber;
	if CC jump 1f;
	fail
1:
	.endm

	move R0, R1, R2
	move R0, R2, R3
	move R0, R2, R4
	move R0, R3, R5
	move R0, R4, R6
	move R0, R5, R7
	move R0, R6, R1
	move R0, R7, R2
	move R7, R0, R1
	move R7, R1, R2
	move R7, R2, R3
	move R7, R3, R4
	move R7, R4, R5
	move R7, R5, R6
	move R7, R6, R0

	pass
