/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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

/* This file is compiled from gdb.arch/amd64-entry-value-param.c
   using -g -dA -S -O2.  */

	.file	"amd64-optimout-repeat.c"
	.text
.Ltext0:
	.section	.text.unlikely,"ax",@progbits
.LCOLDB0:
	.section	.text.startup,"ax",@progbits
.LHOTB0:
	.p2align 4,,15
	.section	.text.unlikely
.Ltext_cold0:
	.section	.text.startup
	.globl	main
	.type	main, @function
main:
.LFB0:
	.file 1 "gdb.arch/amd64-optimout-repeat.c"
	# gdb.arch/amd64-optimout-repeat.c:20
	.loc 1 20 0
	.cfi_startproc
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (FALLTHRU)
.LVL0:
	# gdb.arch/amd64-optimout-repeat.c:29
	.loc 1 29 0
	xorl	%eax, %eax
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE0:
	.size	main, .-main
	.section	.text.unlikely
.LCOLDE0:
	.section	.text.startup
.LHOTE0:
	.text
.Letext0:
	.section	.text.unlikely
.Letext_cold0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0x97	# Length of Compilation Unit Info
	.value	0x4	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF1	# DW_AT_producer: "GNU C 4.9.1 20140709 (prerelease) -mtune=generic -march=x86-64 -g -O2"
	.byte	0x1	# DW_AT_language
	.long	.LASF2	# DW_AT_name: "gdb.arch/amd64-optimout-repeat.c"
	.long	.LASF3	# DW_AT_comp_dir: ""
	.long	.Ldebug_ranges0+0	# DW_AT_ranges
	.quad	0	# DW_AT_low_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x29) DW_TAG_subprogram)
			# DW_AT_external
	.long	.LASF4	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-optimout-repeat.c)
	.byte	0x13	# DW_AT_decl_line
			# DW_AT_prototyped
	.long	0x7c	# DW_AT_type
	.quad	.LFB0	# DW_AT_low_pc
	.quad	.LFE0-.LFB0	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.long	0x7c	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x4a) DW_TAG_structure_type)
	.value	0x404	# DW_AT_byte_size
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-optimout-repeat.c)
	.byte	0x15	# DW_AT_decl_line
	.long	0x6a	# DW_AT_sibling
	.uleb128 0x4	# (DIE (0x53) DW_TAG_member)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-optimout-repeat.c)
	.byte	0x17	# DW_AT_decl_line
	.long	0x7c	# DW_AT_type
	.byte	0	# DW_AT_data_member_location
	.uleb128 0x4	# (DIE (0x5d) DW_TAG_member)
	.ascii "xxx\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-optimout-repeat.c)
	.byte	0x18	# DW_AT_decl_line
	.long	0x83	# DW_AT_type
	.byte	0x4	# DW_AT_data_member_location
	.byte	0	# end of children of DIE 0x4a
	.uleb128 0x5	# (DIE (0x6a) DW_TAG_variable)
	.ascii "v\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-optimout-repeat.c)
	.byte	0x1a	# DW_AT_decl_line
	.long	0x4a	# DW_AT_type
	.uleb128 0x7	# DW_AT_location
	.byte	0x30	# DW_OP_lit0
	.byte	0x9f	# DW_OP_stack_value
	.byte	0x93	# DW_OP_piece
	.uleb128 0x4
	.byte	0x93	# DW_OP_piece
	.uleb128 0x400
	.byte	0	# end of children of DIE 0x29
	.uleb128 0x6	# (DIE (0x7c) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x7	# (DIE (0x83) DW_TAG_array_type)
	.long	0x7c	# DW_AT_type
	.long	0x93	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x8c) DW_TAG_subrange_type)
	.long	0x93	# DW_AT_type
	.byte	0xff	# DW_AT_upper_bound
	.byte	0	# end of children of DIE 0x83
	.uleb128 0x9	# (DIE (0x93) DW_TAG_base_type)
	.byte	0x8	# DW_AT_byte_size
	.byte	0x7	# DW_AT_encoding
	.long	.LASF0	# DW_AT_name: "sizetype"
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
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x10	# (DW_AT_stmt_list)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.byte	0
	.byte	0
	.uleb128 0x2	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x7	# (DW_FORM_data8)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x3	# (abbrev code)
	.uleb128 0x13	# (TAG: DW_TAG_structure_type)
	.byte	0x1	# DW_children_yes
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0x5	# (DW_FORM_data2)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x4	# (abbrev code)
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
	.uleb128 0xb	# (DW_FORM_data1)
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
	.uleb128 0x18	# (DW_FORM_exprloc)
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
	.uleb128 0x8	# (DW_FORM_string)
	.byte	0
	.byte	0
	.uleb128 0x7	# (abbrev code)
	.uleb128 0x1	# (TAG: DW_TAG_array_type)
	.byte	0x1	# DW_children_yes
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x8	# (abbrev code)
	.uleb128 0x21	# (TAG: DW_TAG_subrange_type)
	.byte	0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2f	# (DW_AT_upper_bound)
	.uleb128 0xb	# (DW_FORM_data1)
	.byte	0
	.byte	0
	.uleb128 0x9	# (abbrev code)
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
	.byte	0
	.section	.debug_aranges,"",@progbits
	.long	0x2c	# Length of Address Ranges Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.byte	0x8	# Size of Address
	.byte	0	# Size of Segment Descriptor
	.value	0	# Pad to 16 byte boundary
	.value	0
	.quad	.LFB0	# Address
	.quad	.LFE0-.LFB0	# Length
	.quad	0
	.quad	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.quad	.LFB0	# Offset 0
	.quad	.LFE0
	.quad	0
	.quad	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF0:
	.string	"sizetype"
.LASF2:
	.string	"gdb.arch/amd64-optimout-repeat.c"
.LASF1:
	.string	"GNU C 4.9.1 20140709 (prerelease) -mtune=generic -march=x86-64 -g -O2"
.LASF3:
	.string	""
.LASF4:
	.string	"main"
	.ident	"GCC: (GNU) 4.9.1 20140709 (prerelease)"
	.section	.note.GNU-stack,"",@progbits
