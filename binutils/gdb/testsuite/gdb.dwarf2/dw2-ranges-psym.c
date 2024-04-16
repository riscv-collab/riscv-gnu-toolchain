/* Copyright 2018-2024 Free Software Foundation, Inc.

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

volatile int e = 0;

void
baz (void)
{
  asm ("baz_label: .globl baz_label");
}						/* baz end */

void
foo_low (void)
{						/* foo_low prologue */
  asm ("foo_low_label: .globl foo_low_label");
  baz ();					/* foo_low baz call */
  asm ("foo_low_label2: .globl foo_low_label2");
}						/* foo_low end */

void
bar (void)
{
  asm ("bar_label: .globl bar_label");
}						/* bar end */

void
foo (void)
{						/* foo prologue */
  asm ("foo_label: .globl foo_label");
  bar ();					/* foo bar call */
  asm ("foo_label2: .globl foo_label2");
  if (e) foo_low ();				/* foo foo_low call */
  asm ("foo_label3: .globl foo_label3");
}						/* foo end */
