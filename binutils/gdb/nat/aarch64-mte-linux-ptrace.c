/* Common Linux native ptrace code for AArch64 MTE.

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

#include "gdbsupport/common-defs.h"
#include "gdbsupport/byte-vector.h"

#include "linux-ptrace.h"

#include "arch/aarch64.h"
#include "arch/aarch64-mte-linux.h"
#include "nat/aarch64-linux.h"
#include "nat/aarch64-mte-linux-ptrace.h"

#include <sys/uio.h>

/* Helper function to display various possible errors when reading
   MTE tags.  */

static void ATTRIBUTE_NORETURN
aarch64_mte_linux_peek_error (int error)
{
  switch (error)
    {
    case EIO:
      perror_with_name (_("PEEKMTETAGS not supported"));
      break;
    case EFAULT:
      perror_with_name (_("Couldn't fetch allocation tags"));
      break;
    case EOPNOTSUPP:
      perror_with_name (_("PROT_MTE not enabled for requested address"));
    default:
      perror_with_name (_("Unknown MTE error"));
      break;
    }
}

/* Helper function to display various possible errors when writing
   MTE tags.  */

static void ATTRIBUTE_NORETURN
aarch64_mte_linux_poke_error (int error)
{
  switch (error)
    {
    case EIO:
      perror_with_name (_("POKEMTETAGS not supported"));
      break;
    case EFAULT:
      perror_with_name (_("Couldn't store allocation tags"));
      break;
    case EOPNOTSUPP:
      perror_with_name (_("PROT_MTE not enabled for requested address"));
    default:
      perror_with_name (_("Unknown MTE error"));
      break;
    }
}

/* Helper to prepare a vector of tags to be passed on to the kernel.  The
   main purpose of this function is to optimize the number of calls to
   ptrace if we're writing too many tags at once, like a pattern fill
   request.

   Return a vector of tags of up to MAX_SIZE size, containing the tags that
   must be passed on to the kernel, extracted from TAGS, starting at POS.
   GRANULES is the number of tag granules to be modified.  */

static gdb::byte_vector
prepare_tag_vector (size_t granules, const gdb::byte_vector &tags, size_t pos,
		    size_t max_size)
{
  gdb::byte_vector t;

  if (granules == 0)
    return t;

  gdb_assert (tags.size () > 0 && max_size > 0);

  if (granules > AARCH64_MTE_TAGS_MAX_SIZE)
    t.resize (AARCH64_MTE_TAGS_MAX_SIZE);
  else
    t.resize (granules);

  size_t tag_count = tags.size ();

  for (size_t i = 0; i < t.size (); i++)
    t[i] = tags[(pos + i) % tag_count];

  return t;
}

/* See nat/aarch64-mte-linux-ptrace.h */

bool
aarch64_mte_fetch_memtags (int tid, CORE_ADDR address, size_t len,
			   gdb::byte_vector &tags)
{
  size_t ntags = aarch64_mte_get_tag_granules (address, len,
					       AARCH64_MTE_GRANULE_SIZE);

  /* If the memory range contains no tags, nothing left to do.  */
  if (ntags == 0)
    return true;

  gdb_byte tagbuf[ntags];

  struct iovec iovec;
  iovec.iov_base = tagbuf;
  iovec.iov_len = ntags;

  tags.clear ();
  bool done_reading = false;

  /* The kernel may return less tags than we requested.  Loop until we've read
     all the requested tags or until we get an error.  */
  while (!done_reading)
    {
      /* Attempt to read ntags allocation tags from the kernel.  */
      if (ptrace (PTRACE_PEEKMTETAGS, tid, address, &iovec) < 0)
	aarch64_mte_linux_peek_error (errno);

      /* Make sure the kernel returned at least one tag.  */
      if (iovec.iov_len <= 0)
	{
	  tags.clear ();
	  return false;
	}

      /* Copy the tags the kernel returned.  */
      for (size_t i = 0; i < iovec.iov_len; i++)
	tags.push_back (tagbuf[i]);

      /* Are we done reading tags?  */
      if (tags.size () == ntags)
	done_reading = true;
      else
	{
	  address += iovec.iov_len * AARCH64_MTE_GRANULE_SIZE;
	  iovec.iov_len = ntags - iovec.iov_len;
	}
    }
  return true;
}

/* See nat/aarch64-mte-linux-ptrace.h */

bool
aarch64_mte_store_memtags (int tid, CORE_ADDR address, size_t len,
			   const gdb::byte_vector &tags)
{
  if (tags.size () == 0)
    return true;

  /* Get the number of tags we need to write.  */
  size_t ntags = aarch64_mte_get_tag_granules (address, len,
					       AARCH64_MTE_GRANULE_SIZE);

  /* If the memory range contains no tags, nothing left to do.  */
  if (ntags == 0)
    return true;

  bool done_writing = false;
  size_t tags_written = 0;

  /* Write all the tags, AARCH64_MTE_TAGS_MAX_SIZE blocks at a time.  */
  while (!done_writing)
    {
      gdb::byte_vector t = prepare_tag_vector (ntags - tags_written, tags,
					       tags_written,
					       AARCH64_MTE_TAGS_MAX_SIZE);

      struct iovec iovec;
      iovec.iov_base = t.data ();
      iovec.iov_len = t.size ();

      /* Request the kernel to update the allocation tags.  */
      if (ptrace (PTRACE_POKEMTETAGS, tid, address, &iovec) < 0)
	aarch64_mte_linux_poke_error (errno);

      /* Make sure the kernel wrote at least one tag.  */
      if (iovec.iov_len <= 0)
	return false;

      tags_written += iovec.iov_len;

      /* Are we done writing tags?  */
      if (tags_written == ntags)
	done_writing = true;
      else
	address += iovec.iov_len * AARCH64_MTE_GRANULE_SIZE;
    }

  return true;
}
