//Original:/proj/frio/dv/testcases/core/c_cactrl_iflush_pr/c_cactrl_iflush_pr.dsp
// Spec Reference: c_cactrl iflush_pr
# mach: bfin

.include "testutils.inc"
	start

// initial values
//p1=0x448;
//imm32 p1, CODE_ADDR_1;
	loadsym p1, SUBR1;
// set all regs

	imm32 r0, 0x13545abd;
	imm32 r1, 0xadbcfec7;
	imm32 r2, 0xa1245679;
	imm32 r3, 0x00060007;
	imm32 r4, 0xefbc4569;
	imm32 r5, 0x1235000b;
	imm32 r6, 0x000c000d;
	imm32 r7, 0x678e000f;
// The result accumulated in A0 and A1, and stored to a reg half
	R2.H = ( A1 = R1.L * R0.H ), A0 = R1.H * R0.L;
	R3.H = A1 , A0 = R7.H * R6.L (T);
// begin of iflush
	IFLUSH [ P1 ];	// p1 = 0xf00
	R7 = 0;
	ASTAT = R7;
	IF !CC JUMP SUBR1;
JBACK:
	R6 = 0;

//r4  = (a1 = l*h) M,  a0  = h*l  (r3,r2);
//r5     a1 = l*h, =  (a0  = h*l) (r1,r0) IS;
	CHECKREG r2, 0xFFD15679;
	CHECKREG r3, 0xFFD00007;
	CHECKREG r4, 0x00074569;
	CHECKREG r5, 0x12358000;

	pass

//.code 0x448
//.code CODE_ADDR_1
SUBR1:
	R4.H = ( A1 = R3.L * R2.H ) (M), A0 = R3.H * R2.L;
	A1 = R1.L * R0.H, R5.L = ( A0 = R1.H * R0.L ) (ISS2);
	IF !CC JUMP JBACK;
	NOP;	NOP; NOP; NOP; NOP;

// Pre-load memory with known data
// More data is defined than will actually be used

	.data
DATA_ADDR_1:
	.dd 0x00010203
	.dd 0x04050607
	.dd 0x08090A0B
	.dd 0x0C0D0E0F
	.dd 0x10111213
	.dd 0x14151617
	.dd 0x18191A1B
	.dd 0x1C1D1E1F

DATA_ADDR_2:
	.dd 0x20212223
	.dd 0x24252627
	.dd 0x28292A2B
	.dd 0x2C2D2E2F
	.dd 0x30313233
	.dd 0x34353637
	.dd 0x38393A3B
	.dd 0x3C3D3E3F

DATA_ADDR_3:
	.dd 0x40414243
	.dd 0x44454647
	.dd 0x48494A4B
	.dd 0x4C4D4E4F
	.dd 0x50515253
	.dd 0x54555657
	.dd 0x58595A5B
	.dd 0x5C5D5E5F

DATA_ADDR_4:
	.dd 0x60616263
	.dd 0x64656667
	.dd 0x68696A6B
	.dd 0x6C6D6E6F
	.dd 0x70717273
	.dd 0x74757677
	.dd 0x78797A7B
	.dd 0x7C7D7E7F

DATA_ADDR_5:
	.dd 0x80818283
	.dd 0x84858687
	.dd 0x88898A8B
	.dd 0x8C8D8E8F
	.dd 0x90919293
	.dd 0x94959697
	.dd 0x98999A9B
	.dd 0x9C9D9E9F
