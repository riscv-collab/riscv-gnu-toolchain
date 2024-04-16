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

/* This file is compiled from gdb.arch/amd64-entry-value-paramref.cc
   using -g -dA -S -O2.
   Additionally it has been patched.  */

	.file	"amd64-entry-value-paramref.cc"
	.text
.Ltext0:
	.p2align 4,,15
	.type	_ZL3barRi.constprop.0, @function
_ZL3barRi.constprop.0:
.LFB2:
	.file 1 "gdb.arch/amd64-entry-value-paramref.cc"
	# gdb.arch/amd64-entry-value-paramref.cc:21
	.loc 1 21 0
	.cfi_startproc
.LVL0:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (FALLTHRU)
	# gdb.arch/amd64-entry-value-paramref.cc:23
	.loc 1 23 0
	movl	vv(%rip), %eax
	# gdb.arch/amd64-entry-value-paramref.cc:24
	.loc 1 24 0
	movq	%rdi, p(%rip)
	# gdb.arch/amd64-entry-value-paramref.cc:23
	.loc 1 23 0
	addl	$1, %eax
	movl	%eax, vv(%rip)
	# gdb.arch/amd64-entry-value-paramref.cc:25
	.loc 1 25 0
	movl	(%rdi), %eax
# SUCC: EXIT [100.0%] 
	# gdb.arch/amd64-entry-value-paramref.cc:26
	.loc 1 26 0
	ret
	.cfi_endproc
.LFE2:
	.size	_ZL3barRi.constprop.0, .-_ZL3barRi.constprop.0
	.section	.text.startup,"ax",@progbits
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB1:
	# gdb.arch/amd64-entry-value-paramref.cc:30
	.loc 1 30 0
	.cfi_startproc
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (FALLTHRU)
	subq	$16, %rsp
	.cfi_def_cfa_offset 24
.LBB2:
	# gdb.arch/amd64-entry-value-paramref.cc:32
	.loc 1 32 0
	leaq	12(%rsp), %rdi
	# gdb.arch/amd64-entry-value-paramref.cc:31
	.loc 1 31 0
	movl	$10, 12(%rsp)
	# gdb.arch/amd64-entry-value-paramref.cc:32
	.loc 1 32 0
	call	_ZL3barRi.constprop.0
.LVL1:
.LBE2:
	# gdb.arch/amd64-entry-value-paramref.cc:33
	.loc 1 33 0
	addq	$16, %rsp
	.cfi_def_cfa_offset 8
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE1:
	.size	main, .-main
	.globl	p
	.bss
	.align 8
	.type	p, @object
	.size	p, 8
p:
	.zero	8
	.globl	vv
	.align 4
	.type	vv, @object
	.size	vv, 4
vv:
	.zero	4
	.text
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	.Linfo_end - .Linfo_start	# Length of Compilation Unit Info
.Linfo_start:
	.value	0x4	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF0	# DW_AT_producer: "GNU C++ 4.8.2 20131212 (Red Hat 4.8.2-7) -mtune=generic -march=x86-64 -g -O2"
	.byte	0x4	# DW_AT_language
	.long	.LASF1	# DW_AT_name: "gdb.arch/amd64-entry-value-paramref.cc"
	.long	.LASF2	# DW_AT_comp_dir: ""
	.long	.Ldebug_ranges0+0	# DW_AT_ranges
	.quad	0	# DW_AT_low_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
DIE29:	.uleb128 0x2	# (DIE (0x29) DW_TAG_subprogram)
	.ascii "bar\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-paramref.cc)
	.byte	0x15	# DW_AT_decl_line
	.long	DIE45 - .Ldebug_info0	# DW_AT_type
	.byte	0x1	# DW_AT_inline
DIE39:	.uleb128 0x3	# (DIE (0x39) DW_TAG_formal_parameter)
	.ascii "ref\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-paramref.cc)
	.byte	0x15	# DW_AT_decl_line
	.long	DIE4c - .Ldebug_info0	# DW_AT_type
	.byte	0	# end of children of DIE 0x29
DIE45:	.uleb128 0x4	# (DIE (0x45) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
DIE4c:	.uleb128 0x5	# (DIE (0x4c) DW_TAG_const_type)
	.long	DIE51 - .Ldebug_info0	# DW_AT_type
DIE51:	.uleb128 0x6	# (DIE (0x51) DW_TAG_reference_type)
	.byte	0x8	# DW_AT_byte_size
	.long	DIE45 - .Ldebug_info0	# DW_AT_type
DIE57:	.uleb128 0x7	# (DIE (0x57) DW_TAG_subprogram)
	.long	DIE29 - .Ldebug_info0	# DW_AT_abstract_origin
	.quad	.LFB2	# DW_AT_low_pc
	.quad	.LFE2-.LFB2	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
DIE72:	.uleb128 0x8	# (DIE (0x72) DW_TAG_formal_parameter)
	.long	DIE39 - .Ldebug_info0	# DW_AT_abstract_origin
	.uleb128 0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0	# end of children of DIE 0x57
DIE7a:	.uleb128 0x9	# (DIE (0x7a) DW_TAG_subprogram)
			# DW_AT_external
	.long	.LASF3	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-paramref.cc)
	.byte	0x1d	# DW_AT_decl_line
	.long	DIE45 - .Ldebug_info0	# DW_AT_type
	.quad	.LFB1	# DW_AT_low_pc
	.quad	.LFE1-.LFB1	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
DIE9b:	.uleb128 0xa	# (DIE (0x9b) DW_TAG_lexical_block)
	.quad	.LBB2	# DW_AT_low_pc
	.quad	.LBE2-.LBB2	# DW_AT_high_pc
DIEac:	.uleb128 0xb	# (DIE (0xac) DW_TAG_variable)
	.ascii "var\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-paramref.cc)
	.byte	0x1f	# DW_AT_decl_line
	.long	DIE45 - .Ldebug_info0	# DW_AT_type
	.uleb128 0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -12
DIEba:	.uleb128 0xc	# (DIE (0xba) DW_TAG_GNU_call_site)
	.quad	.LVL1	# DW_AT_low_pc
	.long	DIE57 - .Ldebug_info0	# DW_AT_abstract_origin
DIEc7:	.uleb128 0xd	# (DIE (0xc7) DW_TAG_GNU_call_site_parameter)
	.uleb128 0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.uleb128 0x2	# DW_AT_GNU_call_site_value
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -12
#if 0
	.uleb128 0x1	# DW_AT_GNU_call_site_data_value
	.byte	0x3a	# DW_OP_lit10
#else
	.uleb128 1f - 2f	# DW_AT_GNU_call_site_data_value
2:
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 1f - 3f
3:
	.byte	0x55	# DW_OP_reg5
1:
#endif
	.byte	0	# end of children of DIE 0xba
	.byte	0	# end of children of DIE 0x9b
	.byte	0	# end of children of DIE 0x7a
DIEd2:	.uleb128 0xe	# (DIE (0xd2) DW_TAG_variable)
	.ascii "vv\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-paramref.cc)
	.byte	0x12	# DW_AT_decl_line
	.long	DIEe6 - .Ldebug_info0	# DW_AT_type
			# DW_AT_external
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	vv
DIEe6:	.uleb128 0xf	# (DIE (0xe6) DW_TAG_volatile_type)
	.long	DIE45 - .Ldebug_info0	# DW_AT_type
DIEeb:	.uleb128 0xe	# (DIE (0xeb) DW_TAG_variable)
	.ascii "p\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-paramref.cc)
	.byte	0x12	# DW_AT_decl_line
	.long	DIEfe - .Ldebug_info0	# DW_AT_type
			# DW_AT_external
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	p
DIEfe:	.uleb128 0x10	# (DIE (0xfe) DW_TAG_pointer_type)
	.byte	0x8	# DW_AT_byte_size
	.long	DIEe6 - .Ldebug_info0	# DW_AT_type
	.byte	0	# end of children of DIE 0xb
.Linfo_end:
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
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x20	# (DW_AT_inline)
	.uleb128 0xb	# (DW_FORM_data1)
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
	.uleb128 0x26	# (TAG: DW_TAG_const_type)
	.byte	0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x6	# (abbrev code)
	.uleb128 0x10	# (TAG: DW_TAG_reference_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x7	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x7	# (DW_FORM_data8)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.byte	0
	.byte	0
	.uleb128 0x8	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0x9	# (abbrev code)
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
	.byte	0
	.byte	0
	.uleb128 0xa	# (abbrev code)
	.uleb128 0xb	# (TAG: DW_TAG_lexical_block)
	.byte	0x1	# DW_children_yes
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x7	# (DW_FORM_data8)
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
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0xc	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0x1	# DW_children_yes
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xd	# (abbrev code)
	.uleb128 0x410a	# (TAG: DW_TAG_GNU_call_site_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2111	# (DW_AT_GNU_call_site_value)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2112	# (DW_AT_GNU_call_site_data_value)
	.uleb128 0x18	# (DW_FORM_exprloc)
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
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0xf	# (abbrev code)
	.uleb128 0x35	# (TAG: DW_TAG_volatile_type)
	.byte	0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x10	# (abbrev code)
	.uleb128 0xf	# (TAG: DW_TAG_pointer_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.byte	0
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
	.quad	.LFB1	# Address
	.quad	.LFE1-.LFB1	# Length
	.quad	0
	.quad	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.quad	.Ltext0	# Offset 0
	.quad	.Letext0
	.quad	.LFB1	# Offset 0x10
	.quad	.LFE1
	.quad	0
	.quad	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF1:
	.string	"gdb.arch/amd64-entry-value-paramref.cc"
.LASF2:
	.string	""
.LASF0:
	.string	"GNU C++ 4.8.2 20131212 (Red Hat 4.8.2-7) -mtune=generic -march=x86-64 -g -O2"
.LASF3:
	.string	"main"
	.ident	"GCC: (GNU) 4.8.2 20131212 (Red Hat 4.8.2-7)"
	.section	.note.GNU-stack,"",@progbits
