/* This testcase is part of GDB, the GNU debugger.

   Copyright (C) 2015-2024 Free Software Foundation, Inc.

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

   This file was created with gcc -O2 -g -S -fverbose-asm -dA disasm-optim.c
   and then cleaning up the output.  */

	.file	"disasm-optim.c"

	.text
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB1:
	.file 1 "disasm-optim.c"
	# disasm-optim.c:24
	.loc 1 24 0
	.cfi_startproc
	# disasm-optim.c:25
	.loc 1 25 0
	movl	y(%rip), %eax
.LVL0:
.LBB4:
.LBB5:
	.file 2 "disasm-optim.h"
	# disasm-optim.h:21
	.loc 2 21 0
	testl	%eax, %eax
	js	.L6
	# disasm-optim.h:25
	.loc 2 25 0
	leal	10(%rax), %edx
	testl	%eax, %eax
	movl	$1, %eax
.LVL1:
	cmovne	%edx, %eax
.LVL2:
.L3:
.LBE5:
.LBE4:
	# disasm-optim.c:25
	.loc 1 25 0
	movl	%eax, x(%rip)
	# disasm-optim.c:27
	.loc 1 27 0
	xorl	%eax, %eax
	ret
.LVL3:
.L6:
.LBB7:
.LBB6:
	# disasm-optim.h:22
	.loc 2 22 0
	addl	%eax, %eax
.LVL4:
	jmp	.L3
.LBE6:
.LBE7:
	.cfi_endproc
.LFE1:
	.size	main, .-main
	.comm	y,4,4
	.comm	x,4,4
	.text
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0xb1	# Length of Compilation Unit Info
	.value	0x4	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF0	# DW_AT_producer: "GNU C 4.9.2 20150212 (Red Hat 4.9.2-6) -mtune=generic -march=x86-64 -g -O2"
	.byte	0x1	# DW_AT_language
	.long	.LASF1	# DW_AT_name: "disasm-optim.c"
	.long	.LASF2	# DW_AT_comp_dir: "/main/disassemble3/gdb/testsuite/gdb.base"
	.long	.Ldebug_ranges0+0x30	# DW_AT_ranges
	.quad	0	# DW_AT_low_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x29) DW_TAG_subprogram)
			# DW_AT_external
	.ascii "foo\0"	# DW_AT_name
	.byte	0x2	# DW_AT_decl_file (disasm-optim.h)
	.byte	0x13	# DW_AT_decl_line
			# DW_AT_prototyped
	.long	0x43	# DW_AT_type
	.byte	0x3	# DW_AT_inline
	.long	0x43	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x39) DW_TAG_formal_parameter)
	.ascii "a\0"	# DW_AT_name
	.byte	0x2	# DW_AT_decl_file (disasm-optim.h)
	.byte	0x13	# DW_AT_decl_line
	.long	0x43	# DW_AT_type
	.byte	0	# end of children of DIE 0x29
	.uleb128 0x4	# (DIE (0x43) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x5	# (DIE (0x4a) DW_TAG_subprogram)
			# DW_AT_external
	.long	.LASF3	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (disasm-optim.c)
	.byte	0x17	# DW_AT_decl_line
	.long	0x43	# DW_AT_type
	.quad	.LFB1	# DW_AT_low_pc
	.quad	.LFE1-.LFB1	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.long	0x89	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x6b) DW_TAG_inlined_subroutine)
	.long	0x29	# DW_AT_abstract_origin
	.quad	.LBB4	# DW_AT_entry_pc
	.long	.Ldebug_ranges0+0	# DW_AT_ranges
	.byte	0x1	# DW_AT_call_file (disasm-optim.c)
	.byte	0x19	# DW_AT_call_line
	.uleb128 0x7	# (DIE (0x7e) DW_TAG_formal_parameter)
	.long	0x39	# DW_AT_abstract_origin
	.long	.LLST0	# DW_AT_location
	.byte	0	# end of children of DIE 0x6b
	.byte	0	# end of children of DIE 0x4a
	.uleb128 0x8	# (DIE (0x89) DW_TAG_variable)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (disasm-optim.c)
	.byte	0x14	# DW_AT_decl_line
	.long	0x9c	# DW_AT_type
			# DW_AT_external
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	x
	.uleb128 0x9	# (DIE (0x9c) DW_TAG_volatile_type)
	.long	0x43	# DW_AT_type
	.uleb128 0x8	# (DIE (0xa1) DW_TAG_variable)
	.ascii "y\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (disasm-optim.c)
	.byte	0x14	# DW_AT_decl_line
	.long	0x9c	# DW_AT_type
			# DW_AT_external
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	y
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
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x20	# (DW_AT_inline)
	.uleb128 0xb	# (DW_FORM_data1)
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
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
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
	.uleb128 0x7	# (DW_FORM_data8)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x6	# (abbrev code)
	.uleb128 0x1d	# (TAG: DW_TAG_inlined_subroutine)
	.byte	0x1	# DW_children_yes
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x52	# (DW_AT_entry_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x55	# (DW_AT_ranges)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x58	# (DW_AT_call_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x59	# (DW_AT_call_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.byte	0
	.byte	0
	.uleb128 0x7	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x17	# (DW_FORM_sec_offset)
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
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0x9	# (abbrev code)
	.uleb128 0x35	# (TAG: DW_TAG_volatile_type)
	.byte	0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST0:
	.quad	.LVL0	# Location list begin address (*.LLST0)
	.quad	.LVL1	# Location list end address (*.LLST0)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	.LVL1	# Location list begin address (*.LLST0)
	.quad	.LVL2	# Location list end address (*.LLST0)
	.value	0x3	# Location expression size
	.byte	0x71	# DW_OP_breg1
	.sleb128 -10
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL3	# Location list begin address (*.LLST0)
	.quad	.LVL4	# Location list end address (*.LLST0)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0	# Location list terminator begin (*.LLST0)
	.quad	0	# Location list terminator end (*.LLST0)
	.section	.debug_aranges,"",@progbits
	.long	0x2c	# Length of Address Ranges Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.byte	0x8	# Size of Address
	.byte	0	# Size of Segment Descriptor
	.value	0	# Pad to 16 byte boundary
	.value	0
	.quad	.LFB1	# Address
	.quad	.LFE1-.LFB1	# Length
	.quad	0
	.quad	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.quad	.LBB4	# Offset 0
	.quad	.LBE4
	.quad	.LBB7
	.quad	.LBE7
	.quad	0
	.quad	0
	.quad	.LFB1	# Offset 0x30
	.quad	.LFE1
	.quad	0
	.quad	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF2:
	.string	"/main/disassemble3/gdb/testsuite/gdb.base"
.LASF1:
	.string	"disasm-optim.c"
.LASF3:
	.string	"main"
.LASF0:
	.string	"GNU C 4.9.2 20150212 (Red Hat 4.9.2-6) -mtune=generic -march=x86-64 -g -O2"
	.ident	"GCC: (GNU) 4.9.2 20150212 (Red Hat 4.9.2-6)"
	.section	.note.GNU-stack,"",@progbits
