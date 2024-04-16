/* This testcase is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

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

/* This program is intended to be started outside of gdb, and then
   attached to by gdb.  It loops for a while, but not forever.  */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

int
main ()
{
  /* In case we inherit SIG_IGN.  */
  signal (SIGTTOU, SIG_DFL);

  alarm (240);

  int count = 0;
  while (1)
    {
      struct termios termios;

      printf ("pid=%ld, count=%d\n", (long) getpid (), count++);

      /* This generates a SIGTTOU if our progress group is not in the
	 foreground.  */
      tcgetattr (0, &termios);
      tcsetattr (0, TCSANOW, &termios);

      usleep (100000);
    }

  return 0;
}
