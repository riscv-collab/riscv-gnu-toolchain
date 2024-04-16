/* This test program is part of GDB, the GNU debugger.

   Copyright 2015-2024 Free Software Foundation, Inc.

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

typedef void (*callback_t) (void);

extern void process (callback_t cb);
extern void parent (int first, callback_t cb);

void
ignore (int unused)
{
  (void) unused;
}

void
process (callback_t cb)
{
  parent (0, cb);
}

void
parent (int first, callback_t cb)
{
  void child (void)
  {
    /* When reaching this, there are two block instances for PARENT on the
       stack: the one that is right in the upper frame is not the one actually
       used for non-local references, so GDB has to follow the static link in
       order to get the correct instance, and thus in order to read the proper
       variables.

       As a simple check, we can verify that under GDB, the following is true:
       parent_first == first (which should be one: see the IF block below).  */
    const int parent_first = first;
    ignore (parent_first); /* STOP */
    ignore (first);
  }

  if (first)
    process (&child);
  else
    cb ();
}

int
main ()
{
  parent (1, NULL);
  return 0;
}
