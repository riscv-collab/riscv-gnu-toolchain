/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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

	.text
SYMBOL(main):	.globl SYMBOL(main)

gcc42:	.globl gcc42
.Lgcc42_procstart:
	.int 0
	.type gcc42, %function
	.size gcc42, . - gcc42
.Lgcc42_procend:

gcc43:	.globl gcc43
.Lgcc43_procstart:
	.int 0
	.type gcc43, %function
	.size gcc43, . - gcc43
.Lgcc43_procend:

/* Debug information */

	.section .debug_info
.Lcu1_begin:
	/* CU header */
	.4byte	.Lcu1_end - .Lcu1_start	/* Length of Compilation Unit */
.Lcu1_start:
	.2byte	2				/* DWARF Version */
	.4byte	.Labbrev1_begin			/* Offset into abbrev section */
	.byte	4				/* Pointer size */

	.uleb128 1				/* Abbrev: DW_TAG_compile_unit */
	.4byte	.Lgcc42_begin			/* DW_AT_stmt_list */
	.4byte	.Lgcc42_procstart		/* DW_AT_low_pc */
	.4byte	.Lgcc42_procend			/* DW_AT_high_pc */
	.ascii	"/dir/d/dw2-compdir-oldgcc42.S\0"	/* DW_AT_name */
	.ascii	"GNU C 4.2.0\0"			/* DW_AT_producer */
	.byte	1				/* DW_AT_language (C) */

	.uleb128	2			/* Abbrev: DW_TAG_subprogram */
	.byte		1			/* DW_AT_external */
	.byte		1			/* DW_AT_decl_file */
	.byte		1			/* DW_AT_decl_line */
	.ascii		"gcc42\0"		/* DW_AT_name */
	.4byte		.Lgcc42_procstart	/* DW_AT_low_pc */
	.4byte		.Lgcc42_procend		/* DW_AT_high_pc */

	.byte		0			/* End of children of CU */
.Lcu1_end:

.Lcu2_begin:
	/* CU header */
	.4byte	.Lcu2_end - .Lcu2_start	/* Length of Compilation Unit */
.Lcu2_start:
	.2byte	2				/* DWARF Version */
	.4byte	.Labbrev1_begin			/* Offset into abbrev section */
	.byte	4				/* Pointer size */

	.uleb128 1				/* Abbrev: DW_TAG_compile_unit */
	.4byte	.Lgcc43_begin			/* DW_AT_stmt_list */
	.4byte	.Lgcc43_procstart		/* DW_AT_low_pc */
	.4byte	.Lgcc43_procend			/* DW_AT_high_pc */
	.ascii	"/dir/d/dw2-compdir-oldgcc43.S\0"	/* DW_AT_name */
	.ascii	"GNU C 4.3.0\0"			/* DW_AT_producer */
	.byte	1				/* DW_AT_language (C) */

	.uleb128	2			/* Abbrev: DW_TAG_subprogram */
	.byte		1			/* DW_AT_external */
	.byte		1			/* DW_AT_decl_file */
	.byte		1			/* DW_AT_decl_line */
	.ascii		"gcc43\0"		/* DW_AT_name */
	.4byte		.Lgcc43_procstart	/* DW_AT_low_pc */
	.4byte		.Lgcc43_procend		/* DW_AT_high_pc */

	.byte		0			/* End of children of CU */
.Lcu2_end:

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
	.byte		0			/* has_children */
	.uleb128	0x3f			/* DW_AT_external */
	.uleb128	0xc			/* DW_FORM_flag */
	.uleb128	0x3a			/* DW_AT_decl_file */
	.uleb128	0xb			/* DW_FORM_data1 */
	.uleb128	0x3b			/* DW_AT_decl_line */
	.uleb128	0xb			/* DW_FORM_data1 */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x11			/* DW_AT_low_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.uleb128	0x12			/* DW_AT_high_pc */
	.uleb128	0x1			/* DW_FORM_addr */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

/* Line table */
	.section .debug_line
.Lgcc42_begin:
	.4byte		.Lgcc42_end - .Lgcc42_start	/* Initial length */
.Lgcc42_start:
	.2byte		2			/* Version */
	.4byte		.Lgcc42_lines - .Lgcc42_hdr	/* header_length */
.Lgcc42_hdr:
	.byte		1			/* Minimum insn length */
	.byte		1			/* default_is_stmt */
	.byte		1			/* line_base */
 	.byte		1			/* line_range */
	.byte		4			/* opcode_base */

	/* Standard lengths */
	.byte		0
	.byte		1
	.byte		1

	/* Include directories */
	.byte		0

	/* File names */
	.ascii		"dw2-compdir-oldgcc42.S\0"
	.uleb128	0	/* directory */
	.uleb128	0
	.uleb128	0

	.byte		0

.Lgcc42_lines:
	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	.4byte		gcc42

	.byte		3	/* DW_LNS_advance_line */
	.sleb128	41	/* ... to 42 */

	.byte		1	/* DW_LNS_copy */

	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	.4byte		.Lgcc42_procend

	.byte		0	/* DW_LNE_end_of_sequence */
	.uleb128	1
	.byte		1
.Lgcc42_end:

.Lgcc43_begin:
	.4byte		.Lgcc43_end - .Lgcc43_start	/* Initial length */
.Lgcc43_start:
	.2byte		2			/* Version */
	.4byte		.Lgcc43_lines - .Lgcc43_hdr	/* header_length */
.Lgcc43_hdr:
	.byte		1			/* Minimum insn length */
	.byte		1			/* default_is_stmt */
	.byte		1			/* line_base */
 	.byte		1			/* line_range */
	.byte		4			/* opcode_base */

	/* Standard lengths */
	.byte		0
	.byte		1
	.byte		1

	/* Include directories */
	.byte		0

	/* File names */
	.ascii		"dw2-compdir-oldgcc43.S\0"
	.uleb128	0	/* directory */
	.uleb128	0
	.uleb128	0

	.byte		0

.Lgcc43_lines:
	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	.4byte		gcc43

	.byte		3	/* DW_LNS_advance_line */
	.sleb128	42	/* ... to 43 */

	.byte		1	/* DW_LNS_copy */

	.byte		0	/* DW_LNE_set_address */
	.uleb128	5
	.byte		2
	.4byte		.Lgcc43_procend

	.byte		0	/* DW_LNE_end_of_sequence */
	.uleb128	1
	.byte		1
.Lgcc43_end:
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
