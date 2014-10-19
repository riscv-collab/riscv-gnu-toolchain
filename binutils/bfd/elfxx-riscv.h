/* RISC-V ELF specific backend routines.
   Copyright 2011-2014 Free Software Foundation, Inc.

   Contributed by Andrew Waterman (waterman@cs.berkeley.edu) at UC Berkeley.
   Based on MIPS target.

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

#include "elf/common.h"
#include "elf/internal.h"

extern enum elf_reloc_type_class
riscv_reloc_type_class (const struct bfd_link_info *,
			const asection *,
			const Elf_Internal_Rela *);

extern reloc_howto_type *
riscv_reloc_name_lookup (bfd *, const char *);

extern struct bfd_link_hash_table *
riscv_elf_link_hash_table_create (bfd *);

extern reloc_howto_type *
riscv_reloc_type_lookup (bfd *, bfd_reloc_code_real_type);

extern void
riscv_elf_copy_indirect_symbol (struct bfd_link_info *,
				struct elf_link_hash_entry *,
				struct elf_link_hash_entry *);

extern bfd_boolean
riscv_elf_create_dynamic_sections (bfd *, struct bfd_link_info *);

extern bfd_boolean
riscv_elf_check_relocs (bfd *, struct bfd_link_info *,
			asection *, const Elf_Internal_Rela *);

extern bfd_boolean
riscv_elf_adjust_dynamic_symbol (struct bfd_link_info *,
				 struct elf_link_hash_entry *);

extern bfd_boolean
riscv_elf_size_dynamic_sections (bfd *, struct bfd_link_info *);

extern bfd_boolean
riscv_elf_relocate_section (bfd *, struct bfd_link_info *,
			    bfd *, asection *,
			    bfd_byte *, Elf_Internal_Rela *,
			    Elf_Internal_Sym *,
			    asection **);

extern asection *
riscv_elf_gc_mark_hook (asection *,
			struct bfd_link_info *,
			Elf_Internal_Rela *,
			struct elf_link_hash_entry *,
			Elf_Internal_Sym *);

extern bfd_boolean
riscv_elf_gc_sweep_hook (bfd *, struct bfd_link_info *,
			  asection *, const Elf_Internal_Rela *);

extern bfd_vma
riscv_elf_plt_sym_val (bfd_vma, const asection *, const arelent *);

extern void
riscv_info_to_howto_rela (bfd *, arelent *, Elf_Internal_Rela *);

extern bfd_boolean
riscv_elf_finish_dynamic_symbol (bfd *,
				 struct bfd_link_info *,
				 struct elf_link_hash_entry *,
				 Elf_Internal_Sym *);

extern bfd_boolean
riscv_elf_finish_dynamic_sections (bfd *, struct bfd_link_info *);

extern bfd_boolean
_bfd_riscv_elf_merge_private_bfd_data (bfd *, bfd *);

extern bfd_boolean
_bfd_riscv_relax_section (bfd *, asection *, struct bfd_link_info *,
			  bfd_boolean *);
