/* Shared allocation functions for GDB, the GNU debugger.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

/* This file is unusual.

   Because both libiberty and readline define xmalloc and friends, the
   functions in this file can't appear in a library -- that will cause
   link errors.

   And, because we want to turn the common code into a library, this
   file can't live there.

   So, it lives in gdb and is built separately by gdb and gdbserver.
   Please be aware of this when modifying it.

   This also explains why this file includes common-defs.h and not
   defs.h or server.h -- we'd prefer to avoid depending on the
   GDBSERVER define when possible, and for this file it seemed
   simple to do so.  */

#include "gdbsupport/common-defs.h"
#include "libiberty.h"
#include "gdbsupport/errors.h"

/* The xmalloc() (libiberty.h) family of memory management routines.

   These are like the ISO-C malloc() family except that they implement
   consistent semantics and guard against typical memory management
   problems.  */

void *
xmalloc (size_t size)
{
  void *val;

  /* See libiberty/xmalloc.c.  This function need's to match that's
     semantics.  It never returns NULL.  */
  if (size == 0)
    size = 1;

  val = malloc (size);         /* ARI: malloc */
  if (val == NULL)
    malloc_failure (size);

  return val;
}

void *
xrealloc (void *ptr, size_t size)
{
  void *val;

  /* See libiberty/xmalloc.c.  This function need's to match that's
     semantics.  It never returns NULL.  */
  if (size == 0)
    size = 1;

  if (ptr != NULL)
    val = realloc (ptr, size);	/* ARI: realloc */
  else
    val = malloc (size);	        /* ARI: malloc */
  if (val == NULL)
    malloc_failure (size);

  return val;
}

void *
xcalloc (size_t number, size_t size)
{
  void *mem;

  /* See libiberty/xmalloc.c.  This function need's to match that's
     semantics.  It never returns NULL.  */
  if (number == 0 || size == 0)
    {
      number = 1;
      size = 1;
    }

  mem = calloc (number, size);      /* ARI: xcalloc */
  if (mem == NULL)
    malloc_failure (number * size);

  return mem;
}

void
xmalloc_failed (size_t size)
{
  malloc_failure (size);
}
