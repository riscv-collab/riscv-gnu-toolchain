/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <http://www.gnu.org/licenses/>.  */

#ifdef __GNUC__
#define ATTR __attribute__((always_inline))
#else
#define ATTR
#endif

int global_num = 0;
int global_value = 0;

inline ATTR
static void
func ()
{ /* func prologue */
  global_num = 42;
  int num = 42;
  if (num > 2)
    {
      asm ("scope_label1: .globl scope_label1");
      global_value = num;
      int value = num;
      asm ("breakpoint_label: .globl breakpoint_label");
      global_value += value;
      asm ("scope_label2: .globl scope_label2");
    }
} /* func end */

int
main ()
{ /* main prologue */
  asm ("main_label: .globl main_label");
  func (); /* func call */
  asm ("main_label2: .globl main_label2");
  return 0; /* main return */
} /* main end */
