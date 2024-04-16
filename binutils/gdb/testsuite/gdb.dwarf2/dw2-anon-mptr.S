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

/* This test triggers a crash when a physname for a subprogram has
   a method pointer to an anonymous struct as a parameter.  This
   situation is possible with templates and lambda expressions,
   reproduced here by the following source code:

   class a
   {
    public:
     int doit (void) { return 0; }
   };

  class crash
   {
    public:
     crash (int (a::*mp) (void)) {}
   };

   In order to avoid lambdas, the resulting output has been edited to
   remove the DW_AT_name attribute from the DIEs describing "class a."  */

#define OFFSET(LBL) .Ldie_ ## LBL - .Lcu1_begin

	.section	.debug_info
.Lcu1_begin:
	.4byte	.Lcu1_end - .Lcu1_start /* Length of Compilation Unit Info */
.Lcu1_start:
	.2byte	0x3	/* DWARF version number */
	.4byte	.Labbrev1_begin	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */

.Ldie_b:
	.uleb128 0x1	/* (DIE (0xb) DW_TAG_compile_unit) */
	.ascii	"GNU C++ 4.6.3\0"	/* DW_AT_producer */
	.byte	0x4	/* DW_AT_language */
	.ascii	"anon-member-ptr.cc\0"	/* DW_AT_name */
	.4byte	.Ldebug_line0	/* DW_AT_stmt_list */

.Ldie_31:
	.uleb128 0x2	/* (DIE (0x31) DW_TAG_class_type  */
#if 0
	/* This is the "fudging" part...  */
	.ascii	"a\0"	/* DW_AT_name */
#endif
	.byte	0x1	/* DW_AT_byte_size */

.Ldie_59:
	.uleb128 0x4	/* (DIE (0x59) DW_TAG_base_type */
	.byte	0x4	/* DW_AT_byte_size */
	.byte	0x5	/* DW_AT_encoding */
	.ascii	"int\0"	/* DW_AT_name */

.Ldie_60:
	.uleb128 0x5	/* (DIE (0x60) DW_TAG_pointer_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.4byte	OFFSET (31)	/* DW_AT_type */

.Ldie_66:
	.uleb128 0x6	/* (DIE (0x66) DW_TAG_class_type) */
	.ascii	"crash\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_byte_size */
	.4byte	OFFSET (8d)	/* DW_AT_sibling */

.Ldie_72:
	.uleb128 0x7	/* (DIE (0x72) DW_TAG_subprogram) */
	.byte	0x1	/* DW_AT_external */
	.ascii	"crash\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_accessibility */
	.byte	0x1	/* DW_AT_declaration */
	.4byte	OFFSET (80)	/* DW_AT_object_pointer */

.Ldie_80:
	.uleb128 0x3	/* (DIE (0x80) DW_TAG_formal_parameter */
	.4byte	OFFSET (8d)	/* DW_AT_type */
	.byte	0x1	/* DW_AT_artificial */

.Ldie_86:
	.uleb128 0x3	/* (DIE (0x86) DW_TAG_formal_parameter */
	.4byte	OFFSET (93)	/* DW_AT_type */
	.byte	0x0	/* DW_AT_artificial */
	.byte	0	/* end of children of DIE 0x72 */
	.byte	0	/* end of children of DIE 0x66 */

.Ldie_8d:
	.uleb128 0x5	/* (DIE (0x8d) DW_TAG_pointer_type */
	.byte	0x4	/* DW_AT_size */
	.4byte	OFFSET (66)	/* DW_AT_type */

.Ldie_93:
	.uleb128 0x8	/* (DIE (0x93) DW_TAG_structure_type */
	.byte	0x10	/* DW_AT_byte_size */
	.4byte	OFFSET (b4)	/* DW_AT_sibling */

.Ldie_9b:
	.uleb128 0x9	/* (DIE (0x9b) DW_TAG_member */
	.ascii	"__pfn\0"	/* DW_AT_name */
	.4byte	OFFSET (c8)	/* DW_AT_type */
	.byte	0	/* DW_AT_data_member_location */

.Ldie_a7:
	.uleb128 0x9	/* (DIE (0xa7) DW_TAG_member */
	.ascii	"__delta\0"	/* DW_AT_name */
	.4byte	OFFSET (ce)	/* DW_AT_type */
	.byte	0x8	/* DW_AT_data_member_location */
	.byte	0	/* end of children of DIE 0x93 */

.Ldie_b4:
	.uleb128 0xa	/* (DIE (0xb4) DW_TAG_subroutine_type) */
	.4byte	OFFSET (59)	/* DW_AT_type */
	.4byte	OFFSET (c1)	/* DW_AT_object_pointer */
	.4byte	OFFSET (c8)	/* DW_AT_sibling */

.Ldie_c1:
	.uleb128 0x3	/* (DIE (0xc1) DW_TAG_formal_parameter) */
	.4byte	OFFSET (60)	/* DW_AT_type */
	.byte	0x1	/* DW_AT_artificial */
	.byte	0	/* end of children of DIE 0xb4 */

.Ldie_c8:
	.uleb128 0x5	/* (DIE (0xc8) DW_TAG_pointer_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.4byte	OFFSET (b4)	/* DW_AT_type */

.Ldie_ce:
	.uleb128 0x4	/* (DIE (0xce) DW_TAG_base_type) */
	.byte	0x8	/* DW_AT_byte_size */
	.byte	0x5	/* DW_AT_encoding */
	.ascii	"long int\0"	/* DW_AT_name */

.Ldie_107:
	.uleb128 0xc	/* (DIE (0x107) DW_TAG_subprogram) */
	.4byte	OFFSET (72)	/* DW_AT_specification */
	.byte	0x2	/* DW_AT_inline */
	.4byte	OFFSET (115)	/* DW_AT_object_pointer */
	.4byte	OFFSET (12a)	/* DW_AT_sibling */

.Ldie_115:
	.uleb128 0xd	/* (DIE (0x115) DW_TAG_formal_parameter) */
	.ascii	"this\0"	/* DW_AT_name */
	.4byte	OFFSET (12a)	/* DW_AT_type */
	.byte	0x1	/* DW_AT_artificial */

.Ldie_11f:
	.uleb128 0xd	/* (DIE (0x11f) DW_TAG_formal_parameter) */
	.ascii	"mp\0"	/* DW_AT_name */
	.4byte	OFFSET (93)	/* DW_AT_type */
	.byte	0x0	/* DW_AT_artificial */
	.byte	0	/* end of children of DIE 0x107 */

.Ldie_12a:
	.uleb128 0xb	/* (DIE (0x12a) DW_TAG_const_type) */
	.4byte	OFFSET (8d)	/* DW_AT_type */

.Ldie_12f:
	.uleb128 0xe	/* (DIE (0x12f) DW_TAG_subprogram) */
	.4byte	OFFSET (107)	/* DW_AT_abstract_origin */
	.4byte	0x20	/* DW_AT_low_pc */
	.4byte	0x2c	/* DW_AT_high_pc */
	.4byte	OFFSET (14e)	/* DW_AT_object_pointer */

.Ldie_14e:
	.uleb128 0xf	/* (DIE (0x14e) DW_TAG_formal_parameter) */
	.4byte	OFFSET (115)	/* DW_AT_abstract_origin */
	.byte	0x2	/* DW_AT_location */
	.byte	0x91
	.sleb128 -24

.Ldie_156:
	.uleb128 0xf	/* (DIE (0x156) DW_TAG_formal_parameter) */
	.4byte	OFFSET (11f)	/* DW_at_abstract_origin */
	.byte	0x2	/* DW_AT_location */
	.byte	0x91
	.sleb128 -40
	.byte	0	/* end of children of DIE 0x12f */
	.byte	0	/* end of children of DIE 0x5b */
.Lcu1_end:

/* Abbrev table */
	.section	.debug_abbrev
	.uleb128 0x1	/* (abbrev code) */
	.uleb128 0x11	/* (TAG: DW_TAG_compile_unit) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x25	/* (DW_AT_producer) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x13	/* (DW_AT_language) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x10	/* (DW_AT_stmt_list) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.byte	0
	.byte	0

	.uleb128 0x2	/* (abbrev code) */
	.uleb128 0x2	/* (TAG: DW_TAG_class_type) */
	.byte	0x0	/* DW_children_no */
#if 0
	/* This is the "fudging" part...  */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
#endif
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0
	.byte	0

	.uleb128 0x3	/* (abbrev code) */
	.uleb128 0x5	/* (TAG: DW_TAG_formal_parameter) */
	.byte	0	/* DW_children_no */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x34	/* (DW_AT_artificial) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.byte	0
	.byte	0

	.uleb128 0x4	/* (abbrev code) */
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

	.uleb128 0x5	/* (abbrev code) */
	.uleb128 0xf	/* (TAG: DW_TAG_pointer_type) */
	.byte	0	/* DW_children_no */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0

	.uleb128 0x6	/* (abbrev code) */
	.uleb128 0x2	/* (TAG: DW_TAG_class_type) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13
	.byte	0
	.byte	0

	.uleb128 0x7	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x32	/* (DW_AT_accessibility) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3c	/* (DW_AT_declaration) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x64	/* (DW_AT_object_pointer) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0

	.uleb128 0x8	/* (abbrev code) */
	.uleb128 0x13	/* (TAG: DW_TAG_structure_type) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0

	.uleb128 0x9	/* (abbrev code) */
	.uleb128 0xd	/* (TAG: DW_TAG_member) */
	.byte	0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x38	/* (DW_AT_data_member_location) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0
	.byte	0

	.uleb128 0xa	/* (abbrev code) */
	.uleb128 0x15	/* (TAG: DW_TAG_subroutine_type) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x64	/* (DW_AT_object_pointer) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0

	.uleb128 0xb	/* (abbrev code) */
	.uleb128 0x26	/* (TAG: DW_TAG_const_type) */
	.byte	0	/* DW_children_no */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0

	.uleb128 0xc	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x47	/* (DW_AT_specification) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x20	/* (DW_AT_inline) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x64	/* (DW_AT_object_pointer) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x1	/* (DW_AT_sibling) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0

	.uleb128 0xd	/* (abbrev code) */
	.uleb128 0x5	/* (TAG: DW_TAG_formal_parameter) */
	.byte	0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x34	/* (DW_AT_artificial) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.byte	0
	.byte	0

	.uleb128 0xe	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x64	/* (DW_AT_object_pointer) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0
	.byte	0

	.uleb128 0xf	/* (abbrev code) */
	.uleb128 0x5	/* (TAG: DW_TAG_formal_parameter) */
	.byte	0	/* DW_children_no */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x2	/* (DW_AT_location) */
	.uleb128 0xa	/* (DW_FORM_block) */
	.byte	0
	.byte	0
	.byte	0

	.section	.debug_line
.Ldebug_line0:
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
