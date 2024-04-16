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

/* This file is compiled from gdb.arch/amd64-entry-value-param-dwarf5.c
   using -g -dA -S -O2.  */

	.file	"amd64-entry-value-param-dwarf5.c"
	.text
.Ltext0:
	.p2align 4,,15
	.type	foo.isra.0.constprop.2, @function
foo.isra.0.constprop.2:
.LFB4:
	/* gdb.arch/amd64-entry-value-param-dwarf5.c:21 */
.LM1:
	.cfi_startproc
.LVL0:
/* BLOCK 2 freq:10000 seq:0 */
/* PRED: ENTRY [100.0%]  (FALLTHRU) */
	/* gdb.arch/amd64-entry-value-param-dwarf5.c:26 */
.LM2:
	movl	vv(%rip), %eax
	addl	$1, %eax
	movl	%eax, vv(%rip)
	/* gdb.arch/amd64-entry-value-param-dwarf5.c:27 */
.LM3:
	leal	3(%rdi), %eax
/* SUCC: EXIT [100.0%]  */
	/* gdb.arch/amd64-entry-value-param-dwarf5.c:28 */
.LM4:
	ret
	.cfi_endproc
.LFE4:
	.size	foo.isra.0.constprop.2, .-foo.isra.0.constprop.2
	.p2align 4,,15
	.type	bar.constprop.1, @function
bar.constprop.1:
.LFB5:
	/* gdb.arch/amd64-entry-value-param-dwarf5.c:31 */
.LM5:
	.cfi_startproc
/* BLOCK 2 freq:10000 seq:0 */
/* PRED: ENTRY [100.0%]  (FALLTHRU) */
.LVL1:
	/* gdb.arch/amd64-entry-value-param-dwarf5.c:33 */
.LM6:
	movl	$10, %edi
	call	foo.isra.0.constprop.2
.LVL2:
	movl	$10, %edi
	movl	%eax, %edx
	call	foo.isra.0.constprop.2
.LVL3:
	movl	$16, %edi
	addl	%eax, %edx
	call	foo.isra.0.constprop.2
.LVL4:
	leal	10(%rdx,%rax), %eax
/* SUCC: EXIT [100.0%]  */
	/* gdb.arch/amd64-entry-value-param-dwarf5.c:34 */
.LM7:
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
	/* gdb.arch/amd64-entry-value-param-dwarf5.c:38 */
.LM8:
	.cfi_startproc
/* BLOCK 2 freq:10000 seq:0 */
/* PRED: ENTRY [100.0%]  (FALLTHRU) */
/* SUCC: EXIT [100.0%]  (ABNORMAL,SIBCALL) */
	/* gdb.arch/amd64-entry-value-param-dwarf5.c:39 */
.LM9:
	jmp	bar.constprop.1
.LVL5:
	.cfi_endproc
.LFE2:
	.size	main, .-main
	.comm	vv,4,4
	.text
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0x199	/* Length of Compilation Unit Info */
	.value	0x5	/* DWARF version number */
	.byte	0x1	/* DW_UT_compile */
	.byte	0x8	/* Pointer Size (in bytes) */
	.long	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.uleb128 0xa	/* (DIE (0xc) DW_TAG_compile_unit) */
	.long	.LASF2	/* DW_AT_producer: "GNU C11 7.0.1 20170218 (experimental) -mtune=generic -march=x86-64 -gdwarf-5 -O2" */
	.byte	0x1d	/* DW_AT_language */
	.long	.LASF0	/* DW_AT_name: "gdb.arch/amd64-entry-value-param-dwarf5.c" */
	.long	.LASF1	/* DW_AT_comp_dir: "" */
	.long	.LLRL0	/* DW_AT_ranges */
	.quad	0	/* DW_AT_low_pc */
	.long	.Ldebug_line0	/* DW_AT_stmt_list */
	.uleb128 0xb	/* (DIE (0x2a) DW_TAG_variable) */
	.ascii "vv\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (gdb.arch/amd64-entry-value-param-dwarf5.c) */
	.byte	0x12	/* DW_AT_decl_line */
	.long	0x45	/* DW_AT_type */
			/* DW_AT_external */
	.uleb128 0x9	/* DW_AT_location */
	.byte	0x3	/* DW_OP_addr */
	.quad	vv
	.uleb128 0xc	/* (DIE (0x3e) DW_TAG_base_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.byte	0x5	/* DW_AT_encoding */
	.ascii "int\0"	/* DW_AT_name */
	.uleb128 0xd	/* (DIE (0x45) DW_TAG_volatile_type) */
	.long	0x3e	/* DW_AT_type */
	.uleb128 0xe	/* (DIE (0x4a) DW_TAG_subprogram) */
			/* DW_AT_external */
	.long	.LASF3	/* DW_AT_name: "main" */
	.byte	0x1	/* DW_AT_decl_file (gdb.arch/amd64-entry-value-param-dwarf5.c) */
	.byte	0x25	/* DW_AT_decl_line */
			/* DW_AT_prototyped */
	.long	0x3e	/* DW_AT_type */
	.quad	.LFB2	/* DW_AT_low_pc */
	.quad	.LFE2-.LFB2	/* DW_AT_high_pc */
	.uleb128 0x1	/* DW_AT_frame_base */
	.byte	0x9c	/* DW_OP_call_frame_cfa */
			/* DW_AT_call_all_calls */
	.long	0x79	/* DW_AT_sibling */
	.uleb128 0xf	/* (DIE (0x6b) DW_TAG_call_site) */
	.quad	.LVL5	/* DW_AT_call_return_pc */
			/* DW_AT_call_tail_call */
	.long	0x125	/* DW_AT_call_origin */
	.byte	0	/* end of children of DIE 0x4a */
	.uleb128 0x5	/* (DIE (0x79) DW_TAG_subprogram) */
	.ascii "bar\0"	/* DW_AT_name */
			/* DW_AT_decl_file (1, gdb.arch/amd64-entry-value-param-dwarf5.c) */
	.byte	0x1f	/* DW_AT_decl_line */
			/* DW_AT_prototyped */
	.long	0x3e	/* DW_AT_type */
	.byte	0x1	/* DW_AT_inline */
	.long	0x91	/* DW_AT_sibling */
	.uleb128 0x1	/* (DIE (0x88) DW_TAG_formal_parameter) */
	.ascii "x\0"	/* DW_AT_name */
			/* DW_AT_decl_file (1, gdb.arch/amd64-entry-value-param-dwarf5.c) */
	.byte	0x1f	/* DW_AT_decl_line */
	.long	0x3e	/* DW_AT_type */
	.byte	0	/* end of children of DIE 0x79 */
	.uleb128 0x5	/* (DIE (0x91) DW_TAG_subprogram) */
	.ascii "foo\0"	/* DW_AT_name */
			/* DW_AT_decl_file (1, gdb.arch/amd64-entry-value-param-dwarf5.c) */
	.byte	0x15	/* DW_AT_decl_line */
			/* DW_AT_prototyped */
	.long	0x3e	/* DW_AT_type */
	.byte	0	/* DW_AT_inline */
	.long	0xd1	/* DW_AT_sibling */
	.uleb128 0x1	/* (DIE (0xa0) DW_TAG_formal_parameter) */
	.ascii "x\0"	/* DW_AT_name */
			/* DW_AT_decl_file (1, gdb.arch/amd64-entry-value-param-dwarf5.c) */
	.byte	0x15	/* DW_AT_decl_line */
	.long	0x3e	/* DW_AT_type */
	.uleb128 0x1	/* (DIE (0xa8) DW_TAG_formal_parameter) */
	.ascii "y\0"	/* DW_AT_name */
			/* DW_AT_decl_file (1, gdb.arch/amd64-entry-value-param-dwarf5.c) */
	.byte	0x15	/* DW_AT_decl_line */
	.long	0x3e	/* DW_AT_type */
	.uleb128 0x1	/* (DIE (0xb0) DW_TAG_formal_parameter) */
	.ascii "z\0"	/* DW_AT_name */
			/* DW_AT_decl_file (1, gdb.arch/amd64-entry-value-param-dwarf5.c) */
	.byte	0x15	/* DW_AT_decl_line */
	.long	0x3e	/* DW_AT_type */
	.uleb128 0x2	/* (DIE (0xb8) DW_TAG_variable) */
	.ascii "a\0"	/* DW_AT_name */
			/* DW_AT_decl_file (1, gdb.arch/amd64-entry-value-param-dwarf5.c) */
	.byte	0x17	/* DW_AT_decl_line */
	.long	0x3e	/* DW_AT_type */
	.uleb128 0x2	/* (DIE (0xc0) DW_TAG_variable) */
	.ascii "b\0"	/* DW_AT_name */
			/* DW_AT_decl_file (1, gdb.arch/amd64-entry-value-param-dwarf5.c) */
	.byte	0x18	/* DW_AT_decl_line */
	.long	0x3e	/* DW_AT_type */
	.uleb128 0x2	/* (DIE (0xc8) DW_TAG_variable) */
	.ascii "c\0"	/* DW_AT_name */
			/* DW_AT_decl_file (1, gdb.arch/amd64-entry-value-param-dwarf5.c) */
	.byte	0x19	/* DW_AT_decl_line */
	.long	0x3e	/* DW_AT_type */
	.byte	0	/* end of children of DIE 0x91 */
	.uleb128 0x10	/* (DIE (0xd1) DW_TAG_subprogram) */
	.long	0x91	/* DW_AT_abstract_origin */
	.quad	.LFB4	/* DW_AT_low_pc */
	.quad	.LFE4-.LFB4	/* DW_AT_high_pc */
	.uleb128 0x1	/* DW_AT_frame_base */
	.byte	0x9c	/* DW_OP_call_frame_cfa */
			/* DW_AT_call_all_calls */
	.long	0x125	/* DW_AT_sibling */
	.uleb128 0x6	/* (DIE (0xec) DW_TAG_formal_parameter) */
	.long	0xa0	/* DW_AT_abstract_origin */
	.uleb128 0x1	/* DW_AT_location */
	.byte	0x55	/* DW_OP_reg5 */
	.uleb128 0x6	/* (DIE (0xf3) DW_TAG_formal_parameter) */
	.long	0xa8	/* DW_AT_abstract_origin */
	.uleb128 0x6	/* DW_AT_location */
	.byte	0xfa	/* DW_OP_GNU_parameter_ref */
	.long	0xa8
	.byte	0x9f	/* DW_OP_stack_value */
	.uleb128 0x7	/* (DIE (0xff) DW_TAG_variable) */
	.long	0xb8	/* DW_AT_abstract_origin */
	.uleb128 0x5	/* DW_AT_location */
	.byte	0x75	/* DW_OP_breg5 */
	.sleb128 0
	.byte	0x31	/* DW_OP_lit1 */
	.byte	0x24	/* DW_OP_shl */
	.byte	0x9f	/* DW_OP_stack_value */
	.uleb128 0x7	/* (DIE (0x10a) DW_TAG_variable) */
	.long	0xc0	/* DW_AT_abstract_origin */
	.uleb128 0x8	/* DW_AT_location */
	.byte	0xfa	/* DW_OP_GNU_parameter_ref */
	.long	0xa8
	.byte	0x31	/* DW_OP_lit1 */
	.byte	0x24	/* DW_OP_shl */
	.byte	0x9f	/* DW_OP_stack_value */
	.uleb128 0x11	/* (DIE (0x118) DW_TAG_variable) */
	.long	0xc8	/* DW_AT_abstract_origin */
	.byte	0x6	/* DW_AT_const_value */
	.uleb128 0x8	/* (DIE (0x11e) DW_TAG_formal_parameter) */
	.long	0xb0	/* DW_AT_abstract_origin */
	.byte	0x3	/* DW_AT_const_value */
	.byte	0	/* end of children of DIE 0xd1 */
	.uleb128 0x12	/* (DIE (0x125) DW_TAG_subprogram) */
	.long	0x79	/* DW_AT_abstract_origin */
	.quad	.LFB5	/* DW_AT_low_pc */
	.quad	.LFE5-.LFB5	/* DW_AT_high_pc */
	.uleb128 0x1	/* DW_AT_frame_base */
	.byte	0x9c	/* DW_OP_call_frame_cfa */
			/* DW_AT_call_all_calls */
	.uleb128 0x8	/* (DIE (0x13c) DW_TAG_formal_parameter) */
	.long	0x88	/* DW_AT_abstract_origin */
	.byte	0xa	/* DW_AT_const_value */
	.uleb128 0x9	/* (DIE (0x142) DW_TAG_call_site) */
	.quad	.LVL2	/* DW_AT_call_return_pc */
	.long	0xd1	/* DW_AT_call_origin */
	.long	0x161	/* DW_AT_sibling */
	.uleb128 0x3	/* (DIE (0x153) DW_TAG_call_site_parameter) */
	.uleb128 0x1	/* DW_AT_location */
	.byte	0x55	/* DW_OP_reg5 */
	.uleb128 0x2	/* DW_AT_call_value */
	.byte	0x75	/* DW_OP_breg5 */
	.sleb128 0
	.uleb128 0x4	/* (DIE (0x159) DW_TAG_call_site_parameter) */
	.long	0xa8	/* DW_AT_call_parameter */
	.uleb128 0x1	/* DW_AT_call_value */
	.byte	0x32	/* DW_OP_lit2 */
	.byte	0	/* end of children of DIE 0x142 */
	.uleb128 0x9	/* (DIE (0x161) DW_TAG_call_site) */
	.quad	.LVL3	/* DW_AT_call_return_pc */
	.long	0xd1	/* DW_AT_call_origin */
	.long	0x180	/* DW_AT_sibling */
	.uleb128 0x3	/* (DIE (0x172) DW_TAG_call_site_parameter) */
	.uleb128 0x1	/* DW_AT_location */
	.byte	0x55	/* DW_OP_reg5 */
	.uleb128 0x2	/* DW_AT_call_value */
	.byte	0x75	/* DW_OP_breg5 */
	.sleb128 0
	.uleb128 0x4	/* (DIE (0x178) DW_TAG_call_site_parameter) */
	.long	0xa8	/* DW_AT_call_parameter */
	.uleb128 0x1	/* DW_AT_call_value */
	.byte	0x34	/* DW_OP_lit4 */
	.byte	0	/* end of children of DIE 0x161 */
	.uleb128 0x13	/* (DIE (0x180) DW_TAG_call_site) */
	.quad	.LVL4	/* DW_AT_call_return_pc */
	.long	0xd1	/* DW_AT_call_origin */
	.uleb128 0x3	/* (DIE (0x18d) DW_TAG_call_site_parameter) */
	.uleb128 0x1	/* DW_AT_location */
	.byte	0x55	/* DW_OP_reg5 */
	.uleb128 0x2	/* DW_AT_call_value */
	.byte	0x75	/* DW_OP_breg5 */
	.sleb128 0
	.uleb128 0x4	/* (DIE (0x193) DW_TAG_call_site_parameter) */
	.long	0xa8	/* DW_AT_call_parameter */
	.uleb128 0x1	/* DW_AT_call_value */
	.byte	0x3a	/* DW_OP_lit10 */
	.byte	0	/* end of children of DIE 0x180 */
	.byte	0	/* end of children of DIE 0x125 */
	.byte	0	/* end of children of DIE 0xc */
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1	/* (abbrev code) */
	.uleb128 0x5	/* (TAG: DW_TAG_formal_parameter) */
	.byte	0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0x21	/* (DW_FORM_implicit_const) */
	.sleb128 1	/* (gdb.arch/amd64-entry-value-param-dwarf5.c) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0x2	/* (abbrev code) */
	.uleb128 0x34	/* (TAG: DW_TAG_variable) */
	.byte	0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0x21	/* (DW_FORM_implicit_const) */
	.sleb128 1	/* (gdb.arch/amd64-entry-value-param-dwarf5.c) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0x3	/* (abbrev code) */
	.uleb128 0x49	/* (TAG: DW_TAG_call_site_parameter) */
	.byte	0	/* DW_children_no */
	.uleb128 0x2	/* (DW_AT_location) */
	.uleb128 0x18	/* (DW_FORM_exprloc) */
	.uleb128 0x7e	/* (DW_AT_call_value) */
	.uleb128 0x18	/* (DW_FORM_exprloc) */
	.byte	0
	.byte	0
	.uleb128 0x4	/* (abbrev code) */
	.uleb128 0x49	/* (TAG: DW_TAG_call_site_parameter) */
	.byte	0	/* DW_children_no */
	.uleb128 0x80	/* (DW_AT_call_parameter) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x7e	/* (DW_AT_call_value) */
	.uleb128 0x18	/* (DW_FORM_exprloc) */
	.byte	0
	.byte	0
	.uleb128 0x5	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0x21	/* (DW_FORM_implicit_const) */
	.sleb128 1	/* (gdb.arch/amd64-entry-value-param-dwarf5.c) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x27	/* (DW_AT_prototyped) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x20	/* (DW_AT_inline) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0x6	/* (abbrev code) */
	.uleb128 0x5	/* (TAG: DW_TAG_formal_parameter) */
	.byte	0	/* DW_children_no */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x2	/* (DW_AT_location) */
	.uleb128 0x18	/* (DW_FORM_exprloc) */
	.byte	0
	.byte	0
	.uleb128 0x7	/* (abbrev code) */
	.uleb128 0x34	/* (TAG: DW_TAG_variable) */
	.byte	0	/* DW_children_no */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x2	/* (DW_AT_location) */
	.uleb128 0x18	/* (DW_FORM_exprloc) */
	.byte	0
	.byte	0
	.uleb128 0x8	/* (abbrev code) */
	.uleb128 0x5	/* (TAG: DW_TAG_formal_parameter) */
	.byte	0	/* DW_children_no */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x1c	/* (DW_AT_const_value) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0
	.byte	0
	.uleb128 0x9	/* (abbrev code) */
	.uleb128 0x48	/* (TAG: DW_TAG_call_site) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x7d	/* (DW_AT_call_return_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x7f	/* (DW_AT_call_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0xa	/* (abbrev code) */
	.uleb128 0x11	/* (TAG: DW_TAG_compile_unit) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x25	/* (DW_AT_producer) */
	.uleb128 0xe	/* (DW_FORM_strp) */
	.uleb128 0x13	/* (DW_AT_language) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x1f	/* (DW_FORM_line_strp) */
	.uleb128 0x1b	/* (DW_AT_comp_dir) */
	.uleb128 0x1f	/* (DW_FORM_line_strp) */
	.uleb128 0x55	/* (DW_AT_ranges) */
	.uleb128 0x17	/* (DW_FORM_sec_offset) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x10	/* (DW_AT_stmt_list) */
	.uleb128 0x17	/* (DW_FORM_sec_offset) */
	.byte	0
	.byte	0
	.uleb128 0xb	/* (abbrev code) */
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
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.uleb128 0x2	/* (DW_AT_location) */
	.uleb128 0x18	/* (DW_FORM_exprloc) */
	.byte	0
	.byte	0
	.uleb128 0xc	/* (abbrev code) */
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
	.uleb128 0xd	/* (abbrev code) */
	.uleb128 0x35	/* (TAG: DW_TAG_volatile_type) */
	.byte	0	/* DW_children_no */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0xe	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0xe	/* (DW_FORM_strp) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x27	/* (DW_AT_prototyped) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x7	/* (DW_FORM_data8) */
	.uleb128 0x40	/* (DW_AT_frame_base) */
	.uleb128 0x18	/* (DW_FORM_exprloc) */
	.uleb128 0x7a	/* (DW_AT_call_all_calls) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0xf	/* (abbrev code) */
	.uleb128 0x48	/* (TAG: DW_TAG_call_site) */
	.byte	0	/* DW_children_no */
	.uleb128 0x7d	/* (DW_AT_call_return_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x82	/* (DW_AT_call_tail_call) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.uleb128 0x7f	/* (DW_AT_call_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0x10	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x7	/* (DW_FORM_data8) */
	.uleb128 0x40	/* (DW_AT_frame_base) */
	.uleb128 0x18	/* (DW_FORM_exprloc) */
	.uleb128 0x7a	/* (DW_AT_call_all_calls) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.uleb128 0x11	/* (abbrev code) */
	.uleb128 0x34	/* (TAG: DW_TAG_variable) */
	.byte	0	/* DW_children_no */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x1c	/* (DW_AT_const_value) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0
	.byte	0
	.uleb128 0x12	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x7	/* (DW_FORM_data8) */
	.uleb128 0x40	/* (DW_AT_frame_base) */
	.uleb128 0x18	/* (DW_FORM_exprloc) */
	.uleb128 0x7a	/* (DW_AT_call_all_calls) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.byte	0
	.byte	0
	.uleb128 0x13	/* (abbrev code) */
	.uleb128 0x48	/* (TAG: DW_TAG_call_site) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x7d	/* (DW_AT_call_return_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x7f	/* (DW_AT_call_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.long	0x3c	/* Length of Address Ranges Info */
	.value	0x2	/* DWARF Version */
	.long	.Ldebug_info0	/* Offset of Compilation Unit Info */
	.byte	0x8	/* Size of Address */
	.byte	0	/* Size of Segment Descriptor */
	.value	0	/* Pad to 16 byte boundary */
	.value	0
	.quad	.Ltext0	/* Address */
	.quad	.Letext0-.Ltext0	/* Length */
	.quad	.LFB2	/* Address */
	.quad	.LFE2-.LFB2	/* Length */
	.quad	0
	.quad	0
	.section	.debug_rnglists,"",@progbits
.Ldebug_ranges0:
	.long	.Ldebug_ranges3-.Ldebug_ranges2	/* Length of Range Lists */
.Ldebug_ranges2:
	.value	0x5	/* DWARF Version */
	.byte	0x8	/* Address Size */
	.byte	0	/* Segment Size */
	.long	0	/* Offset Entry Count */
.LLRL0:
	.byte	0x7	/* DW_RLE_start_length (*.LLRL0) */
	.quad	.Ltext0	/* Range begin address (*.LLRL0) */
	.uleb128 .Letext0-.Ltext0	/* Range length (*.LLRL0) */
	.byte	0x7	/* DW_RLE_start_length (*.LLRL0) */
	.quad	.LFB2	/* Range begin address (*.LLRL0) */
	.uleb128 .LFE2-.LFB2	/* Range length (*.LLRL0) */
	.byte	0	/* DW_RLE_end_of_list (*.LLRL0) */
.Ldebug_ranges3:
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.long	.LELT0-.LSLT0	/* Length of Source Line Info */
.LSLT0:
	.value	0x5	/* DWARF Version */
	.byte	0x8	/* Address Size */
	.byte	0	/* Segment Size */
	.long	.LELTP0-.LASLTP0	/* Prolog Length */
.LASLTP0:
	.byte	0x1	/* Minimum Instruction Length */
	.byte	0x1	/* Maximum Operations Per Instruction */
	.byte	0x1	/* Default is_stmt_start flag */
	.byte	0xf6	/* Line Base Value (Special Opcodes) */
	.byte	0xf2	/* Line Range Value (Special Opcodes) */
	.byte	0xd	/* Special Opcode Base */
	.byte	0	/* opcode: 0x1 has 0 args */
	.byte	0x1	/* opcode: 0x2 has 1 args */
	.byte	0x1	/* opcode: 0x3 has 1 args */
	.byte	0x1	/* opcode: 0x4 has 1 args */
	.byte	0x1	/* opcode: 0x5 has 1 args */
	.byte	0	/* opcode: 0x6 has 0 args */
	.byte	0	/* opcode: 0x7 has 0 args */
	.byte	0	/* opcode: 0x8 has 0 args */
	.byte	0x1	/* opcode: 0x9 has 1 args */
	.byte	0	/* opcode: 0xa has 0 args */
	.byte	0	/* opcode: 0xb has 0 args */
	.byte	0x1	/* opcode: 0xc has 1 args */
	.byte	0x1	/* Directory entry format count */
	.uleb128 0x1	/* DW_LNCT_path */
	.uleb128 0x1f	/* DW_FORM_line_strp */
	.uleb128 0x2	/* Directories count */
	.long	.LASF1	/* Directory Entry: 0: "" */
	.long	.LASF4	/* Directory Entry: 0: "gdb.arch" */
	.byte	0x2	/* File name entry format count */
	.uleb128 0x1	/* DW_LNCT_path */
	.uleb128 0x1f	/* DW_FORM_line_strp */
	.uleb128 0x2	/* DW_LNCT_directory_index */
	.uleb128 0xb	/* DW_FORM_data1 */
	.uleb128 0x2	/* File names count */
	.long	.LASF0	/* File Entry: 0: "gdb.arch/amd64-entry-value-param-dwarf5.c" */
	.byte	0
	.long	.LASF5	/* File Entry: 0: "amd64-entry-value-param-dwarf5.c" */
	.byte	0x1
.LELTP0:
	.byte	0	/* set address *.LM8 */
	.uleb128 0x9
	.byte	0x2
	.quad	.LM8
	.byte	0x3c	/* line 38 */
	.byte	0	/* set address *.LM9 */
	.uleb128 0x9
	.byte	0x2
	.quad	.LM9
	.byte	0x18	/* line 39 */
	.byte	0	/* set address *.LFE2 */
	.uleb128 0x9
	.byte	0x2
	.quad	.LFE2
	.byte	0	/* end sequence */
	.uleb128 0x1
	.byte	0x1
	.byte	0	/* set address *.LM1 */
	.uleb128 0x9
	.byte	0x2
	.quad	.LM1
	.byte	0x2b	/* line 21 */
	.byte	0	/* set address *.LM2 */
	.uleb128 0x9
	.byte	0x2
	.quad	.LM2
	.byte	0x1c	/* line 26 */
	.byte	0	/* set address *.LM3 */
	.uleb128 0x9
	.byte	0x2
	.quad	.LM3
	.byte	0x18	/* line 27 */
	.byte	0	/* set address *.LM4 */
	.uleb128 0x9
	.byte	0x2
	.quad	.LM4
	.byte	0x18	/* line 28 */
	.byte	0	/* set address *.LM5 */
	.uleb128 0x9
	.byte	0x2
	.quad	.LM5
	.byte	0x1a	/* line 31 */
	.byte	0	/* set address *.LM6 */
	.uleb128 0x9
	.byte	0x2
	.quad	.LM6
	.byte	0x19	/* line 33 */
	.byte	0	/* set address *.LM7 */
	.uleb128 0x9
	.byte	0x2
	.quad	.LM7
	.byte	0x18	/* line 34 */
	.byte	0	/* set address *.Letext0 */
	.uleb128 0x9
	.byte	0x2
	.quad	.Letext0
	.byte	0	/* end sequence */
	.uleb128 0x1
	.byte	0x1
.LELT0:
	.section	.debug_str,"MS",@progbits,1
.LASF2:
	.string	"GNU C11 7.0.1 20170218 (experimental) -mtune=generic -march=x86-64 -gdwarf-5 -O2"
.LASF3:
	.string	"main"
	.section	.debug_line_str,"MS",@progbits,1
.LASF5:
	.string	"amd64-entry-value-param-dwarf5.c"
.LASF0:
	.string	"gdb.arch/amd64-entry-value-param-dwarf5.c"
.LASF1:
	.string	""
.LASF4:
	.string	"gdb.arch"
	.ident	"GCC: (GNU) 7.0.1 20170218 (experimental)"
	.section	.note.GNU-stack,"",@progbits
