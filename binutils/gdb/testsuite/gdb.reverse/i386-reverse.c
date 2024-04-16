/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

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

/* Architecture tests for intel i386 platform.  */

void 
inc_dec_tests (void)
{
  asm ("inc %eax");
  asm ("inc %ecx");
  asm ("inc %edx");
  asm ("inc %ebx");
  asm ("inc %esp");
  asm ("inc %ebp");
  asm ("inc %esi");
  asm ("inc %edi");
  asm ("dec %eax");
  asm ("dec %ecx");
  asm ("dec %edx");
  asm ("dec %ebx");
  asm ("dec %esp");
  asm ("dec %ebp");
  asm ("dec %esi");
  asm ("dec %edi");
} /* end inc_dec_tests */

int 
main ()
{
  inc_dec_tests ();
  return 0;	/* end of main */
}
