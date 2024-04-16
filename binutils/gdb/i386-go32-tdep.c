/* Target-dependent code for DJGPP/i386.

   Copyright (C) 1988-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

#include "defs.h"
#include "i386-tdep.h"
#include "gdbsupport/x86-xstate.h"
#include "target-descriptions.h"
#include "osabi.h"

static void
i386_go32_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  /* DJGPP doesn't have any special frames for signal handlers.  */
  tdep->sigtramp_p = NULL;

  tdep->jb_pc_offset = 36;

  /* DJGPP does not support the SSE registers.  */
  if (!tdesc_has_registers (info.target_desc))
    tdep->tdesc = i386_target_description (X86_XSTATE_X87_MASK, false);

  /* Native compiler is GCC, which uses the SVR4 register numbering
     even in COFF and STABS.  See the comment in i386_gdbarch_init,
     before the calls to set_gdbarch_stab_reg_to_regnum and
     set_gdbarch_sdb_reg_to_regnum.  */
  set_gdbarch_stab_reg_to_regnum (gdbarch, i386_svr4_reg_to_regnum);
  set_gdbarch_sdb_reg_to_regnum (gdbarch, i386_svr4_reg_to_regnum);

  set_gdbarch_has_dos_based_file_system (gdbarch, 1);

  set_gdbarch_wchar_bit (gdbarch, 16);
  set_gdbarch_wchar_signed (gdbarch, 0);
}


static enum gdb_osabi
i386_coff_osabi_sniffer (bfd *abfd)
{
  if (strcmp (bfd_get_target (abfd), "coff-go32-exe") == 0
      || strcmp (bfd_get_target (abfd), "coff-go32") == 0)
    return GDB_OSABI_GO32;

  return GDB_OSABI_UNKNOWN;
}


void _initialize_i386_go32_tdep ();
void
_initialize_i386_go32_tdep ()
{
  gdbarch_register_osabi_sniffer (bfd_arch_i386, bfd_target_coff_flavour,
				  i386_coff_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_GO32,
			  i386_go32_init_abi);
}
