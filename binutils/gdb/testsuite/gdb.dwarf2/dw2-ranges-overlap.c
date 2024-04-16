/*
   Copyright 2020-2024 Free Software Foundation, Inc.

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
bar (int a)
{
  asm ("bar_label: .globl bar_label");
  return a + 1;
}

int
foo (int a)
{
  asm ("foo_label: .globl foo_label");
  return bar (a * 2) + 3;
}

int
quux (int a)
{
  asm ("quux_label: .globl quux_label");
  return foo (a);
}

int
main (void)
{
  asm ("main_label: .globl main_label");
  return quux (5) + 1;
}
