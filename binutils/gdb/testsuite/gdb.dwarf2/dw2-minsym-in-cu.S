/* This testcase is part of GDB, the GNU debugger.

   Copyright 2004-2024 Free Software Foundation, Inc.

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
.Lbegin_text1:

	.globl SYMBOL(main)
	.type SYMBOL(main), %function
SYMBOL(main):
.Lbegin_main:
	.int 0
.Lend_main:
	.size SYMBOL(main), .-SYMBOL(main)

	.globl func2
	.type func2, %function
func2:
	.int 0
	.size func2, .-func2

.Lend_text1:

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
	.4byte	.Lend_text1			/* DW_AT_high_pc */
	.4byte	.Lbegin_text1			/* DW_AT_low_pc */
	.ascii	"file1.txt\0"			/* DW_AT_name */
	.ascii	"GNU C 3.3.3\0"			/* DW_AT_producer */
	.byte	1				/* DW_AT_language (C) */

	/* main */
	.uleb128	2			/* Abbrev: DW_TAG_subprogram */
	.byte		1			/* DW_AT_external */
	.ascii		"main\0"		/* DW_AT_name */
	.4byte	.Ldebug_ranges			/* DW_AT_ranges */

	.byte		0			/* End of children of CU */
.Lcu1_end:

/* DW_AT_ranges.  */
	.section	.debug_ranges
.Ldebug_ranges:
	.4byte	.Lbegin_main
	.4byte	.Lend_main - 1
	/* Make it slightly more interesting to set pending_addrmap_interesting.  */
	.4byte	.Lend_main - 1
	.4byte	.Lend_main
	.4byte	0
	.4byte	0

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
	.uleb128	0x2e			/* DW_TAG_subprogram */
	.byte		0			/* has_children */
	.uleb128	0x3f			/* DW_AT_external */
	.uleb128	0xc			/* DW_FORM_flag */
	.uleb128	0x3			/* DW_AT_name */
	.uleb128	0x8			/* DW_FORM_string */
	.uleb128	0x55			/* DW_AT_ranges */
	.uleb128	0x6			/* DW_FORM_data4 */
	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */

	.byte		0x0			/* Terminator */
	.byte		0x0			/* Terminator */
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
