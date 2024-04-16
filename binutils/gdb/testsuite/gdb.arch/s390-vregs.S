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

	/* 'check_vx': Yield SIGILL unless vector support is
	available.  Have a "pit stop" breakpoint here.  */

	.align	8
	.type	check_vx, @function
check_vx:
	.cfi_startproc
	/* vlr  %v0,%v0 */
	.byte 0xe7,0x00,0x00,0x00,0x00,0x56
	br	%r14
	.cfi_endproc
	.size	check_vx, .-check_vx


	/* 'store_vrs': Store vector registers in save_area.  */

	.align 8
	.type	store_vrs, @function
store_vrs:
	.cfi_startproc
	larl	%r1,save_area
	/* vstm %v0,%v15,0(%r1) */
	.byte 0xe7,0x0f,0x10,0x00,0x00,0x3e
	/* vstm %v16,%v31,256(%r1) */
	.byte 0xe7,0x0f,0x11,0x00,0x0c,0x3e
	br	%r14
	.cfi_endproc
	.size	store_vrs, .-store_vrs


	/* 'change_vrs': Manipulate vector registers according to a
	simple algorithm.  */

	.align 8
	.type	change_vrs, @function
change_vrs:
	.cfi_startproc
	lghi	%r1,16
	lghi	%r3,0xff
1:	.insn	ril,0xc60000000000,%r3,2f /* exrl */
	.insn	ril,0xc60000000000,%r3,1f /* exrl */
	aghi	%r3,-0x11
	brctg	%r1,1b
	br	%r14
	.cfi_endproc
	/* vmlf %v0,%v0,%v0 */
1:	.byte	0xe7,0x00,0x00,0x00,0x20,0xa2
	/* vmlf %v16,%v16,%v0 */
2:	.byte	0xe7,0x00,0x00,0x00,0x2c,0xa2


	/* 'main': Perform actions according to test case logic.
	Invoke check_vx whenever a pit stop is required.  */

	.section	.text.startup,"ax",@progbits
	.align	8
.globl main
	.type	main, @function
main:
	.cfi_startproc
	stmg	%r14,%r15,112(%r15)
	aghi	%r15,-160
	bras	%r14,check_vx
	bras	%r14,store_vrs
	bras	%r14,check_vx
	bras	%r14,change_vrs
	bras	%r14,check_vx
	lmg	%r14,%r15,272(%r15)
	lghi	%r2,0
	br	%r14
	.cfi_endproc
	.size	main, .-main

	.local	save_area
	.comm	save_area,512,16

	.section	.note.GNU-stack,"",@progbits
