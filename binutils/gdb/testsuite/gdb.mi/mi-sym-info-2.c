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

static int global_i1;
static float __attribute__ ((used)) global_f1;
int global_i2;
int global_f2;

typedef int another_int_t;
typedef float another_float_t;

static another_float_t
f1 (int arg)
{
  return (float) arg;
}

float
f2 (another_float_t arg)
{
  return arg + f1 (1);
}

int
f3 (another_int_t arg)
{
  return arg + 2;
}

typedef char another_char_t;
typedef short another_short_t;

static another_char_t __attribute__ ((used)) var1;
static another_short_t __attribute__ ((used)) var2;
