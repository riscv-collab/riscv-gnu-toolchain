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


/* Test the behaviour of gdb in the following situation, the dwarf debug
   information describes a parameter as being in a register but a more
   recent (inner) frame marks the register as being undefined.

   This can arrise if the dwarf producer has the location of a parameter in
   a callee clobbered register at the time of a call.  Older versions of
   gcc used to do this, newer versions seem to avoid this issue.

   Though it could be argued that such dwarf is incorrect, we would still
   like gdb to behave in a user friendly, and helpful way when presented
   with such dwarf.  */

/* There are 4 test cases in this assembler file.  In each case function
   main calls each test function in turn, each test case then calls the
   breakpt function.

   We don't actually pass any parameters around, we don't need to, instead
   the dwarf for each test function declares that the function has some
   parameters, and tells us which registers these parameters are in.  The
   breakpt function marks some of these registers as undefined.  The main
   function helpfully places some marker values into all the registers that
   are used as parameters so we can see if they ever get printed.

   We use gdb to break in the breakpt function for each of the 4 test
   cases, and then backtrace through the test function back to main.  In
   each case some or all of the parameters to the test function should be
   marked as optimized out, due to the breakpt function effectively
   clobbering them.

   The dwarf register numbering is different to the gdb register number.
   In some of the tests we rely on gdb behaviour of being able to specify a
   struct using a single register location, the structure will then "flow"
   into the next gdb register.  The initial register is specified using a
   dwarf register number, but the "next" register will depend on gdb
   register ordering.

   Exposing this internal gdb register numbering is really a gdb bug, the
   functionality for selecting the "next" register should be moved into
   target dependent code (such as AVR).  Right now we work around this
   bug in this test; if the bug is ever fixed this test is going to need
   some tweaking.

   The breakpt function always marks rcx, rsi, and rdi as undefined.

      register | dwarf | gdb   |         |
      name     | reg # | reg # | breakpt |
     ----------|-------|-------|---------|
        rdx    | 1     | 3     |         |
        rcx    | 2     | 2     | undef   |
        rbx    | 3     | 1     |         |
        rsi    | 4     | 4     | undef   |
        rdi    | 5     | 5     | undef   |

   We declare the test parameters to be in the register rdx, rcx, rbx, rsi,
   and rdi.  Of these, rdx and rbx are not traditionally used for parameter
   passing, but that really doesn't matter for this test.

   int_param_single_reg_loc: Passes 8-byte integer parameters in 8-byte
                             registers using DW_OP_regn style location
                             information.  The parameters are placed as
                             follows, operand0 (rcx), operand1 (rbx),
                             operand2 (rsi).  We expect operand0 and
                             operand2 to be marked as optimised out, but
                             operand1 to be valid.

   struct_param_single_reg_loc: Passes 16-byte structures in two 8-byte
                                registers using dwarf DW_OP_regn location
                                information to describe a single register,
                                gdb will assume that the structure flows
                                into the next sequential register.  The
                                parameters are placed as follows, operand0
                                (rbx/rcx), operand1 (rcx/rdx), and operand2
                                (rsi/rdi).  The reuse of rcx between
                                operand0 and operand1 is intentional.

   struct_param_two_reg_pieces: Passes 16-byte structure in two 8-byte
                                registers using dwarf DW_OP_piece based
                                location information to describe both
                                registers.  The parameters are placed as
                                follows, operand0 (rdx/rcx), operand1
                                (rcx/rbx), and operand2 (rsi/rdi).  The
                                reuse of rcx between operand0 and operand1
                                is intentional.

   int_param_two_reg_pieces: Passes 8-byte integer values in two 8-byte
                             registers with 4-bytes being placed in each
                             register, using dwarf DW_OP_piece based
                             location information to describe how the
                             parameters are split up.The parameters are
                             placed as follows, operand0 (rdx/rcx),
                             operand1 (rcx/rbx), and operand2 (rsi/rdi).
                             The reuse of rcx between operand0 and operand1
                             is intentional.
*/

        .text

.Ltext0:

        /* main */
.globl main
        .type   main, @function
main:
.Ltext1:
        sub    $0x8,%rsp
.Ltext2:
        movq    $0xdeadbe00deadbe01, %rbx
        movq    $0xdeadbe02deadbe03, %rcx
        movq    $0xdeadbe04deadbe05, %rdx
        movq    $0xdeadbe06deadbe07, %rsi
        movq    $0xdeadbe08deadbe09, %rdi

        callq    int_param_single_reg_loc
        nop

        callq    struct_param_single_reg_loc
        nop

        callq    struct_param_two_reg_pieces
        nop

        callq    int_param_two_reg_pieces
        nop

        add    $0x8,%rsp
        retq
.Ltext3:
        .size   main, .-main

        /* breakpt */
.globl breakpt
        .type   breakpt, @function
breakpt:
.Ltext4:
        sub    $0x8,%rsp
        add    $0x8, %rsp
        retq
.Ltext5a:
        .size   breakpt, .-breakpt

        /* int_param_single_reg_loc */
.globl int_param_single_reg_loc
        .type   int_param_single_reg_loc, @function
int_param_single_reg_loc:
.Ltext5b:
        sub    $0x8,%rsp
.Ltext6:
        nop
        callq    breakpt
        nop
        add    $0x8,%rsp
        retq
.Ltext7:
        .size   int_param_single_reg_loc, .-int_param_single_reg_loc

        /* struct_param_single_reg_loc */
.globl struct_param_single_reg_loc
        .type   struct_param_single_reg_loc, @function
struct_param_single_reg_loc:
.Ltext8:
        sub    $0x8,%rsp
.Ltext9:
        nop
        callq    breakpt
        nop
        add    $0x8,%rsp
        retq
.Ltext10:
        .size   struct_param_single_reg_loc, .-struct_param_single_reg_loc

        /* struct_param_two_reg_pieces */
.globl struct_param_two_reg_pieces
        .type   struct_param_two_reg_pieces, @function
struct_param_two_reg_pieces:
.Ltext11:
        sub    $0x8,%rsp
.Ltext12:
        nop
        callq    breakpt
        nop
        add    $0x8,%rsp
        retq
.Ltext13:
        .size   struct_param_two_reg_pieces, .-struct_param_two_reg_pieces

        /* int_param_two_reg_pieces */
.globl int_param_two_reg_pieces
        .type   int_param_two_reg_pieces, @function
int_param_two_reg_pieces:
.Ltext14:
        sub    $0x8,%rsp
.Ltext15:
        nop
        callq    breakpt
        nop
        add    $0x8,%rsp
        retq
.Ltext16:
        .size   int_param_two_reg_pieces, .-int_param_two_reg_pieces


.Letext0:

        /*******************************************************/

        .section	.debug_frame,"",@progbits

        /* CIE */
.Lframe0:
	.long	.LECIE0-.LSCIE0 /* length */
.LSCIE0:
	.long	0xffffffff      /* CIE_id */
	.byte	0x1             /* version */
	.string	""              /* augmentation */
	.uleb128 0x1            /* code alignment */
	.sleb128 -8             /* data alignment */
	.byte	0x10            /* R/A column */
        /* Initial instructions */
	.byte	0xc             /* DW_CFA_def_cfa */
	.uleb128 0x7            /* reg# */
	.uleb128 0x8            /* offset */
	.byte	0x90            /* DW_CFA_offset (r16) */
	.uleb128 0x1            /* offset */
	.align 8
.LECIE0:

        /* FDE : breakpt */
.LSFDE0:
	.long	.LEFDE0-.LASFDE0        /* length */
.LASFDE0:
	.long	.Lframe0                /* CIE reference */
	.quad	.Ltext4                 /* start */
	.quad	.Ltext5a-.Ltext4        /* length */
        /* Instructions */
        .byte   0x7                     /* DW_CFA_undefined */
        .uleb128 0x2                    /* reg# */
        .byte   0x7                     /* DW_CFA_undefined */
        .uleb128 0x4                    /* reg# */
        .byte   0x7                     /* DW_CFA_undefined */
        .uleb128 0x5                    /* reg# */
	.align 8
.LEFDE0:

        /* FDE : int_param_single_reg_loc */
.LSFDE2:
	.long	.LEFDE2-.LASFDE2        /* length */
.LASFDE2:
	.long	.Lframe0                /* CIE reference */
	.quad	.Ltext5b                /* start */
	.quad	.Ltext7-.Ltext5b        /* length */
        /* Instructions */
 	.byte	0x4
	.long	.Ltext6-.Ltext5b
	.byte	0xe
	.uleb128 0x10
	.align 8
.LEFDE2:

        /* FDE : struct_param_single_reg_loc */
.LSFDE3:
	.long	.LEFDE3-.LASFDE3        /* length */
.LASFDE3:
	.long	.Lframe0                /* CIE reference */
	.quad	.Ltext8                 /* start */
	.quad	.Ltext10-.Ltext8        /* length */
        /* Instructions */
 	.byte	0x4
	.long	.Ltext9-.Ltext8
	.byte	0xe
	.uleb128 0x10
	.align 8
.LEFDE3:

        /* FDE : struct_param_two_reg_pieces */
.LSFDE4:
	.long	.LEFDE4-.LASFDE4        /* length */
.LASFDE4:
	.long	.Lframe0                /* CIE reference */
	.quad	.Ltext11                /* start */
	.quad	.Ltext13-.Ltext11       /* length */
        /* Instructions */
 	.byte	0x4
	.long	.Ltext12-.Ltext11
	.byte	0xe
	.uleb128 0x10
	.align 8
.LEFDE4:

        /* FDE : int_param_two_reg_pieces */
.LSFDE5:
	.long	.LEFDE5-.LASFDE5        /* length */
.LASFDE5:
	.long	.Lframe0                /* CIE reference */
	.quad	.Ltext14                /* start */
	.quad	.Ltext16-.Ltext14       /* length */
        /* Instructions */
 	.byte	0x4
	.long	.Ltext15-.Ltext14
	.byte	0xe
	.uleb128 0x10
	.align 8
.LEFDE5:

        /* FDE : main */
.LSFDE9:
	.long	.LEFDE9-.LASFDE9        /* length */
.LASFDE9:
	.long	.Lframe0                /* CIE reference */
	.quad	.Ltext1                 /* start */
	.quad	.Ltext3-.Ltext1         /* length */
        /* Instructions */
 	.byte	0x4
	.long	.Ltext2-.Ltext1
	.byte	0xe
	.uleb128 0x10
	.align 8
.LEFDE9:

        /*******************************************************/

	.section	.debug_info,"",@progbits
.Ldebug_info0:
        .long  .Ldebug_info_end - .Ldebug_info_start   /* Length */
.Ldebug_info_start:
	.value	0x2                     /* DWARF version number. */
	.long	.Ldebug_abbrev0         /* Offset into .debug_abbrev */
	.byte	0x8                     /* Pointer size */

.LDI0:
	.uleb128 0x1                    /* DW_TAG_compile_unit */
	.string	"GNU C 4.2.1"           /* DW_AT_producer */
	.byte	0x1                     /* DW_AT_language */
	.quad	.Ltext0                 /* DW_AT_low_pc */
	.quad	.Letext0                /* DW_AT_high_pc */

.LDI1:
	.uleb128 0x2                    /* DW_TAG_subprogram */
	.byte	0x1                     /* DW_AT_external */
	.string	"breakpt"               /* DW_AT_name */
	.byte	0x1                     /* DW_AT_prototyped */
	.quad	.Ltext4                 /* DW_AT_low_pc */
	.quad	.Ltext5a                /* DW_AT_high_pc */

.LDI2:
	.uleb128 0x5                    /* DW_TAG_base_type */
	.byte	0x8                     /* DW_AT_byte_size */
	.byte	0x5                     /* DW_AT_encoding (DW_ATE_signed) */
	.string	"long int"              /* DW_AT_name */

.LDI3:
        .uleb128 0x7                    /* DW_TAG_structure_type */
        .string "big"                   /* DW_AT_name */
        .byte   0x10                    /* DW_AT_byte_size */
        .long   .LDI6 - .Ldebug_info0   /* DW_AT_sibling */

.LDI4:
        .uleb128 0x8                    /* DW_TAG_member */
        .string "a"                     /* DW_AT_name */
        .long   .LDI2 - .Ldebug_info0   /* DW_AT_type */
        .byte   0x2                     /* DW_AT_data_member_location : length */
        .byte   0x23                    /* DW_OP_plus_uconst */
        .uleb128 0x0                    /*   + 0 */

.LDI5:
        .uleb128 0x8                    /* DW_TAG_member */
        .string "b"                     /* DW_AT_name */
        .long   .LDI2 - .Ldebug_info0   /* DW_AT_type */
        .byte   0x2                     /* DW_AT_data_member_location : length */
        .byte   0x23                    /* DW_OP_plus_uconst */
        .uleb128 0x8                    /*   + 8 */
        .byte   0x0

.LDI6:
	.uleb128 0x6                    /* DW_TAG_subprogram */
	.byte	0x1                     /* DW_AT_external */
	.string	"main"                  /* DW_AT_name */
	.long	.LDI2 - .Ldebug_info0   /* DW_AT_type */
	.quad	.Ltext1                 /* DW_AT_low_pc */
	.quad	.Ltext3                 /* DW_AT_high_pc */

.LDI7:
	.uleb128 0x3                    /* DW_TAG_subprogram */
	.byte	0x1                     /* DW_AT_external */
	.string	"int_param_single_reg_loc"                 /* DW_AT_name */
	.byte	0x1                     /* DW_AT_prototyped */
	.quad	.Ltext5b                /* DW_AT_low_pc */
	.quad	.Ltext7                 /* DW_AT_high_pc */
	.long	.LDI11 - .Ldebug_info0  /* DW_AT_sibling */

.LDI8:
	.uleb128 0x4                    /* DW_TAG_formal_parameter */
	.string	"operand0"              /* DW_AT_name */
	.long	.LDI2 - .Ldebug_info0   /* DW_AT_type */
	.byte	1                       /* DW_AT_location : length */
	.byte	0x52                    /* DW_OP_reg2 */

.LDI9:
	.uleb128 0x4                    /* DW_TAG_formal_parameter */
	.string	"operand1"              /* DW_AT_name */
	.long	.LDI2 - .Ldebug_info0   /* DW_AT_type */
	.byte	1                       /* DW_AT_location : length */
	.byte	0x53                    /* DW_OP_reg3 */

.LDI10:
	.uleb128 0x4                    /* DW_TAG_formal_parameter */
	.string	"operand2"              /* DW_AT_name */
	.long	.LDI2 - .Ldebug_info0   /* DW_AT_type */
	.byte	1                       /* DW_AT_location : length */
	.byte	0x54                    /* DW_OP_reg4 */

	.byte	0x0

.LDI11:
	.uleb128 0x3                    /* DW_TAG_subprogram */
	.byte	0x1                     /* DW_AT_external */
	.string	"struct_param_single_reg_loc"                 /* DW_AT_name */
	.byte	0x1                     /* DW_AT_prototyped */
	.quad	.Ltext8                 /* DW_AT_low_pc */
	.quad	.Ltext10                /* DW_AT_high_pc */
	.long	.LDI15 - .Ldebug_info0  /* DW_AT_sibling */

.LDI12:
	.uleb128 0x4                    /* DW_TAG_formal_parameter */
	.string	"operand0"              /* DW_AT_name */
	.long	.LDI3 - .Ldebug_info0   /* DW_AT_type */
	.byte	1                       /* DW_AT_location : length */
	.byte	0x53                    /* DW_OP_reg3 */

.LDI13:
	.uleb128 0x4                    /* DW_TAG_formal_parameter */
	.string	"operand1"              /* DW_AT_name */
	.long	.LDI3 - .Ldebug_info0   /* DW_AT_type */
	.byte	1                       /* DW_AT_location : length */
	.byte	0x52                    /* DW_OP_reg2 */

.LDI14:
	.uleb128 0x4                    /* DW_TAG_formal_parameter */
	.string	"operand2"              /* DW_AT_name */
	.long	.LDI3 - .Ldebug_info0   /* DW_AT_type */
	.byte	1                       /* DW_AT_location : length */
	.byte	0x54                    /* DW_OP_reg4 */

	.byte	0x0

.LDI15:
	.uleb128 0x3                    /* DW_TAG_subprogram */
	.byte	0x1                     /* DW_AT_external */
	.string	"struct_param_two_reg_pieces"                 /* DW_AT_name */
	.byte	0x1                     /* DW_AT_prototyped */
	.quad	.Ltext11                /* DW_AT_low_pc */
	.quad	.Ltext13                /* DW_AT_high_pc */
	.long	.LDI19 - .Ldebug_info0  /* DW_AT_sibling */

.LDI16:
	.uleb128 0x4                    /* DW_TAG_formal_parameter */
	.string	"operand0"              /* DW_AT_name */
	.long	.LDI3 - .Ldebug_info0   /* DW_AT_type */
	.byte	6                       /* DW_AT_location : length */
	.byte	0x51                    /* DW_OP_reg1 */
        .byte   0x93                    /* DW_OP_piece */
        .uleb128 0x8                    /*    8 bytes */
	.byte	0x52                    /* DW_OP_reg2 */
        .byte   0x93                    /* DW_OP_piece */
        .uleb128 0x8                    /*    8 bytes */

.LDI17:
	.uleb128 0x4                    /* DW_TAG_formal_parameter */
	.string	"operand1"              /* DW_AT_name */
	.long	.LDI3 - .Ldebug_info0   /* DW_AT_type */
	.byte	6                       /* DW_AT_location : length */
	.byte	0x52                    /* DW_OP_reg2 */
        .byte   0x93                    /* DW_OP_piece */
        .uleb128 0x8                    /*    8 bytes */
	.byte	0x53                    /* DW_OP_reg3 */
        .byte   0x93                    /* DW_OP_piece */
        .uleb128 0x8                    /*    8 bytes */

.LDI18:
	.uleb128 0x4                    /* DW_TAG_formal_parameter */
	.string	"operand2"              /* DW_AT_name */
	.long	.LDI3 - .Ldebug_info0   /* DW_AT_type */
	.byte	6                       /* DW_AT_location : length */
	.byte	0x54                    /* DW_OP_reg4 */
        .byte   0x93                    /* DW_OP_piece */
        .uleb128 0x8                    /*    8 bytes */
	.byte	0x55                    /* DW_OP_reg5 */
        .byte   0x93                    /* DW_OP_piece */
        .uleb128 0x8                    /*    8 bytes */

	.byte	0x0

.LDI19:
	.uleb128 0x3                    /* DW_TAG_subprogram */
	.byte	0x1                     /* DW_AT_external */
	.string	"int_param_two_reg_pieces"                 /* DW_AT_name */
	.byte	0x1                     /* DW_AT_prototyped */
	.quad	.Ltext14                /* DW_AT_low_pc */
	.quad	.Ltext16                /* DW_AT_high_pc */
	.long	.LDIE0 - .Ldebug_info0  /* DW_AT_sibling */

.LDI20:
	.uleb128 0x4                    /* DW_TAG_formal_parameter */
	.string	"operand0"              /* DW_AT_name */
	.long	.LDI2 - .Ldebug_info0   /* DW_AT_type */
	.byte	6                       /* DW_AT_location : length */
	.byte	0x51                    /* DW_OP_reg1 */
        .byte   0x93                    /* DW_OP_piece */
        .uleb128 0x4                    /*    4 bytes */
	.byte	0x52                    /* DW_OP_reg2 */
        .byte   0x93                    /* DW_OP_piece */
        .uleb128 0x4                    /*    4 bytes */

.LDI21:
	.uleb128 0x4                    /* DW_TAG_formal_parameter */
	.string	"operand1"              /* DW_AT_name */
	.long	.LDI2 - .Ldebug_info0   /* DW_AT_type */
	.byte	6                       /* DW_AT_location : length */
	.byte	0x52                    /* DW_OP_reg2 */
        .byte   0x93                    /* DW_OP_piece */
        .uleb128 0x4                    /*    4 bytes */
	.byte	0x53                    /* DW_OP_reg3 */
        .byte   0x93                    /* DW_OP_piece */
        .uleb128 0x4                    /*    4 bytes */

.LDI22:
	.uleb128 0x4                    /* DW_TAG_formal_parameter */
	.string	"operand2"              /* DW_AT_name */
	.long	.LDI2 - .Ldebug_info0   /* DW_AT_type */
	.byte	6                       /* DW_AT_location : length */
	.byte	0x54                    /* DW_OP_reg4 */
        .byte   0x93                    /* DW_OP_piece */
        .uleb128 0x4                    /*    4 bytes */
	.byte	0x55                    /* DW_OP_reg5 */
        .byte   0x93                    /* DW_OP_piece */
        .uleb128 0x4                    /*    4 bytes */

	.byte	0x0

.LDIE0:
	.byte	0x0
.Ldebug_info_end:

        /*******************************************************/

	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1    /* abbrev code */
	.uleb128 0x11   /* TAG: DW_TAG_compile_unit */
	.byte	0x1     /* DW_CHILDREN_yes */
	.uleb128 0x25   /* DW_AT_producer */
	.uleb128 0x8    /*   DW_FORM_string */
	.uleb128 0x13   /* DW_AT_language */
	.uleb128 0xb    /*   DW_FORM_data1 */
        .uleb128 0x11   /* DW_AT_low_pc */
        .uleb128 0x1    /*   DW_FORM_addr */
        .uleb128 0x12   /* DW_AT_high_pc */
        .uleb128 0x1    /*   DW_FORM_addr */
	.byte	0x0
	.byte	0x0

	.uleb128 0x2    /* abbrev code */
	.uleb128 0x2e   /* TAG: DW_TAG_subprogram */
	.byte	0x0     /* DW_CHILDREN_no */
	.uleb128 0x3f   /* DW_AT_external */
	.uleb128 0xc    /*   DW_FORM_flag */
	.uleb128 0x3    /* DW_AT_name */
	.uleb128 0x8    /*   DW_FORM_string */
	.uleb128 0x27   /* DW_AT_prototyped */
	.uleb128 0xc    /*    DW_FORM_flag*/
	.uleb128 0x11   /* DW_AT_low_pc */
	.uleb128 0x1    /*   DW_FORM_addr */
	.uleb128 0x12   /* DW_AT_high_pc */
	.uleb128 0x1    /*   DW_FORM_addr */
	.byte	0x0
	.byte	0x0

	.uleb128 0x3    /* abbrev code */
	.uleb128 0x2e   /* TAG: DW_TAG_subprogram */
	.byte	0x1     /* DW_CHILDREN_yes */
	.uleb128 0x3f   /* DW_AT_external */
	.uleb128 0xc    /*   DW_FORM_flag */
	.uleb128 0x3    /* DW_AT_name */
	.uleb128 0x8    /*   DW_FORM_string */
	.uleb128 0x27   /* DW_AT_prototyped */
	.uleb128 0xc    /*    DW_FORM_flag*/
	.uleb128 0x11   /* DW_AT_low_pc */
	.uleb128 0x1    /*   DW_FORM_addr */
	.uleb128 0x12   /* DW_AT_high_pc */
	.uleb128 0x1    /*   DW_FORM_addr */
	.uleb128 0x1    /* DW_AT_sibling */
	.uleb128 0x13   /*   DW_FORM_ref4 */
	.byte	0x0
	.byte	0x0

	.uleb128 0x4    /* abbrev code */
	.uleb128 0x5    /* TAG: DW_TAG_formal_parameter */
	.byte	0x0     /* DW_CHILDREN_no */
	.uleb128 0x3    /* DW_AT_name */
	.uleb128 0x8    /*   DW_FORM_string */
	.uleb128 0x49   /* DW_AT_type */
	.uleb128 0x13   /*   DW_FORM_ref4 */
	.uleb128 0x2    /* DW_AT_location */
	.uleb128 0xa    /*   DW_FORM_block1 */
	.byte	0x0
	.byte	0x0

	.uleb128 0x5    /* abbrev code */
	.uleb128 0x24   /* TAG: DW_TAG_base_type */
	.byte	0x0     /* DW_CHILDREN_no */
	.uleb128 0xb    /* DW_AT_byte_size */
	.uleb128 0xb    /*   DW_FORM_data1 */
	.uleb128 0x3e   /* DW_AT_encoding */
	.uleb128 0xb    /*   DW_FORM_data1 */
	.uleb128 0x3    /* DW_AT_name */
	.uleb128 0x8    /*   DW_FORM_string */
	.byte	0x0
	.byte	0x0

	.uleb128 0x6    /* abbrev code */
	.uleb128 0x2e   /* TAG: DW_TAG_subprogram */
	.byte	0x0     /* DW_CHILDREN_no */
	.uleb128 0x3f   /* DW_AT_external */
	.uleb128 0xc    /*   DW_FORM_flag */
	.uleb128 0x3    /* DW_AT_name */
	.uleb128 0x8    /*   DW_FORM_string */
	.uleb128 0x49   /* DW_AT_type */
	.uleb128 0x13   /*   DW_FORM_ref4 */
	.uleb128 0x11   /* DW_AT_low_pc */
	.uleb128 0x1    /*   DW_FORM_addr */
	.uleb128 0x12   /* DW_AT_high_pc */
	.uleb128 0x1    /*   DW_FORM_addr */
	.byte	0x0
	.byte	0x0

        .uleb128 0x7    /* abbrev code */
        .uleb128 0x13   /* DW_TAG_structure_type */
        .byte   0x1     /* DW_CHILDREN_yes */
        .uleb128 0x3    /* DW_AT_name */
        .uleb128 0x8    /*   DW_FORM_string */
        .uleb128 0xb    /* DW_AT_byte_size */
        .uleb128 0xb    /*    DW_FORM_data1 */
        .uleb128 0x1    /* DW_AT_sibling */
        .uleb128 0x13   /*   DW_FORM_ref4 */
        .byte   0x0
        .byte   0x0

        .uleb128 0x8    /* abbrev code */
        .uleb128 0xd    /* DW_TAG_member */
        .byte   0x0     /* DW_children_no */
        .uleb128 0x3    /* DW_AT_name */
        .uleb128 0x8    /*   DW_FORM_string */
        .uleb128 0x49   /* DW_AT_type */
        .uleb128 0x13   /*   DW_FORM_ref4 */
        .uleb128 0x38   /* DW_AT_data_member_location */
        .uleb128 0xa    /*   DW_FORM_block1 */
        .byte   0x0
        .byte   0x0

        .byte	0x0
	.section	.note.GNU-stack,"",@progbits
