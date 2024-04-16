/* Copyright (C) 2013-2024 Free Software Foundation, Inc.

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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   This file was generated from the equivalent .c file using the
   following command:

     #> gcc -S amd64-stap-triplet.c -o amd64-stap-triplet.S

   Then, the SystemTap SDT probe definition below was tweaked.  See below
   for more details.  */

	.file	"amd64-stap-triplet.c"
	.text
	.globl	main
	.type	main, @function
main:
.LFB0:
	.cfi_startproc
# BLOCK 2 seq:0
# PRED: ENTRY (fallthru)
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, -20(%rbp)
	movq	%rsi, -32(%rbp)
	movl	$10, -4(%rbp)
#APP
# 8 "amd64-stap-triplet.c" 1
	990: nop
.pushsection .note.stapsdt,"?","note"
.balign 4
.4byte 992f-991f,994f-993f,3
991: .asciz "stapsdt"
992: .balign 4
993: .8byte 990b
.8byte _.stapsdt.base
.8byte 0
.asciz "test"
.asciz "triplet"
/* The probe's argument definition below was tweaked in order to mimic a
   triplet displacement in x86 asm.  The original probe argument was:

     -4@-4(%rbp)

   The argument below is equivalent to that.  */
.asciz "-4@-4+16-16(%rbp)"
994: .balign 4
.popsection

# 0 "" 2
# 8 "amd64-stap-triplet.c" 1
	.ifndef _.stapsdt.base
.pushsection .stapsdt.base,"aG","progbits",.stapsdt.base,comdat
.weak _.stapsdt.base
.hidden _.stapsdt.base
_.stapsdt.base: .space 1
.size _.stapsdt.base,1
.popsection
.endif

# 0 "" 2
#NO_APP
	movl	$0, %eax
	popq	%rbp
	.cfi_def_cfa 7, 8
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE0:
	.size	main, .-main
	.ident	"GCC: (GNU) 4.7.2 20120921 (Red Hat 4.7.2-2)"
	.section	.note.GNU-stack,"",@progbits
