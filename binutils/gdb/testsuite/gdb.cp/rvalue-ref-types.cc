/* This test script is part of GDB, the GNU debugger.

   Copyright 1999-2024 Free Software Foundation, Inc.

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

/* Tests for reference types with short type variables in GDB, based on
   gdb.cp/ref-types.cc.  */

#include <utility>

int main2 ();

void
marker1 ()
{
}

int
main ()
{
  short t = -1;
  short *pt;
  short &&rrt = std::move (t);
  pt = &rrt;

  short *&&rrpt = std::move (pt);
  short at[4];
  at[0] = 0;
  at[1] = 1;
  at[2] = 2;
  at[3] = 3;

  short (&&rrat)[4] = std::move( at);

  marker1();

  main2();

  return 0;
}

int
f ()
{
  int f1;
  f1 = 1;
  return f1;
}

int
main2 ()
{
  char &&rrC = 'A';
  unsigned char &&rrUC = 21;
  short &&rrS = -14;
  unsigned short &&rrUS = 7;
  int &&rrI = 102;
  unsigned int &&rrUI = 1002;
  long &&rrL = -234;
  unsigned long &&rrUL = 234;
  float &&rrF = 1.25E10;
  double &&rrD = -1.375E-123;

  f ();

  return 0;
}
