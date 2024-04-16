/* This testcase is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

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

/* This source file was generated from typeddwarf.c using the following
   command line:

   gcc -m32 -dA -S -g -O2 typeddwarf.c -o typeddwarf.S

*/


	.file	"typeddwarf.c"
	.text
.Ltext0:
	.p2align 4,,15
	.globl	f1
	.type	f1, @function
f1:
.LFB0:
	# typeddwarf.c:10
.LM1:
.LVL0:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# typeddwarf.c:29
.LM2:
	movl	vv, %eax
	addl	$1, %eax
	movl	%eax, vv
# SUCC: EXIT [100.0%] 
	# typeddwarf.c:30
.LM3:
	ret
.LFE0:
	.size	f1, .-f1
	.p2align 4,,15
	.globl	f2
	.type	f2, @function
f2:
.LFB1:
	# typeddwarf.c:34
.LM4:
.LVL1:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	subl	$12, %esp
.LCFI0:
	# typeddwarf.c:53
.LM5:
	movl	vv, %eax
	# typeddwarf.c:54
.LM6:
	fnstcw	6(%esp)
	# typeddwarf.c:34
.LM7:
	flds	40(%esp)
.LVL2:
	# typeddwarf.c:54
.LM8:
	fldl	16(%esp)
	# typeddwarf.c:53
.LM9:
	addl	$1, %eax
	movl	%eax, vv
	# typeddwarf.c:54
.LM10:
	movzwl	6(%esp), %eax
	movb	$12, %ah
	movw	%ax, 4(%esp)
	fldcw	4(%esp)
	fistpl	(%esp)
	fldcw	6(%esp)
	movl	(%esp), %eax
	# typeddwarf.c:55
.LM11:
	fldl	24(%esp)
	fldcw	4(%esp)
	fistpl	(%esp)
	fldcw	6(%esp)
	# typeddwarf.c:54
.LM12:
	movl	%eax, vv
	# typeddwarf.c:55
.LM13:
	movl	(%esp), %eax
	# typeddwarf.c:56
.LM14:
	fldl	32(%esp)
	fldcw	4(%esp)
	fistpl	(%esp)
	fldcw	6(%esp)
	# typeddwarf.c:55
.LM15:
	movl	%eax, vv
	# typeddwarf.c:56
.LM16:
	movl	(%esp), %eax
	# typeddwarf.c:57
.LM17:
	fldcw	4(%esp)
	fistl	(%esp)
	fldcw	6(%esp)
	# typeddwarf.c:56
.LM18:
	movl	%eax, vv
	# typeddwarf.c:57
.LM19:
	movl	(%esp), %eax
	# typeddwarf.c:58
.LM20:
	flds	44(%esp)
	# typeddwarf.c:57
.LM21:
	movl	%eax, vv
	# typeddwarf.c:58
.LM22:
	fldcw	4(%esp)
	fistpl	(%esp)
	fldcw	6(%esp)
	movl	(%esp), %eax
	movl	%eax, vv
	# typeddwarf.c:59
.LM23:
	movl	48(%esp), %eax
	# typeddwarf.c:63
.LM24:
	fldcw	4(%esp)
	fistpl	(%esp)
	fldcw	6(%esp)
	# typeddwarf.c:59
.LM25:
	movl	%eax, vv
	# typeddwarf.c:60
.LM26:
	movl	52(%esp), %eax
	movl	%eax, vv
	# typeddwarf.c:61
.LM27:
	movl	56(%esp), %eax
	movl	%eax, vv
	# typeddwarf.c:62
.LM28:
	movl	64(%esp), %eax
	movl	%eax, vv
	# typeddwarf.c:63
.LM29:
	movl	(%esp), %eax
	movl	%eax, vv
	# typeddwarf.c:64
.LM30:
	addl	$12, %esp
.LCFI1:
# SUCC: EXIT [100.0%] 
	ret
.LFE1:
	.size	f2, .-f2
	.p2align 4,,15
	.globl	f3
	.type	f3, @function
f3:
.LFB2:
	# typeddwarf.c:68
.LM31:
.LVL3:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# typeddwarf.c:73
.LM32:
	movl	vv, %eax
	addl	$1, %eax
	movl	%eax, vv
# SUCC: EXIT [100.0%] 
	# typeddwarf.c:74
.LM33:
	ret
.LFE2:
	.size	f3, .-f3
	.p2align 4,,15
	.globl	f4
	.type	f4, @function
f4:
.LFB3:
	# typeddwarf.c:78
.LM34:
.LVL4:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# typeddwarf.c:82
.LM35:
	movl	vv, %eax
	addl	$1, %eax
	movl	%eax, vv
# SUCC: EXIT [100.0%] 
	# typeddwarf.c:83
.LM36:
	ret
.LFE3:
	.size	f4, .-f4
	.section	.text.startup,"ax",@progbits
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB4:
	# typeddwarf.c:87
.LM37:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	pushl	%ebp
.LCFI2:
	movl	%esp, %ebp
.LCFI3:
	pushl	%esi
.LCFI4:
	# typeddwarf.c:88
.LM38:
	movl	$0x40a00000, %esi
	# typeddwarf.c:87
.LM39:
	pushl	%ebx
.LCFI5:
	# typeddwarf.c:88
.LM40:
	movl	$0x40800000, %ebx
	# typeddwarf.c:87
.LM41:
	andl	$-16, %esp
	subl	$112, %esp
.LCFI6:
	# typeddwarf.c:88
.LM42:
	flds	.LC3
	fstl	16(%esp)
	movl	%esi, 28(%esp)
	flds	.LC4
	fstl	8(%esp)
	movl	%ebx, 24(%esp)
	fld1
	fstl	(%esp)
	movl	$9, 48(%esp)
	fstps	64(%esp)
	fstps	80(%esp)
	movl	$0, 52(%esp)
	fstps	96(%esp)
	movl	$8, 40(%esp)
	movl	$0, 44(%esp)
	movl	$7, 36(%esp)
	movl	$6, 32(%esp)
	call	f1
.LVL5:
	# typeddwarf.c:89
.LM43:
	movl	%esi, 28(%esp)
	movl	%ebx, 24(%esp)
	movl	$9, 48(%esp)
	movl	$0, 52(%esp)
	movl	$8, 40(%esp)
	flds	96(%esp)
	fstpl	16(%esp)
	movl	$0, 44(%esp)
	flds	80(%esp)
	fstpl	8(%esp)
	movl	$7, 36(%esp)
	flds	64(%esp)
	fstpl	(%esp)
	movl	$6, 32(%esp)
	call	f2
.LVL6:
	# typeddwarf.c:90
.LM44:
	movl	$4, 20(%esp)
	movl	$3, 12(%esp)
	movl	$0, 16(%esp)
	movl	$2, 8(%esp)
	movl	$1, (%esp)
	movl	$0, 4(%esp)
	call	f3
.LVL7:
	# typeddwarf.c:91
.LM45:
	movl	$640, 16(%esp)
	movl	$0, 20(%esp)
	movl	$0, 24(%esp)
	movl	$809369600, 28(%esp)
	movl	$160, 4(%esp)
	movl	$832569344, 8(%esp)
	movl	$838860880, (%esp)
	call	f4
.LVL8:
	# typeddwarf.c:93
.LM46:
	leal	-8(%ebp), %esp
	xorl	%eax, %eax
	popl	%ebx
.LCFI7:
	popl	%esi
.LCFI8:
	popl	%ebp
.LCFI9:
# SUCC: EXIT [100.0%] 
	ret
.LFE4:
	.size	main, .-main
	.comm	vv,4,4
	.section	.rodata.cst4,"aM",@progbits,4
	.align 4
.LC3:
	.long	1077936128
	.align 4
.LC4:
	.long	1073741824
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
	.align 4
.LEFDE0:
.LSFDE2:
	.long	.LEFDE2-.LASFDE2	# FDE Length
.LASFDE2:
	.long	.Lframe0	# FDE CIE offset
	.long	.LFB1	# FDE initial location
	.long	.LFE1-.LFB1	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI0-.LFB1
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI1-.LCFI0
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
	.align 4
.LEFDE4:
.LSFDE6:
	.long	.LEFDE6-.LASFDE6	# FDE Length
.LASFDE6:
	.long	.Lframe0	# FDE CIE offset
	.long	.LFB3	# FDE initial location
	.long	.LFE3-.LFB3	# FDE address range
	.align 4
.LEFDE6:
.LSFDE8:
	.long	.LEFDE8-.LASFDE8	# FDE Length
.LASFDE8:
	.long	.Lframe0	# FDE CIE offset
	.long	.LFB4	# FDE initial location
	.long	.LFE4-.LFB4	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI2-.LFB4
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x8
	.byte	0x85	# DW_CFA_offset, column 0x5
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI3-.LCFI2
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x5
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI4-.LCFI3
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI5-.LCFI4
	.byte	0x83	# DW_CFA_offset, column 0x3
	.uleb128 0x4
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI7-.LCFI5
	.byte	0xc3	# DW_CFA_restore, column 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI8-.LCFI7
	.byte	0xc6	# DW_CFA_restore, column 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI9-.LCFI8
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x4
	.uleb128 0x4
	.byte	0xc5	# DW_CFA_restore, column 0x5
	.align 4
.LEFDE8:
#NO_APP
#APP
	.section	.eh_frame,"aw",@progbits
.Lframe1:
	.long	.LECIE1-.LSCIE1	# Length of Common Information Entry
.LSCIE1:
	.long	0	# CIE Identifier Tag
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
.LECIE1:
.LSFDE11:
	.long	.LEFDE11-.LASFDE11	# FDE Length
.LASFDE11:
	.long	.LASFDE11-.Lframe1	# FDE CIE offset
	.long	.LFB0	# FDE initial location
	.long	.LFE0-.LFB0	# FDE address range
	.align 4
.LEFDE11:
.LSFDE13:
	.long	.LEFDE13-.LASFDE13	# FDE Length
.LASFDE13:
	.long	.LASFDE13-.Lframe1	# FDE CIE offset
	.long	.LFB1	# FDE initial location
	.long	.LFE1-.LFB1	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI0-.LFB1
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI1-.LCFI0
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x4
	.align 4
.LEFDE13:
.LSFDE15:
	.long	.LEFDE15-.LASFDE15	# FDE Length
.LASFDE15:
	.long	.LASFDE15-.Lframe1	# FDE CIE offset
	.long	.LFB2	# FDE initial location
	.long	.LFE2-.LFB2	# FDE address range
	.align 4
.LEFDE15:
.LSFDE17:
	.long	.LEFDE17-.LASFDE17	# FDE Length
.LASFDE17:
	.long	.LASFDE17-.Lframe1	# FDE CIE offset
	.long	.LFB3	# FDE initial location
	.long	.LFE3-.LFB3	# FDE address range
	.align 4
.LEFDE17:
.LSFDE19:
	.long	.LEFDE19-.LASFDE19	# FDE Length
.LASFDE19:
	.long	.LASFDE19-.Lframe1	# FDE CIE offset
	.long	.LFB4	# FDE initial location
	.long	.LFE4-.LFB4	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI2-.LFB4
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x8
	.byte	0x85	# DW_CFA_offset, column 0x5
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI3-.LCFI2
	.byte	0xd	# DW_CFA_def_cfa_register
	.uleb128 0x5
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI4-.LCFI3
	.byte	0x86	# DW_CFA_offset, column 0x6
	.uleb128 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI5-.LCFI4
	.byte	0x83	# DW_CFA_offset, column 0x3
	.uleb128 0x4
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI7-.LCFI5
	.byte	0xc3	# DW_CFA_restore, column 0x3
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI8-.LCFI7
	.byte	0xc6	# DW_CFA_restore, column 0x6
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI9-.LCFI8
	.byte	0xc	# DW_CFA_def_cfa
	.uleb128 0x4
	.uleb128 0x4
	.byte	0xc5	# DW_CFA_restore, column 0x5
	.align 4
.LEFDE19:
#NO_APP
	.text
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0x64e	# Length of Compilation Unit Info
	.value	0x2	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x4	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF5	# DW_AT_producer: "GNU C 4.7.0 20110504 (experimental)"
	.byte	0x1	# DW_AT_language
	.long	.LASF6	# DW_AT_name: "typeddwarf.c"
	.long	.LASF7	# DW_AT_comp_dir: "/usr/src/gcc/obj/gcc"
	.long	.Ldebug_ranges0+0	# DW_AT_ranges
	.long	0	# DW_AT_low_pc
	.long	0	# DW_AT_entry_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x29) DW_TAG_base_type)
	.byte	0x8	# DW_AT_byte_size
	.byte	0x4	# DW_AT_encoding
	.long	.LASF0	# DW_AT_name: "double"
	.uleb128 0x2	# (DIE (0x30) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x4	# DW_AT_encoding
	.long	.LASF1	# DW_AT_name: "float"
	.uleb128 0x2	# (DIE (0x37) DW_TAG_base_type)
	.byte	0x8	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.long	.LASF2	# DW_AT_name: "long long int"
	.uleb128 0x3	# (DIE (0x3e) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x2	# (DIE (0x45) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x7	# DW_AT_encoding
	.long	.LASF3	# DW_AT_name: "unsigned int"
	.uleb128 0x2	# (DIE (0x4c) DW_TAG_base_type)
	.byte	0x8	# DW_AT_byte_size
	.byte	0x7	# DW_AT_encoding
	.long	.LASF4	# DW_AT_name: "long long unsigned int"
	.uleb128 0x4	# (DIE (0x53) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.ascii "f1\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x9	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	.LFB0	# DW_AT_low_pc
	.long	.LFE0	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x232	# DW_AT_sibling
	.uleb128 0x5	# (DIE (0x6b) DW_TAG_formal_parameter)
	.ascii "a\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x9	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.uleb128 0x5	# (DIE (0x77) DW_TAG_formal_parameter)
	.ascii "b\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x9	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 8
	.uleb128 0x5	# (DIE (0x83) DW_TAG_formal_parameter)
	.ascii "c\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x9	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 16
	.uleb128 0x5	# (DIE (0x8f) DW_TAG_formal_parameter)
	.ascii "d\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x9	# DW_AT_decl_line
	.long	0x30	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 24
	.uleb128 0x5	# (DIE (0x9b) DW_TAG_formal_parameter)
	.ascii "e\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x9	# DW_AT_decl_line
	.long	0x30	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 28
	.uleb128 0x5	# (DIE (0xa7) DW_TAG_formal_parameter)
	.ascii "f\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x9	# DW_AT_decl_line
	.long	0x3e	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 32
	.uleb128 0x5	# (DIE (0xb3) DW_TAG_formal_parameter)
	.ascii "g\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x9	# DW_AT_decl_line
	.long	0x45	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 36
	.uleb128 0x5	# (DIE (0xbf) DW_TAG_formal_parameter)
	.ascii "h\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x9	# DW_AT_decl_line
	.long	0x37	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 40
	.uleb128 0x5	# (DIE (0xcb) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x9	# DW_AT_decl_line
	.long	0x4c	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 48
	.uleb128 0x6	# (DIE (0xd7) DW_TAG_variable)
	.ascii "j\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0xb	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x8	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 24
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x4
	.uleb128 0x30
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x29
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0xe9) DW_TAG_variable)
	.ascii "l\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0xc	# DW_AT_decl_line
	.long	0x37	# DW_AT_type
	.byte	0xa	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 24
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x4
	.uleb128 0x30
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x29
	.byte	0xf9	# DW_OP_GNU_reinterpret
	.uleb128 0x37
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0xfd) DW_TAG_variable)
	.ascii "m\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0xe	# DW_AT_decl_line
	.long	0x37	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 16
	.uleb128 0x6	# (DIE (0x109) DW_TAG_variable)
	.ascii "n\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x10	# DW_AT_decl_line
	.long	0x30	# DW_AT_type
	.byte	0xa	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 48
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x37
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x4c
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x30
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x11d) DW_TAG_variable)
	.ascii "o\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x11	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x8	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 40
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x37
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x29
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x12f) DW_TAG_variable)
	.ascii "p\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x12	# DW_AT_decl_line
	.long	0x30	# DW_AT_type
	.byte	0x8	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 36
	.byte	0x6	# DW_OP_deref
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x45
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x30
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x141) DW_TAG_variable)
	.ascii "q\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x13	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x8	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 32
	.byte	0x6	# DW_OP_deref
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x3e
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x29
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x153) DW_TAG_variable)
	.ascii "r\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x14	# DW_AT_decl_line
	.long	0x4c	# DW_AT_type
	.byte	0xa	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x4c
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x37
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x167) DW_TAG_variable)
	.ascii "s\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x15	# DW_AT_decl_line
	.long	0x37	# DW_AT_type
	.byte	0x8	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 16
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x37
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x179) DW_TAG_variable)
	.ascii "t\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x16	# DW_AT_decl_line
	.long	0x45	# DW_AT_type
	.byte	0xa	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 24
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x4
	.uleb128 0x30
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x45
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x3e
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x18d) DW_TAG_variable)
	.ascii "u\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x17	# DW_AT_decl_line
	.long	0x3e	# DW_AT_type
	.byte	0x8	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 8
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x3e
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x19f) DW_TAG_variable)
	.ascii "v\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x18	# DW_AT_decl_line
	.long	0x30	# DW_AT_type
	.byte	0x8	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x30
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x1b1) DW_TAG_variable)
	.ascii "w\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x19	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x14	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 24
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x4
	.uleb128 0x30
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x29
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x29
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x3fd00000	# fp or vector constant word 1
	.byte	0x1e	# DW_OP_mul
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x1cf) DW_TAG_variable)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x1a	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x18	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0x91	# DW_OP_fbreg
	.sleb128 8
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0x22	# DW_OP_plus
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x29
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x3ff00000	# fp or vector constant word 1
	.byte	0x22	# DW_OP_plus
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x1f1) DW_TAG_variable)
	.ascii "y\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x1b	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x18	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 8
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0x91	# DW_OP_fbreg
	.sleb128 16
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0x22	# DW_OP_plus
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x29
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x40000000	# fp or vector constant word 1
	.byte	0x22	# DW_OP_plus
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x213) DW_TAG_variable)
	.ascii "z\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x1c	# DW_AT_decl_line
	.long	0x30	# DW_AT_type
	.byte	0x14	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 24
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x4
	.uleb128 0x30
	.byte	0x91	# DW_OP_fbreg
	.sleb128 28
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x4
	.uleb128 0x30
	.byte	0x22	# DW_OP_plus
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x30
	.byte	0x4
	.long	0x40400000	# fp or vector constant word 0
	.byte	0x22	# DW_OP_plus
	.byte	0x9f	# DW_OP_stack_value
	.byte	0	# end of children of DIE 0x53
	.uleb128 0x7	# (DIE (0x232) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.ascii "f2\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x21	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	.LFB1	# DW_AT_low_pc
	.long	.LFE1	# DW_AT_high_pc
	.long	.LLST0	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x40a	# DW_AT_sibling
	.uleb128 0x5	# (DIE (0x24b) DW_TAG_formal_parameter)
	.ascii "a\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x21	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.uleb128 0x5	# (DIE (0x257) DW_TAG_formal_parameter)
	.ascii "b\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x21	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 8
	.uleb128 0x5	# (DIE (0x263) DW_TAG_formal_parameter)
	.ascii "c\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x21	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 16
	.uleb128 0x5	# (DIE (0x26f) DW_TAG_formal_parameter)
	.ascii "d\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x21	# DW_AT_decl_line
	.long	0x30	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 24
	.uleb128 0x5	# (DIE (0x27b) DW_TAG_formal_parameter)
	.ascii "e\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x21	# DW_AT_decl_line
	.long	0x30	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 28
	.uleb128 0x5	# (DIE (0x287) DW_TAG_formal_parameter)
	.ascii "f\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x21	# DW_AT_decl_line
	.long	0x3e	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 32
	.uleb128 0x5	# (DIE (0x293) DW_TAG_formal_parameter)
	.ascii "g\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x21	# DW_AT_decl_line
	.long	0x45	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 36
	.uleb128 0x5	# (DIE (0x29f) DW_TAG_formal_parameter)
	.ascii "h\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x21	# DW_AT_decl_line
	.long	0x37	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 40
	.uleb128 0x5	# (DIE (0x2ab) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x21	# DW_AT_decl_line
	.long	0x4c	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 48
	.uleb128 0x6	# (DIE (0x2b7) DW_TAG_variable)
	.ascii "j\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x23	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x8	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 24
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x4
	.uleb128 0x30
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x29
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x2c9) DW_TAG_variable)
	.ascii "l\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x24	# DW_AT_decl_line
	.long	0x37	# DW_AT_type
	.byte	0xa	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 24
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x4
	.uleb128 0x30
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x29
	.byte	0xf9	# DW_OP_GNU_reinterpret
	.uleb128 0x37
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x2dd) DW_TAG_variable)
	.ascii "m\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x26	# DW_AT_decl_line
	.long	0x37	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 16
	.uleb128 0x8	# (DIE (0x2e9) DW_TAG_variable)
	.ascii "n\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x28	# DW_AT_decl_line
	.long	0x30	# DW_AT_type
	.uleb128 0x8	# (DIE (0x2f2) DW_TAG_variable)
	.ascii "o\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x29	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.uleb128 0x6	# (DIE (0x2fb) DW_TAG_variable)
	.ascii "p\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x2a	# DW_AT_decl_line
	.long	0x30	# DW_AT_type
	.byte	0x8	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 36
	.byte	0x6	# DW_OP_deref
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x45
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x30
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x30d) DW_TAG_variable)
	.ascii "q\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x2b	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x8	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 32
	.byte	0x6	# DW_OP_deref
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x3e
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x29
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x31f) DW_TAG_variable)
	.ascii "r\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x2c	# DW_AT_decl_line
	.long	0x4c	# DW_AT_type
	.byte	0xa	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x4c
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x37
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x333) DW_TAG_variable)
	.ascii "s\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x2d	# DW_AT_decl_line
	.long	0x37	# DW_AT_type
	.byte	0x8	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 16
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x37
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x345) DW_TAG_variable)
	.ascii "t\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x2e	# DW_AT_decl_line
	.long	0x45	# DW_AT_type
	.byte	0xa	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 24
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x4
	.uleb128 0x30
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x45
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x3e
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x359) DW_TAG_variable)
	.ascii "u\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x2f	# DW_AT_decl_line
	.long	0x3e	# DW_AT_type
	.byte	0x8	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 8
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x3e
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x36b) DW_TAG_variable)
	.ascii "v\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x30	# DW_AT_decl_line
	.long	0x30	# DW_AT_type
	.byte	0x8	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x30
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x37d) DW_TAG_variable)
	.ascii "w\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x31	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x14	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 24
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x4
	.uleb128 0x30
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x29
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x29
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x3fd00000	# fp or vector constant word 1
	.byte	0x1e	# DW_OP_mul
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x39b) DW_TAG_variable)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x32	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x24	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0x91	# DW_OP_fbreg
	.sleb128 8
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0x22	# DW_OP_plus
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x29
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x40080000	# fp or vector constant word 1
	.byte	0x1c	# DW_OP_minus
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x29
	.byte	0x8
	.long	0x78b58c40	# fp or vector constant word 0
	.long	0x4415af1d	# fp or vector constant word 1
	.byte	0x22	# DW_OP_plus
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x3c9) DW_TAG_variable)
	.ascii "y\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x33	# DW_AT_decl_line
	.long	0x29	# DW_AT_type
	.byte	0x18	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 16
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x29
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x401c0000	# fp or vector constant word 1
	.byte	0x1e	# DW_OP_mul
	.byte	0x91	# DW_OP_fbreg
	.sleb128 8
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x29
	.byte	0x22	# DW_OP_plus
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x3eb) DW_TAG_variable)
	.ascii "z\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x34	# DW_AT_decl_line
	.long	0x30	# DW_AT_type
	.byte	0x14	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 24
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x4
	.uleb128 0x30
	.byte	0x91	# DW_OP_fbreg
	.sleb128 28
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x4
	.uleb128 0x30
	.byte	0x22	# DW_OP_plus
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x30
	.byte	0x4
	.long	0x40400000	# fp or vector constant word 0
	.byte	0x22	# DW_OP_plus
	.byte	0x9f	# DW_OP_stack_value
	.byte	0	# end of children of DIE 0x232
	.uleb128 0x4	# (DIE (0x40a) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.ascii "f3\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x43	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	.LFB2	# DW_AT_low_pc
	.long	.LFE2	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x4fd	# DW_AT_sibling
	.uleb128 0x5	# (DIE (0x422) DW_TAG_formal_parameter)
	.ascii "a\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x43	# DW_AT_decl_line
	.long	0x37	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.uleb128 0x5	# (DIE (0x42e) DW_TAG_formal_parameter)
	.ascii "b\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x43	# DW_AT_decl_line
	.long	0x3e	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 8
	.uleb128 0x5	# (DIE (0x43a) DW_TAG_formal_parameter)
	.ascii "c\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x43	# DW_AT_decl_line
	.long	0x37	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 12
	.uleb128 0x5	# (DIE (0x446) DW_TAG_formal_parameter)
	.ascii "d\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x43	# DW_AT_decl_line
	.long	0x45	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 20
	.uleb128 0x6	# (DIE (0x452) DW_TAG_variable)
	.ascii "w\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x45	# DW_AT_decl_line
	.long	0x37	# DW_AT_type
	.byte	0x16	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 20
	.byte	0x6	# DW_OP_deref
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x45
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x37
	.byte	0x12	# DW_OP_dup
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x37
	.byte	0x16	# DW_OP_swap
	.byte	0x14	# DW_OP_over
	.byte	0x2b	# DW_OP_gt
	.byte	0x28	# DW_OP_bra
	.value	0x1
	.byte	0x16	# DW_OP_swap
	.byte	0x13	# DW_OP_drop
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x472) DW_TAG_variable)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x46	# DW_AT_decl_line
	.long	0x37	# DW_AT_type
	.byte	0x1a	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 8
	.byte	0x6	# DW_OP_deref
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x3e
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x37
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x37
	.byte	0x22	# DW_OP_plus
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x37
	.byte	0x8
	.quad	0x7
	.byte	0x22	# DW_OP_plus
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x496) DW_TAG_variable)
	.ascii "y\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x47	# DW_AT_decl_line
	.long	0x37	# DW_AT_type
	.byte	0x1a	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 20
	.byte	0x6	# DW_OP_deref
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x45
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x37
	.byte	0x91	# DW_OP_fbreg
	.sleb128 12
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x37
	.byte	0x22	# DW_OP_plus
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x37
	.byte	0x8
	.quad	0x912345678
	.byte	0x22	# DW_OP_plus
	.byte	0x9f	# DW_OP_stack_value
	.uleb128 0x6	# (DIE (0x4ba) DW_TAG_variable)
	.ascii "z\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x48	# DW_AT_decl_line
	.long	0x3e	# DW_AT_type
	.byte	0x38	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 8
	.byte	0x6	# DW_OP_deref
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x3e
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x37
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x37
	.byte	0x22	# DW_OP_plus
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x37
	.byte	0x8
	.quad	0x7
	.byte	0x22	# DW_OP_plus
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x3e
	.byte	0x91	# DW_OP_fbreg
	.sleb128 20
	.byte	0x6	# DW_OP_deref
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x45
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x37
	.byte	0x91	# DW_OP_fbreg
	.sleb128 12
	.byte	0xf6	# DW_OP_GNU_deref_type
	.byte	0x8
	.uleb128 0x37
	.byte	0x22	# DW_OP_plus
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x37
	.byte	0x8
	.quad	0x912345678
	.byte	0x22	# DW_OP_plus
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x3e
	.byte	0x22	# DW_OP_plus
	.byte	0x9f	# DW_OP_stack_value
	.byte	0	# end of children of DIE 0x40a
	.uleb128 0x4	# (DIE (0x4fd) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.ascii "f4\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x4d	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	.LFB3	# DW_AT_low_pc
	.long	.LFE3	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x555	# DW_AT_sibling
	.uleb128 0x5	# (DIE (0x515) DW_TAG_formal_parameter)
	.ascii "a\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x4d	# DW_AT_decl_line
	.long	0x555	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.uleb128 0x5	# (DIE (0x521) DW_TAG_formal_parameter)
	.ascii "b\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x4d	# DW_AT_decl_line
	.long	0x55c	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 4
	.uleb128 0x5	# (DIE (0x52d) DW_TAG_formal_parameter)
	.ascii "c\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x4d	# DW_AT_decl_line
	.long	0x563	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 16
	.uleb128 0x8	# (DIE (0x539) DW_TAG_variable)
	.ascii "w\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x4f	# DW_AT_decl_line
	.long	0x555	# DW_AT_type
	.uleb128 0x8	# (DIE (0x542) DW_TAG_variable)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x50	# DW_AT_decl_line
	.long	0x55c	# DW_AT_type
	.uleb128 0x8	# (DIE (0x54b) DW_TAG_variable)
	.ascii "y\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x51	# DW_AT_decl_line
	.long	0x563	# DW_AT_type
	.byte	0	# end of children of DIE 0x4fd
	.uleb128 0x2	# (DIE (0x555) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0xf	# DW_AT_encoding
	.long	.LASF8	# DW_AT_name: "_Decimal32"
	.uleb128 0x2	# (DIE (0x55c) DW_TAG_base_type)
	.byte	0x8	# DW_AT_byte_size
	.byte	0xf	# DW_AT_encoding
	.long	.LASF9	# DW_AT_name: "_Decimal64"
	.uleb128 0x2	# (DIE (0x563) DW_TAG_base_type)
	.byte	0x10	# DW_AT_byte_size
	.byte	0xf	# DW_AT_encoding
	.long	.LASF10	# DW_AT_name: "_Decimal128"
	.uleb128 0x9	# (DIE (0x56a) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF11	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x56	# DW_AT_decl_line
	.long	0x3e	# DW_AT_type
	.long	.LFB4	# DW_AT_low_pc
	.long	.LFE4	# DW_AT_high_pc
	.long	.LLST1	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x62f	# DW_AT_sibling
	.uleb128 0xa	# (DIE (0x587) DW_TAG_GNU_call_site)
	.long	.LVL5	# DW_AT_low_pc
	.long	0x53	# DW_AT_abstract_origin
	.long	0x5e1	# DW_AT_sibling
	.uleb128 0xb	# (DIE (0x594) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x74	# DW_OP_breg4
	.sleb128 0
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x29
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x3ff00000	# fp or vector constant word 1
	.uleb128 0xb	# (DIE (0x5a4) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x29
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x40000000	# fp or vector constant word 1
	.uleb128 0xb	# (DIE (0x5b4) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x74	# DW_OP_breg4
	.sleb128 16
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x29
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0x40080000	# fp or vector constant word 1
	.uleb128 0xb	# (DIE (0x5c4) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x74	# DW_OP_breg4
	.sleb128 24
	.byte	0x3	# DW_AT_GNU_call_site_value
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x3
	.uleb128 0x30
	.uleb128 0xb	# (DIE (0x5cc) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x74	# DW_OP_breg4
	.sleb128 28
	.byte	0x3	# DW_AT_GNU_call_site_value
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x6
	.uleb128 0x30
	.uleb128 0xb	# (DIE (0x5d4) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x74	# DW_OP_breg4
	.sleb128 32
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x36	# DW_OP_lit6
	.uleb128 0xb	# (DIE (0x5da) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x74	# DW_OP_breg4
	.sleb128 36
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x37	# DW_OP_lit7
	.byte	0	# end of children of DIE 0x587
	.uleb128 0xa	# (DIE (0x5e1) DW_TAG_GNU_call_site)
	.long	.LVL6	# DW_AT_low_pc
	.long	0x232	# DW_AT_abstract_origin
	.long	0x60b	# DW_AT_sibling
	.uleb128 0xb	# (DIE (0x5ee) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x74	# DW_OP_breg4
	.sleb128 24
	.byte	0x3	# DW_AT_GNU_call_site_value
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x3
	.uleb128 0x30
	.uleb128 0xb	# (DIE (0x5f6) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x74	# DW_OP_breg4
	.sleb128 28
	.byte	0x3	# DW_AT_GNU_call_site_value
	.byte	0xf5	# DW_OP_GNU_regval_type
	.uleb128 0x6
	.uleb128 0x30
	.uleb128 0xb	# (DIE (0x5fe) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x74	# DW_OP_breg4
	.sleb128 32
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x36	# DW_OP_lit6
	.uleb128 0xb	# (DIE (0x604) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x74	# DW_OP_breg4
	.sleb128 36
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x37	# DW_OP_lit7
	.byte	0	# end of children of DIE 0x5e1
	.uleb128 0xa	# (DIE (0x60b) DW_TAG_GNU_call_site)
	.long	.LVL7	# DW_AT_low_pc
	.long	0x40a	# DW_AT_abstract_origin
	.long	0x625	# DW_AT_sibling
	.uleb128 0xb	# (DIE (0x618) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x32	# DW_OP_lit2
	.uleb128 0xb	# (DIE (0x61e) DW_TAG_GNU_call_site_parameter)
	.byte	0x2	# DW_AT_location
	.byte	0x74	# DW_OP_breg4
	.sleb128 20
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x34	# DW_OP_lit4
	.byte	0	# end of children of DIE 0x60b
	.uleb128 0xc	# (DIE (0x625) DW_TAG_GNU_call_site)
	.long	.LVL8	# DW_AT_low_pc
	.long	0x4fd	# DW_AT_abstract_origin
	.byte	0	# end of children of DIE 0x56a
	.uleb128 0xd	# (DIE (0x62f) DW_TAG_variable)
	.ascii "vv\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x5	# DW_AT_decl_line
	.long	0x63b	# DW_AT_type
	.byte	0x1	# DW_AT_external
	.byte	0x1	# DW_AT_declaration
	.uleb128 0xe	# (DIE (0x63b) DW_TAG_volatile_type)
	.long	0x3e	# DW_AT_type
	.uleb128 0xf	# (DIE (0x640) DW_TAG_variable)
	.ascii "vv\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (typeddwarf.c)
	.byte	0x5	# DW_AT_decl_line
	.long	0x63b	# DW_AT_type
	.byte	0x1	# DW_AT_external
	.byte	0x5	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.long	vv
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
	.uleb128 0x55	# (DW_AT_ranges)
	.uleb128 0x6	# (DW_FORM_data4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x52	# (DW_AT_entry_pc)
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
	.uleb128 0xa	# (DW_FORM_block1)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x5	# (abbrev code)
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
	.uleb128 0x6	# (abbrev code)
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
	.uleb128 0x7	# (abbrev code)
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
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x8	# (abbrev code)
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
	.byte	0
	.byte	0
	.uleb128 0x9	# (abbrev code)
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
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xa	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0x1	# DW_children_yes
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xb	# (abbrev code)
	.uleb128 0x410a	# (TAG: DW_TAG_GNU_call_site_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.uleb128 0x2111	# (DW_AT_GNU_call_site_value)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.uleb128 0xc	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0	# DW_children_no
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
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
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x3c	# (DW_AT_declaration)
	.uleb128 0xc	# (DW_FORM_flag)
	.byte	0
	.byte	0
	.uleb128 0xe	# (abbrev code)
	.uleb128 0x35	# (TAG: DW_TAG_volatile_type)
	.byte	0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xf	# (abbrev code)
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
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST0:
	.long	.LFB1	# Location list begin address (*.LLST0)
	.long	.LCFI0	# Location list end address (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	.LCFI0	# Location list begin address (*.LLST0)
	.long	.LCFI1	# Location list end address (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 16
	.long	.LCFI1	# Location list begin address (*.LLST0)
	.long	.LFE1	# Location list end address (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	0	# Location list terminator begin (*.LLST0)
	.long	0	# Location list terminator end (*.LLST0)
.LLST1:
	.long	.LFB4	# Location list begin address (*.LLST1)
	.long	.LCFI2	# Location list end address (*.LLST1)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	.LCFI2	# Location list begin address (*.LLST1)
	.long	.LCFI3	# Location list end address (*.LLST1)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 8
	.long	.LCFI3	# Location list begin address (*.LLST1)
	.long	.LCFI9	# Location list end address (*.LLST1)
	.value	0x2	# Location expression size
	.byte	0x75	# DW_OP_breg5
	.sleb128 8
	.long	.LCFI9	# Location list begin address (*.LLST1)
	.long	.LFE4	# Location list end address (*.LLST1)
	.value	0x2	# Location expression size
	.byte	0x74	# DW_OP_breg4
	.sleb128 4
	.long	0	# Location list terminator begin (*.LLST1)
	.long	0	# Location list terminator end (*.LLST1)
	.section	.debug_aranges,"",@progbits
	.long	0x24	# Length of Address Ranges Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.byte	0x4	# Size of Address
	.byte	0	# Size of Segment Descriptor
	.value	0	# Pad to 8 byte boundary
	.value	0
	.long	.Ltext0	# Address
	.long	.Letext0-.Ltext0	# Length
	.long	.LFB4	# Address
	.long	.LFE4-.LFB4	# Length
	.long	0
	.long	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.long	.Ltext0	# Offset 0
	.long	.Letext0
	.long	.LFB4	# Offset 0x8
	.long	.LFE4
	.long	0
	.long	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.long	.LELT0-.LSLT0	# Length of Source Line Info
.LSLT0:
	.value	0x2	# DWARF Version
	.long	.LELTP0-.LASLTP0	# Prolog Length
.LASLTP0:
	.byte	0x1	# Minimum Instruction Length
	.byte	0x1	# Default is_stmt_start flag
	.byte	0xf6	# Line Base Value (Special Opcodes)
	.byte	0xf2	# Line Range Value (Special Opcodes)
	.byte	0xd	# Special Opcode Base
	.byte	0	# opcode: 0x1 has 0 args
	.byte	0x1	# opcode: 0x2 has 1 args
	.byte	0x1	# opcode: 0x3 has 1 args
	.byte	0x1	# opcode: 0x4 has 1 args
	.byte	0x1	# opcode: 0x5 has 1 args
	.byte	0	# opcode: 0x6 has 0 args
	.byte	0	# opcode: 0x7 has 0 args
	.byte	0	# opcode: 0x8 has 0 args
	.byte	0x1	# opcode: 0x9 has 1 args
	.byte	0	# opcode: 0xa has 0 args
	.byte	0	# opcode: 0xb has 0 args
	.byte	0x1	# opcode: 0xc has 1 args
	.byte	0	# End directory table
	.ascii "typeddwarf.c\0"	# File Entry: 0x1
	.uleb128 0
	.uleb128 0
	.uleb128 0
	.byte	0	# End file name table
.LELTP0:
	.byte	0	# set address *.LM37
	.uleb128 0x5
	.byte	0x2
	.long	.LM37
	.byte	0x6d	# line 87
	.byte	0	# set address *.LM38
	.uleb128 0x5
	.byte	0x2
	.long	.LM38
	.byte	0x18	# line 88
	.byte	0	# set address *.LM39
	.uleb128 0x5
	.byte	0x2
	.long	.LM39
	.byte	0x16	# line 87
	.byte	0	# set address *.LM40
	.uleb128 0x5
	.byte	0x2
	.long	.LM40
	.byte	0x18	# line 88
	.byte	0	# set address *.LM41
	.uleb128 0x5
	.byte	0x2
	.long	.LM41
	.byte	0x16	# line 87
	.byte	0	# set address *.LM42
	.uleb128 0x5
	.byte	0x2
	.long	.LM42
	.byte	0x18	# line 88
	.byte	0	# set address *.LM43
	.uleb128 0x5
	.byte	0x2
	.long	.LM43
	.byte	0x18	# line 89
	.byte	0	# set address *.LM44
	.uleb128 0x5
	.byte	0x2
	.long	.LM44
	.byte	0x18	# line 90
	.byte	0	# set address *.LM45
	.uleb128 0x5
	.byte	0x2
	.long	.LM45
	.byte	0x18	# line 91
	.byte	0	# set address *.LM46
	.uleb128 0x5
	.byte	0x2
	.long	.LM46
	.byte	0x19	# line 93
	.byte	0	# set address *.LFE4
	.uleb128 0x5
	.byte	0x2
	.long	.LFE4
	.byte	0	# end sequence
	.uleb128 0x1
	.byte	0x1
	.byte	0	# set address *.LM1
	.uleb128 0x5
	.byte	0x2
	.long	.LM1
	.byte	0x20	# line 10
	.byte	0	# set address *.LM2
	.uleb128 0x5
	.byte	0x2
	.long	.LM2
	.byte	0x2a	# line 29
	.byte	0	# set address *.LM3
	.uleb128 0x5
	.byte	0x2
	.long	.LM3
	.byte	0x18	# line 30
	.byte	0	# set address *.LM4
	.uleb128 0x5
	.byte	0x2
	.long	.LM4
	.byte	0x1b	# line 34
	.byte	0	# set address *.LM5
	.uleb128 0x5
	.byte	0x2
	.long	.LM5
	.byte	0x2a	# line 53
	.byte	0	# set address *.LM6
	.uleb128 0x5
	.byte	0x2
	.long	.LM6
	.byte	0x18	# line 54
	.byte	0	# set address *.LM7
	.uleb128 0x5
	.byte	0x2
	.long	.LM7
	.byte	0x3	# advance to line 34
	.sleb128 -20
	.byte	0x1
	.byte	0	# set address *.LM8
	.uleb128 0x5
	.byte	0x2
	.long	.LM8
	.byte	0x2b	# line 54
	.byte	0	# set address *.LM9
	.uleb128 0x5
	.byte	0x2
	.long	.LM9
	.byte	0x16	# line 53
	.byte	0	# set address *.LM10
	.uleb128 0x5
	.byte	0x2
	.long	.LM10
	.byte	0x18	# line 54
	.byte	0	# set address *.LM11
	.uleb128 0x5
	.byte	0x2
	.long	.LM11
	.byte	0x18	# line 55
	.byte	0	# set address *.LM12
	.uleb128 0x5
	.byte	0x2
	.long	.LM12
	.byte	0x16	# line 54
	.byte	0	# set address *.LM13
	.uleb128 0x5
	.byte	0x2
	.long	.LM13
	.byte	0x18	# line 55
	.byte	0	# set address *.LM14
	.uleb128 0x5
	.byte	0x2
	.long	.LM14
	.byte	0x18	# line 56
	.byte	0	# set address *.LM15
	.uleb128 0x5
	.byte	0x2
	.long	.LM15
	.byte	0x16	# line 55
	.byte	0	# set address *.LM16
	.uleb128 0x5
	.byte	0x2
	.long	.LM16
	.byte	0x18	# line 56
	.byte	0	# set address *.LM17
	.uleb128 0x5
	.byte	0x2
	.long	.LM17
	.byte	0x18	# line 57
	.byte	0	# set address *.LM18
	.uleb128 0x5
	.byte	0x2
	.long	.LM18
	.byte	0x16	# line 56
	.byte	0	# set address *.LM19
	.uleb128 0x5
	.byte	0x2
	.long	.LM19
	.byte	0x18	# line 57
	.byte	0	# set address *.LM20
	.uleb128 0x5
	.byte	0x2
	.long	.LM20
	.byte	0x18	# line 58
	.byte	0	# set address *.LM21
	.uleb128 0x5
	.byte	0x2
	.long	.LM21
	.byte	0x16	# line 57
	.byte	0	# set address *.LM22
	.uleb128 0x5
	.byte	0x2
	.long	.LM22
	.byte	0x18	# line 58
	.byte	0	# set address *.LM23
	.uleb128 0x5
	.byte	0x2
	.long	.LM23
	.byte	0x18	# line 59
	.byte	0	# set address *.LM24
	.uleb128 0x5
	.byte	0x2
	.long	.LM24
	.byte	0x1b	# line 63
	.byte	0	# set address *.LM25
	.uleb128 0x5
	.byte	0x2
	.long	.LM25
	.byte	0x13	# line 59
	.byte	0	# set address *.LM26
	.uleb128 0x5
	.byte	0x2
	.long	.LM26
	.byte	0x18	# line 60
	.byte	0	# set address *.LM27
	.uleb128 0x5
	.byte	0x2
	.long	.LM27
	.byte	0x18	# line 61
	.byte	0	# set address *.LM28
	.uleb128 0x5
	.byte	0x2
	.long	.LM28
	.byte	0x18	# line 62
	.byte	0	# set address *.LM29
	.uleb128 0x5
	.byte	0x2
	.long	.LM29
	.byte	0x18	# line 63
	.byte	0	# set address *.LM30
	.uleb128 0x5
	.byte	0x2
	.long	.LM30
	.byte	0x18	# line 64
	.byte	0	# set address *.LM31
	.uleb128 0x5
	.byte	0x2
	.long	.LM31
	.byte	0x1b	# line 68
	.byte	0	# set address *.LM32
	.uleb128 0x5
	.byte	0x2
	.long	.LM32
	.byte	0x1c	# line 73
	.byte	0	# set address *.LM33
	.uleb128 0x5
	.byte	0x2
	.long	.LM33
	.byte	0x18	# line 74
	.byte	0	# set address *.LM34
	.uleb128 0x5
	.byte	0x2
	.long	.LM34
	.byte	0x1b	# line 78
	.byte	0	# set address *.LM35
	.uleb128 0x5
	.byte	0x2
	.long	.LM35
	.byte	0x1b	# line 82
	.byte	0	# set address *.LM36
	.uleb128 0x5
	.byte	0x2
	.long	.LM36
	.byte	0x18	# line 83
	.byte	0	# set address *.Letext0
	.uleb128 0x5
	.byte	0x2
	.long	.Letext0
	.byte	0	# end sequence
	.uleb128 0x1
	.byte	0x1
.LELT0:
	.section	.debug_str,"MS",@progbits,1
.LASF2:
	.string	"long long int"
.LASF3:
	.string	"unsigned int"
.LASF8:
	.string	"_Decimal32"
.LASF4:
	.string	"long long unsigned int"
.LASF11:
	.string	"main"
.LASF9:
	.string	"_Decimal64"
.LASF0:
	.string	"double"
.LASF10:
	.string	"_Decimal128"
.LASF7:
	.string	"/usr/src/gcc/obj/gcc"
.LASF1:
	.string	"float"
.LASF6:
	.string	"typeddwarf.c"
.LASF5:
	.string	"GNU C 4.7.0 20110504 (experimental)"
	.ident	"GCC: (GNU) 4.7.0 20110504 (experimental)"
	.section	.note.GNU-stack,"",@progbits
