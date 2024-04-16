/* GDB generic memory tagging definitions.
   Copyright (C) 2022-2024 Free Software Foundation, Inc.

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

#ifndef MEMTAG_H
#define MEMTAG_H

#include "bfd.h"

struct memtag_section_info
{
  /* The start address of the tagged memory range.  */
  CORE_ADDR start_address;
  /* The final address of the tagged memory range.  */
  CORE_ADDR end_address;
  /* The section containing tags for the memory range
     [start_address, end_address).  */
  asection *memtag_section;
};

/* Helper function to walk through memory tag sections in a core file.

   Return TRUE if there is a "memtag" section containing ADDRESS.  Return FALSE
   otherwise.

   If SECTION is provided, search from that section onwards. If SECTION is
   nullptr, then start a new search.

   If a "memtag" section containing ADDRESS is found, fill INFO with data
   about such section.  Otherwise leave it unchanged.  */

bool get_next_core_memtag_section (bfd *abfd, asection *section,
				   CORE_ADDR address,
				   memtag_section_info &info);

#endif /* MEMTAG_H */
