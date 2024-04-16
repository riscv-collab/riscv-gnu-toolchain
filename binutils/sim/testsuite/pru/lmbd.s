# Check that lmbd insn works.
# mach: pru

# Copyright (C) 2020-2024 Free Software Foundation, Inc.
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

.include "testutils.inc"

	start

	ldi32 r14, 0xffffffff
	ldi32 r15, 0x0
	ldi32 r16, 0x40000000
	ldi32 r17, 8

	lmbd r0, r14, 0
	qbne 2f, r0, 32

	lmbd r0, r14, 1
	qbne 2f, r0, 31

	lmbd r0, r15, 1
	qbne 2f, r0, 32

	lmbd r0, r15, 0
	qbne 2f, r0, 31

	lmbd r0, r16, r15
	qbne 2f, r0, 31

	lmbd r0, r16, 1
	qbne 2f, r0, 30

	lmbd r0, r14.w1, 1
	qbne 2f, r0, 15

	lmbd r0, r17.b0, 1
	qbne 2f, r0, 3

	lmbd r0, r17.b0, r15
	qbne 2f, r0, 7


1:
	pass
2:	fail
