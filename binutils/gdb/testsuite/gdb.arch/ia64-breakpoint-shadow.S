/* Copyright 2009-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   This file is part of the gdb testsuite.
   It tests displaced stepping over various insns that require special
   handling.  */

	.text
	.align 16
	.global main
	.proc main
main:
	mov r2=r12
	mov r14=1
	;;
bundle:
	/* Store value 1 into `long' variable on stack.  */
	st8.rel [r2]=r14
	/* This long constant requires L-X slot in this bundle.  */
	movl r14=0x7fffffff
	;;
	/* Store value 0x7fffffff into `long' variable on stack.  */
	st8.rel [r2]=r14
	mov r14=r0
	;;
	mov r8=r14
	mov r12=r2
	br.ret.sptk.many b0

	.endp main

	.section	.note.GNU-stack,"",@progbits
