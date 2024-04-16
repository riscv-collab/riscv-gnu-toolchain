/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2024 Free Software Foundation, Inc.

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

.Ltext0:
#ifdef __arm__
	.type	p__top__middle__inside.3062, %function
#else
	.type	p__top__middle__inside.3062, @function
#endif
p__top__middle__inside.3062:
.LFB4:
	.file 1 "p.adb"
        .4byte 0
.LBE6:

	.globl	p__top
#ifdef __arm__
	.type	p__top, %function
#else
	.type	p__top, @function
#endif
p__top:
.LFB2:
        .4byte 0
.LFE2:
.Letext0:

	.section	.debug_info
.Ldebug_info0:
	.4byte	.Ledebug_info0 - .Lsdebug_info0  /* Length of CU Info */
.Lsdebug_info0:
	.2byte	0x4	/* DWARF version number */
	.4byte	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */
	.uleb128 0x1	/* (DIE (0xb) DW_TAG_compile_unit) */
	.ascii	"GNU Ada 4.9.0 20140126\0" /* DW_AT_producer */
	.byte	0xd	/* DW_AT_language */
	.ascii	"p.adb\0" /* DW_AT_name */
	.ascii	"/tmp\0"  /* DW_AT_comp_dir */
	.4byte	.Ltext0	/* DW_AT_low_pc */
	.4byte	.Letext0-.Ltext0	/* DW_AT_high_pc */
.S0x142:
	.uleb128 0x8	/* (DIE (0x142) DW_TAG_base_type) */
	.byte	0x4	/* DW_AT_byte_size */
	.byte	0x5	/* DW_AT_encoding */
	.ascii	"integer\0" /* DW_AT_name */

	.uleb128 0x13	/* (DIE (0x1b4) DW_TAG_subprogram) */
			/* DW_AT_external */
	.ascii	"p__top\0" /* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (p.adb) */
	.byte	0x3	/* DW_AT_decl_line */
	.4byte	.LFB2	/* DW_AT_low_pc */
	.4byte	.LFE2-.LFB2	/* DW_AT_high_pc */
	.uleb128 0x1	/* DW_AT_frame_base */
	.byte	0x9c	/* DW_OP_call_frame_cfa */
			/* DW_AT_GNU_all_call_sites */
.S0x1e0:
	.uleb128 0x15	/* (DIE (0x1e0) DW_TAG_subprogram) */
	.ascii	"p__top__middle\0" /* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (p.adb) */
	.byte	0x4	/* DW_AT_decl_line */
	.byte	0x1	/* DW_AT_inline */
.S0x202:
	.uleb128 0x15	/* (DIE (0x202) DW_TAG_subprogram) */
	.ascii	"p__top__middle__inside\0" /* DW_AT_name */
	.byte	0x1	/* DW_AT_decl_file (p.adb) */
	.byte	0x5	/* DW_AT_decl_line */
	.byte	0x1	/* DW_AT_inline */
	.byte	0	/* end of children of DIE 0x202 */
.S0x225:
	.uleb128 0x18	/* (DIE (0x225) DW_TAG_subprogram) */
	.4byte	.S0x202 - .Ldebug_info0	/* DW_AT_abstract_origin */
	.4byte	.LFB4	/* DW_AT_low_pc */
	.4byte	.LBE6-.LFB4	/* DW_AT_high_pc */
	.uleb128 0x1	/* DW_AT_frame_base */
	.byte	0x9c	/* DW_OP_call_frame_cfa */
	.uleb128 0x1	/* DW_AT_static_link */
	.byte	0x56	/* DW_OP_reg6 */
			/* DW_AT_GNU_all_call_sites */
	.uleb128 0x1a	/* (DIE (0x247) DW_TAG_inlined_subroutine) */
	.4byte	.S0x1e0 - .Ldebug_info0	/* DW_AT_abstract_origin */
	.4byte	.LFB4	/* DW_AT_low_pc */
	.4byte	.LBE6-.LFB4	/* DW_AT_high_pc */
	.byte	0x1	/* DW_AT_call_file (p.adb) */
	.byte	0x14	/* DW_AT_call_line */
	.byte	0	/* end of children of DIE 0x247 */
	.byte	0	/* end of children of DIE 0x225 */
	.byte	0	/* end of children of DIE 0x1e0 */
.S0x374:
	.uleb128 0x23	/* (DIE (0x382) DW_TAG_inlined_subroutine) */
	.4byte	.S0x1e0 - .Ldebug_info0 /* DW_AT_abstract_origin */
	.4byte	.LFB4	/* DW_AT_low_pc */
	.4byte	.LBE6-.LFB4	/* DW_AT_high_pc */
	.byte	0x1	/* DW_AT_call_file (p.adb) */
	.byte	0x1d	/* DW_AT_call_line */
	.byte	0	/* end of children of DIE 0x382 */
	.byte	0	/* end of children of DIE 0x1b4 */
.S0x4fc:
	.uleb128 0x28	/* (DIE (0x52e) DW_TAG_subprogram) */
			/* DW_AT_external */
	.ascii	"__gnat_rcheck_PE_Explicit_Raise\0" /* DW_AT_name */
			/* DW_AT_artificial */
			/* DW_AT_declaration */
	.byte	0	/* end of children of DIE 0x52e */
	.byte	0	/* end of children of DIE 0xb */
.Ledebug_info0:

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
	.uleb128 0x1b	/* (DW_AT_comp_dir) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.byte	0
	.byte	0
	.uleb128 0x8	/* (abbrev code) */
	.uleb128 0x24	/* (TAG: DW_TAG_base_type) */
	.byte	0	/* DW_children_no */
	.uleb128 0xb	/* (DW_AT_byte_size) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3e	/* (DW_AT_encoding) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.byte	0
	.byte	0
	.uleb128 0x13	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.uleb128 0x40	/* (DW_AT_frame_base) */
	.uleb128 0x18	/* (DW_FORM_exprloc) */
	.uleb128 0x2117	/* (DW_AT_GNU_all_call_sites) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.byte	0
	.byte	0
	.uleb128 0x15	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x20	/* (DW_AT_inline) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0
	.byte	0
	.uleb128 0x18	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.uleb128 0x40	/* (DW_AT_frame_base) */
	.uleb128 0x18	/* (DW_FORM_exprloc) */
	.uleb128 0x48	/* (DW_AT_static_link) */
	.uleb128 0x18	/* (DW_FORM_exprloc) */
	.uleb128 0x2117	/* (DW_AT_GNU_all_call_sites) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.byte	0
	.byte	0
	.uleb128 0x1a	/* (abbrev code) */
	.uleb128 0x1d	/* (TAG: DW_TAG_inlined_subroutine) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.uleb128 0x58	/* (DW_AT_call_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x59	/* (DW_AT_call_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0
	.byte	0
	.uleb128 0x23	/* (abbrev code) */
	.uleb128 0x1d	/* (TAG: DW_TAG_inlined_subroutine) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x31	/* (DW_AT_abstract_origin) */
	.uleb128 0x13	/* (DW_FORM_ref4) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x1	/* (DW_FORM_addr) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.uleb128 0x58	/* (DW_AT_call_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x59	/* (DW_AT_call_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.byte	0
	.byte	0
	.uleb128 0x28	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8	/* (DW_FORM_string) */
	.uleb128 0x34	/* (DW_AT_artificial) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.uleb128 0x3c	/* (DW_AT_declaration) */
	.uleb128 0x19	/* (DW_FORM_flag_present) */
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0

        .section .debug_line
.Lline1_begin:
        .byte   0

#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
