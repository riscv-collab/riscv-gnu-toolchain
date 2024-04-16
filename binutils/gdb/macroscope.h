/* Interface to functions for deciding which macros are currently in scope.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.
   Contributed by Red Hat, Inc.

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

#ifndef MACROSCOPE_H
#define MACROSCOPE_H

#include "macrotab.h"
#include "symtab.h"


/* The table of macros defined by the user.  */
extern struct macro_table *macro_user_macros;

/* All the information we need to decide which macro definitions are
   in scope: a source file (either a main source file or an
   #inclusion), and a line number in that file.  */
struct macro_scope {
  struct macro_source_file *file;
  int line;
};


/* Return a `struct macro_scope' object corresponding to the symtab
   and line given in SAL.  If we have no macro information for that
   location, or if SAL's pc is zero, return zero.  */
gdb::unique_xmalloc_ptr<struct macro_scope> sal_macro_scope
    (struct symtab_and_line sal);


/* Return a `struct macro_scope' object representing just the
   user-defined macros.  */
gdb::unique_xmalloc_ptr<struct macro_scope> user_macro_scope (void);

/* Return a `struct macro_scope' object describing the scope the `macro
   expand' and `macro expand-once' commands should use for looking up
   macros.  If we have a selected frame, this is the source location of
   its PC; otherwise, this is the last listing position.

   If we have no macro information for the current location, return
   the user macro scope.  */
gdb::unique_xmalloc_ptr<struct macro_scope> default_macro_scope (void);

/* Look up the definition of the macro named NAME in scope at the source
   location given by MS.  */
macro_definition *standard_macro_lookup (const char *name,
					 const macro_scope &ms);

#endif /* MACROSCOPE_H */
