/* This testcase is part of GDB, the GNU debugger.

   Copyright 2006-2024 Free Software Foundation, Inc.

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

#include <stdio.h>

void
hello (void)
{
  printf ("Hello world.\n");
}

/* The test case uses "break *hello" to make sure to step at the very
   first instruction of the function.  This causes a problem running
   the test on powerpc64le-linux, since the first instruction belongs
   to the global entry point prologue, which is skipped when doing a
   local direct function call.  To make sure that first instruction is
   indeed being executed and the breakpoint hits, we make sure to call
   the routine via an indirect call.  */
void (*ptr) (void) = hello;

int
main (void)
{
  ptr ();

  return 0;
}
