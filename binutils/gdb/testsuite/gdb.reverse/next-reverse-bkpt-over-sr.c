/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <string.h>

/* Test reverse finish command.  The code for this test is based on the code
   in test step-reverse.c.  The code was modified to call the function via
   a function pointer so the test will behave the same on all platforms.
   See comments in next-reverse-bkpt-over-sr.exp.  */

int myglob = 0;

int
callee() {
  return myglob++;
}

int
main () {
   int (*funp) (void) = callee;

   /* Test next-reverse-bkpt-over-sr.exp needs to call function callee using
      a function pointer to work correctly on PowerPC.  See comments in
      next-reverse-bkpt-over-sr.exp.  */
   funp ();       /* FUNCTION PTR CALL TO CALLEE */

   exit (0);      /* END OF MAIN */
}
