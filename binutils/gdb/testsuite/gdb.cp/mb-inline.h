/* Test gdb support for setting multiple file:line breakpoints on static
   functions.  In practice the functions may be inline fns compiled with -O0.
   We avoid using inline here for simplicity's sake.

   This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

static int
foo (int i)
{
  return i; // set breakpoint here
}

static int
multi_line_foo (int i)
{
  return // set multi-line breakpoint here
    i;
}

extern int afn ();
extern int bfn ();
