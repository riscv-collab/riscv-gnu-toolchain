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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   This file was created by doing:
   gcc -gdwarf-4 -gsplit-debug -S -dA fission-base.c
   and then massaging the output.
*/

	.file	"fission-base.c"

	.text
.Ltext0:

	.globl	func
	.type	func, @function
func:
.LFB0:
	.file 1 "fission-base.c"
	# fission-base.c:20
	.loc 1 20 0
	.cfi_startproc
	pushq	%rbp	#
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp	#,
	.cfi_def_cfa_register 6
	movl	%edi, -4(%rbp)	# arg, arg
	# fission-base.c:21
	.loc 1 21 0
	movl	-4(%rbp), %eax	# arg, tmp61
	addl	$1, %eax	#, D.1617
	# fission-base.c:22
	.loc 1 22 0
	popq	%rbp	#
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	func, .-func
	.globl	main
	.type	main, @function
main:
.LFB1:
	# fission-base.c:26
	.loc 1 26 0
	.cfi_startproc
	pushq	%rbp	#
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp	#,
	.cfi_def_cfa_register 6
	# fission-base.c:27
	.loc 1 27 0
	movl	$-1, %edi	#,
	call	func	#
	# fission-base.c:28
	.loc 1 28 0
	popq	%rbp	#
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	main, .-main
.Letext0:
	.section	.debug_info.dwo,"e",@progbits
.Ldebug_info0:
	.4byte	.Ldebug_info0_end - .Ldebug_info0 - 4 # Length of Compilation Unit Info
	.2byte	0x4	# DWARF version number
	.4byte	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.ascii "GNU C 4.6.x-fission\0"	# DW_AT_producer
	.byte	0x1	# DW_AT_language
	.ascii "fission-base.c\0"	# DW_AT_name
	.ascii "/tmp/src/gdb/testsuite\0"	# DW_AT_comp_dir
	.byte	1	# DW_AT_GNU_dwo_id
	.byte	2
	.byte	3
	.byte	4
	.byte	5
	.byte	6
	.byte	7
	.byte	8
	.uleb128 0x2	# (DIE (0x89) DW_TAG_subprogram)
			# DW_AT_external
	.ascii "func\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (fission-base.c)
	.byte	0x13	# DW_AT_decl_line
			# DW_AT_prototyped
	.4byte	.Ldebug_info0_int - .Ldebug_info0 # DW_AT_type
	.4byte	0x0	# DW_AT_ranges
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.4byte	.Ldebug_info0_int - .Ldebug_info0 # DW_AT_sibling
	.uleb128 0x3	# (DIE (0xa4) DW_TAG_formal_parameter)
	.ascii "arg\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (fission-base.c)
	.byte	0x13	# DW_AT_decl_line
	.4byte	.Ldebug_info0_int - .Ldebug_info0 # DW_AT_type
	.uleb128 0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -20
	.byte	0	# end of children of DIE 0x89
.Ldebug_info0_int:
	.uleb128 0x4	# (DIE (0xb3) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x5	# (DIE (0xba) DW_TAG_subprogram)
			# DW_AT_external
	.ascii "main\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (fission-base.c)
	.byte	0x19	# DW_AT_decl_line
	.4byte	.Ldebug_info0_int - .Ldebug_info0 # DW_AT_type
	.uleb128 0x1	# DW_AT_low_pc
	.8byte	.LFE1-.LFB1	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.byte	0	# end of children of DIE 0xb
.Ldebug_info0_end:
	.section	.debug_info,"",@progbits
.Lskeleton_debug_info0:
	.4byte	.Lskeleton_debug_info0_end - .Lskeleton_debug_info0 - 4 # Length of Compilation Unit Info
	.2byte	0x4	# DWARF version number
	.4byte	.Lskeleton_debug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0) DW_TAG_compile_unit)
	.8byte	.Ltext0	# DW_AT_low_pc
	.8byte	.Letext0-.Ltext0	# DW_AT_high_pc
	.4byte	.Ldebug_line0	# DW_AT_stmt_list
	.ascii "/tmp/src/gdb/testsuite\0"	# DW_AT_comp_dir
	.asciz DWO	# DW_AT_GNU_dwo_name
	.4byte	.Ldebug_pubnames0	# DW_AT_GNU_pubnames
	.4byte	.Ldebug_pubtypes0	# DW_AT_GNU_pubtypes
	.4byte	.Ldebug_addr0_begin	# DW_AT_GNU_addr_base
	.4byte	.Ldebug_ranges0_begin	# DW_AT_GNU_ranges_base
	.byte	1	# DW_AT_GNU_dwo_id
	.byte	2
	.byte	3
	.byte	4
	.byte	5
	.byte	6
	.byte	7
	.byte	8
.Lskeleton_debug_info0_end:
	.section	.debug_abbrev,"",@progbits
.Lskeleton_debug_abbrev0:
	.uleb128 0x1	# (abbrev code)
	.uleb128 0x11	# (TAG: DW_TAG_compile_unit)
	.byte	0	# DW_children_no
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x7	# (DW_FORM_data8)
	.uleb128 0x10	# (DW_AT_stmt_list)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x1b	# (DW_AT_comp_dir)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x2130	# (DW_AT_GNU_dwo_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x2134	# (DW_AT_GNU_pubnames)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x2135	# (DW_AT_GNU_pubtypes)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x2133	# (DW_AT_GNU_addr_base)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x2132	# (DW_AT_GNU_ranges_base)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x2131	# (DW_AT_GNU_dwo_id)
	.uleb128 0x7	# (DW_FORM_data8)
	.byte	0
	.byte	0
	.byte	0	# end of skeleton .debug_abbrev
	.section	.debug_abbrev.dwo,"e",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1	# (abbrev code)
	.uleb128 0x11	# (TAG: DW_TAG_compile_unit)
	.byte	0x1	# DW_children_yes
	.uleb128 0x25	# (DW_AT_producer)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x13	# (DW_AT_language)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x1b	# (DW_AT_comp_dir)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x2131	# (DW_AT_GNU_dwo_id)
	.uleb128 0x7	# (DW_FORM_data8)
	.byte	0
	.byte	0
	.uleb128 0x2	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x55	# (DW_AT_ranges)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x18	# (DW_FORM_exprloc)
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
	.uleb128 0x18	# (DW_FORM_exprloc)
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
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0	# DW_children_no
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1f01	# (DW_FORM_GNU_addr_index)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x7	# (DW_FORM_data8)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_gnu_pubnames,"",@progbits
.Ldebug_pubnames0:
	.long	0x22	# Pub Info Length
	.value	0x2	# DWARF Version
	.long	.Lskeleton_debug_info0	# Offset of Compilation Unit Info
	.long	0x59	# Compilation Unit Length
	.long	0x18	# DIE offset
	.byte	0x30	# GDB-index flags
	.ascii "func\0"	# external name
	.long	0x45	# DIE offset
	.byte	0x30	# GDB-index flags
	.ascii "main\0"	# external name
	.long	0
	.section	.debug_gnu_pubtypes,"",@progbits
.Ldebug_pubtypes0:
	.long	0x17	# Pub Info Length
	.value	0x2	# DWARF Version
	.long	.Lskeleton_debug_info0	# Offset of Compilation Unit Info
	.long	0x59	# Compilation Unit Length
	.long	0x3e	# DIE offset
	.byte	0x90	# GDB-index flags
	.ascii "int\0"	# external name
	.long	0
	.section	.debug_aranges,"",@progbits
	.4byte	0x2c	# Length of Address Ranges Info
	.2byte	0x2	# DWARF Version
	.4byte	.Lskeleton_debug_info0	# Offset of Compilation Unit Info
	.byte	0x8	# Size of Address
	.byte	0	# Size of Segment Descriptor
	.2byte	0	# Pad to 16 byte boundary
	.2byte	0
	.8byte	.Ltext0	# Address
	.8byte	.Letext0-.Ltext0	# Length
	.8byte	0
	.8byte	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_line.dwo,"e",@progbits
.Lskeleton_debug_line0:
	.4byte	.LELT0-.LSLT0	# Length of Source Line Info
.LSLT0:
	.2byte	0x4	# DWARF Version
	.4byte	.LELTP0-.LASLTP0	# Prolog Length
.LASLTP0:
	.byte	0x1	# Minimum Instruction Length
	.byte	0x1	# Maximum Operations Per Instruction
	.byte	0x1	# Default is_stmt_start flag
	.byte	0xf6	# Line Base Value (Special Opcodes)
	.byte	0xf5	# Line Range Value (Special Opcodes)
	.byte	0xa	# Special Opcode Base
	.byte	0	# opcode: 0x1 has 0 args
	.byte	0x1	# opcode: 0x2 has 1 args
	.byte	0x1	# opcode: 0x3 has 1 args
	.byte	0x1	# opcode: 0x4 has 1 args
	.byte	0x1	# opcode: 0x5 has 1 args
	.byte	0	# opcode: 0x6 has 0 args
	.byte	0	# opcode: 0x7 has 0 args
	.byte	0	# opcode: 0x8 has 0 args
	.byte	0x1	# opcode: 0x9 has 1 args
	.byte	0	# End directory table
	.ascii "fission-base.c\0"	# File Entry: 0x1
	.uleb128 0
	.uleb128 0
	.uleb128 0
	.byte	0	# End file name table
.LELTP0:
.LELT0:
	.section	.debug_addr,"",@progbits
.Ldebug_addr0:
	# Shift the real entries down by a non-zero amount to test
	# DW_AT_GNU_addr_base.
	.8byte	0,0
.Ldebug_addr0_begin:
	.8byte	.LFB0	# DW_AT_low_pc
	.8byte	.LFB1	# DW_AT_low_pc

	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	# Shift the real entries down by a non-zero amount to test
	# DW_AT_GNU_ranges_base.
	.8byte	0,0
.Ldebug_ranges0_begin:
	# Note: Since the DW_TAG_compile_unit specifies low_pc, that sets
	# the base address, and thus we have to subtract it here.
	.8byte	.LFB0 - .Ltext0	# Offset 0
	.8byte	.LFE0 - .Ltext0
	.8byte	0
	.8byte	0

	.section	.note.GNU-stack,"",@progbits
