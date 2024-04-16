/* Legacy support routines for building symbol tables in GDB's internal format.
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

#include "defs.h"
#include "buildsym-legacy.h"
#include "symtab.h"

/* The work-in-progress of the compunit we are building.
   This is created first, before any subfiles by start_compunit_symtab.  */

static struct buildsym_compunit *buildsym_compunit;

void
record_debugformat (const char *format)
{
  buildsym_compunit->record_debugformat (format);
}

void
record_producer (const char *producer)
{
  buildsym_compunit->record_producer (producer);
}



/* See buildsym.h.  */

void
set_last_source_file (const char *name)
{
  gdb_assert (buildsym_compunit != nullptr || name == nullptr);
  if (buildsym_compunit != nullptr)
    buildsym_compunit->set_last_source_file (name);
}

/* See buildsym.h.  */

const char *
get_last_source_file ()
{
  if (buildsym_compunit == nullptr)
    return nullptr;
  return buildsym_compunit->get_last_source_file ();
}

/* See buildsym.h.  */

void
set_last_source_start_addr (CORE_ADDR addr)
{
  gdb_assert (buildsym_compunit != nullptr);
  buildsym_compunit->set_last_source_start_addr (addr);
}

/* See buildsym.h.  */

CORE_ADDR
get_last_source_start_addr ()
{
  gdb_assert (buildsym_compunit != nullptr);
  return buildsym_compunit->get_last_source_start_addr ();
}

/* See buildsym.h.  */

bool
outermost_context_p ()
{
  gdb_assert (buildsym_compunit != nullptr);
  return buildsym_compunit->outermost_context_p ();
}

/* See buildsym.h.  */

int
get_context_stack_depth ()
{
  gdb_assert (buildsym_compunit != nullptr);
  return buildsym_compunit->get_context_stack_depth ();
}

/* See buildsym.h.  */

struct subfile *
get_current_subfile ()
{
  gdb_assert (buildsym_compunit != nullptr);
  return buildsym_compunit->get_current_subfile ();
}

/* See buildsym.h.  */

struct pending **
get_local_symbols ()
{
  gdb_assert (buildsym_compunit != nullptr);
  return buildsym_compunit->get_local_symbols ();
}

/* See buildsym.h.  */

struct pending **
get_file_symbols ()
{
  gdb_assert (buildsym_compunit != nullptr);
  return buildsym_compunit->get_file_symbols ();
}

/* See buildsym.h.  */

struct pending **
get_global_symbols ()
{
  gdb_assert (buildsym_compunit != nullptr);
  return buildsym_compunit->get_global_symbols ();
}

void
start_subfile (const char *name)
{
  gdb_assert (buildsym_compunit != nullptr);
  buildsym_compunit->start_subfile (name);
}

void
patch_subfile_names (struct subfile *subfile, const char *name)
{
  gdb_assert (buildsym_compunit != nullptr);
  buildsym_compunit->patch_subfile_names (subfile, name);
}

void
push_subfile ()
{
  gdb_assert (buildsym_compunit != nullptr);
  buildsym_compunit->push_subfile ();
}

const char *
pop_subfile ()
{
  gdb_assert (buildsym_compunit != nullptr);
  return buildsym_compunit->pop_subfile ();
}

/* Delete the buildsym compunit.  */

static void
free_buildsym_compunit (void)
{
  if (buildsym_compunit == NULL)
    return;
  delete buildsym_compunit;
  buildsym_compunit = NULL;
}

struct compunit_symtab *
end_compunit_symtab (CORE_ADDR end_addr)
{
  gdb_assert (buildsym_compunit != nullptr);
  struct compunit_symtab *result
    = buildsym_compunit->end_compunit_symtab (end_addr);
  free_buildsym_compunit ();
  return result;
}

struct context_stack *
push_context (int desc, CORE_ADDR valu)
{
  gdb_assert (buildsym_compunit != nullptr);
  return buildsym_compunit->push_context (desc, valu);
}

struct context_stack
pop_context ()
{
  gdb_assert (buildsym_compunit != nullptr);
  return buildsym_compunit->pop_context ();
}

struct block *
finish_block (struct symbol *symbol, struct pending_block *old_blocks,
	      const struct dynamic_prop *static_link,
	      CORE_ADDR start, CORE_ADDR end)
{
  gdb_assert (buildsym_compunit != nullptr);
  return buildsym_compunit->finish_block (symbol, old_blocks, static_link,
					  start, end);
}

void
record_line (struct subfile *subfile, int line, unrelocated_addr pc)
{
  gdb_assert (buildsym_compunit != nullptr);
  /* Assume every line entry is a statement start, that is a good place to
     put a breakpoint for that line number.  */
  buildsym_compunit->record_line (subfile, line, pc, LEF_IS_STMT);
}

/* Start a new compunit_symtab for a new source file in OBJFILE.  Called, for
   example, when a stabs symbol of type N_SO is seen, or when a DWARF
   DW_TAG_compile_unit DIE is seen.  It indicates the start of data for one
   original source file.

   NAME is the name of the file (cannot be NULL).  COMP_DIR is the
   directory in which the file was compiled (or NULL if not known).
   START_ADDR is the lowest address of objects in the file (or 0 if
   not known).  LANGUAGE is the language of the source file, or
   language_unknown if not known, in which case it'll be deduced from
   the filename.  */

struct compunit_symtab *
start_compunit_symtab (struct objfile *objfile, const char *name,
		       const char *comp_dir, CORE_ADDR start_addr,
		       enum language language)
{
  /* These should have been reset either by successful completion of building
     a symtab, or by the scoped_free_pendings destructor.  */
  gdb_assert (buildsym_compunit == nullptr);

  buildsym_compunit = new struct buildsym_compunit (objfile, name, comp_dir,
						    language, start_addr);

  return buildsym_compunit->get_compunit_symtab ();
}

/* At end of reading syms, or in case of quit, ensure everything
   associated with building symtabs is freed.

   N.B. This is *not* intended to be used when building psymtabs.  Some debug
   info readers call this anyway, which is harmless if confusing.  */

scoped_free_pendings::~scoped_free_pendings ()
{
  free_buildsym_compunit ();
}

/* See buildsym-legacy.h.  */

struct buildsym_compunit *
get_buildsym_compunit ()
{
  gdb_assert (buildsym_compunit != nullptr);
  return buildsym_compunit;
}
