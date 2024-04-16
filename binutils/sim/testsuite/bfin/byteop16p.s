# Blackfin testcase for BYTEOP16P
# mach: bfin

	.include "testutils.inc"

	start

	.macro check_it resL:req, resH:req
	imm32 R6, \resL
	CC = R4 == R6;
	IF !CC JUMP 1f;
	imm32 R7, \resH
	CC = R5 == R7;
	IF !CC JUMP 1f;
	.endm
	.macro test_byteop16p i0:req, i1:req, resL:req, resH:req, resLR:req, resHR:req
	dmm32 I0, \i0
	dmm32 I1, \i1

	(R4, R5) = BYTEOP16P (R1:0, R3:2);
	check_it \resL, \resH
	(R4, R5) = BYTEOP16P (R1:0, R3:2) (R);
	check_it \resLR, \resHR

	jump 2f;
1:	fail
2:
	.endm

	imm32 R0, 0x01020304
	imm32 R1, 0x10203040
	imm32 R2, 0x0a0b0c0d
	imm32 R3, 0xa0b0c0d0

	test_byteop16p 0, 0, 0x000b000d, 0x000f0011, 0x00b000d0, 0x00f00110
	test_byteop16p 0, 1, 0x00d1000c, 0x000e0010, 0x001d00c0, 0x00e00100
	test_byteop16p 0, 2, 0x00c100d2, 0x000d000f, 0x001c002d, 0x00d000f0
	test_byteop16p 0, 3, 0x00b100c2, 0x00d3000e, 0x001b002c, 0x003d00e0
	test_byteop16p 1, 0, 0x004a000c, 0x000e0010, 0x00a400c0, 0x00e00100
	test_byteop16p 1, 1, 0x0110000b, 0x000d000f, 0x001100b0, 0x00d000f0
	test_byteop16p 1, 2, 0x010000d1, 0x000c000e, 0x0010001d, 0x00c000e0
	test_byteop16p 1, 3, 0x00f000c1, 0x00d2000d, 0x000f001c, 0x002d00d0
	test_byteop16p 2, 0, 0x003a004b, 0x000d000f, 0x00a300b4, 0x00d000f0
	test_byteop16p 2, 1, 0x0100004a, 0x000c000e, 0x001000a4, 0x00c000e0
	test_byteop16p 2, 2, 0x00f00110, 0x000b000d, 0x000f0011, 0x00b000d0
	test_byteop16p 2, 3, 0x00e00100, 0x00d1000c, 0x000e0010, 0x001d00c0
	test_byteop16p 3, 0, 0x002a003b, 0x004c000e, 0x00a200b3, 0x00c400e0
	test_byteop16p 3, 1, 0x00f0003a, 0x004b000d, 0x000f00a3, 0x00b400d0
	test_byteop16p 3, 2, 0x00e00100, 0x004a000c, 0x000e0010, 0x00a400c0
	test_byteop16p 3, 3, 0x00d000f0, 0x0110000b, 0x000d000f, 0x001100b0

	imm32 R0, ~0x01020304
	imm32 R1, ~0x10203040
	imm32 R2, ~0x0a0b0c0d
	imm32 R3, ~0xa0b0c0d0

	test_byteop16p 0, 0, 0x01f301f1, 0x01ef01ed, 0x014e012e, 0x010e00ee
	test_byteop16p 0, 1, 0x012d01f2, 0x01f001ee, 0x01e1013e, 0x011e00fe
	test_byteop16p 0, 2, 0x013d012c, 0x01f101ef, 0x01e201d1, 0x012e010e
	test_byteop16p 0, 3, 0x014d013c, 0x012b01f0, 0x01e301d2, 0x01c1011e
	test_byteop16p 1, 0, 0x01b401f2, 0x01f001ee, 0x015a013e, 0x011e00fe
	test_byteop16p 1, 1, 0x00ee01f3, 0x01f101ef, 0x01ed014e, 0x012e010e
	test_byteop16p 1, 2, 0x00fe012d, 0x01f201f0, 0x01ee01e1, 0x013e011e
	test_byteop16p 1, 3, 0x010e013d, 0x012c01f1, 0x01ef01e2, 0x01d1012e
	test_byteop16p 2, 0, 0x01c401b3, 0x01f101ef, 0x015b014a, 0x012e010e
	test_byteop16p 2, 1, 0x00fe01b4, 0x01f201f0, 0x01ee015a, 0x013e011e
	test_byteop16p 2, 2, 0x010e00ee, 0x01f301f1, 0x01ef01ed, 0x014e012e
	test_byteop16p 2, 3, 0x011e00fe, 0x012d01f2, 0x01f001ee, 0x01e1013e
	test_byteop16p 3, 0, 0x01d401c3, 0x01b201f0, 0x015c014b, 0x013a011e
	test_byteop16p 3, 1, 0x010e01c4, 0x01b301f1, 0x01ef015b, 0x014a012e
	test_byteop16p 3, 2, 0x011e00fe, 0x01b401f2, 0x01f001ee, 0x015a013e
	test_byteop16p 3, 3, 0x012e010e, 0x00ee01f3, 0x01f101ef, 0x01ed014e

	pass
