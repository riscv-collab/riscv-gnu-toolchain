# Blackfin testcase for multiple pending IVGs vs masked state
# mach: bfin
# sim: --environment operating

#include "test.h"
	.include "testutils.inc"

	# This test keeps P5 as the base of the EVT table

	.macro set_evt lvl:req, sym:req
	loadsym R1, \sym;
	[P5 + 4 * \lvl\()] = R1;
	.endm

	.macro check_cec mmr:req, valid:req
	imm32 P3, \mmr;
	R0 = [P3];
	R1 = ~0x1f;
	R0 = R0 & R1;
	imm32 R1, \valid;
	CC = R1 == R0;
	IF CC JUMP 1f;
	dbg_fail
1:
	.endm

	.macro delay cnt:req
	imm32 P2, \cnt
	LSETUP (1f, 1f) LC1 = P2;
1:	mnop;
	.endm

	start

	# First mark all EVTs as fails (they shouldn't be activated)
	imm32 P5, EVT0;
	P1 = P5;
	loadsym R1, fail_lvl
	imm32 P2, 16
	LSETUP (1f, 1f) LC0 = P2;
1:	[P1++] = R1;

	# Lower ourselves to EVT15
	set_evt 15, evt15;
	R7 = 0 (x);
	BITSET (R7, 15);
	sti R7;
	loadsym R1, wait;
	RETI = R1;
	RAISE 15;
	RTI;

wait:
	jump wait;

evt15:
	# We shouldn't come back here
	set_evt 15, fail_lvl;

	# Activate interrupt nesting early
	[--SP] = RETI;

	# Raise some higher levels, but they should be masked and so
	# they should never be activated ...
	RAISE 6;
	RAISE 5;
	RAISE 9;
	RAISE 12;

	# Only IVG15 should be pending
	check_cec IPEND, (1<<15);

	# But all should be latched
	check_cec ILAT, (1<<5) | (1<<6) | (1<<9) | (1<<12);

	# Delay a little in case a higher level wrongly activates
	delay 30

	# If we're still here, things are still good.  So let's
	# transition up *slightly*, but not to the highest latched.
	set_evt 12, evt12;
	cli R7;
	BITSET (R7, 12);
	sti R7;

	# Let CEC raise us to IVG12
	delay 30
	# CEC should have been faster than this ...
	dbg_fail

evt12:
	# We shouldn't come back here
	set_evt 12, fail_lvl;

	# Raise some higher levels, but they should be masked and so
	# they should never be activated ...
	RAISE 11;

	# Both IVG15 and IVG12 should be pending
	check_cec IPEND, (1<<15) | (1<<12);

	# But all should be latched
	check_cec ILAT, (1<<5) | (1<<6) | (1<<9) | (1<<11);

	# Activate interrupt nesting a little later
	[--SP] = RETI;

	# Still here, so unmask a higher IVG again to move up
	set_evt 9, evt9;
	cli R7;
	BITSET (R7, 9);
	sti R7;
	delay 30

	# CEC should have been faster than this ...
	dbg_fail

evt9:
	# We shouldn't come back here
	set_evt 9, fail_lvl;

	# IVG9 should also be pending now
	check_cec IPEND, (1<<15) | (1<<12) | (1<<9);

	# But all should be latched
	check_cec ILAT, (1<<5) | (1<<6) | (1<<11);

	# Unmask the next level, but IPEND[4] is set, so we should stay here
	set_evt 6, evt6;
	cli R7;
	BITSET (R7, 6);
	sti R7;

	# Delay a little in case a higher level wrongly activates
	delay 30

	# Good, now unmask things globally
	[--SP] = RETI;
	delay 30

	# CEC should have been faster than this ...
	dbg_fail

evt6:
	# We shouldn't come back here
	set_evt 6, fail_lvl;

	# IVG6 should also be pending now
	check_cec IPEND, (1<<15) | (1<<12) | (1<<9) | (1<<6);

	# But all should be latched
	check_cec ILAT, (1<<5) | (1<<11);

	# Activate interrupt nesting a little later
	[--SP] = RETI;

	# Unmask the next level, but do it via IMASK
	set_evt 5, evt5;
	imm32 P2, IMASK;
	R7 = [P2];
	BITSET (R7, 5);
	[P2] = R7;
	delay 30

	# CEC should have been faster than this ...
	dbg_fail

evt5:
	# We shouldn't come back here
	set_evt 5, fail_lvl;

	# IVG5 should also be pending now
	check_cec IPEND, (1<<15) | (1<<12) | (1<<9) | (1<<6) | (1<<5);

	# But all should be latched
	check_cec ILAT, (1<<11);

	# All good!
	dbg_pass;

fail_lvl:
	dbg_fail;
