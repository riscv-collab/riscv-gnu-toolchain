/* Target-dependent code for Newlib ARC.

   Copyright (C) 2016-2024 Free Software Foundation, Inc.
   Contributed by Synopsys Inc.

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
#include "arc-tdep.h"
#include "osabi.h"

/* Print an "arc-newlib" debug statement.  */

#define arc_newlib_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (arc_debug, "arc-newlib", fmt, ##__VA_ARGS__)

/* Implement the 'init_osabi' method of struct gdb_osabi_handler.  */

static void
arc_newlib_init_osabi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  arc_newlib_debug_printf ("Initialization.");

  arc_gdbarch_tdep *tdep = gdbarch_tdep<arc_gdbarch_tdep> (gdbarch);

  /* Offset of original PC in longjmp jump buffer (in registers).  Value of PC
     offset can be found in newlib/libc/machine/arc/setjmp.S.  */
  tdep->jb_pc = 18;
}

/* Recognize ARC Newlib object files.  */

static enum gdb_osabi
arc_newlib_osabi_sniffer (bfd *abfd)
{
  arc_newlib_debug_printf ("OS/ABI sniffer.");

  /* crt0.S in libgloss for ARC defines .ivt section for interrupt handlers.
     If this section is not present then this is likely not a newlib - could be
     a Linux application or some non-newlib baremetal application.  */
  if (bfd_get_section_by_name (abfd, ".ivt") != NULL)
    return GDB_OSABI_NEWLIB;
  else
    return GDB_OSABI_UNKNOWN;
}

void _initialize_arc_newlib_tdep ();
void
_initialize_arc_newlib_tdep ()
{
  gdbarch_register_osabi_sniffer (bfd_arch_arc, bfd_target_elf_flavour,
				  arc_newlib_osabi_sniffer);
  gdbarch_register_osabi (bfd_arch_arc, 0, GDB_OSABI_NEWLIB,
			  arc_newlib_init_osabi);
}
