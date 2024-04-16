# Check that loop insn works.
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

	ldi r25, 0
	ldi r26, 0
	ldi r27, 0

	add r27, r27, 1
	loop 1f, 10
	add r25, r25, 1
	add r26, r26, 2
1:
	add r27, r27, 1

	qbne F, r25, 10
	qbne F, r26, 20
	qbne F, r27, 2

	pass

F:	fail
