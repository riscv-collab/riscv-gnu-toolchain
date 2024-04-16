/* Test single-stepping system call instructions in sparc.

   Copyright 2014-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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
#include <unistd.h>

int global;

static void
handler (int sig)
{
}

int
main ()
{
  signal (SIGALRM, handler);
  kill (getpid (), SIGALRM);
  return 0; /* sparc-sysstep.exp: last */
}
