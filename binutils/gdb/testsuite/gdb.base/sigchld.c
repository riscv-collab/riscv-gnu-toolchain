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

/* Check that GDB isn't messing the SIGCHLD mask while creating an
   inferior.  */

#include <signal.h>
#include <stdlib.h>

int
main ()
{
  sigset_t mask;

  sigemptyset (&mask);
  sigprocmask (SIG_BLOCK, NULL, &mask);

  if (!sigismember (&mask, SIGCHLD))
    return 0; /* good, not blocked */
  else
    return 1; /* bad, blocked */
}
