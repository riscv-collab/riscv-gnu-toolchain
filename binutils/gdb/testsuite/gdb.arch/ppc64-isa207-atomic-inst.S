/* This file is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

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
	.globl test_atomic_sequences
#if _CALL_ELF == 2
	.type test_atomic_sequences,@function
test_atomic_sequences:
#else
	.section ".opd","aw"
	.align 3
test_atomic_sequences:
	.quad .test_atomic_sequences,.TOC.@tocbase,0
	.size test_atomic_sequences,.-test_atomic_sequences
	.previous
	.globl .test_atomic_sequences
	.type .test_atomic_sequences,@function
.test_atomic_sequences:
#endif

	li	0,0
	addi	4,1,-8

	stb	0,0(4)
1:	lbarx	5,0,4
	cmpdi	5,0
	bne	2f
	addi	5,5,1
	stbcx.	5,0,4
	bne	1b

	sth	0,0(4)
2:	lharx	5,0,4
	cmpdi	5,0
	bne	3f
	addi	5,5,1
	sthcx.	5,0,4
	bne	2b

#ifdef	__BIG_ENDIAN__
	li 10,0
	li 6,0
	li 7,1
	std 10,-16(1)
	li 10,1
	std 10,-8(1)
	addi 4,1,-16
#else
	std 9,40(1)
	li 9,1
	addi 4,1,32
	std 9,32(1)
	mr 8,9
	ld 3,8(4)
#endif
3:	lqarx 10,0,4
#ifdef	__BIG_ENDIAN__
	li 8,0
	li 9,2
	mr 5,10
	xor 10,11,7
	xor 5,5,6
	or. 4,5,10
	bne 4f
	addi 10,1,-16
	stqcx. 8,0,10
#else
	xor 9,11,8
	mr 6,11
	xor 11,10,3
	or. 0,9,11
	bne 4f
	li 14,0
	li 15,2
	stqcx. 14,0,4
#endif
	bne 3b

4:	li	3,0
	blr

#if _CALL_ELF == 2
	.size test_atomic_sequences,.-test_atomic_sequences
#else
	.size .test_atomic_sequences,.-.test_atomic_sequences
#endif
	.section	.note.GNU-stack,"",@progbits
