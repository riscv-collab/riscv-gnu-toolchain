/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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

#define LL(N) asm ("line_label_" #N ": .globl line_label_" #N)

/* Fake parameter location.  */
int global_param = 0;

int
func (int arg)
{
  asm ("func_label: .globl func_label");
  LL(4);
  return arg + 1;
}

int
main ()
{
  asm ("main_label: .globl main_label");
  LL(1);
  global_param = -1;
  LL(2);
  func (-1);
  LL(3);
  return 0;
}
