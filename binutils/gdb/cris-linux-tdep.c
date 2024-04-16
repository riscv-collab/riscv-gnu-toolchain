/* Target-dependent code for GNU/Linux on CRIS processors, for GDB.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

   Contributed by Axis Communications AB.
   Written by Hendrik Ruijter, Stefan Andersson, Orjan Friberg,
   Edgar Iglesias and Ricard Wanderlof.

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
#include "osabi.h"
#include "linux-tdep.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "gdbarch.h"

#include "cris-tdep.h"

static void
cris_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  cris_gdbarch_tdep *tdep = gdbarch_tdep<cris_gdbarch_tdep> (gdbarch);

  linux_init_abi (info, gdbarch, 0);

  if (tdep->cris_version == 32)
    /* Threaded debugging is only supported on CRISv32 for now.  */
    set_gdbarch_fetch_tls_load_module_address (gdbarch,
					       svr4_fetch_objfile_link_map);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 linux_ilp32_fetch_link_map_offsets);

}

void _initialize_cris_linux_tdep ();
void
_initialize_cris_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_cris, 0, GDB_OSABI_LINUX,
			  cris_linux_init_abi);
}
