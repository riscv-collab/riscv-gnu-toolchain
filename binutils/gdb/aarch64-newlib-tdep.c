/* Target-dependent code for Newlib AArch64.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

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

#include "gdbarch.h"
#include "aarch64-tdep.h"
#include "osabi.h"

/* Implement the 'init_osabi' method of struct gdb_osabi_handler.  */

static void
aarch64_newlib_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  /* Jump buffer - support for longjmp.
     Offset of original PC in jump buffer (in registers).  */
  tdep->jb_pc = 11;
}

void _initialize_aarch64_newlib_tdep ();
void
_initialize_aarch64_newlib_tdep ()
{
  gdbarch_register_osabi (bfd_arch_aarch64, 0, GDB_OSABI_NEWLIB,
			  aarch64_newlib_init_abi);
}
