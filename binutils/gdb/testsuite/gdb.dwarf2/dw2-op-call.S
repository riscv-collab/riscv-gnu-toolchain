/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

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

/* Test DW_OP_call2 and DW_OP_call4, PR gdb/10640.  */

	.section .data
array1:	.2byte	1
array2:	.2byte	2
array3:	.2byte	3

	.section .debug_info
.Lcu0_begin:
	/* CU header */
	.4byte	.Lcu0_end - .Lcu0_start		/* Length of Compilation Unit */
.Lcu0_start:
	.2byte	2				/* DWARF Version */
	.4byte	.Labbrev1_begin			/* Offset into abbrev section */
	.byte	4				/* Pointer size */

	/* CU die */
	.uleb128 1				/* Abbrev: DW_TAG_compile_unit */
	.ascii	"file0.txt\0"			/* DW_AT_name */
	.ascii	"GNU C 3.3.3\0"			/* DW_AT_producer */
	.byte	2				/* DW_LANG_C (C) */

	.byte	0				/* End of children of CU */
.Lcu0_end:

.Lcu1_begin:
	/* CU header */
	.4byte	.Lcu1_end - .Lcu1_start		/* Length of Compilation Unit */
.Lcu1_start:
	.2byte	2				/* DWARF Version */
	.4byte	.Labbrev1_begin			/* Offset into abbrev section */
	.byte	4				/* Pointer size */

	/* CU die */
	.uleb128 1				/* Abbrev: DW_TAG_compile_unit */
	.ascii	"file1.txt\0"			/* DW_AT_name */
	.ascii	"GNU C 3.3.3\0"			/* DW_AT_producer */
	.byte	2				/* DW_LANG_C (C) */

.L2byte_type:
	.uleb128	2			/* Abbrev: DW_TAG_base_type */
	.ascii		"2byte\0"		/* DW_AT_name */
	.byte		2			/* DW_AT_byte_size */
	.byte		7			/* DW_AT_encoding: DW_ATE_unsigned */

.Larray1:
	.uleb128	3			/* Abbrev: DW_TAG_variable */
	.ascii		"array1\0"		/* DW_AT_name */
	.4byte		.L2byte_type-.Lcu1_begin	/* DW_AT_type */
	.byte		2f - 1f			/* DW_AT_location */
1:	.byte		3			/*   DW_OP_addr */
	.4byte		array1			/*     <addr> */
2:

	.uleb128	3			/* Abbrev: DW_TAG_variable */
	.ascii		"array2\0"		/* DW_AT_name */
	.4byte		.L2byte_type-.Lcu1_begin	/* DW_AT_type */
	.byte		2f - 1f			/* DW_AT_location */
1:	.byte		0x98			/*   DW_OP_call2 */
	.2byte		.Larray1-.Lcu1_begin	/*     <current CU offset> */
	.byte		0x23			/*   DW_OP_plus_uconst */
	.uleb128	array2-array1		/*     <uconst> */
2:

	.uleb128	3			/* Abbrev: DW_TAG_variable */
	.ascii		"array3\0"		/* DW_AT_name */
	.4byte		.L2byte_type-.Lcu1_begin	/* DW_AT_type */
	.byte		2f - 1f			/* DW_AT_location */
1:	.byte		0x99			/*   DW_OP_call4 */
	.4byte		.Larray1-.Lcu1_begin	/*     <current CU offset> */
	.byte		0x23			/*   DW_OP_plus_uconst */
	.uleb128	array3-array1		/*     <uconst> */
2:

	.uleb128	3			/* Abbrev: DW_TAG_variable */
	.ascii		"arraycallnoloc\0"	/* DW_AT_name */
	.4byte		.L2byte_type-.Lcu1_begin /* DW_AT_type */
	.byte		2f - 1f			/* DW_AT_location */
1:	.byte		0x99			/*   DW_OP_call4 */
	.4byte		.Larraynoloc-.Lcu1_begin /*     <current CU offset> */
2:

.Larraynoloc:
	.uleb128	4			/* Abbrev: DW_TAG_variable-loclist */
	.ascii		"arraynoloc\0"		/* DW_AT_name */
	.4byte		.L2byte_type-.Lcu1_begin /* DW_AT_type */
	.4byte		loclist			/* DW_AT_location */

	.byte		0			/* End of children of CU */

.Lcu1_end:

/* Location list.  */
	.section	.debug_loc
loclist:
	/* Location list end.  */
	.4byte	0, 0

/* Abbrev table */
	.section .debug_abbrev
.Labbrev1_begin:
	.uleb128	1			/* Abbrev code */
	.uleb128	0x11			/* DW_TAG_compile_unit */
	.byte		1			/* has_children */
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
	.uleb128	0x34			/* DW_TAG_variable */
	.byte		0			/* has_children */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x49			/* DW_AT_type */
	.uleb128	0x13			/* DW_FORM_ref4 */
	.uleb128	0x2			/* DW_AT_location */
	.uleb128	0xa			/* DW_FORM_block1 */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.uleb128	4			/* Abbrev code */
	.uleb128	0x34			/* DW_TAG_variable-loclist */
	.byte		0			/* has_children */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x49			/* DW_AT_type */
	.uleb128	0x13			/* DW_FORM_ref4 */
	.uleb128	0x2			/* DW_AT_location */
	.uleb128	0x06			/* DW_FORM_data4 */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
