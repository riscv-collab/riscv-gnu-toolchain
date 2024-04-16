/* Copyright 2011-2024 Free Software Foundation, Inc.

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

	.section	.debug_info
debug_start:
	.4byte	debug_end - 1f	/* Length of Compilation Unit Info */
1:
	.2byte	0x3	/* DWARF version number */
	.4byte	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */
	.uleb128 0x1	/* (DIE (0xb) DW_TAG_compile_unit) */
	.ascii "GNU C 4.4.3\0"	/* DW_AT_producer */
	.byte	0x1	/* DW_AT_language */
	.ascii "1.c\0"	/* DW_AT_name */
	.uleb128 0x4	/* (DIE (0x3c) DW_TAG_subprogram) */
	.ascii "f\0"	/* DW_AT_name */
	.4byte	f_start	/* DW_AT_low_pc */
	.4byte	f_end	/* DW_AT_high_pc */
	.byte	0x1	/* DW_AT_prototyped */
	.uleb128 0x5	/* (DIE (0x42) DW_TAG_formal_parameter) */
	.ascii	"param\0"		/* DW_AT_name */
	.4byte	type - debug_start	/* DW_AT_type */
	.4byte	loclist	/* DW_AT_location */
	.byte	0x0	/* end of children of DIE 0x3c */
type:
	.uleb128	3			/* Abbrev: DW_TAG_base_type */
	.ascii		"int\0"			/* DW_AT_name */
	.byte		4			/* DW_AT_byte_size */
	.byte		5			/* DW_AT_encoding */

	.byte	0x0	/* end of children of DIE 0xb */
debug_end:

	.section	.debug_loc
loclist:
	.4byte	f_start, f_end
	.2byte	2f-1f
1:	.byte	0xf3	/* DW_OP_GNU_entry_value */
	.uleb128  2f-3f
3:	.byte	0x50	/* DW_OP_reg0 */
2:
	.4byte	0, 0

	.section	.debug_abbrev
.Ldebug_abbrev0:
	.uleb128 0x1	/* (abbrev code) */
	.uleb128 0x11	/* (TAG: DW_TAG_compile_unit) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x25	/* (DW_AT_producer) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x13	/* (DW_AT_language) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.byte	0x0
	.byte	0x0
	.uleb128	3			/* Abbrev code */
	.uleb128	0x24			/* DW_TAG_base_type */
	.byte		0			/* has_children */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0xb			/* DW_AT_byte_size */
	.uleb128	0xb			/* DW_FORM_data1 */
	.uleb128	0x3e			/* DW_AT_encoding */
	.uleb128	0xb			/* DW_FORM_data1 */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */
	.uleb128 0x4	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x27	/* (DW_AT_prototyped) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.byte	0x0
	.byte	0x0
	.uleb128 0x5	/* (abbrev code) */
	.uleb128 0x5	/* (TAG: DW_TAG_formal_parameter) */
	.byte	0x0	/* DW_children_no */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x02	/* (DW_AT_location) */
	.uleb128 0x06	/* (DW_FORM_data4) */
	.byte	0x0
	.byte	0x0
	.byte	0x0
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
