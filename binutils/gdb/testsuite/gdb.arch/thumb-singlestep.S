/* Test program with deliberately incorrect execution mode transition

   Copyright 2011-2024 Free Software Foundation, Inc.

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

	.text
	.align 2
	.global foo
	.thumb
	/* .thumb_func deliberately omitted */
foo:
	mov r0,#42
	bx lr

        .text
	.align  2
	.global main
	.thumb
	.thumb_func
	.type	main, %function
main:
        push	{r3, lr}
	blx	foo
        pop	{r3, pc}
	.size	main, .-main

	.section	.note.GNU-stack,"",%progbits
