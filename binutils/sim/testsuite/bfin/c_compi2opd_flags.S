//Original:/proj/frio/dv/testcases/core/c_compi2opd_flags/c_compi2opd_flags.dsp
// Spec Reference: compi2opd dregs += imm7 flags (az, an, ac, av0)
# mach: bfin

#include "test.h"
.include "testutils.inc"
	start

	INIT_R_REGS 0;
	ASTAT = R0;	// initialize astat

// AZ for R0
	imm32 r0, 0x00000000;
	R0 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R7 = ASTAT;
	R0 += 1;	// az = 0  an = 0   ac = 0  av0 = 0
	R6 = ASTAT;
	R0 += -1;	// az = 1  an = 0   ac = 1  av0 = 0
	R5 = ASTAT;
	R1 = R0;
	R0 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R4 = ASTAT;
	R0 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R3 = ASTAT;
	CHECKREG r0,  0x00000000;
	CHECKREG r1,  0x00000000;
	CHECKREG r3,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r4,  (_AN);
	CHECKREG r5,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r6,  0x00000000;
	CHECKREG r7,  (_AZ);

// AN, AC for R0
	imm32 r0, 0xffffffff;
	R0 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R7 = ASTAT;
	R1 = R0;
	R0 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R6 = ASTAT;
	R0 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0xFFFFFFFF;
	CHECKREG r1,  0x00000000;
	CHECKREG r5,  (_AN);
	CHECKREG r6,  (_AZ);
	CHECKREG r7,  (_AC0|_AC0_COPY|_AZ);

// AC, AV0 for R0
	imm32 r0, 0x7fffffff;
	R0 += 1;	// az = 0  an = 1   ac = 0  av0 = 1
	R7 = ASTAT;
	R1 = R0;
	R0 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R6 = ASTAT;
	R2 = R0;
	R0 += -1;	// az = 0  an = 0   ac = 1  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0x7FFFFFFE;
	CHECKREG r1,  0x80000000;
	CHECKREG r2,  0x7FFFFFFF;
	CHECKREG r5,  (_VS|_AC0|_AC0_COPY);
	CHECKREG r6,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);  //C
	CHECKREG r7,  (_VS|_V|_V_COPY|_AN); // A

// AZ, AN, AC, AV0 for R0
	R0 = 0;
	ASTAT = R0;
	imm32 r0, 0x80000000;
	R0 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R7 = ASTAT;
	R1 = R0;
	R0 += 1;	// az = 1  an = 1   ac = 0  av0 = 1
	R6 = ASTAT;
	R2 = R0;
	R0 += 1;	// az = 0  an = 1   ac = 0  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0x80000001;
	CHECKREG r1,  0x7FFFFFFF;
	CHECKREG r2,  0x80000000;
	CHECKREG r5,  (_VS|_AN);
	CHECKREG r6,  (_VS|_V|_V_COPY|_AN);
	CHECKREG r7,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);

// AZ for R0
	R1 = 0;
	ASTAT = R1;
	imm32 r1, 0x00000000;
	R1 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R7 = ASTAT;
	R1 += 1;	// az = 0  an = 0   ac = 0  av0 = 0
	R6 = ASTAT;
	R1 += -1;	// az = 1  an = 0   ac = 1  av0 = 0
	R5 = ASTAT;
	R0 = R1;
	R1 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R4 = ASTAT;
	R1 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R3 = ASTAT;
	CHECKREG r0,  0x00000000;
	CHECKREG r1,  0x00000000;
	CHECKREG r3,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r4,  (_AN);
	CHECKREG r5,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r6,  0x00000000;
	CHECKREG r7,  (_AZ);

// AN, AC for R1
	imm32 r1, 0xffffffff;
	R1 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R7 = ASTAT;
	R0 = R1;
	R1 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R6 = ASTAT;
	R1 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0x00000000;
	CHECKREG r1,  0xFFFFFFFF;
	CHECKREG r5,  (_AN);
	CHECKREG r6,  (_AZ);
	CHECKREG r7,  (_AC0|_AC0_COPY|_AZ);

// AC, AV0 for R1
	imm32 r1, 0x7fffffff;
	R1 += 1;	// az = 0  an = 1   ac = 0  av0 = 1
	R7 = ASTAT;
	R0 = R1;
	R1 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R6 = ASTAT;
	R2 = R1;
	R1 += -1;	// az = 0  an = 0   ac = 1  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0x80000000;
	CHECKREG r1,  0x7FFFFFFE;
	CHECKREG r2,  0x7FFFFFFF;
	CHECKREG r5,  (_VS|_AC0|_AC0_COPY);
	CHECKREG r6,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);
	CHECKREG r7,  (_VS|_V|_V_COPY|_AN);

// AZ, AN, AC, AV0 for R1
	R1 = 0;
	ASTAT = R1;
	imm32 r1, 0x80000000;
	R1 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R7 = ASTAT;
	R0 = R1;
	R1 += 1;	// az = 1  an = 1   ac = 0  av0 = 1
	R6 = ASTAT;
	R2 = R1;
	R1 += 1;	// az = 0  an = 1   ac = 0  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0x7FFFFFFF;
	CHECKREG r1,  0x80000001;
	CHECKREG r2,  0x80000000;
	CHECKREG r5,  (_VS|_AN);
	CHECKREG r6,  (_VS|_V|_V_COPY|_AN);
	CHECKREG r7,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);

// AZ for R2
	imm32 r2, 0x00000000;
	ASTAT = R2;
	R2 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R7 = ASTAT;
	R2 += 1;	// az = 0  an = 0   ac = 0  av0 = 0
	R6 = ASTAT;
	R2 += -1;	// az = 1  an = 0   ac = 1  av0 = 0
	R5 = ASTAT;
	R1 = R2;
	R2 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R4 = ASTAT;
	R2 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R3 = ASTAT;
	CHECKREG r1,  0x00000000;
	CHECKREG r2,  0x00000000;
	CHECKREG r3,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r4,  (_AN);
	CHECKREG r5,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r6,  0x00000000;
	CHECKREG r7,  (_AZ);

// AN, AC for R2
	imm32 r2, 0xffffffff;
	R2 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R7 = ASTAT;
	R1 = R2;
	R2 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R6 = ASTAT;
	R2 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R5 = ASTAT;
	CHECKREG r2,  0xFFFFFFFF;
	CHECKREG r1,  0x00000000;
	CHECKREG r5,  (_AN);
	CHECKREG r6,  (_AZ);
	CHECKREG r7,  (_AC0|_AC0_COPY|_AZ);

// AC, AV0 for R2
	imm32 r2, 0x7fffffff;
	R2 += 1;	// az = 0  an = 1   ac = 0  av0 = 1
	R7 = ASTAT;
	R0 = R2;
	R2 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R6 = ASTAT;
	R1 = R2;
	R2 += -1;	// az = 0  an = 0   ac = 1  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0x80000000;
	CHECKREG r1,  0x7FFFFFFF;
	CHECKREG r2,  0x7FFFFFFE;
	CHECKREG r5,  (_VS|_AC0|_AC0_COPY);
	CHECKREG r6,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);
	CHECKREG r7,  (_VS|_V|_V_COPY|_AN);

// AZ, AN, AC, AV0 for R2
	R2 = 0;
	ASTAT = R2;
	imm32 r2, 0x80000000;
	R2 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R7 = ASTAT;
	R0 = R2;
	R2 += 1;	// az = 1  an = 1   ac = 0  av0 = 1
	R6 = ASTAT;
	R1 = R2;
	R2 += 1;	// az = 0  an = 1   ac = 0  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0x7FFFFFFF;
	CHECKREG r1,  0x80000000;
	CHECKREG r2,  0x80000001;
	CHECKREG r5,  (_VS|_AN);
	CHECKREG r6,  (_VS|_V|_V_COPY|_AN);
	CHECKREG r7,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);

// AZ for R3
	imm32 r3, 0x00000000;
	ASTAT = R3;
	R3 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R7 = ASTAT;
	R3 += 1;	// az = 0  an = 0   ac = 0  av0 = 0
	R6 = ASTAT;
	R3 += -1;	// az = 1  an = 0   ac = 1  av0 = 0
	R5 = ASTAT;
	R0 = R3;
	R3 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R4 = ASTAT;
	R3 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R2 = ASTAT;
	CHECKREG r0,  0x00000000;
	CHECKREG r2,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r3,  0x00000000;
	CHECKREG r4,  (_AN);
	CHECKREG r5,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r6,  0x00000000;
	CHECKREG r7,  (_AZ);

// AN, AC for R3
	imm32 r3, 0xffffffff;
	R3 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R7 = ASTAT;
	R0 = R3;
	R3 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R6 = ASTAT;
	R3 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0x00000000;
	CHECKREG r3,  0xFFFFFFFF;
	CHECKREG r5,  (_AN);
	CHECKREG r6,  (_AZ);
	CHECKREG r7,  (_AC0|_AC0_COPY|_AZ);

// AC, AV0 for R3
	imm32 r3, 0x7fffffff;
	R3 += 1;	// az = 0  an = 1   ac = 0  av0 = 1
	R7 = ASTAT;
	R0 = R3;
	R3 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R6 = ASTAT;
	R1 = R3;
	R3 += -1;	// az = 0  an = 0   ac = 1  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0x80000000;
	CHECKREG r1,  0x7FFFFFFF;
	CHECKREG r3,  0x7FFFFFFE;
	CHECKREG r5,  (_VS|_AC0|_AC0_COPY);
	CHECKREG r6,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);
	CHECKREG r7,  (_VS|_V|_V_COPY|_AN);

// AZ, AN, AC, AV0 for R3
	R3 = 0;
	ASTAT = R3;
	imm32 r3, 0x80000000;
	R3 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R7 = ASTAT;
	R0 = R3;
	R3 += 1;	// az = 1  an = 1   ac = 0  av0 = 1
	R6 = ASTAT;
	R1 = R3;
	R3 += 1;	// az = 0  an = 1   ac = 0  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0x7FFFFFFF;
	CHECKREG r1,  0x80000000;
	CHECKREG r3,  0x80000001;
	CHECKREG r5,  (_VS|_AN);
	CHECKREG r6,  (_VS|_V|_V_COPY|_AN);
	CHECKREG r7,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);

// AZ for R4
	imm32 r4, 0x00000000;
	ASTAT = R4;
	R4 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R7 = ASTAT;
	R4 += 1;	// az = 0  an = 0   ac = 0  av0 = 0
	R6 = ASTAT;
	R4 += -1;	// az = 1  an = 0   ac = 1  av0 = 0
	R5 = ASTAT;
	R1 = R4;
	R4 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R3 = ASTAT;
	R4 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R2 = ASTAT;
	CHECKREG r1,  0x00000000;
	CHECKREG r2,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r3,  (_AN);
	CHECKREG r4,  0x00000000;
	CHECKREG r5,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r6,  0x00000000;
	CHECKREG r7,  (_AZ);

// AN, AC for R4
	imm32 r4, 0xffffffff;
	R4 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R7 = ASTAT;
	R1 = R4;
	R4 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R6 = ASTAT;
	R4 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R5 = ASTAT;
	CHECKREG r1,  0x00000000;
	CHECKREG r4,  0xFFFFFFFF;
	CHECKREG r5,  (_AN);
	CHECKREG r6,  (_AZ);
	CHECKREG r7,  (_AC0|_AC0_COPY|_AZ);

// AC, AV0 for R4
	imm32 r4, 0x7fffffff;
	R4 += 1;	// az = 0  an = 1   ac = 0  av0 = 1
	R7 = ASTAT;
	R1 = R4;
	R4 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R6 = ASTAT;
	R2 = R4;
	R4 += -1;	// az = 0  an = 0   ac = 1  av0 = 0
	R5 = ASTAT;
	CHECKREG r1,  0x80000000;
	CHECKREG r2,  0x7FFFFFFF;
	CHECKREG r4,  0x7FFFFFFE;
	CHECKREG r5,  (_VS|_AC0|_AC0_COPY);
	CHECKREG r6,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);
	CHECKREG r7,  (_VS|_V|_V_COPY|_AN);

// AZ, AN, AC, AV0 for R4
	R4 = 0;
	ASTAT = R4;
	imm32 r4, 0x80000000;
	R4 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R7 = ASTAT;
	R1 = R4;
	R4 += 1;	// az = 1  an = 1   ac = 0  av0 = 1
	R6 = ASTAT;
	R2 = R4;
	R4 += 1;	// az = 0  an = 1   ac = 0  av0 = 0
	R5 = ASTAT;
	CHECKREG r1,  0x7FFFFFFF;
	CHECKREG r2,  0x80000000;
	CHECKREG r4,  0x80000001;
	CHECKREG r5,  (_VS|_AN);
	CHECKREG r6,  (_VS|_V|_V_COPY|_AN);
	CHECKREG r7,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);

// AZ for R5
	imm32 r5, 0x00000000;
	ASTAT = R5;
	R5 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R7 = ASTAT;
	R5 += 1;	// az = 0  an = 0   ac = 0  av0 = 0
	R6 = ASTAT;
	R5 += -1;	// az = 1  an = 0   ac = 1  av0 = 0
	R2 = ASTAT;
	R0 = R5;
	R5 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R4 = ASTAT;
	R5 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R3 = ASTAT;
	CHECKREG r0,  0x00000000;
	CHECKREG r2,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r3,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r4,  (_AN);
	CHECKREG r5,  0x00000000;
	CHECKREG r6,  0x00000000;
	CHECKREG r7,  (_AZ);

// AN, AC for R5
	imm32 r5, 0xffffffff;
	R5 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R7 = ASTAT;
	R0 = R5;
	R5 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R6 = ASTAT;
	R5 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R4 = ASTAT;
	CHECKREG r0,  0x00000000;
	CHECKREG r4,  (_AN);
	CHECKREG r5,  0xFFFFFFFF;
	CHECKREG r6,  (_AZ);
	CHECKREG r7,  (_AC0|_AC0_COPY|_AZ);

// AC, AV0 for R5
	imm32 r5, 0x7fffffff;
	R5 += 1;	// az = 0  an = 1   ac = 0  av0 = 1
	R7 = ASTAT;
	R0 = R5;
	R5 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R6 = ASTAT;
	R2 = R5;
	R5 += -1;	// az = 0  an = 0   ac = 1  av0 = 0
	R4 = ASTAT;
	CHECKREG r0,  0x80000000;
	CHECKREG r2,  0x7FFFFFFF;
	CHECKREG r4,  (_VS|_AC0|_AC0_COPY);
	CHECKREG r5,  0x7FFFFFFE;
	CHECKREG r6,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);
	CHECKREG r7,  (_VS|_V|_V_COPY|_AN);

// AZ, AN, AC, AV0 for R5
	R5 = 0;
	ASTAT = R5;
	imm32 r5, 0x80000000;
	R5 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R7 = ASTAT;
	R0 = R5;
	R5 += 1;	// az = 1  an = 1   ac = 0  av0 = 1
	R6 = ASTAT;
	R2 = R5;
	R5 += 1;	// az = 0  an = 1   ac = 0  av0 = 0
	R4 = ASTAT;
	CHECKREG r0,  0x7FFFFFFF;
	CHECKREG r2,  0x80000000;
	CHECKREG r4,  (_VS|_AN);
	CHECKREG r5,  0x80000001;
	CHECKREG r6,  (_VS|_V|_V_COPY|_AN);
	CHECKREG r7,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);

// AZ for R6
	imm32 r6, 0x00000000;
	ASTAT = R6;
	R6 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R7 = ASTAT;
	R6 += 1;	// az = 0  an = 0   ac = 0  av0 = 0
	R0 = ASTAT;
	R6 += -1;	// az = 1  an = 0   ac = 1  av0 = 0
	R5 = ASTAT;
	R1 = R6;
	R6 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R4 = ASTAT;
	R6 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R3 = ASTAT;
	CHECKREG r0,  0x00000000;
	CHECKREG r1,  0x00000000;
	CHECKREG r3,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r4,  (_AN);
	CHECKREG r5,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r6,  0x00000000;
	CHECKREG r7,  (_AZ);

// AN, AC for R6
	imm32 r6, 0xffffffff;
	R6 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R7 = ASTAT;
	R1 = R6;
	R6 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R4 = ASTAT;
	R6 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R5 = ASTAT;
	CHECKREG r1,  0x00000000;
	CHECKREG r4,  (_AZ);
	CHECKREG r5,  (_AN);
	CHECKREG r6,  0xFFFFFFFF;
	CHECKREG r7,  (_AC0|_AC0_COPY|_AZ);

// AC, AV0 for R6
	R6 = 0;
	ASTAT = R6;
	imm32 r6, 0x7fffffff;
	R6 += 1;	// az = 0  an = 1   ac = 0  av0 = 1
	R7 = ASTAT;
	R0 = R6;
	R6 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R4 = ASTAT;
	R1 = R6;
	R6 += -1;	// az = 0  an = 0   ac = 1  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0x80000000;
	CHECKREG r1,  0x7FFFFFFF;
	CHECKREG r4,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);
	CHECKREG r5,  (_VS|_AC0|_AC0_COPY);
	CHECKREG r6,  0x7FFFFFFE;
	CHECKREG r7,  (_VS|_V|_V_COPY|_AN);

// AZ, AN, AC, AV0 for R6
	R6 = 0;
	ASTAT = R6;
	imm32 r6, 0x80000000;
	R6 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R7 = ASTAT;
	R0 = R6;
	R6 += 1;	// az = 1  an = 1   ac = 0  av0 = 1
	R4 = ASTAT;
	R1 = R6;
	R6 += 1;	// az = 0  an = 1   ac = 0  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0x7FFFFFFF;
	CHECKREG r1,  0x80000000;
	CHECKREG r4,  (_VS|_V|_V_COPY|_AN);
	CHECKREG r5,  (_VS|_AN);
	CHECKREG r6,  0x80000001;
	CHECKREG r7,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);

// AZ for R7
	imm32 r7, 0x00000000;
	ASTAT = R7;
	R7 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R1 = ASTAT;
	R7 += 1;	// az = 0  an = 0   ac = 0  av0 = 0
	R6 = ASTAT;
	R7 += -1;	// az = 1  an = 0   ac = 1  av0 = 0
	R5 = ASTAT;
	R0 = R7;
	R7 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R4 = ASTAT;
	R7 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R2 = ASTAT;
	CHECKREG r0,  0x00000000;
	CHECKREG r1,  (_AZ);
	CHECKREG r2,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r4,  (_AN);
	CHECKREG r5,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r6,  0x00000000;
	CHECKREG r7,  0x00000000;

// AN, AC for R7
	imm32 r7, 0xffffffff;
	R7 += 1;	// az = 1  an = 0   ac = 1  av0 = 0
	R4 = ASTAT;
	R0 = R7;
	R7 += 0;	// az = 1  an = 0   ac = 0  av0 = 0
	R6 = ASTAT;
	R7 += -1;	// az = 0  an = 1   ac = 0  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0x00000000;
	CHECKREG r4,  (_AC0|_AC0_COPY|_AZ);
	CHECKREG r5,  (_AN);
	CHECKREG r6,  (_AZ);
	CHECKREG r7,  0xFFFFFFFF;

// AC, AV0 for R7
	R7 = 0;
	ASTAT = R7;
	imm32 r7, 0x7fffffff;
	R7 += 1;	// az = 0  an = 1   ac = 0  av0 = 1
	R4 = ASTAT;
	R0 = R7;
	R7 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R6 = ASTAT;
	R1 = R7;
	R7 += -1;	// az = 0  an = 0   ac = 1  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0x80000000;
	CHECKREG r1,  0x7FFFFFFF;
	CHECKREG r4,  (_VS|_V|_V_COPY|_AN);
	CHECKREG r5,  (_VS|_AC0|_AC0_COPY);
	CHECKREG r6,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);
	CHECKREG r7,  0x7FFFFFFE;

// AZ, AN, AC, AV0 for R7
	R7 = 0;
	ASTAT = R7;
	imm32 r7, 0x80000000;
	R7 += -1;	// az = 0  an = 0   ac = 1  av0 = 1
	R4 = ASTAT;
	R0 = R7;
	R7 += 1;	// az = 1  an = 1   ac = 0  av0 = 1
	R6 = ASTAT;
	R1 = R7;
	R7 += 1;	// az = 0  an = 1   ac = 0  av0 = 0
	R5 = ASTAT;
	CHECKREG r0,  0x7FFFFFFF;
	CHECKREG r1,  0x80000000;
	CHECKREG r4,  (_VS|_V|_V_COPY|_AC0|_AC0_COPY);
	CHECKREG r5,  (_VS|_AN);
	CHECKREG r6,  (_VS|_V|_V_COPY|_AN);
	CHECKREG r7,  0x80000001;

	pass
