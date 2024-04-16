/* This testcase is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

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

#if PTRBITS == 64
# define PTRBYTE .8byte
#elif PTRBITS == 32
# define PTRBYTE .4byte
#else
# error "Unsupported pointer size"
#endif

	.section .debug_info
.Lcu1_begin:
	/* CU header */
	.4byte	.Lcu1_end - .Lcu1_start		/* Length of Compilation Unit */
.Lcu1_start:
	.2byte	2				/* DWARF Version */
	.4byte	.Labbrev1_begin			/* Offset into abbrev section */
	.byte	PTRBITS / 8			/* Pointer size */

	/* CU die */
	.uleb128 1				/* Abbrev: DW_TAG_compile_unit */
	.4byte	.Lline1_begin			/* DW_AT_stmt_list */
	PTRBYTE	func_start			/* DW_AT_low_pc */
	PTRBYTE	func_end			/* DW_AT_high_pc */
	.ascii	"main.c\0"			/* DW_AT_name */
	.ascii	"GNU C 4.5.0\0"			/* DW_AT_producer must be >= 4.5  */
	.byte	2				/* DW_AT_language (DW_LANG_C) */

	.uleb128	2			/* Abbrev: DW_TAG_subprogram */
	.byte		1			/* DW_AT_external */
	.ascii		"func\0"		/* DW_AT_name */
	.4byte		.Ltype_int-.Lcu1_begin	/* DW_AT_type */
	PTRBYTE		func_start		/* DW_AT_low_pc */
	PTRBYTE		func_end		/* DW_AT_high_pc */

/* GDB `has_loclist' detection of -O2 -g code needs to see a DW_AT_location
   location list.  There may exist -O2 -g CUs still not needing/using any such
   location list - exactly like this CU.  Make one up.  */

	.uleb128	0x7			/* (DIE (0x42) DW_TAG_formal_parameter) */
	.ascii		"param\0"		/* DW_AT_name */
	.4byte		.Ltype_int - .Lcu1_begin	/* DW_AT_type */
	.4byte		loclist			/* DW_AT_location */

	.uleb128	4			/* Abbrev: DW_TAG_inlined_subroutine */
	.ascii		"inlined\0"		/* DW_AT_name */
	PTRBYTE		func0			/* DW_AT_low_pc */
	PTRBYTE		func1			/* DW_AT_high_pc */
	.byte		3			/* DW_AT_inline (DW_INL_declared_inlined) */
	.byte		1			/* DW_AT_call_file */
	.byte		8			/* DW_AT_call_line */

	.uleb128	4			/* Abbrev: DW_TAG_inlined_subroutine */
	.ascii		"inlined2\0"		/* DW_AT_name */
	PTRBYTE		func2			/* DW_AT_low_pc */
	PTRBYTE		func3			/* DW_AT_high_pc */
	.byte		3			/* DW_AT_inline (DW_INL_declared_inlined) */
	.byte		1			/* DW_AT_call_file */
	.byte		11			/* DW_AT_call_line */

#ifdef INLINED
	.uleb128	4			/* Abbrev: DW_TAG_inlined_subroutine */
	.ascii		"otherinline\0"		/* DW_AT_name */
	PTRBYTE		func3			/* DW_AT_low_pc */
	PTRBYTE		func_end		/* DW_AT_high_pc */
	.byte		3			/* DW_AT_inline (DW_INL_declared_inlined) */
	.byte		1			/* DW_AT_call_file */
	.byte		9			/* DW_AT_call_line */
#endif

#ifdef LEXICAL
	.uleb128	5			/* Abbrev: DW_TAG_lexical_block */
	PTRBYTE		func3			/* DW_AT_low_pc */
	PTRBYTE		func_end		/* DW_AT_high_pc */

	/* GDB would otherwise ignore the DW_TAG_lexical_block.  */
	.uleb128	6			/* Abbrev: DW_TAG_variable */
	.ascii		"lexicalvar\0"		/* DW_AT_name */
	.4byte		.Ltype_int-.Lcu1_begin	/* DW_AT_type */

	.byte		0			/* End of children of DW_TAG_lexical_block */
#endif

	.byte		0			/* End of children of DW_TAG_subprogram */

/* Simulate `fund' is also named `func' so that the function name matches and
   fund's SAL is not discarded in expand_line_sal_maybe.  */

	.uleb128	2			/* Abbrev: DW_TAG_subprogram */
	.byte		1			/* DW_AT_external */
	.ascii		"func\0"		/* DW_AT_name */
	.4byte		.Ltype_int-.Lcu1_begin	/* DW_AT_type */
	PTRBYTE		fund_start		/* DW_AT_low_pc */
	PTRBYTE		fund_end		/* DW_AT_high_pc */

	.byte		0			/* End of children of DW_TAG_subprogram */

.Ltype_int:
	.uleb128	3			/* Abbrev: DW_TAG_base_type */
	.ascii		"int\0"			/* DW_AT_name */
	.byte		4			/* DW_AT_byte_size */
	.byte		5			/* DW_AT_encoding */

	.byte		0			/* End of children of CU */

.Lcu1_end:

	.section	.debug_loc
loclist:
	/* Reset the location list base address first.  */
	PTRBYTE		-1, 0

	PTRBYTE		func_start, func_end
	.2byte		2f-1f
1:	.byte		0x50	/* DW_OP_reg0 */
2:
	/* Location list end.  */
	.4byte	0, 0

/* Abbrev table */
	.section .debug_abbrev
.Labbrev1_begin:
	.uleb128	1			/* Abbrev code */
	.uleb128	0x11			/* DW_TAG_compile_unit */
	.byte		1			/* has_children */
	.uleb128	0x10			/* DW_AT_stmt_list */
	.uleb128	0x6			/* DW_FORM_data4 */
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

	.uleb128	2			/* Abbrev code */
	.uleb128	0x2e			/* DW_TAG_subprogram */
	.byte		1			/* has_children */
	.uleb128	0x3f			/* DW_AT_external */
	.uleb128	0xc			/* DW_FORM_flag */
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
	.uleb128	0x1d			/* DW_TAG_inlined_subroutine */
	.byte		0			/* has_children */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x11			/* DW_AT_low_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x12			/* DW_AT_high_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x20			/* DW_AT_inline */
	.uleb128	0xb			/* DW_FORM_data1 */
	.uleb128	0x58			/* DW_AT_call_file */
	.uleb128	0xb			/* DW_FORM_data1 */
	.uleb128	0x59			/* DW_AT_call_line */
	.uleb128	0xb			/* DW_FORM_data1 */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	5			/* Abbrev code */
	.uleb128	0x0b			/* DW_TAG_lexical_block */
	.byte		1			/* has_children */
	.uleb128	0x11			/* DW_AT_low_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x12			/* DW_AT_high_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	6			/* Abbrev code */
	.uleb128	0x34			/* DW_TAG_variable */
	.byte		0			/* has_children */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x49			/* DW_AT_type */
	.uleb128	0x13			/* DW_FORM_ref4 */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	0x7			/* (abbrev code) */
	.uleb128	0x5			/* (TAG: DW_TAG_formal_parameter) */
	.byte		0x0			/* DW_children_no */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x49			/* (DW_AT_type) */
	.uleb128	0x13			/* (DW_FORM_ref4) */
	.uleb128	0x02			/* (DW_AT_location) */
	.uleb128	0x06			/* (DW_FORM_data4) */
	.byte		0x0
	.byte		0x0

	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

/* Line table */
	.section .debug_line
.Lline1_begin:
	.4byte		.Lline1_end - .Lline1_start	/* Initial length */
.Lline1_start:
	.2byte		2			/* Version */
	.4byte		.Lline1_lines - .Lline1_hdr	/* header_length */
.Lline1_hdr:
	.byte		1			/* Minimum insn length */
	.byte		1			/* default_is_stmt */
	.byte		1			/* line_base */
	.byte		1			/* line_range */
	.byte		0x10			/* opcode_base */

	/* Standard lengths */
	.byte		0
	.byte		1
	.byte		1
	.byte		1
	.byte		1
	.byte		0
	.byte		0
	.byte		0
	.byte		1
	.byte		0
	.byte		0
	.byte		1
	.byte		0
	.byte		0
	.byte		0

	/* Include directories */
	.byte		0

	/* File names */
	.ascii		"main.c\0"
	.uleb128	0
	.uleb128	0
	.uleb128	0

	.ascii		"other.c\0"
	.uleb128	0
	.uleb128	0
	.uleb128	0

	.byte		0

.Lline1_lines:
	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	PTRBYTE		func_start
	.byte		3	/* DW_LNS_advance_line */
	.sleb128	4	/* ... to 5 */
	.byte		1	/* DW_LNS_copy */

	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	PTRBYTE		func0
	.byte		4	/* DW_LNS_set_file */
	.uleb128	2
	.byte		3	/* DW_LNS_advance_line */
	.sleb128	-4	/* ... to 1 */
	.byte		1	/* DW_LNS_copy */

	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	PTRBYTE		func1
	.byte		4	/* DW_LNS_set_file */
	.uleb128	1
	.byte		3	/* DW_LNS_advance_line */
	.sleb128	8	/* ... to 9 */
	.byte		1	/* DW_LNS_copy */

	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	PTRBYTE		func2
	.byte		4	/* DW_LNS_set_file */
	.uleb128	2
	.byte		3	/* DW_LNS_advance_line */
	.sleb128	-8	/* ... to 1 */
	.byte		1	/* DW_LNS_copy */

	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	PTRBYTE		func3
	.byte		4	/* DW_LNS_set_file */
	.uleb128	1
	.byte		3	/* DW_LNS_advance_line */
	.sleb128	8	/* ... to 9 */
	.byte		1	/* DW_LNS_copy */

	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	PTRBYTE		func_end

/* Equivalent copy but renamed s/func/fund/.  */

	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	PTRBYTE		fund_start
	.byte		3	/* DW_LNS_advance_line */
	.sleb128	-4	/* ... to 5 */
	.byte		1	/* DW_LNS_copy */

	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	PTRBYTE		fund0
	.byte		4	/* DW_LNS_set_file */
	.uleb128	2
	.byte		3	/* DW_LNS_advance_line */
	.sleb128	-4	/* ... to 1 */
	.byte		1	/* DW_LNS_copy */

	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	PTRBYTE		fund1
	.byte		4	/* DW_LNS_set_file */
	.uleb128	1
	.byte		3	/* DW_LNS_advance_line */
	.sleb128	8	/* ... to 9 */
	.byte		1	/* DW_LNS_copy */

	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	PTRBYTE		fund2
	.byte		4	/* DW_LNS_set_file */
	.uleb128	2
	.byte		3	/* DW_LNS_advance_line */
	.sleb128	-8	/* ... to 1 */
	.byte		1	/* DW_LNS_copy */

	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	PTRBYTE		fund3
	.byte		4	/* DW_LNS_set_file */
	.uleb128	1
	.byte		3	/* DW_LNS_advance_line */
	.sleb128	8	/* ... to 9 */
	.byte		1	/* DW_LNS_copy */

	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	PTRBYTE		fund_end

/* Line numbering end.  */

	.byte		0	/* DW_LNE_end_of_sequence */
	.uleb128	1
	.byte		1

.Lline1_end:
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
