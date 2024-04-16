/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

   Contributed by Intel Corp. <markus.t.metzger@intel.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.


   This file has been generated using:
   gcc -S -dA -g record_goto.c -o x86_64-record_goto.S  */

	.file	"record_goto.c"
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.text
.Ltext0:
.globl fun1
	.type	fun1, @function
fun1:
.LFB0:
	.file 1 "record_goto.c"
	# record_goto.c:22
	.loc 1 22 0
	.cfi_startproc
	# basic block 2
	pushq	%rbp
	.cfi_def_cfa_offset 16
	movq	%rsp, %rbp
	.cfi_offset 6, -16
	.cfi_def_cfa_register 6
	# record_goto.c:23
	.loc 1 23 0
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	fun1, .-fun1
.globl fun2
	.type	fun2, @function
fun2:
.LFB1:
	# record_goto.c:27
	.loc 1 27 0
	.cfi_startproc
	# basic block 2
	pushq	%rbp
	.cfi_def_cfa_offset 16
	movq	%rsp, %rbp
	.cfi_offset 6, -16
	.cfi_def_cfa_register 6
	# record_goto.c:28
	.loc 1 28 0
	call	fun1
	# record_goto.c:29
	.loc 1 29 0
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	fun2, .-fun2
.globl fun3
	.type	fun3, @function
fun3:
.LFB2:
	# record_goto.c:33
	.loc 1 33 0
	.cfi_startproc
	# basic block 2
	pushq	%rbp
	.cfi_def_cfa_offset 16
	movq	%rsp, %rbp
	.cfi_offset 6, -16
	.cfi_def_cfa_register 6
	# record_goto.c:34
	.loc 1 34 0
	call	fun1
	# record_goto.c:35
	.loc 1 35 0
	call	fun2
	# record_goto.c:36
	.loc 1 36 0
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	fun3, .-fun3
.globl fun4
	.type	fun4, @function
fun4:
.LFB3:
	# record_goto.c:40
	.loc 1 40 0
	.cfi_startproc
	# basic block 2
	pushq	%rbp
	.cfi_def_cfa_offset 16
	movq	%rsp, %rbp
	.cfi_offset 6, -16
	.cfi_def_cfa_register 6
	# record_goto.c:41
	.loc 1 41 0
	call	fun1
	# record_goto.c:42
	.loc 1 42 0
	call	fun2
	# record_goto.c:43
	.loc 1 43 0
	call	fun3
	# record_goto.c:44
	.loc 1 44 0
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE3:
	.size	fun4, .-fun4
.globl main
	.type	main, @function
main:
.LFB4:
	# record_goto.c:48
	.loc 1 48 0
	.cfi_startproc
	# basic block 2
	pushq	%rbp
	.cfi_def_cfa_offset 16
	movq	%rsp, %rbp
	.cfi_offset 6, -16
	.cfi_def_cfa_register 6
	# record_goto.c:49
	.loc 1 49 0
	call	fun4
	# record_goto.c:50
	.loc 1 50 0
	movl	$0, %eax
	# record_goto.c:51
	.loc 1 51 0
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE4:
	.size	main, .-main
.Letext0:
	.section	.debug_info
	.long	0xbc	# Length of Compilation Unit Info
	.value	0x3	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF4	# DW_AT_producer: "GNU C 4.4.4 20100726 (Red Hat 4.4.4-13)"
	.byte	0x1	# DW_AT_language
	.long	.LASF5	# DW_AT_name: "record_goto.c"
	.long	.LASF6	# DW_AT_comp_dir: ""
	.quad	.Ltext0	# DW_AT_low_pc
	.quad	.Letext0	# DW_AT_high_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.uleb128 0x2	# (DIE (0x2d) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF0	# DW_AT_name: "fun1"
	.byte	0x1	# DW_AT_decl_file (record_goto.c)
	.byte	0x15	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.quad	.LFB0	# DW_AT_low_pc
	.quad	.LFE0	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.uleb128 0x2	# (DIE (0x48) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF1	# DW_AT_name: "fun2"
	.byte	0x1	# DW_AT_decl_file (record_goto.c)
	.byte	0x1a	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.quad	.LFB1	# DW_AT_low_pc
	.quad	.LFE1	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.uleb128 0x2	# (DIE (0x63) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF2	# DW_AT_name: "fun3"
	.byte	0x1	# DW_AT_decl_file (record_goto.c)
	.byte	0x20	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.quad	.LFB2	# DW_AT_low_pc
	.quad	.LFE2	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.uleb128 0x2	# (DIE (0x7e) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF3	# DW_AT_name: "fun4"
	.byte	0x1	# DW_AT_decl_file (record_goto.c)
	.byte	0x27	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.quad	.LFB3	# DW_AT_low_pc
	.quad	.LFE3	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.uleb128 0x3	# (DIE (0x99) DW_TAG_subprogram)
	.byte	0x1	# DW_AT_external
	.long	.LASF7	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (record_goto.c)
	.byte	0x2f	# DW_AT_decl_line
	.byte	0x1	# DW_AT_prototyped
	.long	0xb8	# DW_AT_type
	.quad	.LFB4	# DW_AT_low_pc
	.quad	.LFE4	# DW_AT_high_pc
	.byte	0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
	.uleb128 0x4	# (DIE (0xb8) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.byte	0x0	# end of children of DIE 0xb
	.section	.debug_abbrev
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
	.uleb128 0x6	# (DW_FORM_data4)
	.byte	0x0
	.byte	0x0
	.uleb128 0x2	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x0	# DW_children_no
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0x0
	.byte	0x0
	.uleb128 0x3	# (abbrev code)
	.uleb128 0x2e	# (TAG: DW_TAG_subprogram)
	.byte	0x0	# DW_children_no
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0xc	# (DW_FORM_flag)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0xa	# (DW_FORM_block1)
	.byte	0x0
	.byte	0x0
	.uleb128 0x4	# (abbrev code)
	.uleb128 0x24	# (TAG: DW_TAG_base_type)
	.byte	0x0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3e	# (DW_AT_encoding)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.byte	0x0
	.byte	0x0
	.byte	0x0
	.section	.debug_pubnames,"",@progbits
	.long	0x3b	# Length of Public Names Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.long	0xc0	# Compilation Unit Length
	.long	0x2d	# DIE offset
	.ascii "fun1\0"	# external name
	.long	0x48	# DIE offset
	.ascii "fun2\0"	# external name
	.long	0x63	# DIE offset
	.ascii "fun3\0"	# external name
	.long	0x7e	# DIE offset
	.ascii "fun4\0"	# external name
	.long	0x99	# DIE offset
	.ascii "main\0"	# external name
	.long	0x0
	.section	.debug_aranges,"",@progbits
	.long	0x2c	# Length of Address Ranges Info
	.value	0x2	# DWARF Version
	.long	.Ldebug_info0	# Offset of Compilation Unit Info
	.byte	0x8	# Size of Address
	.byte	0x0	# Size of Segment Descriptor
	.value	0x0	# Pad to 16 byte boundary
	.value	0x0
	.quad	.Ltext0	# Address
	.quad	.Letext0-.Ltext0	# Length
	.quad	0x0
	.quad	0x0
	.section	.debug_str,"MS",@progbits,1
.LASF3:
	.string	"fun4"
.LASF5:
	.string	"record_goto.c"
.LASF4:
	.string	"GNU C 4.4.4 20100726 (Red Hat 4.4.4-13)"
.LASF7:
	.string	"main"
.LASF1:
	.string	"fun2"
.LASF0:
	.string	"fun1"
.LASF6:
	.string	""
.LASF2:
	.string	"fun3"
	.ident	"GCC: (GNU) 4.4.4 20100726 (Red Hat 4.4.4-13)"
	.section	.note.GNU-stack,"",@progbits
