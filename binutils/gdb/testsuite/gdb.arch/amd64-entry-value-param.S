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

	.file	"amd64-entry-value-param.c"
	.text
.Ltext0:
	.p2align 4,,15
	.type	foo.isra.0.constprop.2, @function
foo.isra.0.constprop.2:
.LFB4:
	.file 1 "gdb.arch/amd64-entry-value-param.c"
	/* gdb.arch/amd64-entry-value-param.c:21 */
	.loc 1 21 0
	.cfi_startproc
.LVL0:
/* BLOCK 2 freq:10000 seq:0 */
/* PRED: ENTRY [100.0%]  (fallthru) */
	/* gdb.arch/amd64-entry-value-param.c:26 */
	.loc 1 26 0
	movl	vv(%rip), %eax
	addl	$1, %eax
	movl	%eax, vv(%rip)
	/* gdb.arch/amd64-entry-value-param.c:27 */
	.loc 1 27 0
	leal	3(%rdi), %eax
/* SUCC: EXIT [100.0%]  */
	/* gdb.arch/amd64-entry-value-param.c:28 */
	.loc 1 28 0
	ret
	.cfi_endproc
.LFE4:
	.size	foo.isra.0.constprop.2, .-foo.isra.0.constprop.2
	.p2align 4,,15
	.type	bar.constprop.1, @function
bar.constprop.1:
.LFB5:
	/* gdb.arch/amd64-entry-value-param.c:31 */
	.loc 1 31 0
	.cfi_startproc
/* BLOCK 2 freq:10000 seq:0 */
/* PRED: ENTRY [100.0%]  (fallthru) */
.LVL1:
	pushq	%rbx
.LCFI0:
	.cfi_def_cfa_offset 16
	.cfi_offset 3, -16
	/* gdb.arch/amd64-entry-value-param.c:33 */
	.loc 1 33 0
	movl	$10, %edi
	call	foo.isra.0.constprop.2
.LVL2:
	movl	$10, %edi
	movl	%eax, %ebx
	call	foo.isra.0.constprop.2
.LVL3:
	movl	$16, %edi
	addl	%eax, %ebx
	call	foo.isra.0.constprop.2
.LVL4:
	leal	10(%rbx,%rax), %eax
	/* gdb.arch/amd64-entry-value-param.c:34 */
	.loc 1 34 0
	popq	%rbx
.LCFI1:
	.cfi_def_cfa_offset 8
/* SUCC: EXIT [100.0%]  */
	ret
	.cfi_endproc
.LFE5:
	.size	bar.constprop.1, .-bar.constprop.1
	.section	.text.startup,"ax",@progbits
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB2:
	/* gdb.arch/amd64-entry-value-param.c:38 */
	.loc 1 38 0
	.cfi_startproc
/* BLOCK 2 freq:10000 seq:0 */
/* PRED: ENTRY [100.0%]  (fallthru) */
	/* gdb.arch/amd64-entry-value-param.c:39 */
	.loc 1 39 0
	jmp	bar.constprop.1
/* SUCC: EXIT [100.0%]  (ab,sibcall) */
.LVL5:
	.cfi_endproc
.LFE2:
	.size	main, .-main
	.comm	vv,4,4
	.text
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0x1b7	/* Length of Compilation Unit Info */
	.2byte	0x2	/* DWARF version number */
	.4byte	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	0x8	/* Pointer Size (in bytes) */
	.uleb128 0x1	/* (DIE (0xb) DW_TAG_compile_unit) */
	.4byte	.LASF0	/* DW_AT_producer: "GNU C 4.7.1 20120612 (prerelease)" */
	.byte	0x1	/* DW_AT_language */
	.4byte	.LASF1	/* DW_AT_name: "gdb.arch/amd64-entry-value-param.c" */
	.4byte	.LASF2	/* DW_AT_comp_dir: "" */
	.4byte	.Ldebug_ranges0+0	/* DW_AT_ranges */
	.quad	0	/* DW_AT_low_pc */
	.quad	0	/* DW_AT_entry_pc */
	.4byte	.Ldebug_line0	/* DW_AT_stmt_list */
	.uleb128 0x2	/* (DIE (0x31) DW_TAG_subprogram) */
	.ascii "foo\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (gdb.arch/amd64-entry-value-param.c) */
	.byte	0x15	/* DW_AT_decl_line */
	.byte	0x1	/* DW_AT_prototyped */
	.4byte	0x79	/* DW_AT_type */
	.byte	0	/* DW_AT_inline */
	.4byte	0x79	/* DW_AT_sibling */
	.uleb128 0x3	/* (DIE (0x42) DW_TAG_formal_parameter) */
	.ascii "x\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (gdb.arch/amd64-entry-value-param.c) */
	.byte	0x15	/* DW_AT_decl_line */
	.4byte	0x79	/* DW_AT_type */
	.uleb128 0x3	/* (DIE (0x4b) DW_TAG_formal_parameter) */
	.ascii "y\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (gdb.arch/amd64-entry-value-param.c) */
	.byte	0x15	/* DW_AT_decl_line */
	.4byte	0x79	/* DW_AT_type */
	.uleb128 0x3	/* (DIE (0x54) DW_TAG_formal_parameter) */
	.ascii "z\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (gdb.arch/amd64-entry-value-param.c) */
	.byte	0x15	/* DW_AT_decl_line */
	.4byte	0x79	/* DW_AT_type */
	.uleb128 0x4	/* (DIE (0x5d) DW_TAG_variable) */
	.ascii "a\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (gdb.arch/amd64-entry-value-param.c) */
	.byte	0x17	/* DW_AT_decl_line */
	.4byte	0x79	/* DW_AT_type */
	.uleb128 0x4	/* (DIE (0x66) DW_TAG_variable) */
	.ascii "b\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (gdb.arch/amd64-entry-value-param.c) */
	.byte	0x18	/* DW_AT_decl_line */
	.4byte	0x79	/* DW_AT_type */
	.uleb128 0x4	/* (DIE (0x6f) DW_TAG_variable) */
	.ascii "c\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (gdb.arch/amd64-entry-value-param.c) */
	.byte	0x19	/* DW_AT_decl_line */
	.4byte	0x79	/* DW_AT_type */
	.byte	0	/* end of children of DIE 0x31 */
	.uleb128 0x5	/* (DIE (0x79) DW_TAG_base_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.byte	0x5	/* DW_AT_encoding */
	.ascii "int\0"	/* DW_AT_name */
	.uleb128 0x2	/* (DIE (0x80) DW_TAG_subprogram) */
	.ascii "bar\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (gdb.arch/amd64-entry-value-param.c) */
	.byte	0x1f	/* DW_AT_decl_line */
	.byte	0x1	/* DW_AT_prototyped */
	.4byte	0x79	/* DW_AT_type */
	.byte	0x1	/* DW_AT_inline */
	.4byte	0x9b	/* DW_AT_sibling */
	.uleb128 0x3	/* (DIE (0x91) DW_TAG_formal_parameter) */
	.ascii "x\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (gdb.arch/amd64-entry-value-param.c) */
	.byte	0x1f	/* DW_AT_decl_line */
	.4byte	0x79	/* DW_AT_type */
	.byte	0	/* end of children of DIE 0x80 */
	.uleb128 0x6	/* (DIE (0x9b) DW_TAG_subprogram) */
	.4byte	0x31	/* DW_AT_abstract_origin */
	.quad	.LFB4	/* DW_AT_low_pc */
	.quad	.LFE4	/* DW_AT_high_pc */
	.byte	0x2	/* DW_AT_frame_base */
	.byte	0x77	/* DW_OP_breg7 */
	.sleb128 8
	.byte	0x1	/* DW_AT_GNU_all_call_sites */
	.4byte	0xf1	/* DW_AT_sibling */
	.uleb128 0x7	/* (DIE (0xb8) DW_TAG_formal_parameter) */
	.4byte	0x42	/* DW_AT_abstract_origin */
	.byte	0x1	/* DW_AT_location */
	.byte	0x55	/* DW_OP_reg5 */
	.uleb128 0x7	/* (DIE (0xbf) DW_TAG_formal_parameter) */
	.4byte	0x4b	/* DW_AT_abstract_origin */
	.byte	0x6	/* DW_AT_location */
	.byte	0xfa	/* DW_OP_GNU_parameter_ref */
	.4byte	0x4b
	.byte	0x9f	/* DW_OP_stack_value */
	.uleb128 0x8	/* (DIE (0xcb) DW_TAG_variable) */
	.4byte	0x5d	/* DW_AT_abstract_origin */
	.byte	0x5	/* DW_AT_location */
	.byte	0x75	/* DW_OP_breg5 */
	.sleb128 0
	.byte	0x31	/* DW_OP_lit1 */
	.byte	0x24	/* DW_OP_shl */
	.byte	0x9f	/* DW_OP_stack_value */
	.uleb128 0x8	/* (DIE (0xd6) DW_TAG_variable) */
	.4byte	0x66	/* DW_AT_abstract_origin */
	.byte	0x8	/* DW_AT_location */
	.byte	0xfa	/* DW_OP_GNU_parameter_ref */
	.4byte	0x4b
	.byte	0x31	/* DW_OP_lit1 */
	.byte	0x24	/* DW_OP_shl */
	.byte	0x9f	/* DW_OP_stack_value */
	.uleb128 0x9	/* (DIE (0xe4) DW_TAG_variable) */
	.4byte	0x6f	/* DW_AT_abstract_origin */
	.byte	0x6	/* DW_AT_const_value */
	.uleb128 0xa	/* (DIE (0xea) DW_TAG_formal_parameter) */
	.4byte	0x54	/* DW_AT_abstract_origin */
	.byte	0x3	/* DW_AT_const_value */
	.byte	0	/* end of children of DIE 0x9b */
	.uleb128 0xb	/* (DIE (0xf1) DW_TAG_subprogram) */
	.4byte	0x80	/* DW_AT_abstract_origin */
	.quad	.LFB5	/* DW_AT_low_pc */
	.quad	.LFE5	/* DW_AT_high_pc */
	.4byte	.LLST0	/* DW_AT_frame_base */
	.byte	0x1	/* DW_AT_GNU_all_call_sites */
	.4byte	0x16c	/* DW_AT_sibling */
	.uleb128 0xa	/* (DIE (0x10f) DW_TAG_formal_parameter) */
	.4byte	0x91	/* DW_AT_abstract_origin */
	.byte	0xa	/* DW_AT_const_value */
	.uleb128 0xc	/* (DIE (0x115) DW_TAG_GNU_call_site) */
	.quad	.LVL2	/* DW_AT_low_pc */
	.4byte	0x9b	/* DW_AT_abstract_origin */
	.4byte	0x133	/* DW_AT_sibling */
	.uleb128 0xd	/* (DIE (0x126) DW_TAG_GNU_call_site_parameter) */
	.byte	0x1	/* DW_AT_location */
	.byte	0x55	/* DW_OP_reg5 */
	.byte	0x1	/* DW_AT_GNU_call_site_value */
	.byte	0x3a	/* DW_OP_lit10 */
	.uleb128 0xe	/* (DIE (0x12b) DW_TAG_GNU_call_site_parameter) */
	.4byte	0x4b	/* DW_AT_abstract_origin */
	.byte	0x1	/* DW_AT_GNU_call_site_value */
	.byte	0x32	/* DW_OP_lit2 */
	.byte	0	/* end of children of DIE 0x115 */
	.uleb128 0xc	/* (DIE (0x133) DW_TAG_GNU_call_site) */
	.quad	.LVL3	/* DW_AT_low_pc */
	.4byte	0x9b	/* DW_AT_abstract_origin */
	.4byte	0x151	/* DW_AT_sibling */
	.uleb128 0xd	/* (DIE (0x144) DW_TAG_GNU_call_site_parameter) */
	.byte	0x1	/* DW_AT_location */
	.byte	0x55	/* DW_OP_reg5 */
	.byte	0x1	/* DW_AT_GNU_call_site_value */
	.byte	0x3a	/* DW_OP_lit10 */
	.uleb128 0xe	/* (DIE (0x149) DW_TAG_GNU_call_site_parameter) */
	.4byte	0x4b	/* DW_AT_abstract_origin */
	.byte	0x1	/* DW_AT_GNU_call_site_value */
	.byte	0x34	/* DW_OP_lit4 */
	.byte	0	/* end of children of DIE 0x133 */
	.uleb128 0xf	/* (DIE (0x151) DW_TAG_GNU_call_site) */
	.quad	.LVL4	/* DW_AT_low_pc */
	.4byte	0x9b	/* DW_AT_abstract_origin */
	.uleb128 0xd	/* (DIE (0x15e) DW_TAG_GNU_call_site_parameter) */
	.byte	0x1	/* DW_AT_location */
	.byte	0x55	/* DW_OP_reg5 */
	.byte	0x1	/* DW_AT_GNU_call_site_value */
	.byte	0x40	/* DW_OP_lit16 */
	.uleb128 0xe	/* (DIE (0x163) DW_TAG_GNU_call_site_parameter) */
	.4byte	0x4b	/* DW_AT_abstract_origin */
	.byte	0x1	/* DW_AT_GNU_call_site_value */
	.byte	0x3a	/* DW_OP_lit10 */
	.byte	0	/* end of children of DIE 0x151 */
	.byte	0	/* end of children of DIE 0xf1 */
	.uleb128 0x10	/* (DIE (0x16c) DW_TAG_subprogram) */
	.byte	0x1	/* DW_AT_external */
	.4byte	.LASF3	/* DW_AT_name: "main" */
	.byte	0x1	/* DW_AT_decl_file (gdb.arch/amd64-entry-value-param.c) */
	.byte	0x25	/* DW_AT_decl_line */
	.byte	0x1	/* DW_AT_prototyped */
	.4byte	0x79	/* DW_AT_type */
	.quad	.LFB2	/* DW_AT_low_pc */
	.quad	.LFE2	/* DW_AT_high_pc */
	.byte	0x2	/* DW_AT_frame_base */
	.byte	0x77	/* DW_OP_breg7 */
	.sleb128 8
	.byte	0x1	/* DW_AT_GNU_all_call_sites */
	.4byte	0x1a0	/* DW_AT_sibling */
	.uleb128 0x11	/* (DIE (0x191) DW_TAG_GNU_call_site) */
	.quad	.LVL5	/* DW_AT_low_pc */
	.byte	0x1	/* DW_AT_GNU_tail_call */
	.4byte	0xf1	/* DW_AT_abstract_origin */
	.byte	0	/* end of children of DIE 0x16c */
	.uleb128 0x12	/* (DIE (0x1a0) DW_TAG_variable) */
	.ascii "vv\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (gdb.arch/amd64-entry-value-param.c) */
	.byte	0x12	/* DW_AT_decl_line */
	.4byte	0x1b5	/* DW_AT_type */
	.byte	0x1	/* DW_AT_external */
	.byte	0x9	/* DW_AT_location */
	.byte	0x3	/* DW_OP_addr */
	.quad	vv
	.uleb128 0x13	/* (DIE (0x1b5) DW_TAG_volatile_type) */
	.4byte	0x79	/* DW_AT_type */
	.byte	0	/* end of children of DIE 0xb */
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1	/* (abbrev code) */
	.uleb128 0x11	/* (TAG: DW_TAG_compile_unit) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x25	/* (DW_AT_producer) */
	.uleb128 0xe	/* (DW_FORM_strp) */
	.uleb128 0x13	/* (DW_AT_language) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0xe	/* (DW_FORM_strp) */
	.uleb128 0x1b	/* (DW_AT_comp_dir) */
	.uleb128 0xe	/* (DW_FORM_strp) */
	.uleb128 0x55	/* (DW_AT_ranges) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x52	/* (DW_AT_entry_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x10	/* (DW_AT_stmt_list) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.byte	0
	.byte	0
	.uleb128 0x2	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x27	/* (DW_AT_prototyped) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x20	/* (DW_AT_inline) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0x3	/* (abbrev code) */
	.uleb128 0x5	/* (TAG: DW_TAG_formal_parameter) */
	.byte	0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0x4	/* (abbrev code) */
	.uleb128 0x34	/* (TAG: DW_TAG_variable) */
	.byte	0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0x5	/* (abbrev code) */
	.uleb128 0x24	/* (TAG: DW_TAG_base_type) */
	.byte	0	/* DW_children_no */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3e	/* (DW_AT_encoding) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.byte	0
	.byte	0
	.uleb128 0x6	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x40	/* (DW_AT_frame_base) */
	.uleb128 0xa	/* (DW_FORM_block1) */
	.uleb128 0x2117	/* (DW_AT_GNU_all_call_sites) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0x7	/* (abbrev code) */
	.uleb128 0x5	/* (TAG: DW_TAG_formal_parameter) */
	.byte	0	/* DW_children_no */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x2	/* (DW_AT_location) */
	.uleb128 0xa	/* (DW_FORM_block1) */
	.byte	0
	.byte	0
	.uleb128 0x8	/* (abbrev code) */
	.uleb128 0x34	/* (TAG: DW_TAG_variable) */
	.byte	0	/* DW_children_no */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x2	/* (DW_AT_location) */
	.uleb128 0xa	/* (DW_FORM_block1) */
	.byte	0
	.byte	0
	.uleb128 0x9	/* (abbrev code) */
	.uleb128 0x34	/* (TAG: DW_TAG_variable) */
	.byte	0	/* DW_children_no */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x1c	/* (DW_AT_const_value) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0
	.byte	0
	.uleb128 0xa	/* (abbrev code) */
	.uleb128 0x5	/* (TAG: DW_TAG_formal_parameter) */
	.byte	0	/* DW_children_no */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x1c	/* (DW_AT_const_value) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0
	.byte	0
	.uleb128 0xb	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x40	/* (DW_AT_frame_base) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.uleb128 0x2117	/* (DW_AT_GNU_all_call_sites) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0xc	/* (abbrev code) */
	.uleb128 0x4109	/* (TAG: DW_TAG_GNU_call_site) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0xd	/* (abbrev code) */
	.uleb128 0x410a	/* (TAG: DW_TAG_GNU_call_site_parameter) */
	.byte	0	/* DW_children_no */
	.uleb128 0x2	/* (DW_AT_location) */
	.uleb128 0xa	/* (DW_FORM_block1) */
	.uleb128 0x2111	/* (DW_AT_GNU_call_site_value) */
	.uleb128 0xa	/* (DW_FORM_block1) */
	.byte	0
	.byte	0
	.uleb128 0xe	/* (abbrev code) */
	.uleb128 0x410a	/* (TAG: DW_TAG_GNU_call_site_parameter) */
	.byte	0	/* DW_children_no */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x2111	/* (DW_AT_GNU_call_site_value) */
	.uleb128 0xa	/* (DW_FORM_block1) */
	.byte	0
	.byte	0
	.uleb128 0xf	/* (abbrev code) */
	.uleb128 0x4109	/* (TAG: DW_TAG_GNU_call_site) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0x10	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0xe	/* (DW_FORM_strp) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x27	/* (DW_AT_prototyped) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x40	/* (DW_AT_frame_base) */
	.uleb128 0xa	/* (DW_FORM_block1) */
	.uleb128 0x2117	/* (DW_AT_GNU_all_call_sites) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0x11	/* (abbrev code) */
	.uleb128 0x4109	/* (TAG: DW_TAG_GNU_call_site) */
	.byte	0	/* DW_children_no */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x2115	/* (DW_AT_GNU_tail_call) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0x12	/* (abbrev code) */
	.uleb128 0x34	/* (TAG: DW_TAG_variable) */
	.byte	0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x2	/* (DW_AT_location) */
	.uleb128 0xa	/* (DW_FORM_block1) */
	.byte	0
	.byte	0
	.uleb128 0x13	/* (abbrev code) */
	.uleb128 0x35	/* (TAG: DW_TAG_volatile_type) */
	.byte	0	/* DW_children_no */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST0:
	.quad	.LFB5	/* Location list begin address (*.LLST0) */
	.quad	.LCFI0	/* Location list end address (*.LLST0) */
	.2byte	0x2	/* Location expression size */
	.byte	0x77	/* DW_OP_breg7 */
	.sleb128 8
	.quad	.LCFI0	/* Location list begin address (*.LLST0) */
	.quad	.LCFI1	/* Location list end address (*.LLST0) */
	.2byte	0x2	/* Location expression size */
	.byte	0x77	/* DW_OP_breg7 */
	.sleb128 16
	.quad	.LCFI1	/* Location list begin address (*.LLST0) */
	.quad	.LFE5	/* Location list end address (*.LLST0) */
	.2byte	0x2	/* Location expression size */
	.byte	0x77	/* DW_OP_breg7 */
	.sleb128 8
	.quad	0	/* Location list terminator begin (*.LLST0) */
	.quad	0	/* Location list terminator end (*.LLST0) */
	.section	.debug_aranges,"",@progbits
	.4byte	0x3c	/* Length of Address Ranges Info */
	.2byte	0x2	/* DWARF Version */
	.4byte	.Ldebug_info0	/* Offset of Compilation Unit Info */
	.byte	0x8	/* Size of Address */
	.byte	0	/* Size of Segment Descriptor */
	.2byte	0	/* Pad to 16 byte boundary */
	.2byte	0
	.quad	.Ltext0	/* Address */
	.quad	.Letext0-.Ltext0	/* Length */
	.quad	.LFB2	/* Address */
	.quad	.LFE2-.LFB2	/* Length */
	.quad	0
	.quad	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.quad	.Ltext0	/* Offset 0 */
	.quad	.Letext0
	.quad	.LFB2	/* Offset 0x10 */
	.quad	.LFE2
	.quad	0
	.quad	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF1:
	.string	"gdb.arch/amd64-entry-value-param.c"
.LASF3:
	.string	"main"
.LASF2:
	.string	""
.LASF0:
	.string	"GNU C 4.7.1 20120612 (prerelease)"
	.ident	"GCC: (GNU) 4.7.1 20120612 (prerelease)"
	.section	.note.GNU-stack,"",@progbits
