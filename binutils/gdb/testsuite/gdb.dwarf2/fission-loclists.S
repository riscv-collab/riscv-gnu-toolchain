/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

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

   This file was created by doing:
   gcc -O2 -gdwarf-4 -gsplit-debug -S -dA fission-loclists.c
   and then massaging the output.

   fission-loclists.c:

   volatile v;

   __attribute__ ((noinline)) void foo (int* x) { v = *x; }

   int
   main (int argc, char *argv[])
   {
     foo (&argc);
     return 0;
   }
*/

	.file	"fission-loclists.c"
	.text
.Ltext0:
	.p2align 4,,15
	.globl	foo
	.type	foo, @function
foo:
.LFB0:
	.file 1 "fission-loclists.c"
	# fission-loclists.c:3
	.loc 1 3 0
	.cfi_startproc
.LVL0:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	# fission-loclists.c:3
	.loc 1 3 0
	movl	(%rdi), %eax
	movl	%eax, v(%rip)
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE0:
	.size	foo, .-foo
	.text # was .section .text.startup,"ax",@progbits
	.p2align 4,,15
	.globl	main
	.type	main, @function
main:
.LFB1:
	# fission-loclists.c:7
	.loc 1 7 0
	.cfi_startproc
.LVL1:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (fallthru)
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	# fission-loclists.c:7
	.loc 1 7 0
	movl	%edi, 4(%rsp)
	# fission-loclists.c:8
	.loc 1 8 0
	leaq	4(%rsp), %rdi
.LVL2:
	call	foo
.LVL3:
	# fission-loclists.c:10
	.loc 1 10 0
	xorl	%eax, %eax
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE1:
	.size	main, .-main
	.comm	v,4,4
	.text
.Letext0:
	.section	.debug_addr,"",@progbits
.Ldebug_addr0:
	.8byte	.LVL1	# 0x0
	.8byte	v	# 0x1
	.8byte	.LVL3	# 0x2
	.8byte	.LFB0	# 0x3
	.8byte	.LFB1	# 0x4
	.8byte	.LVL3-1	# 0x5
	.8byte	.LVL2	# 0x6
	.section	.debug_info.dwo,"",@progbits
.Ldebug_info0:
	.4byte	.Ldebug_info0_end - .Ldebug_info0 - 4 # Length of Compilation Unit Info
	.value	0x4	# DWARF version number
	.4byte	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.ascii "GNU C 4.7.x-fission\0"	# DW_AT_producer
	.byte	0x1	# DW_AT_language
	.ascii "fission-loclists.c\0"	# DW_AT_name
	.ascii "/tmp\0"	# DW_AT_comp_dir
	.byte	0xcb	# DW_AT_GNU_dwo_id
	.byte	0x64
	.byte	0x3e
	.byte	0x34
	.byte	0x1b
	.byte	0x8a
	.byte	0xd6
	.byte	0x5
.Lfoo_func:
	.uleb128 0x2	# (DIE (0x5c) DW_TAG_subprogram)
			# DW_AT_external
	.ascii "foo\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (fission-loclists.c)
	.byte	0x3	# DW_AT_decl_line
			# DW_AT_prototyped
	.uleb128 0x3	# DW_AT_low_pc
	.8byte	.LFE0-.LFB0	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.4byte	.Lint_ptr_type - .Ldebug_info0 # DW_AT_sibling
	.uleb128 0x3	# (DIE (0x72) DW_TAG_formal_parameter)
	.ascii "x\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (fission-loclists.c)
	.byte	0x3	# DW_AT_decl_line
	.4byte	.Lint_ptr_type - .Ldebug_info0 # DW_AT_type
	.uleb128 0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0	# end of children of DIE 0x5c
.Lint_ptr_type:
	.uleb128 0x4	# (DIE (0x7e) DW_TAG_pointer_type)
	.byte	0x8	# DW_AT_byte_size
	.4byte	.Lint_type - .Ldebug_info0 # DW_AT_type
.Lint_type:
	.uleb128 0x5	# (DIE (0x84) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x6	# (DIE (0x8b) DW_TAG_subprogram)
			# DW_AT_external
	.ascii "main\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (fission-loclists.c)
	.byte	0x6	# DW_AT_decl_line
			# DW_AT_prototyped
	.4byte	.Lint_type - .Ldebug_info0 # DW_AT_type
	.uleb128 0x4	# DW_AT_low_pc
	.8byte	.LFE1-.LFB1	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.4byte	.Lchar_ptr_ptr_type - .Ldebug_info0 # DW_AT_sibling
	.uleb128 0x7	# (DIE (0xa6) DW_TAG_formal_parameter)
	.ascii "argc\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (fission-loclists.c)
	.byte	0x6	# DW_AT_decl_line
	.4byte	.Lint_type - .Ldebug_info0 # DW_AT_type
	.4byte	.LLST0-.Ldebug_loc0	# DW_AT_location
	.uleb128 0x7	# (DIE (0xb6) DW_TAG_formal_parameter)
	.ascii "argv\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (fission-loclists.c)
	.byte	0x6	# DW_AT_decl_line
	.4byte	.Lchar_ptr_ptr_type - .Ldebug_info0 # DW_AT_type
	.4byte	.LLST1-.Ldebug_loc0	# DW_AT_location
	.uleb128 0x8	# (DIE (0xc6) DW_TAG_GNU_call_site)
	.uleb128 0x2	# DW_AT_low_pc
	.4byte	.Lfoo_func - .Ldebug_info0 # DW_AT_abstract_origin
	.uleb128 0x9	# (DIE (0xcc) DW_TAG_GNU_call_site_parameter)
	.uleb128 0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.uleb128 0x2	# DW_AT_GNU_call_site_value
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -12
	.byte	0	# end of children of DIE 0xc6
	.byte	0	# end of children of DIE 0x8b
.Lchar_ptr_ptr_type:
	.uleb128 0x4	# (DIE (0xd4) DW_TAG_pointer_type)
	.byte	0x8	# DW_AT_byte_size
	.4byte	.Lchar_ptr_type - .Ldebug_info0 # DW_AT_type
.Lchar_ptr_type:
	.uleb128 0x4	# (DIE (0xda) DW_TAG_pointer_type)
	.byte	0x8	# DW_AT_byte_size
	.4byte	.Lchar_type - .Ldebug_info0 # DW_AT_type
.Lchar_type:
	.uleb128 0x5	# (DIE (0xe0) DW_TAG_base_type)
	.byte	0x1	# DW_AT_byte_size
	.byte	0x6	# DW_AT_encoding
	.ascii "char\0"	# DW_AT_name
	.uleb128 0xa	# (DIE (0xe8) DW_TAG_variable)
	.ascii "v\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (fission-loclists.c)
	.byte	0x1	# DW_AT_decl_line
	.4byte	.Lvolatile_int_type - .Ldebug_info0 # DW_AT_type
			# DW_AT_external
	.uleb128 0x2	# DW_AT_location
	.byte	0xfb	# DW_OP_GNU_addr_index
	.uleb128 0x1	# (index into .debug_addr)
.Lvolatile_int_type:
	.uleb128 0xb	# (DIE (0xf4) DW_TAG_volatile_type)
	.4byte	.Lint_type - .Ldebug_info0 # DW_AT_type
	.byte	0	# end of children of DIE 0xb
.Ldebug_info0_end:
	.section	.debug_info,"",@progbits
.Lskeleton_debug_info0:
	.4byte	.Lskeleton_debug_info0_end - .Lskeleton_debug_info0 - 4 # Length of Compilation Unit Info
	.value	0x4	# DWARF version number
	.4byte	.Lskeleton_debug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0) DW_TAG_compile_unit)
	.4byte	.Ldebug_ranges0+0	# DW_AT_ranges
	# This next line is the core of the test.
	# We want a non-zero value for low_pc to verify gdb is *not* using it
	# (the initial base address) when evaluating location list addresses in
	# .debug_loc.dwo.
	# Note: Setting a non-zero value here means we had to adjust the
	# values in .debug_ranges.
	.8byte	.Ltext0	# DW_AT_low_pc
	.4byte	.Ldebug_line0	# DW_AT_stmt_list
	.byte	0xcb	# DW_AT_GNU_dwo_id
	.byte	0x64
	.byte	0x3e
	.byte	0x34
	.byte	0x1b
	.byte	0x8a
	.byte	0xd6
	.byte	0x5
	.4byte	.Ldebug_ranges0	# DW_AT_GNU_ranges_base
	.ascii "/tmp/src/gdb/testsuite\0"	# DW_AT_comp_dir
	.asciz DWO	# DW_AT_GNU_dwo_name
	.4byte	.Ldebug_pubnames0	# DW_AT_GNU_pubnames
	.4byte	.Ldebug_pubtypes0	# DW_AT_GNU_pubtypes
	.4byte	.Ldebug_addr0	# DW_AT_GNU_addr_base
.Lskeleton_debug_info0_end:
	.section	.debug_abbrev,"",@progbits
.Lskeleton_debug_abbrev0:
	.uleb128 0x1	# (abbrev code)
	.uleb128 0x11	# (TAG: DW_TAG_compile_unit)
	.byte	0	# DW_children_no
	.uleb128 0x55	# (DW_AT_ranges)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x10	# (DW_AT_stmt_list)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x2131	# (DW_AT_GNU_dwo_id)
	.uleb128 0x7	# (DW_FORM_data8)
	.uleb128 0x2132	# (DW_AT_GNU_ranges_base)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x1b	# (DW_AT_comp_dir)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x2130	# (DW_AT_GNU_dwo_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x2134	# (DW_AT_GNU_pubnames)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x2135	# (DW_AT_GNU_pubtypes)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x2133	# (DW_AT_GNU_addr_base)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.byte	0
	.byte	0
	.uleb128 0x2	# (abbrev code)
	.uleb128 0x41	# (TAG: DW_TAG_type_unit)
	.byte	0	# DW_children_no
	.uleb128 0x1b	# (DW_AT_comp_dir)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x2130	# (DW_AT_GNU_dwo_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x2134	# (DW_AT_GNU_pubnames)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x2135	# (DW_AT_GNU_pubtypes)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x2133	# (DW_AT_GNU_addr_base)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.byte	0
	.byte	0
	.byte	0	# end of skeleton .debug_abbrev
	.section	.debug_abbrev.dwo,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1	# (abbrev code)
	.uleb128 0x11	# (TAG: DW_TAG_compile_unit)
	.byte	0x1	# DW_children_yes
	.uleb128 0x25	# (DW_AT_producer)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x13	# (DW_AT_language)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x1b	# (DW_AT_comp_dir)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x2131	# (DW_AT_GNU_dwo_id)
	.uleb128 0x7	# (DW_FORM_data8)
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
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1f01	# (DW_FORM_GNU_addr_index)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x7	# (DW_FORM_data8)
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
	.uleb128 0xf	# (TAG: DW_TAG_pointer_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
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
	.byte	0x1	# DW_children_yes
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x8	# (DW_FORM_string)
	.uleb128 0x3a	# (DW_AT_decl_file)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3b	# (DW_AT_decl_line)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x27	# (DW_AT_prototyped)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1f01	# (DW_FORM_GNU_addr_index)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x7	# (DW_FORM_data8)
	.uleb128 0x40	# (DW_AT_frame_base)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2117	# (DW_AT_GNU_all_call_sites)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x1	# (DW_AT_sibling)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x7	# (abbrev code)
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
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.byte	0
	.byte	0
	.uleb128 0x8	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0x1	# DW_children_yes
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1f01	# (DW_FORM_GNU_addr_index)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x9	# (abbrev code)
	.uleb128 0x410a	# (TAG: DW_TAG_GNU_call_site_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2111	# (DW_AT_GNU_call_site_value)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.byte	0
	.byte	0
	.uleb128 0xa	# (abbrev code)
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
	.uleb128 0xb	# (abbrev code)
	.uleb128 0x35	# (TAG: DW_TAG_volatile_type)
	.byte	0	# DW_children_no
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc.dwo,"",@progbits
.Ldebug_loc0:
.LLST0:
	.byte	0x3	# Location list start/length entry (*.LLST0)
	.uleb128 0	# Location list range start index (*.LVL1)
	.4byte	.LVL2-.LVL1	# Location list range length (*.LLST0)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.byte	0x3	# Location list start/length entry (*.LLST0)
	.uleb128 0x6	# Location list range start index (*.LVL2)
	.4byte	.LFE1-.LVL2	# Location list range length (*.LLST0)
	.value	0x2	# Location expression size
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -12
	.byte	0	# Location list terminator (*.LLST0)
.LLST1:
	.byte	0x3	# Location list start/length entry (*.LLST1)
	.uleb128 0	# Location list range start index (*.LVL1)
	.4byte	.LVL3-1-.LVL1	# Location list range length (*.LLST1)
	.value	0x1	# Location expression size
	.byte	0x54	# DW_OP_reg4
	.byte	0x3	# Location list start/length entry (*.LLST1)
	.uleb128 0x5	# Location list range start index (*.LVL3-1)
	.4byte	.LFE1-.LVL3-1	# Location list range length (*.LLST1)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x54	# DW_OP_reg4
	.byte	0x9f	# DW_OP_stack_value
	.byte	0	# Location list terminator (*.LLST1)
	.section	.debug_gnu_pubnames,"",@progbits
.Ldebug_pubnames0:
	.long	0x28	# Pub Info Length
	.value	0x2	# DWARF Version
	.long	.Lskeleton_debug_info0	# Offset of Compilation Unit Info
	.long	0xa6	# Compilation Unit Length
	.long	0x18	# DIE offset
	.byte	0x30	# GDB-index flags
	.ascii "foo\0"	# external name
	.long	0x47	# DIE offset
	.byte	0x30	# GDB-index flags
	.ascii "main\0"	# external name
	.long	0x94	# DIE offset
	.byte	0x20	# GDB-index flags
	.ascii "v\0"	# external name
	.long	0
	.section	.debug_gnu_pubtypes,"",@progbits
.Ldebug_pubtypes0:
	.long	0x21	# Pub Info Length
	.value	0x2	# DWARF Version
	.long	.Lskeleton_debug_info0	# Offset of Compilation Unit Info
	.long	0xa6	# Compilation Unit Length
	.long	0x40	# DIE offset
	.byte	0x90	# GDB-index flags
	.ascii "int\0"	# external name
	.long	0x90	# DIE offset
	.byte	0x90	# GDB-index flags
	.ascii "char\0"	# external name
	.long	0
	.section	.debug_aranges,"",@progbits
	.long	0x3c	# Length of Address Ranges Info
	.value	0x2	# DWARF Version
	.long	.Lskeleton_debug_info0	# Offset of Compilation Unit Info
	.byte	0x8	# Size of Address
	.byte	0	# Size of Segment Descriptor
	.value	0	# Pad to 16 byte boundary
	.value	0
	.quad	.Ltext0	# Address
	.quad	.Letext0-.Ltext0	# Length
	.quad	.LFB1	# Address
	.quad	.LFE1-.LFB1	# Length
	.quad	0
	.quad	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.8byte	.Ltext0 - .Ltext0	# Offset 0
	.8byte	.Letext0 - .Ltext0
	.8byte	.LFB1 - .Ltext0	# Offset 0x10
	.8byte	.LFE1 - .Ltext0
	.8byte	0
	.8byte	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_line.dwo,"",@progbits
.Lskeleton_debug_line0:
	.4byte	.LELT0-.LSLT0	# Length of Source Line Info
.LSLT0:
	.value	0x4	# DWARF Version
	.4byte	.LELTP0-.LASLTP0	# Prolog Length
.LASLTP0:
	.byte	0x1	# Minimum Instruction Length
	.byte	0x1	# Maximum Operations Per Instruction
	.byte	0x1	# Default is_stmt_start flag
	.byte	0xf6	# Line Base Value (Special Opcodes)
	.byte	0xf2	# Line Range Value (Special Opcodes)
	.byte	0xd	# Special Opcode Base
	.byte	0	# opcode: 0x1 has 0 args
	.byte	0x1	# opcode: 0x2 has 1 args
	.byte	0x1	# opcode: 0x3 has 1 args
	.byte	0x1	# opcode: 0x4 has 1 args
	.byte	0x1	# opcode: 0x5 has 1 args
	.byte	0	# opcode: 0x6 has 0 args
	.byte	0	# opcode: 0x7 has 0 args
	.byte	0	# opcode: 0x8 has 0 args
	.byte	0x1	# opcode: 0x9 has 1 args
	.byte	0	# opcode: 0xa has 0 args
	.byte	0	# opcode: 0xb has 0 args
	.byte	0x1	# opcode: 0xc has 1 args
	.byte	0	# End directory table
	.ascii "fission-loclists.c\0"	# File Entry: 0x1
	.uleb128 0
	.uleb128 0
	.uleb128 0
	.byte	0	# End file name table
.LELTP0:
.LELT0:
	.section	.debug_str_offsets.dwo,"",@progbits
	.section	.debug_str.dwo,"",@progbits
	.ident	"GCC: 4.7.x-fission"
	.section	.note.GNU-stack,"",@progbits
