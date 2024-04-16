# Blackfin testcase for returning to the right place while bouncing between
# multiple CEC levels (like in a Linux system call)
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

	# The OS exception handler
	set_evt 3, _evx;
	# The OS system call handler
	set_evt 15, _evt15;

	# Lower ourselves to userspace
	loadsym R1, _user;
	loadsym R2, _next_user;
	RETI = R1;
	R7 = -1;
	sti R7;
	RTI;

_user:
	EXCPT 0;
_next_user:
	dbg_pass

_evx:
	# RETX should be pointing to the right place
	R1 = RETX;
	CC = R1 == R2;
	IF !CC JUMP fail_lvl;

	# Lower ourselves to the system call handler
	RAISE 15;
	RTX;

_evt15:
	# RETI should be pointing to the right place
	R1 = RETI;
	CC = R1 == R2;
	IF !CC JUMP fail_lvl;

	# Return to userspace now
	RTI;

fail_lvl:
	dbg_fail
