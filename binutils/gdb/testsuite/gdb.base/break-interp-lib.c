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

#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Force REL->RELA conversion on i386, see "Prelink", March 4, 2004.  */
volatile int v[2];
volatile int *vptr = &v[1];

void
libfunc (const char *action)
{
  assert (action != NULL);

  if (strcmp (action, "segv") == 0)
    raise (SIGSEGV);

  if (strcmp (action, "sleep") == 0)
    {
      puts ("sleeping");
      fflush (stdout);

      sleep (60);
    }

  assert (0);
}
