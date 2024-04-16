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
   gcc -O1 -gdwarf-4 -gsplit-debug -fPIE -S -dA fission-loclists-pie.c
   and then massaging the output.

   fission-loclists-pie.c:

   __attribute__((noinline)) int
   init (int *b)
   { 
     return 0; 
   } 

   int
   main (int argc, char *argv[])
   {
     return init (&argc);
   }
*/

	.file	"fission-loclists-pie.c"
	.text
.Ltext0:
	.globl	init
	.type	init, @function
init:
.LFB0:
	.file 1 "testsuite/gdb.dwarf2/fission-loclists-pie.c"
	# testsuite/gdb.dwarf2/fission-loclists-pie.c:22
	.loc 1 22 0
	.cfi_startproc
.LVL0:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (FALLTHRU)
	# testsuite/gdb.dwarf2/fission-loclists-pie.c:24
	.loc 1 24 0
	movl	$0, %eax
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE0:
	.size	init, .-init
	.globl	main
	.type	main, @function
main:
.LFB1:
	# testsuite/gdb.dwarf2/fission-loclists-pie.c:28
	.loc 1 28 0
	.cfi_startproc
.LVL1:
# BLOCK 2 freq:10000 seq:0
# PRED: ENTRY [100.0%]  (FALLTHRU)
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	# testsuite/gdb.dwarf2/fission-loclists-pie.c:29
	.loc 1 29 0
	leaq	4(%rsp), %rdi
.LVL2:
	call	init
.LVL3:
	# testsuite/gdb.dwarf2/fission-loclists-pie.c:30
	.loc 1 30 0
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
# SUCC: EXIT [100.0%] 
	ret
	.cfi_endproc
.LFE1:
	.size	main, .-main
.Letext0:
	.section	.debug_addr,"",@progbits
.Ldebug_addr0:
	.quad	.LVL1	# 0x0
	.quad	.LVL2	# 0x1
	.quad	.LVL3	# 0x2
	.quad	.LFB0	# 0x3
	.quad	.LFB1	# 0x4
	.quad	.LVL3-1	# 0x5
	.section	.debug_info.dwo,"e",@progbits
.Ldebug_info0:
	.long	0x91	# Length of Compilation Unit Info
	.value	0x4	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.uleb128 0x4	# DW_AT_producer: "GNU C 4.9.x -m64 -mtune=generic -march=x86-64 -gdwarf-4 -gsplit-dwarf -O1 -fPIE"
	.byte	0x1	# DW_AT_language
	.uleb128 0x6	# DW_AT_name: "testsuite/gdb.dwarf2/fission-loclists-pie.c"
	.byte	0xed	# DW_AT_GNU_dwo_id
	.byte	0x50
	.byte	0xa4
	.byte	0xe0
	.byte	0xc9
	.byte	0xf8
	.byte	0xd3
	.byte	0xf0
	.uleb128 0x2	# (DIE (0x17) DW_TAG_subprogram)
			# DW_AT_external
	.uleb128 0x5	# DW_AT_name: "init"
	.byte	0x1	# DW_AT_decl_file (testsuite/gdb.dwarf2/fission-loclists-pie.c)
	.byte	0x15	# DW_AT_decl_line
			# DW_AT_prototyped
	.long	0x3a	# DW_AT_type
	.uleb128 0x3	# DW_AT_low_pc
	.quad	.LFE0-.LFB0	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.long	0x3a	# DW_AT_sibling
	.uleb128 0x3	# (DIE (0x2e) DW_TAG_formal_parameter)
	.ascii "b\0"	# DW_AT_name
	.byte	0x1	# DW_AT_decl_file (testsuite/gdb.dwarf2/fission-loclists-pie.c)
	.byte	0x15	# DW_AT_decl_line
	.long	0x41	# DW_AT_type
	.uleb128 0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.byte	0	# end of children of DIE 0x17
	.uleb128 0x4	# (DIE (0x3a) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.uleb128 0x5	# (DIE (0x41) DW_TAG_pointer_type)
	.byte	0x8	# DW_AT_byte_size
	.long	0x3a	# DW_AT_type
	.uleb128 0x2	# (DIE (0x47) DW_TAG_subprogram)
			# DW_AT_external
	.uleb128 0x2	# DW_AT_name: "main"
	.byte	0x1	# DW_AT_decl_file (testsuite/gdb.dwarf2/fission-loclists-pie.c)
	.byte	0x1b	# DW_AT_decl_line
			# DW_AT_prototyped
	.long	0x3a	# DW_AT_type
	.uleb128 0x4	# DW_AT_low_pc
	.quad	.LFE1-.LFB1	# DW_AT_high_pc
	.uleb128 0x1	# DW_AT_frame_base
	.byte	0x9c	# DW_OP_call_frame_cfa
			# DW_AT_GNU_all_call_sites
	.long	0x84	# DW_AT_sibling
	.uleb128 0x6	# (DIE (0x5e) DW_TAG_formal_parameter)
	.uleb128 0x1	# DW_AT_name: "argc"
	.byte	0x1	# DW_AT_decl_file (testsuite/gdb.dwarf2/fission-loclists-pie.c)
	.byte	0x1b	# DW_AT_decl_line
	.long	0x3a	# DW_AT_type
	.long	.LLST0-.Ldebug_loc0	# DW_AT_location
	.uleb128 0x6	# (DIE (0x6a) DW_TAG_formal_parameter)
	.uleb128 0	# DW_AT_name: "argv"
	.byte	0x1	# DW_AT_decl_file (testsuite/gdb.dwarf2/fission-loclists-pie.c)
	.byte	0x1b	# DW_AT_decl_line
	.long	0x84	# DW_AT_type
	.long	.LLST1-.Ldebug_loc0	# DW_AT_location
	.uleb128 0x7	# (DIE (0x76) DW_TAG_GNU_call_site)
	.uleb128 0x2	# DW_AT_low_pc
	.long	0x17	# DW_AT_abstract_origin
	.uleb128 0x8	# (DIE (0x7c) DW_TAG_GNU_call_site_parameter)
	.uleb128 0x1	# DW_AT_location
	.byte	0x55	# DW_OP_reg5
	.uleb128 0x2	# DW_AT_GNU_call_site_value
	.byte	0x91	# DW_OP_fbreg
	.sleb128 -12
	.byte	0	# end of children of DIE 0x76
	.byte	0	# end of children of DIE 0x47
	.uleb128 0x5	# (DIE (0x84) DW_TAG_pointer_type)
	.byte	0x8	# DW_AT_byte_size
	.long	0x8a	# DW_AT_type
	.uleb128 0x5	# (DIE (0x8a) DW_TAG_pointer_type)
	.byte	0x8	# DW_AT_byte_size
	.long	0x90	# DW_AT_type
	.uleb128 0x9	# (DIE (0x90) DW_TAG_base_type)
	.byte	0x1	# DW_AT_byte_size
	.byte	0x6	# DW_AT_encoding
	.uleb128 0x3	# DW_AT_name: "char"
	.byte	0	# end of children of DIE 0xb
	.section	.debug_info,"",@progbits
.Lskeleton_debug_info0:
	.long	0x30	# Length of Compilation Unit Info
	.value	0x4	# DWARF version number
	.long	.Lskeleton_debug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0) DW_TAG_compile_unit)
	.quad	.Ltext0	# DW_AT_low_pc
	.quad	.Letext0-.Ltext0	# DW_AT_high_pc
	.long	.Ldebug_line0	# DW_AT_stmt_list
	.long	.LASF0	# DW_AT_GNU_dwo_name: "fission-loclists-pie.dwo"
	.long	.LASF1	# DW_AT_comp_dir: "/tmp/19999/obj64/gdb/testsuite"
			# DW_AT_GNU_pubnames
	.long	.Ldebug_addr0	# DW_AT_GNU_addr_base
	.byte	0xed	# DW_AT_GNU_dwo_id
	.byte	0x50
	.byte	0xa4
	.byte	0xe0
	.byte	0xc9
	.byte	0xf8
	.byte	0xd3
	.byte	0xf0
	.section	.debug_abbrev,"",@progbits
.Lskeleton_debug_abbrev0:
	.uleb128 0x1	# (abbrev code)
	.uleb128 0x11	# (TAG: DW_TAG_compile_unit)
	.byte	0	# DW_children_no
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1	# (DW_FORM_addr)
	.uleb128 0x12	# (DW_AT_high_pc)
	.uleb128 0x7	# (DW_FORM_data8)
	.uleb128 0x10	# (DW_AT_stmt_list)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x2130	# (DW_AT_GNU_dwo_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x1b	# (DW_AT_comp_dir)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x2134	# (DW_AT_GNU_pubnames)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x2133	# (DW_AT_GNU_addr_base)
	.uleb128 0x17	# (DW_FORM_sec_offset)
	.uleb128 0x2131	# (DW_AT_GNU_dwo_id)
	.uleb128 0x7	# (DW_FORM_data8)
	.byte	0
	.byte	0
	.byte	0	# end of skeleton .debug_abbrev
	.section	.debug_abbrev.dwo,"e",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1	# (abbrev code)
	.uleb128 0x11	# (TAG: DW_TAG_compile_unit)
	.byte	0x1	# DW_children_yes
	.uleb128 0x25	# (DW_AT_producer)
	.uleb128 0x1f02	# (DW_FORM_GNU_str_index)
	.uleb128 0x13	# (DW_AT_language)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x1f02	# (DW_FORM_GNU_str_index)
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
	.uleb128 0x1f02	# (DW_FORM_GNU_str_index)
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
	.uleb128 0x5	# (abbrev code)
	.uleb128 0xf	# (TAG: DW_TAG_pointer_type)
	.byte	0	# DW_children_no
	.uleb128 0xb	# (DW_AT_byte_size)
	.uleb128 0xb	# (DW_FORM_data1)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x6	# (abbrev code)
	.uleb128 0x5	# (TAG: DW_TAG_formal_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0x1f02	# (DW_FORM_GNU_str_index)
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
	.uleb128 0x7	# (abbrev code)
	.uleb128 0x4109	# (TAG: DW_TAG_GNU_call_site)
	.byte	0x1	# DW_children_yes
	.uleb128 0x11	# (DW_AT_low_pc)
	.uleb128 0x1f01	# (DW_FORM_GNU_addr_index)
	.uleb128 0x31	# (DW_AT_abstract_origin)
	.uleb128 0x13	# (DW_FORM_ref4)
	.byte	0
	.byte	0
	.uleb128 0x8	# (abbrev code)
	.uleb128 0x410a	# (TAG: DW_TAG_GNU_call_site_parameter)
	.byte	0	# DW_children_no
	.uleb128 0x2	# (DW_AT_location)
	.uleb128 0x18	# (DW_FORM_exprloc)
	.uleb128 0x2111	# (DW_AT_GNU_call_site_value)
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
	.uleb128 0x1f02	# (DW_FORM_GNU_str_index)
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc.dwo,"e",@progbits
.Ldebug_loc0:
.LLST0:
	.byte	0x3	# Location list start/length entry (*.LLST0)
	.uleb128 0	# Location list range start index (*.LVL1)
	.long	.LVL2-.LVL1	# Location list range length (*.LLST0)
	.value	0x1	# Location expression size
	.byte	0x55	# DW_OP_reg5
	.byte	0x3	# Location list start/length entry (*.LLST0)
	.uleb128 0x1	# Location list range start index (*.LVL2)
	.long	.LFE1-.LVL2	# Location list range length (*.LLST0)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x55	# DW_OP_reg5
	.byte	0x9f	# DW_OP_stack_value
	.byte	0	# Location list terminator (*.LLST0)
.LLST1:
	.byte	0x3	# Location list start/length entry (*.LLST1)
	.uleb128 0	# Location list range start index (*.LVL1)
	.long	.LVL3-1-.LVL1	# Location list range length (*.LLST1)
	.value	0x1	# Location expression size
	.byte	0x54	# DW_OP_reg4
	.byte	0x3	# Location list start/length entry (*.LLST1)
	.uleb128 0x5	# Location list range start index (*.LVL3-1)
	.long	.LFE1-.LVL3-1	# Location list range length (*.LLST1)
	.value	0x4	# Location expression size
	.byte	0xf3	# DW_OP_GNU_entry_value
	.uleb128 0x1
	.byte	0x54	# DW_OP_reg4
	.byte	0x9f	# DW_OP_stack_value
	.byte	0	# Location list terminator (*.LLST1)
	.section	.debug_gnu_pubnames,"",@progbits
	.long	0x22	# Pub Info Length
	.value	0x2	# DWARF Version
	.long	.Lskeleton_debug_info0	# Offset of Compilation Unit Info
	.long	0x95	# Compilation Unit Length
	.long	0x17	# DIE offset
	.byte	0x30	# GDB-index flags
	.ascii "init\0"	# external name
	.long	0x47	# DIE offset
	.byte	0x30	# GDB-index flags
	.ascii "main\0"	# external name
	.long	0
	.section	.debug_gnu_pubtypes,"",@progbits
	.long	0x21	# Pub Info Length
	.value	0x2	# DWARF Version
	.long	.Lskeleton_debug_info0	# Offset of Compilation Unit Info
	.long	0x95	# Compilation Unit Length
	.long	0x3a	# DIE offset
	.byte	0x90	# GDB-index flags
	.ascii "int\0"	# external name
	.long	0x90	# DIE offset
	.byte	0x90	# GDB-index flags
	.ascii "char\0"	# external name
	.long	0
	.section	.debug_aranges,"",@progbits
	.long	0x2c	# Length of Address Ranges Info
	.value	0x2	# DWARF Version
	.long	.Lskeleton_debug_info0	# Offset of Compilation Unit Info
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
	.section	.debug_line.dwo,"e",@progbits
.Lskeleton_debug_line0:
	.long	.LELT0-.LSLT0	# Length of Source Line Info
.LSLT0:
	.value	0x4	# DWARF Version
	.long	.LELTP0-.LASLTP0	# Prolog Length
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
	.ascii "testsuite/gdb.dwarf2\0"	# Directory Entry: 0x1
	.byte	0	# End directory table
	.ascii "fission-loclists-pie.c\0"	# File Entry: 0x1
	.uleb128 0x1
	.uleb128 0
	.uleb128 0
	.byte	0	# End file name table
.LELTP0:
.LELT0:
	.section	.debug_str,"MS",@progbits,1
.LASF1:
	.string	"/tmp/19999/obj64/gdb/testsuite"
.LASF0:
	.string	DWO
	.section	.debug_str_offsets.dwo,"e",@progbits
	.long	0	# indexed string 0x0: argv
	.long	0x5	# indexed string 0x1: argc
	.long	0xa	# indexed string 0x2: main
	.long	0xf	# indexed string 0x3: char
	.long	0x14	# indexed string 0x4: GNU C 4.9.x -m64 -mtune=generic -march=x86-64 -gdwarf-4 -gsplit-dwarf -O1 -fPIE
	.long	0x64	# indexed string 0x5: init
	.long	0x69	# indexed string 0x6: testsuite/gdb.dwarf2/fission-loclists-pie.c
	.section	.debug_str.dwo,"e",@progbits
	.string	"argv"
	.string	"argc"
	.string	"main"
	.string	"char"
	.string	"GNU C 4.9.x -m64 -mtune=generic -march=x86-64 -gdwarf-4 -gsplit-dwarf -O1 -fPIE"
	.string	"init"
	.string	"testsuite/gdb.dwarf2/fission-loclists-pie.c"
	.ident	"GCC: 4.9.x"
	.section	.note.GNU-stack,"",@progbits
