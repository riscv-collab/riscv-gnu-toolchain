# Check that DMEM zero address access can be trapped.
# mach: pru
# sim: --error-null-deref
# xerror:
# output: core: 4 byte read to unmapped address 0x0 at *\n

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

	ldi r1, 0
	lbbo &r2, r1, 0, 4

	pass
