# Check that subregister addressing works.
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

	ldi r0, 0x01ff
	add r0, r0.b0, r0.b1
	qbne F, r0.b0, 0x00
	qbne F, r0.b1, 0x01
	qbne F, r0.w2, 0x00

	ldi r0, 0x01ff
	add r0.b0, r0.b0, r0.b1
	adc r0, r0.b1, r0.b3
	qbne F, r0.b0, 0x02
	qbne F, r0.b1, 0x00
	qbne F, r0.w2, 0x00

	pass
F:	fail
