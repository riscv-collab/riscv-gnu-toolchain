//Original:/testcases/seq/se_rets_hazard/se_rets_hazard.dsp
# mach: bfin

.include "testutils.inc"
	start


BOOT:
	FP = SP;	// and frame pointer

	INIT_R_REGS 0;	// initialize general purpose regs




	ASTAT = r0;	// reset sequencer registers

// The Main Program


START:
	loadsym r1, SUB1;
	RETS = r1;
	RTS;

MID1:
	CHECKREG r6, 0;	// shouldn't be BAD
	R6.L = 0xBAD2;	// In case we come back to MID1
	loadsym P1, MID2;
	CALL ( P1 );
	RTS;

MID2:
	loadsym R1, END;
	RETS = r1;
	[ -- SP ] = I0;
	LINK 0;
	I0 = FP;
	UNLINK;
	RTS;

END:

	pass	// Call Endtest Macro

// Subroutines and Functions

SUB1:               // Code goes here
	CHECKREG r7, 0;	// should be if sub executed
	R7.L = 0xBAD;	// In case we come back to SUB1
	loadsym R2, MID1;
	[ -- SP ] = R2;
	RETS = [sp++];
	RTS;
	R6.L = 0xBAD;
