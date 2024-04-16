/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

/* Debug information */

	.section	.debug_info
.Lcu1_begin:
	.4byte		.Lcu1_end - .Lcu1_start	/* Length of Compilation Unit */
.Lcu1_start:
	.2byte		2			/* DWARF version number */
	.4byte		.Ldebug_abbrev0		/* Offset Into Abbrev. Section */
	.byte		4			/* Pointer Size (in bytes) */

	/* CU die */
	.uleb128	1			/* Abbrev: DW_TAG_compile_unit */
	.4byte		.Lproducer		/* DW_AT_producer */
	/* Use C++ to exploit a bug in parsing DW_AT_name "".  */
	.byte		4			/* DW_AT_language (C++) -  */

.Larray_type:
	.uleb128	2			/* Abbrev: DW_TAG_array_type */
	.4byte		.Lchar_type-.Lcu1_begin	/* DW_AT_type */

	.uleb128	3			/* Abbrev: DW_TAG_subrange_type */
	.4byte		.Luint_type-.Lcu1_begin	/* DW_AT_type */
	.byte		0xd			/* DW_AT_upper_bound */
	.byte		0			/* End of children of die */

.Luint_type:
	.uleb128	4			/* Abbrev: DW_TAG_base_type */
	.4byte		.Luint_str		/* DW_AT_name */
	.byte		4			/* DW_AT_byte_size */
	.byte		7			/* DW_AT_encoding */

.Lchar_type:
	.uleb128	4			/* Abbrev: DW_TAG_base_type */
	.4byte		.Lchar_str		/* DW_AT_name */
	.byte		1			/* DW_AT_byte_size */
	.byte		6			/* DW_AT_encoding */

	.uleb128	5			/* Abbrev: DW_TAG_variable DW_FORM_strp */
	.4byte		.Lvarname		/* DW_AT_name */
	.4byte		.Lconst_type-.Lcu1_begin/* DW_AT_type */
	.4byte		.Lvarcontents		/* DW_AT_const_value */
	.byte		1			/* DW_AT_external */

	.uleb128	6			/* Abbrev: DW_TAG_variable DW_FORM_string */
	.string		"a_string2"		/* DW_AT_name */
	.4byte		.Lconst_type-.Lcu1_begin/* DW_AT_type */
	.string		"hello world2\n"	/* DW_AT_const_value */
	.byte		1			/* DW_AT_external */

.Lconst_type:
	.uleb128	7			/* Abbrev: DW_TAG_const_type */
	.4byte		.Larray_type-.Lcu1_begin/* DW_AT_type */

	.uleb128	8			/* Abbrev: DW_TAG_variable (name "") */
	.4byte		.Lemptyname		/* DW_AT_name */

	.byte		0			/* End of children of CU */
.Lcu1_end:

	.section .debug_abbrev
.Ldebug_abbrev0:
	.uleb128	1			/* Abbrev code */
	.uleb128	0x11			/* DW_TAG_compile_unit */
	.byte		0x1			/* has_children */
	.uleb128	0x25			/* DW_AT_producer */
	.uleb128	0xe			/* DW_FORM_strp */
	.uleb128	0x13			/* DW_AT_language */
	.uleb128	0xb			/* DW_FORM_data1 */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	2			/* Abbrev code */
	.uleb128	0x1			/* TAG: DW_TAG_array_type */
	.byte		0x1			/* DW_children_yes */
	.uleb128	0x49			/* DW_AT_type */
	.uleb128	0x13			/* DW_FORM_ref4 */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	3			/* Abbrev code */
	.uleb128	0x21			/* DW_TAG_subrange_type */
	.byte		0x0			/* no children */
	.uleb128	0x49			/* DW_AT_type */
	.uleb128	0x13			/* DW_FORM_ref4 */
	.uleb128	0x2f			/* DW_AT_upper_bound */
	.uleb128	0xb			/* DW_FORM_data1 */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	4			/* Abbrev code */
	.uleb128	0x24			/* DW_TAG_base_type */
	.byte		0x0			/* no_children */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0xe			/* DW_FORM_strp */
	.uleb128	0xb			/* DW_AT_byte_size */
	.uleb128	0xb			/* DW_FORM_data1 */
	.uleb128	0x3e			/* DW_AT_encoding */
	.uleb128	0xb			/* DW_FORM_data1 */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	5			/* Abbrev code */
	.uleb128	0x34			/* DW_TAG_variable */
	.byte		0x0			/* no_children */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0xe			/* DW_FORM_strp */
	.uleb128	0x49			/* DW_AT_type */
	.uleb128	0x13			/* DW_FORM_ref4 */
	.uleb128	0x1c			/* DW_AT_const_value */
	.uleb128	0xe			/* DW_FORM_strp */
	.uleb128	0x3f			/* DW_AT_external */
	.uleb128	0xc			/* DW_FORM_flag */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	6			/* Abbrev code */
	.uleb128	0x34			/* DW_TAG_variable */
	.byte		0x0			/* no_children */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x49			/* DW_AT_type */
	.uleb128	0x13			/* DW_FORM_ref4 */
	.uleb128	0x1c			/* DW_AT_const_value */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x3f			/* DW_AT_external */
	.uleb128	0xc			/* DW_FORM_flag */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	7			/* Abbrev code */
	.uleb128	0x26			/* DW_TAG_const_type */
	.byte		0x0			/* DW_children_no */
	.uleb128	0x49			/* DW_AT_type */
	.uleb128	0x13			/* DW_FORM_ref4 */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	8			/* Abbrev code */
	.uleb128	0x34			/* DW_TAG_variable */
	.byte		0x0			/* DW_children_no */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0xe			/* DW_FORM_strp */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.byte		0x0			/* Terminator */

/* String table */
	.section .debug_str
.Lproducer:
	.string		"GNU C 3.3.3"
.Lchar_str:
	.string		"char"
.Luint_str:
	.string		"unsigned int"
.Lvarname:
	.string		"a_string"
.Lvarcontents:
	.string		"hello world!\n"
.Lemptyname:
	.string		""
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
