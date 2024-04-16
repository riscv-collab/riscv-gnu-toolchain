# Blackfin testcase for BYTEOP1P
# mach: bfin

	.include "testutils.inc"

	start

	.macro check_it res:req
	imm32 R7, \res
	CC = R6 == R7;
	IF !CC JUMP 1f;
	.endm
	.macro test_byteop1p i0:req, i1:req, res:req, resT:req, resR:req, resTR:req
	dmm32 I0, \i0
	dmm32 I1, \i1

	R6 = BYTEOP1P (R1:0, R3:2);
	check_it \res
	R6 = BYTEOP1P (R1:0, R3:2) (T);
	check_it \resT
	R6 = BYTEOP1P (R1:0, R3:2) (R);
	check_it \resR
	R6 = BYTEOP1P (R1:0, R3:2) (T, R);
	check_it \resTR

	jump 2f;
1:	fail
2:
	.endm

	imm32 R0, 0x01020304
	imm32 R1, 0x10203040
	imm32 R2, 0x0a0b0c0d
	imm32 R3, 0xa0b0c0d0

	test_byteop1p 0, 0, 0x06070809, 0x05060708, 0x58687888, 0x58687888
	test_byteop1p 0, 1, 0x69060708, 0x68060708, 0x0f607080, 0x0e607080
	test_byteop1p 0, 2, 0x61690708, 0x60690607, 0x0e176878, 0x0e166878
	test_byteop1p 0, 3, 0x59616a07, 0x58616907, 0x0e161f70, 0x0d161e70
	test_byteop1p 1, 0, 0x25060708, 0x25060708, 0x52607080, 0x52607080
	test_byteop1p 1, 1, 0x88060708, 0x88050607, 0x09586878, 0x08586878
	test_byteop1p 1, 2, 0x80690607, 0x80680607, 0x080f6070, 0x080e6070
	test_byteop1p 1, 3, 0x78616907, 0x78606906, 0x080e1768, 0x070e1668
	test_byteop1p 2, 0, 0x1d260708, 0x1d250607, 0x525a6878, 0x515a6878
	test_byteop1p 2, 1, 0x80250607, 0x80250607, 0x08526070, 0x08526070
	test_byteop1p 2, 2, 0x78880607, 0x78880506, 0x08095868, 0x07085868
	test_byteop1p 2, 3, 0x70806906, 0x70806806, 0x07080f60, 0x07080e60
	test_byteop1p 3, 0, 0x151e2607, 0x151d2607, 0x515a6270, 0x51596270
	test_byteop1p 3, 1, 0x781d2607, 0x781d2506, 0x08525a68, 0x07515a68
	test_byteop1p 3, 2, 0x70802506, 0x70802506, 0x07085260, 0x07085260
	test_byteop1p 3, 3, 0x68788806, 0x68788805, 0x07080958, 0x06070858

	imm32 R0, ~0x01020304
	imm32 R1, ~0x10203040
	imm32 R2, ~0x0a0b0c0d
	imm32 R3, ~0xa0b0c0d0

	test_byteop1p 0, 0, 0xfaf9f8f7, 0xf9f8f7f6, 0xa7978777, 0xa7978777
	test_byteop1p 0, 1, 0x97f9f8f7, 0x96f9f8f7, 0xf19f8f7f, 0xf09f8f7f
	test_byteop1p 0, 2, 0x9f96f9f8, 0x9e96f8f7, 0xf1e99787, 0xf1e89787
	test_byteop1p 0, 3, 0xa79e96f8, 0xa69e95f8, 0xf2e9e18f, 0xf1e9e08f
	test_byteop1p 1, 0, 0xdaf9f8f7, 0xdaf9f8f7, 0xad9f8f7f, 0xad9f8f7f
	test_byteop1p 1, 1, 0x77faf9f8, 0x77f9f8f7, 0xf7a79787, 0xf6a79787
	test_byteop1p 1, 2, 0x7f97f9f8, 0x7f96f9f8, 0xf7f19f8f, 0xf7f09f8f
	test_byteop1p 1, 3, 0x879f96f9, 0x879e96f8, 0xf8f1e997, 0xf7f1e897
	test_byteop1p 2, 0, 0xe2daf9f8, 0xe2d9f8f7, 0xaea59787, 0xada59787
	test_byteop1p 2, 1, 0x7fdaf9f8, 0x7fdaf9f8, 0xf7ad9f8f, 0xf7ad9f8f
	test_byteop1p 2, 2, 0x8777faf9, 0x8777f9f8, 0xf8f7a797, 0xf7f6a797
	test_byteop1p 2, 3, 0x8f7f97f9, 0x8f7f96f9, 0xf8f7f19f, 0xf8f7f09f
	test_byteop1p 3, 0, 0xeae2d9f8, 0xeae1d9f8, 0xaea69d8f, 0xaea59d8f
	test_byteop1p 3, 1, 0x87e2daf9, 0x87e2d9f8, 0xf8aea597, 0xf7ada597
	test_byteop1p 3, 2, 0x8f7fdaf9, 0x8f7fdaf9, 0xf8f7ad9f, 0xf8f7ad9f
	test_byteop1p 3, 3, 0x978777fa, 0x978777f9, 0xf9f8f7a7, 0xf8f7f6a7

	pass
