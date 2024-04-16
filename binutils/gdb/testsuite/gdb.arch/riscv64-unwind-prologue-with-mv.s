/* Copyright 2021-2024 Free Software Foundation, Inc.

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

/* This testcase contains a function where the 'c.mv' instruction is used in
   the prologue.

   The following functions are roughly equivalent to the following C code (with
   prologue crafted to contain the c.mv instruction):

     int bar () { return 0; }
     int foo () { return bar (); } */

	.option pic
	.text
	.align	1
	.globl	bar
	.type	bar, @function
bar:
	li	a0,0
	jr	ra
	.size	bar, .-bar

	.align	1
	.globl	foo
	.type	foo, @function
foo:
	addi	sp,sp,-32
	c.mv	t3,ra
	sd	t3,8(sp)
	call	bar
	ld	t3,8(sp)
	mv	ra,t3
	addi	sp,sp,32
	jr	ra
	.size	foo, .-foo
	.section	.note.GNU-stack,"",@progbits
