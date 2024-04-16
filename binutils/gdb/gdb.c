/* Main function for CLI gdb.  
   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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
#include "main.h"
#include "interps.h"
#include "run-on-main-thread.h"

int
main (int argc, char **argv)
{
  /* The first call to is_main_thread () should be from the main thread.
     If this is the first call, then that requirement is fulfilled here.
     If this is not the first call, then this verifies that the first call
     fulfilled that requirement.  */
  gdb_assert (is_main_thread ());

  struct captured_main_args args;

  memset (&args, 0, sizeof args);
  args.argc = argc;
  args.argv = argv;
  args.interpreter_p = INTERP_CONSOLE;
  return gdb_main (&args);
}
