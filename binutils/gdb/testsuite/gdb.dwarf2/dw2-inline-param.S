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

/* Debug information */

	.section .debug_info
.Lcu1_begin:
	/* CU header */
	.4byte	.Lcu1_end - .Lcu1_start		/* Length of Compilation Unit */
.Lcu1_start:
	.2byte	2				/* DWARF Version */
	.4byte	.Labbrev1_begin			/* Offset into abbrev section */
	.byte	4				/* Pointer size */

	/* CU die */
	.uleb128 1				/* Abbrev: DW_TAG_compile_unit */
	.4byte	cu_text_start			/* DW_AT_low_pc */
	.4byte	cu_text_end			/* DW_AT_high_pc */
	.ascii	"file1.txt\0"			/* DW_AT_name */
	.ascii	"GNU C 3.3.3\0"			/* DW_AT_producer */
	.byte	1				/* DW_AT_language (C) */

.Ltype_int:
	.uleb128	3			/* Abbrev: DW_TAG_base_type */
	.ascii		"int\0"			/* DW_AT_name */
	.byte		4			/* DW_AT_byte_size */
	.byte		5			/* DW_AT_encoding */

	.uleb128	4			/* Abbrev: DW_TAG_subprogram */
	.ascii		"main\0"		/* DW_AT_name */
	.4byte		cu_text_start		/* DW_AT_low_pc */
	.4byte		cu_text_end		/* DW_AT_high_pc */
	.4byte		.Ltype_int-.Lcu1_begin	/* DW_AT_type */
	.byte		1			/* DW_AT_external */
	.byte		1			/* DW_AT_prototyped */

	.uleb128	6			/* Abbrev: DW_TAG_formal_parameter */
	.ascii		"mainparam\0"		/* DW_AT_name */

	.uleb128	5			/* Abbrev: DW_TAG_inlined_subroutine */
	.ascii		"func\0"		/* DW_AT_name */
	.4byte		block_start		/* DW_AT_low_pc */
	.4byte		block_end		/* DW_AT_high_pc */
	.byte		3			/* DW_AT_inline (DW_INL_declared_inlined) */
	.byte		1			/* DW_AT_prototyped */
	.byte		0			/* DW_AT_call_file */
	.byte		1			/* DW_AT_call_line */

	.uleb128	6			/* Abbrev: DW_TAG_formal_parameter */
	.ascii		"funcparam\0"		/* DW_AT_name */

	.byte		0			/* End of children of func */

	.byte		0			/* End of children of main */

	.byte		0			/* End of children of CU */

.Lcu1_end:

/* Abbrev table */
	.section .debug_abbrev
.Labbrev1_begin:
	.uleb128	1			/* Abbrev code */
	.uleb128	0x11			/* DW_TAG_compile_unit */
	.byte		1			/* has_children */
	.uleb128	0x11			/* DW_AT_low_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x12			/* DW_AT_high_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x25			/* DW_AT_producer */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x13			/* DW_AT_language */
	.uleb128	0xb			/* DW_FORM_data1 */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

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

	.uleb128	4			/* Abbrev code */
	.uleb128	0x2e			/* DW_TAG_subprogram */
	.byte		1			/* has_children */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x11			/* DW_AT_low_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x12			/* DW_AT_high_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x49			/* DW_AT_type */
	.uleb128	0x13			/* DW_FORM_ref4 */
	.uleb128	0x3f			/* DW_AT_external */
	.uleb128	0xc			/* DW_FORM_flag */
	.uleb128	0x27			/* DW_AT_prototyped */
	.uleb128	0xc			/* DW_FORM_flag */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	5			/* Abbrev code */
	.uleb128	0x1d			/* DW_TAG_inlined_subroutine */
	.byte		1			/* has_children */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x11			/* DW_AT_low_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x12			/* DW_AT_high_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x20			/* DW_AT_inline */
	.uleb128	0xb			/* DW_FORM_data1 */
	.uleb128	0x27			/* DW_AT_prototyped */
	.uleb128	0xc			/* DW_FORM_flag */
	.uleb128	0x58			/* DW_AT_call_file */
	.uleb128	0xb			/* DW_FORM_data1 */
	.uleb128	0x59			/* DW_AT_call_line */
	.uleb128	0xb			/* DW_FORM_data1 */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	6			/* Abbrev code */
	.uleb128	0x05			/* DW_TAG_formal_parameter */
	.byte		0			/* has_children */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
