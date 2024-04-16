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

.data
value_callee:
.quad 0x2021222324252627
value_caller:
.quad 0x1011121314151617

.text
.arm
.global callee
callee:
	/* Standard prologue.  */
.cfi_startproc
	push {fp, lr}
.cfi_def_cfa fp, 4
.cfi_offset fp, -8
.cfi_offset lr, -4
	add fp, sp, #4

	/* Save caller's d8 value on the stack.  */
.cfi_offset d8, -16
	vpush {d8}

	/* Put our own d8 value.  */
	ldr r0, =value_callee
	vldr d8, [r0]
break_here_asm:

	/* Restore caller's d8 value.  */
	vpop {d8}

	/* Standard epilogue.  */
	pop {fp, pc}
.cfi_endproc


.global caller
caller:
	/* Standard prologue.  */
.cfi_startproc
	push {fp, lr}
.cfi_def_cfa fp, 4
.cfi_offset fp, -8
.cfi_offset lr, -4
	add fp, sp, #4

	/* Put our own d8 value.  */
	ldr r0, =value_caller
	vldr d8, [r0]

	/* Call callee.  */
	bl callee

	/* Store our d8 value in r0-r1 to return it.  */
	vpush {d8}
	pop {r0}
	pop {r1}

	/* Standard epilogue.  */
	pop {fp, pc}
.cfi_endproc
