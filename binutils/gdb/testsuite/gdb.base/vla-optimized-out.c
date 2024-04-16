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

/* The version of this test-case with f1 tagged with noinline only is equivalent
   to gcc/testsuite/gcc.dg/guality/vla-1.c.  */

#include "attributes.h"

int
#ifdef NOCLONE
__attribute__((noinline,weak)) ATTRIBUTE_NOCLONE
#else
__attribute__((noinline,weak))
#endif
f1 (int i)
{
  char a[i + 1];
  a[0] = 5;
  return a[0];
}

int
main (void)
{
  volatile int j;
  int i = 5;
  asm volatile ("" : "=r" (i) : "0" (i));
  j = f1 (i);
  return 0;
}
