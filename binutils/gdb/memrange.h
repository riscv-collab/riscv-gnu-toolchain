/* The memory range data structure, and associated utilities.

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

#ifndef MEMRANGE_H
#define MEMRANGE_H

/* Defines a [START, START + LENGTH) memory range.  */

struct mem_range
{
  mem_range () = default;

  mem_range (CORE_ADDR start_, int length_)
  : start (start_), length (length_)
  {}

  bool operator< (const mem_range &other) const
  {
    return this->start < other.start;
  }

  bool operator== (const mem_range &other) const
  {
    return (this->start == other.start
	    && this->length == other.length);
  }

  /* Lowest address in the range.  */
  CORE_ADDR start;

  /* Length of the range.  */
  int length;
};

/* Returns true if the ranges defined by [start1, start1+len1) and
   [start2, start2+len2) overlap.  */

extern int mem_ranges_overlap (CORE_ADDR start1, int len1,
			       CORE_ADDR start2, int len2);

/* Returns true if ADDR is in RANGE.  */

extern int address_in_mem_range (CORE_ADDR addr,
				 const struct mem_range *range);

/* Sort ranges by start address, then coalesce contiguous or
   overlapping ranges.  */

extern void normalize_mem_ranges (std::vector<mem_range> *memory);

#endif
