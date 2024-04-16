/* Copyright 2019-2024 Free Software Foundation, Inc.

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

   Test that a breakpoint placed by line number in an assembler file
   will stop at the specified line.  Previously versions of GDB have
   incorrectly invoked the prologue analysis logic and skipped
   forward.  */

	.text
	.global main
main:
	nop
test:
	/* The next two instructions are required to look like an
	   x86-64 prologue so that GDB's prologue scanner will spot
	   them and skip forward.  */
	push    %rbp		/* Break here.  */
	mov	%rsp, %rbp
	nop			/* Incorrect.  */
	nop
	nop
	.section	.note.GNU-stack,"",@progbits
