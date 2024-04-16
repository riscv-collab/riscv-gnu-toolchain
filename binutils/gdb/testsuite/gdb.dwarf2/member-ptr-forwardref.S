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

/* The goal of this test is to provide anonymous DW_TAG_structure_type
   containing __pfn + __delta (read in by quirk_gcc_member_function_pointer)
   before DW_TAG_structure_type of "class C" to exploit the forward reference
   through "__pfn" and back from DW_TAG_member "fp" of "class C".  As the
   compiler may place the DIEs in arbitrary order they are rather prepared as
   a .S file.

   class C;
   typedef int (C::*fp_t) ();
   fp_t fp;
   class C
     {
       int (C::*fp) ();
     } c;  */

	.file	"member-ptr-forwardref.cc"
	.section	.debug_abbrev,"",%progbits
.Ldebug_abbrev0:
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.section	.debug_line,"",%progbits
.Ldebug_line0:
	.text
.Ltext0:
.globl fp
	.data
	.align 4
	.type	fp, %object
	.size	fp, 8
fp:
	.zero	8
.globl c
	.align 4
	.type	c, %object
	.size	c, 8
c:
	.zero	8
	.text
.Letext0:
	.file 1 "gdb.dwarf2/member-ptr-forwardref.cc"
	.section	.debug_info
	.4byte	0xa9	/* Length of Compilation Unit Info */
	.2byte	0x3	/* DWARF version number */
	.4byte	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */
	.uleb128 0x1	/* (DIE (0xb) DW_TAG_compile_unit) */
	.4byte	.LASF2	/* DW_AT_producer: "GNU C++ 4.4.2 20091222 (Red Hat 4.4.2-20)" */
	.byte	0x4	/* DW_AT_language */
	.4byte	.LASF3	/* DW_AT_name: "gdb.dwarf2/member-ptr-forwardref.cc" */
	.4byte	.LASF4	/* DW_AT_comp_dir: "gdb/testsuite" */
	.4byte	.Ltext0	/* DW_AT_low_pc */
	.4byte	.Letext0	/* DW_AT_high_pc */
	.4byte	.Ldebug_line0	/* DW_AT_stmt_list */
	.uleb128 0x2	/* (DIE (0x25) DW_TAG_typedef) */
	.4byte	.LASF5	/* DW_AT_name: "fp_t" */
	.byte	0x1	/* DW_AT_decl_file (gdb.dwarf2/member-ptr-forwardref.cc) */
	.byte	0x13	/* DW_AT_decl_line */
	.4byte	0x30	/* DW_AT_type */
	.uleb128 0x3	/* (DIE (0x30) DW_TAG_structure_type) */
	.byte	0x8	/* DW_AT_byte_size */
	.byte	0x1	/* DW_AT_decl_file (gdb.dwarf2/member-ptr-forwardref.cc) */
	.byte	0x13	/* DW_AT_decl_line */
	.4byte	0x51	/* DW_AT_sibling */
	.uleb128 0x4	/* (DIE (0x38) DW_TAG_member) */
	.4byte	.LASF0	/* DW_AT_name: "__pfn" */
	.byte	0x1	/* DW_AT_decl_file (gdb.dwarf2/member-ptr-forwardref.cc) */
	.byte	0x13	/* DW_AT_decl_line */
	.4byte	0x85	/* DW_AT_type */
	.sleb128 0	/* DW_AT_data_member_location */
	.uleb128 0x4	/* (DIE (0x44) DW_TAG_member) */
	.4byte	.LASF1	/* DW_AT_name: "__delta" */
	.byte	0x1	/* DW_AT_decl_file (gdb.dwarf2/member-ptr-forwardref.cc) */
	.byte	0x13	/* DW_AT_decl_line */
	.4byte	0x61	/* DW_AT_type */
	.sleb128 4	/* DW_AT_data_member_location */
	.byte	0x0	/* end of children of DIE 0x30 */
	.uleb128 0x5	/* (DIE (0x51) DW_TAG_subroutine_type) */
	.4byte	0x61	/* DW_AT_type */
	.4byte	0x61	/* DW_AT_sibling */
	.uleb128 0x6	/* (DIE (0x5a) DW_TAG_formal_parameter) */
	.4byte	0x68	/* DW_AT_type */
	.byte	0x1	/* DW_AT_artificial */
	.byte	0x0	/* end of children of DIE 0x51 */
	.uleb128 0x7	/* (DIE (0x61) DW_TAG_base_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.byte	0x5	/* DW_AT_encoding */
	.ascii "int\0"	/* DW_AT_name */
	.uleb128 0x8	/* (DIE (0x68) DW_TAG_pointer_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.4byte	0x6e	/* DW_AT_type */
	.uleb128 0x9	/* (DIE (0x6e) DW_TAG_structure_type) */
	.ascii "C\0"	/* DW_AT_name */
	.byte	0x8	/* DW_AT_byte_size */
	.byte	0x1	/* DW_AT_decl_file (gdb.dwarf2/member-ptr-forwardref.cc) */
	.byte	0x16	/* DW_AT_decl_line */
	.4byte	0x85	/* DW_AT_sibling */
	.uleb128 0xa	/* (DIE (0x78) DW_TAG_member) */
	.ascii "fp\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (gdb.dwarf2/member-ptr-forwardref.cc) */
	.byte	0x17	/* DW_AT_decl_line */
	.4byte	0x30	/* DW_AT_type */
	.sleb128 0	/* DW_AT_data_member_location */
	.byte	0x3	/* DW_AT_accessibility */
	.byte	0x0	/* end of children of DIE 0x6e */
	.uleb128 0x8	/* (DIE (0x85) DW_TAG_pointer_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.4byte	0x51	/* DW_AT_type */
	.uleb128 0xb	/* (DIE (0x8b) DW_TAG_variable) */
	.ascii "fp\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (gdb.dwarf2/member-ptr-forwardref.cc) */
	.byte	0x14	/* DW_AT_decl_line */
	.4byte	0x25	/* DW_AT_type */
	.byte	0x1	/* DW_AT_external */
	.byte	0x5	/* DW_AT_location */
	.byte	0x3	/* DW_OP_addr */
	.4byte	fp
	.uleb128 0xb	/* (DIE (0x9c) DW_TAG_variable) */
	.ascii "c\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (gdb.dwarf2/member-ptr-forwardref.cc) */
	.byte	0x18	/* DW_AT_decl_line */
	.4byte	0x6e	/* DW_AT_type */
	.byte	0x1	/* DW_AT_external */
	.byte	0x5	/* DW_AT_location */
	.byte	0x3	/* DW_OP_addr */
	.4byte	c
	.byte	0x0	/* end of children of DIE 0xb */
	.section	.debug_abbrev
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
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x10	/* (DW_AT_stmt_list) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.byte	0x0
	.byte	0x0
	.uleb128 0x2	/* (abbrev code) */
	.uleb128 0x16	/* (TAG: DW_TAG_typedef) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0xe	/* (DW_FORM_strp) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0x0
	.byte	0x0
	.uleb128 0x3	/* (abbrev code) */
	.uleb128 0x13	/* (TAG: DW_TAG_structure_type) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0x0
	.byte	0x0
	.uleb128 0x4	/* (abbrev code) */
	.uleb128 0xd	/* (TAG: DW_TAG_member) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0xe	/* (DW_FORM_strp) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x38	/* (DW_AT_data_member_location) */
	.uleb128 0xd	/* (DW_FORM_sdata) */
	.byte	0x0
	.byte	0x0
	.uleb128 0x5	/* (abbrev code) */
	.uleb128 0x15	/* (TAG: DW_TAG_subroutine_type) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0x0
	.byte	0x0
	.uleb128 0x6	/* (abbrev code) */
	.uleb128 0x5	/* (TAG: DW_TAG_formal_parameter) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x34	/* (DW_AT_artificial) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.byte	0x0
	.byte	0x0
	.uleb128 0x7	/* (abbrev code) */
	.uleb128 0x24	/* (TAG: DW_TAG_base_type) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3e	/* (DW_AT_encoding) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.byte	0x0
	.byte	0x0
	.uleb128 0x8	/* (abbrev code) */
	.uleb128 0xf	/* (TAG: DW_TAG_pointer_type) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0x0
	.byte	0x0
	.uleb128 0x9	/* (abbrev code) */
	.uleb128 0x13	/* (TAG: DW_TAG_structure_type) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0x0
	.byte	0x0
	.uleb128 0xa	/* (abbrev code) */
	.uleb128 0xd	/* (TAG: DW_TAG_member) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x38	/* (DW_AT_data_member_location) */
	.uleb128 0xd	/* (DW_FORM_sdata) */
	.uleb128 0x32	/* (DW_AT_accessibility) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0x0
	.byte	0x0
	.uleb128 0xb	/* (abbrev code) */
	.uleb128 0x34	/* (TAG: DW_TAG_variable) */
	.byte	0x0	/* DW_children_no */
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
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.section	.debug_pubnames,"",%progbits
	.4byte	0x1b	/* Length of Public Names Info */
	.2byte	0x2	/* DWARF Version */
	.4byte	.Ldebug_info0	/* Offset of Compilation Unit Info */
	.4byte	0xad	/* Compilation Unit Length */
	.4byte	0x8b	/* DIE offset */
	.ascii "fp\0"	/* external name */
	.4byte	0x9c	/* DIE offset */
	.ascii "c\0"	/* external name */
	.4byte	0x0
	.section	.debug_pubtypes,"",%progbits
	.4byte	0x1d	/* Length of Public Type Names Info */
	.2byte	0x2	/* DWARF Version */
	.4byte	.Ldebug_info0	/* Offset of Compilation Unit Info */
	.4byte	0xad	/* Compilation Unit Length */
	.4byte	0x25	/* DIE offset */
	.ascii "fp_t\0"	/* external name */
	.4byte	0x6e	/* DIE offset */
	.ascii "C\0"	/* external name */
	.4byte	0x0
	.section	.debug_str,"MS",%progbits,1
.LASF5:
	.string	"fp_t"
.LASF2:
	.string	"GNU C++ 4.4.2 20091222 (Red Hat 4.4.2-20)"
.LASF0:
	.string	"__pfn"
.LASF4:
	.string	"gdb/testsuite"
.LASF3:
	.string	"gdb.dwarf2/member-ptr-forwardref.cc"
.LASF1:
	.string	"__delta"
	.ident	"GCC: (GNU) 4.4.2 20091222 (Red Hat 4.4.2-20)"
	.section	.note.GNU-stack,"",%progbits
