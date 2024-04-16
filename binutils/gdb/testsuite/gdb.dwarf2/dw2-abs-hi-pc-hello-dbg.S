/* Copyright 2014-2024 Free Software Foundation, Inc.

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

	.section	.debug_info
	.4byte	.Ledebug_info0 - .Lsdebug_info0	/* Length of Compilation Unit Info */
.Lsdebug_info0:
	.2byte	0x2	/* DWARF version number */
	.4byte	.Ldebug_abbrev0	/* Offset Into Abbrev. Section */
	.byte	0x4	/* Pointer Size (in bytes) */
	.uleb128 0x1	/* (DIE (0xb) DW_TAG_compile_unit) */
        .ascii "GNU C 4.7.4 20140206 for GNAT Pro 7.3.0w (20140206)\0"
	.byte	0x1	/* DW_AT_language */
	.ascii	"dw2-abs-hi-pc-hello.c\0"     /* DW_AT_name */
	.ascii  "/tmp\0"        /* DW_AT_comp_dir */
	.4byte	HELLO_START	/* DW_AT_low_pc */
	.4byte	HELLO_END	/* DW_AT_high_pc */
	.4byte	.Ldebug_line0	/* DW_AT_stmt_list */
	.uleb128 0x2	/* (DIE (0x2d) DW_TAG_subprogram) */
	.byte	0x1	/* DW_AT_external */
	.ascii	"hello\0"
	.byte	0x1	/* DW_AT_decl_file (hello.c) */
	.byte	0x13	/* DW_AT_decl_line */
	.byte	0x1	/* DW_AT_prototyped */
	.4byte	HELLO_START	/* DW_AT_low_pc */
	.4byte	HELLO_END	/* DW_AT_high_pc */
	.byte	0	/* end of children of DIE 0xb */
.Ledebug_info0:

	.section	.debug_abbrev
.Ldebug_abbrev0:
	.uleb128 0x1	/* (abbrev code) */
	.uleb128 0x11	/* (TAG: DW_TAG_compile_unit) */
	.byte	0x1	/* DW_children_yes */
	.uleb128 0x25	/* (DW_AT_producer) */
	.uleb128 0x8    /* (DW_FORM_string) */
	.uleb128 0x13	/* (DW_AT_language) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8    /* (DW_FORM_string) */
	.uleb128 0x1b	/* (DW_AT_comp_dir) */
	.uleb128 0x8    /* (DW_FORM_string) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.uleb128 0x10	/* (DW_AT_stmt_list) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.byte	0
	.byte	0
	.uleb128 0x2	/* (abbrev code) */
	.uleb128 0x2e	/* (TAG: DW_TAG_subprogram) */
	.byte	0	/* DW_children_no */
	.uleb128 0x3f	/* (DW_AT_external) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x3	/* (DW_AT_name) */
	.uleb128 0x8    /* (DW_FORM_string) */
	.uleb128 0x3a	/* (DW_AT_decl_file) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x3b	/* (DW_AT_decl_line) */
	.uleb128 0xb	/* (DW_FORM_data1) */
	.uleb128 0x27	/* (DW_AT_prototyped) */
	.uleb128 0xc	/* (DW_FORM_flag) */
	.uleb128 0x11	/* (DW_AT_low_pc) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.uleb128 0x12	/* (DW_AT_high_pc) */
	.uleb128 0x6	/* (DW_FORM_data4) */
	.byte	0
	.byte	0
	.byte	0

	.section	.debug_line
.Ldebug_line0:
        .4byte  LELT-LSLT  /* Length of Source Line Info */
LSLT:
        .2byte  0x2    /* DWARF Version */
        .4byte  LELTP-LASLTP     /* Prolog Length */
LASLTP:
        .byte   0x1      /* Minimum Instruction Length */
        .byte   0x1      /* Default is_stmt_start flag */
        .byte   0x1      /* Line Base Value (Special Opcodes) */
        .byte   0x1      /* Line Range Value (Special Opcodes) */
        .byte   0xd      /* Special Opcode Base */
        .byte   0        /* opcode: 0x1 has 0 args */
        .byte   0x1      /* opcode: 0x2 has 1 args */
        .byte   0x1      /* opcode: 0x3 has 1 args */
        .byte   0x1      /* opcode: 0x4 has 1 args */
        .byte   0x1      /* opcode: 0x5 has 1 args */
        .byte   0        /* opcode: 0x6 has 0 args */
        .byte   0        /* opcode: 0x7 has 0 args */
        .byte   0        /* opcode: 0x8 has 0 args */
        .byte   0x1      /* opcode: 0x9 has 1 args */
        .byte   0        /* opcode: 0xa has 0 args */
        .byte   0        /* opcode: 0xb has 0 args */
        .byte   0x1      /* opcode: 0xc has 1 args */
        .byte   0        /* End directory table */
        .ascii "dw2-abs-hi-pc-hello.c\0"       /* File Entry: 0x1 */
        .uleb128 0
        .uleb128 0
        .uleb128 0
        .byte   0        /* End file name table */
LELTP:
        .byte   0        /* set address to .hello_start */
        .uleb128 0x5
        .byte   0x2
        .4byte  HELLO_START
        .byte   0x3      /* DW_LNS_advance_line */
        .sleb128 22      /* ... to 23 */
        .byte   0x5      /* column 0 */
        .uleb128 0       /* 0 */
        .byte           1       /* DW_LNS_copy */

        .byte   0        /* set address to .hello0 */
        .uleb128 0x5
        .byte   0x2
        .4byte  .hello0
        .byte   0x3      /* DW_LNS_advance_line */
        .sleb128 1       /* ... to 24 */
        .byte   0x5      /* column 0 */
        .uleb128 0       /* 0 */
        .byte           1       /* DW_LNS_copy */

        .byte   0        /* set address to .hello1 */
        .uleb128 0x5
        .byte   0x2
        .4byte  .hello1
        .byte   0x3      /* DW_LNS_advance_line */
        .sleb128 1       /* ... to 25 */
        .byte   0x5      /* column 0 */
        .uleb128 0       /* 0 */
        .byte           1       /* DW_LNS_copy */

        .byte   0        /* set address to .hello_end */
        .uleb128 0x5
        .byte   0x2
        .4byte  HELLO_END
        .byte   0        /* end sequence */
        .uleb128 0x1
        .byte   0x1
LELT:
#ifdef __arm__
	.section	.note.GNU-stack,"",%progbits
#else
	.section	.note.GNU-stack,"",@progbits
#endif
