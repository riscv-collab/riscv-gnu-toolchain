# Blackfin testcase for CC/A0/A1 compares
# mach: bfin

#include "test.h"
	.include "testutils.inc"

	start

/* Clear ASTAT before test */
#define CHECK_ASTAT(op, exp) ASTAT = R2; CC = A0 op A1; check_astat exp
	.macro check_astat exp:req
	R5 = ASTAT;
	R6 = \exp;
	CC = R5 == R6;
	IF !CC JUMP 1f;
	.endm

	.macro _acc_test exp_eq:req, exp_le:req, exp_lt:req
	CHECK_ASTAT(==, \exp_eq)
	CHECK_ASTAT(<=, \exp_le)
	CHECK_ASTAT(<, \exp_lt)

	jump 2f;
1:	fail
2:
	.endm

	.macro acc_test acc0:req, acc1:req, eq:req, le:req, lt:req
	dmm32 A0, \acc0
	dmm32 A1, \acc1
	_acc_test \eq, \le, \lt
	.endm

	.macro acc_ex_test a0x:req, a0w:req, a1x:req, a1w:req, eq:req, le:req, lt:req
	imm32 R0, \a0w
	A0.W = R0;
	R0 = \a0x;
	A0.X = R0;
	imm32 R1, \a1w
	A1.W = R1;
	R1 = \a1x;
	A1.X = R1;
	_acc_test \eq, \le, \lt
	.endm

	# Keep R2 with a value of 0
	imm32 R2, 0

#define _EQ     _AC0|_CC|_AC0_COPY|_AZ, _AC0|_CC|_AC0_COPY|_AZ, _AC0|    _AC0_COPY|_AZ
#define _POS_GT                    _AN,      _CC|          _AN,      _CC|          _AN
#define _POS_LT _AC0|    _AC0_COPY    , _AC0|    _AC0_COPY    , _AC0|    _AC0_COPY
#define _NEG_GT _AC0|    _AC0_COPY|_AN, _AC0|_CC|_AC0_COPY|_AN, _AC0|_CC|_AC0_COPY|_AN
#define _NEG_LT 0, 0, 0

	# Simple tests around zero
	acc_test      0,      0, _EQ
	acc_test      0,      1, _POS_GT
	acc_test      0,  10000, _POS_GT
	acc_test      1,      0, _POS_LT
	acc_test  10000,      0, _POS_LT
	acc_test      0,     -1, _NEG_LT
	acc_test      0, -10000, _NEG_LT
	acc_test     -1,      0, _NEG_GT
	acc_test -10000,      0, _NEG_GT

	# Simple positive-only tests
	acc_test      1,      1, _EQ
	acc_test  10000,  10000, _EQ
	acc_test      1,      2, _POS_GT
	acc_test      1,  20000, _POS_GT
	acc_test      2,      1, _POS_LT
	acc_test  20000,      1, _POS_LT

	# Simple negative-only tests
	acc_test     -1,     -1, _EQ
	acc_test -10000, -10000, _EQ
	acc_test     -1,     -2, _POS_LT
	acc_test     -1, -20000, _POS_LT
	acc_test     -2,     -1, _POS_GT
	acc_test -20000,     -1, _POS_GT

	# Simple postitive/negative tests
	acc_test      1,     -1, _NEG_LT
	acc_test     -1,      1, _NEG_GT
	acc_test      1, -10000, _NEG_LT
	acc_test -10000,      1, _NEG_GT
	acc_test     -1,  10000, _NEG_GT
	acc_test  10000,     -1, _NEG_LT
	acc_test -10000,  10000, _NEG_GT
	acc_test  10000, -10000, _NEG_LT

	# Max boundary limits
#define MAX_POS 0x7f, 0xffffffff
#define MAX_NEG 0x80, 0x00000000
	acc_ex_test    0, 0, MAX_POS, _POS_GT
	acc_ex_test MAX_POS,    0, 0, _POS_LT
	acc_ex_test    0, 1, MAX_POS, _POS_GT
	acc_ex_test MAX_POS,    0, 1, _POS_LT
	acc_ex_test  -1, -1, MAX_POS, _NEG_GT
	acc_ex_test MAX_POS,  -1, -1, _NEG_LT
	acc_ex_test MAX_POS, MAX_POS, _EQ
	acc_ex_test    0, 0, MAX_POS, _POS_GT
	acc_ex_test MAX_POS,    0, 0, _POS_LT
	acc_ex_test    0, 1, MAX_POS, _POS_GT
	acc_ex_test MAX_POS,    0, 1, _POS_LT
	acc_ex_test  -1, -1, MAX_POS, _NEG_GT
	acc_ex_test MAX_POS,  -1, -1, _NEG_LT

	acc_ex_test    0, 0, MAX_NEG, _NEG_LT
	acc_ex_test MAX_NEG,    0, 0, _NEG_GT
	acc_ex_test    0, 1, MAX_NEG, _NEG_LT
	acc_ex_test MAX_NEG,    0, 1, _NEG_GT
	acc_ex_test  -1, -1, MAX_NEG, _POS_LT
	acc_ex_test MAX_NEG,  -1, -1, _POS_GT
	acc_ex_test MAX_NEG, MAX_NEG, _EQ
	acc_ex_test    0, 0, MAX_NEG, _NEG_LT
	acc_ex_test MAX_NEG,    0, 0, _NEG_GT
	acc_ex_test    0, 1, MAX_NEG, _NEG_LT
	acc_ex_test MAX_NEG,    0, 1, _NEG_GT
	acc_ex_test  -1, -1, MAX_NEG, _POS_LT
	acc_ex_test MAX_NEG,  -1, -1, _POS_GT

	acc_ex_test MAX_POS, MAX_NEG, _NEG_LT
	acc_ex_test MAX_NEG, MAX_POS, _NEG_GT

	pass
