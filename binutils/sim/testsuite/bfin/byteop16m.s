# Blackfin testcase for BYTEOP16M
# mach: bfin

	.include "testutils.inc"

	start

	.macro check_it resL:req, resH:req
	imm32 R6, \resL
	CC = R4 == R6;
	IF !CC JUMP 1f;
#DBG R4
	imm32 R7, \resH
	CC = R5 == R7;
	IF !CC JUMP 1f;
#DBG R5
	.endm
	.macro test_byteop16m i0:req, i1:req, resL:req, resH:req, resLR:req, resHR:req
	dmm32 I0, \i0
	dmm32 I1, \i1

	(R4, R5) = BYTEOP16M (R1:0, R3:2);
	check_it \resL, \resH
	(R4, R5) = BYTEOP16M (R1:0, R3:2) (R);
	check_it \resLR, \resHR

	jump 2f;
1:	fail
2:
	.endm

	imm32 R0, 0x01020304
	imm32 R1, 0x10203040
	imm32 R2, 0x0a0b0c0d
	imm32 R3, 0xa0b0c0d0

	test_byteop16m 0, 0, 0xfff7fff7, 0xfff7fff7, 0xff70ff70, 0xff70ff70
	test_byteop16m 0, 1, 0xff31fff8, 0xfff8fff8, 0x0003ff80, 0xff80ff80
	test_byteop16m 0, 2, 0xff41ff32, 0xfff9fff9, 0x00040013, 0xff90ff90
	test_byteop16m 0, 3, 0xff51ff42, 0xff33fffa, 0x00050014, 0x0023ffa0
	test_byteop16m 1, 0, 0x0036fff6, 0xfff6fff6, 0xff64ff60, 0xff60ff60
	test_byteop16m 1, 1, 0xff70fff7, 0xfff7fff7, 0xfff7ff70, 0xff70ff70
	test_byteop16m 1, 2, 0xff80ff31, 0xfff8fff8, 0xfff80003, 0xff80ff80
	test_byteop16m 1, 3, 0xff90ff41, 0xff32fff9, 0xfff90004, 0x0013ff90
	test_byteop16m 2, 0, 0x00260035, 0xfff5fff5, 0xff63ff54, 0xff50ff50
	test_byteop16m 2, 1, 0xff600036, 0xfff6fff6, 0xfff6ff64, 0xff60ff60
	test_byteop16m 2, 2, 0xff70ff70, 0xfff7fff7, 0xfff7fff7, 0xff70ff70
	test_byteop16m 2, 3, 0xff80ff80, 0xff31fff8, 0xfff8fff8, 0x0003ff80
	test_byteop16m 3, 0, 0x00160025, 0x0034fff4, 0xff62ff53, 0xff44ff40
	test_byteop16m 3, 1, 0xff500026, 0x0035fff5, 0xfff5ff63, 0xff54ff50
	test_byteop16m 3, 2, 0xff60ff60, 0x0036fff6, 0xfff6fff6, 0xff64ff60
	test_byteop16m 3, 3, 0xff70ff70, 0xff70fff7, 0xfff7fff7, 0xfff7ff70

	imm32 R0, ~0x01020304
	imm32 R1, ~0x10203040
	imm32 R2, ~0x0a0b0c0d
	imm32 R3, ~0xa0b0c0d0

	test_byteop16m 0, 0, 0x00090009, 0x00090009, 0x00900090, 0x00900090
	test_byteop16m 0, 1, 0x00cf0008, 0x00080008, 0xfffd0080, 0x00800080
	test_byteop16m 0, 2, 0x00bf00ce, 0x00070007, 0xfffcffed, 0x00700070
	test_byteop16m 0, 3, 0x00af00be, 0x00cd0006, 0xfffbffec, 0xffdd0060
	test_byteop16m 1, 0, 0xffca000a, 0x000a000a, 0x009c00a0, 0x00a000a0
	test_byteop16m 1, 1, 0x00900009, 0x00090009, 0x00090090, 0x00900090
	test_byteop16m 1, 2, 0x008000cf, 0x00080008, 0x0008fffd, 0x00800080
	test_byteop16m 1, 3, 0x007000bf, 0x00ce0007, 0x0007fffc, 0xffed0070
	test_byteop16m 2, 0, 0xffdaffcb, 0x000b000b, 0x009d00ac, 0x00b000b0
	test_byteop16m 2, 1, 0x00a0ffca, 0x000a000a, 0x000a009c, 0x00a000a0
	test_byteop16m 2, 2, 0x00900090, 0x00090009, 0x00090009, 0x00900090
	test_byteop16m 2, 3, 0x00800080, 0x00cf0008, 0x00080008, 0xfffd0080
	test_byteop16m 3, 0, 0xffeaffdb, 0xffcc000c, 0x009e00ad, 0x00bc00c0
	test_byteop16m 3, 1, 0x00b0ffda, 0xffcb000b, 0x000b009d, 0x00ac00b0
	test_byteop16m 3, 2, 0x00a000a0, 0xffca000a, 0x000a000a, 0x009c00a0
	test_byteop16m 3, 3, 0x00900090, 0x00900009, 0x00090009, 0x00090090

	pass
