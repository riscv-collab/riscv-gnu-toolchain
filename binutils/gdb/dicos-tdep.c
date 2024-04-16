/* Target-dependent, architecture-independent code for DICOS, for GDB.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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
#include "solib.h"
#include "solib-target.h"
#include "inferior.h"
#include "dicos-tdep.h"
#include "gdbarch.h"

void
dicos_init_abi (struct gdbarch *gdbarch)
{
  set_gdbarch_so_ops (gdbarch, &solib_target_so_ops);

  /* Every process, although has its own address space, sees the same
     list of shared libraries.  There's no "main executable" in DICOS,
     so this accounts for all code.  */
  set_gdbarch_has_global_solist (gdbarch, 1);

  /* The DICOS breakpoint API takes care of magically making
     breakpoints visible to all inferiors.  */
  set_gdbarch_has_global_breakpoints (gdbarch, 1);

  /* There's no (standard definition of) entry point or a guaranteed
     text location with a symbol where to place the call dummy, so we
     need it on the stack.  Rely on i386_gdbarch_init used also for
     amd64 to set up ON_STACK inferior calls.  */

  /* DICOS rewinds the PC itself.  */
  set_gdbarch_decr_pc_after_break (gdbarch, 0);
}

/* Return true if ABFD is a dicos load module.  HEADER_SIZE is the
   expected size of the "header" section in bytes.  */

int
dicos_load_module_p (bfd *abfd, int header_size)
{
  long storage_needed;
  int ret = 0;
  asymbol **symbol_table = NULL;
  const char *symname = "Dicos_loadModuleInfo";
  asection *section;

  /* DICOS files don't have a .note.ABI-tag marker or something
     similar.  We do know there's always a "header" section of
     HEADER_SIZE bytes (size depends on architecture), and there's
     always a "Dicos_loadModuleInfo" symbol defined.  Look for the
     section first, as that should be cheaper.  */

  section = bfd_get_section_by_name (abfd, "header");
  if (!section)
    return 0;

  if (bfd_section_size (section) != header_size)
    return 0;

  /* Dicos LMs always have a "Dicos_loadModuleInfo" symbol
     defined.  Look for it.  */

  storage_needed = bfd_get_symtab_upper_bound (abfd);
  if (storage_needed < 0)
    {
      warning (_("Can't read elf symbols from %s: %s"),
	       bfd_get_filename (abfd),
	       bfd_errmsg (bfd_get_error ()));
      return 0;
    }

  if (storage_needed > 0)
    {
      long i, symcount;

      symbol_table = (asymbol **) xmalloc (storage_needed);
      symcount = bfd_canonicalize_symtab (abfd, symbol_table);

      if (symcount < 0)
	warning (_("Can't read elf symbols from %s: %s"),
		 bfd_get_filename (abfd),
		 bfd_errmsg (bfd_get_error ()));
      else
	{
	  for (i = 0; i < symcount; i++)
	    {
	      asymbol *sym = symbol_table[i];
	      if (sym->name != NULL
		  && symname[0] == sym->name[0]
		  && strcmp (symname + 1, sym->name + 1) == 0)
		{
		  ret = 1;
		  break;
		}
	    }
	}
    }

  xfree (symbol_table);
  return ret;
}
