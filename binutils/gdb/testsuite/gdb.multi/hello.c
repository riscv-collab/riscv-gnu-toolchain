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

#include <stdlib.h>
#include <unistd.h>

short hglob = 1;

short glob = 92;

void
bar()
{
  if (glob == 0)
    exit(1);
}

int commonfun() { bar(); return 0; } /* from hello */

int
hello(int x)
{
  x *= 2;
  return x + 45;
}

static void
hello_loop (void)
{
}

int
main()
{
  int tmpx;

  alarm (240);

  bar();
  tmpx = hello(glob);
  commonfun();
  glob = tmpx;
  commonfun();

  while (1)
    {
      hello_loop ();
      usleep (20);
    }
}
