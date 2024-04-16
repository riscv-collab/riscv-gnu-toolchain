/* Things needed for both reading and writing DWARF indices.

   Copyright (C) 1994-2024 Free Software Foundation, Inc.

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
#include "dwarf2/index-common.h"

/* See dwarf-index-common.h.  */

hashval_t
mapped_index_string_hash (int index_version, const void *p)
{
  const unsigned char *str = (const unsigned char *) p;
  hashval_t r = 0;
  unsigned char c;

  while ((c = *str++) != 0)
    {
      if (index_version >= 5)
	c = tolower (c);
      r = r * 67 + c - 113;
    }

  return r;
}

/* See dwarf-index-common.h.  */

uint32_t
dwarf5_djb_hash (const char *str_)
{
  const unsigned char *str = (const unsigned char *) str_;

  /* Note: tolower here ignores UTF-8, which isn't fully compliant.
     See http://dwarfstd.org/ShowIssue.php?issue=161027.1.  */

  uint32_t hash = 5381;
  while (int c = *str++)
    hash = hash * 33 + tolower (c);
  return hash;
}

/* See dwarf-index-common.h.  */

uint32_t
dwarf5_djb_hash (std::string_view str)
{
  /* Note: tolower here ignores UTF-8, which isn't fully compliant.
     See http://dwarfstd.org/ShowIssue.php?issue=161027.1.  */

  uint32_t hash = 5381;
  for (char c : str)
    hash = hash * 33 + tolower (c & 0xff);
  return hash;
}
