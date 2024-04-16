# Blackfin testcase for push/pop multiples instructions
# mach: bfin

	.include "testutils.inc"

	# Tests follow the pattern:
	#  - do the push multiple
	#  - write a garbage value to all registers pushed
	#  - do the pop multiple
	#  - check all registers popped against known values

	start

	# Repeat the same operation multiple times, so this:
	#	do_x moo, R, 1
	# becomes this:
	#	moo R1, 0x11111111
	#	moo R0, 0x00000000
	.macro _do_x func:req, reg:req, max:req, x:req
	.ifle (\max - \x)
	\func \reg\()\x, 0x\x\x\x\x\x\x\x\x
	.endif
	.endm
	.macro do_x func:req, reg:req, max:req
	.ifc \reg, R
	_do_x \func, \reg, \max, 7
	_do_x \func, \reg, \max, 6
	.endif
	_do_x \func, \reg, \max, 5
	_do_x \func, \reg, \max, 4
	_do_x \func, \reg, \max, 3
	_do_x \func, \reg, \max, 2
	_do_x \func, \reg, \max, 1
	_do_x \func, \reg, \max, 0
	.endm

	# Keep the garbage value in I0
	.macro loadi reg:req, val:req
	\reg = I0;
	.endm
	imm32 I0, 0xAABCDEFF

	#
	# Test push/pop multiples with (R7:x) syntax
	#

	_push_r_tests:

	# initialize all Rx regs with a known value
	do_x imm32, R, 0

	.macro checkr tochk:req, val:req
	P0 = \tochk;
	imm32 P1, \val
	CC = P0 == P1;
	IF !CC JUMP 8f;
	.endm

	.macro pushr maxr:req
	_push_r\maxr:
	[--SP] = (R7:\maxr);
	do_x loadi, R, \maxr
	(R7:\maxr) = [SP++];
	do_x checkr, R, \maxr
	# need to do a long jump to avoid PCREL issues
	jump 9f;
	8: jump.l 1f;
	9:
	.endm

	pushr 7
	pushr 6
	pushr 5
	pushr 4
	pushr 3
	pushr 2
	pushr 1
	pushr 0

	#
	# Test push/pop multiples with (P5:x) syntax
	#

	_push_p_tests:

	# initialize all Px regs with a known value
	do_x imm32, P, 0

	.macro checkp tochk:req, val:req
	R0 = \tochk;
	imm32 R1, \val
	CC = R0 == R1;
	IF !CC JUMP 8f;
	.endm

	.macro pushp maxp:req
	_push_p\maxp:
	[--SP] = (P5:\maxp);
	do_x loadi, P, \maxp
	(P5:\maxp) = [SP++];
	do_x checkp, P, \maxp
	# need to do a long jump to avoid PCREL issues
	jump 9f;
	8: jump.l 1f;
	9:
	.endm

	# checkp func clobbers R0/R1
	L0 = R0;
	L1 = R1;
	pushp 5
	pushp 4
	pushp 3
	pushp 2
	pushp 1
	pushp 0
	R0 = L0;
	R1 = L1;

	#
	# Test push/pop multiples with (R7:x, P5:x) syntax
	#

	_push_rp_tests:

	.macro _pushrp maxr:req, maxp:req
	_push_r\maxr\()_p\maxp:
	[--SP] = (R7:\maxr, P5:\maxp);
	do_x loadi, R, \maxr
	do_x loadi, P, \maxp
	(R7:\maxr, P5:\maxp) = [SP++];
	# checkr func clobbers P0/P1
	L0 = P0;
	L1 = P1;
	do_x checkr, R, \maxr
	P1 = L1;
	P0 = L0;
	# checkp func clobbers R0/R1
	L0 = R0;
	L1 = R1;
	do_x checkp, P, \maxp
	R0 = L0;
	R1 = L1;
	# need to do a long jump to avoid PCREL issues
	jump 9f;
	8: jump.l 1f;
	9:
	.endm
	.macro pushrp maxr:req
	_pushrp \maxr, 5
	_pushrp \maxr, 4
	_pushrp \maxr, 3
	_pushrp \maxr, 2
	_pushrp \maxr, 1
	_pushrp \maxr, 0
	.endm

	pushrp 7
	pushrp 6
	pushrp 5
	pushrp 4
	pushrp 3
	pushrp 2
	pushrp 1
	pushrp 0

	pass
1:
	fail
