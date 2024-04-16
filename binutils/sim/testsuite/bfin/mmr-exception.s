# Blackfin testcase for MMR exceptions in a lower EVT
# mach: bfin
# sim: --environment operating

	.include "testutils.inc"

	start

	imm32 P0, 0xFFE02000
	loadsym R1, _evx
	[P0 + (4 * 3)] = R1;
	loadsym R1, _ivg9
	[P0 + (4 * 9)] = R1;
	CSYNC;

	RETI = R1;
	RAISE 9;
	R0 = -1;
	STI R0;
	RTI;
	dbg_fail

_ivg9:
	# Invalid MMR
	imm32 P0, 0xFFEE0000
1:	[P0] = R0;
9:	dbg_fail

_evx:
	# Make sure SEQSTAT is set to correct value
	R0 = SEQSTAT;
	R0 = R0.B;
	R1 = 0x2e (x);
	CC = R0 == R1;
	IF !CC JUMP 9b;

	# Make sure RETX is set to correct address
	loadsym R0, 1b;
	R1 = RETX;
	CC = R0 == R1;
	IF !CC JUMP 9b;

	dbg_pass
