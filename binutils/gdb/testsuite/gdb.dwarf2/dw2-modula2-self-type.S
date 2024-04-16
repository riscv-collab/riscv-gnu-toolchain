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

/* Compiled from:
   MODULE m;
   TYPE t = PROCEDURE (t);
   VAR v: t;
   BEGIN
   END m.
   But "v" has been removed afterwards.  */

	.data
v:	.long	0

	.section	.debug_info
d:
	.4byte	.Ldebug_info_end - 1f	/* Length of Compilation Unit Info */
1:
	.2byte	0x3	/* DWARF version number */
	.4byte	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */
dieb:
	.uleb128 0x1	/* (DIE (0xb) DW_TAG_compile_unit) */
	.4byte	.LASF0	/* DW_AT_producer */
	.byte	0xa	/* DW_AT_language = DW_LANG_Modula2 */
	.4byte	.LASF1	/* DW_AT_name: "2.mod" */
	.4byte	.LASF2	/* DW_AT_comp_dir: "" */

die210:
	.uleb128 0x5	/* (DIE (0x210) DW_TAG_subroutine_type) */
die215:
	.uleb128 0x6	/* (DIE (0x215) DW_TAG_formal_parameter) */
	.4byte	die21b-d	/* DW_AT_type */
	.byte	0x0	/* end of children of DIE 0x210 */
die21b:
	.uleb128 0x7	/* (DIE (0x21b) DW_TAG_pointer_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.4byte	die210-d	/* DW_AT_type */
die221:
	.uleb128 0x8	/* (DIE (0x221) DW_TAG_variable) */
	.ascii "v\0"	/* DW_AT_name */
	.4byte	die21b-d	/* DW_AT_type */
	.byte	2f-1f	/* DW_AT_location */
1:	.byte	0x3	/* DW_OP_addr */
	.4byte	v
2:

	.byte	0x0	/* end of children of DIE 0xb */
.Ldebug_info_end:
	.section	.debug_abbrev
.Ldebug_abbrev0:

	.uleb128 0x1	/* (abbrev code) */
	.uleb128 0x11	/* (TAG: DW_TAG_compile_unit) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x25	/* (DW_AT_producer) */
	.uleb128 0xe	/* (DW_FORM_strp) */
	.uleb128 0x13	/* (DW_AT_language) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0xe	/* (DW_FORM_strp) */
	.uleb128 0x1b	/* (DW_AT_comp_dir) */
	.uleb128 0xe	/* (DW_FORM_strp) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x5	/* (abbrev code) */
	.uleb128 0x15	/* (TAG: DW_TAG_subroutine_type) */
	.byte	0x1	/* DW_children_yes */
	.byte	0x0
	.byte	0x0

	.uleb128 0x6	/* (abbrev code) */
	.uleb128 0x5	/* (TAG: DW_TAG_formal_parameter) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x7	/* (abbrev code) */
	.uleb128 0xf	/* (TAG: DW_TAG_pointer_type) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.byte	0x0
	.byte	0x0

	.uleb128 0x8	/* (abbrev code) */
	.uleb128 0x34	/* (TAG: DW_TAG_variable) */
	.byte	0x0	/* DW_children_no */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x49	/* (DW_AT_type) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x2	/* (DW_AT_location) */
	.uleb128 0xa	/* (DW_FORM_block1) */
	.byte	0x0
	.byte	0x0

	.byte	0x0

	.section	.debug_str
.LASF1:
	.string	"2.mod"
.LASF0:
	.string	"GNU Modula-2 0.78 (20100402) grafted onto GCC 4.1.2"
.LASF2:
	.string	""
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
