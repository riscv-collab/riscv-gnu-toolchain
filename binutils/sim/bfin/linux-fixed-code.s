/* Linux fixed code userspace ABI

   Copyright (C) 2005-2024 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc.

   This file is part of simulators.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* For more info, see this page:
   http://docs.blackfin.uclinux.org/doku.php?id=linux-kernel:fixed-code  */

.text

.align 16
_sigreturn_stub:
	P0 = 173;
	EXCPT 0;
0:	JUMP.S 0b;

.align 16
_atomic_xchg32:
	R0 = [P0];
	[P0] = R1;
	rts;

.align 16
_atomic_cas32:
	R0 = [P0];
	CC = R0 == R1;
	IF !CC JUMP 1f;
	[P0] = R2;
1:
	rts;

.align 16
_atomic_add32:
	R1 = [P0];
	R0 = R1 + R0;
	[P0] = R0;
	rts;

.align 16
_atomic_sub32:
	R1 = [P0];
	R0 = R1 - R0;
	[P0] = R0;
	rts;

.align 16
_atomic_ior32:
	R1 = [P0];
	R0 = R1 | R0;
	[P0] = R0;
	rts;

.align 16
_atomic_and32:
	R1 = [P0];
	R0 = R1 & R0;
	[P0] = R0;
	rts;

.align 16
_atomic_xor32:
	R1 = [P0];
	R0 = R1 ^ R0;
	[P0] = R0;
	rts;

.align 16
_safe_user_instruction:
	NOP; NOP; NOP; NOP;
	EXCPT 0x4;
