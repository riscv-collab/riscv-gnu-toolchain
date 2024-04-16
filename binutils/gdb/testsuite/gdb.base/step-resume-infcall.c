/* This testcase is part of GDB, the GNU debugger.

   Copyright 2005-2024 Free Software Foundation, Inc.

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

volatile int cond_hit;

int
cond (void)
{
  cond_hit++;

  return 1;
}

int
func (void)
{
  return 0;	/* in-func */
}

int
main (void)
{
  /* Variable is used so that there is always at least one instruction on the
     same line after FUNC returns.  */
  int i = func ();	/* call-func */

  /* Dummy calls.  */
  cond ();
  func ();
  return 0;
}
