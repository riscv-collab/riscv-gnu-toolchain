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

/* In the generated DWARF, we'll use the locations of foo_entry_label and
   foobar_entry_label as the low_pc's of our entry point TAGs.  */

int I = 0;
int J = 0;
int K = 0;

__attribute__ ((noinline))
void
bar_helper (void)
{
  asm ("bar_helper_label: .globl bar_helper_label");
  I++;
  J++;
  asm ("foo_entry_label: .globl foo_entry_label");
  J++;
  K++;
  asm ("foobar_entry_label: .globl foobar_entry_label");
}

int
main (void)
{
  asm ("main_label: .globl main_label");
  bar_helper ();

  return 0;
}
