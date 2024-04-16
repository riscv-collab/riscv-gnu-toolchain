# Blackfin testcase for BYTEOP2P
# mach: bfin

	.include "testutils.inc"

	start

	.macro check_it res:req
	imm32 R7, \res
	CC = R6 == R7;
	IF !CC JUMP 1f;
	.endm
	.macro test_byteop2p i0:req, resRL:req, resRH:req, resTL:req, resTH:req, resRLr:req, resRHr:req, resTLr:req, resTHr:req
	dmm32 I0, \i0

	R6 = BYTEOP2P (R1:0, R3:2) (rndl);
	check_it \resRL
	R6 = BYTEOP2P (R1:0, R3:2) (rndh);
	check_it \resRH
	R6 = BYTEOP2P (R1:0, R3:2) (tl);
	check_it \resTL
	R6 = BYTEOP2P (R1:0, R3:2) (th);
	check_it \resTH
	R6 = BYTEOP2P (R1:0, R3:2) (rndl, r);
	check_it \resRLr
	R6 = BYTEOP2P (R1:0, R3:2) (rndh, r);
	check_it \resRHr
	R6 = BYTEOP2P (R1:0, R3:2) (tl, r);
	check_it \resTLr
	R6 = BYTEOP2P (R1:0, R3:2) (th, r);
	check_it \resTHr

	jump 2f;
1:	fail
2:
	.endm

	imm32 R0, 0x01020304
	imm32 R1, 0x10203040
	imm32 R2, 0x0a0b0c0d
	imm32 R3, 0xa0b0c0d0

	test_byteop2p 0, 0x00060008, 0x06000800, 0x00060008, 0x06000800, 0x00600080, 0x60008000, 0x00600080, 0x60008000
	test_byteop2p 1, 0x00470007, 0x47000700, 0x00460007, 0x46000700, 0x00300070, 0x30007000, 0x00300070, 0x30007000
	test_byteop2p 2, 0x00800006, 0x80000600, 0x00800006, 0x80000600, 0x00080060, 0x08006000, 0x00080060, 0x08006000
	test_byteop2p 3, 0x00700047, 0x70004700, 0x00700046, 0x70004600, 0x00070030, 0x07003000, 0x00070030, 0x07003000

	imm32 R0, ~0x01020304
	imm32 R1, ~0x10203040
	imm32 R2, ~0x0a0b0c0d
	imm32 R3, ~0xa0b0c0d0

	test_byteop2p 0, 0x00f900f7, 0xf900f700, 0x00f900f7, 0xf900f700, 0x009f007f, 0x9f007f00, 0x009f007f, 0x9f007f00
	test_byteop2p 1, 0x00b800f8, 0xb800f800, 0x00b800f8, 0xb800f800, 0x00cf008f, 0xcf008f00, 0x00ce008f, 0xce008f00
	test_byteop2p 2, 0x007f00f9, 0x7f00f900, 0x007f00f9, 0x7f00f900, 0x00f7009f, 0xf7009f00, 0x00f7009f, 0xf7009f00
	test_byteop2p 3, 0x008f00b8, 0x8f00b800, 0x008f00b8, 0x8f00b800, 0x00f800cf, 0xf800cf00, 0x00f800ce, 0xf800ce00

	pass
