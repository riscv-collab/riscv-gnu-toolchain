/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

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

int my_global_symbol = 42;

/* Symbol MY_BSS_SYMBOL is referenced, and should be placed into .bss
   section.  */

static int my_bss_symbol;

/* Symbol MY_STATIC_SYMBOL is never referenced and so will be eliminated.  */

static int my_static_symbol;

int
main ()
{
  int v_in_main;

  return v_in_main + my_bss_symbol;
}

int
my_global_func ()
{
  int v_in_global_func;

  return v_in_global_func;
}
