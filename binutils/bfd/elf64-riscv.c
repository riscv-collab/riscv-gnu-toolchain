/* RISC-V-specific support for 64-bit ELF
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


/* This file supports the 64-bit RISC-V ELF ABI. */

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

#include "opcode/riscv.h"

static bfd_boolean riscv_elf64_object_p
  (bfd *);
static bfd_boolean elf64_riscv_grok_prstatus
  (bfd *, Elf_Internal_Note *);
static bfd_boolean elf64_riscv_grok_psinfo
  (bfd *, Elf_Internal_Note *);

/* The number of local .got entries we reserve.  */
#define RISCV_RESERVED_GOTNO (2)

/* Set the right machine number for a RISC-V ELF file.  */

static bfd_boolean
riscv_elf64_object_p (bfd *abfd)
{
  bfd_default_set_arch_mach (abfd, bfd_arch_riscv, bfd_mach_riscv64);
  return TRUE;
}

/* Support for core dump NOTE sections.  */
static bfd_boolean
elf64_riscv_grok_prstatus (bfd *abfd, Elf_Internal_Note *note)
{
  int offset;
  unsigned int size;

  switch (note->descsz)
    {
      default:
	return FALSE;

      case 480:		/* Linux/RISC-V - N64 kernel */
	/* pr_cursig */
	elf_tdata (abfd)->core->signal = bfd_get_16 (abfd, note->descdata + 12);

	/* pr_pid */
	elf_tdata (abfd)->core->lwpid = bfd_get_32 (abfd, note->descdata + 32);

	/* pr_reg */
	offset = 112;
	size = 360;

	break;
    }

  /* Make a ".reg/999" section.  */
  return _bfd_elfcore_make_pseudosection (abfd, ".reg",
					  size, note->descpos + offset);
}

static bfd_boolean
elf64_riscv_grok_psinfo (bfd *abfd, Elf_Internal_Note *note)
{
  switch (note->descsz)
    {
      default:
	return FALSE;

      case 136:		/* Linux/RISC-V - 64-bit kernel elf_prpsinfo */
	elf_tdata (abfd)->core->program
	 = _bfd_elfcore_strndup (abfd, note->descdata + 40, 16);
	elf_tdata (abfd)->core->command
	 = _bfd_elfcore_strndup (abfd, note->descdata + 56, 80);
    }

  /* Note that for some reason, a spurious space is tacked
     onto the end of the args in some (at least one anyway)
     implementations, so strip it off if it exists.  */

  {
    char *command = elf_tdata (abfd)->core->command;
    int n = strlen (command);

    if (0 < n && command[n - 1] == ' ')
      command[n - 1] = '\0';
  }

  return TRUE;
}

#define ELF_ARCH			bfd_arch_riscv
#define ELF_TARGET_ID			RISCV_ELF_DATA
#define ELF_MACHINE_CODE		EM_RISCV

#define elf_backend_collect		TRUE
#define elf_backend_type_change_ok	TRUE
#define elf_backend_can_gc_sections	TRUE
#define elf_info_to_howto		riscv_elf_info_to_howto_rela
#define elf_backend_object_p		riscv_elf64_object_p
#define elf_backend_symbol_processing	_bfd_riscv_elf_symbol_processing
#define elf_backend_create_dynamic_sections \
				_bfd_riscv_elf_create_dynamic_sections
#define elf_backend_check_relocs	_bfd_riscv_elf_check_relocs
#define elf_backend_merge_symbol_attribute \
				_bfd_riscv_elf_merge_symbol_attribute
#define elf_backend_get_target_dtag	_bfd_riscv_elf_get_target_dtag
#define elf_backend_adjust_dynamic_symbol \
				_bfd_riscv_elf_adjust_dynamic_symbol
#define elf_backend_always_size_sections \
				_bfd_riscv_elf_always_size_sections
#define elf_backend_size_dynamic_sections \
				_bfd_riscv_elf_size_dynamic_sections
#define elf_backend_init_index_section	_bfd_elf_init_1_index_section
#define elf_backend_relocate_section    _bfd_riscv_elf_relocate_section
#define elf_backend_finish_dynamic_symbol \
				_bfd_riscv_elf_finish_dynamic_symbol
#define elf_backend_finish_dynamic_sections \
				_bfd_riscv_elf_finish_dynamic_sections
#define elf_backend_additional_program_headers \
				_bfd_riscv_elf_additional_program_headers
#define elf_backend_modify_segment_map	_bfd_riscv_elf_modify_segment_map
#define elf_backend_copy_indirect_symbol \
					_bfd_riscv_elf_copy_indirect_symbol
#define elf_backend_ignore_discarded_relocs \
					_bfd_riscv_elf_ignore_discarded_relocs

#define elf_backend_grok_prstatus	elf64_riscv_grok_prstatus
#define elf_backend_grok_psinfo		elf64_riscv_grok_psinfo

#define elf_backend_got_header_size	(4 * RISCV_RESERVED_GOTNO)

/* RISC-V ELF64 can use a mixture of REL and RELA, but some Relocations
   work better/work only in RELA, so we default to this.  */
#define elf_backend_may_use_rel_p	1
#define elf_backend_may_use_rela_p	1
#define elf_backend_default_use_rela_p	1
#define elf_backend_rela_plts_and_copies_p 0
#define elf_backend_plt_readonly	1
#define elf_backend_plt_sym_val		_bfd_riscv_elf_plt_sym_val

#define elf_backend_sign_extend_vma	TRUE

#define elf_backend_write_section	_bfd_riscv_elf_write_section

#define bfd_elf64_new_section_hook	_bfd_riscv_elf_new_section_hook
#define bfd_elf64_bfd_get_relocated_section_contents \
				bfd_generic_get_relocated_section_contents
#define bfd_elf64_bfd_link_hash_table_create \
				_bfd_riscv_elf_link_hash_table_create
#define bfd_elf64_bfd_final_link	_bfd_riscv_elf_final_link
#define bfd_elf64_bfd_merge_private_bfd_data \
				_bfd_riscv_elf_merge_private_bfd_data
#define bfd_elf64_bfd_print_private_bfd_data \
				_bfd_riscv_elf_print_private_bfd_data

#define bfd_elf64_bfd_relax_section     _bfd_riscv_relax_section

/* RISC-V ELF64 archive functions.  */
#define bfd_elf64_archive_functions
extern bfd_boolean bfd_elf64_archive_slurp_armap
  (bfd *);
extern bfd_boolean bfd_elf64_archive_write_armap
  (bfd *, unsigned int, struct orl *, unsigned int, int);
#define bfd_elf64_archive_slurp_extended_name_table \
			_bfd_archive_coff_slurp_extended_name_table
#define bfd_elf64_archive_construct_extended_name_table \
			_bfd_archive_coff_construct_extended_name_table
#define bfd_elf64_archive_truncate_arname \
			_bfd_archive_coff_truncate_arname
#define bfd_elf64_archive_read_ar_hdr	_bfd_archive_coff_read_ar_hdr
#define bfd_elf64_archive_write_ar_hdr	_bfd_archive_coff_write_ar_hdr
#define bfd_elf64_archive_openr_next_archived_file \
			_bfd_archive_coff_openr_next_archived_file
#define bfd_elf64_archive_get_elt_at_index \
			_bfd_archive_coff_get_elt_at_index
#define bfd_elf64_archive_generic_stat_arch_elt \
			_bfd_archive_coff_generic_stat_arch_elt
#define bfd_elf64_archive_update_armap_timestamp \
			_bfd_archive_coff_update_armap_timestamp
#define bfd_elf64_bfd_reloc_type_lookup \
			riscv_elf_bfd_reloc_type_lookup
#define bfd_elf64_bfd_reloc_name_lookup \
			riscv_elf_bfd_reloc_name_lookup

#define TARGET_LITTLE_SYM		bfd_elf64_riscv_vec
#define TARGET_LITTLE_NAME		"elf64-littleriscv"

#define ELF_MAXPAGESIZE			0x2000
#define ELF_COMMONPAGESIZE		0x2000

#include "elf64-target.h"
