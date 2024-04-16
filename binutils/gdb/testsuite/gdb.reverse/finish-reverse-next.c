/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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

/* The reverse finish command should return from a function and stop on
   the first instruction of the source line where the function call is made.
   Specifically, the behavior should match doing a reverse next from the
   first instruction in the function.  GDB should only require one reverse
   step or next statement to reach the previous source code line.

   This test verifies the fix for gdb bugzilla:

   https://sourceware.org/bugzilla/show_bug.cgi?id=29927

   PowerPC supports two entry points to a function.  The normal entry point
   is called the local entry point (LEP).  The alternate entry point is called
   the global entry point (GEP).  The GEP is only used if the table of
   contents (TOC) value stored in register r2 needs to be setup prior to
   execution starting at the LEP.  A function call via a function pointer
   will entry via the GEP.  A normal function call will enter via the LEP.

   This test has been expanded to include tests to verify the reverse-finish
   command works properly if the function is called via the GEP.  The original
   test only verified the reverse-finish command for a normal call that used
   the LEP.  */

int
function2 (int a, int b)
{
  int ret = 0;
  ret = ret + a + b;
  return ret;
}

int
function1 (int a, int b)   // FUNCTION1
{
  int ret = 0;
  int (*funp) (int, int) = &function2;
  /* The assembly code for this function when compiled for PowerPC is as
     follows:

     0000000010000758 <function1>:
     10000758:	02 10 40 3c 	lis     r2,4098        <- GEP
     1000075c:	00 7f 42 38 	addi    r2,r2,32512
     10000760:	a6 02 08 7c 	mflr    r0             <- LEP
     10000764:	10 00 01 f8 	std     r0,16(r1)
     ....

     When the function is called on PowerPC with function1 (a, b) the call
     enters at the Local Entry Point (LEP).  When the function is called via
     a function pointer, the Global Entry Point (GEP) for function1 is used.
     The GEP sets up register 2 before reaching the LEP.
  */
  ret = funp (a + 1, b + 2);
  return ret;
}

int
main(int argc, char* argv[])
{
  int a, b;
  int (*funp) (int, int) = &function1;

  /* Call function via Local Entry Point (LEP).  */

  a = 1;
  b = 5;

  function1 (a, b);   // CALL VIA LEP

  /* Call function via Global Entry Point (GEP).  */
  a = 10;
  b = 50;

  funp (a, b);        // CALL VIA GEP
  return 0;
}
