# This shell script emits a C file. -*- C -*-
#   Copyright 2004, 2006, 2007, 2008 Free Software Foundation, Inc.
#
# This file is part of the GNU Binutils.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
# MA 02110-1301, USA.

fragment <<EOF

#include "ldmain.h"
#include "ldctor.h"
#include "elf/riscv.h"
#include "elfxx-riscv.h"

#define is_riscv_elf(bfd)				\
  (bfd_get_flavour (bfd) == bfd_target_elf_flavour	\
   && elf_tdata (bfd) != NULL				\
   && elf_object_id (bfd) == RISCV_ELF_DATA)

static void
riscv_after_parse (void)
{
  /* .gnu.hash and the RISC-V ABI require .dynsym to be sorted in different
     ways.  .gnu.hash needs symbols to be grouped by hash code whereas the
     RISC-V ABI requires a mapping between the GOT and the symbol table.  */
  if (link_info.emit_gnu_hash)
    {
      einfo ("%X%P: .gnu.hash is incompatible with the RISC-V ABI\n");
      link_info.emit_hash = TRUE;
      link_info.emit_gnu_hash = FALSE;
    }
  after_parse_default ();
}

static void
riscv_before_allocation (void)
{
  gld${EMULATION_NAME}_before_allocation ();

  if (link_info.discard == discard_sec_merge)
    link_info.discard = discard_l;

  if (RELAXATION_DISABLED_BY_DEFAULT)
    ENABLE_RELAXATION;
}

EOF

LDEMUL_AFTER_PARSE=riscv_after_parse
LDEMUL_BEFORE_ALLOCATION=riscv_before_allocation
