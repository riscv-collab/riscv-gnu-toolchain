/* Copyright 2018-2024 Free Software Foundation, Inc.

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

.section .note.GNU-stack,"",%progbits

.text
.global callee
callee:
	/* Standard prologue.  */
.cfi_startproc
	push %rbp
.cfi_def_cfa rbp, 16
	mov %rsp, %rbp

	/* Save caller's rbx value on the stack.  */
.cfi_offset rbx, -24
	push %rbx

	/* Put our own rbx value.  */
	mov $0x2021222324252627, %rbx
break_here_asm:

	/* Restore caller's rbx value.  */
	pop %rbx

	/* Standard epilogue.  */
	pop %rbp
	ret
.cfi_endproc


.global caller
caller:
.cfi_startproc
	/* Standard prologue.  */
	push %rbp
.cfi_def_cfa_offset 16
	mov %rsp, %rbp

	/* Put our own rbx value.  */
	mov $0x1011121314151617, %rbx

	/* Call callee.  */
	call callee

	/* Store our rbx value in rax to return it.  */
	mov %rbx, %rax

	/* Standard epilogue.  */
	pop %rbp
	ret
.cfi_endproc
