# Test for mf{hi,lo} -> mult/div/mt{hi,lo} with 2 nops inbetween.
#
# mach:		all
# as:		-mabi=eabi -mmicromips
# ld:		-N -Ttext=0x80010000
# output:	pass\\n

# Copyright (C) 2013-2024 Free Software Foundation, Inc.
# Contributed by Andrew Bennett (andrew.bennett@imgtec.com)
#
# This file is part of the MIPS sim.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, see <http://www.gnu.org/licenses/>.

	.include "hilo-hazard.inc"
	.include "testutils.inc"

	setup

	.set noreorder
	.ent DIAG
DIAG:
	hilo
	pass
	.end DIAG
