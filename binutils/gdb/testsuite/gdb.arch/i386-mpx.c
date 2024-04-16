/* Test program for MPX registers.

   Copyright 2013-2024 Free Software Foundation, Inc.

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

int
main (void)
{
#ifdef __x86_64__
  asm ("mov $10, %rax\n\t"
       "mov $9, %rdx\n\t"
       "bndmk (%rax,%rdx), %bnd0\n\t"
       "mov $20, %rax\n\t"
       "mov $9, %rdx\n\t"
       "bndmk (%rax,%rdx), %bnd1\n\t"
       "mov $30, %rax\n\t"
       "mov $9, %rdx\n\t"
       "bndmk (%rax,%rdx), %bnd2\n\t"
       "mov $40, %rax\n\t"
       "mov $9, %rdx\n\t"
       "bndmk (%rax,%rdx), %bnd3\n\t"
       "bndstx %bnd3, (%rax) \n\t"
       "nop\n\t"
       );
#else
  asm ("mov $10, %eax\n\t"
       "mov $9, %edx\n\t"
       "bndmk (%eax,%edx), %bnd0\n\t"
       "mov $20, %eax\n\t"
       "mov $9, %edx\n\t"
       "bndmk (%eax,%edx), %bnd1\n\t"
       "mov $30, %eax\n\t"
       "mov $9, %edx\n\t"
       "bndmk (%eax,%edx), %bnd2\n\t"
       "mov $40, %eax\n\t"
       "mov $9, %edx\n\t"
       "bndmk (%eax,%edx), %bnd3\n\t"
       "bndstx  %bnd3, (%eax)\n\t"
       "nop\n\t"
       );
#endif
  asm ("nop\n\t");	/* break here.  */

  return 0;
}
