/* This file is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

	.align 2
	.globl main
#if _CALL_ELF == 2
	.type main,@function
main:
#else
	.section ".opd","aw"
	.align 3
main:
	.quad .main,.TOC.@tocbase,0
	.size main,.-main
	.previous
	.globl .main
	.type .main,@function
.main:
#endif

	li	0,0
	addi	4,1,-8

	stw	0,0(4)
1:	lwarx	5,0,4
	cmpwi	5,0
	bne	2f
	addi	5,5,1
	stwcx.	5,0,4
	bne	1b

	std	0,0(4)
2:	ldarx	5,0,4
	cmpdi	5,0
	bne	3f
	addi	5,5,1
	stdcx.	5,0,4
	bne	1b

3:	li	3,0
	blr

#if _CALL_ELF == 2
	.size main,.-main
#else
	.size .main,.-.main
#endif
	.section	.note.GNU-stack,"",@progbits
