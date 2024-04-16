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

/* The difference between this and arm-pseudo-unwind is that here, the CFI
   directives use the obsolete DWARF number for the s16 register (a
   pseudo-register in GDB), whereas arm-pseudo-unwind uses the number for the d8
   register (the underlying raw register for s16).  */

.section .note.GNU-stack,"",%progbits

.data
value_callee:
.quad 0x20212223
value_caller:
.quad 0x10111213

.text
.arm
.global callee
callee:
.cfi_startproc
	/* Standard prologue.  */
	push {fp, lr}
.cfi_def_cfa fp, 4
.cfi_offset fp, -8
.cfi_offset lr, -4
	add fp, sp, #4

	/* Save caller's s16 value on the stack.  */
.cfi_offset 80, -12
	vpush {s16}

	/* Put our own s16 value.  */
	ldr r0, =value_callee
	vldr s16, [r0]
break_here_asm:

	/* Restore caller's s16 value.  */
	vpop {s16}

	/* Standard epilogue.  */
	pop {fp, pc}
.cfi_endproc


.global caller
caller:
.cfi_startproc
	/* Standard prologue.  */
	push {fp, lr}
.cfi_def_cfa fp, 4
.cfi_offset fp, -8
.cfi_offset lr, -4
	add fp, sp, #4

	/* Put our own s16 value.  */
	ldr r0, =value_caller
	vldr s16, [r0]

	/* Call callee.  */
	bl callee

	/* Store our s16 value in r0-r1 to return it.  */
	vpush {s16}
	pop {r0}
	mov r1, #0

	/* Standard epilogue.  */
	pop {fp, pc}
.cfi_endproc
