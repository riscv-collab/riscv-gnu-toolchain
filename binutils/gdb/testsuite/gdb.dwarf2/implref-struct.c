/* Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

/* Test program for synthetic C++ references to structs.  */

struct S {
  int a;
  int b;
  int c;
};

struct S s1 = {
  0,
  1,
  2
};

struct S s2 = {
  10,
  11,
  12
};

int
main (void)
{
  asm ("main_label: .globl main_label");
  return 0;
}
