/* This testcase is part of GDB, the GNU debugger.

   Copyright 2007-2024 Free Software Foundation, Inc.

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

#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b

#ifdef SYMBOL_PREFIX
# define SYMBOL(str)     CONCAT1(SYMBOL_PREFIX, str)
#else
# define SYMBOL(str)     str
#endif

	.data

/* VAR1 is wrong here, in the real inferior it is located on stack.  As both
   places are never modified and they are initialized to the same value it
   makes no difference.  Ensure the name clash for "var".  */
SYMBOL(var1):	.byte	1

	.globl	SYMBOL(var)
SYMBOL(var):	.byte	2

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
	.4byte	cu_text_end			/* DW_AT_high_pc */
	.4byte	cu_text_start			/* DW_AT_low_pc */
	.ascii	"dw2-unresolved-main.c\0"	/* DW_AT_name */
	.ascii	"GNU C 4.3.2\0"			/* DW_AT_producer */
	.byte	1				/* DW_AT_language (C) */

.Ltype_uchar:
	.uleb128	2			/* Abbrev: DW_TAG_base_type */
	.ascii		"unsigned char\0"	/* DW_AT_name */
	.byte		1			/* DW_AT_byte_size */
	.byte		7			/* DW_AT_encoding */

	/* main */
	.uleb128	3			/* Abbrev: DW_TAG_subprogram */
	.byte		1			/* DW_AT_decl_file */
	.byte		2			/* DW_AT_decl_line */
	.ascii		"main\0"		/* DW_AT_name */
	.4byte		.Ltype_uchar-.Lcu1_begin	/* DW_AT_type */
	.4byte		cu_text_start		/* DW_AT_low_pc */
	.4byte		cu_text_end		/* DW_AT_high_pc */

	.uleb128	4			/* Abbrev: DW_TAG_variable (location) */
	.ascii		"var\0"			/* DW_AT_name */
	.byte		2f - 1f			/* DW_AT_location */
1:	.byte		3			/*   DW_OP_addr */
/* See VAR1 definition why this DIE is not correct.  */
	.4byte		SYMBOL(var1)			/*   <addr> */
2:	.4byte		.Ltype_uchar-.Lcu1_begin	/* DW_AT_type */

	.uleb128	6			/* Abbrev: DW_TAG_lexical_block */
	.4byte		SYMBOL(extern_block_start)	/* DW_AT_low_pc */
	.4byte		SYMBOL(extern_block_end)	/* DW_AT_high_pc */

	.uleb128	5			/* Abbrev: DW_TAG_variable (extern) */
	.ascii		"var\0"			/* DW_AT_name */
	.4byte		.Ltype_uchar-.Lcu1_begin	/* DW_AT_type */
	.byte		1			/* DW_AT_external */

	.byte		0			/* End of children of the lexical block */

	.byte		0			/* End of children of main */

	.byte		0			/* End of children of CU */

.Lcu1_end:

/* Abbrev table */
	.section .debug_abbrev
.Labbrev1_begin:
	.uleb128	1			/* Abbrev code */
	.uleb128	0x11			/* DW_TAG_compile_unit */
	.byte		1			/* has_children */
	.uleb128	0x12			/* DW_AT_high_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x11			/* DW_AT_low_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x25			/* DW_AT_producer */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x13			/* DW_AT_language */
	.uleb128	0xb			/* DW_FORM_data1 */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	2			/* Abbrev code */
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

	.uleb128	3			/* Abbrev code */
	.uleb128	0x2e			/* DW_TAG_subprogram */
	.byte		1			/* has_children */
	.uleb128	0x3a			/* DW_AT_decl_file */
	.uleb128	0xb			/* DW_FORM_data1 */
	.uleb128	0x3b			/* DW_AT_decl_line */
	.uleb128	0xb			/* DW_FORM_data1 */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x49			/* DW_AT_type */
	.uleb128	0x13			/* DW_FORM_ref4 */
	.uleb128	0x11			/* DW_AT_low_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x12			/* DW_AT_high_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	4			/* Abbrev code (location) */
	.uleb128	0x34			/* DW_TAG_variable */
	.byte		0			/* has_children */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x2			/* DW_AT_location */
	.uleb128	0xa			/* DW_FORM_block1 */
	.uleb128	0x49			/* DW_AT_type */
	.uleb128	0x13			/* DW_FORM_ref4 */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	5			/* Abbrev code (extern) */
	.uleb128	0x34			/* DW_TAG_variable */
	.byte		0			/* has_children */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x49			/* DW_AT_type */
	.uleb128	0x13			/* DW_FORM_ref4 */
	.uleb128	0x3f			/* DW_AT_external */
	.uleb128	0xc			/* DW_FORM_flag */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	6			/* Abbrev code */
	.uleb128	0x0b			/* DW_TAG_lexical_block */
	.byte		1			/* has_children */
	.uleb128	0x11			/* DW_AT_low_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x12			/* DW_AT_high_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
