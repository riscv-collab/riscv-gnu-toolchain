# Blackfin testcase for setting all ASTAT bits via CC
# mach: bfin

# We encode the opcodes directly since we test reserved bits
# which lack an insn in the ISA for it.  It's a 16bit insn;
# the low 8 bits are always 0x03 while the encoding for the
# high 8 bits are:
#  bit 7   - direction
#            0: CC=...;
#            1: ...=CC;
#  bit 6/5 - operation
#            0: = assignment
#            1: | bit or
#            2: & bit and
#            3: ^ bit xor
#  bit 4-0 - the bit in ASTAT to access

	.include "testutils.inc"

	.macro _do dir:req, op:req, bit:req, bit_in:req, cc_in:req, bg_val:req, bit_out:req, cc_out:req
	/* CC = CC; is invalid, so skip it */
	.if \bit != 5

	/* Calculate the before and after ASTAT values */
	imm32 R1, (\bg_val & ~((1 << \bit) | (1 << 5))) | (\bit_in << \bit) | (\cc_in << 5);
	imm32 R3, (\bg_val & ~((1 << \bit) | (1 << 5))) | (\bit_out << \bit) | (\cc_out << 5);

	/* Test the actual opcode */
	ASTAT = R1;
	.byte (\dir << 7) | (\op << 5) | \bit
	.byte 0x03
	R2 = ASTAT;

	/* Make sure things line up */
	CC = R3 == R2;
	IF !CC JUMP 1f;
	JUMP 2f;
1:	fail
2:
	.endif

	/* Recurse through all the bits */
	.if \bit > 0
	_do \dir, \op, \bit - 1, \bit_in, \cc_in, \bg_val, \bit_out, \cc_out
	.endif
	.endm

	/* Test different background fields on ASTAT */
	.macro do dir:req, op:req, bit_in:req, cc_in:req, bit_out:req, cc_out:req
	_do \dir, \op, 31, \bit_in, \cc_in, 0, \bit_out, \cc_out
	_do \dir, \op, 31, \bit_in, \cc_in, -1, \bit_out, \cc_out
	.endm

	start
	nop;

_cc_eq_bit:	/* CC = bit */
	do 0, 0, 0, 0, 0, 0
	do 0, 0, 0, 1, 0, 0
	do 0, 0, 1, 0, 1, 1
	do 0, 0, 1, 1, 1, 1
_bit_eq_cc:	/* bit = CC */
	do 1, 0, 0, 0, 0, 0
	do 1, 0, 0, 1, 1, 1
	do 1, 0, 1, 0, 0, 0
	do 1, 0, 1, 1, 1, 1

_cc_or_bit:	/* CC |= bit */
	do 0, 1, 0, 0, 0, 0
	do 0, 1, 0, 1, 0, 1
	do 0, 1, 1, 0, 1, 1
	do 0, 1, 1, 1, 1, 1
_bit_or_cc:	/* bit |= CC */
	do 1, 1, 0, 0, 0, 0
	do 1, 1, 0, 1, 1, 1
	do 1, 1, 1, 0, 1, 0
	do 1, 1, 1, 1, 1, 1

_cc_and_bit:	/* CC &= bit */
	do 0, 2, 0, 0, 0, 0
	do 0, 2, 0, 1, 0, 0
	do 0, 2, 1, 0, 1, 0
	do 0, 2, 1, 1, 1, 1
_bit_and_cc:	/* bit &= CC */
	do 1, 2, 0, 0, 0, 0
	do 1, 2, 0, 1, 0, 1
	do 1, 2, 1, 0, 0, 0
	do 1, 2, 1, 1, 1, 1

_cc_xor_bit:	/* CC ^= bit */
	do 0, 3, 0, 0, 0, 0
	do 0, 3, 0, 1, 0, 1
	do 0, 3, 1, 0, 1, 1
	do 0, 3, 1, 1, 1, 0
_bit_xor_cc:	/* bit ^= CC */
	do 1, 3, 0, 0, 0, 0
	do 1, 3, 0, 1, 1, 1
	do 1, 3, 1, 0, 1, 0
	do 1, 3, 1, 1, 0, 1

	pass
