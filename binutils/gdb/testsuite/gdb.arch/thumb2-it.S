/* Thumb-2 IT blocks test program.

   Copyright 2010-2024 Free Software Foundation, Inc.

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

	.syntax unified
	.text
	.p2align 2
	.code 16

#ifndef __thumb2__

	.type main,%function
	.thumb_func
	.globl main
main:
	mov	r0, #0
	bx	lr	@ No Thumb-2

#else

	.type main,%function
	.thumb_func
	.globl main
main:
	mov	r0, #0
	bx	lr	@ Thumb-2 OK

	@ One conditional instruction, executed.
	.type it_1,%function
	.thumb_func
it_1:
	mov	r0, #0	@ Setup
	cmp	r0, #0	@ Setup
	it	eq	@ IT instruction, Expected == 1
	addeq	r0, #1	@ Reached
	bx	lr	@ Done

	@ One conditional instruction, skipped.
	.type it_2,%function
	.thumb_func
it_2:
	mov	r0, #0	@ Setup
	cmp	r0, #0	@ Setup
	it	ne	@ IT instruction, Expected == 0
	addne	r0, #1	@ Not reached
	bx	lr	@ Done, Check $r0 == 0

	@ Block of four, alternating, starting with executed.
	.type it_3,%function
	.thumb_func
it_3:
	mov	r0, #0	@ Setup
	cmp	r0, #0	@ Setup
	itete	ge	@ IT instruction, Expected == 2
	addge	r0, #1	@ Reached
	addlt	r0, #2	@ Not reached
	addge	r0, #4	@ Reached
	addlt	r0, #8	@ Not reached
	bx	lr	@ Done, Check $r0 == 5

	@ Block of four, changing flags.
	.type it_4,%function
	.thumb_func
it_4:
	mov	r0, #0	@ Setup
	cmp	r0, #0	@ Setup
	itttt	ge	@ IT instruction, Expected == 2
	addge	r0, #1	@ Reached
	cmpge	r0, #10	@ Reached
	addge	r0, #4	@ Not reached
	addge	r0, #8	@ Not reached
	bx	lr	@ Done, Check $r0 == 1

	@ Block of two, ending with taken branch.
	.type it_5,%function
	.thumb_func
it_5:
	mov	r0, #0	@ Setup
	cmp	r0, #0	@ Setup
	itt	ge	@ IT instruction, Expected == 2
	addge	r0, #1	@ Reached
	bge	.L5	@ Reached
	add	r0, #2	@ Never reached
.L5:	bx	lr	@ Done, Check $r0 == 1

	@ Block of two, ending with untaken branch.
	.type it_6,%function
	.thumb_func
it_6:
	mov	r0, #0	@ Setup
	cmp	r0, #0	@ Setup
	ite	ge	@ IT instruction, Expected == 2
	addge	r0, #1	@ Reached
	blt	.L6	@ Not reached
	add	r0, #2	@ Reached
.L6:	bx	lr	@ Done, Check $r0 == 3

	@ Block of four, taken, of different sizes
	.type it_7,%function
	.thumb_func
it_7:
	mov	r0, #0	@ Setup
	cmp	r0, #0	@ Setup
	itttt	ge	@ IT instruction, Expected == 4
	addge.n	r0, #1	@ Reached
	addge.w	r0, #2	@ Reached
	addge.n	r0, #4	@ Reached
	addge.w	r0, #8	@ Reached
	bx	lr	@ Done, Check $r0 == 15

	@ Block of four, only first executed.
	.type it_8,%function
	.thumb_func
it_8:
	mov	r0, #0	@ Setup
	cmp	r0, #0	@ Setup
	iteee	ge	@ IT instruction, Expected == 1
	addge	r0, #1	@ Reached
	addlt	r0, #2	@ Not reached
	addlt	r0, #4	@ Not reached
	addlt	r0, #8	@ Not reached
	bx	lr	@ Done, Check $r0 == 1

	.type it_breakpoints,%function
	.thumb_func
it_breakpoints:
	mov	r0, #0
	cmp	r0, #0
	it	eq	@ Location 1 @ Break 1
	moveq	r0, #0

	it	eq	@ Location 2
	moveq	r0, #0	@ Break 2

	it	ne	@ Location 3
	movne	r0, #0	@ Break 3

	@ An IT block of maximum size.
	itttt	eq	@ Location 4
	moveq.w	r0, #0
	moveq.w	r0, #0
	moveq.w	r0, #0
	moveq.w	r0, #0	@ Break 4

	@ Just outside an IT block.
	it	eq
	moveq	r0, #0
	mov	r0, #0	@ Location 5 @ Break 5

	@ After something that looks like an IT block, but
	@ is the second half of an instruction.
	.p2align 6
	cmp	r0, r0
	b	1f
	b.w	.+0xe14	@ 0xf000 0xbf08 -> second half is IT EQ
1:	mov	r0, #0	@ Location 6 @ Break 6

	@ After something that looks like an IT block, but
	@ is data.
	.p2align 6
	b	1f
	.short	0xbf08
1:	mov	r0, #0	@ Location 7 @ Break 7

	bx	lr

#endif /* __thumb2__ */
	.section	.note.GNU-stack,"",%progbits
