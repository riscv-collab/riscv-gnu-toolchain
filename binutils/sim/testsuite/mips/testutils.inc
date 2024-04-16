# MIPS simulator testsuite utility functions.
# Copyright (C) 2004-2024 Free Software Foundation, Inc.
# Contributed by Chris Demetriou of Broadcom Corporation.
#
# This file is part of the GNU simulators.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


# $1, $4, $5, %6, are used as temps by the macros defined here.

	.macro writemsg msg
	la	$5, 901f
	li	$6, 902f - 901f
	.data
901:	.ascii	"\msg\n"
902:
	.previous
	.set push
	.set noreorder
	jal	_dowrite
	li	$4, 0
	.set pop
	.endm


	# The MIPS simulator uses "break 0x3ff" as the code to exit,
	# with the return value in $4 (a0).
	.macro exit rc
	li	$4, \rc
	break	0x3ff
	.endm


	.macro setup

	.global _start
	.global __start
	.ent _start
_start:
__start:
	.set push
	.set noreorder
	j	DIAG
	nop
	.set pop
	.end _start

	.global _fail
	.ent _fail
_fail:
	writemsg "fail"
	exit 1
	.end _fail

	.global _pass
	.ent _pass
_pass:
	writemsg "pass"
	exit 0
	.end _pass

	# The MIPS simulator can use multiple different monitor types,
	# so we hard-code the simulator "write" reserved instruction opcode,
	# rather than jumping to a vector that invokes it.  The operation
	# expects RA to point to the location at which to continue
	# after writing.
	.global _dowrite
	.ent _dowrite
_dowrite:
	# Write opcode (reserved instruction).  See sim_monitor and its
	# callers in sim/mips/interp.c.
	.word	0x00000039 | ((8 << 1) << 6)
	.end _dowrite

	.endm	# setup


	.macro pass
	.set push
	.set noreorder
	j	_pass
	nop	
	.set pop
	.endm


	.macro fail
	.set push
	.set noreorder
	j	_fail
	nop	
	.set pop
	.endm


	.macro load32 reg, val
	li	\reg, \val
	.endm


	.macro load64 reg, val
	dli	\reg, \val
	.endm


	.macro loadaddr reg, addr
	la	\reg, \addr
	.endm


	.macro checkreg reg, expreg
	.set push
	.set noat
	.set noreorder
	beq	\expreg, \reg, 901f
	nop
	fail
901:
	.set pop
	.endm


	.macro check32 reg, val
	.set push
	.set noat
	load32	$1, \val
	checkreg \reg, $1
	.set pop
	.endm


	.macro check64 reg, val
	.set push
	.set noat
	load64	$1, \val
	checkreg \reg, $1
	.set pop
	.endm


	# Check hi-lo register pair against data stored at base+o1 and base+o2
	# Clobbers $1 - $5
	.macro checkpair lo, hi, base, w, o1, o2
	move  $2, \lo
	move  $3, \hi
	.set noat
	la   $1, \base
	l\w	$4, \o1($1)
	l\w	$5, \o2($1)
	.set at
	checkreg  $2, $4
	checkreg  $3, $5
	.endm

	.macro checkpair_le_d lo, hi, base
	checkpair \lo, \hi, \base, w, 0, 4
	.endm

	.macro checkpair_be_d lo, hi, base
	checkpair \lo, \hi, \base, w, 4, 0
	.endm

	.macro checkpair_le_q lo, hi, base
	checkpair \lo, \hi, \base, d, 0, 8
	.endm

	.macro checkpair_be_q lo, hi, base
	checkpair \lo, \hi, \base, d, 8, 0
	.endm

	# Endian-ness for comparison is determined by reading a word at ec
	.macro checkpair_xendian lo, hi, base, ec, w
	.set noat
	lw   $1, \ec
	andi $1, $1, 0x1
	# check endianess
	beqz  $1, 2f
	.set at
1: # big endian
	checkpair_be_\w \lo, \hi, \base
	b 3f
2: # little endian
	checkpair_le_\w \lo, \hi, \base
3:
	.endm

	.macro checkpair_qword lo, hi, base, oe
	checkpair_xendian \lo, \hi, \base, \oe, q
	.endm

	.macro checkpair_dword lo, hi, base, oe
	checkpair_xendian \lo, \hi, \base, \oe, d
	.endm
