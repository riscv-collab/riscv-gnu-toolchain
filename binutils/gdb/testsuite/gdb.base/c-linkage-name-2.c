/* This testcase is part of GDB, the GNU debugger.

   Copyright 2018-2024 Free Software Foundation, Inc.

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

struct wrapper
{
  int a;
};

/* Create a global variable whose name in the assembly code
   (aka the "linkage name") is different from the name in
   the source code.  The goal is to create a symbol described
   in DWARF using a DW_AT_linkage_name attribute, with a name
   which follows the C++ mangling.

   In this particular case, we chose "symada__cS" which, if it were
   demangled, would translate to "symada (char, signed)".  */
struct wrapper mundane asm ("symada__cS") = {0x060287af};

void
do_something (struct wrapper *w)
{
  w->a++;
}

extern void do_something_other_cu (void);

void
do_something_other_cu (void)
{
  do_something (&mundane);
}
