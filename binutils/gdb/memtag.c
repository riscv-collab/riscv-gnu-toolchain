/* GDB generic memory tagging functions.

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

#include "defs.h"
#include "memtag.h"
#include "bfd.h"

/* See memtag.h */

bool
get_next_core_memtag_section (bfd *abfd, asection *section,
			      CORE_ADDR address, memtag_section_info &info)
{
  /* If the caller provided no SECTION to start from, search from the
     beginning.  */
  if (section == nullptr)
    section = bfd_get_section_by_name (abfd, "memtag");

  /* Go through all the memtag sections and figure out if ADDRESS
     falls within one of the memory ranges that contain tags.  */
  while (section != nullptr)
    {
      size_t memtag_range_size = section->rawsize;
      size_t tags_size = bfd_section_size (section);

      /* Empty memory range or empty tag dump should not happen.  Warn about
	 it but keep going through the sections.  */
      if (memtag_range_size == 0 || tags_size == 0)
	{
	  warning (_("Found memtag section with empty memory "
		     "range or empty tag dump"));
	  continue;
	}
      else
	{
	  CORE_ADDR start_address = bfd_section_vma (section);
	  CORE_ADDR end_address = start_address + memtag_range_size;

	  /* Is the address within [start_address, end_address)?  */
	  if (address >= start_address
	      && address < end_address)
	    {
	      info.start_address = start_address;
	      info.end_address = end_address;
	      info.memtag_section = section;
	      return true;
	    }
	}
      section = bfd_get_next_section_by_name (abfd, section);
    }
  return false;
}
