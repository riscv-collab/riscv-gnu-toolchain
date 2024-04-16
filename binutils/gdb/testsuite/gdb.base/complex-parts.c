/* Copyright 2019-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

/* Unlike the other 'complex.c' test, this one uses the "standard" header
   file to pull in the complex types.  The testing is around printing the
   complex numbers, and using the convenience function $_cimag and $_creal
   to extract the parts of the complex numbers.  */

#include <complex.h>

void
keep_around (volatile void *ptr)
{
  asm ("" ::: "memory");
}

int
main (void)
{
  double complex z1 = 1.5 + 4.5 * I;
  float complex z2 = 2.5 - 5.5 * I;
  long double complex z3 = 3.5 + 6.5 * I;

  double d1 = 1.5;
  float f1 = 2.5;
  int i1 = 3;

  keep_around (&z1);
  keep_around (&z2);
  keep_around (&z3);
  keep_around (&d1);
  keep_around (&f1);
  keep_around (&i1);

  return 0;	/* Break Here.  */
}
