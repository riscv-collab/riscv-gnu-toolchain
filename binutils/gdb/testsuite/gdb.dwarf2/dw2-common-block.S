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

/* This was made from ../gdb.fortran/common-block.f90 using
	gfortran -dA -S -g
   and then hand-editing the assembly a bit to introduce
   DW_AT_data_member_location.

   Search for "#if 0" in this file to see the important edits.  */

	.file	"common-block.f90"
	.text
.Ltext0:
	.globl	in_
	.type	in_, @function
in_:
.LFB0:
	.file 1 "../gdb.fortran/common-block.f90"
	# ../gdb.fortran/common-block.f90:21
	.loc 1 21 0
	.cfi_startproc
	# basic block 2
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	# ../gdb.fortran/common-block.f90:34
	.loc 1 34 0
	movl	$5, -4(%rbp)
	# ../gdb.fortran/common-block.f90:35
	.loc 1 35 0
	movl	$55, -8(%rbp)
	# ../gdb.fortran/common-block.f90:21
	.loc 1 21 0
	movl	fo_o_(%rip), %eax
	# ../gdb.fortran/common-block.f90:37
	.loc 1 37 0
	cmpl	$11, %eax
	jne	.L2
	# basic block 3
	# ../gdb.fortran/common-block.f90:21
	.loc 1 21 0 discriminator 2
	movss	fo_o_+4(%rip), %xmm0
	# ../gdb.fortran/common-block.f90:37
	.loc 1 37 0 discriminator 2
	ucomiss	.LC0(%rip), %xmm0
	jp	.L2
	# basic block 4
	ucomiss	.LC0(%rip), %xmm0
	jne	.L2
	# basic block 5
	# ../gdb.fortran/common-block.f90:21
	.loc 1 21 0 discriminator 3
	movsd	fo_o_+8(%rip), %xmm0
	# ../gdb.fortran/common-block.f90:37
	.loc 1 37 0 discriminator 3
	ucomisd	.LC1(%rip), %xmm0
	jp	.L2
	# basic block 6
	ucomisd	.LC1(%rip), %xmm0
	je	.L8
.L2:
	# basic block 7
	# ../gdb.fortran/common-block.f90:37
	.loc 1 37 0 is_stmt 0 discriminator 1
	movl	$0, %eax
	call	_gfortran_abort
.L8:
	# basic block 8
	# ../gdb.fortran/common-block.f90:21
	.loc 1 21 0 is_stmt 1
	movl	foo_(%rip), %eax
	# ../gdb.fortran/common-block.f90:38
	.loc 1 38 0
	cmpl	$1, %eax
	jne	.L5
	# basic block 9
	# ../gdb.fortran/common-block.f90:21
	.loc 1 21 0 discriminator 2
	movss	foo_+4(%rip), %xmm0
	# ../gdb.fortran/common-block.f90:38
	.loc 1 38 0 discriminator 2
	ucomiss	.LC2(%rip), %xmm0
	jp	.L5
	# basic block 10
	ucomiss	.LC2(%rip), %xmm0
	jne	.L5
	# basic block 11
	# ../gdb.fortran/common-block.f90:21
	.loc 1 21 0 discriminator 3
	movsd	foo_+8(%rip), %xmm0
	# ../gdb.fortran/common-block.f90:38
	.loc 1 38 0 discriminator 3
	ucomisd	.LC3(%rip), %xmm0
	jp	.L5
	# basic block 12
	ucomisd	.LC3(%rip), %xmm0
	je	.L9
.L5:
	# basic block 13
	# ../gdb.fortran/common-block.f90:38
	.loc 1 38 0 is_stmt 0 discriminator 1
	movl	$0, %eax
	call	_gfortran_abort
.L9:
	# basic block 14
	# ../gdb.fortran/common-block.f90:40
	.loc 1 40 0 is_stmt 1
	movl	$0, fo_o_(%rip)
	# ../gdb.fortran/common-block.f90:42
	.loc 1 42 0
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	in_, .-in_
	.type	MAIN__, @function
MAIN__:
.LFB1:
	# ../gdb.fortran/common-block.f90:44
	.loc 1 44 0
	.cfi_startproc
	# basic block 2
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	# ../gdb.fortran/common-block.f90:57
	.loc 1 57 0
	movl	$1, foo_(%rip)
	# ../gdb.fortran/common-block.f90:58
	.loc 1 58 0
	movl	$0x40000000, %eax
	movl	%eax, foo_+4(%rip)
	# ../gdb.fortran/common-block.f90:59
	.loc 1 59 0
	movabsq	$4613937818241073152, %rax
	movq	%rax, foo_+8(%rip)
	# ../gdb.fortran/common-block.f90:61
	.loc 1 61 0
	movl	$11, fo_o_(%rip)
	# ../gdb.fortran/common-block.f90:62
	.loc 1 62 0
	movl	$0x41b00000, %eax
	movl	%eax, fo_o_+4(%rip)
	# ../gdb.fortran/common-block.f90:63
	.loc 1 63 0
	movabsq	$4629841154425225216, %rax
	movq	%rax, fo_o_+8(%rip)
	# ../gdb.fortran/common-block.f90:65
	.loc 1 65 0
	movl	$0, %eax
	call	in_
	# ../gdb.fortran/common-block.f90:67
	.loc 1 67 0
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	MAIN__, .-MAIN__
	.globl	main
	.type	main, @function
main:
.LFB2:
	# ../gdb.fortran/common-block.f90:67
	.loc 1 67 0
	.cfi_startproc
	# basic block 2
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	movq	%rsi, -16(%rbp)
	# ../gdb.fortran/common-block.f90:67
	.loc 1 67 0
	movq	-16(%rbp), %rdx
	movl	-4(%rbp), %eax
	movq	%rdx, %rsi
	movl	%eax, %edi
	call	_gfortran_set_args
	movl	$options.0.1570, %esi
	movl	$8, %edi
	call	_gfortran_set_options
	call	MAIN__
	movl	$0, %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	main, .-main
	.comm	foo_,16,16
	.comm	fo_o_,16,16
	.section	.rodata
	.align 32
	.type	options.0.1570, @object
	.size	options.0.1570, 32
options.0.1570:
	.long	68
	.long	511
	.long	0
	.long	0
	.long	0
	.long	1
	.long	0
	.long	1
	.align 4
.LC0:
	.long	1102053376
	.align 8
.LC1:
	.long	0
	.long	1077968896
	.align 4
.LC2:
	.long	1073741824
	.align 8
.LC3:
	.long	0
	.long	1074266112
	.text
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
#if 0
	.long	0x247	# Length of Compilation Unit Info
#else
	.long	.Ldebuginfo_end - .Ldebug_info1	# Length of Compilation Unit Info
.Ldebug_info1:
#endif
	.value	0x4	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF13	# DW_AT_producer: "GNU Fortran 4.6.3 plus hand hackery"
	.byte	0xe	# DW_AT_language
	.byte	0x2	# DW_AT_identifier_case
	.long	.LASF14	# DW_AT_name: "../gdb.fortran/common-block.f90"
	.long	.LASF15	# DW_AT_comp_dir: "gdb.dwarf2"
	.quad	.Ltext0	# DW_AT_low_pc
	.quad	.Letext0	# DW_AT_high_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x2e) DW_TAG_subprogram)
			# DW_AT_external
	.ascii "in\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x15	# DW_AT_decl_line
	.ascii "in_\0"	# DW_AT_linkage_name
	.quad	.LFB0	# DW_AT_low_pc
	.quad	.LFE0	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_tail_call_sites
	.long	0x11a	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x4e) DW_TAG_common_block)
	.long	.LASF0	# DW_AT_name: "fo_o"
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x1f	# DW_AT_decl_line
	.long	.LASF8	# DW_AT_linkage_name: "fo_o_"
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	fo_o_
	.long	0xa5	# DW_AT_sibling
#if 0
	.uleb128 0x4	# (DIE (0x67) DW_TAG_variable)
#else
	.uleb128 0x10	# (DIE (0x1bb) DW_TAG_variable)
#endif
	.ascii "ix\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x17	# DW_AT_decl_line
	.long	0x11a	# DW_AT_type
			# DW_AT_external
#if 0
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	fo_o_
#else
	.uleb128 0x9	# DW_AT_data_member_location
			# (size chosen to avoid renumbering all DIEs)
	.byte	0x8	# DW_OP_const1u
	.byte	0x0
	.byte	0x96	# DW_OP_nop
	.byte	0x96	# DW_OP_nop
	.byte	0x96	# DW_OP_nop
	.byte	0x96	# DW_OP_nop
	.byte	0x96	# DW_OP_nop
	.byte	0x96	# DW_OP_nop
	.byte	0x96	# DW_OP_nop
#endif
	.uleb128 0x4	# (DIE (0x7b) DW_TAG_variable)
	.ascii "iy2\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x18	# DW_AT_decl_line
	.long	0x121	# DW_AT_type
			# DW_AT_external
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	fo_o_+4
	.uleb128 0x4	# (DIE (0x90) DW_TAG_variable)
	.ascii "iz\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x19	# DW_AT_decl_line
	.long	0x128	# DW_AT_type
			# DW_AT_external
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	fo_o_+8
	.byte	0	# end of children of DIE 0x4e
	.uleb128 0x5	# (DIE (0xa5) DW_TAG_common_block)
	.ascii "foo\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x20	# DW_AT_decl_line
	.long	.LASF16	# DW_AT_linkage_name: "foo_"
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	foo_
	.long	0xfe	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0xbe) DW_TAG_variable)
	.long	.LASF1	# DW_AT_name: "ix_x"
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x1b	# DW_AT_decl_line
	.long	0x11a	# DW_AT_type
			# DW_AT_external
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	foo_
	.uleb128 0x6	# (DIE (0xd3) DW_TAG_variable)
	.long	.LASF2	# DW_AT_name: "iy_y"
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x1c	# DW_AT_decl_line
	.long	0x121	# DW_AT_type
			# DW_AT_external
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	foo_+4
	.uleb128 0x6	# (DIE (0xe8) DW_TAG_variable)
	.long	.LASF3	# DW_AT_name: "iz_z2"
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x1d	# DW_AT_decl_line
	.long	0x128	# DW_AT_type
			# DW_AT_external
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	foo_+8
	.byte	0	# end of children of DIE 0xa5
	.uleb128 0x7	# (DIE (0xfe) DW_TAG_variable)
	.ascii "iy\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x22	# DW_AT_decl_line
	.long	0x11a	# DW_AT_type
	.uleb128 0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -20
	.uleb128 0x8	# (DIE (0x10b) DW_TAG_variable)
	.long	.LASF4	# DW_AT_name: "iz_z"
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x23	# DW_AT_decl_line
	.long	0x11a	# DW_AT_type
	.uleb128 0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -24
	.byte	0	# end of children of DIE 0x2e
	.uleb128 0x9	# (DIE (0x11a) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.long	.LASF5	# DW_AT_name: "integer(kind=4)"
	.uleb128 0x9	# (DIE (0x121) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x4	# DW_AT_encoding
	.long	.LASF6	# DW_AT_name: "real(kind=4)"
	.uleb128 0x9	# (DIE (0x128) DW_TAG_base_type)
	.byte	0x8	# DW_AT_byte_size
	.byte	0x4	# DW_AT_encoding
	.long	.LASF7	# DW_AT_name: "real(kind=8)"
	.uleb128 0xa	# (DIE (0x12f) DW_TAG_subprogram)
	.long	.LASF17	# DW_AT_name: "common_test"
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x2c	# DW_AT_decl_line
	.quad	.LFB1	# DW_AT_low_pc
	.quad	.LFE1	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_tail_call_sites
			# DW_AT_main_subprogram
	.byte	0x2	# DW_AT_calling_convention
	.long	0x1f9	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x14d) DW_TAG_common_block)
	.long	.LASF0	# DW_AT_name: "fo_o"
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x1f	# DW_AT_decl_line
	.long	.LASF8	# DW_AT_linkage_name: "fo_o_"
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	fo_o_
	.long	0x1a6	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x166) DW_TAG_variable)
	.long	.LASF1	# DW_AT_name: "ix_x"
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x32	# DW_AT_decl_line
	.long	0x11a	# DW_AT_type
			# DW_AT_external
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	fo_o_
	.uleb128 0x6	# (DIE (0x17b) DW_TAG_variable)
	.long	.LASF2	# DW_AT_name: "iy_y"
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x33	# DW_AT_decl_line
	.long	0x121	# DW_AT_type
			# DW_AT_external
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	fo_o_+4
	.uleb128 0x6	# (DIE (0x190) DW_TAG_variable)
	.long	.LASF4	# DW_AT_name: "iz_z"
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x34	# DW_AT_decl_line
	.long	0x128	# DW_AT_type
			# DW_AT_external
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	fo_o_+8
	.byte	0	# end of children of DIE 0x14d
	.uleb128 0xb	# (DIE (0x1a6) DW_TAG_common_block)
	.ascii "foo\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x20	# DW_AT_decl_line
	.long	.LASF16	# DW_AT_linkage_name: "foo_"
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	foo_
#if 0
	.uleb128 0x4	# (DIE (0x1bb) DW_TAG_variable)
#else
	.uleb128 0x10	# (DIE (0x1bb) DW_TAG_variable)
#endif
	.ascii "ix\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x2e	# DW_AT_decl_line
	.long	0x11a	# DW_AT_type
			# DW_AT_external
#if 0
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	foo_
#else
	.uleb128 0x9	# DW_AT_data_member_location
			# (size chosen to avoid renumbering all DIEs)
	.byte	0x8	# DW_OP_const1u
	.byte	0x0
	.byte	0x96	# DW_OP_nop
	.byte	0x96	# DW_OP_nop
	.byte	0x96	# DW_OP_nop
	.byte	0x96	# DW_OP_nop
	.byte	0x96	# DW_OP_nop
	.byte	0x96	# DW_OP_nop
	.byte	0x96	# DW_OP_nop
#endif
	.uleb128 0x4	# (DIE (0x1cf) DW_TAG_variable)
	.ascii "iy\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x2f	# DW_AT_decl_line
	.long	0x121	# DW_AT_type
			# DW_AT_external
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	foo_+4
	.uleb128 0x4	# (DIE (0x1e3) DW_TAG_variable)
	.ascii "iz\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x30	# DW_AT_decl_line
	.long	0x128	# DW_AT_type
			# DW_AT_external
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	foo_+8
	.byte	0	# end of children of DIE 0x1a6
	.byte	0	# end of children of DIE 0x12f
	.uleb128 0xc	# (DIE (0x1f9) DW_TAG_subprogram)
			# DW_AT_external
	.long	.LASF9	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x43	# DW_AT_decl_line
	.long	0x11a	# DW_AT_type
	.quad	.LFB2	# DW_AT_low_pc
	.quad	.LFE2	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_tail_call_sites
	.long	0x238	# DW_AT_sibling
	.uleb128 0xd	# (DIE (0x21a) DW_TAG_formal_parameter)
	.long	.LASF10	# DW_AT_name: "argc"
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x43	# DW_AT_decl_line
	.long	0x238	# DW_AT_type
	.uleb128 0x2	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -20
	.uleb128 0xd	# (DIE (0x228) DW_TAG_formal_parameter)
	.long	.LASF11	# DW_AT_name: "argv"
	.byte	0x1	# DW_AT_decl_file (../gdb.fortran/common-block.f90)
	.byte	0x43	# DW_AT_decl_line
	.long	0x23d	# DW_AT_type
	.uleb128 0x3	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -32
	.byte	0x6	# DW_OP_deref
	.byte	0	# end of children of DIE 0x1f9
	.uleb128 0xe	# (DIE (0x238) DW_TAG_const_type)
	.long	0x11a	# DW_AT_type
	.uleb128 0xf	# (DIE (0x23d) DW_TAG_pointer_type)
	.byte	0x8	# DW_AT_byte_size
	.long	0x243	# DW_AT_type
	.uleb128 0x9	# (DIE (0x243) DW_TAG_base_type)
	.byte	0x1	# DW_AT_byte_size
	.byte	0x8	# DW_AT_encoding
	.long	.LASF12	# DW_AT_name: "character(kind=1)"
	.byte	0	# end of children of DIE 0xb
#if 0
#else
.Ldebuginfo_end:
#endif
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1	# (abbrev code)
	.uleb128 0x11	# (TAG: DW_TAG_compile_unit)
	.byte	0x1	# DW_children_yes
	.uleb128 0x25	# (DW_AT_producer)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x13	# (DW_AT_language)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x42	# (DW_AT_identifier_case)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x1b	# (DW_AT_comp_dir)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x10	# (DW_AT_stmt_list)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.byte	0
	.byte	0
	.uleb128 0x2	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x6e	# (DW_AT_linkage_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2116	# (DW_AT_GNU_all_tail_call_sites)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x3	# (abbrev code)
	.uleb128 0x1a	# (TAG: DW_TAG_common_block)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x6e	# (DW_AT_linkage_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x4	# (abbrev code)
	.uleb128 0x34	# (TAG: DW_TAG_variable)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0x5	# (abbrev code)
	.uleb128 0x1a	# (TAG: DW_TAG_common_block)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x6e	# (DW_AT_linkage_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x6	# (abbrev code)
	.uleb128 0x34	# (TAG: DW_TAG_variable)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0x7	# (abbrev code)
	.uleb128 0x34	# (TAG: DW_TAG_variable)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0x8	# (abbrev code)
	.uleb128 0x34	# (TAG: DW_TAG_variable)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0x9	# (abbrev code)
	.uleb128 0x24	# (TAG: DW_TAG_base_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3e	# (DW_AT_encoding)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.byte	0
	.byte	0
	.uleb128 0xa	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2116	# (DW_AT_GNU_all_tail_call_sites)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x6a	# (DW_AT_main_subprogram)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x36	# (DW_AT_calling_convention)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xb	# (abbrev code)
	.uleb128 0x1a	# (TAG: DW_TAG_common_block)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x6e	# (DW_AT_linkage_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0xc	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2116	# (DW_AT_GNU_all_tail_call_sites)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xd	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0xe	# (abbrev code)
	.uleb128 0x26	# (TAG: DW_TAG_const_type)
	.byte	0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xf	# (abbrev code)
	.uleb128 0xf	# (TAG: DW_TAG_pointer_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
#if 0
#else
	.uleb128 0x10	# (abbrev code)
	.uleb128 0x34	# (TAG: DW_TAG_variable)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x38	# (DW_AT_data_member_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
#endif
	.byte	0
	.section	.debug_aranges,"",@progbits
	.long	0x2c	# Length of Address Ranges Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.byte	0x8	# Size of Address
	.byte	0	# Size of Segment Descriptor
	.value	0	# Pad to 16 byte boundary
	.value	0
	.quad	.Ltext0	# Address
	.quad	.Letext0-.Ltext0	# Length
	.quad	0
	.quad	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF0:
	.string	"fo_o"
.LASF8:
	.string	"fo_o_"
.LASF3:
	.string	"iz_z2"
.LASF1:
	.string	"ix_x"
.LASF9:
	.string	"main"
.LASF4:
	.string	"iz_z"
.LASF13:
	.string	"GNU Fortran 4.6.3 plus hand hackery"
.LASF16:
	.string	"foo_"
.LASF7:
	.string	"real(kind=8)"
.LASF5:
	.string	"integer(kind=4)"
.LASF17:
	.string	"common_test"
.LASF12:
	.string	"character(kind=1)"
.LASF14:
	.string	"../gdb.fortran/common-block.f90"
.LASF10:
	.string	"argc"
.LASF2:
	.string	"iy_y"
.LASF11:
	.string	"argv"
.LASF15:
	.string	"gdb.dwarf2"
.LASF6:
	.string	"real(kind=4)"
	.ident	"GCC: (GNU) 4.6.3 20120306 (Red Hat 4.6.3-2)"
	.section	.note.GNU-stack,"",@progbits
