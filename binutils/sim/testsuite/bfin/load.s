# Blackfin testcase for register load instructions
# mach: bfin


	.include "testutils.inc"

	start

	.macro load32 num:req, reg0:req, reg1:req
	imm32 \reg0 \num
	imm32 \reg1 \num
	CC = \reg0 == \reg1
	if CC jump 2f;
	fail
2:
	.endm

	.macro load32p num:req preg:req
	imm32 r0 \num
	imm32 \preg \num
	r1 = \preg
	cc = r0 == r1
	if CC jump 3f;
	fail
3:
	imm32 \preg 0
	.endm

	.macro load16z num:req reg0:req reg1:req
	\reg0 = \num (Z);
	imm32 \reg1 \num
	CC = \reg0 == \reg1
	if CC jump 4f;
	fail
4:
	.endm

	.macro load16zp num:req reg:req
	\reg = \num (Z);
	imm32 r1 \num;
	r0 = \reg;
	cc = r0 == r1
	if CC jump 5f;
	fail
5:
	.endm

	.macro load16x num:req reg0:req reg1:req
	\reg0 = \num (X);
	imm32 \reg1, \num
	CC = \reg0 == \reg1
	if CC jump 6f;
	fail
6:
	.endm

	/* Clobbers R0 */
	.macro loadinc preg0:req, preg1:req, dreg:req
	loadsym \preg0, _buf
	\preg1 = \preg0;
	\dreg  = \preg0;
	[\preg0\()++] = \preg0;
	\dreg += 4;
	R0 = \preg0;
	CC = \dreg == R0;
	if CC jump 7f;
	fail
7:
	R0 = [ \preg1\() ];
	\dreg += -4;
	CC = \dreg == R0;
	if CC jump 8f;
	fail
8:
	.endm

	/* test a bunch of values */

	/* load_immediate (Half-Word Load)
	 * register = constant
	 *    reg_lo = uimm16;
	 *    reg_hi = uimm16;
	 */

	load32 0 R0 R1
	load32 0xFFFFFFFF R0 R1
	load32 0x55aaaa55 r0 r1
	load32 0x12345678 r0 r1
	load32 0x12345678 R0 R2
	load32 0x23456789 R0 R3
	load32 0x3456789a R0 R4
	load32 0x456789ab R0 R5
	load32 0x56789abc R0 R6
	load32 0x6789abcd R0 R7
	load32 0x789abcde R0 R0
	load32 0x89abcdef R1 R0
	load32 0x9abcdef0 R2 R0
	load32 0xabcdef01 R3 R0
	load32 0xbcdef012 R4 R0
	load32 0xcdef0123 R5 R0
	load32 0xdef01234 R6 R0
	load32 0xef012345 R7 R0

	load32p 0xf0123456 P0
	load32p 0x01234567 P1
	load32p 0x12345678 P2
.ifndef BFIN_HOST
	load32p 0x23456789 P3
.endif
	load32p 0x3456789a P4
	load32p 0x456789ab P5
	load32p 0x56789abc SP
	load32p 0x6789abcd FP

	load32p 0x789abcde I0
	load32p 0x89abcdef I1
	load32p 0x9abcdef0 I2
	load32p 0xabcdef01 I3
	load32p 0xbcdef012 M0
	load32p 0xcdef0123 M1
	load32p 0xdef01234 M2
	load32p 0xef012345 M3

	load32p 0xf0123456 B0
	load32p 0x01234567 B1
	load32p 0x12345678 B2
	load32p 0x23456789 B3
	load32p 0x3456789a L0
	load32p 0x456789ab L1
	load32p 0x56789abc L2
	load32p 0x6789abcd L3

	/* Zero Extended  */
	load16z 0x1234 R0 R1
	load16z 0x2345 R0 R1
	load16z 0x3456 R0 R2
	load16z 0x4567 R0 R3
	load16z 0x5678 R0 R4
	load16z 0x6789 R0 R5
	load16z 0x789a R0 R6
	load16z 0x89ab R0 R7
	load16z 0x9abc R1 R0
	load16z 0xabcd R2 R0
	load16z 0xbcde R3 R0
	load16z 0xcdef R4 R0
	load16z 0xdef0 R5 R0
	load16z 0xef01 R6 R0
	load16z 0xf012 R7 R0

	load16zp 0x0123 P0
	load16zp 0x1234 P1
	load16zp 0x1234 p2
.ifndef BFIN_HOST
	load16zp 0x2345 p3
.endif
	load16zp 0x3456 p4
	load16zp 0x4567 p5
	load16zp 0x5678 sp
	load16zp 0x6789 fp
	load16zp 0x789a i0
	load16zp 0x89ab i1
	load16zp 0x9abc i2
	load16zp 0xabcd i3
	load16zp 0xbcde m0
	load16zp 0xcdef m1
	load16zp 0xdef0 m2
	load16zp 0xef01 m3
	load16zp 0xf012 b0
	load16zp 0x0123 b1
	load16zp 0x1234 b2
	load16zp 0x2345 b3
	load16zp 0x3456 l0
	load16zp 0x4567 l1
	load16zp 0x5678 l2
	load16zp 0x6789 l3

	/* Sign Extended */
	load16x 0x20 R0 R1
	load16x 0x3F R0 R1
	load16x -0x20 R0 R1
	load16x -0x3F R0 R1
	load16x 0x1234 R0 R1
	load16x 0x2345 R0 R1
	load16x 0x3456 R0 R2
	load16x 0x4567 R0 R3
	load16x 0x5678 R0 R4
	load16x 0x6789 R0 R5
	load16x 0x789a R0 R6
	load16x 0x09ab R0 R7
	load16x -0x1abc R1 R0
	load16x -0x2bcd R2 R0
	load16x -0x3cde R3 R0
	load16x -0x4def R4 R0
	load16x -0x5ef0 R5 R0
	load16x -0x6f01 R6 R0
	load16x -0x7012 R7 R0

	loadinc P0, P1, R1
	loadinc P1, P2, R1
	loadinc P2, P1, R2
.ifndef BFIN_HOST
	loadinc P3, P4, R3
.endif
	loadinc P4, P5, R4
	loadinc FP, P0, R7
	loadinc P0, I0, R1
	loadinc P1, I1, R1
	loadinc P2, I2, R1
.ifndef BFIN_HOST
	loadinc P3, I0, R1
.endif
	loadinc P4, I2, R1
	loadinc P5, I3, R1

	A1 = A0 = 0;
	R0 = 0x01 (Z);
	A0.x = R0;
	imm32 r4, 0x32e02d1a
	A1.x = R4;
	A0.w = A1.x;
	R3 = A0.w;
	R2 = A0.x;
	imm32 r0, 0x0000001a
	imm32 r1, 0x00000001
	CC = R1 == R2;
	if CC jump 1f;
	fail
1:
	CC = R0 == R3
	if CC jump 2f;
	fail
2:
	pass

.data
_buf:
	.rept 0x80
	.long 0
	.endr
