/* Test program for byte registers.

   Copyright 2010-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

int data[] = {
  0x14131211,
  0x24232221,
  0x34333231,
  0x44434241,
};

int
main (int argc, char **argv)
{
  register int eax asm ("eax");
  register int ebx asm ("ebx");
  register int ecx asm ("ecx");
  register int edx asm ("edx");

  asm ("mov 0(%0), %%eax\n\t"
       "mov 4(%0), %%ebx\n\t"
       "mov 8(%0), %%ecx\n\t"
       "mov 12(%0), %%edx\n\t"
       : /* no output operands */
       : "r" (data) 
       : "eax", "ebx", "ecx", "edx");

  asm ("nop" /* first breakpoint here */
       /* i386-{byte,word}.exp write eax-edx here.
	  Tell gcc/clang they're live.  */
       : "=r" (eax), "=r" (ebx), "=r" (ecx), "=r" (edx)
       : /* no inputs */);

  asm ("mov %%eax, 0(%0)\n\t"
       "mov %%ebx, 4(%0)\n\t"
       "mov %%ecx, 8(%0)\n\t"
       "mov %%edx, 12(%0)\n\t"
       : /* no output operands */
       : "r" (data),
	 /* Mark these as inputs so that gcc/clang won't try to use them as
	    a temp to build %0.  */
	 "r" (eax), "r" (ebx), "r" (ecx), "r" (edx));
  puts ("Bye!"); /* second breakpoint here */

  return 0;
}
