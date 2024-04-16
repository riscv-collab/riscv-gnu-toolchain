/* This testcase is part of GDB, the GNU debugger.

   Copyright 2019-2024 Free Software Foundation, Inc.

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

#include <stdio.h>
extern void func ();
void
__acle_se_func ()
{
  printf ("__acle_se_func\n");
}

/* The following code is written using asm so that the instructions in function
 * "func" will be placed in .gnu.sgstubs section of the executable.  */
asm ("\t.section .gnu.sgstubs,\"ax\",%progbits\n"
     "\t.global func\n"
     "\t.type func, %function\n"
     "func:\n"
     "\tnop @sg\n"
     "\tb __acle_se_func @b.w");

void
fun1 ()
{
  printf ("In fun1\n");
}

int
main (void)
{
  func ();
  fun1 ();
  __acle_se_func ();
  func ();

  return 0;
}
