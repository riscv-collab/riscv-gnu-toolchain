/* Shared library declarations for GDB, the GNU Debugger.
   Copyright (C) 1990-2024 Free Software Foundation, Inc.

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

#ifndef SOLIST_H
#define SOLIST_H

#define SO_NAME_MAX_PATH_SIZE 512	/* FIXME: Should be dynamic */
/* For domain_enum domain.  */
#include "symtab.h"
#include "gdb_bfd.h"
#include "target-section.h"

/* Base class for target-specific link map information.  */

struct lm_info
{
  lm_info () = default;
  lm_info (const lm_info &) = default;
  virtual ~lm_info () = 0;
};

using lm_info_up = std::unique_ptr<lm_info>;

struct shobj : intrusive_list_node<shobj>
{
  /* Free symbol-file related contents of SO and reset for possible reloading
     of SO.  If we have opened a BFD for SO, close it.  If we have placed SO's
     sections in some target's section table, the caller is responsible for
     removing them.

     This function doesn't mess with objfiles at all.  If there is an
     objfile associated with SO that needs to be removed, the caller is
     responsible for taking care of that.  */
  void clear () ;

  /* The following fields of the structure come directly from the
     dynamic linker's tables in the inferior, and are initialized by
     current_sos.  */

  /* A pointer to target specific link map information.  Often this
     will be a copy of struct link_map from the user process, but
     it need not be; it can be any collection of data needed to
     traverse the dynamic linker's data structures.  */
  lm_info_up lm_info;

  /* Shared object file name, exactly as it appears in the
     inferior's link map.  This may be a relative path, or something
     which needs to be looked up in LD_LIBRARY_PATH, etc.  We use it
     to tell which entries in the inferior's dynamic linker's link
     map we've already loaded.  */
  std::string so_original_name;

  /* Shared object file name, expanded to something GDB can open.  */
  std::string so_name;

  /* The following fields of the structure are built from
     information gathered from the shared object file itself, and
     are set when we actually add it to our symbol tables.

     current_sos must initialize these fields to 0.  */

  gdb_bfd_ref_ptr abfd;
  char symbols_loaded = 0;	/* flag: symbols read in yet?  */

  /* objfile with symbols for a loaded library.  Target memory is read from
     ABFD.  OBJFILE may be NULL either before symbols have been loaded, if
     the file cannot be found or after the command "nosharedlibrary".  */
  struct objfile *objfile = nullptr;

  std::vector<target_section> sections;

  /* Record the range of addresses belonging to this shared library.
     There may not be just one (e.g. if two segments are relocated
     differently).  This is used for "info sharedlibrary" and
     the MI command "-file-list-shared-libraries".  The latter has a format
     that supports outputting multiple segments once the related code
     supports them.  */
  CORE_ADDR addr_low = 0, addr_high = 0;
};

struct target_so_ops
{
  /* Adjust the section binding addresses by the base address at
     which the object was actually mapped.  */
  void (*relocate_section_addresses) (shobj &so, target_section *);

  /* Reset private data structures associated with SO.
     This is called when SO is about to be reloaded.
     It is also called when SO is about to be freed.  */
  void (*clear_so) (const shobj &so);

  /* Free private data structures associated to PSPACE.  This method
     should not free resources associated to individual so_list entries,
     those are cleared by the clear_so method.  */
  void (*clear_solib) (program_space *pspace);

  /* Target dependent code to run after child process fork.  */
  void (*solib_create_inferior_hook) (int from_tty);

  /* Construct a list of the currently loaded shared objects.  This
     list does not include an entry for the main executable file.

     Note that we only gather information directly available from the
     inferior --- we don't examine any of the shared library files
     themselves.  The declaration of `struct shobj' says which fields
     we provide values for.  */
  intrusive_list<shobj> (*current_sos) ();

  /* Find, open, and read the symbols for the main executable.  If
     FROM_TTY is non-zero, allow messages to be printed.  */
  int (*open_symbol_file_object) (int from_ttyp);

  /* Determine if PC lies in the dynamic symbol resolution code of
     the run time loader.  */
  int (*in_dynsym_resolve_code) (CORE_ADDR pc);

  /* Find and open shared library binary file.  */
  gdb_bfd_ref_ptr (*bfd_open) (const char *pathname);

  /* Optional extra hook for finding and opening a solib.
     If TEMP_PATHNAME is non-NULL: If the file is successfully opened a
     pointer to a malloc'd and realpath'd copy of SONAME is stored there,
     otherwise NULL is stored there.  */
  int (*find_and_open_solib) (const char *soname,
			      unsigned o_flags,
			      gdb::unique_xmalloc_ptr<char> *temp_pathname);

  /* Given two so_list objects, one from the GDB thread list
     and another from the list returned by current_sos, return 1
     if they represent the same library.
     Falls back to using strcmp on so_original_name field when set
     to NULL.  */
  int (*same) (const shobj &gdb, const shobj &inferior);

  /* Return whether a region of memory must be kept in a core file
     for shared libraries loaded before "gcore" is used to be
     handled correctly when the core file is loaded.  This only
     applies when the section would otherwise not be kept in the
     core file (in particular, for readonly sections).  */
  int (*keep_data_in_core) (CORE_ADDR vaddr,
			    unsigned long size);

  /* Enable or disable optional solib event breakpoints as
     appropriate.  This should be called whenever
     stop_on_solib_events is changed.  This pointer can be
     NULL, in which case no enabling or disabling is necessary
     for this target.  */
  void (*update_breakpoints) (void);

  /* Target-specific processing of solib events that will be
     performed before solib_add is called.  This pointer can be
     NULL, in which case no specific preprocessing is necessary
     for this target.  */
  void (*handle_event) (void);
};

/* A unique pointer to a so_list.  */
using shobj_up = std::unique_ptr<shobj>;

/* Find main executable binary file.  */
extern gdb::unique_xmalloc_ptr<char> exec_file_find (const char *in_pathname,
						     int *fd);

/* Find shared library binary file.  */
extern gdb::unique_xmalloc_ptr<char> solib_find (const char *in_pathname,
						 int *fd);

/* Open BFD for shared library file.  */
extern gdb_bfd_ref_ptr solib_bfd_fopen (const char *pathname, int fd);

/* Find solib binary file and open it.  */
extern gdb_bfd_ref_ptr solib_bfd_open (const char *in_pathname);

#endif
