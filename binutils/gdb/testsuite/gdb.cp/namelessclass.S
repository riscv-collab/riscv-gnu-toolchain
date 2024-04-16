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

/* This file was generated using:

   $ clang++ -g namelessclass.cc -std=c++11 -S -o namelessclass.S

  with

  $ clang++ -v
  clang version 3.3 (tags/RELEASE_33/final)
  Target: x86_64-redhat-linux-gnu
  Thread model: posix

  This is a test for c++/16597.  */

	.file	"namelessclass.cc"
	.section	.debug_info,"",@progbits
.Lsection_info:
	.section	.debug_abbrev,"",@progbits
.Lsection_abbrev:
	.section	.debug_aranges,"",@progbits
	.section	.debug_macinfo,"",@progbits
	.section	.debug_line,"",@progbits
.Lsection_line:
	.section	.debug_loc,"",@progbits
	.section	.debug_pubtypes,"",@progbits
	.section	.debug_str,"MS",@progbits,1
.Linfo_string:
	.section	.debug_ranges,"",@progbits
.Ldebug_range:
	.section	.debug_loc,"",@progbits
.Lsection_debug_loc:
	.text
.Ltext_begin:
	.data
	.file	1 "namelessclass.cc"
	.text
	.globl	main
	.align	16, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
.Lfunc_begin0:
	.loc	1 42 0                  # namelessclass.cc:42:0
# BB#0:
	pushq	%rbp
.Ltmp2:
	.cfi_def_cfa_offset 16
.Ltmp3:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
.Ltmp4:
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	leaq	-8(%rbp), %rdi
	movl	$0, -4(%rbp)
.Ltmp5:
	#DEBUG_VALUE: main:a <- RDI+0
	.loc	1 43 0 prologue_end     # namelessclass.cc:43:0
	callq	_ZN1AC1Ev
	leaq	-8(%rbp), %rdi
.Ltmp6:
	.loc	1 45 0                  # namelessclass.cc:45:0
	callq	_ZN1A4doitEv
	addq	$16, %rsp
	popq	%rbp
	ret
.Ltmp7:
.Ltmp8:
	.size	main, .Ltmp8-main
.Lfunc_end0:
	.cfi_endproc

	.section	.text._ZN1AC1Ev,"axG",@progbits,_ZN1AC1Ev,comdat
	.weak	_ZN1AC1Ev
	.align	16, 0x90
	.type	_ZN1AC1Ev,@function
_ZN1AC1Ev:                              # @_ZN1AC1Ev
	.cfi_startproc
.Lfunc_begin1:
	.loc	1 25 0                  # namelessclass.cc:25:0
# BB#0:
	pushq	%rbp
.Ltmp11:
	.cfi_def_cfa_offset 16
.Ltmp12:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
.Ltmp13:
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	.loc	1 25 0 prologue_end     # namelessclass.cc:25:0
.Ltmp14:
	callq	_ZN1AC2Ev
	addq	$16, %rsp
	popq	%rbp
	ret
.Ltmp15:
.Ltmp16:
	.size	_ZN1AC1Ev, .Ltmp16-_ZN1AC1Ev
.Lfunc_end1:
	.cfi_endproc

	.section	.text._ZN1A4doitEv,"axG",@progbits,_ZN1A4doitEv,comdat
	.weak	_ZN1A4doitEv
	.align	16, 0x90
	.type	_ZN1A4doitEv,@function
_ZN1A4doitEv:                           # @_ZN1A4doitEv
	.cfi_startproc
.Lfunc_begin2:
	.loc	1 26 0                  # namelessclass.cc:26:0
# BB#0:
	pushq	%rbp
.Ltmp19:
	.cfi_def_cfa_offset 16
.Ltmp20:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
.Ltmp21:
	.cfi_def_cfa_register %rbp
	subq	$32, %rsp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	.loc	1 27 0 prologue_end     # namelessclass.cc:27:0
.Ltmp22:
	movq	%rdi, -24(%rbp)
	movq	-24(%rbp), %rsi
	callq	_ZN1A5fudgeIZNS_4doitEvEUlvE_EEiT_
	movl	%eax, -12(%rbp)
	.loc	1 31 0                  # namelessclass.cc:31:0
	movl	-12(%rbp), %eax
	addq	$32, %rsp
	popq	%rbp
	ret
.Ltmp23:
.Ltmp24:
	.size	_ZN1A4doitEv, .Ltmp24-_ZN1A4doitEv
.Lfunc_end2:
	.cfi_endproc

	.section	.text._ZN1A5fudgeIZNS_4doitEvEUlvE_EEiT_,"axG",@progbits,_ZN1A5fudgeIZNS_4doitEvEUlvE_EEiT_,comdat
	.weak	_ZN1A5fudgeIZNS_4doitEvEUlvE_EEiT_
	.align	16, 0x90
	.type	_ZN1A5fudgeIZNS_4doitEvEUlvE_EEiT_,@function
_ZN1A5fudgeIZNS_4doitEvEUlvE_EEiT_:     # @_ZN1A5fudgeIZNS_4doitEvEUlvE_EEiT_
	.cfi_startproc
.Lfunc_begin3:
	.loc	1 36 0                  # namelessclass.cc:36:0
# BB#0:
	pushq	%rbp
.Ltmp27:
	.cfi_def_cfa_offset 16
.Ltmp28:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
.Ltmp29:
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	leaq	-16(%rbp), %rax
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	#DEBUG_VALUE: fudge<<lambda at namelessclass.cc:27:22> >:func <- RAX+0
	.loc	1 36 34 prologue_end    # namelessclass.cc:36:34
.Ltmp30:
	movq	%rax, %rdi
	callq	_ZZN1A4doitEvENKUlvE_clEv
.Ltmp31:
	addq	$16, %rsp
	popq	%rbp
	ret
.Ltmp32:
.Ltmp33:
	.size	_ZN1A5fudgeIZNS_4doitEvEUlvE_EEiT_, .Ltmp33-_ZN1A5fudgeIZNS_4doitEvEUlvE_EEiT_
.Lfunc_end3:
	.cfi_endproc

	.section	.text._ZZN1A4doitEvENKUlvE_clEv,"axG",@progbits,_ZZN1A4doitEvENKUlvE_clEv,comdat
	.weak	_ZZN1A4doitEvENKUlvE_clEv
	.align	16, 0x90
	.type	_ZZN1A4doitEvENKUlvE_clEv,@function
_ZZN1A4doitEvENKUlvE_clEv:              # @_ZZN1A4doitEvENKUlvE_clEv
	.cfi_startproc
.Lfunc_begin4:
	.loc	1 27 0                  # namelessclass.cc:27:0
# BB#0:
	pushq	%rbp
.Ltmp36:
	.cfi_def_cfa_offset 16
.Ltmp37:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
.Ltmp38:
	.cfi_def_cfa_register %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	movq	(%rdi), %rdi
	.loc	1 28 0 prologue_end     # namelessclass.cc:28:0
.Ltmp39:
	movl	(%rdi), %eax
	popq	%rbp
	ret
.Ltmp40:
.Ltmp41:
	.size	_ZZN1A4doitEvENKUlvE_clEv, .Ltmp41-_ZZN1A4doitEvENKUlvE_clEv
.Lfunc_end4:
	.cfi_endproc

	.section	.text._ZN1AC2Ev,"axG",@progbits,_ZN1AC2Ev,comdat
	.weak	_ZN1AC2Ev
	.align	16, 0x90
	.type	_ZN1AC2Ev,@function
_ZN1AC2Ev:                              # @_ZN1AC2Ev
	.cfi_startproc
.Lfunc_begin5:
	.loc	1 25 0                  # namelessclass.cc:25:0
# BB#0:
	pushq	%rbp
.Ltmp44:
	.cfi_def_cfa_offset 16
.Ltmp45:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
.Ltmp46:
	.cfi_def_cfa_register %rbp
	movq	%rdi, -8(%rbp)
	movq	-8(%rbp), %rdi
	.loc	1 25 0 prologue_end     # namelessclass.cc:25:0
.Ltmp47:
	movl	$48879, (%rdi)          # imm = 0xBEEF
	popq	%rbp
	ret
.Ltmp48:
.Ltmp49:
	.size	_ZN1AC2Ev, .Ltmp49-_ZN1AC2Ev
.Lfunc_end5:
	.cfi_endproc

	.text
.Ltext_end:
	.data
.Ldata_end:
	.text
.Lsection_end1:
	.section	.debug_info,"",@progbits
.L.debug_info_begin0:
	.long	531                     # Length of Compilation Unit Info
	.short	2                       # DWARF version number
	.long	.L.debug_abbrev_begin   # Offset Into Abbrev. Section
	.byte	8                       # Address Size (in bytes)
	.byte	1                       # Abbrev [1] 0xb:0x20c DW_TAG_compile_unit
	.long	.Linfo_string0          # DW_AT_producer
	.short	4                       # DW_AT_language
	.long	.Linfo_string1          # DW_AT_name
	.quad	0                       # DW_AT_low_pc
	.long	.Lsection_line          # DW_AT_stmt_list
	.long	.Linfo_string2          # DW_AT_comp_dir
	.byte	2                       # Abbrev [2] 0x26:0x2c DW_TAG_subprogram
	.long	.Linfo_string3          # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	41                      # DW_AT_decl_line
	.long	82                      # DW_AT_type
                                        # DW_AT_external
	.quad	.Lfunc_begin0           # DW_AT_low_pc
	.quad	.Lfunc_end0             # DW_AT_high_pc
	.byte	1                       # DW_AT_frame_base
	.byte	86
	.byte	3                       # Abbrev [3] 0x43:0xe DW_TAG_variable
	.long	.Linfo_string15         # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	43                      # DW_AT_decl_line
	.long	186                     # DW_AT_type
	.byte	2                       # DW_AT_location
	.byte	145
	.byte	120
	.byte	0                       # End Of Children Mark
	.byte	4                       # Abbrev [4] 0x52:0x7 DW_TAG_base_type
	.long	.Linfo_string4          # DW_AT_name
	.byte	5                       # DW_AT_encoding
	.byte	4                       # DW_AT_byte_size
	.byte	5                       # Abbrev [5] 0x59:0x5 DW_TAG_pointer_type
	.long	186                     # DW_AT_type
	.byte	5                       # Abbrev [5] 0x5e:0x5 DW_TAG_pointer_type
	.long	186                     # DW_AT_type
	.byte	6                       # Abbrev [6] 0x63:0x5 DW_TAG_const_type
	.long	119                     # DW_AT_type
	.byte	5                       # Abbrev [5] 0x68:0x5 DW_TAG_pointer_type
	.long	99                      # DW_AT_type
	.byte	5                       # Abbrev [5] 0x6d:0x5 DW_TAG_pointer_type
	.long	119                     # DW_AT_type
	.byte	7                       # Abbrev [7] 0x72:0x5 DW_TAG_rvalue_reference_type
	.long	119                     # DW_AT_type
	.byte	8                       # Abbrev [8] 0x77:0x43 DW_TAG_class_type
	.byte	8                       # DW_AT_byte_size
	.byte	1                       # DW_AT_decl_file
	.byte	27                      # DW_AT_decl_line
	.byte	9                       # Abbrev [9] 0x7b:0xf DW_TAG_member
	.long	.Linfo_string7          # DW_AT_name
	.long	94                      # DW_AT_type
	.byte	1                       # DW_AT_decl_file
	.byte	27                      # DW_AT_decl_line
	.byte	2                       # DW_AT_data_member_location
	.byte	35
	.byte	0
	.byte	3                       # DW_AT_accessibility
                                        # DW_ACCESS_private
	.byte	10                      # Abbrev [10] 0x8a:0x12 DW_TAG_subprogram
	.long	.Linfo_string8          # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	27                      # DW_AT_decl_line
	.long	82                      # DW_AT_type
                                        # DW_AT_declaration
                                        # DW_AT_external
	.byte	1                       # DW_AT_accessibility
                                        # DW_ACCESS_public
	.byte	11                      # Abbrev [11] 0x96:0x5 DW_TAG_formal_parameter
	.long	104                     # DW_AT_type
                                        # DW_AT_artificial
	.byte	0                       # End Of Children Mark
	.byte	12                      # Abbrev [12] 0x9c:0xe DW_TAG_subprogram
	.long	.Linfo_string9          # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	27                      # DW_AT_decl_line
                                        # DW_AT_declaration
                                        # DW_AT_artificial
                                        # DW_AT_external
	.byte	1                       # DW_AT_accessibility
                                        # DW_ACCESS_public
	.byte	11                      # Abbrev [11] 0xa4:0x5 DW_TAG_formal_parameter
	.long	109                     # DW_AT_type
                                        # DW_AT_artificial
	.byte	0                       # End Of Children Mark
	.byte	13                      # Abbrev [13] 0xaa:0xf DW_TAG_subprogram
	.byte	1                       # DW_AT_decl_file
	.byte	27                      # DW_AT_decl_line
                                        # DW_AT_declaration
                                        # DW_AT_artificial
                                        # DW_AT_external
	.byte	1                       # DW_AT_accessibility
                                        # DW_ACCESS_public
	.byte	11                      # Abbrev [11] 0xae:0x5 DW_TAG_formal_parameter
	.long	109                     # DW_AT_type
                                        # DW_AT_artificial
	.byte	14                      # Abbrev [14] 0xb3:0x5 DW_TAG_formal_parameter
	.long	114                     # DW_AT_type
	.byte	0                       # End Of Children Mark
	.byte	0                       # End Of Children Mark
	.byte	15                      # Abbrev [15] 0xba:0x60 DW_TAG_class_type
	.long	.Linfo_string6          # DW_AT_name
	.byte	4                       # DW_AT_byte_size
	.byte	1                       # DW_AT_decl_file
	.byte	22                      # DW_AT_decl_line
	.byte	9                       # Abbrev [9] 0xc2:0xf DW_TAG_member
	.long	.Linfo_string5          # DW_AT_name
	.long	82                      # DW_AT_type
	.byte	1                       # DW_AT_decl_file
	.byte	37                      # DW_AT_decl_line
	.byte	2                       # DW_AT_data_member_location
	.byte	35
	.byte	0
	.byte	3                       # DW_AT_accessibility
                                        # DW_ACCESS_private
	.byte	16                      # Abbrev [16] 0xd1:0xe DW_TAG_subprogram
	.long	.Linfo_string6          # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	25                      # DW_AT_decl_line
                                        # DW_AT_declaration
                                        # DW_AT_external
	.byte	1                       # DW_AT_accessibility
                                        # DW_ACCESS_public
	.byte	11                      # Abbrev [11] 0xd9:0x5 DW_TAG_formal_parameter
	.long	89                      # DW_AT_type
                                        # DW_AT_artificial
	.byte	0                       # End Of Children Mark
	.byte	17                      # Abbrev [17] 0xdf:0x16 DW_TAG_subprogram
	.byte	1                       # DW_AT_accessibility
                                        # DW_ACCESS_public
	.long	.Linfo_string13         # DW_AT_MIPS_linkage_name
	.long	.Linfo_string14         # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	26                      # DW_AT_decl_line
	.long	82                      # DW_AT_type
                                        # DW_AT_declaration
                                        # DW_AT_external
	.byte	11                      # Abbrev [11] 0xef:0x5 DW_TAG_formal_parameter
	.long	89                      # DW_AT_type
                                        # DW_AT_artificial
	.byte	0                       # End Of Children Mark
	.byte	18                      # Abbrev [18] 0xf5:0x24 DW_TAG_subprogram
	.long	.Linfo_string11         # DW_AT_MIPS_linkage_name
	.long	.Linfo_string12         # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	36                      # DW_AT_decl_line
	.long	82                      # DW_AT_type
                                        # DW_AT_declaration
                                        # DW_AT_external
	.byte	3                       # DW_AT_accessibility
                                        # DW_ACCESS_private
	.byte	19                      # Abbrev [19] 0x105:0x9 DW_TAG_template_type_parameter
	.long	119                     # DW_AT_type
	.long	.Linfo_string10         # DW_AT_name
	.byte	11                      # Abbrev [11] 0x10e:0x5 DW_TAG_formal_parameter
	.long	89                      # DW_AT_type
                                        # DW_AT_artificial
	.byte	14                      # Abbrev [14] 0x113:0x5 DW_TAG_formal_parameter
	.long	119                     # DW_AT_type
	.byte	0                       # End Of Children Mark
	.byte	0                       # End Of Children Mark
	.byte	20                      # Abbrev [20] 0x11a:0x62 DW_TAG_subprogram
	.long	223                     # DW_AT_specification
	.quad	.Lfunc_begin2           # DW_AT_low_pc
	.quad	.Lfunc_end2             # DW_AT_high_pc
	.byte	1                       # DW_AT_frame_base
	.byte	86
	.long	351                     # DW_AT_object_pointer
	.byte	20                      # Abbrev [20] 0x135:0x2a DW_TAG_subprogram
	.long	138                     # DW_AT_specification
	.quad	.Lfunc_begin4           # DW_AT_low_pc
	.quad	.Lfunc_end4             # DW_AT_high_pc
	.byte	1                       # DW_AT_frame_base
	.byte	86
	.long	336                     # DW_AT_object_pointer
	.byte	21                      # Abbrev [21] 0x150:0xe DW_TAG_formal_parameter
	.long	.Linfo_string7          # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	27                      # DW_AT_decl_line
	.long	529                     # DW_AT_type
                                        # DW_AT_artificial
	.byte	2                       # DW_AT_location
	.byte	145
	.byte	120
	.byte	0                       # End Of Children Mark
	.byte	21                      # Abbrev [21] 0x15f:0xe DW_TAG_formal_parameter
	.long	.Linfo_string7          # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	26                      # DW_AT_decl_line
	.long	94                      # DW_AT_type
                                        # DW_AT_artificial
	.byte	2                       # DW_AT_location
	.byte	145
	.byte	120
	.byte	3                       # Abbrev [3] 0x16d:0xe DW_TAG_variable
	.long	.Linfo_string16         # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	27                      # DW_AT_decl_line
	.long	82                      # DW_AT_type
	.byte	2                       # DW_AT_location
	.byte	145
	.byte	116
	.byte	0                       # End Of Children Mark
	.byte	20                      # Abbrev [20] 0x17c:0x41 DW_TAG_subprogram
	.long	245                     # DW_AT_specification
	.quad	.Lfunc_begin3           # DW_AT_low_pc
	.quad	.Lfunc_end3             # DW_AT_high_pc
	.byte	1                       # DW_AT_frame_base
	.byte	86
	.long	416                     # DW_AT_object_pointer
	.byte	19                      # Abbrev [19] 0x197:0x9 DW_TAG_template_type_parameter
	.long	119                     # DW_AT_type
	.long	.Linfo_string10         # DW_AT_name
	.byte	21                      # Abbrev [21] 0x1a0:0xe DW_TAG_formal_parameter
	.long	.Linfo_string7          # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	36                      # DW_AT_decl_line
	.long	94                      # DW_AT_type
                                        # DW_AT_artificial
	.byte	2                       # DW_AT_location
	.byte	145
	.byte	120
	.byte	22                      # Abbrev [22] 0x1ae:0xe DW_TAG_formal_parameter
	.long	.Linfo_string17         # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	36                      # DW_AT_decl_line
	.long	119                     # DW_AT_type
	.byte	2                       # DW_AT_location
	.byte	145
	.byte	112
	.byte	0                       # End Of Children Mark
	.byte	20                      # Abbrev [20] 0x1bd:0x2a DW_TAG_subprogram
	.long	209                     # DW_AT_specification
	.quad	.Lfunc_begin1           # DW_AT_low_pc
	.quad	.Lfunc_end1             # DW_AT_high_pc
	.byte	1                       # DW_AT_frame_base
	.byte	86
	.long	472                     # DW_AT_object_pointer
	.byte	21                      # Abbrev [21] 0x1d8:0xe DW_TAG_formal_parameter
	.long	.Linfo_string7          # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	25                      # DW_AT_decl_line
	.long	94                      # DW_AT_type
                                        # DW_AT_artificial
	.byte	2                       # DW_AT_location
	.byte	145
	.byte	120
	.byte	0                       # End Of Children Mark
	.byte	20                      # Abbrev [20] 0x1e7:0x2a DW_TAG_subprogram
	.long	209                     # DW_AT_specification
	.quad	.Lfunc_begin5           # DW_AT_low_pc
	.quad	.Lfunc_end5             # DW_AT_high_pc
	.byte	1                       # DW_AT_frame_base
	.byte	86
	.long	514                     # DW_AT_object_pointer
	.byte	21                      # Abbrev [21] 0x202:0xe DW_TAG_formal_parameter
	.long	.Linfo_string7          # DW_AT_name
	.byte	1                       # DW_AT_decl_file
	.byte	25                      # DW_AT_decl_line
	.long	94                      # DW_AT_type
                                        # DW_AT_artificial
	.byte	2                       # DW_AT_location
	.byte	145
	.byte	120
	.byte	0                       # End Of Children Mark
	.byte	5                       # Abbrev [5] 0x211:0x5 DW_TAG_pointer_type
	.long	99                      # DW_AT_type
	.byte	0                       # End Of Children Mark
.L.debug_info_end0:
	.section	.debug_abbrev,"",@progbits
.L.debug_abbrev_begin:
	.byte	1                       # Abbreviation Code
	.byte	17                      # DW_TAG_compile_unit
	.byte	1                       # DW_CHILDREN_yes
	.byte	37                      # DW_AT_producer
	.byte	14                      # DW_FORM_strp
	.byte	19                      # DW_AT_language
	.byte	5                       # DW_FORM_data2
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	17                      # DW_AT_low_pc
	.byte	1                       # DW_FORM_addr
	.byte	16                      # DW_AT_stmt_list
	.byte	6                       # DW_FORM_data4
	.byte	27                      # DW_AT_comp_dir
	.byte	14                      # DW_FORM_strp
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	2                       # Abbreviation Code
	.byte	46                      # DW_TAG_subprogram
	.byte	1                       # DW_CHILDREN_yes
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	63                      # DW_AT_external
	.byte	25                      # DW_FORM_flag_present
	.byte	17                      # DW_AT_low_pc
	.byte	1                       # DW_FORM_addr
	.byte	18                      # DW_AT_high_pc
	.byte	1                       # DW_FORM_addr
	.byte	64                      # DW_AT_frame_base
	.byte	10                      # DW_FORM_block1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	3                       # Abbreviation Code
	.byte	52                      # DW_TAG_variable
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	2                       # DW_AT_location
	.byte	10                      # DW_FORM_block1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	4                       # Abbreviation Code
	.byte	36                      # DW_TAG_base_type
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	62                      # DW_AT_encoding
	.byte	11                      # DW_FORM_data1
	.byte	11                      # DW_AT_byte_size
	.byte	11                      # DW_FORM_data1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	5                       # Abbreviation Code
	.byte	15                      # DW_TAG_pointer_type
	.byte	0                       # DW_CHILDREN_no
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	6                       # Abbreviation Code
	.byte	38                      # DW_TAG_const_type
	.byte	0                       # DW_CHILDREN_no
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	7                       # Abbreviation Code
	.byte	66                      # DW_TAG_rvalue_reference_type
	.byte	0                       # DW_CHILDREN_no
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	8                       # Abbreviation Code
	.byte	2                       # DW_TAG_class_type
	.byte	1                       # DW_CHILDREN_yes
	.byte	11                      # DW_AT_byte_size
	.byte	11                      # DW_FORM_data1
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	9                       # Abbreviation Code
	.byte	13                      # DW_TAG_member
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	56                      # DW_AT_data_member_location
	.byte	10                      # DW_FORM_block1
	.byte	50                      # DW_AT_accessibility
	.byte	11                      # DW_FORM_data1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	10                      # Abbreviation Code
	.byte	46                      # DW_TAG_subprogram
	.byte	1                       # DW_CHILDREN_yes
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	60                      # DW_AT_declaration
	.byte	25                      # DW_FORM_flag_present
	.byte	63                      # DW_AT_external
	.byte	25                      # DW_FORM_flag_present
	.byte	50                      # DW_AT_accessibility
	.byte	11                      # DW_FORM_data1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	11                      # Abbreviation Code
	.byte	5                       # DW_TAG_formal_parameter
	.byte	0                       # DW_CHILDREN_no
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	52                      # DW_AT_artificial
	.byte	25                      # DW_FORM_flag_present
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	12                      # Abbreviation Code
	.byte	46                      # DW_TAG_subprogram
	.byte	1                       # DW_CHILDREN_yes
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	60                      # DW_AT_declaration
	.byte	25                      # DW_FORM_flag_present
	.byte	52                      # DW_AT_artificial
	.byte	25                      # DW_FORM_flag_present
	.byte	63                      # DW_AT_external
	.byte	25                      # DW_FORM_flag_present
	.byte	50                      # DW_AT_accessibility
	.byte	11                      # DW_FORM_data1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	13                      # Abbreviation Code
	.byte	46                      # DW_TAG_subprogram
	.byte	1                       # DW_CHILDREN_yes
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	60                      # DW_AT_declaration
	.byte	25                      # DW_FORM_flag_present
	.byte	52                      # DW_AT_artificial
	.byte	25                      # DW_FORM_flag_present
	.byte	63                      # DW_AT_external
	.byte	25                      # DW_FORM_flag_present
	.byte	50                      # DW_AT_accessibility
	.byte	11                      # DW_FORM_data1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	14                      # Abbreviation Code
	.byte	5                       # DW_TAG_formal_parameter
	.byte	0                       # DW_CHILDREN_no
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	15                      # Abbreviation Code
	.byte	2                       # DW_TAG_class_type
	.byte	1                       # DW_CHILDREN_yes
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	11                      # DW_AT_byte_size
	.byte	11                      # DW_FORM_data1
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	16                      # Abbreviation Code
	.byte	46                      # DW_TAG_subprogram
	.byte	1                       # DW_CHILDREN_yes
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	60                      # DW_AT_declaration
	.byte	25                      # DW_FORM_flag_present
	.byte	63                      # DW_AT_external
	.byte	25                      # DW_FORM_flag_present
	.byte	50                      # DW_AT_accessibility
	.byte	11                      # DW_FORM_data1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	17                      # Abbreviation Code
	.byte	46                      # DW_TAG_subprogram
	.byte	1                       # DW_CHILDREN_yes
	.byte	50                      # DW_AT_accessibility
	.byte	11                      # DW_FORM_data1
	.ascii	 "\207@"                # DW_AT_MIPS_linkage_name
	.byte	14                      # DW_FORM_strp
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	60                      # DW_AT_declaration
	.byte	25                      # DW_FORM_flag_present
	.byte	63                      # DW_AT_external
	.byte	25                      # DW_FORM_flag_present
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	18                      # Abbreviation Code
	.byte	46                      # DW_TAG_subprogram
	.byte	1                       # DW_CHILDREN_yes
	.ascii	 "\207@"                # DW_AT_MIPS_linkage_name
	.byte	14                      # DW_FORM_strp
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	60                      # DW_AT_declaration
	.byte	25                      # DW_FORM_flag_present
	.byte	63                      # DW_AT_external
	.byte	25                      # DW_FORM_flag_present
	.byte	50                      # DW_AT_accessibility
	.byte	11                      # DW_FORM_data1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	19                      # Abbreviation Code
	.byte	47                      # DW_TAG_template_type_parameter
	.byte	0                       # DW_CHILDREN_no
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	20                      # Abbreviation Code
	.byte	46                      # DW_TAG_subprogram
	.byte	1                       # DW_CHILDREN_yes
	.byte	71                      # DW_AT_specification
	.byte	19                      # DW_FORM_ref4
	.byte	17                      # DW_AT_low_pc
	.byte	1                       # DW_FORM_addr
	.byte	18                      # DW_AT_high_pc
	.byte	1                       # DW_FORM_addr
	.byte	64                      # DW_AT_frame_base
	.byte	10                      # DW_FORM_block1
	.byte	100                     # DW_AT_object_pointer
	.byte	19                      # DW_FORM_ref4
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	21                      # Abbreviation Code
	.byte	5                       # DW_TAG_formal_parameter
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	52                      # DW_AT_artificial
	.byte	25                      # DW_FORM_flag_present
	.byte	2                       # DW_AT_location
	.byte	10                      # DW_FORM_block1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	22                      # Abbreviation Code
	.byte	5                       # DW_TAG_formal_parameter
	.byte	0                       # DW_CHILDREN_no
	.byte	3                       # DW_AT_name
	.byte	14                      # DW_FORM_strp
	.byte	58                      # DW_AT_decl_file
	.byte	11                      # DW_FORM_data1
	.byte	59                      # DW_AT_decl_line
	.byte	11                      # DW_FORM_data1
	.byte	73                      # DW_AT_type
	.byte	19                      # DW_FORM_ref4
	.byte	2                       # DW_AT_location
	.byte	10                      # DW_FORM_block1
	.byte	0                       # EOM(1)
	.byte	0                       # EOM(2)
	.byte	0                       # EOM(3)
.L.debug_abbrev_end:
	.section	.debug_aranges,"",@progbits
	.section	.debug_ranges,"",@progbits
	.section	.debug_macinfo,"",@progbits
	.section	.debug_str,"MS",@progbits,1
.Linfo_string0:
	.asciz	 "clang version 3.3 (tags/RELEASE_33/final)"
.Linfo_string1:
	.asciz	 "namelessclass.cc"
.Linfo_string2:
	.asciz	 "/tmp"
.Linfo_string3:
	.asciz	 "main"
.Linfo_string4:
	.asciz	 "int"
.Linfo_string5:
	.asciz	 "a_"
.Linfo_string6:
	.asciz	 "A"
.Linfo_string7:
	.asciz	 "this"
.Linfo_string8:
	.asciz	 "operator()"
.Linfo_string9:
	.asciz	 "~"
.Linfo_string10:
	.asciz	 "Func"
.Linfo_string11:
	.asciz	 "_ZN1A5fudgeIZNS_4doitEvEUlvE_EEiT_"
.Linfo_string12:
	.asciz	 "fudge<<lambda at namelessclass.cc:27:22> >"
.Linfo_string13:
	.asciz	 "_ZN1A4doitEv"
.Linfo_string14:
	.asciz	 "doit"
.Linfo_string15:
	.asciz	 "a"
.Linfo_string16:
	.asciz	 "ret"
.Linfo_string17:
	.asciz	 "func"

	.section	".note.GNU-stack","",@progbits
