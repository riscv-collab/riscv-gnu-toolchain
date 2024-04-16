# Blackfin testcase for SYSCFG register
# mach: bfin

	.include "testutils.inc"

	.macro syscfg_test val:req
	imm32 R0, \val
	R0 = SYSCFG;
	SYSCFG = R0;
	R1 = SYSCFG;
	CC = R0 == R1;
	IF !CC JUMP 1f;
	.endm

	start

	syscfg_test 0
	syscfg_test 1
	syscfg_test -1
	syscfg_test 0x12345678
	# leave in sane state
	syscfg_test 0x30

	pass
1:	fail
