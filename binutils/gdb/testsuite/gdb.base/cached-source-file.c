/* This testcase is part of GDB, the GNU debugger.

   Copyright 2020-2024 Free Software Foundation, Inc.

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

/* Test for PR tui/25126.

   The bug is about a regression that makes GDB not reload its source
   code cache when the inferior's symbols are reloaded, which leads to
   wrong backtraces/listings.

   This bug is reproducible even without using the TUI.

   The .exp testcase depends on the line numbers and contents from
   this file  If you change this file, make sure to double-check the
   testcase.  */

#include <stdio.h>

void
foo (void)
{
  printf ("hello\n"); /* break-here */
}

int
main ()
{
  foo ();
  return 0;
}
