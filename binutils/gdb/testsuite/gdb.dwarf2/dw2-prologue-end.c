/* Copyright 2022-2024 Free Software Foundation, Inc.

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


int
main (void)
{							/* main prologue */
  asm ("main_label: .global main_label");
  int m = 42;						/* main assign m */
  asm ("main_assign_n: .global main_assign_n");
  int n = 54;						/* main assign n */
  asm ("main_end_prologue: .global main_end_prologue");
  int o = 96;						/* main assign o */
  asm ("main_end: .global main_end");			/* main end */
  return m + n - o;
}
