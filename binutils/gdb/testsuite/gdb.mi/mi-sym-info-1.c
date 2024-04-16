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

/* Function and variables declared in mi-sym-info-2.c.  */
extern float f2 (float arg);
extern int f3 (int arg);
extern int global_i2;
extern float global_f2;

static int global_i1;
static float __attribute__ ((used)) global_f1;

typedef int my_int_t;

static my_int_t
f1 (int arg1, int arg2)
{
  return arg1 + arg2;
}

void
f4 (int *arg)
{
  (*arg)++;
}

int
main ()
{
  int v = f3 (4);
  f4 (&v);
  float tmp = f2 (1.0);
  return f1 (3, v) + ((int) tmp);
}
