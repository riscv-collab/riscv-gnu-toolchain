/* This testcase is part of GDB, the GNU debugger.

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

/* This source file was generated from watch-notconst2.c using the following
   command line:

   gcc -m32 -dA -S -g -O2 watch-notconst2.c -o watch-notconst2.S

*/


	.file	"watch-notconst2.c"
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.p2align 4,,15
.globl f
	.type	f, @function
f:
.LFB0:
	.file 1 "watch-notconst2.c"
	# watch-notconst2.c:30
	.loc 1 30 0
	.cfi_startproc
.LVL0:
	# basic block 2
	pushl	%ebp
	.cfi_def_cfa_offset 8
	movl	%esp, %ebp
	.cfi_offset 5, -8
	.cfi_def_cfa_register 5
	pushl	%ebx
	subl	$20, %esp
	# watch-notconst2.c:30
	.loc 1 30 0
	movl	8(%ebp), %ebx
	.cfi_offset 3, -12
	# watch-notconst2.c:32
	.loc 1 32 0
	movl	$2, (%esp)
	call	g
.LVL1:
	# watch-notconst2.c:34
	.loc 1 34 0
	movl	%ebx, 8(%ebp)
	# watch-notconst2.c:35
	.loc 1 35 0
	addl	$20, %esp
	popl	%ebx
	.cfi_restore 3
	popl	%ebp
	.cfi_restore 5
	.cfi_def_cfa 4, 4
.LVL2:
	# watch-notconst2.c:34
	.loc 1 34 0
	jmp	g
	.cfi_endproc
.LFE0:
	.size	f, .-f
.Letext0:
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST0:
	.long	.LVL0-.Ltext0	# Location list begin address (*.LLST0)
	.long	.LVL1-.Ltext0	# Location list end address (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x35	# DW_OP_lit5
	.byte	0x9f	# DW_OP_stack_value
	.long	.LVL1-.Ltext0	# Location list begin address (*.LLST0)
	.long	.LVL2-.Ltext0	# Location list end address (*.LLST0)
	.value	0x1	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.long	0x0	# Location list terminator begin (*.LLST0)
	.long	0x0	# Location list terminator end (*.LLST0)
	.section	.debug_info
	.long	0x5c	# Length of Compilation Unit Info
	.value	0x3	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x4	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF0	# DW_AT_producer: "GNU C 4.4.3 20100127 (Red Hat 4.4.3-4)"
	.byte	0x1	# DW_AT_language
	.long	.LASF1	# DW_AT_name: "watch-notconst2.c"
	.long	.LASF2	# DW_AT_comp_dir: ""
	.long	.Ltext0	# DW_AT_low_pc
	.long	.Letext0	# DW_AT_high_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x25) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.ascii "f\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (watch-notconst2.c)
	.byte	0x1d	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x58	# DW_AT_type
	.long	.LFB0	# DW_AT_low_pc
	.long	.LFE0	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.long	0x58	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x3e) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (watch-notconst2.c)
	.byte	0x1d	# DW_AT_decl_line
	.long	0x58	# DW_AT_type
	.byte	0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 0
	.uleb128 0x4	# (DIE (0x4a) DW_TAG_variable)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (watch-notconst2.c)
	.byte	0x1f	# DW_AT_decl_line
	.long	0x58	# DW_AT_type
	.long	.LLST0	# DW_AT_location
	.byte	0x0	# end of children of DIE 0x25
	.uleb128 0x5	# (DIE (0x58) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.byte	0x0	# end of children of DIE 0xb
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
	.byte	0x0
	.byte	0x0
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
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0x0
	.byte	0x0
	.uleb128 0x3	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0x0	# DW_children_no
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
	.byte	0x0
	.byte	0x0
	.uleb128 0x4	# (abbrev code)
	.uleb128 0x34	# (TAG: DW_TAG_variable)
	.byte	0x0	# DW_children_no
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
	.byte	0x0
	.byte	0x0
	.uleb128 0x5	# (abbrev code)
	.uleb128 0x24	# (TAG: DW_TAG_base_type)
	.byte	0x0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3e	# (DW_AT_encoding)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.section	.debug_pubnames,"",@progbits
	.long	0x14	# Length of Public Names Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.long	0x60	# Compilation Unit Length
	.long	0x25	# DIE offset
	.ascii "f\0"	# external name
	.long	0x0
	.section	.debug_aranges,"",@progbits
	.long	0x1c	# Length of Address Ranges Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.byte	0x4	# Size of Address
	.byte	0x0	# Size of Segment Descriptor
	.value	0x0	# Pad to 8 byte boundary
	.value	0x0
	.long	.Ltext0	# Address
	.long	.Letext0-.Ltext0	# Length
	.long	0x0
	.long	0x0
	.section	.debug_str,"MS",@progbits,1
.LASF1:
	.string	"watch-notconst2.c"
.LASF2:
	.string	""
.LASF0:
	.string	"GNU C 4.4.3 20100127 (Red Hat 4.4.3-4)"
	.ident	"GCC: (GNU) 4.4.3 20100127 (Red Hat 4.4.3-4)"
	.section	.note.GNU-stack,"",@progbits
