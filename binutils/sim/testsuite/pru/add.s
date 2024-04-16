# Check that basic add insn works.
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

	ldi r4, 10
	add r4, r4, 23
	qbne 2f, r4, 33

	qblt 2f, r4, 33

	qbgt 2f, r4, 33

	jmp 1f

	fail

1:
	pass
2:	fail
