/* This testcase is part of GDB, the GNU debugger.

   Copyright 2022-2024 Free Software Foundation, Inc.

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

#include <sys/random.h>

void
marker1 (void)
{
}

void
marker2 (void)
{
}

unsigned char buf[6];

int
main (void)
{
  buf[0] = 0xff;
  buf[5] = 0xff;
  marker1 ();
  volatile ssize_t r = getrandom (&buf[1], 4, 0);
  marker2 ();
  return 0;
}
