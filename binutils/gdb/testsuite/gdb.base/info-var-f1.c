/* Copyright 2019-2024 Free Software Foundation, Inc.

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

#include "info-var.h"

/* Some array variables.  */
int * const foo_1[3];
const int *foo_2[3];
int *foo_3[3];
int const foo_4[3];
const int foo_5[3];
int foo_6[3];

static int f1_var = -3;

int
main ()
{
  return global_var + get_offset() + f1_var;
}
