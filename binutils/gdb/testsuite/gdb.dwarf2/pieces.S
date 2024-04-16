/*
   Copyright 2010-2024 Free Software Foundation, Inc.

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

/* This was compiled with a version of gcc modified to emit better
   debuginfo for SRA'd structures.  See:
	http://gcc.gnu.org/bugzilla/show_bug.cgi?id=43983
	
    The original program is "pieces.c", in this directory.
*/

	.file	"pieces.c"
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.text
.Ltext0:
	.p2align 4,,15
.globl bar
	.type	bar, @function
bar:
.LFB0:
	.file 1 "pieces.c"
	# pieces.c:29
	.loc 1 29 0
.LVL0:
	# basic block 2
	pushl	%ebp
.LCFI0:
	movl	%esp, %ebp
.LCFI1:
	# pieces.c:30
	.loc 1 30 0
	movl	8(%ebp), %eax
	# pieces.c:31
	.loc 1 31 0
	popl	%ebp
.LCFI2:
	ret
.LFE0:
	.size	bar, .-bar
	.p2align 4,,15
.globl f1
	.type	f1, @function
f1:
.LFB1:
	# pieces.c:35
	.loc 1 35 0
.LVL1:
	# basic block 2
	pushl	%ebp
.LCFI3:
	movl	%esp, %ebp
.LCFI4:
.LVL2:
	subl	$12, %esp
.LCFI5:
	movl	%esi, -4(%ebp)
.LCFI6:
	# pieces.c:38
	.loc 1 38 0
	movl	8(%ebp), %esi
	# pieces.c:35
	.loc 1 35 0
	movl	%ebx, -8(%ebp)
.LCFI7:
	# pieces.c:37
	.loc 1 37 0
	movl	$4, %ebx
.LVL3:
	# pieces.c:39
	.loc 1 39 0
	movl	%ebx, (%esp)
	# pieces.c:38
	.loc 1 38 0
	addl	$7, %esi
.LVL4:
	# pieces.c:39
	.loc 1 39 0
	call	bar
	# pieces.c:40
	.loc 1 40 0
	movl	%esi, (%esp)
	call	bar
	# pieces.c:41
	.loc 1 41 0
	leal	(%ebx,%esi), %eax
	# pieces.c:42
	.loc 1 42 0
	movl	-8(%ebp), %ebx
.LVL5:
	movl	-4(%ebp), %esi
.LVL6:
	movl	%ebp, %esp
.LCFI8:
	popl	%ebp
.LCFI9:
	ret
.LFE1:
	.size	f1, .-f1
	.p2align 4,,15
.globl f2
	.type	f2, @function
f2:
.LFB2:
	# pieces.c:46
	.loc 1 46 0
.LVL7:
	# basic block 2
	pushl	%ebp
.LCFI10:
	movl	%esp, %ebp
.LCFI11:
.LVL8:
	subl	$12, %esp
.LCFI12:
	movl	%esi, -4(%ebp)
.LCFI13:
	# pieces.c:49
	.loc 1 49 0
	movl	8(%ebp), %esi
	# pieces.c:46
	.loc 1 46 0
	movl	%ebx, -8(%ebp)
.LCFI14:
	# pieces.c:48
	.loc 1 48 0
	movl	$4, %ebx
.LVL9:
	# pieces.c:50
	.loc 1 50 0
	movl	%ebx, (%esp)
	# pieces.c:49
	.loc 1 49 0
	addl	$7, %esi
.LVL10:
	# pieces.c:50
	.loc 1 50 0
	call	bar
	# pieces.c:51
	.loc 1 51 0
	movl	%esi, (%esp)
	call	bar
	# pieces.c:52
	.loc 1 52 0
	leal	(%ebx,%esi), %eax
	# pieces.c:53
	.loc 1 53 0
	movl	-8(%ebp), %ebx
.LVL11:
	movl	-4(%ebp), %esi
.LVL12:
	movl	%ebp, %esp
.LCFI15:
	popl	%ebp
.LCFI16:
	ret
.LFE2:
	.size	f2, .-f2
	.p2align 4,,15
.globl f3
	.type	f3, @function
f3:
.LFB3:
	# pieces.c:57
	.loc 1 57 0
.LVL13:
	# basic block 2
	pushl	%ebp
.LCFI17:
	# pieces.c:59
	.loc 1 59 0
	movl	$4, %edx
	# pieces.c:57
	.loc 1 57 0
	movl	%esp, %ebp
.LCFI18:
.LVL14:
	subl	$12, %esp
.LCFI19:
	# pieces.c:59
	.loc 1 59 0
.LVL15:
	# pieces.c:57
	.loc 1 57 0
	movl	%esi, -4(%ebp)
.LCFI20:
	# pieces.c:61
	.loc 1 61 0
	movswl	%dx, %esi
	# pieces.c:57
	.loc 1 57 0
	movl	%ebx, -8(%ebp)
.LCFI21:
	# pieces.c:61
	.loc 1 61 0
	movl	%esi, (%esp)
	call	bar
.LVL16:
	# pieces.c:58
	.loc 1 58 0
	movl	8(%ebp), %edx
	sall	$4, %edx
	# pieces.c:60
	.loc 1 60 0
	addl	$112, %edx
	sarw	$4, %dx
	# pieces.c:62
	.loc 1 62 0
	movswl	%dx, %ebx
	movl	%ebx, (%esp)
	call	bar
	# pieces.c:63
	.loc 1 63 0
	leal	(%esi,%ebx), %eax
	# pieces.c:64
	.loc 1 64 0
	movl	-8(%ebp), %ebx
	movl	-4(%ebp), %esi
.LVL17:
	movl	%ebp, %esp
.LCFI22:
	popl	%ebp
.LCFI23:
	ret
.LFE3:
	.size	f3, .-f3
	.p2align 4,,15
.globl f4
	.type	f4, @function
f4:
.LFB4:
	# pieces.c:68
	.loc 1 68 0
.LVL18:
	# basic block 2
	pushl	%ebp
.LCFI24:
	movl	%esp, %ebp
.LCFI25:
	subl	$12, %esp
.LCFI26:
	movl	%esi, -4(%ebp)
.LCFI27:
	movl	8(%ebp), %esi
.LVL19:
	movl	%ebx, -8(%ebp)
.LCFI28:
	# pieces.c:70
	.loc 1 70 0
	movl	%esi, %ebx
	# pieces.c:71
	.loc 1 71 0
	addl	$1, %esi
	# pieces.c:70
	.loc 1 70 0
.LVL20:
	# pieces.c:72
	.loc 1 72 0
	movl	%ebx, (%esp)
	call	bar
	# pieces.c:73
	.loc 1 73 0
	movl	%esi, (%esp)
	call	bar
	# pieces.c:74
	.loc 1 74 0
	leal	(%ebx,%esi), %eax
	# pieces.c:75
	.loc 1 75 0
	movl	-8(%ebp), %ebx
.LVL21:
	movl	-4(%ebp), %esi
.LVL22:
	movl	%ebp, %esp
.LCFI29:
	popl	%ebp
.LCFI30:
	ret
.LFE4:
	.size	f4, .-f4
	.p2align 4,,15
.globl f5
	.type	f5, @function
f5:
.LFB5:
	# pieces.c:79
	.loc 1 79 0
.LVL23:
	# basic block 2
	pushl	%ebp
.LCFI31:
	movl	%esp, %ebp
.LCFI32:
	subl	$12, %esp
.LCFI33:
	movl	%esi, -4(%ebp)
.LCFI34:
	movl	8(%ebp), %esi
.LVL24:
	movl	%ebx, -8(%ebp)
.LCFI35:
	# pieces.c:81
	.loc 1 81 0
	movl	%esi, %ebx
	# pieces.c:82
	.loc 1 82 0
	addl	$1, %esi
	# pieces.c:81
	.loc 1 81 0
.LVL25:
	# pieces.c:83
	.loc 1 83 0
	movl	%ebx, (%esp)
	call	bar
	# pieces.c:84
	.loc 1 84 0
	movl	%esi, (%esp)
	call	bar
	# pieces.c:85
	.loc 1 85 0
	leal	(%ebx,%esi), %eax
	# pieces.c:86
	.loc 1 86 0
	movl	-8(%ebp), %ebx
.LVL26:
	movl	-4(%ebp), %esi
.LVL27:
	movl	%ebp, %esp
.LCFI36:
	popl	%ebp
.LCFI37:
	ret
.LFE5:
	.size	f5, .-f5
	.p2align 4,,15
.globl f6
	.type	f6, @function
f6:
.LFB6:
	# pieces.c:90
	.loc 1 90 0
.LVL28:
	# basic block 2
	pushl	%ebp
.LCFI38:
	movl	%esp, %ebp
.LCFI39:
	subl	$12, %esp
.LCFI40:
	movl	%esi, -4(%ebp)
.LCFI41:
	movl	8(%ebp), %esi
.LVL29:
	movl	%ebx, -8(%ebp)
.LCFI42:
	# pieces.c:93
	.loc 1 93 0
	movl	%esi, %ebx
	# pieces.c:94
	.loc 1 94 0
	addl	$1, %esi
	# pieces.c:93
	.loc 1 93 0
.LVL30:
	# pieces.c:95
	.loc 1 95 0
	movl	%ebx, (%esp)
	call	bar
	# pieces.c:96
	.loc 1 96 0
	movl	%esi, (%esp)
	call	bar
	# pieces.c:97
	.loc 1 97 0
	leal	(%ebx,%esi), %eax
	# pieces.c:98
	.loc 1 98 0
	movl	-8(%ebp), %ebx
.LVL31:
	movl	-4(%ebp), %esi
.LVL32:
	movl	%ebp, %esp
.LCFI43:
	popl	%ebp
.LCFI44:
	ret
.LFE6:
	.size	f6, .-f6
	.p2align 4,,15
.globl main
	.type	main, @function
main:
.LFB7:
	# pieces.c:102
	.loc 1 102 0
	# basic block 2
	pushl	%ebp
.LCFI45:
	movl	%esp, %ebp
.LCFI46:
	pushl	%ebx
.LCFI47:
	# pieces.c:104
	.loc 1 104 0
	movl	$7, %ebx
	# pieces.c:102
	.loc 1 102 0
	subl	$4, %esp
.LCFI48:
	# pieces.c:104
	.loc 1 104 0
.LVL33:
	# pieces.c:105
	.loc 1 105 0
	movl	%ebx, (%esp)
	call	f1
	# pieces.c:106
	.loc 1 106 0
	movl	%ebx, (%esp)
	call	f2
	# pieces.c:107
	.loc 1 107 0
	movl	%ebx, (%esp)
	call	f3
	# pieces.c:108
	.loc 1 108 0
	movl	%ebx, (%esp)
	call	f4
	# pieces.c:109
	.loc 1 109 0
	movl	%ebx, (%esp)
	call	f5
	# pieces.c:110
	.loc 1 110 0
	movl	%ebx, (%esp)
	call	f6
	# pieces.c:112
	.loc 1 112 0
	addl	$4, %esp
	xorl	%eax, %eax
	popl	%ebx
.LCFI49:
.LVL34:
	popl	%ebp
.LCFI50:
	ret
.LFE7:
	.size	main, .-main
#APP
	.section	.debug_frame,"",@progbits
.Lframe0:
	.long	.LECIE0-.LSCIE0	# Length of Common Information Entry
.LSCIE0:
	.long	0xffffffff	# CIE Identifier Tag
	.byte	0x1	# CIE Version
	.ascii "\0"	# CIE Augmentation
	.uleb128 0x1	# CIE Code Alignment Factor
	.sleb128 -4	# CIE Data Alignment Factor
	.byte	0x8	# CIE RA Column
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x4
	.uleb128 0x4
	.byte	0x88	# DW_CFA_offset, column 0x8
	.uleb128 0x1
	.align 4
.LECIE0:
.LSFDE0:
	.long	.LEFDE0-.LASFDE0	# FDE Length
.LASFDE0:
	.long	.Lframe0	# FDE CIE offset
	.long	.LFB0	# FDE initial location
	.long	.LFE0-.LFB0	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI0-.LFB0
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x8
	.byte	0x85	# DW_CFA_offset, column 0x5
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI1-.LCFI0
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x5
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI2-.LCFI1
	.byte	0xc5	# DW_CFA_restore, column 0x5
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x4
	.uleb128 0x4
	.align 4
.LEFDE0:
.LSFDE2:
	.long	.LEFDE2-.LASFDE2	# FDE Length
.LASFDE2:
	.long	.Lframe0	# FDE CIE offset
	.long	.LFB1	# FDE initial location
	.long	.LFE1-.LFB1	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI3-.LFB1
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x8
	.byte	0x85	# DW_CFA_offset, column 0x5
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI4-.LCFI3
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x5
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI6-.LCFI4
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI7-.LCFI6
	.byte	0x83	# DW_CFA_offset, column 0x3
	.uleb128 0x4
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI8-.LCFI7
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x4
	.byte	0xc6	# DW_CFA_restore, column 0x6
	.byte	0xc3	# DW_CFA_restore, column 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI9-.LCFI8
	.byte	0xc5	# DW_CFA_restore, column 0x5
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x4
	.align 4
.LEFDE2:
.LSFDE4:
	.long	.LEFDE4-.LASFDE4	# FDE Length
.LASFDE4:
	.long	.Lframe0	# FDE CIE offset
	.long	.LFB2	# FDE initial location
	.long	.LFE2-.LFB2	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI10-.LFB2
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x8
	.byte	0x85	# DW_CFA_offset, column 0x5
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI11-.LCFI10
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x5
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI13-.LCFI11
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI14-.LCFI13
	.byte	0x83	# DW_CFA_offset, column 0x3
	.uleb128 0x4
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI15-.LCFI14
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x4
	.byte	0xc6	# DW_CFA_restore, column 0x6
	.byte	0xc3	# DW_CFA_restore, column 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI16-.LCFI15
	.byte	0xc5	# DW_CFA_restore, column 0x5
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x4
	.align 4
.LEFDE4:
.LSFDE6:
	.long	.LEFDE6-.LASFDE6	# FDE Length
.LASFDE6:
	.long	.Lframe0	# FDE CIE offset
	.long	.LFB3	# FDE initial location
	.long	.LFE3-.LFB3	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI17-.LFB3
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x8
	.byte	0x85	# DW_CFA_offset, column 0x5
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI18-.LCFI17
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x5
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI20-.LCFI18
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI21-.LCFI20
	.byte	0x83	# DW_CFA_offset, column 0x3
	.uleb128 0x4
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI22-.LCFI21
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x4
	.byte	0xc6	# DW_CFA_restore, column 0x6
	.byte	0xc3	# DW_CFA_restore, column 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI23-.LCFI22
	.byte	0xc5	# DW_CFA_restore, column 0x5
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x4
	.align 4
.LEFDE6:
.LSFDE8:
	.long	.LEFDE8-.LASFDE8	# FDE Length
.LASFDE8:
	.long	.Lframe0	# FDE CIE offset
	.long	.LFB4	# FDE initial location
	.long	.LFE4-.LFB4	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI24-.LFB4
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x8
	.byte	0x85	# DW_CFA_offset, column 0x5
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI25-.LCFI24
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x5
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI27-.LCFI25
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI28-.LCFI27
	.byte	0x83	# DW_CFA_offset, column 0x3
	.uleb128 0x4
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI29-.LCFI28
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x4
	.byte	0xc6	# DW_CFA_restore, column 0x6
	.byte	0xc3	# DW_CFA_restore, column 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI30-.LCFI29
	.byte	0xc5	# DW_CFA_restore, column 0x5
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x4
	.align 4
.LEFDE8:
.LSFDE10:
	.long	.LEFDE10-.LASFDE10	# FDE Length
.LASFDE10:
	.long	.Lframe0	# FDE CIE offset
	.long	.LFB5	# FDE initial location
	.long	.LFE5-.LFB5	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI31-.LFB5
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x8
	.byte	0x85	# DW_CFA_offset, column 0x5
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI32-.LCFI31
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x5
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI34-.LCFI32
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI35-.LCFI34
	.byte	0x83	# DW_CFA_offset, column 0x3
	.uleb128 0x4
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI36-.LCFI35
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x4
	.byte	0xc6	# DW_CFA_restore, column 0x6
	.byte	0xc3	# DW_CFA_restore, column 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI37-.LCFI36
	.byte	0xc5	# DW_CFA_restore, column 0x5
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x4
	.align 4
.LEFDE10:
.LSFDE12:
	.long	.LEFDE12-.LASFDE12	# FDE Length
.LASFDE12:
	.long	.Lframe0	# FDE CIE offset
	.long	.LFB6	# FDE initial location
	.long	.LFE6-.LFB6	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI38-.LFB6
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x8
	.byte	0x85	# DW_CFA_offset, column 0x5
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI39-.LCFI38
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x5
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI41-.LCFI39
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI42-.LCFI41
	.byte	0x83	# DW_CFA_offset, column 0x3
	.uleb128 0x4
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI43-.LCFI42
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x4
	.byte	0xc6	# DW_CFA_restore, column 0x6
	.byte	0xc3	# DW_CFA_restore, column 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI44-.LCFI43
	.byte	0xc5	# DW_CFA_restore, column 0x5
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x4
	.align 4
.LEFDE12:
.LSFDE14:
	.long	.LEFDE14-.LASFDE14	# FDE Length
.LASFDE14:
	.long	.Lframe0	# FDE CIE offset
	.long	.LFB7	# FDE initial location
	.long	.LFE7-.LFB7	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI45-.LFB7
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x8
	.byte	0x85	# DW_CFA_offset, column 0x5
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI46-.LCFI45
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x5
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI47-.LCFI46
	.byte	0x83	# DW_CFA_offset, column 0x3
	.uleb128 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI49-.LCFI47
	.byte	0xc3	# DW_CFA_restore, column 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI50-.LCFI49
	.byte	0xc5	# DW_CFA_restore, column 0x5
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x4
	.uleb128 0x4
	.align 4
.LEFDE14:
#NO_APP
	.text
.Letext0:
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST0:
	.long	.LFB0-.Ltext0	# Location list begin address (*.LLST0)
	.long	.LCFI0-.Ltext0	# Location list end address (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	.LCFI0-.Ltext0	# Location list begin address (*.LLST0)
	.long	.LCFI1-.Ltext0	# Location list end address (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.long	.LCFI1-.Ltext0	# Location list begin address (*.LLST0)
	.long	.LCFI2-.Ltext0	# Location list end address (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 8
	.long	.LCFI2-.Ltext0	# Location list begin address (*.LLST0)
	.long	.LFE0-.Ltext0	# Location list end address (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	0	# Location list terminator begin (*.LLST0)
	.long	0	# Location list terminator end (*.LLST0)
.LLST1:
	.long	.LFB1-.Ltext0	# Location list begin address (*.LLST1)
	.long	.LCFI3-.Ltext0	# Location list end address (*.LLST1)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	.LCFI3-.Ltext0	# Location list begin address (*.LLST1)
	.long	.LCFI4-.Ltext0	# Location list end address (*.LLST1)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.long	.LCFI4-.Ltext0	# Location list begin address (*.LLST1)
	.long	.LCFI8-.Ltext0	# Location list end address (*.LLST1)
	.value	0x2	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 8
	.long	.LCFI8-.Ltext0	# Location list begin address (*.LLST1)
	.long	.LCFI9-.Ltext0	# Location list end address (*.LLST1)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.long	.LCFI9-.Ltext0	# Location list begin address (*.LLST1)
	.long	.LFE1-.Ltext0	# Location list end address (*.LLST1)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	0	# Location list terminator begin (*.LLST1)
	.long	0	# Location list terminator end (*.LLST1)
.LLST2:
	.long	.LVL1-.Ltext0	# Location list begin address (*.LLST2)
	.long	.LVL2-.Ltext0	# Location list end address (*.LLST2)
	.value	0x6	# Location expression size
	.byte	0x34	# DW_OP_lit4
	.byte	0x9f	# DW_OP_stack_value
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	.LVL2-.Ltext0	# Location list begin address (*.LLST2)
	.long	.LVL3-.Ltext0	# Location list end address (*.LLST2)
	.value	0xc	# Location expression size
	.byte	0x34	# DW_OP_lit4
	.byte	0x9f	# DW_OP_stack_value
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0x6	# DW_OP_deref
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x6
	.byte	0x9f	# DW_OP_stack_value
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	.LVL3-.Ltext0	# Location list begin address (*.LLST2)
	.long	.LVL4-.Ltext0	# Location list end address (*.LLST2)
	.value	0xb	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0x6	# DW_OP_deref
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x6
	.byte	0x9f	# DW_OP_stack_value
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	.LVL4-.Ltext0	# Location list begin address (*.LLST2)
	.long	.LVL5-.Ltext0	# Location list end address (*.LLST2)
	.value	0x6	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x56	# DW_OP_reg6
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	.LVL5-.Ltext0	# Location list begin address (*.LLST2)
	.long	.LVL6-.Ltext0	# Location list end address (*.LLST2)
	.value	0x5	# Location expression size
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x56	# DW_OP_reg6
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	0	# Location list terminator begin (*.LLST2)
	.long	0	# Location list terminator end (*.LLST2)
.LLST3:
	.long	.LFB2-.Ltext0	# Location list begin address (*.LLST3)
	.long	.LCFI10-.Ltext0	# Location list end address (*.LLST3)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	.LCFI10-.Ltext0	# Location list begin address (*.LLST3)
	.long	.LCFI11-.Ltext0	# Location list end address (*.LLST3)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.long	.LCFI11-.Ltext0	# Location list begin address (*.LLST3)
	.long	.LCFI15-.Ltext0	# Location list end address (*.LLST3)
	.value	0x2	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 8
	.long	.LCFI15-.Ltext0	# Location list begin address (*.LLST3)
	.long	.LCFI16-.Ltext0	# Location list end address (*.LLST3)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.long	.LCFI16-.Ltext0	# Location list begin address (*.LLST3)
	.long	.LFE2-.Ltext0	# Location list end address (*.LLST3)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	0	# Location list terminator begin (*.LLST3)
	.long	0	# Location list terminator end (*.LLST3)
.LLST4:
	.long	.LVL7-.Ltext0	# Location list begin address (*.LLST4)
	.long	.LVL8-.Ltext0	# Location list end address (*.LLST4)
	.value	0x6	# Location expression size
	.byte	0x34	# DW_OP_lit4
	.byte	0x9f	# DW_OP_stack_value
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	.LVL8-.Ltext0	# Location list begin address (*.LLST4)
	.long	.LVL9-.Ltext0	# Location list end address (*.LLST4)
	.value	0xc	# Location expression size
	.byte	0x34	# DW_OP_lit4
	.byte	0x9f	# DW_OP_stack_value
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0x6	# DW_OP_deref
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x6
	.byte	0x9f	# DW_OP_stack_value
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	.LVL9-.Ltext0	# Location list begin address (*.LLST4)
	.long	.LVL10-.Ltext0	# Location list end address (*.LLST4)
	.value	0xb	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0x6	# DW_OP_deref
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x6
	.byte	0x9f	# DW_OP_stack_value
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	.LVL10-.Ltext0	# Location list begin address (*.LLST4)
	.long	.LVL11-.Ltext0	# Location list end address (*.LLST4)
	.value	0x6	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x56	# DW_OP_reg6
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	.LVL11-.Ltext0	# Location list begin address (*.LLST4)
	.long	.LVL12-.Ltext0	# Location list end address (*.LLST4)
	.value	0x5	# Location expression size
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x56	# DW_OP_reg6
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	0	# Location list terminator begin (*.LLST4)
	.long	0	# Location list terminator end (*.LLST4)
.LLST5:
	.long	.LFB3-.Ltext0	# Location list begin address (*.LLST5)
	.long	.LCFI17-.Ltext0	# Location list end address (*.LLST5)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	.LCFI17-.Ltext0	# Location list begin address (*.LLST5)
	.long	.LCFI18-.Ltext0	# Location list end address (*.LLST5)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.long	.LCFI18-.Ltext0	# Location list begin address (*.LLST5)
	.long	.LCFI22-.Ltext0	# Location list end address (*.LLST5)
	.value	0x2	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 8
	.long	.LCFI22-.Ltext0	# Location list begin address (*.LLST5)
	.long	.LCFI23-.Ltext0	# Location list end address (*.LLST5)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.long	.LCFI23-.Ltext0	# Location list begin address (*.LLST5)
	.long	.LFE3-.Ltext0	# Location list end address (*.LLST5)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	0	# Location list terminator begin (*.LLST5)
	.long	0	# Location list terminator end (*.LLST5)
.LLST6:
	.long	.LVL13-.Ltext0	# Location list begin address (*.LLST6)
	.long	.LVL14-.Ltext0	# Location list end address (*.LLST6)
	.value	0x8	# Location expression size
	.byte	0x34	# DW_OP_lit4
	.byte	0x9f	# DW_OP_stack_value
	.byte	0x9d	# DW_OP_bit_piece
	.uleb128 0xc
	.uleb128 0
	.byte	0x9d	# DW_OP_bit_piece
	.uleb128 0x14
	.uleb128 0
	.long	.LVL14-.Ltext0	# Location list begin address (*.LLST6)
	.long	.LVL15-.Ltext0	# Location list end address (*.LLST6)
	.value	0x11	# Location expression size
	.byte	0x34	# DW_OP_lit4
	.byte	0x9f	# DW_OP_stack_value
	.byte	0x9d	# DW_OP_bit_piece
	.uleb128 0xc
	.uleb128 0
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0x94	# DW_OP_deref_size
	.byte	0x2
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x6
	.byte	0x9f	# DW_OP_stack_value
	.byte	0x9d	# DW_OP_bit_piece
	.uleb128 0xc
	.uleb128 0
	.byte	0x93	# DW_OP_piece
	.uleb128 0x1
	.long	.LVL15-.Ltext0	# Location list begin address (*.LLST6)
	.long	.LVL16-1-.Ltext0	# Location list end address (*.LLST6)
	.value	0x10	# Location expression size
	.byte	0x52	# DW_OP_reg2
	.byte	0x9d	# DW_OP_bit_piece
	.uleb128 0xc
	.uleb128 0
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0x94	# DW_OP_deref_size
	.byte	0x2
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x7
	.byte	0x9f	# DW_OP_stack_value
	.byte	0x9d	# DW_OP_bit_piece
	.uleb128 0xc
	.uleb128 0
	.byte	0x93	# DW_OP_piece
	.uleb128 0x1
	.long	.LVL16-1-.Ltext0	# Location list begin address (*.LLST6)
	.long	.LVL17-.Ltext0	# Location list end address (*.LLST6)
	.value	0x10	# Location expression size
	.byte	0x56	# DW_OP_reg6
	.byte	0x9d	# DW_OP_bit_piece
	.uleb128 0xc
	.uleb128 0
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0x94	# DW_OP_deref_size
	.byte	0x2
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x7
	.byte	0x9f	# DW_OP_stack_value
	.byte	0x9d	# DW_OP_bit_piece
	.uleb128 0xc
	.uleb128 0
	.byte	0x93	# DW_OP_piece
	.uleb128 0x1
	.long	.LVL17-.Ltext0	# Location list begin address (*.LLST6)
	.long	.LFE3-.Ltext0	# Location list end address (*.LLST6)
	.value	0xf	# Location expression size
	.byte	0x9d	# DW_OP_bit_piece
	.uleb128 0xc
	.uleb128 0
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0x94	# DW_OP_deref_size
	.byte	0x2
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x7
	.byte	0x9f	# DW_OP_stack_value
	.byte	0x9d	# DW_OP_bit_piece
	.uleb128 0xc
	.uleb128 0
	.byte	0x93	# DW_OP_piece
	.uleb128 0x1
	.long	0	# Location list terminator begin (*.LLST6)
	.long	0	# Location list terminator end (*.LLST6)
.LLST7:
	.long	.LFB4-.Ltext0	# Location list begin address (*.LLST7)
	.long	.LCFI24-.Ltext0	# Location list end address (*.LLST7)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	.LCFI24-.Ltext0	# Location list begin address (*.LLST7)
	.long	.LCFI25-.Ltext0	# Location list end address (*.LLST7)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.long	.LCFI25-.Ltext0	# Location list begin address (*.LLST7)
	.long	.LCFI29-.Ltext0	# Location list end address (*.LLST7)
	.value	0x2	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 8
	.long	.LCFI29-.Ltext0	# Location list begin address (*.LLST7)
	.long	.LCFI30-.Ltext0	# Location list end address (*.LLST7)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.long	.LCFI30-.Ltext0	# Location list begin address (*.LLST7)
	.long	.LFE4-.Ltext0	# Location list end address (*.LLST7)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	0	# Location list terminator begin (*.LLST7)
	.long	0	# Location list terminator end (*.LLST7)
.LLST8:
	.long	.LVL19-.Ltext0	# Location list begin address (*.LLST8)
	.long	.LVL20-.Ltext0	# Location list end address (*.LLST8)
	.value	0x8	# Location expression size
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	.LVL20-.Ltext0	# Location list begin address (*.LLST8)
	.long	.LVL21-.Ltext0	# Location list end address (*.LLST8)
	.value	0x6	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x56	# DW_OP_reg6
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	.LVL21-.Ltext0	# Location list begin address (*.LLST8)
	.long	.LVL22-.Ltext0	# Location list end address (*.LLST8)
	.value	0x5	# Location expression size
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x56	# DW_OP_reg6
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	0	# Location list terminator begin (*.LLST8)
	.long	0	# Location list terminator end (*.LLST8)
.LLST9:
	.long	.LFB5-.Ltext0	# Location list begin address (*.LLST9)
	.long	.LCFI31-.Ltext0	# Location list end address (*.LLST9)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	.LCFI31-.Ltext0	# Location list begin address (*.LLST9)
	.long	.LCFI32-.Ltext0	# Location list end address (*.LLST9)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.long	.LCFI32-.Ltext0	# Location list begin address (*.LLST9)
	.long	.LCFI36-.Ltext0	# Location list end address (*.LLST9)
	.value	0x2	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 8
	.long	.LCFI36-.Ltext0	# Location list begin address (*.LLST9)
	.long	.LCFI37-.Ltext0	# Location list end address (*.LLST9)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.long	.LCFI37-.Ltext0	# Location list begin address (*.LLST9)
	.long	.LFE5-.Ltext0	# Location list end address (*.LLST9)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	0	# Location list terminator begin (*.LLST9)
	.long	0	# Location list terminator end (*.LLST9)
.LLST10:
	.long	.LVL24-.Ltext0	# Location list begin address (*.LLST10)
	.long	.LVL25-.Ltext0	# Location list end address (*.LLST10)
	.value	0x8	# Location expression size
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	.LVL25-.Ltext0	# Location list begin address (*.LLST10)
	.long	.LVL26-.Ltext0	# Location list end address (*.LLST10)
	.value	0x6	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x56	# DW_OP_reg6
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	.LVL26-.Ltext0	# Location list begin address (*.LLST10)
	.long	.LVL27-.Ltext0	# Location list end address (*.LLST10)
	.value	0x5	# Location expression size
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x56	# DW_OP_reg6
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	0	# Location list terminator begin (*.LLST10)
	.long	0	# Location list terminator end (*.LLST10)
.LLST11:
	.long	.LFB6-.Ltext0	# Location list begin address (*.LLST11)
	.long	.LCFI38-.Ltext0	# Location list end address (*.LLST11)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	.LCFI38-.Ltext0	# Location list begin address (*.LLST11)
	.long	.LCFI39-.Ltext0	# Location list end address (*.LLST11)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.long	.LCFI39-.Ltext0	# Location list begin address (*.LLST11)
	.long	.LCFI43-.Ltext0	# Location list end address (*.LLST11)
	.value	0x2	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 8
	.long	.LCFI43-.Ltext0	# Location list begin address (*.LLST11)
	.long	.LCFI44-.Ltext0	# Location list end address (*.LLST11)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.long	.LCFI44-.Ltext0	# Location list begin address (*.LLST11)
	.long	.LFE6-.Ltext0	# Location list end address (*.LLST11)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	0	# Location list terminator begin (*.LLST11)
	.long	0	# Location list terminator end (*.LLST11)
.LLST12:
	.long	.LVL29-.Ltext0	# Location list begin address (*.LLST12)
	.long	.LVL30-.Ltext0	# Location list end address (*.LLST12)
	.value	0xa	# Location expression size
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	.LVL30-.Ltext0	# Location list begin address (*.LLST12)
	.long	.LVL31-.Ltext0	# Location list end address (*.LLST12)
	.value	0x8	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x56	# DW_OP_reg6
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	.LVL31-.Ltext0	# Location list begin address (*.LLST12)
	.long	.LVL32-.Ltext0	# Location list end address (*.LLST12)
	.value	0x7	# Location expression size
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x56	# DW_OP_reg6
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.long	0	# Location list terminator begin (*.LLST12)
	.long	0	# Location list terminator end (*.LLST12)
.LLST13:
	.long	.LFB7-.Ltext0	# Location list begin address (*.LLST13)
	.long	.LCFI45-.Ltext0	# Location list end address (*.LLST13)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	.LCFI45-.Ltext0	# Location list begin address (*.LLST13)
	.long	.LCFI46-.Ltext0	# Location list end address (*.LLST13)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.long	.LCFI46-.Ltext0	# Location list begin address (*.LLST13)
	.long	.LCFI50-.Ltext0	# Location list end address (*.LLST13)
	.value	0x2	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 8
	.long	.LCFI50-.Ltext0	# Location list begin address (*.LLST13)
	.long	.LFE7-.Ltext0	# Location list end address (*.LLST13)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	0	# Location list terminator begin (*.LLST13)
	.long	0	# Location list terminator end (*.LLST13)
.LLST14:
	.long	.LVL33-.Ltext0	# Location list begin address (*.LLST14)
	.long	.LVL34-.Ltext0	# Location list end address (*.LLST14)
	.value	0x1	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.long	0	# Location list terminator begin (*.LLST14)
	.long	0	# Location list terminator end (*.LLST14)
	.section	.debug_info
	.long	0x252	# Length of Compilation Unit Info
	.value	0x2	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x4	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF1	# DW_AT_producer: "GNU C 4.6.0 20100506 (experimental) [trunk revision 159117]"
	.byte	0x1	# DW_AT_language
	.long	.LASF2	# DW_AT_name: "pieces.c"
	.long	.LASF3	# DW_AT_comp_dir: "/home/tromey/gnu/archer/archer/gdb/testsuite/gdb.dwarf2"
	.long	.Ltext0	# DW_AT_low_pc
	.long	.Letext0	# DW_AT_high_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x25) DW_TAG_structure_type)
	.ascii "A\0"	# DW_AT_name
	.byte	0x8	# DW_AT_byte_size
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x17	# DW_AT_decl_line
	.long	0x48	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x2f) DW_TAG_member)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x17	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x2	# DW_AT_data_member_location
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0
	.uleb128 0x3	# (DIE (0x3b) DW_TAG_member)
	.ascii "j\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x17	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x2	# DW_AT_data_member_location
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x4
	.byte	0	# end of children of DIE 0x25
	.uleb128 0x4	# (DIE (0x48) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x2	# (DIE (0x4f) DW_TAG_structure_type)
	.ascii "B\0"	# DW_AT_name
	.byte	0x4	# DW_AT_byte_size
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x18	# DW_AT_decl_line
	.long	0x78	# DW_AT_sibling
	.uleb128 0x5	# (DIE (0x59) DW_TAG_member)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x18	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x4	# DW_AT_byte_size
	.byte	0xc	# DW_AT_bit_size
	.byte	0x14	# DW_AT_bit_offset
	.byte	0x2	# DW_AT_data_member_location
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0
	.uleb128 0x5	# (DIE (0x68) DW_TAG_member)
	.ascii "j\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x18	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x4	# DW_AT_byte_size
	.byte	0xc	# DW_AT_bit_size
	.byte	0x8	# DW_AT_bit_offset
	.byte	0x2	# DW_AT_data_member_location
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0
	.byte	0	# end of children of DIE 0x4f
	.uleb128 0x2	# (DIE (0x78) DW_TAG_structure_type)
	.ascii "C\0"	# DW_AT_name
	.byte	0xc	# DW_AT_byte_size
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x19	# DW_AT_decl_line
	.long	0xa7	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x82) DW_TAG_member)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x19	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x2	# DW_AT_data_member_location
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0
	.uleb128 0x3	# (DIE (0x8e) DW_TAG_member)
	.ascii "j\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x19	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x2	# DW_AT_data_member_location
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x4
	.uleb128 0x3	# (DIE (0x9a) DW_TAG_member)
	.ascii "q\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x19	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x2	# DW_AT_data_member_location
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x8
	.byte	0	# end of children of DIE 0x78
	.uleb128 0x6	# (DIE (0xa7) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.ascii "bar\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x1c	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	.LFB0	# DW_AT_low_pc
	.long	.LFE0	# DW_AT_high_pc
	.long	.LLST0	# DW_AT_frame_base
	.long	0xcd	# DW_AT_sibling
	.uleb128 0x7	# (DIE (0xc0) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x1c	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0	# end of children of DIE 0xa7
	.uleb128 0x8	# (DIE (0xcd) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.ascii "f1\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x22	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.long	.LFB1	# DW_AT_low_pc
	.long	.LFE1	# DW_AT_high_pc
	.long	.LLST1	# DW_AT_frame_base
	.long	0x103	# DW_AT_sibling
	.uleb128 0x7	# (DIE (0xe9) DW_TAG_formal_parameter)
	.ascii "k\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x22	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.uleb128 0x9	# (DIE (0xf5) DW_TAG_variable)
	.ascii "a\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x24	# DW_AT_decl_line
	.long	0x25	# DW_AT_type
	.long	.LLST2	# DW_AT_location
	.byte	0	# end of children of DIE 0xcd
	.uleb128 0x8	# (DIE (0x103) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.ascii "f2\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x2d	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.long	.LFB2	# DW_AT_low_pc
	.long	.LFE2	# DW_AT_high_pc
	.long	.LLST3	# DW_AT_frame_base
	.long	0x139	# DW_AT_sibling
	.uleb128 0x7	# (DIE (0x11f) DW_TAG_formal_parameter)
	.ascii "k\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x2d	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.uleb128 0x9	# (DIE (0x12b) DW_TAG_variable)
	.ascii "a\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x2f	# DW_AT_decl_line
	.long	0x139	# DW_AT_type
	.long	.LLST4	# DW_AT_location
	.byte	0	# end of children of DIE 0x103
	.uleb128 0xa	# (DIE (0x139) DW_TAG_array_type)
	.long	0x48	# DW_AT_type
	.long	0x149	# DW_AT_sibling
	.uleb128 0xb	# (DIE (0x142) DW_TAG_subrange_type)
	.long	0x149	# DW_AT_type
	.byte	0x1	# DW_AT_upper_bound
	.byte	0	# end of children of DIE 0x139
	.uleb128 0xc	# (DIE (0x149) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x7	# DW_AT_encoding
	.uleb128 0x8	# (DIE (0x14c) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.ascii "f3\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x38	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.long	.LFB3	# DW_AT_low_pc
	.long	.LFE3	# DW_AT_high_pc
	.long	.LLST5	# DW_AT_frame_base
	.long	0x182	# DW_AT_sibling
	.uleb128 0x7	# (DIE (0x168) DW_TAG_formal_parameter)
	.ascii "k\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x38	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.uleb128 0x9	# (DIE (0x174) DW_TAG_variable)
	.ascii "a\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x3a	# DW_AT_decl_line
	.long	0x4f	# DW_AT_type
	.long	.LLST6	# DW_AT_location
	.byte	0	# end of children of DIE 0x14c
	.uleb128 0x8	# (DIE (0x182) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.ascii "f4\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x43	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.long	.LFB4	# DW_AT_low_pc
	.long	.LFE4	# DW_AT_high_pc
	.long	.LLST7	# DW_AT_frame_base
	.long	0x1b8	# DW_AT_sibling
	.uleb128 0x7	# (DIE (0x19e) DW_TAG_formal_parameter)
	.ascii "k\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x43	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.uleb128 0x9	# (DIE (0x1aa) DW_TAG_variable)
	.ascii "a\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x45	# DW_AT_decl_line
	.long	0x139	# DW_AT_type
	.long	.LLST8	# DW_AT_location
	.byte	0	# end of children of DIE 0x182
	.uleb128 0x8	# (DIE (0x1b8) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.ascii "f5\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x4e	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.long	.LFB5	# DW_AT_low_pc
	.long	.LFE5	# DW_AT_high_pc
	.long	.LLST9	# DW_AT_frame_base
	.long	0x1ee	# DW_AT_sibling
	.uleb128 0x7	# (DIE (0x1d4) DW_TAG_formal_parameter)
	.ascii "k\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x4e	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.uleb128 0x9	# (DIE (0x1e0) DW_TAG_variable)
	.ascii "a\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x50	# DW_AT_decl_line
	.long	0x25	# DW_AT_type
	.long	.LLST10	# DW_AT_location
	.byte	0	# end of children of DIE 0x1b8
	.uleb128 0x8	# (DIE (0x1ee) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.ascii "f6\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x59	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.long	.LFB6	# DW_AT_low_pc
	.long	.LFE6	# DW_AT_high_pc
	.long	.LLST11	# DW_AT_frame_base
	.long	0x22e	# DW_AT_sibling
	.uleb128 0x7	# (DIE (0x20a) DW_TAG_formal_parameter)
	.ascii "k\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x59	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.uleb128 0xd	# (DIE (0x216) DW_TAG_variable)
	.ascii "z\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x5b	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.byte	0x17	# DW_AT_const_value
	.uleb128 0x9	# (DIE (0x220) DW_TAG_variable)
	.ascii "a\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x5c	# DW_AT_decl_line
	.long	0x78	# DW_AT_type
	.long	.LLST12	# DW_AT_location
	.byte	0	# end of children of DIE 0x1ee
	.uleb128 0xe	# (DIE (0x22e) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF0	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x65	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x48	# DW_AT_type
	.long	.LFB7	# DW_AT_low_pc
	.long	.LFE7	# DW_AT_high_pc
	.long	.LLST13	# DW_AT_frame_base
	.uleb128 0x9	# (DIE (0x247) DW_TAG_variable)
	.ascii "k\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (pieces.c)
	.byte	0x67	# DW_AT_decl_line
	.long	0x48	# DW_AT_type
	.long	.LLST14	# DW_AT_location
	.byte	0	# end of children of DIE 0x22e
	.byte	0	# end of children of DIE 0xb
	.section	.debug_abbrev
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
	.uleb128 0x13	# (TAG: DW_TAG_structure_type)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x3	# (abbrev code)
	.uleb128 0xd	# (TAG: DW_TAG_member)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x38	# (DW_AT_data_member_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.uleb128 0x4	# (abbrev code)
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
	.uleb128 0x5	# (abbrev code)
	.uleb128 0xd	# (TAG: DW_TAG_member)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0xd	# (DW_AT_bit_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0xc	# (DW_AT_bit_offset)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x38	# (DW_AT_data_member_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.uleb128 0x6	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
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
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x7	# (abbrev code)
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
	.uleb128 0x8	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
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
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x9	# (abbrev code)
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
	.uleb128 0x6	# (DW_FORM_data4)
	.byte	0
	.byte	0
	.uleb128 0xa	# (abbrev code)
	.uleb128 0x1	# (TAG: DW_TAG_array_type)
	.byte	0x1	# DW_children_yes
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xb	# (abbrev code)
	.uleb128 0x21	# (TAG: DW_TAG_subrange_type)
	.byte	0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2f	# (DW_AT_upper_bound)
	.uleb128 0xb	# (DW_FORM_data1)
	.byte	0
	.byte	0
	.uleb128 0xc	# (abbrev code)
	.uleb128 0x24	# (TAG: DW_TAG_base_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3e	# (DW_AT_encoding)
	.uleb128 0xb	# (DW_FORM_data1)
	.byte	0
	.byte	0
	.uleb128 0xd	# (abbrev code)
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
	.uleb128 0x1c	# (DW_AT_const_value)
	.uleb128 0xb	# (DW_FORM_data1)
	.byte	0
	.byte	0
	.uleb128 0xe	# (abbrev code)
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
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x6	# (DW_FORM_data4)
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_pubnames,"",@progbits
	.long	0x49	# Length of Public Names Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.long	0x256	# Compilation Unit Length
	.long	0xa7	# DIE offset
	.ascii "bar\0"	# external name
	.long	0xcd	# DIE offset
	.ascii "f1\0"	# external name
	.long	0x103	# DIE offset
	.ascii "f2\0"	# external name
	.long	0x14c	# DIE offset
	.ascii "f3\0"	# external name
	.long	0x182	# DIE offset
	.ascii "f4\0"	# external name
	.long	0x1b8	# DIE offset
	.ascii "f5\0"	# external name
	.long	0x1ee	# DIE offset
	.ascii "f6\0"	# external name
	.long	0x22e	# DIE offset
	.ascii "main\0"	# external name
	.long	0
	.section	.debug_pubtypes,"",@progbits
	.long	0x20	# Length of Public Type Names Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.long	0x256	# Compilation Unit Length
	.long	0x25	# DIE offset
	.ascii "A\0"	# external name
	.long	0x4f	# DIE offset
	.ascii "B\0"	# external name
	.long	0x78	# DIE offset
	.ascii "C\0"	# external name
	.long	0
	.section	.debug_aranges,"",@progbits
	.long	0x1c	# Length of Address Ranges Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.byte	0x4	# Size of Address
	.byte	0	# Size of Segment Descriptor
	.value	0	# Pad to 8 byte boundary
	.value	0
	.long	.Ltext0	# Address
	.long	.Letext0-.Ltext0	# Length
	.long	0
	.long	0
	.section	.debug_str,"MS",@progbits,1
.LASF2:
	.string	"pieces.c"
.LASF3:
	.string	"/home/tromey/gnu/archer/archer/gdb/testsuite/gdb.dwarf2"
.LASF1:
	.string	"GNU C 4.6.0 20100506 (experimental) [trunk revision 159117]"
.LASF0:
	.string	"main"
	.ident	"GCC: (GNU) 4.6.0 20100506 (experimental) [trunk revision 159117]"
	.section	.note.GNU-stack,"",@progbits
