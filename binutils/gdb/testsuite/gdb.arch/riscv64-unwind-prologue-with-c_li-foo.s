/* Copyright 2023-2024 Free Software Foundation, Inc.

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


/* Simple asm function that makes use of c.li and c.lui in the
   function prologue before the return address and frame pointers are
   written the stack.  This ensures that GDB's prologue unwinder can
   understand these instructions.  */

	.option pic
	.text

	.globl	foo
	.type	foo, @function
foo:
	addi	sp,sp,-32
	sd	s1,8(sp)

	c.li	s1,-4
	c.lui	s1,4

	sd	fp,24(sp)
	sd	ra,16(sp)
	addi	fp,sp,32

	call	bar@plt

	ld	s1,8(sp)
	ld	ra,16(sp)
	ld	fp,24(sp)
	addi	sp,sp,32
	jr	ra

	.size	foo, .-foo
	.section	.note.GNU-stack,"",@progbits
