/* Copyright 2023-2024 Free Software Foundation, Inc.

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

void
__attribute__((used))
trivial (void)
{
  asm ("trivial_label: .global trivial_label");		/* trivial function */
}

char global;

void
watch (void)
{							/* watch start */
  asm ("watch_label: .global watch_label");
  asm ("mov $0x0, %rax");
  int local = 0;					/* watch prologue */

  asm ("watch_start: .global watch_start");
  asm ("mov $0x1, %rax");
  local = 1;						/* watch assign */
  asm ("watch_reassign: .global watch_reassign");
  asm ("mov $0x2, %rax");
  local = 2;						/* watch reassign */
  asm ("watch_end: .global watch_end");			/* watch end */
}

int
main (void)
{							/* main prologue */
  asm ("main_label: .global main_label");
  global = 0;
  asm ("main_fun_call: .global main_fun_call");
  watch ();						/* main function call */
  asm ("main_epilogue: .global main_epilogue");
  global = 10;
  return 0;						/* main end */
}
