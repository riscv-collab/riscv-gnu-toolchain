/* This testcase is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

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

#include <ust/marker.h>

static void
end (void)
{}

int
main (void)
{
  /* Some code to make sure that breakpoints on `main' and `ust/bar' marker
     are set at different addresses.  */
  int a = 0;
  int b = a;

  trace_mark(ust, bar, "str %s", "FOOBAZ");
  trace_mark(ust, bar2, "number1 %d number2 %d", 53, 9800);

  end ();
  return 0;
}
