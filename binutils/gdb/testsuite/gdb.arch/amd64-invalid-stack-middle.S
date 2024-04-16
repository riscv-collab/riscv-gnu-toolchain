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

/* This file is compiled from gdb.arch/amd64-invalid-stack-middle.c
   using: 'gcc -g -O0 -S -dA' and gcc version '4.7.2'.
   Changes were then made to the CFI entry for func2.  */
        
	.file	"amd64-invalid-stack-middle.c"
	.text
.Ltext0:
	.globl	breakpt
	.type	breakpt, @function
breakpt:
.LFB0:
	.file 1 "amd64-invalid-stack-middle.c"
	# amd64-invalid-stack-middle.c:25
	.loc 1 25 0
# BLOCK 2 seq:0
# PRED: ENTRY (fallthru)
	pushq	%rbp
.LCFI0:
	movq	%rsp, %rbp
.LCFI1:
	# amd64-invalid-stack-middle.c:27
	.loc 1 27 0
	popq	%rbp
.LCFI2:
# SUCC: EXIT [100.0%] 
	ret
.LFE0:
	.size	breakpt, .-breakpt
	.globl	func5
	.type	func5, @function
func5:
.LFB1:
	# amd64-invalid-stack-middle.c:31
	.loc 1 31 0
# BLOCK 2 seq:0
# PRED: ENTRY (fallthru)
	pushq	%rbp
.LCFI3:
	movq	%rsp, %rbp
.LCFI4:
	# amd64-invalid-stack-middle.c:32
	.loc 1 32 0
	movl	$0, %eax
	call	breakpt
	# amd64-invalid-stack-middle.c:33
	.loc 1 33 0
	popq	%rbp
.LCFI5:
# SUCC: EXIT [100.0%] 
	ret
.LFE1:
	.size	func5, .-func5
	.globl	func4
	.type	func4, @function
func4:
.LFB2:
	# amd64-invalid-stack-middle.c:37
	.loc 1 37 0
# BLOCK 2 seq:0
# PRED: ENTRY (fallthru)
	pushq	%rbp
.LCFI6:
	movq	%rsp, %rbp
.LCFI7:
	# amd64-invalid-stack-middle.c:38
	.loc 1 38 0
	movl	$0, %eax
	call	func5
	# amd64-invalid-stack-middle.c:39
	.loc 1 39 0
	popq	%rbp
.LCFI8:
# SUCC: EXIT [100.0%] 
	ret
.LFE2:
	.size	func4, .-func4
	.globl	func3
	.type	func3, @function
func3:
.LFB3:
	# amd64-invalid-stack-middle.c:43
	.loc 1 43 0
# BLOCK 2 seq:0
# PRED: ENTRY (fallthru)
	pushq	%rbp
.LCFI9:
	movq	%rsp, %rbp
.LCFI10:
	# amd64-invalid-stack-middle.c:44
	.loc 1 44 0
	movl	$0, %eax
	call	func4
	# amd64-invalid-stack-middle.c:45
	.loc 1 45 0
	popq	%rbp
.LCFI11:
# SUCC: EXIT [100.0%] 
	ret
.LFE3:
	.size	func3, .-func3
	.globl	func2
	.type	func2, @function
func2:
.LFB4:
	# amd64-invalid-stack-middle.c:49
	.loc 1 49 0
# BLOCK 2 seq:0
# PRED: ENTRY (fallthru)
	pushq	%rbp
.LCFI12:
	movq	%rsp, %rbp
.LCFI13:
	subq	$8, %rsp
	movq	%rdi, -8(%rbp)
	# amd64-invalid-stack-middle.c:50
	.loc 1 50 0
	movl	$0, %eax
	call	func3
	# amd64-invalid-stack-middle.c:51
	.loc 1 51 0
	leave
.LCFI14:
# SUCC: EXIT [100.0%] 
	ret
.LFE4:
	.size	func2, .-func2
	.globl	func1
	.type	func1, @function
func1:
.LFB5:
	# amd64-invalid-stack-middle.c:55
	.loc 1 55 0
# BLOCK 2 seq:0
# PRED: ENTRY (fallthru)
	pushq	%rbp
.LCFI15:
	movq	%rsp, %rbp
.LCFI16:
	subq	$8, %rsp
	movq	%rdi, -8(%rbp)
	# amd64-invalid-stack-middle.c:56
	.loc 1 56 0
	movq	-8(%rbp), %rax
	movq	%rax, %rdi
	call	func2
	# amd64-invalid-stack-middle.c:57
	.loc 1 57 0
	leave
.LCFI17:
# SUCC: EXIT [100.0%] 
	ret
.LFE5:
	.size	func1, .-func1
	.section	.rodata
.LC0:
	.string	"amd64-invalid-stack-middle.c"
.LC1:
	.string	"ptr != ((void *) -1)"
.LC2:
	.string	"ans == 0"
	.text
	.type	make_invalid_ptr, @function
make_invalid_ptr:
.LFB6:
	# amd64-invalid-stack-middle.c:65
	.loc 1 65 0
# BLOCK 2 seq:0
# PRED: ENTRY (fallthru)
	pushq	%rbp
.LCFI18:
	movq	%rsp, %rbp
.LCFI19:
	subq	$32, %rsp
	# amd64-invalid-stack-middle.c:69
	.loc 1 69 0
	call	getpagesize
	movl	%eax, -4(%rbp)
	# amd64-invalid-stack-middle.c:70
	.loc 1 70 0
	movl	-4(%rbp), %eax
	cltq
	movl	$0, %r9d
	movl	$-1, %r8d
	movl	$34, %ecx
	movl	$0, %edx
	movq	%rax, %rsi
	movl	$0, %edi
	call	mmap
	movq	%rax, -16(%rbp)
	# amd64-invalid-stack-middle.c:73
	.loc 1 73 0
	cmpq	$-1, -16(%rbp)
# SUCC: 3 (fallthru) 4
	jne	.L8
# BLOCK 3 seq:1
# PRED: 2 (fallthru)
	movl	$__PRETTY_FUNCTION__.2362, %ecx
	movl	$73, %edx
	movl	$.LC0, %esi
	movl	$.LC1, %edi
# SUCC:
	call	__assert_fail
# BLOCK 4 seq:2
# PRED: 2
.L8:
	# amd64-invalid-stack-middle.c:74
	.loc 1 74 0
	movl	-4(%rbp), %eax
	movslq	%eax, %rdx
	movq	-16(%rbp), %rax
	movq	%rdx, %rsi
	movq	%rax, %rdi
	call	munmap
	movl	%eax, -20(%rbp)
	# amd64-invalid-stack-middle.c:75
	.loc 1 75 0
	cmpl	$0, -20(%rbp)
# SUCC: 5 (fallthru) 6
	je	.L9
# BLOCK 5 seq:3
# PRED: 4 (fallthru)
	movl	$__PRETTY_FUNCTION__.2362, %ecx
	movl	$75, %edx
	movl	$.LC0, %esi
	movl	$.LC2, %edi
# SUCC:
	call	__assert_fail
# BLOCK 6 seq:4
# PRED: 4
.L9:
	# amd64-invalid-stack-middle.c:77
	.loc 1 77 0
	movq	-16(%rbp), %rax
	# amd64-invalid-stack-middle.c:78
	.loc 1 78 0
	leave
.LCFI20:
# SUCC: EXIT [100.0%] 
	ret
.LFE6:
	.size	make_invalid_ptr, .-make_invalid_ptr
	.globl	main
	.type	main, @function
main:
.LFB7:
	# amd64-invalid-stack-middle.c:82
	.loc 1 82 0
# BLOCK 2 seq:0
# PRED: ENTRY (fallthru)
	pushq	%rbp
.LCFI21:
	movq	%rsp, %rbp
.LCFI22:
	subq	$16, %rsp
	# amd64-invalid-stack-middle.c:85
	.loc 1 85 0
	call	make_invalid_ptr
	movq	%rax, -8(%rbp)
	# amd64-invalid-stack-middle.c:86
	.loc 1 86 0
	movq	-8(%rbp), %rax
	movq	%rax, %rdi
	call	func1
	# amd64-invalid-stack-middle.c:88
	.loc 1 88 0
	movl	$0, %eax
	# amd64-invalid-stack-middle.c:89
	.loc 1 89 0
	leave
.LCFI23:
# SUCC: EXIT [100.0%] 
	ret
.LFE7:
	.size	main, .-main
	.section	.rodata
	.align 16
	.type	__PRETTY_FUNCTION__.2362, @object
	.size	__PRETTY_FUNCTION__.2362, 17
__PRETTY_FUNCTION__.2362:
	.string	"make_invalid_ptr"
#APP
	.section	.debug_frame,"",@progbits
.Lframe0:
	.long	.LECIE0-.LSCIE0	# Length of Common Information Entry
.LSCIE0:
	.long	0xffffffff	# CIE Identifier Tag
	.byte	0x1	# CIE Version
	.ascii "\0"	# CIE Augmentation
	.uleb128 0x1	# CIE Code Alignment Factor
	.sleb128 -8	# CIE Data Alignment Factor
	.byte	0x10	# CIE RA Column
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.byte	0x90	# DW_CFA_offset, column 0x10
	.uleb128 0x1
	.align 8
.LECIE0:
.LSFDE0:
	.long	.LEFDE0-.LASFDE0	# FDE Length
.LASFDE0:
	.long	.Lframe0	# FDE CIE offset
	.quad	.LFB0	# FDE initial location
	.quad	.LFE0-.LFB0	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI0-.LFB0
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI1-.LCFI0
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI2-.LCFI1
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.align 8
.LEFDE0:
.LSFDE2:
	.long	.LEFDE2-.LASFDE2	# FDE Length
.LASFDE2:
	.long	.Lframe0	# FDE CIE offset
	.quad	.LFB1	# FDE initial location
	.quad	.LFE1-.LFB1	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI3-.LFB1
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI4-.LCFI3
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI5-.LCFI4
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.align 8
.LEFDE2:
.LSFDE4:
	.long	.LEFDE4-.LASFDE4	# FDE Length
.LASFDE4:
	.long	.Lframe0	# FDE CIE offset
	.quad	.LFB2	# FDE initial location
	.quad	.LFE2-.LFB2	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI6-.LFB2
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI7-.LCFI6
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI8-.LCFI7
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.align 8
.LEFDE4:
.LSFDE6:
	.long	.LEFDE6-.LASFDE6	# FDE Length
.LASFDE6:
	.long	.Lframe0	# FDE CIE offset
	.quad	.LFB3	# FDE initial location
	.quad	.LFE3-.LFB3	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI9-.LFB3
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI10-.LCFI9
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI11-.LCFI10
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.align 8
.LEFDE6:
.LSFDE8:
	.long	.LEFDE8-.LASFDE8	# FDE Length
.LASFDE8:
	.long	.Lframe0	# FDE CIE offset
	.quad	.LFB4		# FDE initial location
	.quad	.LFE4-.LFB4	# FDE address range
        .byte 0xf		# DW_CFA_def_cfa_expression
        .uleb128 .LEDWBLK1 - .LSDWBLK1
.LSDWBLK1:
        .byte 0x75              # DW_OP_breg5
        .sleb128 0x0            #        offset
        .byte 0x94              # DW_OP_dref_size
        .byte 0x8		#        size
.LEDWBLK1:
	.align 8		# Padding.
.LEFDE8:
.LSFDE10:
	.long	.LEFDE10-.LASFDE10	# FDE Length
.LASFDE10:
	.long	.Lframe0	# FDE CIE offset
	.quad	.LFB5	# FDE initial location
	.quad	.LFE5-.LFB5	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI15-.LFB5
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI16-.LCFI15
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI17-.LCFI16
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.align 8
.LEFDE10:
.LSFDE12:
	.long	.LEFDE12-.LASFDE12	# FDE Length
.LASFDE12:
	.long	.Lframe0	# FDE CIE offset
	.quad	.LFB6	# FDE initial location
	.quad	.LFE6-.LFB6	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI18-.LFB6
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI19-.LCFI18
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI20-.LCFI19
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.align 8
.LEFDE12:
.LSFDE14:
	.long	.LEFDE14-.LASFDE14	# FDE Length
.LASFDE14:
	.long	.Lframe0	# FDE CIE offset
	.quad	.LFB7	# FDE initial location
	.quad	.LFE7-.LFB7	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI21-.LFB7
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI22-.LCFI21
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI23-.LCFI22
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.align 8
.LEFDE14:
#NO_APP
#APP
	.section	.eh_frame,"a",@progbits
.Lframe1:
	.long	.LECIE1-.LSCIE1	# Length of Common Information Entry
.LSCIE1:
	.long	0	# CIE Identifier Tag
	.byte	0x1	# CIE Version
	.ascii "zR\0"	# CIE Augmentation
	.uleb128 0x1	# CIE Code Alignment Factor
	.sleb128 -8	# CIE Data Alignment Factor
	.byte	0x10	# CIE RA Column
	.uleb128 0x1	# Augmentation size
	.byte	0x3	# FDE Encoding (udata4)
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.byte	0x90	# DW_CFA_offset, column 0x10
	.uleb128 0x1
	.align 8
.LECIE1:
.LSFDE17:
	.long	.LEFDE17-.LASFDE17	# FDE Length
.LASFDE17:
	.long	.LASFDE17-.Lframe1	# FDE CIE offset
	.long	.LFB0	# FDE initial location
	.long	.LFE0-.LFB0	# FDE address range
	.uleb128 0	# Augmentation size
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI0-.LFB0
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI1-.LCFI0
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI2-.LCFI1
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.align 8
.LEFDE17:
.LSFDE19:
	.long	.LEFDE19-.LASFDE19	# FDE Length
.LASFDE19:
	.long	.LASFDE19-.Lframe1	# FDE CIE offset
	.long	.LFB1	# FDE initial location
	.long	.LFE1-.LFB1	# FDE address range
	.uleb128 0	# Augmentation size
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI3-.LFB1
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI4-.LCFI3
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI5-.LCFI4
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.align 8
.LEFDE19:
.LSFDE21:
	.long	.LEFDE21-.LASFDE21	# FDE Length
.LASFDE21:
	.long	.LASFDE21-.Lframe1	# FDE CIE offset
	.long	.LFB2	# FDE initial location
	.long	.LFE2-.LFB2	# FDE address range
	.uleb128 0	# Augmentation size
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI6-.LFB2
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI7-.LCFI6
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI8-.LCFI7
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.align 8
.LEFDE21:
.LSFDE23:
	.long	.LEFDE23-.LASFDE23	# FDE Length
.LASFDE23:
	.long	.LASFDE23-.Lframe1	# FDE CIE offset
	.long	.LFB3	# FDE initial location
	.long	.LFE3-.LFB3	# FDE address range
	.uleb128 0	# Augmentation size
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI9-.LFB3
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI10-.LCFI9
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI11-.LCFI10
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.align 8
.LEFDE23:
.LSFDE25:
	.long	.LEFDE25-.LASFDE25	# FDE Length
.LASFDE25:
	.long	.LASFDE25-.Lframe1	# FDE CIE offset
	.long	.LFB4	# FDE initial location
	.long	.LFE4-.LFB4	# FDE address range
	.uleb128 0	# Augmentation size
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI12-.LFB4
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI13-.LCFI12
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI14-.LCFI13
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.align 8
.LEFDE25:
.LSFDE27:
	.long	.LEFDE27-.LASFDE27	# FDE Length
.LASFDE27:
	.long	.LASFDE27-.Lframe1	# FDE CIE offset
	.long	.LFB5	# FDE initial location
	.long	.LFE5-.LFB5	# FDE address range
	.uleb128 0	# Augmentation size
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI15-.LFB5
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI16-.LCFI15
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI17-.LCFI16
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.align 8
.LEFDE27:
.LSFDE29:
	.long	.LEFDE29-.LASFDE29	# FDE Length
.LASFDE29:
	.long	.LASFDE29-.Lframe1	# FDE CIE offset
	.long	.LFB6	# FDE initial location
	.long	.LFE6-.LFB6	# FDE address range
	.uleb128 0	# Augmentation size
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI18-.LFB6
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI19-.LCFI18
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI20-.LCFI19
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.align 8
.LEFDE29:
.LSFDE31:
	.long	.LEFDE31-.LASFDE31	# FDE Length
.LASFDE31:
	.long	.LASFDE31-.Lframe1	# FDE CIE offset
	.long	.LFB7	# FDE initial location
	.long	.LFE7-.LFB7	# FDE address range
	.uleb128 0	# Augmentation size
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI21-.LFB7
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI22-.LCFI21
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI23-.LCFI22
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x7
	.uleb128 0x8
	.align 8
.LEFDE31:
#NO_APP
	.text
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0x1f1	# Length of Compilation Unit Info
	.value	0x2	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF17	# DW_AT_producer: "GNU C 4.7.2"
	.byte	0x1	# DW_AT_language
	.long	.LASF18	# DW_AT_name: "amd64-invalid-stack-middle.c"
	.long	.LASF19	# DW_AT_comp_dir: "/home/gdb/binutils-gdb/gdb/testsuite/gdb.arch"
	.quad	.Ltext0	# DW_AT_low_pc
	.quad	.Letext0	# DW_AT_high_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x2d) DW_TAG_base_type)
	.byte	0x8	# DW_AT_byte_size
	.byte	0x7	# DW_AT_encoding
	.long	.LASF0	# DW_AT_name: "long unsigned int"
	.uleb128 0x2	# (DIE (0x34) DW_TAG_base_type)
	.byte	0x1	# DW_AT_byte_size
	.byte	0x8	# DW_AT_encoding
	.long	.LASF1	# DW_AT_name: "unsigned char"
	.uleb128 0x2	# (DIE (0x3b) DW_TAG_base_type)
	.byte	0x2	# DW_AT_byte_size
	.byte	0x7	# DW_AT_encoding
	.long	.LASF2	# DW_AT_name: "short unsigned int"
	.uleb128 0x2	# (DIE (0x42) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x7	# DW_AT_encoding
	.long	.LASF3	# DW_AT_name: "unsigned int"
	.uleb128 0x2	# (DIE (0x49) DW_TAG_base_type)
	.byte	0x1	# DW_AT_byte_size
	.byte	0x6	# DW_AT_encoding
	.long	.LASF4	# DW_AT_name: "signed char"
	.uleb128 0x2	# (DIE (0x50) DW_TAG_base_type)
	.byte	0x2	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.long	.LASF5	# DW_AT_name: "short int"
	.uleb128 0x3	# (DIE (0x57) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x2	# (DIE (0x5e) DW_TAG_base_type)
	.byte	0x8	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.long	.LASF6	# DW_AT_name: "long int"
	.uleb128 0x2	# (DIE (0x65) DW_TAG_base_type)
	.byte	0x8	# DW_AT_byte_size
	.byte	0x7	# DW_AT_encoding
	.long	.LASF7	# DW_AT_name: "sizetype"
	.uleb128 0x4	# (DIE (0x6c) DW_TAG_pointer_type)
	.byte	0x8	# DW_AT_byte_size
	.uleb128 0x2	# (DIE (0x6e) DW_TAG_base_type)
	.byte	0x1	# DW_AT_byte_size
	.byte	0x6	# DW_AT_encoding
	.long	.LASF8	# DW_AT_name: "char"
	.uleb128 0x5	# (DIE (0x75) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF9	# DW_AT_name: "breakpt"
	.byte	0x1	# DW_AT_decl_file (amd64-invalid-stack-middle.c)
	.byte	0x18	# DW_AT_decl_line
	.quad	.LFB0	# DW_AT_low_pc
	.quad	.LFE0	# DW_AT_high_pc
	.long	.LLST0	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.uleb128 0x6	# (DIE (0x92) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF10	# DW_AT_name: "func5"
	.byte	0x1	# DW_AT_decl_file (amd64-invalid-stack-middle.c)
	.byte	0x1e	# DW_AT_decl_line
	.quad	.LFB1	# DW_AT_low_pc
	.quad	.LFE1	# DW_AT_high_pc
	.long	.LLST1	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_tail_call_sites
	.uleb128 0x6	# (DIE (0xaf) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF11	# DW_AT_name: "func4"
	.byte	0x1	# DW_AT_decl_file (amd64-invalid-stack-middle.c)
	.byte	0x24	# DW_AT_decl_line
	.quad	.LFB2	# DW_AT_low_pc
	.quad	.LFE2	# DW_AT_high_pc
	.long	.LLST2	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_tail_call_sites
	.uleb128 0x6	# (DIE (0xcc) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF12	# DW_AT_name: "func3"
	.byte	0x1	# DW_AT_decl_file (amd64-invalid-stack-middle.c)
	.byte	0x2a	# DW_AT_decl_line
	.quad	.LFB3	# DW_AT_low_pc
	.quad	.LFE3	# DW_AT_high_pc
	.long	.LLST3	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_tail_call_sites
	.uleb128 0x7	# (DIE (0xe9) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF13	# DW_AT_name: "func2"
	.byte	0x1	# DW_AT_decl_file (amd64-invalid-stack-middle.c)
	.byte	0x30	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.quad	.LFB4	# DW_AT_low_pc
	.quad	.LFE4	# DW_AT_high_pc
	.long	.LLST4	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_tail_call_sites
	.long	0x11a	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x10b) DW_TAG_formal_parameter)
	.ascii "ptr\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (amd64-invalid-stack-middle.c)
	.byte	0x30	# DW_AT_decl_line
	.long	0x6c	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -24
	.byte	0	# end of children of DIE 0xe9
	.uleb128 0x7	# (DIE (0x11a) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF14	# DW_AT_name: "func1"
	.byte	0x1	# DW_AT_decl_file (amd64-invalid-stack-middle.c)
	.byte	0x36	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.quad	.LFB5	# DW_AT_low_pc
	.quad	.LFE5	# DW_AT_high_pc
	.long	.LLST5	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_tail_call_sites
	.long	0x14b	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x13c) DW_TAG_formal_parameter)
	.ascii "ptr\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (amd64-invalid-stack-middle.c)
	.byte	0x36	# DW_AT_decl_line
	.long	0x6c	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -24
	.byte	0	# end of children of DIE 0x11a
	.uleb128 0x9	# (DIE (0x14b) DW_TAG_subprogram)
	.long	.LASF20	# DW_AT_name: "make_invalid_ptr"
	.byte	0x1	# DW_AT_decl_file (amd64-invalid-stack-middle.c)
	.byte	0x40	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x6c	# DW_AT_type
	.quad	.LFB6	# DW_AT_low_pc
	.quad	.LFE6	# DW_AT_high_pc
	.long	.LLST6	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_tail_call_sites
	.long	0x1af	# DW_AT_sibling
	.uleb128 0xa	# (DIE (0x170) DW_TAG_variable)
	.long	.LASF15	# DW_AT_name: "page_size"
	.byte	0x1	# DW_AT_decl_file (amd64-invalid-stack-middle.c)
	.byte	0x42	# DW_AT_decl_line
	.long	0x57	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -20
	.uleb128 0xb	# (DIE (0x17e) DW_TAG_variable)
	.ascii "ans\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (amd64-invalid-stack-middle.c)
	.byte	0x42	# DW_AT_decl_line
	.long	0x57	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -36
	.uleb128 0xb	# (DIE (0x18c) DW_TAG_variable)
	.ascii "ptr\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (amd64-invalid-stack-middle.c)
	.byte	0x43	# DW_AT_decl_line
	.long	0x6c	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -32
	.uleb128 0xc	# (DIE (0x19a) DW_TAG_variable)
	.long	.LASF21	# DW_AT_name: "__PRETTY_FUNCTION__"
	.long	0x1bf	# DW_AT_type
	.byte	0x1	# DW_AT_artificial
	.byte	0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	__PRETTY_FUNCTION__.2362
	.byte	0	# end of children of DIE 0x14b
	.uleb128 0xd	# (DIE (0x1af) DW_TAG_array_type)
	.long	0x6e	# DW_AT_type
	.long	0x1bf	# DW_AT_sibling
	.uleb128 0xe	# (DIE (0x1b8) DW_TAG_subrange_type)
	.long	0x65	# DW_AT_type
	.byte	0x10	# DW_AT_upper_bound
	.byte	0	# end of children of DIE 0x1af
	.uleb128 0xf	# (DIE (0x1bf) DW_TAG_const_type)
	.long	0x1af	# DW_AT_type
	.uleb128 0x10	# (DIE (0x1c4) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF22	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (amd64-invalid-stack-middle.c)
	.byte	0x51	# DW_AT_decl_line
	.long	0x57	# DW_AT_type
	.quad	.LFB7	# DW_AT_low_pc
	.quad	.LFE7	# DW_AT_high_pc
	.long	.LLST7	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_tail_call_sites
	.uleb128 0xa	# (DIE (0x1e5) DW_TAG_variable)
	.long	.LASF16	# DW_AT_name: "invalid_ptr"
	.byte	0x1	# DW_AT_decl_file (amd64-invalid-stack-middle.c)
	.byte	0x53	# DW_AT_decl_line
	.long	0x6c	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -24
	.byte	0	# end of children of DIE 0x1c4
	.byte	0	# end of children of DIE 0xb
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1	# (abbrev code)
	.uleb128 0x11	# (TAG: DW_TAG_compile_unit)
	.byte	0x1	# DW_children_yes
	.uleb128 0x25	# (DW_AT_producer)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x13	# (DW_AT_language)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x1b	# (DW_AT_comp_dir)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x10	# (DW_AT_stmt_list)
	.uleb128 0x6	# (DW_FORM_data4)
	.byte	0
	.byte	0
	.uleb128 0x2	# (abbrev code)
	.uleb128 0x24	# (TAG: DW_TAG_base_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3e	# (DW_AT_encoding)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.byte	0
	.byte	0
	.uleb128 0x3	# (abbrev code)
	.uleb128 0x24	# (TAG: DW_TAG_base_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3e	# (DW_AT_encoding)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.byte	0
	.byte	0
	.uleb128 0x4	# (abbrev code)
	.uleb128 0xf	# (TAG: DW_TAG_pointer_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.byte	0
	.byte	0
	.uleb128 0x5	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0	# DW_children_no
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.byte	0
	.byte	0
	.uleb128 0x6	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0	# DW_children_no
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x2116	# (DW_AT_GNU_all_tail_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.byte	0
	.byte	0
	.uleb128 0x7	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x2116	# (DW_AT_GNU_all_tail_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x8	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.uleb128 0x9	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x2116	# (DW_AT_GNU_all_tail_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xa	# (abbrev code)
	.uleb128 0x34	# (TAG: DW_TAG_variable)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.uleb128 0xb	# (abbrev code)
	.uleb128 0x34	# (TAG: DW_TAG_variable)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.uleb128 0xc	# (abbrev code)
	.uleb128 0x34	# (TAG: DW_TAG_variable)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x34	# (DW_AT_artificial)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.uleb128 0xd	# (abbrev code)
	.uleb128 0x1	# (TAG: DW_TAG_array_type)
	.byte	0x1	# DW_children_yes
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xe	# (abbrev code)
	.uleb128 0x21	# (TAG: DW_TAG_subrange_type)
	.byte	0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2f	# (DW_AT_upper_bound)
	.uleb128 0xb	# (DW_FORM_data1)
	.byte	0
	.byte	0
	.uleb128 0xf	# (abbrev code)
	.uleb128 0x26	# (TAG: DW_TAG_const_type)
	.byte	0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x10	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x2116	# (DW_AT_GNU_all_tail_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST0:
	.quad	.LFB0-.Ltext0	# Location list begin address (*.LLST0)
	.quad	.LCFI0-.Ltext0	# Location list end address (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI0-.Ltext0	# Location list begin address (*.LLST0)
	.quad	.LCFI1-.Ltext0	# Location list end address (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI1-.Ltext0	# Location list begin address (*.LLST0)
	.quad	.LCFI2-.Ltext0	# Location list end address (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x76	# DW_OP_breg6
	.sleb128 16
	.quad	.LCFI2-.Ltext0	# Location list begin address (*.LLST0)
	.quad	.LFE0-.Ltext0	# Location list end address (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	0	# Location list terminator begin (*.LLST0)
	.quad	0	# Location list terminator end (*.LLST0)
.LLST1:
	.quad	.LFB1-.Ltext0	# Location list begin address (*.LLST1)
	.quad	.LCFI3-.Ltext0	# Location list end address (*.LLST1)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI3-.Ltext0	# Location list begin address (*.LLST1)
	.quad	.LCFI4-.Ltext0	# Location list end address (*.LLST1)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI4-.Ltext0	# Location list begin address (*.LLST1)
	.quad	.LCFI5-.Ltext0	# Location list end address (*.LLST1)
	.value	0x2	# Location expression size
	.byte	0x76	# DW_OP_breg6
	.sleb128 16
	.quad	.LCFI5-.Ltext0	# Location list begin address (*.LLST1)
	.quad	.LFE1-.Ltext0	# Location list end address (*.LLST1)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	0	# Location list terminator begin (*.LLST1)
	.quad	0	# Location list terminator end (*.LLST1)
.LLST2:
	.quad	.LFB2-.Ltext0	# Location list begin address (*.LLST2)
	.quad	.LCFI6-.Ltext0	# Location list end address (*.LLST2)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI6-.Ltext0	# Location list begin address (*.LLST2)
	.quad	.LCFI7-.Ltext0	# Location list end address (*.LLST2)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI7-.Ltext0	# Location list begin address (*.LLST2)
	.quad	.LCFI8-.Ltext0	# Location list end address (*.LLST2)
	.value	0x2	# Location expression size
	.byte	0x76	# DW_OP_breg6
	.sleb128 16
	.quad	.LCFI8-.Ltext0	# Location list begin address (*.LLST2)
	.quad	.LFE2-.Ltext0	# Location list end address (*.LLST2)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	0	# Location list terminator begin (*.LLST2)
	.quad	0	# Location list terminator end (*.LLST2)
.LLST3:
	.quad	.LFB3-.Ltext0	# Location list begin address (*.LLST3)
	.quad	.LCFI9-.Ltext0	# Location list end address (*.LLST3)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI9-.Ltext0	# Location list begin address (*.LLST3)
	.quad	.LCFI10-.Ltext0	# Location list end address (*.LLST3)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI10-.Ltext0	# Location list begin address (*.LLST3)
	.quad	.LCFI11-.Ltext0	# Location list end address (*.LLST3)
	.value	0x2	# Location expression size
	.byte	0x76	# DW_OP_breg6
	.sleb128 16
	.quad	.LCFI11-.Ltext0	# Location list begin address (*.LLST3)
	.quad	.LFE3-.Ltext0	# Location list end address (*.LLST3)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	0	# Location list terminator begin (*.LLST3)
	.quad	0	# Location list terminator end (*.LLST3)
.LLST4:
	.quad	.LFB4-.Ltext0	# Location list begin address (*.LLST4)
	.quad	.LCFI12-.Ltext0	# Location list end address (*.LLST4)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI12-.Ltext0	# Location list begin address (*.LLST4)
	.quad	.LCFI13-.Ltext0	# Location list end address (*.LLST4)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI13-.Ltext0	# Location list begin address (*.LLST4)
	.quad	.LCFI14-.Ltext0	# Location list end address (*.LLST4)
	.value	0x2	# Location expression size
	.byte	0x76	# DW_OP_breg6
	.sleb128 16
	.quad	.LCFI14-.Ltext0	# Location list begin address (*.LLST4)
	.quad	.LFE4-.Ltext0	# Location list end address (*.LLST4)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	0	# Location list terminator begin (*.LLST4)
	.quad	0	# Location list terminator end (*.LLST4)
.LLST5:
	.quad	.LFB5-.Ltext0	# Location list begin address (*.LLST5)
	.quad	.LCFI15-.Ltext0	# Location list end address (*.LLST5)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI15-.Ltext0	# Location list begin address (*.LLST5)
	.quad	.LCFI16-.Ltext0	# Location list end address (*.LLST5)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI16-.Ltext0	# Location list begin address (*.LLST5)
	.quad	.LCFI17-.Ltext0	# Location list end address (*.LLST5)
	.value	0x2	# Location expression size
	.byte	0x76	# DW_OP_breg6
	.sleb128 16
	.quad	.LCFI17-.Ltext0	# Location list begin address (*.LLST5)
	.quad	.LFE5-.Ltext0	# Location list end address (*.LLST5)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	0	# Location list terminator begin (*.LLST5)
	.quad	0	# Location list terminator end (*.LLST5)
.LLST6:
	.quad	.LFB6-.Ltext0	# Location list begin address (*.LLST6)
	.quad	.LCFI18-.Ltext0	# Location list end address (*.LLST6)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI18-.Ltext0	# Location list begin address (*.LLST6)
	.quad	.LCFI19-.Ltext0	# Location list end address (*.LLST6)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI19-.Ltext0	# Location list begin address (*.LLST6)
	.quad	.LCFI20-.Ltext0	# Location list end address (*.LLST6)
	.value	0x2	# Location expression size
	.byte	0x76	# DW_OP_breg6
	.sleb128 16
	.quad	.LCFI20-.Ltext0	# Location list begin address (*.LLST6)
	.quad	.LFE6-.Ltext0	# Location list end address (*.LLST6)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	0	# Location list terminator begin (*.LLST6)
	.quad	0	# Location list terminator end (*.LLST6)
.LLST7:
	.quad	.LFB7-.Ltext0	# Location list begin address (*.LLST7)
	.quad	.LCFI21-.Ltext0	# Location list end address (*.LLST7)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI21-.Ltext0	# Location list begin address (*.LLST7)
	.quad	.LCFI22-.Ltext0	# Location list end address (*.LLST7)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI22-.Ltext0	# Location list begin address (*.LLST7)
	.quad	.LCFI23-.Ltext0	# Location list end address (*.LLST7)
	.value	0x2	# Location expression size
	.byte	0x76	# DW_OP_breg6
	.sleb128 16
	.quad	.LCFI23-.Ltext0	# Location list begin address (*.LLST7)
	.quad	.LFE7-.Ltext0	# Location list end address (*.LLST7)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	0	# Location list terminator begin (*.LLST7)
	.quad	0	# Location list terminator end (*.LLST7)
	.section	.debug_aranges,"",@progbits
	.long	0x2c	# Length of Address Ranges Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.byte	0x8	# Size of Address
	.byte	0	# Size of Segment Descriptor
	.value	0	# Pad to 16 byte boundary
	.value	0
	.quad	.Ltext0	# Address
	.quad	.Letext0-.Ltext0	# Length
	.quad	0
	.quad	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF10:
	.string	"func5"
.LASF20:
	.string	"make_invalid_ptr"
.LASF21:
	.string	"__PRETTY_FUNCTION__"
.LASF18:
	.string	"amd64-invalid-stack-middle.c"
.LASF22:
	.string	"main"
.LASF14:
	.string	"func1"
.LASF17:
	.string	"GNU C 4.7.2"
.LASF11:
	.string	"func4"
.LASF0:
	.string	"long unsigned int"
.LASF1:
	.string	"unsigned char"
.LASF8:
	.string	"char"
.LASF6:
	.string	"long int"
.LASF15:
	.string	"page_size"
.LASF13:
	.string	"func2"
.LASF16:
	.string	"invalid_ptr"
.LASF2:
	.string	"short unsigned int"
.LASF4:
	.string	"signed char"
.LASF9:
	.string	"breakpt"
.LASF19:
	.string	"/home/gdb/binutils-gdb/gdb/testsuite/gdb.arch"
.LASF5:
	.string	"short int"
.LASF3:
	.string	"unsigned int"
.LASF12:
	.string	"func3"
.LASF7:
	.string	"sizetype"
	.ident	"GCC: (GNU) 4.7.2"
	.section	.note.GNU-stack,"",@progbits
