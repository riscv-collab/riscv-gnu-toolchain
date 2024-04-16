/* Copyright 2015-2024 Free Software Foundation, Inc.

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

	.align	8
	.globl  main
	.type	main, @function
	/* Ensure an invalid stack pointer and link register.  Don't
	use zero, since that may be handled specially.  */
main:	la	%r15,3(0,0)
	la	%r14,1(0,0)
	j	1f
	.size	main, .-main

	/* Outside any function.  */
	.byte	0,1
1:	st	%r15,0(%r15)
	br	%r14
	.section	.note.GNU-stack,"",@progbits
