/* Common Linux target-dependent functionality for AArch64 MTE

   Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

#include "arch/aarch64-mte-linux.h"

/* See arch/aarch64-mte-linux.h */

void
aarch64_mte_pack_tags (gdb::byte_vector &tags)
{
  /* Nothing to pack?  */
  if (tags.empty ())
    return;

  /* If the tags vector has an odd number of elements, add another
     zeroed-out element to make it even.  This facilitates packing.  */
  if ((tags.size () % 2) != 0)
    tags.emplace_back (0);

  for (int unpacked = 0, packed = 0; unpacked < tags.size ();
       unpacked += 2, packed++)
    tags[packed] = (tags[unpacked + 1] << 4) | tags[unpacked];

  /* Now we have half the size.  */
  tags.resize (tags.size () / 2);
}

/* See arch/aarch64-mte-linux.h */

void
aarch64_mte_unpack_tags (gdb::byte_vector &tags, bool skip_first)
{
  /* Nothing to unpack?  */
  if (tags.empty ())
    return;

  /* An unpacked MTE tags vector will have twice the number of elements
     compared to an unpacked one.  */
  gdb::byte_vector unpacked_tags (tags.size () * 2);

  int unpacked = 0, packed = 0;

  if (skip_first)
    {
      /* We are not interested in the first unpacked element, just discard
	 it.  */
      unpacked_tags[unpacked] = (tags[packed] >> 4) & 0xf;
      unpacked++;
      packed++;
    }

  for (; packed < tags.size (); unpacked += 2, packed++)
    {
      unpacked_tags[unpacked] = tags[packed] & 0xf;
      unpacked_tags[unpacked + 1] = (tags[packed] >> 4) & 0xf;
    }

  /* Update the original tags vector.  */
  tags = std::move (unpacked_tags);
}

/* See arch/aarch64-mte-linux.h */

size_t
aarch64_mte_get_tag_granules (CORE_ADDR addr, size_t len, size_t granule_size)
{
  /* An empty range has 0 tag granules.  */
  if (len == 0)
    return 0;

  /* Start address */
  CORE_ADDR s_addr = align_down (addr, granule_size);
  /* End address */
  CORE_ADDR e_addr = align_down (addr + len - 1, granule_size);

  /* We always have at least 1 granule because len is non-zero at this
     point.  */
  return 1 + (e_addr - s_addr) / granule_size;
}

/* See arch/aarch64-mte-linux.h */

CORE_ADDR
aarch64_mte_make_ltag_bits (CORE_ADDR value)
{
  return value & AARCH64_MTE_LOGICAL_MAX_VALUE;
}

/* See arch/aarch64-mte-linux.h */

CORE_ADDR
aarch64_mte_make_ltag (CORE_ADDR value)
{
  return (aarch64_mte_make_ltag_bits (value)
	  << AARCH64_MTE_LOGICAL_TAG_START_BIT);
}

/* See arch/aarch64-mte-linux.h */

CORE_ADDR
aarch64_mte_set_ltag (CORE_ADDR address, CORE_ADDR tag)
{
  /* Remove the existing tag.  */
  address &= ~aarch64_mte_make_ltag (AARCH64_MTE_LOGICAL_MAX_VALUE);

  /* Return the new tagged address.  */
  return address | aarch64_mte_make_ltag (tag);
}

/* See arch/aarch64-mte-linux.h */

CORE_ADDR
aarch64_mte_get_ltag (CORE_ADDR address)
{
  CORE_ADDR ltag_addr = address >> AARCH64_MTE_LOGICAL_TAG_START_BIT;
  return aarch64_mte_make_ltag_bits (ltag_addr);
}
