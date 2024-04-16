# Blackfin testcase for having RETI LSB set correctly when not self nested
# mach: bfin
# sim: --environment operating

#include "test.h"
	.include "testutils.inc"

	start

	# Set our handler
	imm32 P5, EVT11;
	loadsym R1, _ivg11;
	[P5] = R1;
	loadsym R1, _fail_lvl;
	[P5 + 4] = R1;	/* IVG12 */
	[P5 + 12] = R1;	/* IVG14 */
	loadsym R1, _ivg13;
	[P5 + 8] = R1;

	# Disable self nesting
	R2 = SYSCFG;
	BITCLR (R2, 2);
	SYSCFG = R2;
	CSYNC;

	# Enable IVG11/IVG13/IVG14 but not IVG12
	cli R3;
	BITSET (R3, 11);
	BITCLR (R3, 12);
	BITSET (R3, 13);
	BITSET (R3, 14);
	sti R3;

	# Counters to keep track of nesting depth
	R7 = 0;
	R5 = 0;

	# Lower ourselves to IVG11
	loadsym R4, _fail_lvl;
	RETI = R4;
	RAISE 11;
	RAISE 12;
	RAISE 13;
	RAISE 14;
	RTI;

# This IVG makes sure we don't re-enter when self RAISE is pending
_ivg11:
	R0 = RETI;

	# Make sure we are indeed at IVG11
	imm32 P0, IPEND;
	R1 = [P0];
	CC = BITTST (R1, 11);
	IF !CC JUMP _fail_lvl;

	# Should not be re-entering
	CC = R5 == 0;
	IF !CC JUMP _fail_lvl;

	# Make sure LSB of RETI is not set
	CC = BITTST (R0, 0);
	IF CC JUMP _fail_lvl;

	# Try to avoid nesting a few times
	R5 += 1;
	R6 = 3;
	CC = R7 < R6;
	IF !CC JUMP 1f;
	[--sp] = RETI;
	R7 += 1;
	RAISE 11;
	MNOP;NOP;MNOP;NOP;
	R5 = 0;
	RTI;

	# Move down to IVG13 for next test
1:	loadsym R4, _fail_lvl;
	RETI = R4;
	RTI;

# This IVG makes sure RETI LSB is ignored on transition out (RTI)
_ivg13:
	R0 = RETI;

	# Make sure we are indeed at IVG13
	imm32 P0, IPEND;
	R1 = [P0];
	CC = BITTST (R1, 13);
	IF !CC JUMP _fail_lvl;

	# RETI LSB should not be set when entering IVG13
	CC = BITTST (R0, 0);
	IF CC JUMP _fail_lvl;

	# Should get here only after a few IVG11 tests
	CC = R7 == R6;
	IF !CC JUMP _fail_lvl;

	# Make sure IVG13 isn't pending
	imm32 P0, ILAT;
	R1 = [P0];
	CC = BITTST (R1, 13);
	IF CC JUMP _fail_lvl;

	# Manually set RETI to with LSB set so we should stay at IVG13
	# even though SNEN is disabled
	loadsym R1, 1f;
	BITSET (R1, 0);
	RETI = R1;
	R7 += 1;
	RTI;

1:	# Make sure we get here in right number of tests
	R6 = 4;
	CC = R7 == R6;
	IF !CC JUMP _fail_lvl;

	# Make sure we are still at IVG13
	imm32 P0, IPEND;
	R1 = [P0];
	CC = BITTST (R1, 13);
	IF !CC JUMP _fail_lvl;

	dbg_pass

_fail_lvl:
	dbg_fail;
