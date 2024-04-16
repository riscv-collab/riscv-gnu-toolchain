/* This testcase is part of GDB, the GNU debugger.

   Copyright 1992-2024 Free Software Foundation, Inc.

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
/*  Test "return" command.  */

void func1 ()
{
  printf("in func1\n");
}

int
func2 ()
{
  return -5;
}

double
func3 ()
{
  return -5.0;
}

int tmp2;
double tmp3;

int main ()
{
  func1 ();
  printf("in main after func1\n");
  tmp2 = func2 ();
  tmp3 = func3 ();
  printf("exiting\n");
  return 0;
}
