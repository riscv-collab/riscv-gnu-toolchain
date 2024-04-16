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

/* This source file was generated from clztest.c using the following
   command line:

   gcc -dA -S -g -O2 clztest.c -o clztest.S

*/

	.file	"clztest.c"
	.text
.Ltext0:
	.p2align 4,,15
	.globl	foo
	.type	foo, @function
foo:
.LFB0:
	# clztest.c:8
.LM1:
.LVL0:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# clztest.c:9
.LM2:
	bsrq	%rdi, %rax
	xorl	$63, %eax
	cltq
.LVL1:
	# clztest.c:11
.LM3:
.LVL2:
	# clztest.c:12
.LM4:
	movl	vv(%rip), %edx
	addl	$1, %edx
	movl	%edx, vv(%rip)
# SUCC: EXIT [100.0%] 
	# clztest.c:14
.LM5:
	ret
.LFE0:
	.size	foo, .-foo
	.p2align 4,,15
	.globl	bar
	.type	bar, @function
bar:
.LFB1:
	# clztest.c:18
.LM6:
.LVL3:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# clztest.c:19
.LM7:
	bsrq	%rdi, %rax
	xorl	$63, %eax
	cltq
.LVL4:
	# clztest.c:21
.LM8:
.LVL5:
	# clztest.c:22
.LM9:
	movl	vv(%rip), %edx
	addl	$1, %edx
	movl	%edx, vv(%rip)
# SUCC: EXIT [100.0%] 
	# clztest.c:24
.LM10:
	ret
.LFE1:
	.size	bar, .-bar
	.section	.text.startup,"ax",@progbits
	.p2align 4,,15
	.globl	_start
	.type	_start, @function
_start:
.LFB2:
	# clztest.c:28
.LM11:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	pushq	%rbx
.LCFI0:
	# clztest.c:29
.LM12:
	movl	vv(%rip), %ebx
.LVL6:
	# clztest.c:30
.LM13:
	movslq	%ebx, %rbx
.LVL7:
	leaq	1193046(%rbx), %rdi
	call	foo
.LVL8:
	# clztest.c:31
.LM14:
	leaq	2147483647(%rbx), %rdi
	call	bar
.LVL9:
	# clztest.c:33
.LM15:
	xorl	%eax, %eax
	popq	%rbx
.LCFI1:
.LVL10:
# SUCC: EXIT [100.0%] 
	ret
.LFE2:
	.size	_start, .-_start
	.comm	vv,4,4
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
	.align 8
.LEFDE0:
.LSFDE2:
	.long	.LEFDE2-.LASFDE2	# FDE Length
.LASFDE2:
	.long	.Lframe0	# FDE CIE offset
	.quad	.LFB1	# FDE initial location
	.quad	.LFE1-.LFB1	# FDE address range
	.align 8
.LEFDE2:
.LSFDE4:
	.long	.LEFDE4-.LASFDE4	# FDE Length
.LASFDE4:
	.long	.Lframe0	# FDE CIE offset
	.quad	.LFB2	# FDE initial location
	.quad	.LFE2-.LFB2	# FDE address range
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI0-.LFB2
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x83	# DW_CFA_offset, column 0x3
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI1-.LCFI0
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x8
	.align 8
.LEFDE4:
#NO_APP
#APP
	.section	.eh_frame,"aw",@progbits
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
.LSFDE7:
	.long	.LEFDE7-.LASFDE7	# FDE Length
.LASFDE7:
	.long	.LASFDE7-.Lframe1	# FDE CIE offset
	.long	.LFB0	# FDE initial location
	.long	.LFE0-.LFB0	# FDE address range
	.uleb128 0	# Augmentation size
	.align 8
.LEFDE7:
.LSFDE9:
	.long	.LEFDE9-.LASFDE9	# FDE Length
.LASFDE9:
	.long	.LASFDE9-.Lframe1	# FDE CIE offset
	.long	.LFB1	# FDE initial location
	.long	.LFE1-.LFB1	# FDE address range
	.uleb128 0	# Augmentation size
	.align 8
.LEFDE9:
.LSFDE11:
	.long	.LEFDE11-.LASFDE11	# FDE Length
.LASFDE11:
	.long	.LASFDE11-.Lframe1	# FDE CIE offset
	.long	.LFB2	# FDE initial location
	.long	.LFE2-.LFB2	# FDE address range
	.uleb128 0	# Augmentation size
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI0-.LFB2
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x10
	.byte	0x83	# DW_CFA_offset, column 0x3
	.uleb128 0x2
	.byte	0x4	# DW_CFA_advance_loc4
	.long	.LCFI1-.LCFI0
	.byte	0xe	# DW_CFA_def_cfa_offset
	.uleb128 0x8
	.align 8
.LEFDE11:
#NO_APP
	.text
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0x15a	# Length of Compilation Unit Info
	.value	0x2	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF1	# DW_AT_producer: "GNU C 4.7.0 20110511 (experimental)"
	.byte	0x1	# DW_AT_language
	.long	.LASF2	# DW_AT_name: "clztest.c"
	.long	.LASF3	# DW_AT_comp_dir: "/usr/src/gcc/obj/gcc"
	.long	.Ldebug_ranges0+0	# DW_AT_ranges
	.quad	0	# DW_AT_low_pc
	.quad	0	# DW_AT_entry_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x31) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.ascii "foo\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (clztest.c)
	.byte	0x7	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x7a	# DW_AT_type
	.quad	.LFB0	# DW_AT_low_pc
	.quad	.LFE0	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x7a	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x56) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (clztest.c)
	.byte	0x7	# DW_AT_decl_line
	.long	0x7a	# DW_AT_type
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.uleb128 0x4	# (DIE (0x61) DW_TAG_variable)
	.ascii "f\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (clztest.c)
	.byte	0x9	# DW_AT_decl_line
	.long	0x7a	# DW_AT_type
	.byte	0x1	# DW_AT_location
	.byte	0x50	# DW_OP_reg0
	.uleb128 0x5	# (DIE (0x6c) DW_TAG_variable)
	.ascii "g\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (clztest.c)
	.byte	0xa	# DW_AT_decl_line
	.long	0x7a	# DW_AT_type
	.long	.LLST0	# DW_AT_location
	.byte	0	# end of children of DIE 0x31
	.uleb128 0x6	# (DIE (0x7a) DW_TAG_base_type)
	.byte	0x8	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.long	.LASF0	# DW_AT_name: "long int"
	.uleb128 0x2	# (DIE (0x81) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.ascii "bar\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (clztest.c)
	.byte	0x11	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x7a	# DW_AT_type
	.quad	.LFB1	# DW_AT_low_pc
	.quad	.LFE1	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0xca	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0xa6) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (clztest.c)
	.byte	0x11	# DW_AT_decl_line
	.long	0x7a	# DW_AT_type
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.uleb128 0x4	# (DIE (0xb1) DW_TAG_variable)
	.ascii "f\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (clztest.c)
	.byte	0x13	# DW_AT_decl_line
	.long	0x7a	# DW_AT_type
	.byte	0x1	# DW_AT_location
	.byte	0x50	# DW_OP_reg0
	.uleb128 0x5	# (DIE (0xbc) DW_TAG_variable)
	.ascii "g\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (clztest.c)
	.byte	0x14	# DW_AT_decl_line
	.long	0x7a	# DW_AT_type
	.long	.LLST1	# DW_AT_location
	.byte	0	# end of children of DIE 0x81
	.uleb128 0x7	# (DIE (0xca) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF4	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (clztest.c)
	.byte	0x1b	# DW_AT_decl_line
	.long	0x130	# DW_AT_type
	.quad	.LFB2	# DW_AT_low_pc
	.quad	.LFE2	# DW_AT_high_pc
	.long	.LLST2	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x130	# DW_AT_sibling
	.uleb128 0x5	# (DIE (0xef) DW_TAG_variable)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (clztest.c)
	.byte	0x1d	# DW_AT_decl_line
	.long	0x7a	# DW_AT_type
	.long	.LLST3	# DW_AT_location
	.uleb128 0x8	# (DIE (0xfc) DW_TAG_GNU_call_site)
	.quad	.LVL8	# DW_AT_low_pc
	.long	0x31	# DW_AT_abstract_origin
	.long	0x117	# DW_AT_sibling
	.uleb128 0x9	# (DIE (0x10d) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x5	# DW_AT_GNU_call_site_value
	.byte	0x73	# DW_OP_breg3
	.sleb128 1193046
	.byte	0	# end of children of DIE 0xfc
	.uleb128 0xa	# (DIE (0x117) DW_TAG_GNU_call_site)
	.quad	.LVL9	# DW_AT_low_pc
	.long	0x81	# DW_AT_abstract_origin
	.uleb128 0x9	# (DIE (0x124) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x6	# DW_AT_GNU_call_site_value
	.byte	0x73	# DW_OP_breg3
	.sleb128 2147483647
	.byte	0	# end of children of DIE 0x117
	.byte	0	# end of children of DIE 0xca
	.uleb128 0xb	# (DIE (0x130) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0xc	# (DIE (0x137) DW_TAG_variable)
	.ascii "vv\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (clztest.c)
	.byte	0x4	# DW_AT_decl_line
	.long	0x143	# DW_AT_type
	.byte	0x1	# DW_AT_external
	.byte	0x1	# DW_AT_declaration
	.uleb128 0xd	# (DIE (0x143) DW_TAG_volatile_type)
	.long	0x130	# DW_AT_type
	.uleb128 0xe	# (DIE (0x148) DW_TAG_variable)
	.ascii "vv\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (clztest.c)
	.byte	0x4	# DW_AT_decl_line
	.long	0x143	# DW_AT_type
	.byte	0x1	# DW_AT_external
	.byte	0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	vv
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
	.uleb128 0xa	# (DW_FORM_block1)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x3	# (abbrev code)
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
	.uleb128 0x4	# (abbrev code)
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
	.uleb128 0x5	# (abbrev code)
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
	.uleb128 0x6	# (abbrev code)
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
	.uleb128 0x8	# (abbrev code)
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
	.uleb128 0x9	# (abbrev code)
	.uleb128 0x410a	# (TAG: DW_TAG_GNU_call_site_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.uleb128 0x2111	# (DW_AT_GNU_call_site_value)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.uleb128 0xa	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0x1	# DW_children_yes
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xb	# (abbrev code)
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
	.uleb128 0xc	# (abbrev code)
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
	.uleb128 0xd	# (abbrev code)
	.uleb128 0x35	# (TAG: DW_TAG_volatile_type)
	.byte	0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xe	# (abbrev code)
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
	.quad	.LVL1	# Location list begin address (*.LLST0)
	.quad	.LVL2	# Location list end address (*.LLST0)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	.LVL2	# Location list begin address (*.LLST0)
	.quad	.LFE0	# Location list end address (*.LLST0)
	.value	0x36	# Location expression size
	.byte	0x8	# DW_OP_const1u
	.byte	0x3f
	.byte	0x75	# DW_OP_breg5
	.sleb128 0
	.byte	0x12	# DW_OP_dup
	.byte	0x28	# DW_OP_bra
	.value	0x6
	.byte	0x13	# DW_OP_drop
	.byte	0x8	# DW_OP_const1u
	.byte	0x40
	.byte	0x2f	# DW_OP_skip
	.value	0x1c
	.byte	0x30	# DW_OP_lit0
	.byte	0x16	# DW_OP_swap
	.byte	0x12	# DW_OP_dup
	.byte	0x10	# DW_OP_constu
	.uleb128 0x8000000000000000
	.byte	0x1a	# DW_OP_and
	.byte	0x28	# DW_OP_bra
	.value	0x9
	.byte	0x31	# DW_OP_lit1
	.byte	0x24	# DW_OP_shl
	.byte	0x16	# DW_OP_swap
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x1
	.byte	0x16	# DW_OP_swap
	.byte	0x2f	# DW_OP_skip
	.value	0xffe7
	.byte	0x13	# DW_OP_drop
	.byte	0x96	# DW_OP_nop
	.byte	0x1c	# DW_OP_minus
	.byte	0x8	# DW_OP_const1u
	.byte	0x3f
	.byte	0x27	# DW_OP_xor
	.byte	0x8	# DW_OP_const1u
	.byte	0x20
	.byte	0x24	# DW_OP_shl
	.byte	0x8	# DW_OP_const1u
	.byte	0x20
	.byte	0x26	# DW_OP_shra
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST0)
	.quad	0	# Location list terminator end (*.LLST0)
.LLST1:
	.quad	.LVL4	# Location list begin address (*.LLST1)
	.quad	.LVL5	# Location list end address (*.LLST1)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	.LVL5	# Location list begin address (*.LLST1)
	.quad	.LFE1	# Location list end address (*.LLST1)
	.value	0x36	# Location expression size
	.byte	0x8	# DW_OP_const1u
	.byte	0x3f
	.byte	0x75	# DW_OP_breg5
	.sleb128 0
	.byte	0x12	# DW_OP_dup
	.byte	0x28	# DW_OP_bra
	.value	0x6
	.byte	0x13	# DW_OP_drop
	.byte	0x8	# DW_OP_const1u
	.byte	0x40
	.byte	0x2f	# DW_OP_skip
	.value	0x1c
	.byte	0x30	# DW_OP_lit0
	.byte	0x16	# DW_OP_swap
	.byte	0x12	# DW_OP_dup
	.byte	0x10	# DW_OP_constu
	.uleb128 0x8000000000000000
	.byte	0x1a	# DW_OP_and
	.byte	0x28	# DW_OP_bra
	.value	0x9
	.byte	0x31	# DW_OP_lit1
	.byte	0x24	# DW_OP_shl
	.byte	0x16	# DW_OP_swap
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x1
	.byte	0x16	# DW_OP_swap
	.byte	0x2f	# DW_OP_skip
	.value	0xffe7
	.byte	0x13	# DW_OP_drop
	.byte	0x96	# DW_OP_nop
	.byte	0x1c	# DW_OP_minus
	.byte	0x8	# DW_OP_const1u
	.byte	0x3f
	.byte	0x27	# DW_OP_xor
	.byte	0x8	# DW_OP_const1u
	.byte	0x20
	.byte	0x24	# DW_OP_shl
	.byte	0x8	# DW_OP_const1u
	.byte	0x20
	.byte	0x26	# DW_OP_shra
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST1)
	.quad	0	# Location list terminator end (*.LLST1)
.LLST2:
	.quad	.LFB2	# Location list begin address (*.LLST2)
	.quad	.LCFI0	# Location list end address (*.LLST2)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI0	# Location list begin address (*.LLST2)
	.quad	.LCFI1	# Location list end address (*.LLST2)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI1	# Location list begin address (*.LLST2)
	.quad	.LFE2	# Location list end address (*.LLST2)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	0	# Location list terminator begin (*.LLST2)
	.quad	0	# Location list terminator end (*.LLST2)
.LLST3:
	.quad	.LVL6	# Location list begin address (*.LLST3)
	.quad	.LVL7	# Location list end address (*.LLST3)
	.value	0x9	# Location expression size
	.byte	0x73	# DW_OP_breg3
	.sleb128 0
	.byte	0x8	# DW_OP_const1u
	.byte	0x20
	.byte	0x24	# DW_OP_shl
	.byte	0x8	# DW_OP_const1u
	.byte	0x20
	.byte	0x26	# DW_OP_shra
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL7	# Location list begin address (*.LLST3)
	.quad	.LVL10	# Location list end address (*.LLST3)
	.value	0x1	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.quad	0	# Location list terminator begin (*.LLST3)
	.quad	0	# Location list terminator end (*.LLST3)
	.section	.debug_aranges,"",@progbits
	.long	0x3c	# Length of Address Ranges Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.byte	0x8	# Size of Address
	.byte	0	# Size of Segment Descriptor
	.value	0	# Pad to 16 byte boundary
	.value	0
	.quad	.Ltext0	# Address
	.quad	.Letext0-.Ltext0	# Length
	.quad	.LFB2	# Address
	.quad	.LFE2-.LFB2	# Length
	.quad	0
	.quad	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.quad	.Ltext0	# Offset 0
	.quad	.Letext0
	.quad	.LFB2	# Offset 0x10
	.quad	.LFE2
	.quad	0
	.quad	0
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
	.ascii "clztest.c\0"	# File Entry: 0x1
	.uleb128 0
	.uleb128 0
	.uleb128 0
	.byte	0	# End file name table
.LELTP0:
	.byte	0	# set address *.LM11
	.uleb128 0x9
	.byte	0x2
	.quad	.LM11
	.byte	0x32	# line 28
	.byte	0	# set address *.LM12
	.uleb128 0x9
	.byte	0x2
	.quad	.LM12
	.byte	0x18	# line 29
	.byte	0	# set address *.LM13
	.uleb128 0x9
	.byte	0x2
	.quad	.LM13
	.byte	0x18	# line 30
	.byte	0	# set address *.LM14
	.uleb128 0x9
	.byte	0x2
	.quad	.LM14
	.byte	0x18	# line 31
	.byte	0	# set address *.LM15
	.uleb128 0x9
	.byte	0x2
	.quad	.LM15
	.byte	0x19	# line 33
	.byte	0	# set address *.LFE2
	.uleb128 0x9
	.byte	0x2
	.quad	.LFE2
	.byte	0	# end sequence
	.uleb128 0x1
	.byte	0x1
	.byte	0	# set address *.LM1
	.uleb128 0x9
	.byte	0x2
	.quad	.LM1
	.byte	0x1e	# line 8
	.byte	0	# set address *.LM2
	.uleb128 0x9
	.byte	0x2
	.quad	.LM2
	.byte	0x18	# line 9
	.byte	0	# set address *.LM3
	.uleb128 0x9
	.byte	0x2
	.quad	.LM3
	.byte	0x19	# line 11
	.byte	0	# set address *.LM4
	.uleb128 0x9
	.byte	0x2
	.quad	.LM4
	.byte	0x18	# line 12
	.byte	0	# set address *.LM5
	.uleb128 0x9
	.byte	0x2
	.quad	.LM5
	.byte	0x19	# line 14
	.byte	0	# set address *.LM6
	.uleb128 0x9
	.byte	0x2
	.quad	.LM6
	.byte	0x1b	# line 18
	.byte	0	# set address *.LM7
	.uleb128 0x9
	.byte	0x2
	.quad	.LM7
	.byte	0x18	# line 19
	.byte	0	# set address *.LM8
	.uleb128 0x9
	.byte	0x2
	.quad	.LM8
	.byte	0x19	# line 21
	.byte	0	# set address *.LM9
	.uleb128 0x9
	.byte	0x2
	.quad	.LM9
	.byte	0x18	# line 22
	.byte	0	# set address *.LM10
	.uleb128 0x9
	.byte	0x2
	.quad	.LM10
	.byte	0x19	# line 24
	.byte	0	# set address *.Letext0
	.uleb128 0x9
	.byte	0x2
	.quad	.Letext0
	.byte	0	# end sequence
	.uleb128 0x1
	.byte	0x1
.LELT0:
	.section	.debug_str,"MS",@progbits,1
.LASF1:
	.string	"GNU C 4.7.0 20110511 (experimental)"
.LASF3:
	.string	"/usr/src/gcc/obj/gcc"
.LASF0:
	.string	"long int"
.LASF4:
	.string	"main"
.LASF2:
	.string	"clztest.c"
	.ident	"GCC: (GNU) 4.7.0 20110511 (experimental)"
	.section	.note.GNU-stack,"",@progbits
