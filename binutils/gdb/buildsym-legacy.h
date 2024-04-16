/* Build symbol tables in GDB's internal format - legacy APIs
   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#ifndef BUILDSYM_LEGACY_H
#define BUILDSYM_LEGACY_H

#include "buildsym.h"

/* This module provides definitions used for creating and adding to
   the symbol table.  These routines are called from various symbol-
   file-reading routines.  This file holds the legacy API, which
   relies on a global variable to work properly.  New or maintained
   symbol readers should use the builder API in buildsym.h.

   The basic way this module is used is as follows:

   scoped_free_pendings free_pending;
   cust = start_compunit_symtab (...);
   ... read debug info ...
   cust = end_compunit_symtab (...);

   The compunit symtab pointer ("cust") is returned from both
   start_compunit_symtab and end_compunit_symtab to simplify the debug info readers.

   dbxread.c and xcoffread.c use another variation:

   scoped_free_pendings free_pending;
   cust = start_compunit_symtab (...);
   ... read debug info ...
   cust = end_compunit_symtab (...);
   ... start_compunit_symtab + read + end_compunit_symtab repeated ...
*/

class scoped_free_pendings
{
public:

  scoped_free_pendings () = default;
  ~scoped_free_pendings ();

  DISABLE_COPY_AND_ASSIGN (scoped_free_pendings);
};

extern struct block *finish_block (struct symbol *symbol,
				   struct pending_block *old_blocks,
				   const struct dynamic_prop *static_link,
				   CORE_ADDR start,
				   CORE_ADDR end);

extern void start_subfile (const char *name);

extern void patch_subfile_names (struct subfile *subfile, const char *name);

extern void push_subfile ();

extern const char *pop_subfile ();

extern struct compunit_symtab *end_compunit_symtab (CORE_ADDR end_addr);

extern struct context_stack *push_context (int desc, CORE_ADDR valu);

extern struct context_stack pop_context ();

extern void record_line (struct subfile *subfile, int line,
			 unrelocated_addr pc);

extern struct compunit_symtab *start_compunit_symtab (struct objfile *objfile,
						      const char *name,
						      const char *comp_dir,
						      CORE_ADDR start_addr,
						      enum language language);

/* Record the name of the debug format in the current pending symbol
   table.  FORMAT must be a string with a lifetime at least as long as
   the symtab's objfile.  */

extern void record_debugformat (const char *format);

/* Record the name of the debuginfo producer (usually the compiler) in
   the current pending symbol table.  PRODUCER must be a string with a
   lifetime at least as long as the symtab's objfile.  */

extern void record_producer (const char *producer);

/* Set the name of the last source file.  NAME is copied by this
   function.  */

extern void set_last_source_file (const char *name);

/* Fetch the name of the last source file.  */

extern const char *get_last_source_file (void);

/* Set the last source start address.  Can only be used between
   start_compunit_symtab and end_compunit_symtab* calls.  */

extern void set_last_source_start_addr (CORE_ADDR addr);

/* Get the last source start address.  Can only be used between
   start_compunit_symtab and end_compunit_symtab* calls.  */

extern CORE_ADDR get_last_source_start_addr ();

/* True if the context stack is empty.  */

extern bool outermost_context_p ();

/* Return the context stack depth.  */

extern int get_context_stack_depth ();

/* Return the current subfile.  */

extern struct subfile *get_current_subfile ();

/* Return the local symbol list.  */

extern struct pending **get_local_symbols ();

/* Return the file symbol list.  */

extern struct pending **get_file_symbols ();

/* Return the global symbol list.  */

extern struct pending **get_global_symbols ();

/* Return the current buildsym_compunit.  */

extern struct buildsym_compunit *get_buildsym_compunit ();

#endif /* BUILDSYM_LEGACY_H */
