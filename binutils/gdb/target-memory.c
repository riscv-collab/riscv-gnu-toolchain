/* Parts of target interface that deal with accessing memory and memory-like
   objects.

   Copyright (C) 2006-2024 Free Software Foundation, Inc.

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
#include "target.h"
#include "memory-map.h"
#include "inferior.h"

#include "gdbsupport/gdb_sys_time.h"
#include <algorithm>

static bool
compare_block_starting_address (const memory_write_request &a_req,
				const memory_write_request &b_req)
{
  return a_req.begin < b_req.begin;
}

/* Adds to RESULT all memory write requests from BLOCK that are
   in [BEGIN, END) range.

   If any memory request is only partially in the specified range,
   that part of the memory request will be added.  */

static void
claim_memory (const std::vector<memory_write_request> &blocks,
	      std::vector<memory_write_request> *result,
	      ULONGEST begin,
	      ULONGEST end)
{
  ULONGEST claimed_begin;
  ULONGEST claimed_end;

  for (const memory_write_request &r : blocks)
    {
      /* If the request doesn't overlap [BEGIN, END), skip it.  We
	 must handle END == 0 meaning the top of memory; we don't yet
	 check for R->end == 0, which would also mean the top of
	 memory, but there's an assertion in
	 target_write_memory_blocks which checks for that.  */

      if (begin >= r.end)
	continue;
      if (end != 0 && end <= r.begin)
	continue;

      claimed_begin = std::max (begin, r.begin);
      if (end == 0)
	claimed_end = r.end;
      else
	claimed_end = std::min (end, r.end);

      if (claimed_begin == r.begin && claimed_end == r.end)
	result->push_back (r);
      else
	{
	  struct memory_write_request n = r;

	  n.begin = claimed_begin;
	  n.end = claimed_end;
	  n.data += claimed_begin - r.begin;

	  result->push_back (n);
	}
    }
}

/* Given a vector of struct memory_write_request objects in BLOCKS,
   add memory requests for flash memory into FLASH_BLOCKS, and for
   regular memory to REGULAR_BLOCKS.  */

static void
split_regular_and_flash_blocks (const std::vector<memory_write_request> &blocks,
				std::vector<memory_write_request> *regular_blocks,
				std::vector<memory_write_request> *flash_blocks)
{
  struct mem_region *region;
  CORE_ADDR cur_address;

  /* This implementation runs in O(length(regions)*length(blocks)) time.
     However, in most cases the number of blocks will be small, so this does
     not matter.

     Note also that it's extremely unlikely that a memory write request
     will span more than one memory region, however for safety we handle
     such situations.  */

  cur_address = 0;
  while (1)
    {
      std::vector<memory_write_request> *r;

      region = lookup_mem_region (cur_address);
      r = region->attrib.mode == MEM_FLASH ? flash_blocks : regular_blocks;
      cur_address = region->hi;
      claim_memory (blocks, r, region->lo, region->hi);

      if (cur_address == 0)
	break;
    }
}

/* Given an ADDRESS, if BEGIN is non-NULL this function sets *BEGIN
   to the start of the flash block containing the address.  Similarly,
   if END is non-NULL *END will be set to the address one past the end
   of the block containing the address.  */

static void
block_boundaries (CORE_ADDR address, CORE_ADDR *begin, CORE_ADDR *end)
{
  struct mem_region *region;
  unsigned blocksize;
  CORE_ADDR offset_in_region;

  region = lookup_mem_region (address);
  gdb_assert (region->attrib.mode == MEM_FLASH);
  blocksize = region->attrib.blocksize;

  offset_in_region = address - region->lo;

  if (begin)
    *begin = region->lo + offset_in_region / blocksize * blocksize;
  if (end)
    *end = region->lo + (offset_in_region + blocksize - 1) / blocksize * blocksize;
}

/* Given the list of memory requests to be WRITTEN, this function
   returns write requests covering each group of flash blocks which must
   be erased.  */

static std::vector<memory_write_request>
blocks_to_erase (const std::vector<memory_write_request> &written)
{
  std::vector<memory_write_request> result;

  for (const memory_write_request &request : written)
    {
      CORE_ADDR begin, end;

      block_boundaries (request.begin, &begin, 0);
      block_boundaries (request.end - 1, 0, &end);

      if (!result.empty () && result.back ().end >= begin)
	result.back ().end = end;
      else
	result.emplace_back (begin, end);
    }

  return result;
}

/* Given ERASED_BLOCKS, a list of blocks that will be erased with
   flash erase commands, and WRITTEN_BLOCKS, the list of memory
   addresses that will be written, compute the set of memory addresses
   that will be erased but not rewritten (e.g. padding within a block
   which is only partially filled by "load").  */

static std::vector<memory_write_request>
compute_garbled_blocks (const std::vector<memory_write_request> &erased_blocks,
			const std::vector<memory_write_request> &written_blocks)
{
  std::vector<memory_write_request> result;

  unsigned j;
  unsigned je = written_blocks.size ();

  /* Look at each erased memory_write_request in turn, and
     see what part of it is subsequently written to.

     This implementation is O(length(erased) * length(written)).  If
     the lists are sorted at this point it could be rewritten more
     efficiently, but the complexity is not generally worthwhile.  */

  for (const memory_write_request &erased_iter : erased_blocks)
    {
      /* Make a deep copy -- it will be modified inside the loop, but
	 we don't want to modify original vector.  */
      struct memory_write_request erased = erased_iter;

      for (j = 0; j != je;)
	{
	  const memory_write_request *written = &written_blocks[j];

	  /* Now try various cases.  */

	  /* If WRITTEN is fully to the left of ERASED, check the next
	     written memory_write_request.  */
	  if (written->end <= erased.begin)
	    {
	      ++j;
	      continue;
	    }

	  /* If WRITTEN is fully to the right of ERASED, then ERASED
	     is not written at all.  WRITTEN might affect other
	     blocks.  */
	  if (written->begin >= erased.end)
	    {
	      result.push_back (erased);
	      goto next_erased;
	    }

	  /* If all of ERASED is completely written, we can move on to
	     the next erased region.  */
	  if (written->begin <= erased.begin
	      && written->end >= erased.end)
	    {
	      goto next_erased;
	    }

	  /* If there is an unwritten part at the beginning of ERASED,
	     then we should record that part and try this inner loop
	     again for the remainder.  */
	  if (written->begin > erased.begin)
	    {
	      result.emplace_back (erased.begin, written->begin);
	      erased.begin = written->begin;
	      continue;
	    }

	  /* If there is an unwritten part at the end of ERASED, we
	     forget about the part that was written to and wait to see
	     if the next write request writes more of ERASED.  We can't
	     push it yet.  */
	  if (written->end < erased.end)
	    {
	      erased.begin = written->end;
	      ++j;
	      continue;
	    }
	}

      /* If we ran out of write requests without doing anything about
	 ERASED, then that means it's really erased.  */
      result.push_back (erased);

    next_erased:
      ;
    }

  return result;
}

int
target_write_memory_blocks (const std::vector<memory_write_request> &requests,
			    enum flash_preserve_mode preserve_flash_p,
			    void (*progress_cb) (ULONGEST, void *))
{
  std::vector<memory_write_request> blocks = requests;
  std::vector<memory_write_request> regular;
  std::vector<memory_write_request> flash;
  std::vector<memory_write_request> erased, garbled;

  /* END == 0 would represent wraparound: a write to the very last
     byte of the address space.  This file was not written with that
     possibility in mind.  This is fixable, but a lot of work for a
     rare problem; so for now, fail noisily here instead of obscurely
     later.  */
  for (const memory_write_request &iter : requests)
    gdb_assert (iter.end != 0);

  /* Sort the blocks by their start address.  */
  std::sort (blocks.begin (), blocks.end (), compare_block_starting_address);

  /* Split blocks into list of regular memory blocks,
     and list of flash memory blocks.  */
  split_regular_and_flash_blocks (blocks, &regular, &flash);

  /* If a variable is added to forbid flash write, even during "load",
     it should be checked here.  Similarly, if this function is used
     for other situations besides "load" in which writing to flash
     is undesirable, that should be checked here.  */

  /* Find flash blocks to erase.  */
  erased = blocks_to_erase (flash);

  /* Find what flash regions will be erased, and not overwritten; then
     either preserve or discard the old contents.  */
  garbled = compute_garbled_blocks (erased, flash);

  std::vector<gdb::unique_xmalloc_ptr<gdb_byte>> mem_holders;
  if (!garbled.empty ())
    {
      if (preserve_flash_p == flash_preserve)
	{
	  /* Read in regions that must be preserved and add them to
	     the list of blocks we read.  */
	  for (memory_write_request &iter : garbled)
	    {
	      gdb_assert (iter.data == NULL);
	      gdb::unique_xmalloc_ptr<gdb_byte> holder
		((gdb_byte *) xmalloc (iter.end - iter.begin));
	      iter.data = holder.get ();
	      mem_holders.push_back (std::move (holder));
	      int err = target_read_memory (iter.begin, iter.data,
					    iter.end - iter.begin);
	      if (err != 0)
		return err;

	      flash.push_back (iter);
	    }

	  std::sort (flash.begin (), flash.end (),
		     compare_block_starting_address);
	}
    }

  /* We could coalesce adjacent memory blocks here, to reduce the
     number of write requests for small sections.  However, we would
     have to reallocate and copy the data pointers, which could be
     large; large sections are more common in loadable objects than
     large numbers of small sections (although the reverse can be true
     in object files).  So, we issue at least one write request per
     passed struct memory_write_request.  The remote stub will still
     have the opportunity to batch flash requests.  */

  /* Write regular blocks.  */
  for (const memory_write_request &iter : regular)
    {
      LONGEST len;

      len = target_write_with_progress (current_inferior ()->top_target (),
					TARGET_OBJECT_MEMORY, NULL,
					iter.data, iter.begin,
					iter.end - iter.begin,
					progress_cb, iter.baton);
      if (len < (LONGEST) (iter.end - iter.begin))
	{
	  /* Call error?  */
	  return -1;
	}
    }

  if (!erased.empty ())
    {
      /* Erase all pages.  */
      for (const memory_write_request &iter : erased)
	target_flash_erase (iter.begin, iter.end - iter.begin);

      /* Write flash data.  */
      for (const memory_write_request &iter : flash)
	{
	  LONGEST len;

	  len = target_write_with_progress (current_inferior ()->top_target (),
					    TARGET_OBJECT_FLASH, NULL,
					    iter.data, iter.begin,
					    iter.end - iter.begin,
					    progress_cb, iter.baton);
	  if (len < (LONGEST) (iter.end - iter.begin))
	    error (_("Error writing data to flash"));
	}

      target_flash_done ();
    }

  return 0;
}
