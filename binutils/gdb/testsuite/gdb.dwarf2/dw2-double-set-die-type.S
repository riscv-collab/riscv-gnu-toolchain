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

/* This test triggers a complaint when gdb attempts to set a DIE's
   type twice.  The test was derived from the following source code:

   #include <vector>

   class a
   {
   private:
     class b
     {
     public:
       b () { }
     };

     std::vector<b> list_;
   };

  What follows below is a much simplified version of the debug info generated
  by gcc from the above code.  */

#define OFFSET(LBL) .Ldie_ ## LBL - .Lcu1_begin

	.section	.debug_info
.Lcu1_begin:
	.4byte	.Lcu1_end - .Lcu1_start	/* Length of Compilation Unit Info */
.Lcu1_start:
	.2byte	0x3	/* DWARF version number */
	.4byte	.Labbrev1_begin	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */

.Ldie_b:
	.uleb128 0x1	/* (DIE (0xb) DW_TAG_compile_unit) */
	.ascii	"GNU C++ 4.4.3 20100127 (Red Hat 4.4.3-4)\0"	/* DW_AT_producer */
	.byte	0x4	/* DW_AT_language */
	.ascii	"duplicate-type.cc\0"	/* DW_AT_name */

.Ldie_38:
	.uleb128 0x3	/* (DIE (0x38) DW_TAG_typedef) */
	.ascii	"size_t\0"	/* DW_AT_name */
	.4byte	OFFSET (43)	/* DW_AT_type */

.Ldie_43:
	.uleb128 0x2	/* (DIE (0x43) DW_TAG_base_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.byte	0x7	/* DW_AT_encoding */
	.ascii	"long unsigned int\0" /* DW_AT_name */

.Ldie_4a:
	.uleb128 0x4	/* (DIE (0x4a) DW_TAG_namespace) */
	.ascii "std\0"	/* DW_AT_name */

.Ldie_70:
	.uleb128 0x8	/* (DIE (0x70) DW_TAG_class_type) */
	.ascii	"allocator<a::b>\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_declaration */

.Ldie_76:
	.uleb128 0x9	/* (DIE (0x76) DW_TAG_structure_type) */
	.ascii	"_Vector_base<a::b, std::allocator<a::b> >\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_declaration */

.Ldie_80:
	.uleb128 0xa	/* (DIE (0x80) DW_TAG_structure_type) */
	.ascii	"_Vector_impl\0"	/* DW_AT_name */
	.byte	0x18	/* DW_AT_byte_size */

.Ldie_88:
	.uleb128 0xb	/* (DIE (0x88) DW_TAG_inheritance) */
	.4byte	OFFSET (3e0)	/* DW_AT_type */
	.sleb128 0	/* DW_AT_data_member_location */
	.byte	0x1	/* DW_AT_accessibility */
	.byte	0x0	/* end of children of DIE 0x80 */
	.byte	0x0	/* end of children of DIE 0x76 */

.Ldie_f7:
	.uleb128 0x8	/* (DIE (0xf7) DW_TAG_class_type) */
	.ascii	"vector<a::b, std::allocator<a::b> >\0" /* DW_AT_name */
	.byte	0x1	/* DW_AT_declaration */
	.byte	0x0	/* end of children of DIE 0x4a */

.Ldie_143:
	.uleb128 0x13	/* (DIE (0x143) DW_TAG_base_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.byte	0x5	/* DW_AT_encoding */

.Ldie_162:
	.uleb128 0x8	/* (DIE (0x162) DW_TAG_class_type) */
	.ascii	"new_allocator<a::b>\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_declaration */

.Ldie_19f:
	.uleb128 0x2	/* (DIE (0x19f) DW_TAG_base_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.byte	0x5	/* DW_AT_encoding */
	.ascii "int\0"	/* DW_AT_name */

.Ldie_1c6:
	.uleb128 0x17	/* (DIE (0x1c6) DW_TAG_pointer_type) */
	.byte	0x8	/* DW_AT_byte_size */
	.4byte	OFFSET (1cc)	/* DW_AT_type */

.Ldie_1cc:
	.uleb128 0x18	/* (DIE (0x1cc) DW_TAG_const_type) */

.Ldie_1cd:
	.uleb128 0x2	/* (DIE (0x1cd) DW_TAG_base_type) */
	.byte	0x1	/* DW_AT_byte_size */
	.byte	0x6	/* DW_AT_encoding */
	.ascii	"char\0"	/* DW_AT_name */

.Ldie_221:
	.uleb128 0x1b	/* (DIE (0x221) DW_TAG_class_type) */
	.ascii "a\0"	/* DW_AT_name */
	.byte	0x18	/* DW_AT_byte_size */

.Ldie_22b:
	.uleb128 0x1b	/* (DIE (0x22b) DW_TAG_class_type) */
	.ascii "b\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_byte_size */

.Ldie_235:
	.uleb128 0x1c	/* (DIE (0x235) DW_TAG_subprogram) */
	.byte	0x1	/* DW_AT_external */
	.ascii "b\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_declaration */

.Ldie_23c:
	.uleb128 0xe	/* (DIE (0x23c) DW_TAG_formal_parameter) */
	.4byte	OFFSET (277)	/* DW_AT_type */
	.byte	0x1	/* DW_AT_artificial */
	.byte	0x0	/* end of children of DIE 0x235 */
	.byte	0x0	/* end of children of DIE 0x22b */

.Ldie_244:
	.uleb128 0x1d	/* (DIE (0x244) DW_TAG_member) */
	.ascii	"list\0"	/* DW_AT_name */
	.4byte	OFFSET (59d)	/* DW_AT_type */
	.sleb128 0	/* DW_AT_data_member_location */
	.byte	0x3	/* DW_AT_accessibility */

.Ldie_251:
	.uleb128 0x1e	/* (DIE (0x251) DW_TAG_subprogram) */
	.byte	0x1	/* DW_AT_external */
	.ascii "a\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_artificial */
	.byte	0x1	/* DW_AT_declaration */

.Ldie_25b:
	.uleb128 0xe	/* (DIE (0x25b) DW_TAG_formal_parameter) */
	.4byte	OFFSET (b61)	/* DW_AT_type */
	.byte	0x1	/* DW_AT_artificial */
	.byte	0x0	/* end of children of DIE 0x251 */

.Ldie_262:
	.uleb128 0x1f	/* (DIE (0x262) DW_TAG_subprogram) */
	.byte	0x1	/* DW_AT_external */
	.ascii "~a\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_artificial */
	.byte	0x1	/* DW_AT_declaration */

.Ldie_269:
	.uleb128 0xe	/* (DIE (0x269) DW_TAG_formal_parameter) */
	.4byte	OFFSET (b61)	/* DW_AT_type */
	.byte	0x1	/* DW_AT_artificial */

.Ldie_26f:
	.uleb128 0xe	/* (DIE (0x26f) DW_TAG_formal_parameter) */
	.4byte	OFFSET (19f)	/* DW_AT_type */
	.byte	0x1	/* DW_AT_artificial */
	.byte	0x0	/* end of children of DIE 0x262 */
	.byte	0x0	/* end of children of DIE 0x221 */

.Ldie_277:
	.uleb128 0x17	/* (DIE (0x277) DW_TAG_pointer_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.4byte	OFFSET (22b)	/* DW_AT_type */

.Ldie_27d:
	.uleb128 0x20	/* (DIE (0x27d) DW_TAG_class_type) */
	.4byte	OFFSET (162)	/* DW_AT_specification */
	.byte	0x1	/* DW_AT_byte_size */

.Ldie_2d0:
	.uleb128 0x21	/* (DIE (0x2d0) DW_TAG_subprogram) */
	.byte	0x1	/* DW_AT_external */
	.ascii	"address\0"	/* DW_AT_name */
	.4byte	OFFSET (277)	/* DW_AT_type */
	.byte	0x1	/* DW_AT_declaration */

.Ldie_2e5:
	.uleb128 0xe	/* (DIE (0x2e5) DW_TAG_formal_parameter) */
	.4byte	OFFSET (3da)	/* DW_AT_type */
	.byte	0x1	/* DW_AT_artificial */

.Ldie_2eb:
	.uleb128 0xf	/* (DIE (0x2eb) DW_TAG_formal_parameter) */
	.4byte	OFFSET (3bd)	/* DW_AT_type */
	.byte	0x0	/* end of children of DIE 0x2d0 */
	.byte	0x0	/* end of children of DIE 0x27d */

.Ldie_3b2:
	.uleb128 0x17	/* (DIE (0x3b2) DW_TAG_pointer_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.4byte	OFFSET (3b8)	/* DW_AT_type */

.Ldie_3b8:
	.uleb128 0x19	/* (DIE (0x3b8) DW_TAG_const_type) */
	.4byte	OFFSET (22b)	/* DW_AT_type */

.Ldie_3bd:
	.uleb128 0x22	/* (DIE (0x3bd) DW_TAG_reference_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.4byte	OFFSET (22b)	/* DW_AT_type */

.Ldie_3d5:
	.uleb128 0x19	/* (DIE (0x3d5) DW_TAG_const_type) */
	.4byte	OFFSET (27d)	/* DW_AT_type */

.Ldie_3da:
	.uleb128 0x17	/* (DIE (0x3da) DW_TAG_pointer_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.4byte	OFFSET (3d5)	/* DW_AT_type */

.Ldie_3e0:
	.uleb128 0x20	/* (DIE (0x3e0) DW_TAG_class_type) */
	.4byte	OFFSET (70)	/* DW_AT_specification */
	.byte	0x1	/* DW_AT_byte_size */

.Ldie_3ec:
	.uleb128 0xb	/* (DIE (0x3ec) DW_TAG_inheritance) */
	.4byte	OFFSET (27d)	/* DW_AT_type */
	.sleb128 0	/* DW_AT_data_member_location */
	.byte	0x1	/* DW_AT_accessibility */
	.byte	0x0	/* end of children of DIE 0x3e0 */

.Ldie_44e:
	.uleb128 0x1a	/* (DIE (0x44e) DW_TAG_structure_type) */
	.4byte	OFFSET (76)	/* DW_AT_specification */
	.byte	0x18	/* DW_AT_byte_size */

.Ldie_505:
	.uleb128 0x11	/* (DIE (0x505) DW_TAG_subprogram) */
	.ascii	"~_Vector_base\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_declaration */

.Ldie_512:
	.uleb128 0xe	/* (DIE (0x512) DW_TAG_formal_parameter) */
	.4byte	OFFSET (58c)	/* DW_AT_type */
	.byte	0x1	/* DW_AT_artificial */

.Ldie_518:
	.uleb128 0xf	/* (DIE (0x518) DW_TAG_formal_parameter) */
	.4byte	OFFSET (19f)	/* DW_AT_type */
	.byte	0x0	/* end of children of DIE 0x505 */

.Ldie_51f:
	.uleb128 0x23	/* (DIE (0x51f) DW_TAG_subprogram) */
	.byte	0x1	/* DW_AT_external */
	.ascii	"_M_allocate\0"	/* DW_AT_name */
	.4byte	OFFSET (277)	/* DW_AT_type */
	.byte	0x1	/* DW_AT_declaration */

.Ldie_540:
	.uleb128 0x11	/* (DIE (0x540) DW_TAG_subprogram) */
	.ascii	"_M_deallocate\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_declaration */

.Ldie_551:
	.uleb128 0xe	/* (DIE (0x551) DW_TAG_formal_parameter) */
	.4byte	OFFSET (58c)	/* DW_AT_type */
	.byte	0x1	/* DW_AT_artificial */

.Ldie_557:
	.uleb128 0xf	/* (DIE (0x557) DW_TAG_formal_parameter) */
	.4byte	OFFSET (277)	/* DW_AT_type */

.Ldie_55c:
	.uleb128 0xf	/* (DIE (0x55c) DW_TAG_formal_parameter) */
	.4byte	OFFSET (43)	/* DW_AT_type */
	.byte	0x0	/* end of children of DIE 0x540 */

.Ldie_562:
	.uleb128 0x5	/* (DIE (0x562) DW_TAG_imported_declaration) */
	.4byte	OFFSET (51f)	/* DW_AT_import */
	.byte	0x0	/* end of children of DIE 0x44e */

.Ldie_586:
	.uleb128 0x22	/* (DIE (0x586) DW_TAG_reference_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.4byte	OFFSET (3e0)	/* DW_AT_type */

.Ldie_58c:
	.uleb128 0x17	/* (DIE (0x58c) DW_TAG_pointer_type) */
	.byte	0x8	/* DW_AT_byte_size */
	.4byte	OFFSET (44e)	/* DW_AT_type */

.Ldie_59d:
	.uleb128 0x20	/* (DIE (0x59d) DW_TAG_class_type) */
	.4byte	OFFSET (f7)	/* DW_AT_specification */
	.byte	0x18	/* DW_AT_byte_size */

.Ldie_5a9:
	.uleb128 0xb	/* (DIE (0x5a9) DW_TAG_inheritance) */
	.4byte	OFFSET (44e)	/* DW_AT_type */
	.sleb128 0	/* DW_AT_data_member_location */
	.byte	0x2	/* DW_AT_accessibility */
	.byte	0x0	/* end of children of DIE 0x59d */

.Ldie_b44:
	.uleb128 0x17	/* (DIE (0xb44) DW_TAG_pointer_type) */
	.byte	0x8	/* DW_AT_byte_size */
	.4byte	OFFSET (59d) /* DW_AT_type */

.Ldie_b61:
	.uleb128 0x17	/* (DIE (0xb61) DW_TAG_pointer_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.4byte	OFFSET (221) /* DW_AT_type */
	.byte	0x0	/* end of children of DIE 0xb */
.Lcu1_end:

/* Abbrev table */
	.section	.debug_abbrev
.Labbrev1_begin:
	.uleb128 0x1	/* (abbrev code) */
	.uleb128 0x11	/* (TAG: DW_TAG_compile_unit) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x25	/* (DW_AT_producer) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x13	/* (DW_AT_language) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_sting) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x2	/* (abbrev code) */
	.uleb128 0x24	/* (DW_TAG_base_type) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3e	/* (DW_AT_encoding) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x3	/* (abbrev code) */
	.uleb128 0x16	/* (TAG: DW_TAG_typedef) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x4	/* (abbrev code) */
	.uleb128 0x39	/* (TAG: DW_TAG_namespace) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x5	/* (abbrev code) */
	.uleb128 0x8	/* (TAG: DW_TAG_imported_declaration) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x18	/* (DW_AT_import) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x8	/* (abbrev code) */
	.uleb128 0x2	/* (TAG: DW_TAG_class_type) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3c	/* (DW_AT_declaration) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x9	/* (abbrev code) */
	.uleb128 0x13	/* (TAG: DW_TAG_structure_type) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3c	/* (DW_AT_declaration) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.byte	0x0
	.byte	0x0

	.uleb128 0xa	/* (abbrev code) */
	.uleb128 0x13	/* (TAG: DW_TAG_structure_type) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0x0
	.byte	0x0

	.uleb128 0xb	/* (abbrev code) */
	.uleb128 0x1c	/* (TAG: DW_TAG_inheritance) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x38	/* (DW_AT_data_member_location) */
	.uleb128 0xd	/* (DW_FORM_sdata) */
	.uleb128 0x32	/* (DW_AT_accessibility) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0x0
	.byte	0x0

	.uleb128 0xe	/* (abbrev code) */
	.uleb128 0x5	/* (TAG: DW_TAG_formal_parameter) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x34	/* (DW_AT_artificial) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.byte	0x0
	.byte	0x0

	.uleb128 0xf	/* (abbrev code) */
	.uleb128 0x5	/* (TAG: DW_TAG_formal_parameter) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x11	/* (abbrev code) */
	.uleb128 0x2e	/* (DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3c	/* (DW_AT_declaration) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x13	/* (abbrev code) */
	.uleb128 0x24	/* (DW_TAG_base_type) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3e	/* (DW_AT_encoding) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x17	/* (abbrev code) */
	.uleb128 0xf	/* (TAG: DW_TAG_pointer_type) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x18	/* (abbrev code) */
	.uleb128 0x26	/* (TAG: DW_TAG_const_type) */
	.byte	0x0	/* DW_children_no */
	.byte	0x0
	.byte	0x0

	.uleb128 0x19	/* (abbrev code) */
	.uleb128 0x26	/* (TAG: DW_TAG_const_type) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x1a	/* (abbrev code) */
	.uleb128 0x13	/* (TAG: DW_TAG_structure_type) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x47	/* (DW_AT_specification) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x1b	/* (abbrev code) */
	.uleb128 0x2	/* (TAG: DW_TAG_class_type) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x1c	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3c	/* (DW_AT_declaration) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x1d	/* (abbrev code) */
	.uleb128 0xd	/* (TAG: DW_TAG_member) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x38	/* (DW_AT_data_member_location) */
	.uleb128 0xd	/* (DW_FORM_sdata) */
	.uleb128 0x32	/* (DW_AT_accessibility) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x1e	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x34	/* (DW_AT_artificial) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x3c	/* (DW_AT_declaration) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x1f	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x34	/* (DW_AT_artificial) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x3c	/* (DW_AT_declaration) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x20	/* (abbrev code) */
	.uleb128 0x2	/* (TAG: DW_TAG_class_type) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x47	/* (DW_AT_specification) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x21	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x3c	/* (DW_AT_declaration) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x22	/* (abbrev code) */
	.uleb128 0x10	/* (TAG: DW_TAG_reference_type) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x23	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x3c	/* (DW_AT_declaration) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.byte	0x0
	.byte	0x0

	.byte	0x0
	.byte	0x0

#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
