/* Copyright (C) 2016-2024 Free Software Foundation, Inc.

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
#include "objfiles.h"
#include "arm-tdep.h"
#include "osabi.h"

/* The gdbarch_register_osabi handler for ARM PikeOS; it performs
   the gdbarch initialization for that platform.  */

static void
arm_pikeos_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  /* Single stepping.  */
  set_gdbarch_software_single_step (gdbarch, arm_software_single_step);
}

/* The ARM PikeOS OSABI sniffer (see gdbarch_register_osabi_sniffer).
   Returns GDB_OSABI_PIKEOS if the given BFD is a PikeOS binary,
   GDB_OSABI_UNKNOWN otherwise.  */

static enum gdb_osabi
arm_pikeos_osabi_sniffer (bfd *abfd)
{
  long number_of_symbols;
  long i;
  int pikeos_stack_found = 0;
  int pikeos_stack_size_found = 0;

  /* The BFD target of PikeOS is really just standard elf, so we
     cannot use it to detect this variant.  The only common thing that
     may be found in PikeOS modules are symbols _vm_stack/__p4_stack and
     _vm_stack_size/__p4_stack_end. They are used to specify the stack
     location and size; and defined by the default linker script.

     OS ABI sniffers are called before the minimal symtabs are
     created. So inspect the symbol table using BFD.  */

  long storage_needed = bfd_get_symtab_upper_bound (abfd);
  if (storage_needed <= 0)
    return GDB_OSABI_UNKNOWN;

  gdb::unique_xmalloc_ptr<asymbol *> symbol_table
    ((asymbol **) xmalloc (storage_needed));
  number_of_symbols = bfd_canonicalize_symtab (abfd, symbol_table.get ());

  if (number_of_symbols <= 0)
    return GDB_OSABI_UNKNOWN;

  for (i = 0; i < number_of_symbols; i++)
    {
      const char *name = bfd_asymbol_name (symbol_table.get ()[i]);

      if (strcmp (name, "_vm_stack") == 0
	  || strcmp (name, "__p4_stack") == 0)
	pikeos_stack_found = 1;

      if (strcmp (name, "_vm_stack_size") == 0
	  || strcmp (name, "__p4_stack_end") == 0)
	pikeos_stack_size_found = 1;
    }

  if (pikeos_stack_found && pikeos_stack_size_found)
    return GDB_OSABI_PIKEOS;
  else
    return GDB_OSABI_UNKNOWN;
}

void _initialize_arm_pikeos_tdep ();
void
_initialize_arm_pikeos_tdep ()
{
  /* Register the sniffer for the PikeOS targets.  */
  gdbarch_register_osabi_sniffer (bfd_arch_arm, bfd_target_elf_flavour,
				  arm_pikeos_osabi_sniffer);
  gdbarch_register_osabi (bfd_arch_arm, 0, GDB_OSABI_PIKEOS,
			  arm_pikeos_init_abi);
}
