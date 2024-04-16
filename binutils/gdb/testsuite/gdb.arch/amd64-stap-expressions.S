/* Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

	.file	"amd64-stap-expressions.S"
	.text
	.globl	main
main:
	/* We use a nop here because we don't want the first probe to
	   be placed at the same location as the main label.  */
	nop

	/* Single operands.  */
	STAP_PROBE1(probe, log_neg, 8@!($0+$1))
	STAP_PROBE1(probe, minus, -8@-($3+$4))
	STAP_PROBE1(probe, bit_neg, -8@~$22)

	/* Arithmetic expressions.  */
	STAP_PROBE1(probe, plus1, 8@$3+($10-$8)-$1)
	STAP_PROBE1(probe, plus2, 8@$100-(  ($8+$10) -$50)+$3)
	STAP_PROBE1(probe, plus3, 8@$100-(($8+$10)-$50)+((($8 - $9) + $40) - $4)+$4)

	/* Bitwise expressions.  */
	STAP_PROBE1(probe, and, 8@$128&$128)
	STAP_PROBE1(probe, or, 8@$8|$4)

	xor	%rax,%rax
	ret
	.section	.note.GNU-stack,"",@progbits
