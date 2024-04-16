/* Target memory searching

   Copyright (C) 2020-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"

#include "gdbsupport/search.h"
#include "gdbsupport/byte-vector.h"

/* This implements a basic search of memory, reading target memory and
   performing the search here (as opposed to performing the search in on the
   target side with, for example, gdbserver).  */

int
simple_search_memory
  (gdb::function_view<target_read_memory_ftype> read_memory,
   CORE_ADDR start_addr, ULONGEST search_space_len,
   const gdb_byte *pattern, ULONGEST pattern_len,
   CORE_ADDR *found_addrp)
{
  const unsigned chunk_size = SEARCH_CHUNK_SIZE;
  /* Buffer to hold memory contents for searching.  */
  unsigned search_buf_size;

  search_buf_size = chunk_size + pattern_len - 1;

  /* No point in trying to allocate a buffer larger than the search space.  */
  if (search_space_len < search_buf_size)
    search_buf_size = search_space_len;

  gdb::byte_vector search_buf (search_buf_size);

  /* Prime the search buffer.  */

  if (!read_memory (start_addr, search_buf.data (), search_buf_size))
    {
      warning (_("Unable to access %s bytes of target "
		 "memory at %s, halting search."),
	       pulongest (search_buf_size), hex_string (start_addr));
      return -1;
    }

  /* Perform the search.

     The loop is kept simple by allocating [N + pattern-length - 1] bytes.
     When we've scanned N bytes we copy the trailing bytes to the start and
     read in another N bytes.  */

  while (search_space_len >= pattern_len)
    {
      gdb_byte *found_ptr;
      unsigned nr_search_bytes
	= std::min (search_space_len, (ULONGEST) search_buf_size);

      found_ptr = (gdb_byte *) memmem (search_buf.data (), nr_search_bytes,
				       pattern, pattern_len);

      if (found_ptr != NULL)
	{
	  CORE_ADDR found_addr = start_addr + (found_ptr - search_buf.data ());

	  *found_addrp = found_addr;
	  return 1;
	}

      /* Not found in this chunk, skip to next chunk.  */

      /* Don't let search_space_len wrap here, it's unsigned.  */
      if (search_space_len >= chunk_size)
	search_space_len -= chunk_size;
      else
	search_space_len = 0;

      if (search_space_len >= pattern_len)
	{
	  unsigned keep_len = search_buf_size - chunk_size;
	  CORE_ADDR read_addr = start_addr + chunk_size + keep_len;
	  int nr_to_read;

	  /* Copy the trailing part of the previous iteration to the front
	     of the buffer for the next iteration.  */
	  gdb_assert (keep_len == pattern_len - 1);
	  if (keep_len > 0)
	    memcpy (&search_buf[0], &search_buf[chunk_size], keep_len);

	  nr_to_read = std::min (search_space_len - keep_len,
				 (ULONGEST) chunk_size);

	  if (!read_memory (read_addr, &search_buf[keep_len], nr_to_read))
	    {
	      warning (_("Unable to access %s bytes of target "
			 "memory at %s, halting search."),
		       plongest (nr_to_read),
		       hex_string (read_addr));
	      return -1;
	    }

	  start_addr += chunk_size;
	}
    }

  /* Not found.  */

  return 0;
}
