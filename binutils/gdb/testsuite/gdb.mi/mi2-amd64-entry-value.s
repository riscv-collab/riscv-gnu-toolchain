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

/* This file is compiled from gdb.mi/mi2-amd64-entry-value.c
   using -g -dA -S -O2.  */

	.file	"mi2-amd64-entry-value.c"
	.text
.Ltext0:
	.p2align 4,,15
	.type	e, @function
e:
.LFB0:
	.file 1 "gdb.mi/mi2-amd64-entry-value.c"
	# gdb.mi/mi2-amd64-entry-value.c:22
	.loc 1 22 0
	.cfi_startproc
.LVL0:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.mi/mi2-amd64-entry-value.c:23
	.loc 1 23 0
	movl	$0, v(%rip)
# SUCC: EXIT [100.0%] 
	# gdb.mi/mi2-amd64-entry-value.c:24
	.loc 1 24 0
	ret
	.cfi_endproc
.LFE0:
	.size	e, .-e
	.p2align 4,,15
	.type	data, @function
data:
.LFB1:
	# gdb.mi/mi2-amd64-entry-value.c:28
	.loc 1 28 0
	.cfi_startproc
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.mi/mi2-amd64-entry-value.c:30
	.loc 1 30 0
	movl	$10, %eax
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE1:
	.size	data, .-data
	.p2align 4,,15
	.type	data2, @function
data2:
.LFB2:
	# gdb.mi/mi2-amd64-entry-value.c:34
	.loc 1 34 0
	.cfi_startproc
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.mi/mi2-amd64-entry-value.c:36
	.loc 1 36 0
	movl	$20, %eax
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE2:
	.size	data2, .-data2
	.p2align 4,,15
	.type	different, @function
different:
.LFB3:
	# gdb.mi/mi2-amd64-entry-value.c:40
	.loc 1 40 0
	.cfi_startproc
.LVL1:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	pushq	%rbx
.LCFI0:
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	# gdb.mi/mi2-amd64-entry-value.c:41
	.loc 1 41 0
	leal	1(%rdi), %ebx
.LVL2:
	# gdb.mi/mi2-amd64-entry-value.c:42
	.loc 1 42 0
	cvtsi2sd	%ebx, %xmm0
	movl	%ebx, %edi
	call	e
.LVL3:
	# gdb.mi/mi2-amd64-entry-value.c:43
	.loc 1 43 0
#APP
# 43 "gdb.mi/mi2-amd64-entry-value.c" 1
	breakhere_different:
# 0 "" 2
	# gdb.mi/mi2-amd64-entry-value.c:45
	.loc 1 45 0
#NO_APP
	movl	%ebx, %eax
	popq	%rbx
.LCFI1:
	.cfi_def_cfa_offset 8
.LVL4:
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE3:
	.size	different, .-different
	.p2align 4,,15
	.type	validity, @function
validity:
.LFB4:
	# gdb.mi/mi2-amd64-entry-value.c:49
	.loc 1 49 0
	.cfi_startproc
.LVL5:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.mi/mi2-amd64-entry-value.c:51
	.loc 1 51 0
	xorpd	%xmm0, %xmm0
	# gdb.mi/mi2-amd64-entry-value.c:49
	.loc 1 49 0
	pushq	%rbx
.LCFI2:
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	# gdb.mi/mi2-amd64-entry-value.c:51
	.loc 1 51 0
	xorl	%edi, %edi
	# gdb.mi/mi2-amd64-entry-value.c:49
	.loc 1 49 0
	movl	%esi, %ebx
	# gdb.mi/mi2-amd64-entry-value.c:51
	.loc 1 51 0
	call	e
.LVL6:
	# gdb.mi/mi2-amd64-entry-value.c:52
	.loc 1 52 0
#APP
# 52 "gdb.mi/mi2-amd64-entry-value.c" 1
	breakhere_validity:
# 0 "" 2
	# gdb.mi/mi2-amd64-entry-value.c:54
	.loc 1 54 0
#NO_APP
	movl	%ebx, %eax
	popq	%rbx
.LCFI3:
	.cfi_def_cfa_offset 8
.LVL7:
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE4:
	.size	validity, .-validity
	.p2align 4,,15
	.type	invalid, @function
invalid:
.LFB5:
	# gdb.mi/mi2-amd64-entry-value.c:58
	.loc 1 58 0
	.cfi_startproc
.LVL8:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.mi/mi2-amd64-entry-value.c:59
	.loc 1 59 0
	xorpd	%xmm0, %xmm0
	xorl	%edi, %edi
.LVL9:
	call	e
.LVL10:
	# gdb.mi/mi2-amd64-entry-value.c:60
	.loc 1 60 0
#APP
# 60 "gdb.mi/mi2-amd64-entry-value.c" 1
	breakhere_invalid:
# 0 "" 2
# SUCC: EXIT [100.0%] 
	# gdb.mi/mi2-amd64-entry-value.c:61
	.loc 1 61 0
#NO_APP
	ret
	.cfi_endproc
.LFE5:
	.size	invalid, .-invalid
	.section	.text.startup,"ax",@progbits
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB6:
	# gdb.mi/mi2-amd64-entry-value.c:65
	.loc 1 65 0
	.cfi_startproc
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# gdb.mi/mi2-amd64-entry-value.c:66
	.loc 1 66 0
	movl	$5, %edi
	call	different
.LVL11:
	# gdb.mi/mi2-amd64-entry-value.c:67
	.loc 1 67 0
	call	data
.LVL12:
	movl	$5, %edi
	movl	%eax, %esi
	call	validity
.LVL13:
	# gdb.mi/mi2-amd64-entry-value.c:68
	.loc 1 68 0
	call	data2
.LVL14:
	movl	%eax, %edi
	call	invalid
.LVL15:
	# gdb.mi/mi2-amd64-entry-value.c:70
	.loc 1 70 0
	xorl	%eax, %eax
	.p2align 4,,1
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE6:
	.size	main, .-main
	.local	v
	.comm	v,4,4
	.text
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0x24e	# Length of Compilation Unit Info
	.value	0x2	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF3	# DW_AT_producer: "GNU C 4.7.0 20110912 (experimental)"
	.byte	0x1	# DW_AT_language
	.long	.LASF4	# DW_AT_name: "gdb.mi/mi2-amd64-entry-value.c"
	.long	.LASF5	# DW_AT_comp_dir: ""
	.long	.Ldebug_ranges0+0	# DW_AT_ranges
	.quad	0	# DW_AT_low_pc
	.quad	0	# DW_AT_entry_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x31) DW_TAG_base_type)
	.byte	0x8	# DW_AT_byte_size
	.byte	0x4	# DW_AT_encoding
	.long	.LASF0	# DW_AT_name: "double"
	.uleb128 0x3	# (DIE (0x38) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x4	# (DIE (0x3f) DW_TAG_subprogram)
	.ascii "e\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.mi/mi2-amd64-entry-value.c)
	.byte	0x15	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.quad	.LFB0	# DW_AT_low_pc
	.quad	.LFE0	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x74	# DW_AT_sibling
	.uleb128 0x5	# (DIE (0x5d) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.mi/mi2-amd64-entry-value.c)
	.byte	0x15	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.uleb128 0x5	# (DIE (0x68) DW_TAG_formal_parameter)
	.ascii "j\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.mi/mi2-amd64-entry-value.c)
	.byte	0x15	# DW_AT_decl_line
	.long	0x31	# DW_AT_type
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0	# end of children of DIE 0x3f
	.uleb128 0x6	# (DIE (0x74) DW_TAG_subprogram)
	.long	.LASF1	# DW_AT_name: "data"
	.byte	0x1	# DW_AT_decl_file (gdb.mi/mi2-amd64-entry-value.c)
	.byte	0x1b	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x38	# DW_AT_type
	.quad	.LFB1	# DW_AT_low_pc
	.quad	.LFE1	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.uleb128 0x6	# (DIE (0x94) DW_TAG_subprogram)
	.long	.LASF2	# DW_AT_name: "data2"
	.byte	0x1	# DW_AT_decl_file (gdb.mi/mi2-amd64-entry-value.c)
	.byte	0x21	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x38	# DW_AT_type
	.quad	.LFB2	# DW_AT_low_pc
	.quad	.LFE2	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.uleb128 0x7	# (DIE (0xb4) DW_TAG_subprogram)
	.long	.LASF6	# DW_AT_name: "different"
	.byte	0x1	# DW_AT_decl_file (gdb.mi/mi2-amd64-entry-value.c)
	.byte	0x27	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x38	# DW_AT_type
	.quad	.LFB3	# DW_AT_low_pc
	.quad	.LFE3	# DW_AT_high_pc
	.long	.LLST0	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x107	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0xd9) DW_TAG_formal_parameter)
	.ascii "val\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.mi/mi2-amd64-entry-value.c)
	.byte	0x27	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST1	# DW_AT_location
	.uleb128 0x9	# (DIE (0xe8) DW_TAG_GNU_call_site)
	.quad	.LVL3	# DW_AT_low_pc
	.long	0x3f	# DW_AT_abstract_origin
	.uleb128 0xa	# (DIE (0xf5) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x2	# DW_AT_GNU_call_site_value
	.byte	0x73	# DW_OP_breg3
	.sleb128 0
	.uleb128 0xa	# (DIE (0xfb) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0x6	# DW_AT_GNU_call_site_value
	.byte	0x73	# DW_OP_breg3
	.sleb128 0
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x38
	.byte	0xf7	# DW_OP_GNU_convert
	.uleb128 0x31
	.byte	0	# end of children of DIE 0xe8
	.byte	0	# end of children of DIE 0xb4
	.uleb128 0x7	# (DIE (0x107) DW_TAG_subprogram)
	.long	.LASF7	# DW_AT_name: "validity"
	.byte	0x1	# DW_AT_decl_file (gdb.mi/mi2-amd64-entry-value.c)
	.byte	0x30	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0x38	# DW_AT_type
	.quad	.LFB4	# DW_AT_low_pc
	.quad	.LFE4	# DW_AT_high_pc
	.long	.LLST2	# DW_AT_frame_base
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x16d	# DW_AT_sibling
	.uleb128 0xb	# (DIE (0x12c) DW_TAG_formal_parameter)
	.long	.LASF8	# DW_AT_name: "lost"
	.byte	0x1	# DW_AT_decl_file (gdb.mi/mi2-amd64-entry-value.c)
	.byte	0x30	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST3	# DW_AT_location
	.uleb128 0xb	# (DIE (0x13b) DW_TAG_formal_parameter)
	.long	.LASF9	# DW_AT_name: "born"
	.byte	0x1	# DW_AT_decl_file (gdb.mi/mi2-amd64-entry-value.c)
	.byte	0x30	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST4	# DW_AT_location
	.uleb128 0x9	# (DIE (0x14a) DW_TAG_GNU_call_site)
	.quad	.LVL6	# DW_AT_low_pc
	.long	0x3f	# DW_AT_abstract_origin
	.uleb128 0xa	# (DIE (0x157) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x30	# DW_OP_lit0
	.uleb128 0xa	# (DIE (0x15c) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0	# fp or vector constant word 1
	.byte	0	# end of children of DIE 0x14a
	.byte	0	# end of children of DIE 0x107
	.uleb128 0xc	# (DIE (0x16d) DW_TAG_subprogram)
	.long	.LASF10	# DW_AT_name: "invalid"
	.byte	0x1	# DW_AT_decl_file (gdb.mi/mi2-amd64-entry-value.c)
	.byte	0x39	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.quad	.LFB5	# DW_AT_low_pc
	.quad	.LFE5	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x1bf	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x18d) DW_TAG_formal_parameter)
	.ascii "inv\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.mi/mi2-amd64-entry-value.c)
	.byte	0x39	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.long	.LLST5	# DW_AT_location
	.uleb128 0x9	# (DIE (0x19c) DW_TAG_GNU_call_site)
	.quad	.LVL10	# DW_AT_low_pc
	.long	0x3f	# DW_AT_abstract_origin
	.uleb128 0xa	# (DIE (0x1a9) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x30	# DW_OP_lit0
	.uleb128 0xa	# (DIE (0x1ae) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x61	# DW_OP_reg17
	.byte	0xb	# DW_AT_GNU_call_site_value
	.byte	0xf4	# DW_OP_GNU_const_type
	.uleb128 0x31
	.byte	0x8
	.long	0	# fp or vector constant word 0
	.long	0	# fp or vector constant word 1
	.byte	0	# end of children of DIE 0x19c
	.byte	0	# end of children of DIE 0x16d
	.uleb128 0xd	# (DIE (0x1bf) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF11	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (gdb.mi/mi2-amd64-entry-value.c)
	.byte	0x40	# DW_AT_decl_line
	.long	0x38	# DW_AT_type
	.quad	.LFB6	# DW_AT_low_pc
	.quad	.LFE6	# DW_AT_high_pc
	.byte	0x2	# DW_AT_frame_base
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.byte	0x1	# DW_AT_GNU_all_call_sites
	.long	0x239	# DW_AT_sibling
	.uleb128 0xe	# (DIE (0x1e3) DW_TAG_GNU_call_site)
	.quad	.LVL11	# DW_AT_low_pc
	.long	0xb4	# DW_AT_abstract_origin
	.long	0x1fa	# DW_AT_sibling
	.uleb128 0xa	# (DIE (0x1f4) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x35	# DW_OP_lit5
	.byte	0	# end of children of DIE 0x1e3
	.uleb128 0xf	# (DIE (0x1fa) DW_TAG_GNU_call_site)
	.quad	.LVL12	# DW_AT_low_pc
	.long	0x74	# DW_AT_abstract_origin
	.uleb128 0xe	# (DIE (0x207) DW_TAG_GNU_call_site)
	.quad	.LVL13	# DW_AT_low_pc
	.long	0x107	# DW_AT_abstract_origin
	.long	0x21e	# DW_AT_sibling
	.uleb128 0xa	# (DIE (0x218) DW_TAG_GNU_call_site_parameter)
	.byte	0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0x1	# DW_AT_GNU_call_site_value
	.byte	0x35	# DW_OP_lit5
	.byte	0	# end of children of DIE 0x207
	.uleb128 0xf	# (DIE (0x21e) DW_TAG_GNU_call_site)
	.quad	.LVL14	# DW_AT_low_pc
	.long	0x94	# DW_AT_abstract_origin
	.uleb128 0xf	# (DIE (0x22b) DW_TAG_GNU_call_site)
	.quad	.LVL15	# DW_AT_low_pc
	.long	0x16d	# DW_AT_abstract_origin
	.byte	0	# end of children of DIE 0x1bf
	.uleb128 0x10	# (DIE (0x239) DW_TAG_variable)
	.ascii "v\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.mi/mi2-amd64-entry-value.c)
	.byte	0x12	# DW_AT_decl_line
	.long	0x24c	# DW_AT_type
	.byte	0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	v
	.uleb128 0x11	# (DIE (0x24c) DW_TAG_volatile_type)
	.long	0x38	# DW_AT_type
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
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0	# DW_children_no
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
	.uleb128 0xa	# (DW_FORM_block1)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.byte	0
	.byte	0
	.uleb128 0x7	# (abbrev code)
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
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
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
	.uleb128 0x6	# (DW_FORM_data4)
	.byte	0
	.byte	0
	.uleb128 0x9	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0x1	# DW_children_yes
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xa	# (abbrev code)
	.uleb128 0x410a	# (TAG: DW_TAG_GNU_call_site_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0xa	# (DW_FORM_block1)
	.uleb128 0x2111	# (DW_AT_GNU_call_site_value)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0
	.byte	0
	.uleb128 0xb	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
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
	.uleb128 0x6	# (DW_FORM_data4)
	.byte	0
	.byte	0
	.uleb128 0xc	# (abbrev code)
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
	.uleb128 0xd	# (abbrev code)
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
	.uleb128 0xa	# (DW_FORM_block1)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xe	# (abbrev code)
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
	.uleb128 0xf	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0	# DW_children_no
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x10	# (abbrev code)
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
	.uleb128 0x11	# (abbrev code)
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
	.quad	.LFB3	# Location list begin address (*.LLST0)
	.quad	.LCFI0	# Location list end address (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI0	# Location list begin address (*.LLST0)
	.quad	.LCFI1	# Location list end address (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI1	# Location list begin address (*.LLST0)
	.quad	.LFE3	# Location list end address (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	0	# Location list terminator begin (*.LLST0)
	.quad	0	# Location list terminator end (*.LLST0)
.LLST1:
	.quad	.LVL1	# Location list begin address (*.LLST1)
	.quad	.LVL2	# Location list end address (*.LLST1)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL2	# Location list begin address (*.LLST1)
	.quad	.LVL4	# Location list end address (*.LLST1)
	.value	0x1	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.quad	.LVL4	# Location list begin address (*.LLST1)
	.quad	.LFE3	# Location list end address (*.LLST1)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0	# Location list terminator begin (*.LLST1)
	.quad	0	# Location list terminator end (*.LLST1)
.LLST2:
	.quad	.LFB4	# Location list begin address (*.LLST2)
	.quad	.LCFI2	# Location list end address (*.LLST2)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	.LCFI2	# Location list begin address (*.LLST2)
	.quad	.LCFI3	# Location list end address (*.LLST2)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 16
	.quad	.LCFI3	# Location list begin address (*.LLST2)
	.quad	.LFE4	# Location list end address (*.LLST2)
	.value	0x2	# Location expression size
	.byte	0x77	# DW_OP_breg7
	.sleb128 8
	.quad	0	# Location list terminator begin (*.LLST2)
	.quad	0	# Location list terminator end (*.LLST2)
.LLST3:
	.quad	.LVL5	# Location list begin address (*.LLST3)
	.quad	.LVL5	# Location list end address (*.LLST3)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	0	# Location list terminator begin (*.LLST3)
	.quad	0	# Location list terminator end (*.LLST3)
.LLST4:
	.quad	.LVL5	# Location list begin address (*.LLST4)
	.quad	.LVL6-1	# Location list end address (*.LLST4)
	.value	0x1	# Location expression size
	.byte	0x54	# DW_OP_reg4
	.quad	.LVL6-1	# Location list begin address (*.LLST4)
	.quad	.LVL7	# Location list end address (*.LLST4)
	.value	0x1	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.quad	.LVL7	# Location list begin address (*.LLST4)
	.quad	.LFE4	# Location list end address (*.LLST4)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0	# Location list terminator begin (*.LLST4)
	.quad	0	# Location list terminator end (*.LLST4)
.LLST5:
	.quad	.LVL8	# Location list begin address (*.LLST5)
	.quad	.LVL9	# Location list end address (*.LLST5)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL9	# Location list begin address (*.LLST5)
	.quad	.LFE5	# Location list end address (*.LLST5)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST5)
	.quad	0	# Location list terminator end (*.LLST5)
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
	.quad	.LFB6	# Address
	.quad	.LFE6-.LFB6	# Length
	.quad	0
	.quad	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.quad	.Ltext0	# Offset 0
	.quad	.Letext0
	.quad	.LFB6	# Offset 0x10
	.quad	.LFE6
	.quad	0
	.quad	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF2:
	.string	"data2"
.LASF3:
	.string	"GNU C 4.7.0 20110912 (experimental)"
.LASF4:
	.string	"gdb.mi/mi2-amd64-entry-value.c"
.LASF9:
	.string	"born"
.LASF6:
	.string	"different"
.LASF7:
	.string	"validity"
.LASF10:
	.string	"invalid"
.LASF0:
	.string	"double"
.LASF11:
	.string	"main"
.LASF1:
	.string	"data"
.LASF8:
	.string	"lost"
.LASF5:
	.string	""
	.ident	"GCC: (GNU) 4.7.0 20110912 (experimental)"
	.section	.note.GNU-stack,"",@progbits
