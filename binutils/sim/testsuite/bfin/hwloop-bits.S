# Blackfin testcase for HW Loops and user->super transitions
# mach: bfin
# sim: --environment operating

#include "test.h"
	.include "testutils.inc"

	.macro check_hwloop_regs lc:req, lt:req, lb:req
	R0 = LC0;
	CC = R0 == \lc;
	IF !CC JUMP fail;

	R0 = LT0;
	CC = R0 == \lt;
	IF !CC JUMP fail;

	R0 = LB0;
	CC = R0 == \lb;
	IF !CC JUMP fail;

	R0 = LC1;
	CC = R0 == \lc;
	IF !CC JUMP fail;

	R0 = LT1;
	CC = R0 == \lt;
	IF !CC JUMP fail;

	R0 = LB1;
	CC = R0 == \lb;
	IF !CC JUMP fail;
	.endm

	start

	imm32 P0, EVT3;
	loadsym R0, exception;
	[P0] = R0;

	imm32 P0, EVT2;
	loadsym R0, nmi;
	[P0] = R0;

	loadsym R0, usermode;
	RETI = R0;

	# Set the LC/LB/LT up with LSB set
	#  - Hardware clears LT LSB, but LB remains until we lower
	imm32 R6, 0xaaaa5555
	R4 = R6;
	BITCLR (R4, 0);
	imm32 R7, 0xaa55aa55
	R5 = R7;
	BITCLR (R5, 0);

	LC0 = R6;
	LT0 = R6;
	LB0 = R7;
	LC1 = R6;
	LT1 = R6;
	LB1 = R7;

	# Sanity check
	check_hwloop_regs R6, R4, R7

	RTI;

usermode:
	# Make sure LSB has been cleared in LB
	check_hwloop_regs R6, R4, R5

	# Clear LSB in all LC/LT/LB
	LC0 = R4;
	LT0 = R4;
	LB0 = R5;
	LC1 = R4;
	LT1 = R4;
	LB1 = R5;

	# Now move back up to supervisor
	EXCPT 4;

exception:
	# Make sure LSB is set in LB
	check_hwloop_regs R4, R4, R7

	# Clear the LSB and move up another supervisor level
	LC0 = R4;
	LT0 = R4;
	LB0 = R5;
	LC1 = R4;
	LT1 = R4;
	LB1 = R5;

	RAISE 2;

nmi:
	# Make sure LSB stayed clear
	check_hwloop_regs R4, R4, R5

	dbg_pass

fail:
	dbg_fail
