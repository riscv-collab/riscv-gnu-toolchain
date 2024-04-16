/* Copyright 2010-2024 Free Software Foundation, Inc.

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

	.uleb128 0x4	/* (DW_TAG_variable) */
	.ascii "underflow\0"	/* DW_AT_name */
	.2byte	2f - 1f	/* DW_AT_location: DW_FORM_block2 */
1:
	.byte	0x22	/* DW_OP_plus */
2:

	.uleb128 0x4	/* (DW_TAG_variable) */
	.ascii "overflow\0"	/* DW_AT_name */
	.2byte	2f - 1f	/* DW_AT_location: DW_FORM_block2 */
1:
	.fill	100, 1, 0x32	/* 100x DW_OP_lit2 */
	.byte	0x9f	/* DW_OP_stack_value */
2:

	.byte	0x0	/* end of children of CU */
debug_end:

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
	.uleb128 0x4	/* (abbrev code) */
	.uleb128 0x34	/* (TAG: DW_TAG_variable) */
	.byte	0x0	/* DW_children_yes */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x02	/* (DW_AT_location) */
	.uleb128 0x3	/* (DW_FORM_block2) */
	.byte	0x0
	.byte	0x0
	.byte	0x0
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
