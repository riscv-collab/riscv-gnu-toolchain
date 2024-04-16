# Blackfin testcase for aborting an excepting insn immediately
# mach: bfin
# sim: --environment operating

#include "test.h"
	.include "testutils.inc"

	# This test keeps P5 as the base of the EVT table

	.macro set_evt lvl:req, sym:req
	loadsym R1, \sym;
	[P5 + 4 * \lvl\()] = R1;
	.endm

	start

	# Set up exception handler
	imm32 P4, EVT3;
	loadsym R1, _evx;
	[P4] = R1;

	# Lower ourselves to userspace
	loadsym R1, _user;
	RETI = R1;
	RTI;

_user:
	imm32 R0, 0x12345678;
	R1 = R0;
	imm32 P0, 0xffffffff;
	P1 = P0;
_user_fail:
	# Sometimes this even causes immediate double faults when
	# exceptions are not exact since this may trigger multiple
	R0 = [P0++];

	JUMP fail_lvl;

_evx:
	# RETX should be pointing to the right place
	loadsym R6, _user_fail;
	R7 = RETX;
	CC = R6 == R7;
	IF !CC JUMP fail_lvl;

	# R0 and P0 should be unchanged
	CC = R1 == R0;
	IF !CC JUMP fail_lvl;
	CC = P1 == P0;
	IF !CC JUMP fail_lvl;

	dbg_pass
fail_lvl:
	dbg_fail
