/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

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

/* This program will be compiled with watch-notconst2.S in order to generate a
   single binary.

   The purpose of this test is to see if GDB can still watch the
   variable `x' (define in watch-notconst2.c:f) even when we compile
   the program using -O2 optimization.  */

int
g (int j)
{
  int l = j + 2;
  return l;
}

extern int f (int i);

int
main (int argc, char **argv)
{
  f (1);
  return 0;
}
