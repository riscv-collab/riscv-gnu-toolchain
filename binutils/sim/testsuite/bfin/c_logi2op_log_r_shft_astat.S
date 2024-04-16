# Test ASTAT bits with logical right shift (>>=)
# mach: bfin

.include "testutils.inc"
#include "test.h"
start

.macro __do val:req, shift:req, exp:req
	# First test when ASTAT starts with all bits cleared
	imm32 R2, \val;
	ASTAT = R0;
	R2 >>= \shift;
	R3 = ASTAT;
	CHECKREG R2, (\val >> \shift);
	CHECKREG R3, \exp;

	# Then test when ASTAT starts with all bits set
	imm32 R2, \val;
	ASTAT = R1;
	R2 >>= \shift;
	R3 = ASTAT;
	CHECKREG R3, (\exp) | ~(_AZ|_AN|_V|_V_COPY);
.endm

.macro _do shift:req, val:req
	# Automatically test all shifted values
	.if ((\val >> \shift) & 0xffffffff) == 0
		__do \val, \shift, _AZ
	.else
		.if (\val >> \shift) == 0x80000000
			__do \val, \shift, _AN
		.else
			__do \val, \shift, 0
		.endif
	.endif
	.if (\val >> 1) & 0xffffffff
		_do \shift, (\val >> 1)
	.endif
.endm

.macro do shift:req
_l_shft_\shift:
	_do \shift, 0x80000000
.endm

R0 = 0;
R1 = -1;

do 0
do 1
do 2
do 3
do 4
do 5
do 6
do 7
do 8
do 9
do 10
do 11
do 12
do 13
do 14
do 15
do 16
do 17
do 18
do 19
do 20
do 21
do 22
do 23
do 24
do 25
do 26
do 27
do 28
do 29
do 30
do 31

pass
