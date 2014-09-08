/* MIPS-specific support for 32-bit ELF
   Copyright 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002,
   2003, 2004, 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.

   Most of the information added by Ian Lance Taylor, Cygnus Support,
   <ian@cygnus.com>.
   N32/64 ABI support added by Mark Mitchell, CodeSourcery, LLC.
   <mark@codesourcery.com>
   Traditional MIPS targets support added by Koundinya.K, Dansk Data
   Elektronik & Operations Research Group. <kk@ddeorg.soft.net>

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


/* This file handles MIPS ELF targets.  SGI Irix 5 uses a slightly
   different MIPS ELF from other targets.  This matters when linking.
   This file supports both, switching at runtime.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "bfdlink.h"
#include "genlink.h"
#include "elf-bfd.h"
#include "elfxx-riscv.h"
#include "elf/riscv.h"
#include "opcode/riscv.h"

#include "opcode/riscv.h"

static bfd_boolean mips_elf_sym_is_global
  (bfd *, asymbol *);
static bfd_boolean mips_elf_n32_object_p
  (bfd *);
static bfd_boolean elf32_mips_grok_prstatus
  (bfd *, Elf_Internal_Note *);
static bfd_boolean elf32_mips_grok_psinfo
  (bfd *, Elf_Internal_Note *);

extern const bfd_target bfd_elf32_nbigmips_vec;
extern const bfd_target bfd_elf32_nlittlemips_vec;

/* The number of local .got entries we reserve.  */
#define MIPS_RESERVED_GOTNO (2)

/* Determine whether a symbol is global for the purposes of splitting
   the symbol table into global symbols and local symbols.  At least
   on Irix 5, this split must be between section symbols and all other
   symbols.  On most ELF targets the split is between static symbols
   and externally visible symbols.  */

static bfd_boolean
mips_elf_sym_is_global (bfd *abfd ATTRIBUTE_UNUSED, asymbol *sym)
{
  return ((sym->flags & (BSF_GLOBAL | BSF_WEAK | BSF_GNU_UNIQUE)) != 0
	  || bfd_is_und_section (bfd_get_section (sym))
	  || bfd_is_com_section (bfd_get_section (sym)));
}

/* Set the right machine number for a MIPS ELF file.  */

static bfd_boolean
mips_elf_n32_object_p (bfd *abfd)
{
  bfd_default_set_arch_mach (abfd, bfd_arch_riscv, bfd_mach_riscv32);
  return TRUE;
}

/* Support for core dump NOTE sections.  */
static bfd_boolean
elf32_mips_grok_prstatus (bfd *abfd, Elf_Internal_Note *note)
{
  int offset;
  unsigned int size;

  switch (note->descsz)
    {
      default:
	return FALSE;

      case 440:		/* Linux/MIPS N32 */
	/* pr_cursig */
	elf_tdata (abfd)->core->signal = bfd_get_16 (abfd, note->descdata + 12);

	/* pr_pid */
	elf_tdata (abfd)->core->lwpid = bfd_get_32 (abfd, note->descdata + 24);

	/* pr_reg */
	offset = 72;
	size = 360;

	break;
    }

  /* Make a ".reg/999" section.  */
  return _bfd_elfcore_make_pseudosection (abfd, ".reg", size,
					  note->descpos + offset);
}

static bfd_boolean
elf32_mips_grok_psinfo (bfd *abfd, Elf_Internal_Note *note)
{
  switch (note->descsz)
    {
      default:
	return FALSE;

      case 128:		/* Linux/MIPS elf_prpsinfo */
	elf_tdata (abfd)->core->program
	 = _bfd_elfcore_strndup (abfd, note->descdata + 32, 16);
	elf_tdata (abfd)->core->command
	 = _bfd_elfcore_strndup (abfd, note->descdata + 48, 80);
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
#define ELF_TARGET_ID			MIPS_ELF_DATA
#define ELF_MACHINE_CODE		EM_RISCV

#define elf_backend_collect		TRUE
#define elf_backend_type_change_ok	TRUE
#define elf_backend_can_gc_sections	TRUE
#define elf_info_to_howto		riscv_elf_info_to_howto_rela
#define elf_backend_sym_is_global	mips_elf_sym_is_global
#define elf_backend_object_p		mips_elf_n32_object_p
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
#define elf_backend_relocate_section	_bfd_riscv_elf_relocate_section
#define elf_backend_finish_dynamic_symbol \
					_bfd_riscv_elf_finish_dynamic_symbol
#define elf_backend_finish_dynamic_sections \
					_bfd_riscv_elf_finish_dynamic_sections
#define elf_backend_additional_program_headers \
					_bfd_riscv_elf_additional_program_headers
#define elf_backend_modify_segment_map	_bfd_riscv_elf_modify_segment_map
#define elf_backend_copy_indirect_symbol \
					_bfd_riscv_elf_copy_indirect_symbol
#define elf_backend_grok_prstatus	elf32_mips_grok_prstatus
#define elf_backend_grok_psinfo		elf32_mips_grok_psinfo

#define elf_backend_got_header_size	(4 * MIPS_RESERVED_GOTNO)

/* MIPS n32 ELF can use a mixture of REL and RELA, but some Relocations
   work better/work only in RELA, so we default to this.  */
#define elf_backend_may_use_rel_p	1
#define elf_backend_may_use_rela_p	1
#define elf_backend_default_use_rela_p	1
#define elf_backend_rela_plts_and_copies_p 0
#define elf_backend_sign_extend_vma	TRUE
#define elf_backend_plt_readonly	1
#define elf_backend_plt_sym_val		_bfd_riscv_elf_plt_sym_val

#define elf_backend_discard_info	_bfd_riscv_elf_discard_info
#define elf_backend_ignore_discarded_relocs \
					_bfd_riscv_elf_ignore_discarded_relocs
#define elf_backend_write_section	_bfd_riscv_elf_write_section
#define bfd_elf32_new_section_hook	_bfd_riscv_elf_new_section_hook
#define bfd_elf32_bfd_get_relocated_section_contents \
				bfd_generic_get_relocated_section_contents
#define bfd_elf32_bfd_link_hash_table_create \
					_bfd_riscv_elf_link_hash_table_create
#define bfd_elf32_bfd_final_link	_bfd_riscv_elf_final_link
#define bfd_elf32_bfd_merge_private_bfd_data \
					_bfd_riscv_elf_merge_private_bfd_data
#define bfd_elf32_bfd_print_private_bfd_data \
					_bfd_riscv_elf_print_private_bfd_data
#define bfd_elf32_bfd_relax_section     _bfd_riscv_relax_section
#define bfd_elf32_bfd_reloc_type_lookup \
			riscv_elf_bfd_reloc_type_lookup
#define bfd_elf32_bfd_reloc_name_lookup \
			riscv_elf_bfd_reloc_name_lookup

/* Support for SGI-ish mips targets using n32 ABI.  */

#define TARGET_LITTLE_SYM               bfd_elf32_riscv_vec
#define TARGET_LITTLE_NAME              "elf32-littleriscv"

#define ELF_MAXPAGESIZE			0x1000
#define ELF_COMMONPAGESIZE		0x1000

#include "elf32-target.h"
