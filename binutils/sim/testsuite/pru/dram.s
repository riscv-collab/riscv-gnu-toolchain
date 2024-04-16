# Check that DRAM memory access works.
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

	fill r20, 16
	ldi r10, 0
	not r10, r10
	qbne F, r20, r10
	qbne F, r21, r10
	qbne F, r22, r10
	qbne F, r23, r10

	zero r20, 16
	qbne F, r20, 0
	qbne F, r21, 0
	qbne F, r22, 0
	qbne F, r23, 0

	ldi r0, testarray
	lbbo &r20, r0, 0, 7
	qbne F, r20.b0, 0x01
	qbne F, r20.b1, 0x23
	qbne F, r20.b2, 0x45
	qbne F, r20.b3, 0x67
	qbne F, r21.b0, 0x89
	qbne F, r21.b1, 0xab
	qbne F, r21.b2, 0xcd
	qbne F, r21.b3, 0x00 ; Should not have been loaded!
	qbne F, r22, 0
	qbne F, r23, 0

	ldi r1, 0x11
	sbbo &r1, r0, 9, 1
	ldi r1, 0x11
	sbbo &r1, r0, 12, 4

	lbbo &r20, r0, 0, 16
	qbne F, r21.b3, 0xef
	qbne F, r22.b0, 0xff
	qbne F, r22.b1, 0x11
	qbne F, r22.b2, 0xff
	qbne F, r22.b3, 0xff
	qbne F, r23, 0x11

	pass
F:	fail

	.data
testarray:
	.byte 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef
	.byte 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
