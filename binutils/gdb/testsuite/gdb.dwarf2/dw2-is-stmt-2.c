/* Copyright 2020-2024 Free Software Foundation, Inc.

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

/* This tests GDB's handling of the DWARF is-stmt field in the line table.

   This field is used when many addresses all represent the same source
   line.  The address(es) at which it is suitable to place a breakpoint for
   a line are marked with is-stmt true, while address(es) that are not good
   places to place a breakpoint are marked as is-stmt false.

   In order to build a reproducible test and exercise GDB's is-stmt
   support, we will be generating our own DWARF.  The test will contain a
   series of C source lines, ensuring that we get a series of assembler
   instructions.  Each C source line will be given an assembler label,
   which we use to generate a fake line table.

   In this fake line table each assembler block is claimed to represent a
   single C source line, however, we will toggle the is-stmt flag.  We can
   then debug this with GDB and test the handling of is-stmt.  */

/* Used to insert labels with which we can build a fake line table.  */
#define LL(N)						\
  do							\
    {							\
      asm ("line_label_" #N ": .globl line_label_" #N); \
      var = (N);					\
      bar = ((N) + 1);					\
    }							\
  while (0)

volatile int var;
volatile int bar;

int
main ()
{					/* main prologue */
  asm ("main_label: .globl main_label");

  /* main start */

  /* Line 1.  */
  /* Line 2.  */
  /* Line 3.  */
  /* Line 4.  */
  /* Line 5.  */

  LL (0);
  LL (1);
  LL (2);
  LL (3);
  LL (4);
  LL (5);
  LL (6);
  LL (7);
  LL (8);
  LL (9);
  LL (10);
  LL (11);
  LL (12);
  LL (13);
  LL (14);
  LL (15);
  LL (16);
  return 0;				/* main end */
}

#if 0

PROLOGUE*
1: L1
2: L1*
3: L2
4: L1
L3
L4
L4*
L2*
L2
L3
L5
L5*
L3
L4
L5
RETURN

#endif
