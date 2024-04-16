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

void
foo (int x)
{

}

void
bar1 (void)
{
  asm ("bar1_label: .globl bar1_label");
  foo (1);
  asm ("bar1_label_2: .globl bar1_label_2");
  foo (2);
  asm ("bar1_label_3: .globl bar1_label_3");
  foo (3);
  asm ("bar1_label_4: .globl bar1_label_4");
  foo (4);
  asm ("bar1_label_5: .globl bar1_label_5");
}

void
bar2 (void)
{
  asm ("bar2_label: .globl bar2_label");
  foo (1);
  asm ("bar2_label_2: .globl bar2_label_2");
  foo (2);
  asm ("bar2_label_3: .globl bar2_label_3");
  foo (3);
  asm ("bar2_label_4: .globl bar2_label_4");
  foo (4);
  asm ("bar2_label_5: .globl bar2_label_5");
}

int
main (void)
{
  asm ("main_label: .globl main_label");

  bar1 ();

  bar2 ();

  return 0;
}

