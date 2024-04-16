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

/* The idea here is to, via use of the dwarf assembler, create a function
   which occupies two non-contiguous address ranges.

   foo_cold and foo will be combined into a single function foo with a
   function bar in between these two ranges.

   This test case was motivated by a bug in which a function which
   occupied two non-contiguous address ranges was calling another
   function which resides in between these ranges.  So we end up with
   a situation in which the low/start address of our constructed foo
   (in this case) will be less than any of the addresses in bar, but
   the high/end address of foo will be greater than any of bar's
   addresses.

   This situation was causing a problem in the caching code of
   find_pc_partial_function:  When the low and high addresses of foo
   are placed in the cache, the simple check that was used to see if
   the cache was applicable would (incorrectly) succeed when presented
   with an address in bar.  I.e. an address in bar presented as an
   input to find_pc_partial_function could produce the answer "this
   address belongs to foo".  */

volatile int e = 0;

void bar (void);
void foo_cold (void);
void baz (void);

void
baz (void)
{
  asm ("baz_label: .globl baz_label");
}						/* baz end */

void
foo_cold (void)
{						/* foo_cold prologue */
  asm ("foo_cold_label: .globl foo_cold_label");
  baz ();					/* foo_cold baz call */
  asm ("foo_cold_label2: .globl foo_cold_label2");
}						/* foo_cold end */

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
  if (e) foo_cold ();				/* foo foo_cold call */
  asm ("foo_label3: .globl foo_label3");
}						/* foo end */

int
main (void)
{						/* main prologue */
  asm ("main_label: .globl main_label");
  foo ();					/* main foo call */
  asm ("main_label2: .globl main_label2");
  return 0;					/* main return */
}						/* main end */

