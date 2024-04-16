//Original:testcases/core/c_linkage/c_linkage.dsp
// Spec Reference: linkage (link & unlnk)
# mach: bfin

.include "testutils.inc"
	start

	INIT_R_REGS(0);

	loadsym sp, DATA_ADDR_1, 0x24;
	p0 = sp;

	FP = 0x0064 (X);
	R0 = 5;
	RETS = R0;

	LINK 4;	// push rets, push fp, fp=sp, sp=sp-framesize (4)

	R1 = 3;
	RETS = R1;	// initialize rets by a different value

	loadsym p1, SUBR
	CALL ( P1 );

	SP = 0x3333 (X);

	UNLINK;	// sp = fp, fp = pop (old fp), rets = pop(old rets),

	R2 = RETS;	// for checking

	CHECKREG r0, 0x00000005;
	CHECKREG r1, 0x00000003;
	CHECKREG r2, 0x00000005;
	CHECKREG r3, 0x00000000;
	CHECKREG r4, 0x00000000;
	CHECKREG r5, 0x00000000;
	CHECKREG r6, 0x00001111;
	CHECKREG r7, 0x00000000;
	CHECKREG fp, 0x00000064;
	CC = SP == P0;
	if CC JUMP 1f;
	fail;
1:
	pass

SUBR:				// should jump here
	R6.L = 0x1111;
	RTS;
	R7.L = 0x2222;	// should not go here
	RTS;

	.data
DATA_ADDR_1:
DATA:
	.space (0x0100);

// Stack Segments

	.space (0x100);
KSTACK:
