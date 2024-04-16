/* Memory attributes support, for GDB.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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

#ifndef MEMATTR_H
#define MEMATTR_H

enum mem_access_mode
{
  MEM_NONE,                     /* Memory that is not physically present.  */
  MEM_RW,			/* read/write */
  MEM_RO,			/* read only */
  MEM_WO,			/* write only */

  /* Read/write, but special steps are required to write to it.  */
  MEM_FLASH
};

enum mem_access_width
{
  MEM_WIDTH_UNSPECIFIED,
  MEM_WIDTH_8,			/*  8 bit accesses */
  MEM_WIDTH_16,			/* 16  "      "    */
  MEM_WIDTH_32,			/* 32  "      "    */
  MEM_WIDTH_64			/* 64  "      "    */
};

/* The set of all attributes that can be set for a memory region.
  
   This structure was created so that memory attributes can be passed
   to target_ functions without exposing the details of memory region
   list, which would be necessary if these fields were simply added to
   the mem_region structure.

   FIXME: It would be useful if there was a mechanism for targets to
   add their own attributes.  For example, the number of wait states.  */
 
struct mem_attrib 
{
  static mem_attrib unknown ()
  {
    mem_attrib attrib;

    attrib.mode = MEM_NONE;

    return attrib;
  }

  /* read/write, read-only, or write-only */
  enum mem_access_mode mode = MEM_RW;

  enum mem_access_width width = MEM_WIDTH_UNSPECIFIED;

  /* enables hardware breakpoints */
  int hwbreak = 0;
  
  /* enables host-side caching of memory region data */
  int cache = 0;
  
  /* Enables memory verification.  After a write, memory is re-read
     to verify that the write was successful.  */
  int verify = 0;

  /* Block size.  Only valid if mode == MEM_FLASH.  */
  int blocksize = -1;
};

struct mem_region 
{
  /* Create a mem_region with default attributes.  */

  mem_region (CORE_ADDR lo_, CORE_ADDR hi_)
    : lo (lo_), hi (hi_)
  {}

  /* Create a mem_region with access mode MODE_, but otherwise default
     attributes.  */

  mem_region (CORE_ADDR lo_, CORE_ADDR hi_, mem_access_mode mode_)
    : lo (lo_), hi (hi_)
  {
    attrib.mode = mode_;
  }

  /* Create a mem_region with attributes ATTRIB_.  */

  mem_region (CORE_ADDR lo_, CORE_ADDR hi_, const mem_attrib &attrib_)
    : lo (lo_), hi (hi_), attrib (attrib_)
  {}

  bool operator< (const mem_region &other) const
  {
    return this->lo < other.lo;
  }

  /* Lowest address in the region.  */
  CORE_ADDR lo;
  /* Address past the highest address of the region. 
     If 0, upper bound is "infinity".  */
  CORE_ADDR hi;

  /* Item number of this memory region.  */
  int number = 0;

  /* Status of this memory region (enabled if true, otherwise
     disabled).  */
  bool enabled_p = true;

  /* Attributes for this region.  */
  mem_attrib attrib;
};

extern struct mem_region *lookup_mem_region (CORE_ADDR);

void invalidate_target_mem_regions (void);

#endif	/* MEMATTR_H */
