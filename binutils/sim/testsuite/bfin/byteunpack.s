# Blackfin testcase for playing with BYTEUNPACK
# mach: bfin

	.include "testutils.inc"

	start

	.macro _bu_pre_test i0:req, src0:req, src1:req
	dmm32 I0, \i0
	imm32 R0, \src0
	imm32 R1, \src1
	.endm
	.macro _bu_chk_test dst0:req, dst1:req
	imm32 R2, \dst0
	imm32 R3, \dst1
	CC = R5 == R2;
	IF !CC jump 1f;
	CC = R6 == R3;
	IF !CC jump 1f;
	.endm
	.macro bu_test i0:req, dst0:req, dst1:req, src0:req, src1:req
	_bu_pre_test \i0, \src0, \src1
	(R6, R5) = BYTEUNPACK R1:0;
	_bu_chk_test \dst0, \dst1
	.endm
	.macro bu_r_test i0:req, dst0:req, dst1:req, src0:req, src1:req
	_bu_pre_test \i0, \src0, \src1
	(R6, R5) = BYTEUNPACK R1:0 (R);
	_bu_chk_test \dst0, \dst1
	.endm

	# Taken from PRM
	bu_test 0, 0x00BA00DD, 0x00BE00EF, 0xBEEFBADD, 0xFEEDFACE
	bu_test 1, 0x00EF00BA, 0x00CE00BE, 0xBEEFBADD, 0xFEEDFACE
	bu_test 2, 0x00BE00EF, 0x00FA00CE, 0xBEEFBADD, 0xFEEDFACE
	bu_test 3, 0x00CE00BE, 0x00ED00FA, 0xBEEFBADD, 0xFEEDFACE

	# Taken from PRM
	bu_r_test 0, 0x00FA00CE, 0x00FE00ED, 0xBEEFBADD, 0xFEEDFACE
	bu_r_test 1, 0x00ED00FA, 0x00DD00FE, 0xBEEFBADD, 0xFEEDFACE
	bu_r_test 2, 0x00FE00ED, 0x00BA00DD, 0xBEEFBADD, 0xFEEDFACE
	bu_r_test 3, 0x00DD00FE, 0x00EF00BA, 0xBEEFBADD, 0xFEEDFACE

	pass
1:	fail
