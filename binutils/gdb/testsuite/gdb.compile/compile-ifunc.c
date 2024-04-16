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

typedef int (*final_t) (int arg);

int
final (int arg)
{
  return arg + 1;
}

asm (".type gnu_ifunc, %gnu_indirect_function");

final_t
gnu_ifunc (void)
{
  return final;
}

extern int gnu_ifunc_alias (int arg) __attribute__ ((alias ("gnu_ifunc")));

static int resultvar;

int
main (void)
{
  if (gnu_ifunc_alias (10) != 11)
    abort ();
  return resultvar;
}
