# Copyright (C) 2016-2024 Free Software Foundation, Inc.
# Contributed by Dimitar Dimitrov <dimitar@dinux.eu>
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
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# MACRO: start
# All assembler tests should start with a call to "main_test"
	.macro start
	.text

	.global _start
_start:

	# Skip over these inlined funcs.
	jmp __main_test;

	.global __pass
	.type __pass, function
__pass:
	# Note - DRAM LMA and VMA are equal for PRU, so
	# we can afford to pass DRAM pointer directly.
	write 1, _passmsg, 5
	exit 0

	.global __fail
	.type __fail, function
__fail:
	write 1, _failmsg, 5
	exit 1

	.data
_passmsg:
	.ascii "pass\n"

_failmsg:
	.ascii "fail\n"

	.text
	.global __main_test
	.type __main_test, function
__main_test:
	.endm

# MACRO: system_call
# Make a libgloss system call
	.macro system_call nr:req, arg1=0, arg2=0, arg3=0
	ldi r1, \nr
	ldi r14, \arg1
	ldi r15, \arg2
	ldi r16, \arg3
	halt
	.endm

# MACRO: exit
# Quit the current test
	.macro exit rc:req
	system_call 1, \rc
	.endm

# MACRO: pass
# Write 'pass' to stdout via syscalls and quit;
# meant for non-OS operating environments
	.macro pass
	jmp __pass;
	.endm

# MACRO: fail
# Write 'fail' to stdout via syscalls and quit;
# meant for non-OS operating environments
	.macro fail
	jmp __fail;
	.endm

# MACRO: write
# Just like the write() C function; uses system calls
	.macro write fd:req, str:req, len:req
	system_call 5, \fd, \str, \len
	.endm

# MACRO: qbne32
# Like qbne instruction, but check a 32-bit constant value.
	.macro qbne32 label:req, op0:req, C0:req
	qbne \label, \op0\().b0, ((\C0) >> 0) & 0xff
	qbne \label, \op0\().b1, ((\C0) >> 8) & 0xff
	qbne \label, \op0\().b2, ((\C0) >> 16) & 0xff
	qbne \label, \op0\().b3, ((\C0) >> 24) & 0xff
	.endm
