/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

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
#include <stdio.h>

volatile int vi = 0;

void
inc_vi ()
{
  vi++;
}

void
print_vi ()
{
  printf ("vi=%d\n", vi);
}

void
increment ()
{
  inc_vi ();
}

int
main (int argc, char **argv)
{
  print_vi ();
  increment ();
  print_vi ();
  increment ();
  print_vi ();
  increment ();
  print_vi ();

  exit (0);
}
