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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

volatile char *p = NULL;

extern long
bowler (void)
{
  return *p;
}

extern void
keeper (int sig)
{
  static int recurse = 0;
  if (++recurse < 3)
    bowler ();

  _exit (0);
}

int
main (void)
{
  struct sigaction act;
  memset (&act, 0, sizeof act);
  act.sa_handler = keeper;
  act.sa_flags = SA_NODEFER;
  sigaction (SIGSEGV, &act, NULL);
  sigaction (SIGBUS, &act, NULL);

  bowler ();
  return 0;
}
