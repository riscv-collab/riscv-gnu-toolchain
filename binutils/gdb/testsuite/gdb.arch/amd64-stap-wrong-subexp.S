/* Copyright (C) 2014-2024 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <sys/sdt.h>

	.file	"amd64-stap-wrong-subexp.S"
	.text
	.globl	main
main:
	STAP_PROBE1(probe, foo, -4@$-3($4+$3))
	STAP_PROBE2(probe, bar, -4@-($4), -4@$-3+($22/$2)-$16)
	xor	%rax,%rax
	ret
	.section	.note.GNU-stack,"",@progbits
