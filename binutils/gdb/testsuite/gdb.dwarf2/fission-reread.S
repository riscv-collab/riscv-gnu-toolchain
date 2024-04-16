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

/* Testcase to exercise the code path in bug 13961 for Fission.
   http://gcc.gnu.org/wiki/DebugFission

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

	.file	"fission-reread.cc"

	.globl	SYMBOL(baz)
	.data	/* Previously this used .bss, but it's not portable.  */
	.align 4
	.type	SYMBOL(baz), %object
	.size	SYMBOL(baz), 4
SYMBOL(baz):
	.zero	4

	.text
.Ltext0:
	.globl	SYMBOL(main)
	.type	SYMBOL(main), %function
SYMBOL(main):
.LFB0:
	.file 1 "fission-reread.cc"
	.loc 1 11 0
	.4byte 0
.LFE0:
	.size	SYMBOL(main), .-SYMBOL(main)
.Letext0:

	.section	.debug_types.dwo
.Ltu_start_dwo:
	.4byte	.Ltu_end_dwo - .Ltu_start_dwo - 4	/* Length of Compilation Unit Info */
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
	.4byte	.Ltu_class_type - .Ltu_start_dwo	/* Offset to Type DIE */
	.uleb128 0x1	/* (DIE (0x17) DW_TAG_type_unit) */
	.byte	0x4	/* DW_AT_language */
	.byte	0x73	/* DW_AT_GNU_odr_signature */
	.byte	0xea
	.byte	0x85
	.byte	0x23
	.byte	0x75
	.byte	0x8a
	.byte	0x7e
	.byte	0x87
	.4byte	.Lskeleton_debug_line0	/* DW_AT_stmt_list */

	/* Manually inserted to have a DW_AT_specification refering to
	   something and appearing ahead of it.  */
	.uleb128 0x8	/* DW_TAG_class_type */
	.4byte .Ltu_class_type - .Ltu_start_dwo
	/* End of manual insertion */

.Ltu_class_type:
	.uleb128 0x2	/* (DIE (0x25) DW_TAG_class_type) */
	.ascii "foo\0"	/* DW_AT_name */
	.byte	0x4	/* DW_AT_byte_size */
	.byte	0x1	/* DW_AT_decl_file (fission-reread.cc) */
	.byte	0x1	/* DW_AT_decl_line */
	.4byte	.Ltu_int - .Ltu_start_dwo /* DW_AT_sibling */
	.uleb128 0x3	/* (DIE (0x31) DW_TAG_member) */
	.ascii "bar\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (fission-reread.cc) */
	.byte	0x4	/* DW_AT_decl_line */
	.4byte	.Ltu_int - .Ltu_start_dwo /* DW_AT_type */
	.byte	0	/* DW_AT_data_member_location */
	.byte	0x1	/* DW_AT_accessibility */
	.byte	0	/* end of children of DIE 0x25 */
.Ltu_int:
	.uleb128 0x4	/* (DIE (0x3f) DW_TAG_base_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.byte	0x5	/* DW_AT_encoding */
	.ascii "int\0"	/* DW_AT_name */
	.byte	0	/* end of children of DIE 0x17 */
.Ltu_end_dwo:

	.section	.debug_types
.Ltu_start:
	.4byte	.Ltu_end - .Ltu_start - 4	/* Length of Type Unit Info */
	.2byte	0x4	/* DWARF version number */
	.4byte	.Lskeleton_debug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */
	.byte	0x19	/* Type Signature */
	.byte	0x8b
	.byte	0x7e
	.byte	0xac
	.byte	0xce
	.byte	0xf1
	.byte	0x22
	.byte	0x90
	.4byte	0	/* Offset to Type DIE */
	.uleb128 0x2	/* (DIE (0) DW_TAG_type_unit) */
	.ascii "/tmp/src/gdb/testsuite\0"	/* DW_AT_comp_dir */
	.asciz DWO /* DW_AT_GNU_dwo_name */
	.4byte	.Ldebug_pubnames0	/* DW_AT_GNU_pubnames */
	.4byte	.Ldebug_pubtypes0	/* DW_AT_GNU_pubtypes */
	.4byte	.Ldebug_addr0	/* DW_AT_GNU_addr_base */
.Ltu_end:

	.section	.debug_info.dwo
.Lcu_start_dwo:
	.4byte	.Lcu_end_dwo - .Lcu_start_dwo - 4	/* Length of Compilation Unit Info */
	.2byte	0x4	/* DWARF version number */
	.4byte	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */
	.uleb128 0x5	/* (DIE (0xb) DW_TAG_compile_unit) */
	.ascii "GNU C++ 4.6.x-fission\0"	/* DW_AT_producer */
	.byte	0x4	/* DW_AT_language */
	.ascii "gdb.dwarf2/fission-reread.cc\0"	/* DW_AT_name */
	.ascii "/tmp/src/gdb/testsuite\0"	/* DW_AT_comp_dir */
	.byte	1	/* DW_AT_GNU_dwo_id */
	.byte	2
	.byte	3
	.byte	4
	.byte	5
	.byte	6
	.byte	7
	.byte	8
.Lcu_int:
	.uleb128 0x4	/* (DIE (0x7f) DW_TAG_base_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.byte	0x5	/* DW_AT_encoding */
	.ascii "int\0"	/* DW_AT_name */
	.uleb128 0x6	/* (DIE (0x86) DW_TAG_subprogram) */
			/* DW_AT_external */
	.ascii "main\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (fission-reread.cc) */
	.byte	0xa	/* DW_AT_decl_line */
	.4byte	.Lcu_int - .Lcu_start_dwo /* DW_AT_type */
	.uleb128 0	/* DW_AT_low_pc */
	.4byte	.LFE0-.LFB0	/* DW_AT_high_pc */
	.uleb128 0x1	/* DW_AT_frame_base */
	.byte	0x9c	/* DW_OP_call_frame_cfa */
	.uleb128 0x7	/* (DIE (0x9d) DW_TAG_variable) */
	.ascii "baz\0"	/* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (fission-reread.cc) */
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
	.uleb128 0x2	/* DW_AT_location */
	.byte	0xfb	/* DW_OP_GNU_addr_index */
	.uleb128 0x1	/* (index into .debug_addr) */
	.byte	0	/* end of children of DIE 0xb */
.Lcu_end_dwo:

	.section	.debug_info
.Lskeleton_debug_info0:
.Lcu_start:
	.4byte	.Lcu_end - .Lcu_start - 4	/* Length of Compilation Unit Info */
	.2byte	0x4	/* DWARF version number */
	.4byte	.Lskeleton_debug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */
	.uleb128 0x1	/* (DIE (0) DW_TAG_compile_unit) */
	.4byte	.Ltext0	/* DW_AT_low_pc */
	.4byte	.Letext0-.Ltext0	/* DW_AT_high_pc */
	.4byte	.Ldebug_line0	/* DW_AT_stmt_list */
	.ascii "/tmp/src/gdb/testsuite\0"	/* DW_AT_comp_dir */
	.asciz DWO /* DW_AT_GNU_dwo_name */
	.4byte	.Ldebug_pubnames0	/* DW_AT_GNU_pubnames */
	.4byte	.Ldebug_pubtypes0	/* DW_AT_GNU_pubtypes */
	.4byte	.Ldebug_addr0	/* DW_AT_GNU_addr_base */
	.byte	1	/* DW_AT_GNU_dwo_id */
	.byte	2
	.byte	3
	.byte	4
	.byte	5
	.byte	6
	.byte	7
	.byte	8
.Lcu_end:

	.section	.debug_abbrev
.Lskeleton_debug_abbrev0:
	.uleb128 0x1	/* (abbrev code) */
	.uleb128 0x11	/* (TAG: DW_TAG_compile_unit) */
	.byte	0	/* DW_children_no */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.uleb128 0x10	/* (DW_AT_stmt_list) */
	.uleb128 0x17	/* (DW_FORM_sec_offset) */
	.uleb128 0x1b	/* (DW_AT_comp_dir) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x2130	/* (DW_AT_GNU_dwo_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x2134	/* (DW_AT_GNU_pubnames) */
	.uleb128 0x17	/* (DW_FORM_sec_offset) */
	.uleb128 0x2135	/* (DW_AT_GNU_pubtypes) */
	.uleb128 0x17	/* (DW_FORM_sec_offset) */
	.uleb128 0x2133	/* (DW_AT_GNU_addr_base) */
	.uleb128 0x17	/* (DW_FORM_sec_offset) */
	.uleb128 0x2131	/* (DW_AT_GNU_dwo_id) */
	.uleb128 0x7	/* (DW_FORM_data8) */
	.byte	0
	.byte	0
	.uleb128 0x2	/* (abbrev code) */
	.uleb128 0x41	/* (TAG: DW_TAG_type_unit) */
	.byte	0	/* DW_children_no */
	.uleb128 0x1b	/* (DW_AT_comp_dir) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x2130	/* (DW_AT_GNU_dwo_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x2134	/* (DW_AT_GNU_pubnames) */
	.uleb128 0x17	/* (DW_FORM_sec_offset) */
	.uleb128 0x2135	/* (DW_AT_GNU_pubtypes) */
	.uleb128 0x17	/* (DW_FORM_sec_offset) */
	.uleb128 0x2133	/* (DW_AT_GNU_addr_base) */
	.uleb128 0x17	/* (DW_FORM_sec_offset) */
	.byte	0
	.byte	0
	.byte	0	/* end of skeleton .debug_abbrev */
	.section	.debug_abbrev.dwo
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
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x13	/* (DW_AT_language) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x1b	/* (DW_AT_comp_dir) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x2131	/* (DW_AT_GNU_dwo_id) */
	.uleb128 0x7	/* (DW_FORM_data8) */
	.byte	0
	.byte	0
	.uleb128 0x6	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0	/* DW_children_no */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1f01	/* (DW_FORM_GNU_addr_index) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.uleb128 0x40	/* (DW_AT_frame_base) */
	.uleb128 0x18	/* (DW_FORM_exprloc) */
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
	.section	.debug_pubnames
.Ldebug_pubnames0:
	.4byte	0x1f	/* Length of Public Names Info */
	.2byte	0x2	/* DWARF Version */
	.4byte	.Lskeleton_debug_info0	/* Offset of Compilation Unit Info */
	.4byte	0xb0	/* Compilation Unit Length */
	.4byte	0x86	/* DIE offset */
	.ascii "main\0"	/* external name */
	.4byte	0x9d	/* DIE offset */
	.ascii "baz\0"	/* external name */
	.4byte	0
	.section	.debug_pubtypes
.Ldebug_pubtypes0:
	.4byte	0x1e	/* Length of Public Type Names Info */
	.2byte	0x2	/* DWARF Version */
	.4byte	.Lskeleton_debug_info0	/* Offset of Compilation Unit Info */
	.4byte	0xb0	/* Compilation Unit Length */
	.4byte	0x7f	/* DIE offset */
	.ascii "int\0"	/* external name */
	.4byte	0x25	/* DIE offset */
	.ascii "foo\0"	/* external name */
	.4byte	0
	.section	.debug_aranges
	.4byte	0x1c	/* Length of Address Ranges Info */
	.2byte	0x2	/* DWARF Version */
	.4byte	.Lskeleton_debug_info0	/* Offset of Compilation Unit Info */
	.byte	0x4	/* Size of Address */
	.byte	0	/* Size of Segment Descriptor */
	.2byte	0	/* Pad to 16 byte boundary */
	.2byte	0
	.4byte	.Ltext0	/* Address */
	.4byte	.Letext0-.Ltext0	/* Length */
	.4byte	0
	.4byte	0
	.section	.debug_line
.Ldebug_line0:
	.section	.debug_line.dwo
.Lskeleton_debug_line0:
	.4byte	.LELT0-.LSLT0	/* Length of Source Line Info */
.LSLT0:
	.2byte	0x4	/* DWARF Version */
	.4byte	.LELTP0-.LASLTP0	/* Prolog Length */
.LASLTP0:
	.byte	0x1	/* Minimum Instruction Length */
	.byte	0x1	/* Maximum Operations Per Instruction */
	.byte	0x1	/* Default is_stmt_start flag */
	.byte	0xf6	/* Line Base Value (Special Opcodes) */
	.byte	0xf5	/* Line Range Value (Special Opcodes) */
	.byte	0xa	/* Special Opcode Base */
	.byte	0	/* opcode: 0x1 has 0 args */
	.byte	0x1	/* opcode: 0x2 has 1 args */
	.byte	0x1	/* opcode: 0x3 has 1 args */
	.byte	0x1	/* opcode: 0x4 has 1 args */
	.byte	0x1	/* opcode: 0x5 has 1 args */
	.byte	0	/* opcode: 0x6 has 0 args */
	.byte	0	/* opcode: 0x7 has 0 args */
	.byte	0	/* opcode: 0x8 has 0 args */
	.byte	0x1	/* opcode: 0x9 has 1 args */
	.byte	0	/* End directory table */
	.ascii "fission-reread.cc\0"	/* File Entry: 0x1 */
	.uleb128 0
	.uleb128 0
	.uleb128 0
	.byte	0	/* End file name table */
.LELTP0:
.LELT0:
	.section	.debug_addr
.Ldebug_addr0:
	.4byte	.LFB0	/* DW_AT_low_pc */
	.4byte	SYMBOL(baz)	/* DW_AT_location */
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
