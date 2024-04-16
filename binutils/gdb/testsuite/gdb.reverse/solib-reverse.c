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

/* Test reverse debugging of shared libraries.

   N.B. Do not call system routines here, we don't want to have to deal with
   whether or not there is debug info present for them.  */

#include "shr.h"

int main ()
{
  char* cptr = "String 1";
  int b[2] = {5,8};

  /* Call these functions once before we start testing so that they get
     resolved by the dynamic loader.  If the system has debug info for
     the dynamic loader installed, reverse-stepping for the first call
     will otherwise stop in the dynamic loader, which is not what we want.  */
  shr1 ("");
  shr2 (0);

  b[0] = shr2(12);		/* begin part two */
  b[1] = shr2(17);		/* middle part two */

  b[0] = 6;   b[1] = 9;		/* generic statement, end part two */

  shr1 ("message 1\n");		/* shr1 one */
  shr1 ("message 2\n");		/* shr1 two */
  shr1 ("message 3\n");		/* shr1 three */

  b[0] = 0;			/* end part one */
  return 0; /* end of main */
}

