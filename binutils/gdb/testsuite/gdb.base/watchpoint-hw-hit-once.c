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

static int watchee;

int
main (void)
{
  volatile int dummy;

  /* Stub lines are present as no breakpoint gets hit if current PC
     already stays on the line PC while entering "step"/"continue".  */

  dummy = 0;	/* Stub to catch WATCHEE access after runto_main.  */
  dummy = watchee;
  dummy = 1;	/* Stub to catch break-at-exit after WATCHEE has been hit.  */
  dummy = 2;	/* break-at-exit */

  return 0;
}
