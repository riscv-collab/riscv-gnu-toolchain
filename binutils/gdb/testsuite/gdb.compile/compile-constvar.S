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

// gcc -o gdb.base/compile-constvar.S -dA -S -g gdb.base/compile-constvar.c

	.file	"compile-constvar.c"
	.file 1 "gdb.base/compile-constvar.c"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	.Lend-.Lstart	# Length of Compilation Unit Info
.Lstart:
	.value	0x4	# DWARF version number
	.long	.Ldebug_abbrev0	# Offset Into Abbrev. Section
	.byte	0x8	# Pointer Size (in bytes)
	.uleb128 0x1	# (DIE (0xb) DW_TAG_compile_unit)
	.long	.LASF0	# DW_AT_producer: "GNU C 4.8.2 20131212 (Red Hat 4.8.2-7) -mtune=generic -march=x86-64 -g"
	.byte	0x1	# DW_AT_language
	.long	.LASF1	# DW_AT_name: "gdb.base/compile-constvar.c"
	.long	.LASF2	# DW_AT_comp_dir: "/home/jkratoch/redhat/gdb-gdbjit/gdb/testsuite"
	.uleb128 0x2	# (DIE (0x1d) DW_TAG_variable)
	.long	.LASF3	# DW_AT_name: "constvar"
	.long	.Linttype-.Ldebug_info0	# DW_AT_type
			# DW_AT_external
	.byte	0x3	# DW_AT_const_value
.Linttype:
	.uleb128 0x3	# (DIE (0x32) DW_TAG_base_type)
	.byte	0x4	# DW_AT_byte_size
	.byte	0x5	# DW_AT_encoding
	.ascii "int\0"	# DW_AT_name
	.byte	0	# end of children of DIE 0xb
.Lend:
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
	.byte	0
	.byte	0
	.uleb128 0x2	# (abbrev code)
	.uleb128 0x34	# (TAG: DW_TAG_variable)
	.byte	0	# DW_children_no
	.uleb128 0x3	# (DW_AT_name)
	.uleb128 0xe	# (DW_FORM_strp)
	.uleb128 0x49	# (DW_AT_type)
	.uleb128 0x13	# (DW_FORM_ref4)
	.uleb128 0x3f	# (DW_AT_external)
	.uleb128 0x19	# (DW_FORM_flag_present)
	.uleb128 0x1c	# (DW_AT_const_value)
	.uleb128 0xb	# (DW_FORM_data1)
	.byte	0
	.byte	0
	.uleb128 0x3	# (abbrev code)
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
	.byte	0
	.section	.debug_str,"MS",@progbits,1
.LASF1:
	.string	"gdb.base/compile-constvar.c"
.LASF3:
	.string	"constvar"
.LASF2:
	.string	""
.LASF0:
	.string	"GNU C 4.8.2 20131212 (Red Hat 4.8.2-7) -mtune=generic -march=x86-64 -g"
	.ident	"GCC: (GNU) 4.8.2 20131212 (Red Hat 4.8.2-7)"
	.section	.note.GNU-stack,"",@progbits
