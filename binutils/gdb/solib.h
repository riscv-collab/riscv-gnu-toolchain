/* Shared library declarations for GDB, the GNU Debugger.

   Copyright (C) 1992-2024 Free Software Foundation, Inc.

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

#ifndef SOLIB_H
#define SOLIB_H

/* Forward decl's for prototypes */
struct shobj;
struct target_ops;
struct target_so_ops;
struct program_space;

#include "gdb_bfd.h"
#include "symfile-add-flags.h"
#include "gdbsupport/function-view.h"

/* Value of the 'set debug solib' configuration variable.  */

extern bool debug_solib;

/* Print an "solib" debug statement.  */

#define solib_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (debug_solib, "solib", fmt, ##__VA_ARGS__)

#define SOLIB_SCOPED_DEBUG_START_END(fmt, ...) \
  scoped_debug_start_end (debug_solib, "solib", fmt, ##__VA_ARGS__)

/* Called when we free all symtabs, to free the shared library information
   as well.  */

extern void clear_solib (void);

/* Called to add symbols from a shared library to gdb's symbol table.  */

extern void solib_add (const char *, int, int);
extern bool solib_read_symbols (shobj &, symfile_add_flags);

/* Function to be called when the inferior starts up, to discover the
   names of shared libraries that are dynamically linked, the base
   addresses to which they are linked, and sufficient information to
   read in their symbols at a later time.  */

extern void solib_create_inferior_hook (int from_tty);

/* If ADDR lies in a shared library, return its name.  */

extern const char *solib_name_from_address (struct program_space *, CORE_ADDR);

/* Return true if ADDR lies within SOLIB.  */

extern bool solib_contains_address_p (const shobj &, CORE_ADDR);

/* Return whether the data starting at VADDR, size SIZE, must be kept
   in a core file for shared libraries loaded before "gcore" is used
   to be handled correctly when the core file is loaded.  This only
   applies when the section would otherwise not be kept in the core
   file (in particular, for readonly sections).  */

extern bool solib_keep_data_in_core (CORE_ADDR vaddr, unsigned long size);

/* Return true if PC lies in the dynamic symbol resolution code of the
   run time loader.  */

extern bool in_solib_dynsym_resolve_code (CORE_ADDR);

/* Discard symbols that were auto-loaded from shared libraries.  */

extern void no_shared_libraries (const char *ignored, int from_tty);

/* Synchronize GDB's shared object list with inferior's.

   Extract the list of currently loaded shared objects from the
   inferior, and compare it with the list of shared objects in the
   current program space's list of shared libraries.  Edit
   so_list_head to bring it in sync with the inferior's new list.

   If we notice that the inferior has unloaded some shared objects,
   free any symbolic info GDB had read about those shared objects.

   Don't load symbolic info for any new shared objects; just add them
   to the list, and leave their symbols_loaded flag clear.

   If FROM_TTY is non-null, feel free to print messages about what
   we're doing.  */

extern void update_solib_list (int from_tty);

/* Return true if NAME is the libpthread shared library.  */

extern bool libpthread_name_p (const char *name);

/* Look up symbol from both symbol table and dynamic string table.  */

extern CORE_ADDR gdb_bfd_lookup_symbol
     (bfd *abfd, gdb::function_view<bool (const asymbol *)> match_sym);

/* Look up symbol from symbol table.  */

extern CORE_ADDR gdb_bfd_lookup_symbol_from_symtab
     (bfd *abfd, gdb::function_view<bool (const asymbol *)> match_sym);

/* Scan for DESIRED_DYNTAG in .dynamic section of ABFD.  If DESIRED_DYNTAG is
   found, 1 is returned and the corresponding PTR and PTR_ADDR are set.  */

extern int gdb_bfd_scan_elf_dyntag (const int desired_dyntag, bfd *abfd,
				    CORE_ADDR *ptr, CORE_ADDR *ptr_addr);

/* If FILENAME refers to an ELF shared object then attempt to return the
   string referred to by its DT_SONAME tag.   */

extern gdb::unique_xmalloc_ptr<char> gdb_bfd_read_elf_soname
  (const char *filename);

/* Enable or disable optional solib event breakpoints as appropriate.  */

extern void update_solib_breakpoints (void);

/* Handle an solib event by calling solib_add.  */

extern void handle_solib_event (void);

/* Associate SONAME with BUILD_ID in ABFD's registry so that it can be
   retrieved with get_cbfd_soname_build_id.  */

extern void set_cbfd_soname_build_id (gdb_bfd_ref_ptr abfd,
				      const char *soname,
				      const bfd_build_id *build_id);

#endif /* SOLIB_H */
