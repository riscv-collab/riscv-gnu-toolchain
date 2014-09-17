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
#include "elf/riscv.h"

extern bfd_boolean _bfd_riscv_elf_new_section_hook
  (bfd *, asection *);
extern void _bfd_riscv_elf_symbol_processing
  (bfd *, asymbol *);
extern unsigned int _bfd_riscv_elf_eh_frame_address_size
  (bfd *, asection *);
extern bfd_boolean _bfd_riscv_elf_fake_sections
  (bfd *, Elf_Internal_Shdr *, asection *);
extern bfd_boolean _bfd_riscv_elf_create_dynamic_sections
  (bfd *, struct bfd_link_info *);
extern bfd_boolean _bfd_riscv_elf_check_relocs
  (bfd *, struct bfd_link_info *, asection *, const Elf_Internal_Rela *);
extern bfd_boolean _bfd_riscv_elf_adjust_dynamic_symbol
  (struct bfd_link_info *, struct elf_link_hash_entry *);
extern bfd_boolean _bfd_riscv_elf_always_size_sections
  (bfd *, struct bfd_link_info *);
extern bfd_boolean _bfd_riscv_elf_size_dynamic_sections
  (bfd *, struct bfd_link_info *);
extern bfd_boolean _bfd_riscv_elf_relocate_section
  (bfd *, struct bfd_link_info *, bfd *, asection *, bfd_byte *,
   Elf_Internal_Rela *, Elf_Internal_Sym *, asection **);
extern bfd_boolean _bfd_riscv_elf_finish_dynamic_symbol
  (bfd *, struct bfd_link_info *, struct elf_link_hash_entry *,
   Elf_Internal_Sym *);
extern bfd_boolean _bfd_riscv_elf_finish_dynamic_sections
  (bfd *, struct bfd_link_info *);
extern int _bfd_riscv_elf_additional_program_headers
  (bfd *, struct bfd_link_info *);
extern bfd_boolean _bfd_riscv_elf_modify_segment_map
  (bfd *, struct bfd_link_info *);
extern void _bfd_riscv_elf_copy_indirect_symbol
  (struct bfd_link_info *, struct elf_link_hash_entry *,
   struct elf_link_hash_entry *);
extern bfd_boolean _bfd_riscv_elf_ignore_discarded_relocs
  (asection *);
extern bfd_boolean _bfd_riscv_elf_find_nearest_line
  (bfd *, asection *, asymbol **, bfd_vma, const char **,
   const char **, unsigned int *);
extern bfd_boolean _bfd_riscv_elf_find_inliner_info
  (bfd *, const char **, const char **, unsigned int *);
extern bfd_boolean _bfd_riscv_elf_set_section_contents
  (bfd *, asection *, const void *, file_ptr, bfd_size_type);
extern struct bfd_link_hash_table *_bfd_riscv_elf_link_hash_table_create
  (bfd *);
extern bfd_boolean _bfd_riscv_elf_final_link
  (bfd *, struct bfd_link_info *);
extern bfd_boolean _bfd_riscv_elf_merge_private_bfd_data
  (bfd *, bfd *);
extern bfd_boolean _bfd_riscv_elf_print_private_bfd_data
  (bfd *, void *);
extern bfd_boolean _bfd_riscv_elf_discard_info
  (bfd *, struct elf_reloc_cookie *, struct bfd_link_info *);
extern bfd_boolean _bfd_riscv_elf_write_section
  (bfd *, struct bfd_link_info *, asection *, bfd_byte *);

extern bfd_reloc_status_type _bfd_riscv_elf_generic_reloc
  (bfd *, arelent *, asymbol *, void *, asection *, bfd *, char **);
extern bfd_boolean _bfd_riscv_relax_section
  (bfd *, asection *, struct bfd_link_info *, bfd_boolean *);
extern void _bfd_riscv_elf_merge_symbol_attribute
  (struct elf_link_hash_entry *, const Elf_Internal_Sym *, bfd_boolean, bfd_boolean);
extern char *_bfd_riscv_elf_get_target_dtag (bfd_vma);
extern void _bfd_riscv_elf_use_plts_and_copy_relocs
  (struct bfd_link_info *);
extern bfd_vma _bfd_riscv_elf_plt_sym_val
  (bfd_vma, const asection *, const arelent *rel);

extern const struct bfd_elf_special_section _bfd_riscv_elf_special_sections [];

extern bfd_boolean _bfd_riscv_elf_common_definition (Elf_Internal_Sym *);
extern reloc_howto_type *riscv_elf_bfd_reloc_type_lookup
  (bfd *, bfd_reloc_code_real_type);
extern reloc_howto_type *riscv_elf_bfd_reloc_name_lookup (bfd *, const char *);
extern void riscv_elf_info_to_howto_rel
  (bfd *, arelent *, Elf_Internal_Rela *);
extern void riscv_elf_info_to_howto_rela
  (bfd *, arelent *, Elf_Internal_Rela *);

#define elf_backend_common_definition   _bfd_riscv_elf_common_definition
#define elf_backend_special_sections _bfd_riscv_elf_special_sections
#define elf_backend_eh_frame_address_size _bfd_riscv_elf_eh_frame_address_size
#define elf_backend_merge_symbol_attribute  _bfd_riscv_elf_merge_symbol_attribute
