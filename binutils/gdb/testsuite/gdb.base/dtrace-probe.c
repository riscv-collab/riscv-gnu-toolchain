/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2024 Free Software Foundation, Inc.

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

#include "dtrace-probe.h"

int
main ()
{
  char *name = "application";

  TEST_TWO_LOCATIONS ();
  
  int i = 0;
  while (i < 10)
    {
      i++;
      if (TEST_PROGRESS_COUNTER_ENABLED ())
	TEST_PROGRESS_COUNTER (name, i);
      else
	TEST_TWO_LOCATIONS ();
    }
      
  return 0; /* last break here */
}
