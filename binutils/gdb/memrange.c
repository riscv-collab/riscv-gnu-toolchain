/* Memory ranges

   Copyright (C) 2010-2024 Free Software Foundation, Inc.

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
#include "memrange.h"
#include <algorithm>

int
mem_ranges_overlap (CORE_ADDR start1, int len1,
		    CORE_ADDR start2, int len2)
{
  ULONGEST h, l;

  l = std::max (start1, start2);
  h = std::min (start1 + len1, start2 + len2);
  return (l < h);
}

/* See memrange.h.  */

int
address_in_mem_range (CORE_ADDR address, const struct mem_range *r)
{
  return (r->start <= address
	  && (address - r->start) < r->length);
}

void
normalize_mem_ranges (std::vector<mem_range> *memory)
{
  if (!memory->empty ())
    {
      std::vector<mem_range> &m = *memory;

      std::sort (m.begin (), m.end ());

      int a = 0;
      for (int b = 1; b < m.size (); b++)
	{
	  /* If mem_range B overlaps or is adjacent to mem_range A,
	     merge them.  */
	  if (m[b].start <= m[a].start + m[a].length)
	    {
	      m[a].length = std::max ((CORE_ADDR) m[a].length,
				      (m[b].start - m[a].start) + m[b].length);
	      continue;		/* next b, same a */
	    }
	  a++;			/* next a */

	  if (a != b)
	    m[a] = m[b];
	}

      m.resize (a + 1);
    }
}
