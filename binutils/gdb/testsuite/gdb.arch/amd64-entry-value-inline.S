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

/* This file is compiled from gdb.arch/amd64-entry-value-inline.c
   using -g -dA -S -O2.  */

	.file	"amd64-entry-value-inline.c"
	.text
.Ltext0:
	.p2align 4,,15
	.type	fn1, @function
fn1:
.LFB0:
	.file 1 "gdb.arch/amd64-entry-value-inline.c"
	# gdb.arch/amd64-entry-value-inline.c:22
	.loc 1 22 0
	.cfi_startproc
.LVL0:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (FALLTHRU)
	# gdb.arch/amd64-entry-value-inline.c:23
	.loc 1 23 0
	movl	v(%rip), %eax
	addl	$1, %eax
	movl	%eax, v(%rip)
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE0:
	.size	fn1, .-fn1
	.p2align 4,,15
	.globl	fn3
	.type	fn3, @function
fn3:
.LFB2:
	# gdb.arch/amd64-entry-value-inline.c:41
	.loc 1 41 0
	.cfi_startproc
.LVL1:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (FALLTHRU)
.LBB4:
.LBB5:
	# gdb.arch/amd64-entry-value-inline.c:29
	.loc 1 29 0
	testl	%esi, %esi
.LBE5:
.LBE4:
	# gdb.arch/amd64-entry-value-inline.c:41
	.loc 1 41 0
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	# gdb.arch/amd64-entry-value-inline.c:41
	.loc 1 41 0
	movl	%edi, %ebx
.LBB7:
.LBB6:
# SUCC: 3 [39.0%]  (FALLTHRU,CAN_FALLTHRU) 4 [61.0%]  (CAN_FALLTHRU)
	# gdb.arch/amd64-entry-value-inline.c:29
	.loc 1 29 0
	je	.L3
# BLOCK 3 freq:3898 seq:1
# PRED: 2 [39.0%]  (FALLTHRU,CAN_FALLTHRU)
	# gdb.arch/amd64-entry-value-inline.c:31
	.loc 1 31 0
	call	fn1
.LVL2:
	# gdb.arch/amd64-entry-value-inline.c:32
	.loc 1 32 0
	leal	-2(%rbx), %eax
.LVL3:
	# gdb.arch/amd64-entry-value-inline.c:33
	.loc 1 33 0
	movl	%eax, %edi
	imull	%eax, %edi
	addl	$1, %edi
.LVL4:
	imull	%edi, %eax
.LVL5:
	# gdb.arch/amd64-entry-value-inline.c:34
	.loc 1 34 0
	leal	(%rbx,%rax), %edi
	call	fn1
.LVL6:
# SUCC: 4 [100.0%]  (FALLTHRU,CAN_FALLTHRU)
# BLOCK 4 freq:10000 seq:2
# PRED: 2 [61.0%]  (CAN_FALLTHRU) 3 [100.0%]  (FALLTHRU,CAN_FALLTHRU)
.L3:
.LBE6:
.LBE7:
	# gdb.arch/amd64-entry-value-inline.c:43
	.loc 1 43 0
	movl	%ebx, %eax
	popq	%rbx
	.cfi_def_cfa_offset 8
.LVL7:
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE2:
	.size	fn3, .-fn3
	.section	.text.startup,"ax",@progbits
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB3:
	# gdb.arch/amd64-entry-value-inline.c:47
	.loc 1 47 0
	.cfi_startproc
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (FALLTHRU)
	# gdb.arch/amd64-entry-value-inline.c:48
	.loc 1 48 0
	movl	$25, %esi
	movl	$6, %edi
	call	fn3
.LVL8:
	# gdb.arch/amd64-entry-value-inline.c:50
	.loc 1 50 0
	xorl	%eax, %eax
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE3:
	.size	main, .-main
	.local	v
	.comm	v,4,4
	.text
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0x164	# Length of Compilation Unit Info
	.value	0x4	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF0	# DW_AT_producer: "GNU C 4.8.0 20121005 (experimental) -mtune=generic -march=x86-64 -g -O2"
	.byte	0x1	# DW_AT_language
	.long	.LASF1	# DW_AT_name: "gdb.arch/amd64-entry-value-inline.c"
	.long	.LASF2	# DW_AT_comp_dir: ""
	.long	.Ldebug_ranges0+0x30	# DW_AT_ranges
	.quad	0	# DW_AT_low_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x29) DW_TAG_subprogram)
	.ascii "fn1\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-inline.c)
	.byte	0x15	# DW_AT_decl_line
			# DW_AT_prototyped
	.quad	.LFB0	# DW_AT_low_pc
	.quad	.LFE0-.LFB0	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.long	0x52	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x46) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-inline.c)
	.byte	0x15	# DW_AT_decl_line
	.long	0x52	# DW_AT_type
	.uleb128 0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0	# end of children of DIE 0x29
	.uleb128 0x4	# (DIE (0x52) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x5	# (DIE (0x59) DW_TAG_subprogram)
	.ascii "fn2\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-inline.c)
	.byte	0x1b	# DW_AT_decl_line
			# DW_AT_prototyped
	.long	0x52	# DW_AT_type
	.byte	0x1	# DW_AT_inline
	.long	0x7c	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x69) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-inline.c)
	.byte	0x1b	# DW_AT_decl_line
	.long	0x52	# DW_AT_type
	.uleb128 0x6	# (DIE (0x72) DW_TAG_formal_parameter)
	.ascii "y\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-inline.c)
	.byte	0x1b	# DW_AT_decl_line
	.long	0x52	# DW_AT_type
	.byte	0	# end of children of DIE 0x59
	.uleb128 0x7	# (DIE (0x7c) DW_TAG_subprogram)
			# DW_AT_external
	.ascii "fn3\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-inline.c)
	.byte	0x28	# DW_AT_decl_line
			# DW_AT_prototyped
	.long	0x52	# DW_AT_type
	.quad	.LFB2	# DW_AT_low_pc
	.quad	.LFE2-.LFB2	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.long	0x115	# DW_AT_sibling
	.uleb128 0x8	# (DIE (0x9d) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-inline.c)
	.byte	0x28	# DW_AT_decl_line
	.long	0x52	# DW_AT_type
	.long	.LLST0	# DW_AT_location
	.uleb128 0x8	# (DIE (0xaa) DW_TAG_formal_parameter)
	.ascii "y\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-inline.c)
	.byte	0x28	# DW_AT_decl_line
	.long	0x52	# DW_AT_type
	.long	.LLST1	# DW_AT_location
	.uleb128 0x9	# (DIE (0xb7) DW_TAG_inlined_subroutine)
	.long	0x59	# DW_AT_abstract_origin
	.quad	.LBB4	# DW_AT_entry_pc
	.long	.Ldebug_ranges0+0	# DW_AT_ranges
	.byte	0x1	# DW_AT_call_file (gdb.arch/amd64-entry-value-inline.c)
	.byte	0x2a	# DW_AT_call_line
	.uleb128 0xa	# (DIE (0xca) DW_TAG_formal_parameter)
	.long	0x72	# DW_AT_abstract_origin
	.long	.LLST2	# DW_AT_location
	.uleb128 0xa	# (DIE (0xd3) DW_TAG_formal_parameter)
	.long	0x69	# DW_AT_abstract_origin
	.long	.LLST0	# DW_AT_location
	.uleb128 0xb	# (DIE (0xdc) DW_TAG_GNU_call_site)
	.quad	.LVL2	# DW_AT_low_pc
	.long	0x29	# DW_AT_abstract_origin
	.long	0xf4	# DW_AT_sibling
	.uleb128 0xc	# (DIE (0xed) DW_TAG_GNU_call_site_parameter)
	.uleb128 0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.uleb128 0x2	# DW_AT_GNU_call_site_value
	.byte	0x73	# DW_OP_breg3
	.sleb128 0
	.byte	0	# end of children of DIE 0xdc
	.uleb128 0xd	# (DIE (0xf4) DW_TAG_GNU_call_site)
	.quad	.LVL6	# DW_AT_low_pc
	.long	0x29	# DW_AT_abstract_origin
	.uleb128 0xc	# (DIE (0x101) DW_TAG_GNU_call_site_parameter)
	.uleb128 0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.uleb128 0xd	# DW_AT_GNU_call_site_value
	.byte	0x73	# DW_OP_breg3
	.sleb128 -2
	.byte	0x73	# DW_OP_breg3
	.sleb128 -2
	.byte	0x73	# DW_OP_breg3
	.sleb128 -2
	.byte	0x1e	# DW_OP_mul
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x1
	.byte	0x1e	# DW_OP_mul
	.byte	0x73	# DW_OP_breg3
	.sleb128 0
	.byte	0x22	# DW_OP_plus
	.byte	0	# end of children of DIE 0xf4
	.byte	0	# end of children of DIE 0xb7
	.byte	0	# end of children of DIE 0x7c
	.uleb128 0xe	# (DIE (0x115) DW_TAG_subprogram)
			# DW_AT_external
	.long	.LASF3	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-inline.c)
	.byte	0x2e	# DW_AT_decl_line
	.long	0x52	# DW_AT_type
	.quad	.LFB3	# DW_AT_low_pc
	.quad	.LFE3-.LFB3	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.long	0x14f	# DW_AT_sibling
	.uleb128 0xd	# (DIE (0x136) DW_TAG_GNU_call_site)
	.quad	.LVL8	# DW_AT_low_pc
	.long	0x7c	# DW_AT_abstract_origin
	.uleb128 0xc	# (DIE (0x143) DW_TAG_GNU_call_site_parameter)
	.uleb128 0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.uleb128 0x1	# DW_AT_GNU_call_site_value
	.byte	0x36	# DW_OP_lit6
	.uleb128 0xc	# (DIE (0x148) DW_TAG_GNU_call_site_parameter)
	.uleb128 0x1	# DW_AT_location
	.byte	0x54	# DW_OP_reg4
	.uleb128 0x1	# DW_AT_GNU_call_site_value
	.byte	0x49	# DW_OP_lit25
	.byte	0	# end of children of DIE 0x136
	.byte	0	# end of children of DIE 0x115
	.uleb128 0xf	# (DIE (0x14f) DW_TAG_variable)
	.ascii "v\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-entry-value-inline.c)
	.byte	0x12	# DW_AT_decl_line
	.long	0x162	# DW_AT_type
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	v
	.uleb128 0x10	# (DIE (0x162) DW_TAG_volatile_type)
	.long	0x52	# DW_AT_type
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
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0x19	# (DW_FORM_flag_present)
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
	.uleb128 0x20	# (DW_AT_inline)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x6	# (abbrev code)
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
	.uleb128 0x7	# (abbrev code)
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
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.byte	0
	.byte	0
	.uleb128 0x9	# (abbrev code)
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
	.uleb128 0xa	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.byte	0
	.byte	0
	.uleb128 0xb	# (abbrev code)
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
	.uleb128 0xc	# (abbrev code)
	.uleb128 0x410a	# (TAG: DW_TAG_GNU_call_site_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2111	# (DW_AT_GNU_call_site_value)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0xd	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0x1	# DW_children_yes
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xe	# (abbrev code)
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
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0x10	# (abbrev code)
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
	.quad	.LVL1	# Location list begin address (*.LLST0)
	.quad	.LVL2-1	# Location list end address (*.LLST0)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.quad	.LVL2-1	# Location list begin address (*.LLST0)
	.quad	.LVL7	# Location list end address (*.LLST0)
	.value	0x1	# Location expression size
	.byte	0x53	# DW_OP_reg3
	.quad	.LVL7	# Location list begin address (*.LLST0)
	.quad	.LFE2	# Location list end address (*.LLST0)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	0	# Location list terminator begin (*.LLST0)
	.quad	0	# Location list terminator end (*.LLST0)
.LLST1:
	.quad	.LVL1	# Location list begin address (*.LLST1)
	.quad	.LVL2-1	# Location list end address (*.LLST1)
	.value	0x1	# Location expression size
	.byte	0x54	# DW_OP_reg4
	.quad	.LVL2-1	# Location list begin address (*.LLST1)
	.quad	.LFE2	# Location list end address (*.LLST1)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x54	# DW_OP_reg4
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST1)
	.quad	0	# Location list terminator end (*.LLST1)
.LLST2:
	.quad	.LVL1	# Location list begin address (*.LLST2)
	.quad	.LVL2-1	# Location list end address (*.LLST2)
	.value	0x1	# Location expression size
	.byte	0x54	# DW_OP_reg4
	.quad	.LVL2-1	# Location list begin address (*.LLST2)
	.quad	.LVL3	# Location list end address (*.LLST2)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x54	# DW_OP_reg4
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL3	# Location list begin address (*.LLST2)
	.quad	.LVL4	# Location list end address (*.LLST2)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	.LVL4	# Location list begin address (*.LLST2)
	.quad	.LVL5	# Location list end address (*.LLST2)
	.value	0x6	# Location expression size
	.byte	0x70	# DW_OP_breg0
	.sleb128 0
	.byte	0x75	# DW_OP_breg5
	.sleb128 0
	.byte	0x1e	# DW_OP_mul
	.byte	0x9f	# DW_OP_stack_value
	.quad	.LVL5	# Location list begin address (*.LLST2)
	.quad	.LVL6-1	# Location list end address (*.LLST2)
	.value	0x1	# Location expression size
	.byte	0x50	# DW_OP_reg0
	.quad	.LVL6-1	# Location list begin address (*.LLST2)
	.quad	.LVL6	# Location list end address (*.LLST2)
	.value	0xb	# Location expression size
	.byte	0x73	# DW_OP_breg3
	.sleb128 -2
	.byte	0x73	# DW_OP_breg3
	.sleb128 -2
	.byte	0x73	# DW_OP_breg3
	.sleb128 -2
	.byte	0x1e	# DW_OP_mul
	.byte	0x23	# DW_OP_plus_uconst
	.uleb128 0x1
	.byte	0x1e	# DW_OP_mul
	.byte	0x9f	# DW_OP_stack_value
	.quad	0	# Location list terminator begin (*.LLST2)
	.quad	0	# Location list terminator end (*.LLST2)
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
	.quad	.LFB3	# Address
	.quad	.LFE3-.LFB3	# Length
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
	.quad	.Ltext0	# Offset 0x30
	.quad	.Letext0
	.quad	.LFB3	# Offset 0x40
	.quad	.LFE3
	.quad	0
	.quad	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF0:
	.string	"GNU C 4.8.0 20121005+patches (experimental) -mtune=generic -march=x86-64 -g -O2"
.LASF1:
	.string	"gdb.arch/amd64-entry-value-inline.c"
.LASF2:
	.string	""
.LASF3:
	.string	"main"
	.ident	"GCC: (GNU) 4.8.0 20121005 (experimental)"
	.section	.note.GNU-stack,"",@progbits
