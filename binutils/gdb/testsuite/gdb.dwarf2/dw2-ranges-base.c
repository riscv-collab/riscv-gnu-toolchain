/*
   Copyright 2015-2024 Free Software Foundation, Inc.

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

void __attribute__ ((section (".text.3")))
frame3 (void)
{
  asm ("frame3_label: .globl frame3_label");
}

void __attribute__ ((section (".text.2")))
frame2 (void)
{
  asm ("frame2_label: .globl frame2_label");
  frame3 ();
}

void __attribute__ ((section (".text.1")))
main (void)
{
  asm ("main_label: .globl main_label");
  frame2 ();
}

