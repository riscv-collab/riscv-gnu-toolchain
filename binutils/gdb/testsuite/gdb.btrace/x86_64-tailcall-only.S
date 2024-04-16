/* This testcase is part of GDB, the GNU debugger.

   Copyright 2016-2024 Free Software Foundation, Inc.

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


   This file has been generated using:
   gcc -S -O2 -dA -g tailcall-only.c -o x86_64-tailcall-only.S  */

	.file	"tailcall-only.c"
	.text
.Ltext0:
	.p2align 4,,15
	.type	bar_1, @function
bar_1:
.LFB0:
	.file 1 "tailcall-only.c"
	# tailcall-only.c:22
	.loc 1 22 0
	.cfi_startproc
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (FALLTHRU)
	# tailcall-only.c:24
	.loc 1 24 0
	movl	$42, %eax
# SUCC: EXIT [100.0%]
	ret
	.cfi_endproc
.LFE0:
	.size	bar_1, .-bar_1
	.p2align 4,,15
	.type	bar, @function
bar:
.LFB1:
	# tailcall-only.c:28
	.loc 1 28 0
	.cfi_startproc
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (FALLTHRU)
	# tailcall-only.c:29
	.loc 1 29 0
	jmp	bar_1
# SUCC: EXIT [100.0%]  (ABNORMAL,SIBCALL)
.LVL0:
	.cfi_endproc
.LFE1:
	.size	bar, .-bar
	.p2align 4,,15
	.type	foo_1, @function
foo_1:
.LFB2:
	# tailcall-only.c:34
	.loc 1 34 0
	.cfi_startproc
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (FALLTHRU)
	# tailcall-only.c:35
	.loc 1 35 0
	jmp	bar
# SUCC: EXIT [100.0%]  (ABNORMAL,SIBCALL)
.LVL1:
	.cfi_endproc
.LFE2:
	.size	foo_1, .-foo_1
	.p2align 4,,15
	.type	foo, @function
foo:
.LFB3:
	# tailcall-only.c:40
	.loc 1 40 0
	.cfi_startproc
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (FALLTHRU)
	# tailcall-only.c:41
	.loc 1 41 0
	jmp	foo_1
# SUCC: EXIT [100.0%]  (ABNORMAL,SIBCALL)
.LVL2:
	.cfi_endproc
.LFE3:
	.size	foo, .-foo
	.section	.text.startup,"ax",@progbits
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB4:
	# tailcall-only.c:46
	.loc 1 46 0
	.cfi_startproc
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (FALLTHRU)
	# tailcall-only.c:49
	.loc 1 49 0
	call	foo
.LVL3:
	# tailcall-only.c:50
	.loc 1 50 0
	addl	$1, %eax
.LVL4:
# SUCC: EXIT [100.0%]
	# tailcall-only.c:53
	.loc 1 53 0
	ret
	.cfi_endproc
.LFE4:
	.size	main, .-main
	.text
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0x111	# Length of Compilation Unit Info
	.value	0x4	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF1	# DW_AT_producer: "GNU C 4.8.3 20140911 (Red Hat 4.8.3-9) -mtune=generic -march=x86-64 -g -O2"
	.byte	0x1	# DW_AT_language
	.long	.LASF2	# DW_AT_name: "tailcall-only.c"
	.long	.LASF3	# DW_AT_comp_dir: ""
	.long	.Ldebug_ranges0+0	# DW_AT_ranges
	.quad	0	# DW_AT_low_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x29) DW_TAG_subprogram)
	.long	.LASF4	# DW_AT_name: "bar_1"
	.byte	0x1	# DW_AT_decl_file (tailcall-only.c)
	.byte	0x15	# DW_AT_decl_line
			# DW_AT_prototyped
	.long	0x46	# DW_AT_type
	.quad	.LFB0	# DW_AT_low_pc
	.quad	.LFE0-.LFB0	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.uleb128 0x3	# (DIE (0x46) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x4	# (DIE (0x4d) DW_TAG_subprogram)
	.ascii "bar\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (tailcall-only.c)
	.byte	0x1b	# DW_AT_decl_line
			# DW_AT_prototyped
	.long	0x46	# DW_AT_type
	.quad	.LFB1	# DW_AT_low_pc
	.quad	.LFE1-.LFB1	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.long	0x7c	# DW_AT_sibling
	.uleb128 0x5	# (DIE (0x6e) DW_TAG_GNU_call_site)
	.quad	.LVL0	# DW_AT_low_pc
			# DW_AT_GNU_tail_call
	.long	0x29	# DW_AT_abstract_origin
	.byte	0	# end of children of DIE 0x4d
	.uleb128 0x6	# (DIE (0x7c) DW_TAG_subprogram)
	.long	.LASF0	# DW_AT_name: "foo_1"
	.byte	0x1	# DW_AT_decl_file (tailcall-only.c)
	.byte	0x21	# DW_AT_decl_line
			# DW_AT_prototyped
	.long	0x46	# DW_AT_type
	.quad	.LFB2	# DW_AT_low_pc
	.quad	.LFE2-.LFB2	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.long	0xab	# DW_AT_sibling
	.uleb128 0x5	# (DIE (0x9d) DW_TAG_GNU_call_site)
	.quad	.LVL1	# DW_AT_low_pc
			# DW_AT_GNU_tail_call
	.long	0x4d	# DW_AT_abstract_origin
	.byte	0	# end of children of DIE 0x7c
	.uleb128 0x4	# (DIE (0xab) DW_TAG_subprogram)
	.ascii "foo\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (tailcall-only.c)
	.byte	0x27	# DW_AT_decl_line
			# DW_AT_prototyped
	.long	0x46	# DW_AT_type
	.quad	.LFB3	# DW_AT_low_pc
	.quad	.LFE3-.LFB3	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.long	0xda	# DW_AT_sibling
	.uleb128 0x5	# (DIE (0xcc) DW_TAG_GNU_call_site)
	.quad	.LVL2	# DW_AT_low_pc
			# DW_AT_GNU_tail_call
	.long	0x7c	# DW_AT_abstract_origin
	.byte	0	# end of children of DIE 0xab
	.uleb128 0x7	# (DIE (0xda) DW_TAG_subprogram)
			# DW_AT_external
	.long	.LASF5	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (tailcall-only.c)
	.byte	0x2d	# DW_AT_decl_line
			# DW_AT_prototyped
	.long	0x46	# DW_AT_type
	.quad	.LFB4	# DW_AT_low_pc
	.quad	.LFE4-.LFB4	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.uleb128 0x8	# (DIE (0xf7) DW_TAG_variable)
	.long	.LASF6	# DW_AT_name: "answer"
	.byte	0x1	# DW_AT_decl_file (tailcall-only.c)
	.byte	0x2f	# DW_AT_decl_line
	.long	0x46	# DW_AT_type
	.long	.LLST0	# DW_AT_location
	.uleb128 0x9	# (DIE (0x106) DW_TAG_GNU_call_site)
	.quad	.LVL3	# DW_AT_low_pc
	.long	0xab	# DW_AT_abstract_origin
	.byte	0	# end of children of DIE 0xda
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
	.byte	0	# DW_children_no
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
	.uleb128 0x5	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0	# DW_children_no
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x2115	# (DW_AT_GNU_tail_call)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x6	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
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
	.uleb128 0x7	# (abbrev code)
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
	.byte	0
	.byte	0
	.uleb128 0x8	# (abbrev code)
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
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.byte	0
	.byte	0
	.uleb128 0x9	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0	# DW_children_no
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST0:
	.quad	.LVL3	# Location list begin address (*.LLST0)
	.quad	.LVL4	# Location list end address (*.LLST0)
	.value	0x3	# Location expression size
	.byte	0x70	# DW_OP_breg0
	.sleb128 1
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL4	# Location list begin address (*.LLST0)
	.quad	.LFE4	# Location list end address (*.LLST0)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0	# Location list terminator begin (*.LLST0)
	.quad	0	# Location list terminator end (*.LLST0)
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
	.quad	.LFB4	# Address
	.quad	.LFE4-.LFB4	# Length
	.quad	0
	.quad	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.quad	.Ltext0	# Offset 0
	.quad	.Letext0
	.quad	.LFB4	# Offset 0x10
	.quad	.LFE4
	.quad	0
	.quad	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF4:
	.string	"bar_1"
.LASF2:
	.string	"tailcall-only.c"
.LASF1:
	.string	"GNU C 4.8.3 20140911 (Red Hat 4.8.3-9) -mtune=generic -march=x86-64 -g -O2"
.LASF6:
	.string	"answer"
.LASF5:
	.string	"main"
.LASF3:
	.string	""
.LASF0:
	.string	"foo_1"
	.ident	"GCC: (GNU) 4.8.3 20140911 (Red Hat 4.8.3-9)"
	.section	.note.GNU-stack,"",@progbits
