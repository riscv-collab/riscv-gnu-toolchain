# Blackfin testcase for having RETI set correctly
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

	# First mark all EVTs as fails (they shouldn't be activated)
	imm32 P5, EVT0;
	P1 = P5;
	loadsym R1, fail_lvl
	imm32 P2, 16
	LSETUP (1f, 1f) LC0 = P2;
1:	[P1++] = R1;

	# We'll bounce up a few
	set_evt 6, evt6;
	set_evt 7, evt7;
	set_evt 8, evt8;
	set_evt 9, evt9;

	# Lower ourselves down so we can RAISE up
	set_evt 14, evt14;
	loadsym R1, wait;
	RETI = R1;
	RAISE 14;
	R7 = -1;
	sti R7;
	RTI;

wait:
	jump wait;

evt14:
	# Activate interrupt nesting early
	[--SP] = RETI;

	# We activate the interrupt here ...
	loadsym R1, 1f;
	RAISE 9;
	# ... but we should RETI here
1:	JUMP fail_lvl;

evt9:
	R2 = RETI;
	CC = R1 == R2;
	IF !CC JUMP fail_lvl;

	# We activate the interrupt here ...
	loadsym R1, 1f;
	RAISE 8;
	[--SP] = RETI;
	# ... but we should RETI here
1:	JUMP fail_lvl;

evt8:
	R2 = RETI;
	CC = R1 == R2;
	IF !CC JUMP fail_lvl;

	# Activate interrupt nesting early
	[--SP] = RETI;

	# We activate the interrupt here ...
	loadsym R1, 1f;
	cli R7;
	RAISE 7;
	sti R7;
	# ... but we should RETI here
1:	JUMP fail_lvl;

evt7:
	R2 = RETI;
	CC = R1 == R2;
	IF !CC JUMP fail_lvl;

	# Activate interrupt nesting early
	[--SP] = RETI;

	# We activate the interrupt here ...
	imm32 P0, IMASK
	R7 = [P0];
	R6 = 0;
	[P0] = R6;
	loadsym R1, 1f;
	RAISE 6;
	[P0] = R7;
	# ... but we should RETI here
	# don't jump to fail_lvl as the pipeline might advance
	# the PC to the fail_lvl point before the ivg actually
	# gets a chance to fire
1:	JUMP 1b;

evt6:
	R2 = RETI;
	CC = R1 == R2;
	IF !CC JUMP fail_lvl;

	dbg_pass

fail_lvl:
	dbg_fail;
