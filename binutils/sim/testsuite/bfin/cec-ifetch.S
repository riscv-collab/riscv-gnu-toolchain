# Blackfin testcase for making sure RETX is the excepting insn
# and not the target of the insn (like indirect jumps)
# mach: bfin
# sim: --environment operating

#include "test.h"
	.include "testutils.inc"

	start

	# Set our handler
	imm32 P5, EVT3;
	loadsym R1, _evtx;
	[P5] = R1;

	# Lower ourselves below EVT3
	loadsym R4, _i_rts;
	RETI = R4;
	RAISE 12;
	RTI;

_i_rts:
	# Check unaligned RETS
	loadsym R6, 1f;
	loadsym R5, 2f;
	R0 = 1;
	RETS = R0;
1:	RTS;
2:

_i_jump:
	# Check unaligned indirect jump
	loadsym R6, 1f;
	loadsym R5, 2f;
	P0 = 1;
1:	JUMP (P0);
2:

_i_jump_off:
	# Check unaligned indirect jump (pc-relative)
	loadsym R6, 1f;
	loadsym R5, 2f;
	P0 = 1;
1:	JUMP (PC + P0);
2:

_i_call:
	# Check unaligned indirect call
	loadsym R6, 1f;
	loadsym R5, 2f;
	P0 = 1;
1:	CALL (P0);
2:

_pass_lvl:
	dbg_pass

_evtx:
	# Make sure R6 matches RETX
	R7 = RETX;
	CC = R6 == R7;
	if !CC jump _fail_lvl;

	# Move on to next test
	RETX = R5;
	RTX;

_fail_lvl:
	dbg_fail
