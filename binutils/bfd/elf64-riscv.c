/* RISC-V-specific support for 64-bit ELF.
   Copyright 2011-2014 Free Software Foundation, Inc.

   Contributed by Andrew Waterman (waterman@cs.berkeley.edu) at UC Berkeley.
   Based on TILE-Gx and MIPS targets.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */


/* This file supports the 64-bit RISC-V ELF ABI.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "aout/ar.h"
#include "bfdlink.h"
#include "genlink.h"
#include "elf-bfd.h"
#include "elfxx-riscv.h"
#include "elf/riscv.h"
#include "opcode/riscv.h"

#define ELF_ARCH			bfd_arch_riscv
#define ELF_TARGET_ID			RISCV_ELF_DATA
#define ELF_MACHINE_CODE		EM_RISCV
#define ELF_MAXPAGESIZE			0x2000
#define ELF_COMMONPAGESIZE		0x2000

#define TARGET_LITTLE_SYM		bfd_elf64_riscv_vec
#define TARGET_LITTLE_NAME		"elf64-littleriscv"

#define elf_backend_reloc_type_class	     riscv_reloc_type_class

#define bfd_elf64_bfd_reloc_name_lookup      riscv_reloc_name_lookup
#define bfd_elf64_bfd_link_hash_table_create riscv_elf_link_hash_table_create
#define bfd_elf64_bfd_reloc_type_lookup	     riscv_reloc_type_lookup
#define bfd_elf64_bfd_merge_private_bfd_data \
  _bfd_riscv_elf_merge_private_bfd_data

#define elf_backend_copy_indirect_symbol     riscv_elf_copy_indirect_symbol
#define elf_backend_create_dynamic_sections  riscv_elf_create_dynamic_sections
#define elf_backend_check_relocs	     riscv_elf_check_relocs
#define elf_backend_adjust_dynamic_symbol    riscv_elf_adjust_dynamic_symbol
#define elf_backend_size_dynamic_sections    riscv_elf_size_dynamic_sections
#define elf_backend_relocate_section	     riscv_elf_relocate_section
#define elf_backend_finish_dynamic_symbol    riscv_elf_finish_dynamic_symbol
#define elf_backend_finish_dynamic_sections  riscv_elf_finish_dynamic_sections
#define elf_backend_gc_mark_hook	     riscv_elf_gc_mark_hook
#define elf_backend_gc_sweep_hook            riscv_elf_gc_sweep_hook
#define elf_backend_plt_sym_val		     riscv_elf_plt_sym_val
#define elf_info_to_howto_rel                NULL
#define elf_info_to_howto                    riscv_info_to_howto_rela
#define bfd_elf64_bfd_relax_section          _bfd_riscv_relax_section

#define elf_backend_init_index_section	_bfd_elf_init_1_index_section

#define elf_backend_can_gc_sections 1
#define elf_backend_can_refcount 1
#define elf_backend_want_got_plt 1
#define elf_backend_plt_readonly 1
#define elf_backend_plt_alignment 4
#define elf_backend_want_plt_sym 1
#define elf_backend_got_header_size 8
#define elf_backend_rela_normal 1
#define elf_backend_default_execstack 0

#include "elf64-target.h"
