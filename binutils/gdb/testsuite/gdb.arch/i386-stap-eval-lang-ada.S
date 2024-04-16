/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2024 Free Software Foundation, Inc.

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

/* This file has been generated using the following command:

     gcc -m32 -S i386-stap-eval-lang-ada.c -o i386-stap-eval-lang-ada.S

   The important thing about this is that the probe argument (below)
   should be exactly "-4@-4(%ebp)" in order to trigger the bug.  */

	.file	"i386-stap-eval-lang-ada.c"
	.text
	.globl	main
	.type	main, @function
main:
.LFB0:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$16, %esp
	movl	$40, -4(%ebp)
#APP
# 27 "i386-stap-eval-lang-ada.c" 1
	990: nop
.pushsection .note.stapsdt,"?","note"
.balign 4
.4byte 992f-991f,994f-993f,3
991: .asciz "stapsdt"
992: .balign 4
993: .4byte 990b
.4byte _.stapsdt.base
.4byte 0
.asciz "foo"
.asciz "bar"
/* The following probe argument should be "-4@-4(%ebp)", or some other
   register displacement expression that references %ebp, otherwise the
   bug will not trigger.  */
.asciz "-4@-4(%ebp)"
994: .balign 4
.popsection

# 0 "" 2
# 27 "i386-stap-eval-lang-ada.c" 1
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
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
.LFE0:
	.size	main, .-main
	.ident	"GCC: (GNU) 4.8.3 20140911 (Red Hat 4.8.3-7)"
	.section	.note.GNU-stack,"",@progbits
