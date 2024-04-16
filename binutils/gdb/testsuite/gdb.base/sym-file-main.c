/* Copyright 2013-2024 Free Software Foundation, Inc.
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>

#include "sym-file-loader.h"

void
gdb_add_symbol_file (void *addr, const char *file)
{
  return;
}

void
gdb_remove_symbol_file (void *addr)
{
  return;
}

/* Load a shared library without relying on the standard
   loader to test GDB's commands for adding and removing
   symbol files at runtime.  */

int
main (int argc, const char *argv[])
{
  const char *file = SHLIB_NAME;
  struct library *lib;
  char *text_addr = NULL;
  int (*pbar) () = NULL;
  int (*pfoo) (int) = NULL;
  int (*pbaz) () = NULL;
  int i;

  lib = load_shlib (file);
  if (lib == NULL)
    return 1;

  if (get_text_addr (lib,  (void **) &text_addr) != 0)
    return 1;

  gdb_add_symbol_file (text_addr, file);

  /* Call bar from SHLIB_NAME.  */
  if (lookup_function (lib, "bar", (void *) &pbar) != 0)
    return 1;

  (*pbar) ();

  /* Call foo from SHLIB_NAME.  */
  if (lookup_function (lib, "foo", (void *) &pfoo) != 0)
    return 1;

  (*pfoo) (2);

  /* Unload the library, invalidating all memory breakpoints.  */
  unload_shlib (lib);

  /* Notify GDB to remove the symbol file.  Also check that GDB
     doesn't complain that it can't remove breakpoints from the
     unmapped library.  */
  gdb_remove_symbol_file (text_addr);

  /* Reload the library.  */
  lib = load_shlib (file); /* reload lib here */
  if (lib == NULL)
    return 1;

  if (get_text_addr (lib,  (void **) &text_addr) != 0)
    return 1;

  gdb_add_symbol_file (text_addr, file);

  if (lookup_function (lib, "baz", (void *) &pbaz) != 0)
    return 1;

  (*pbaz) ();

  return 0; /* end here */
}
