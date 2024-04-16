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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <errno.h>

#ifndef PTRACE_ATTACH
#define PTRACE_ATTACH PT_ATTACH
#endif

int
main (void)
{
  long l;

  switch (fork ())
  {
    case -1:
      perror ("fork");
      exit (1);
    case 0:
      errno = 0;
      /* The 4th argument to ptrace () is 0 on purpose, as it is compatible
	 between kernels that accept void* (like Linux) and int (NetBSD).  */
      ptrace (PTRACE_ATTACH, getppid (), NULL, 0);
      if (errno != 0)
	perror ("PTRACE_ATTACH");
      break;
  }
  sleep (600);
  return 0;
}
