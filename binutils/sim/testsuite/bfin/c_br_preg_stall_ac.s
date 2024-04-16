//Original:/testcases/seq/c_br_preg_stall_ac/c_br_preg_stall_ac.dsp
// Spec Reference: brcc kills  data cache hits
# mach: bfin

.include "testutils.inc"
	start

	/* This test likes to assume the current [SP] is valid */
	SP += -12;

	imm32 r0, 0x00000000;
	imm32 r1, 0x00000001;
	imm32 r2, 0x00000002;
	imm32 r3, 0x00000003;
	imm32 r4, 0x00000004;
	imm32 r5, 0x00000005;
	imm32 r6, 0x00000006;
	imm32 r7, 0x00000007;
	imm32 p1, 0x00000011;
	imm32 p2, 0x00000012;
.ifndef BFIN_HOST;
	imm32 p3, 0x00000013;
.endif
	imm32 p4, 0x00000014;

	P1 = 4;
	P2 = 6;
	loadsym P5, DATA0;
	loadsym I0, DATA1;

begin:
	ASTAT = R0;	// clear CC
	R0 = CC;
	IF CC R1 = R0;
	[ SP ] = P2;
	P2 = [ SP ];
	JUMP ( PC + P2 );	//brf LABEL1;        // (bp);
	CC = R4 < R5;	// CC FLAG   killed
	R1 = 21;
LABEL1:
	JUMP ( PC + P1 );	// EX1 relative to 'brf LABEL1'
	CC = ! CC;
LABEL2:
	JUMP ( PC + P1 );	//brf LABEL3;
	JUMP ( PC + P2 );	//BAD1;              // UJUMP     killed
LABEL3:
	JUMP ( PC + P1 );	//brf LABELCHK1;
BAD1:
	R7 = [ P5 ];	// LDST      killed

LABELCHK1:
	CHECKREG r0, 0x00000000;
	CHECKREG r1, 0x00000001;
	CHECKREG r2, 0x00000002;
	CHECKREG r3, 0x00000003;
	CHECKREG r4, 0x00000004;
	CHECKREG r5, 0x00000005;
	CHECKREG r6, 0x00000006;
	CHECKREG r7, 0x00000007;

	pass

	.data
DATA0:
	.dd 0x000a0000
	.dd 0x000b0001
	.dd 0x000c0002
	.dd 0x000d0003
	.dd 0x000e0004

DATA1:
	.dd 0x00f00100
	.dd 0x00e00101
	.dd 0x00d00102
	.dd 0x00c00103
