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

#include <stdlib.h>

typedef struct foo
{
  int a;
  int b;
} foo;

static foo *foo_array = NULL;

int
main (void)
{
  foo_array = (foo *) calloc (3, sizeof (*foo_array));
  foo_array[1].a = 10;
  foo_array[2].b = 20;
  return 0; /* BREAK */
}
