/* Copyright 2020-2024 Free Software Foundation, Inc.

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
#include <complex.h>
#include <string.h>

struct some_struct
{
  float a, b;
};

/* See https://gcc.gnu.org/onlinedocs/gfortran/\
     Argument-passing-conventions.html.  */
#if !defined (__GNUC__) || __GNUC__ > 7
typedef size_t fortran_charlen_t;
#else
typedef int fortran_charlen_t;
#endif

extern void mixed_func_1d_ (int *, float *, double *, complex float *,
			    char *, fortran_charlen_t);

void
mixed_func_1c (int a, float b, double c, complex float d, char *f,
	       struct some_struct *g)
{
  printf ("a = %d, b = %f, c = %e, d = (%f + %fi)\n", a, b, c,
	  creal(d), cimag(d));

  char *string = "this is a string from C";
  mixed_func_1d_ (&a, &b, &c, &d, string, strlen (string));
}
