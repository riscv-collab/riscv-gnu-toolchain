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
	.2byte	0x4	/* DWARF version number */
	.4byte	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */
	.uleb128 0x3	/* (DIE (0xb) DW_TAG_compile_unit) */
	.ascii "GNU C 4.4.3\0"	/* DW_AT_producer */
	.byte	0x0c	/* DW_AT_language = DW_LANG_C99 */
	.ascii "1.c\0"	/* DW_AT_name */
debug_end:

	.section	.debug_types
types_start:
	.macro	unit sig1 sig2 name
	.4byte	2f - 1f	/* Length of Compilation Unit Info */
1:
	.2byte	0x4	/* DWARF version number */
	.4byte	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */
	/* signature */
	.4byte	\sig1
	.4byte	\sig2
	.4byte	3f-types_start	/* offset into type DIE */
	.uleb128 0x1	/* DW_TAG_type_unit) */
	.byte	0x0c	/* DW_AT_language = DW_LANG_C99 */
3:
	.uleb128 0x2	/* DW_TAG_structure_type */
	.ascii	"\name"	/* DW_AT_name ... */
	.byte	0	/* ... DW_AT_name */
	.4byte	0	/* DW_AT_byte_size */
	.byte	0x0	/* end of children of CU */
2:
	.endm

	unit	0x01234567, 0x01234567, foo
	/* One duplicate.  */
	unit	0x01234567, 0x01234567, foo
	/* One different, but still unused.  */
	unit	0x89abcdef, 0x89abcdef, bar

	.section	.debug_abbrev
.Ldebug_abbrev0:
	.uleb128 0x1	/* (abbrev code) */
	.uleb128 0x41	/* (TAG: DW_TAG_type_unit) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x13	/* (DW_AT_language) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0x0
	.byte	0x0
	.uleb128 0x2	/* (abbrev code) */
	.uleb128 0x13	/* (TAG: DW_TAG_structure_type) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x0b	/* (DW_AT_byte_size) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.byte	0x0
	.byte	0x0
	.uleb128 0x3	/* (abbrev code) */
	.uleb128 0x11	/* (TAG: DW_TAG_compile_unit) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x25	/* (DW_AT_producer) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x13	/* (DW_AT_language) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.byte	0x0
	.byte	0x0
	.byte	0x0
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
