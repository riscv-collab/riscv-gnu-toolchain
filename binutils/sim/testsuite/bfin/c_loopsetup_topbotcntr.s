//Original:/proj/frio/dv/testcases/core/c_loopsetup_topbotcntr/c_loopsetup_topbotcntr.dsp
// Spec Reference: loopsetup top bot counter
# mach: bfin

.include "testutils.inc"
	start

	INIT_R_REGS 0;


	ASTAT = r0;

	R1 = 0x10;
	R2 = 0x20;
	R3 = 0x30;
	R4 = 0x40 (X);
	R5 = 0x08;

	loadsym R6, start1;
	loadsym R7, end1;

	LT0 = R6;
	LB0 = R7;
	LC0 = R5;
//start immmediately
start1:  R0 += 1;
	R1 += -2;
end1:    R2 += 3;
	R3 += 4;

	CHECKREG r0, 0x00000008;
	CHECKREG r1, 0x00000000;
	CHECKREG r2, 0x00000038;
	CHECKREG r3, 0x00000034;
	CHECKREG r4, 0x00000040;
	CHECKREG r5, 0x00000008;
//CHECKREG r6, 0x00000090;
//CHECKREG r7, 0x00000094;

	R0 = 0x05;
	R1 = 0x10;
	R2 = 0x10;
	R3 = 0x10;
	R4 = 0x20;
	R5 = 0x20;
	R6 = 0x30;
	R7 = 0x30;

	loadsym R1, start2;
	R0 = R1;
	loadsym R1, end2;
	LT1 = R0;
	LB1 = R1;
	LC1 = R2;

start2:  R4 += 1;
	R5 += 2;
end2:    R6 += -3;
	R7 += 4;
	CHECKREG r3, 0x00000010;
	CHECKREG r4, 0x00000030;
	CHECKREG r5, 0x00000040;
	CHECKREG r6, 0x00000000;
	CHECKREG r7, 0x00000034;

	R0 = 0x05;
	R1 = 0x10;
	R2 = 0x20;
	R3 = 0x30;
	R4 = 0x40 (X);
	R5 = 0x50 (X);
	R6 = 0x60 (X);
	R7 = 0x70 (X);

	loadsym R1, start3
	r0 = r1;
	loadsym r1, end3;
	LT0 = R0;
	LB0 = R1;
	LC0 = R2;
	loadsym r3, start4;
	loadsym r4, end4;
	LT1 = R3;
	LB1 = R4;
	LC1 = R5;

	R0 = 0x10;
	R1 = 0x15;
	R2 = 0x20;
	R3 = 0x26;
	R4 = 0x30;
	R5 = 0x40 (X);

start3:  R0 += 1;
	R1 += -2;
start4:  R2 += 3;
	R3 += 4;
end4:    R6 += 5;
end3:    R7 += -6;

	CHECKREG r0, 0x00000030;
	CHECKREG r1, 0xFFFFFFD5;
	CHECKREG r2, 0x0000016D;
	CHECKREG r3, 0x000001E2;
	CHECKREG r4, 0x00000030;
	CHECKREG r5, 0x00000040;
	CHECKREG r6, 0x0000028B;
	CHECKREG r7, 0xFFFFFFB0;

	pass
