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

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/uio.h>

void
marker1 (void)
{
}

void
marker2 (void)
{
}

int fds[2] = { -1, -1 };
char buf[5];
const struct iovec v[4] = {
  { &buf[1], 1 },
  { &buf[0], 1 },
  { &buf[3], 1 },
  { &buf[2], 1 },
};

int
main (void)
{
  marker1 ();
  pipe (fds);
  write (fds[1], "UNIX", 4);
  readv (fds[0], v, 4);
  marker2 ();
  return 0;
}
