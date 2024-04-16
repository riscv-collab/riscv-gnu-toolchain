/* KVX-specific backend routines.
   Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Contributed by Kalray SA.

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
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#include "bfd.h"
#include "elf-bfd.h"
#include "stdint.h"


/* KVX stub generation support for ELF64.  Called from the linker.  */
extern int elf64_kvx_setup_section_lists
  (bfd *, struct bfd_link_info *);
extern void elf64_kvx_next_input_section
  (struct bfd_link_info *, struct bfd_section *);
extern bool elf64_kvx_size_stubs
  (bfd *, bfd *, struct bfd_link_info *, bfd_signed_vma,
   struct bfd_section * (*) (const char *, struct bfd_section *),
   void (*) (void));
extern bool elf64_kvx_build_stubs
  (struct bfd_link_info *);
/* Kvx stub generation support for ELF32.  Called from the linker.  */
extern int elf32_kvx_setup_section_lists
  (bfd *, struct bfd_link_info *);
extern void elf32_kvx_next_input_section
  (struct bfd_link_info *, struct bfd_section *);
extern bool elf32_kvx_size_stubs
  (bfd *, bfd *, struct bfd_link_info *, bfd_signed_vma,
   struct bfd_section * (*) (const char *, struct bfd_section *),
   void (*) (void));
extern bool elf32_kvx_build_stubs
  (struct bfd_link_info *);


extern bfd_reloc_status_type
_bfd_kvx_elf_put_addend (bfd *, bfd_byte *, bfd_reloc_code_real_type,
			     reloc_howto_type *, bfd_signed_vma);

bool
kvx_elf32_init_stub_bfd (struct bfd_link_info *info,
			bfd *stub_bfd);
bool
kvx_elf64_init_stub_bfd (struct bfd_link_info *info,
			bfd *stub_bfd);

bool _bfd_kvx_elf_grok_prstatus (bfd *abfd, Elf_Internal_Note *note);

bool _bfd_kvx_elf_grok_psinfo (bfd *abfd, Elf_Internal_Note *note);

#define elf_backend_grok_prstatus _bfd_kvx_elf_grok_prstatus
#define elf_backend_grok_psinfo _bfd_kvx_elf_grok_psinfo
