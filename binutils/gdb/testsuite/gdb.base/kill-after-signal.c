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

#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

void
handler (int signo)
{
  sleep (600);
  assert (0);
}

int
main (void)
{
  signal (SIGUSR1, handler);
  raise (SIGUSR1);
  assert (0);
  return 0;
}
