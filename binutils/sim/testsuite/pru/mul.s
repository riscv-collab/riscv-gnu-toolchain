# Check that multiplication works.
# mach: pru

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

.include "testutils.inc"

	start

	# MUL: Test regular case
	ldi r28, 1001
	ldi r29, 4567
	nop
	xin 0, r26, 4
	qbne32 2f, r26, 1001 * 4567

	# MUL: Test the pipeline emulation
	ldi r28, 1002
	ldi r29, 1003
	ldi r29, 4004
	xin 0, r26, 4
	qbne32 2f, r26, 1002 * 1003
	xin 0, r26, 4
	qbne32 2f, r26, 1002 * 4004

	# MUL: Test 64-bit result
	ldi32 r28, 0x12345678
	ldi32 r29, 0xaabbccdd
	nop
	xin 0, r26, 8
	qbne32 2f, r26, 0x45BE4598
	qbne32 2f, r27, 0xC241C38

	# MAC: Test regular case
	ldi r25, 1
	xout 0, r25, 1
	ldi r25, 3
	xout 0, r25, 1

	ldi r25, 1
	ldi r28, 1001
	ldi r29, 2002
	xout 0, r25, 1
	ldi r28, 3003
	ldi r29, 4004
	xout 0, r25, 1

	xin 0, r26, 4
	qbne32 2f, r26, (1001 * 2002) + (3003 * 4004)

	# MAC: Test 64-bit result
	ldi r25, 3
	xout 0, r25, 1

	ldi r25, 1
	ldi32 r28, 0x10203040
	ldi32 r29, 0x50607080
	xout 0, r25, 1
	ldi32 r28, 0xa0b0c0d0
	ldi32 r29, 0x11223344
	xout 0, r25, 1

	xin 0, r26, 8
	qbne32 2f, r26, 0x8E30C740
	qbne32 2f, r27, 0xFD156B1

	jmp 1f

	fail

1:
	pass
2:	fail
