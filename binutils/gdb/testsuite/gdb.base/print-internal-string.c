/* This testcase is part of GDB, the GNU debugger.

   Copyright 2023-2024 Free Software Foundation, Inc.

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

#include <stddef.h>
#include <string.h>

/* A memory area used as the malloc memory buffer.  */

static char arena[256];

/* Override malloc().  When GDB tries to push strings into the inferior we
   will always return the same pointer (to arena).  This does mean we can't
   have multiple strings in use at the same time, but that's fine for this
   simple test.  On each malloc call the contents of arena are reset, which
   should make it more obvious if GDB tried to print memory that it
   shouldn't.  */

void *
malloc (size_t size)
{
  /* Reset the contents of arena, and ensure there's a null-character at
     the end just in case GDB tries to print memory that it shouldn't.  */
  memset (arena, 'X', sizeof (arena));
  arena [sizeof (arena) - 1] = '\0';
  if (size > sizeof (arena))
    return NULL;
  return arena;
}

/* This function is called from GDB.  */

void
take_string (const char *str)
{
  /* Nothing.  */
}

int
main (void)
{
  return 0;
}
