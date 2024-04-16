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
      	
/* This file is compiled from gdb.arch/amd64-prologue-xmm.c
   using -g -dA -S.  */

	.file	"amd64-prologue-xmm.c"
	.text
.Ltext0:
	.local	v
	.comm	v,4,4
	.local	fail
	.comm	fail,4,4
	.type	func, @function
func:
.LFB0:
	.file 1 "gdb.arch/amd64-prologue-xmm.c"
	# gdb.arch/amd64-prologue-xmm.c:22
	.loc 1 22 0
	.cfi_startproc
	# basic block 2
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$72, %rsp
	movq	%rsi, -168(%rbp)
	movq	%rdx, -160(%rbp)
	movq	%rcx, -152(%rbp)
	movq	%r8, -144(%rbp)
	movq	%r9, -136(%rbp)
	testb	%al, %al
	je	.L2
	# basic block 3
	# gdb.arch/amd64-prologue-xmm.c:22
	.loc 1 22 0
	movaps	%xmm0, -128(%rbp)
	movaps	%xmm1, -112(%rbp)
	movaps	%xmm2, -96(%rbp)
	movaps	%xmm3, -80(%rbp)
	movaps	%xmm4, -64(%rbp)
	movaps	%xmm5, -48(%rbp)
	movaps	%xmm6, -32(%rbp)
	movaps	%xmm7, -16(%rbp)
.L2:
	# basic block 4
	movl	%edi, -180(%rbp)
	# gdb.arch/amd64-prologue-xmm.c:23
	.loc 1 23 0
	movl	-180(%rbp), %eax
	movl	%eax, v(%rip)
	# gdb.arch/amd64-prologue-xmm.c:24
	.loc 1 24 0
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	func, .-func
	.type	marker, @function
marker:
.LFB1:
	# gdb.arch/amd64-prologue-xmm.c:28
	.loc 1 28 0
	.cfi_startproc
	# basic block 2
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	# gdb.arch/amd64-prologue-xmm.c:29
	.loc 1 29 0
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	marker, .-marker
	.globl	main
	.type	main, @function
main:
.LFB2:
	# gdb.arch/amd64-prologue-xmm.c:33
	.loc 1 33 0
	.cfi_startproc
	# basic block 2
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	# gdb.arch/amd64-prologue-xmm.c:34
	.loc 1 34 0
	movl	$1, %edi
	movl	$0, %eax
	call	func
	# gdb.arch/amd64-prologue-xmm.c:35
	.loc 1 35 0
	movl	$1, fail(%rip)
	# gdb.arch/amd64-prologue-xmm.c:36
	.loc 1 36 0
	call	marker
	# gdb.arch/amd64-prologue-xmm.c:37
	.loc 1 37 0
	movl	$0, %eax
	# gdb.arch/amd64-prologue-xmm.c:38
	.loc 1 38 0
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	main, .-main
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0xc0	# Length of Compilation Unit Info
	.value	0x4	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF1	# DW_AT_producer: "GNU C 4.6.1 20110715 (Red Hat 4.6.1-3)"
	.byte	0x1	# DW_AT_language
	.long	.LASF2	# DW_AT_name: "gdb.arch/amd64-prologue-xmm.c"
	.long	.LASF3	# DW_AT_comp_dir: ""
	.quad	.Ltext0	# DW_AT_low_pc
	.quad	.Letext0	# DW_AT_high_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x2d) DW_TAG_subprogram)
	.long	.LASF4	# DW_AT_name: "func"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-prologue-xmm.c)
	.byte	0x15	# DW_AT_decl_line
			# DW_AT_prototyped
	.quad	.LFB0	# DW_AT_low_pc
	.quad	.LFE0	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.long	0x59	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x4a) DW_TAG_formal_parameter)
	.ascii "i\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-prologue-xmm.c)
	.byte	0x15	# DW_AT_decl_line
	.long	0x59	# DW_AT_type
	.uleb128 0x3	# DW_AT_location
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -196
	.uleb128 0x4	# (DIE (0x57) DW_TAG_unspecified_parameters)
	.byte	0	# end of children of DIE 0x2d
	.uleb128 0x5	# (DIE (0x59) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x6	# (DIE (0x60) DW_TAG_subprogram)
	.long	.LASF5	# DW_AT_name: "marker"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-prologue-xmm.c)
	.byte	0x1b	# DW_AT_decl_line
			# DW_AT_prototyped
	.quad	.LFB1	# DW_AT_low_pc
	.quad	.LFE1	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.uleb128 0x7	# (DIE (0x79) DW_TAG_subprogram)
			# DW_AT_external
	.long	.LASF6	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-prologue-xmm.c)
	.byte	0x20	# DW_AT_decl_line
			# DW_AT_prototyped
	.long	0x59	# DW_AT_type
	.quad	.LFB2	# DW_AT_low_pc
	.quad	.LFE2	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_tail_call_sites
	.uleb128 0x8	# (DIE (0x96) DW_TAG_variable)
	.ascii "v\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-prologue-xmm.c)
	.byte	0x12	# DW_AT_decl_line
	.long	0xa9	# DW_AT_type
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	v
	.uleb128 0x9	# (DIE (0xa9) DW_TAG_volatile_type)
	.long	0x59	# DW_AT_type
	.uleb128 0xa	# (DIE (0xae) DW_TAG_variable)
	.long	.LASF0	# DW_AT_name: "fail"
	.byte	0x1	# DW_AT_decl_file (gdb.arch/amd64-prologue-xmm.c)
	.byte	0x12	# DW_AT_decl_line
	.long	0xa9	# DW_AT_type
	.uleb128 0x9	# DW_AT_location
	.byte	0x3	# DW_OP_addr
	.quad	fail
	.byte	0	# end of children of DIE 0xb
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1	# (abbrev code)
	.uleb128 0x11	# (TAG: DW_TAG_compile_unit)
	.byte	0x1	# DW_children_yes
	.uleb128 0x25	# (DW_AT_producer)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x13	# (DW_AT_language)
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
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x3	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
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
	.uleb128 0x4	# (abbrev code)
	.uleb128 0x18	# (TAG: DW_TAG_unspecified_parameters)
	.byte	0	# DW_children_no
	.byte	0
	.byte	0
	.uleb128 0x5	# (abbrev code)
	.uleb128 0x24	# (TAG: DW_TAG_base_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3e	# (DW_AT_encoding)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.byte	0
	.byte	0
	.uleb128 0x6	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.byte	0
	.byte	0
	.uleb128 0x7	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0	# DW_children_no
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0x19	# (DW_FORM_flag_present)
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
	.byte	0
	.byte	0
	.uleb128 0x8	# (abbrev code)
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
	.uleb128 0x9	# (abbrev code)
	.uleb128 0x35	# (TAG: DW_TAG_volatile_type)
	.byte	0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0xa	# (abbrev code)
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
.LASF3:
	.string	""
.LASF0:
	.string	"fail"
.LASF4:
	.string	"func"
.LASF1:
	.string	"GNU C 4.6.1 20110715 (Red Hat 4.6.1-3)"
.LASF2:
	.string	"gdb.arch/amd64-prologue-xmm.c"
.LASF5:
	.string	"marker"
.LASF6:
	.string	"main"
	.ident	"GCC: (GNU) 4.6.1 20110715 (Red Hat 4.6.1-3)"
	.section	.note.GNU-stack,"",@progbits
