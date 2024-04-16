/* This test program is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

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

#include "solib-search.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE 1
#endif

const int lib2_array[ARRAY_SIZE] = { 42 };
const int lib2_size = ARRAY_SIZE;

void
lib2_func2 (void)
{
  lib1_func3 ();
}

/* Make the relative address of func4 different b/w the "wrong" and "right"
   versions of the library" to further ensure backtrace doesn't work with
   the "wrong" version.  */

#ifdef RIGHT

void
lib2_spacer (void)
{
  int i;
  for (i = 0; i < 10; ++i)
    ;
}

#endif

void
lib2_func4 (void)
{
  break_here ();
}
