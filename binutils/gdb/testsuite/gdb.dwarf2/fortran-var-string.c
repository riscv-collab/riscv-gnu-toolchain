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

/* In the generated DWARF, we'll pretend that ARG is a string with dynamic
   length.  */
int
main_helper (void *arg)
{
  asm ("main_helper_label: .globl main_helper_label");
  return 0;
}

int
main (void)
{
  asm ("main_label: .globl main_label");
  main_helper (0);
  return 0;
}
