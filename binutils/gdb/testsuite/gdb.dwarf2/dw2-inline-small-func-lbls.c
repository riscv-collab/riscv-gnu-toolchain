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

/* Used to insert labels with which we can build a fake line table.  */
#define LL(N) asm ("line_label_" #N ": .globl line_label_" #N)

volatile int var;
volatile int bar;

/* Generate some code to take up some space.  */
#define FILLER do { \
    var = 99;	    \
} while (0)

int
main ()
{					/* main prologue */
  asm ("main_label: .globl main_label");
  LL (1);
  FILLER;
  LL (2);
  FILLER;
  LL (3);
  return 0;				/* main end */
}
