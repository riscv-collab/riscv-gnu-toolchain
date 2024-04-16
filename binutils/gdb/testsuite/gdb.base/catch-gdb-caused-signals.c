/* This testcase is part of GDB, the GNU debugger.

   Copyright 2015-2024 Free Software Foundation, Inc.

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

/* This program is intended to be a simple dummy program for gdb to read.  */

#include <unistd.h>
#include <stdio.h>

#include "unbuffer_output.c"

int
main (void)
{
  int i = 0;

  gdb_unbuffer_output ();

  i++; /* set dprintf here */
  return 0; /* set breakpoint here */
}

int
return_one (void)
{
  return 1;
}
