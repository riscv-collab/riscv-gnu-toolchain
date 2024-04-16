/* Copyright (C) 2013-2024 Free Software Foundation, Inc.

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

/* This file is not actually compiled, the .S file is committed alongside
   this file.  The reason is that changes to the compiler might result
   in different debug information being created, this could break the
   test.  */

struct str
{
  int a;
  int b;
  int c : 3;
  int d : 3;
};

int __attribute__ ((noinline))
foo (int arg)
{  
  return arg;
}

int
main ( void )
{
  struct str s = {5, 7, 1, 2};
  int v;

  v = (s.a << 1);
  v += foo (v);
  return v + 5 + s.a;
}

