/* Copyright 2013-2024 Free Software Foundation, Inc.

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

	.text
pc_start:
        /* Enough space to fit at least one instruction.  */
	.4byte	0
pc_end:

	.section	.debug_info
d:
	.4byte	.Ldebug_info_end - 1f	/* Length of Compilation Unit Info */
1:
	.2byte	0x3	/* DWARF version number */
	.4byte	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */
dieb:
	.uleb128 0x1	/* (DIE (0xb) DW_TAG_compile_unit) */
	.ascii	"GCC\0"	/* DW_AT_producer */
	.byte	0x2	/* DW_AT_language = DW_LANG_C */
	.ascii	"z:file.c\0"	/* DW_AT_name */

	.uleb128 0x2		/* (DIE (0xd3) DW_TAG_subprogram) */
	.byte	0x1		/* DW_AT_external */
	.ascii	"func\0"	/* DW_AT_name */
	.4byte	pc_start	/* DW_AT_low_pc */
	.4byte	pc_end		/* DW_AT_high_pc */
	.byte	0x1		/* DW_AT_prototyped */

	.byte	0x0	/* end of children of DIE 0xb */
.Ldebug_info_end:

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

	.uleb128 0x2	/* (abbrev code) */
	.uleb128 0x2e	/* (DW_TAG_subprogram) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0xc	/* (DW_FORM_flag) */
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

	.byte	0x0
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
