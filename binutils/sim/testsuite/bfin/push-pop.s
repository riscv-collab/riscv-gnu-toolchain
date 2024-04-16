# Blackfin testcase for push/pop instructions
# mach: bfin

	.include "testutils.inc"

	start

	# This uses R0/R1 as scratch ... assume those work fine in general
	.macro check loader:req, reg:req
	\loader \reg, 0x12345678
	[--SP] = \reg;
	R0 = [SP];
	R1 = \reg;
	CC = R0 == R1;
	IF !CC JUMP 8f;
	\loader \reg, 0x87654321
	\reg = [SP++];
	CC = R0 == R1;
	IF !CC JUMP 8f;
	# need to do a long jump to avoid PCREL issues
	jump 9f;
	8: jump 1f;
	9:
	.endm
	.macro imm_check reg:req
	check imm32, \reg
	.endm
	.macro dmm_check reg:req
	check dmm32, \reg
	.endm

	imm_check R2
	imm_check R3
	imm_check R4
	imm_check R5
	imm_check R6
	imm_check R7
	imm_check P0
	imm_check P1
	imm_check P2
	imm_check P3
	imm_check P4
	imm_check P5
	imm_check FP
	imm_check I0
	imm_check I1
	imm_check I2
	imm_check I3
	imm_check M0
	imm_check M1
	imm_check M2
	imm_check M3
	imm_check B0
	imm_check B1
	imm_check B2
	imm_check B3
	imm_check L0
	imm_check L1
	imm_check L2
	imm_check L3
	dmm_check A0.X
	dmm_check A0.W
	dmm_check A1.X
	dmm_check A1.W
	dmm_check LC0
	dmm_check LC1
	# Make sure the top/bottom regs have bit 1 set
	dmm_check LT0
	dmm_check LT1
	dmm_check LB0
	dmm_check LB1
	dmm_check RETS

	# These require supervisor resources
.ifndef BFIN_HOST
	dmm_check RETI
	dmm_check RETX
	dmm_check RETN
	# RETE likes to change on the fly with an ICE
	# dmm_check RETE
	# CYCLES can be user mode, but screws kernel
	dmm_check CYCLES
	dmm_check CYCLES2
	dmm_check USP

	# No one pushes/pops these
#	dmm_check EMUDAT
	dmm_check SEQSTAT
	dmm_check SYSCFG
.endif
	dmm_check ASTAT

	pass
1:
	fail
