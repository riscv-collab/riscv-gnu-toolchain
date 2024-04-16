/* Filename-seen cache for the GNU debugger, GDB.

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

#include "defs.h"
#include "filename-seen-cache.h"
#include "filenames.h"

  /* Initial size of the table.  It automagically grows from here.  */
#define INITIAL_FILENAME_SEEN_CACHE_SIZE 100

/* filename_seen_cache constructor.  */

filename_seen_cache::filename_seen_cache ()
  : m_tab (htab_create_alloc (INITIAL_FILENAME_SEEN_CACHE_SIZE,
			      filename_hash, filename_eq,
			      NULL, xcalloc, xfree))
{
}

/* See filename-seen-cache.h.  */

void
filename_seen_cache::clear ()
{
  htab_empty (m_tab.get ());
}

/* See filename-seen-cache.h.  */

bool
filename_seen_cache::seen (const char *file)
{
  void **slot;

  /* Is FILE in tab?  */
  slot = htab_find_slot (m_tab.get (), file, INSERT);
  if (*slot != NULL)
    return true;

  /* No; add it to tab.  */
  *slot = (char *) file;
  return false;
}
