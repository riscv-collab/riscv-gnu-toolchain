//Original:/testcases/seq/c_br_preg_killed_ac/c_br_preg_killed_ac.dsp
// Spec Reference: brcc kills  data cache hits
# mach: bfin

.include "testutils.inc"
	start

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

	P4 = 4;
	P2 = 2;
	loadsym P5, DATA0;
	loadsym I0, DATA1;

begin:
	ASTAT = R0;		// clear CC
	IF !CC JUMP LABEL1;	// (bp);
	CC = R4 < R5;		// CC FLAG   killed
	R1 = 21;
LABEL1:
	JUMP ( PC + P4 );	//brf LABEL2;  // (bp);
	CC = ! CC;
LABEL2:
	JUMP ( PC + P4 );	//brf LABEL3;  //  (bp);
	R2 = - R2;		// ALU2op    killed
LABEL3:
	JUMP ( PC + P4 );	//brf LABEL4;
	R3 <<= 2;		// LOGI2op   killed
LABEL4:
	JUMP ( PC + P4 );	//brf LABEL5;
	R0 = R1 + R2;		// COMP3op   killed
LABEL5:
	JUMP ( PC + P4 );	//brf LABEL6;
	R4 += 3;		// COMPI2opD killed
LABEL6:
	JUMP ( PC + P4 );	//brf LABEL7;  // (bp);
	R5 = 25;		// LDIMMHALF killed
LABEL7:
	JUMP ( PC + P4 );	//brf LABEL8;
	R6 = CC;		// CC2REG    killed
LABEL8:
	JUMP ( PC + P4 );	//brf LABEL9;
	JUMP ( PC + P2 );	//BAD1;              // UJUMP     killed
LABEL9:
	JUMP ( PC + P4 );	//brf LABELCHK1;
BAD1:
	R7 = [ P5 ];		// LDST      killed

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
