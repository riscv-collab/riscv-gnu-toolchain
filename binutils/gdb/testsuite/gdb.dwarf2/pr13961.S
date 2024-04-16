/* Copyright 2012-2024 Free Software Foundation, Inc.

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

/* Testcase to exercise the code path in bug 13961.

   Compiled from:

   class foo
   {
    public:
     int bar;
   };

   foo baz;

   int
   main ()
   {
     return 0;
   }

   And then manually edited to insert the necessary DW_AT_specification
   entries to trigger the desired code path.
   There's no real need to make this architecture-specific, so it has been
   further hand-edited to support that.
*/

#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b

#ifdef SYMBOL_PREFIX
# define SYMBOL(str)     CONCAT1(SYMBOL_PREFIX, str)
#else
# define SYMBOL(str)     str
#endif

	.file	"pr13961.cc"

	.globl	baz
	.data	/* Previously this used .bss, but it's not portable.  */
	.align 4
	.type	baz, %object
	.size	baz, 4
baz:
#if 0
	/* This is disabled because when it was accidentally left out it
	   caused a segv when used with .gdb_index.  */
	.file	1 "foo.c"
	.loc	1 12 0
#endif
	.zero	4

	.text
.Ltext0:
	.globl	SYMBOL(main)
	.type	SYMBOL(main), %function
SYMBOL(main):
.LFB0:
	.4byte 0
.LFE0:
	.size	SYMBOL(main), .-SYMBOL(main)
.Letext0:

	.section	.debug_types,"",%progbits
.Ldebug_types0:
	.4byte	.Ltu_end - .Ltu_start	/* Length of Compilation Unit Info */
.Ltu_start:
	.2byte	0x4	/* DWARF version number */
	.4byte	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */
	.byte	0x19	/* Type Signature */
	.byte	0x8b
	.byte	0x7e
	.byte	0xac
	.byte	0xce
	.byte	0xf1
	.byte	0x22
	.byte	0x90
	.4byte	.Ltu_class_type - .Ldebug_types0 /* Offset to Type DIE */
	.uleb128 0x1	/* DW_TAG_type_unit */
	.byte	0x4	/* DW_AT_language */
	.byte	0x73	/* DW_AT_GNU_odr_signature */
	.byte	0xea
	.byte	0x85
	.byte	0x23
	.byte	0x75
	.byte	0x8a
	.byte	0x7e
	.byte	0x87
	.4byte	.Ldebug_line0	/* DW_AT_stmt_list */

	/* Manually inserted to have a DW_AT_specification refering to
	   something and appearing ahead of it.  */
	.uleb128 0x8	/* DW_TAG_class_type */
	.4byte .Ltu_class_type - .Ldebug_types0
	/* End of manual insertion */

.Ltu_class_type:
	.uleb128 0x2	/* (DW_TAG_class_type) */
	.ascii "foo\0"	/* DW_AT_name */
	.byte	0x4	/* DW_AT_byte_size */
	.byte	0x1	/* DW_AT_decl_file (pr13961.cc) */
	.byte	0x1	/* DW_AT_decl_line */
	.4byte	.Ltu_base_type - .Ldebug_types0	/* DW_AT_sibling */

		.uleb128 0x3	/* (DW_TAG_member) */
		.ascii "bar\0"	/* DW_AT_name */
		.byte	0x1	/* DW_AT_decl_file (pr13961.cc) */
		.byte	0x4	/* DW_AT_decl_line */
		.4byte	.Ltu_base_type - .Ldebug_types0	/* DW_AT_type */
		.byte	0	/* DW_AT_data_member_location */
		.byte	0x1	/* DW_AT_accessibility */
		.byte	0	/* end of children */

.Ltu_base_type:
	.uleb128 0x4	/* DW_TAG_base_type */
	.byte	0x4	/* DW_AT_byte_size */
	.byte	0x5	/* DW_AT_encoding */
	.ascii "int\0"	/* DW_AT_name */
	.byte	0	/* end of children */

.Ltu_end:

	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	.Lcu_end - .Lcu_start	/* Length of Compilation Unit Info */
.Lcu_start:
	.2byte	0x4	/* DWARF version number */
	.4byte	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */

	.uleb128 0x5	/* (DW_TAG_compile_unit) */
	.4byte	.LASF0	/* DW_AT_producer: "GNU C++ 4.6.3 20120306" */
	.byte	0x4	/* DW_AT_language */
	.4byte	.LASF1	/* DW_AT_name: "pr13961.cc" */
	.4byte	.LASF2	/* DW_AT_comp_dir: "." */
	.4byte	.Ltext0	/* DW_AT_low_pc */
	.4byte	.Letext0	/* DW_AT_high_pc */
	.4byte	.Ldebug_line0	/* DW_AT_stmt_list */

.Lint_base_type:
		.uleb128 0x4	/* DW_TAG_base_type */
		.byte	0x4	/* DW_AT_byte_size */
		.byte	0x5	/* DW_AT_encoding */
		.ascii "int\0"	/* DW_AT_name */

		.uleb128 0x6	/* DW_TAG_subprogram */
				/* DW_AT_external */
		.4byte	.LASF3	/* DW_AT_name: "main" */
		.byte	0x1	/* DW_AT_decl_file (pr13961.cc) */
		.byte	0xa	/* DW_AT_decl_line */
		.4byte	.Lint_base_type - .Ldebug_info0	/* DW_AT_type */
		.4byte	.LFB0	/* DW_AT_low_pc */
		.4byte	.LFE0	/* DW_AT_high_pc */
		.uleb128 0x1	/* DW_AT_frame_base */
		.byte	0x9c	/* DW_OP_call_frame_cfa */
				/* DW_AT_GNU_all_call_sites */

		.uleb128 0x7	/* DW_TAG_variable */
		.ascii "baz\0"	/* DW_AT_name */
		.byte	0x1	/* DW_AT_decl_file (pr13961.cc) */
		.byte	0x7	/* DW_AT_decl_line */
		.byte	0x19	/* DW_AT_type */
		.byte	0x8b
		.byte	0x7e
		.byte	0xac
		.byte	0xce
		.byte	0xf1
		.byte	0x22
		.byte	0x90
				/* DW_AT_external */
		.uleb128 0x5	/* DW_AT_location */
		.byte	0x3	/* DW_OP_addr */
		.4byte	baz

		/* Manually inserted to have a DW_AT_specification refering to
		   something and appearing ahead of it.  */
		.uleb128 0x8	/* DW_TAG_class_type */
		.4byte .Lcu_class_type - .Ldebug_info0 /* DW_AT_specification */

.Lcu_int_type:
		.uleb128 0x4	/* DW_TAG_base_type */
		.byte	0x4	/* DW_AT_byte_size */
		.byte	0x5	/* DW_AT_encoding */
		.ascii "int\0"	/* DW_AT_name */

.Lcu_class_type:
		.uleb128 0x2	/* DW_TAG_class_type */
		.ascii "foo2\0"	/* DW_AT_name */
		.byte	0x4	/* DW_AT_byte_size */
		.byte	0x1	/* DW_AT_decl_file (pr13961.cc) */
		.byte	0x1	/* DW_AT_decl_line */
		.4byte	.Lcu_second_int_type - .Ldebug_info0	/* DW_AT_sibling */

			.uleb128 0x3	/* DW_TAG_member */
			.ascii "bar\0"	/* DW_AT_name */
			.byte	0x1	/* DW_AT_decl_file (pr13961.cc) */
			.byte	0x4	/* DW_AT_decl_line */
			.4byte	.Lcu_second_int_type - .Ldebug_info0 /* DW_AT_type */
			.byte	0	/* DW_AT_data_member_location */
			.byte	0x1	/* DW_AT_accessibility */
			.byte	0	/* end of children */

.Lcu_second_int_type:
		.uleb128 0x4	/* DW_TAG_base_type */
		.byte	0x4	/* DW_AT_byte_size */
		.byte	0x5	/* DW_AT_encoding */
		.ascii "int\0"	/* DW_AT_name */
		/* End of manual insertion */

		.byte	0	/* end of children */

.Lcu_end:

	.section	.debug_abbrev,"",%progbits
.Ldebug_abbrev0:
	.uleb128 0x1	/* (abbrev code) */
	.uleb128 0x41	/* (TAG: DW_TAG_type_unit) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x13	/* (DW_AT_language) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x210f	/* (DW_AT_GNU_odr_signature) */
	.uleb128 0x7	/* (DW_FORM_data8) */
	.uleb128 0x10	/* (DW_AT_stmt_list) */
	.uleb128 0x17	/* (DW_FORM_sec_offset) */
	.byte	0
	.byte	0
	.uleb128 0x2	/* (abbrev code) */
	.uleb128 0x2	/* (TAG: DW_TAG_class_type) */
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
	.byte	0
	.byte	0
	.uleb128 0x3	/* (abbrev code) */
	.uleb128 0xd	/* (TAG: DW_TAG_member) */
	.byte	0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x38	/* (DW_AT_data_member_location) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x32	/* (DW_AT_accessibility) */
	.uleb128 0xb	/* (DW_FORM_data1) */
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
	.uleb128 0x17	/* (DW_FORM_sec_offset) */
	.byte	0
	.byte	0
	.uleb128 0x6	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0	/* DW_children_no */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0xe	/* (DW_FORM_strp) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x40	/* (DW_AT_frame_base) */
	.uleb128 0x18	/* (DW_FORM_exprloc) */
	.uleb128 0x2117	/* (DW_AT_GNU_all_call_sites) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.byte	0
	.byte	0
	.uleb128 0x7	/* (abbrev code) */
	.uleb128 0x34	/* (TAG: DW_TAG_variable) */
	.byte	0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x20	/* (DW_FORM_ref_sig8) */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.uleb128 0x2	/* (DW_AT_location) */
	.uleb128 0x18	/* (DW_FORM_exprloc) */
	.byte	0
	.byte	0

	/* Manually inserted.  */
	.uleb128 0x8		/* abbrev code */
	.uleb128 0x2		/* DW_TAG_class_type */
	.byte	0x0		/* DW_has_children_no */
	.uleb128 0x47		/* DW_AT_specification */
	.uleb128 0x13		/* DW_FORM_ref4 */
	.byte	0x0		/* Terminator */
	.byte	0x0		/* Terminator */
	/* End of manual insertion.  */

	.byte	0

	.section	.debug_aranges,"",%progbits
	.4byte	0x1c	/* Length of Address Ranges Info */
	.2byte	0x2	/* DWARF Version */
	.4byte	.Ldebug_info0	/* Offset of Compilation Unit Info */
	.byte	0x4	/* Size of Address */
	.byte	0	/* Size of Segment Descriptor */
	.2byte	0	/* Pad to 8 byte boundary */
	.2byte	0
	.4byte	.Ltext0	/* Address */
	.4byte	.Letext0-.Ltext0	/* Length */
	.4byte	0
	.4byte	0

	.section	.debug_line,"",%progbits
.Ldebug_line0:

	.section	.debug_str,"MS",%progbits,1
.LASF0:
	.string	"GNU C++ 4.6.3 20120306"
.LASF2:
	.string	"."
.LASF1:
	.string	"pr13961.cc"
.LASF3:
	.string	"main"

#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
