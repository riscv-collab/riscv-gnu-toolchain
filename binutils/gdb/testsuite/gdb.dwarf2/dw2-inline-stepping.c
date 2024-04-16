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

/* This test relies on foo being inlined into main and bar not being
   inlined.  The test is checking GDB's behaviour as we single step from
   main through foo and into bar.  */

volatile int global_var;

int  __attribute__ ((noinline))
bar ()
{						/* bar prologue */
  asm ("bar_label: .globl bar_label");
  return global_var;				/* bar return global_var */
}						/* bar end */

static inline int __attribute__ ((always_inline))
foo ()
{						/* foo prologue */
  return bar ();				/* foo call bar */
}						/* foo end */

int
main ()
{						/* main prologue */
  int ans;
  asm ("main_label: .globl main_label");
  global_var = 0;				/* main set global_var */
  asm ("main_label2: .globl main_label2");
  ans = foo ();					/* main call foo */
  asm ("main_label3: .globl main_label3");
  return ans;
}						/* main end */
