/* This file is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

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

/* This file was generated from ppc64-break-on-_exit.c.  */

	.file	"ppc64-break-on-_exit.c"
	.abiversion 2
	.section	".text"
	.align 2
	.p2align 4,,15
	.globl _exit
	.type	_exit, @function
_exit:
.LCF0:
0:	addis 2,12,.TOC.-.LCF0@ha
	addi 2,2,.TOC.-.LCF0@l
	.localentry	_exit,.-_exit
	addis 9,2,__libc_errno@got@tprel@ha
	std 31,-8(1)
	mr 31,3
	std 30,-16(1)
	li 0,234
	ld 9,__libc_errno@got@tprel@l(9)
	mr 3,31
	add 30,9,__libc_errno@tls
#APP
 # 28 "src/gdb/testsuite/gdb.arch/ppc64-break-on-_exit.c" 1
	sc
	mfcr  0
	0:
 # 0 "" 2
#NO_APP
	andis. 9,0,0x1000
	mr 9,3
	li 0,1
	mr 3,31
	bne 0,.L13
	.p2align 4,,15
.L2:
#APP
 # 67 "src/gdb/testsuite/gdb.arch/ppc64-break-on-_exit.c" 1
	sc
	mfcr  0
	0:
 # 0 "" 2
#NO_APP
	andis. 9,0,0x1000
	bne 0,.L14
.L3:
#APP
 # 87 "src/gdb/testsuite/gdb.arch/ppc64-break-on-_exit.c" 1
	.long 0
 # 0 "" 2
#NO_APP
.L15:
	li 0,234
	mr 3,31
#APP
 # 28 "src/gdb/testsuite/gdb.arch/ppc64-break-on-_exit.c" 1
	sc
	mfcr  0
	0:
 # 0 "" 2
#NO_APP
	andis. 9,0,0x1000
	mr 9,3
	li 0,1
	mr 3,31
	beq 0,.L2
.L13:
	stw 9,0(30)
#APP
 # 67 "src/gdb/testsuite/gdb.arch/ppc64-break-on-_exit.c" 1
	sc
	mfcr  0
	0:
 # 0 "" 2
#NO_APP
	andis. 9,0,0x1000
	beq 0,.L3
	.p2align 4,,15
.L14:
	stw 3,0(30)
#APP
 # 87 "src/gdb/testsuite/gdb.arch/ppc64-break-on-_exit.c" 1
	.long 0
 # 0 "" 2
#NO_APP
	b .L15
	.long 0
	.byte 0,0,0,0,0,2,0,0
	.size	_exit,.-_exit
	.ident	"GCC: (SUSE Linux) 7.5.0"
	.section	.note.GNU-stack,"",@progbits
