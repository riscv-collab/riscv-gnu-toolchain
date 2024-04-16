/* String reading

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

#include "gdbsupport/common-defs.h"
#include "target/target.h"

/* Read LEN bytes of target memory at address MEMADDR, placing the
   results in GDB's memory at MYADDR.  Returns a count of the bytes
   actually read, and optionally a target_xfer_status value in the
   location pointed to by ERRPTR if ERRPTR is non-null.  */

static int
partial_memory_read (CORE_ADDR memaddr, gdb_byte *myaddr,
		     int len, int *errptr)
{
  int nread;			/* Number of bytes actually read.  */
  int errcode;			/* Error from last read.  */

  /* First try a complete read.  */
  errcode = target_read_memory (memaddr, myaddr, len);
  if (errcode == 0)
    {
      /* Got it all.  */
      nread = len;
    }
  else
    {
      /* Loop, reading one byte at a time until we get as much as we can.  */
      for (errcode = 0, nread = 0; len > 0 && errcode == 0; nread++, len--)
	{
	  errcode = target_read_memory (memaddr++, myaddr++, 1);
	}
      /* If an error, the last read was unsuccessful, so adjust count.  */
      if (errcode != 0)
	{
	  nread--;
	}
    }
  if (errptr != NULL)
    {
      *errptr = errcode;
    }
  return (nread);
}

/* See target/target.h.  */

int
target_read_string (CORE_ADDR addr, int len, int width,
		    unsigned int fetchlimit,
		    gdb::unique_xmalloc_ptr<gdb_byte> *buffer,
		    int *bytes_read)
{
  int errcode;			/* Errno returned from bad reads.  */
  unsigned int nfetch;		/* Chars to fetch / chars fetched.  */
  gdb_byte *bufptr;		/* Pointer to next available byte in
				   buffer.  */

  /* Loop until we either have all the characters, or we encounter
     some error, such as bumping into the end of the address space.  */

  buffer->reset (nullptr);

  if (len > 0)
    {
      /* We want fetchlimit chars, so we might as well read them all in
	 one operation.  */
      unsigned int fetchlen = std::min ((unsigned) len, fetchlimit);

      buffer->reset ((gdb_byte *) xmalloc (fetchlen * width));
      bufptr = buffer->get ();

      nfetch = partial_memory_read (addr, bufptr, fetchlen * width, &errcode)
	/ width;
      addr += nfetch * width;
      bufptr += nfetch * width;
    }
  else if (len == -1)
    {
      unsigned long bufsize = 0;
      unsigned int chunksize;	/* Size of each fetch, in chars.  */
      int found_nul;		/* Non-zero if we found the nul char.  */
      gdb_byte *limit;		/* First location past end of fetch buffer.  */

      found_nul = 0;
      /* We are looking for a NUL terminator to end the fetching, so we
	 might as well read in blocks that are large enough to be efficient,
	 but not so large as to be slow if fetchlimit happens to be large.
	 So we choose the minimum of 8 and fetchlimit.  We used to use 200
	 instead of 8 but 200 is way too big for remote debugging over a
	  serial line.  */
      chunksize = std::min (8u, fetchlimit);

      do
	{
	  nfetch = std::min ((unsigned long) chunksize, fetchlimit - bufsize);

	  if (*buffer == NULL)
	    buffer->reset ((gdb_byte *) xmalloc (nfetch * width));
	  else
	    buffer->reset ((gdb_byte *) xrealloc (buffer->release (),
						  (nfetch + bufsize) * width));

	  bufptr = buffer->get () + bufsize * width;
	  bufsize += nfetch;

	  /* Read as much as we can.  */
	  nfetch = partial_memory_read (addr, bufptr, nfetch * width, &errcode)
		    / width;

	  /* Scan this chunk for the null character that terminates the string
	     to print.  If found, we don't need to fetch any more.  Note
	     that bufptr is explicitly left pointing at the next character
	     after the null character, or at the next character after the end
	     of the buffer.  */

	  limit = bufptr + nfetch * width;
	  while (bufptr < limit)
	    {
	      bool found_nonzero = false;

	      for (int i = 0; !found_nonzero && i < width; ++i)
		if (bufptr[i] != 0)
		  found_nonzero = true;

	      addr += width;
	      bufptr += width;
	      if (!found_nonzero)
		{
		  /* We don't care about any error which happened after
		     the NUL terminator.  */
		  errcode = 0;
		  found_nul = 1;
		  break;
		}
	    }
	}
      while (errcode == 0	/* no error */
	     && bufptr - buffer->get () < fetchlimit * width	/* no overrun */
	     && !found_nul);	/* haven't found NUL yet */
    }
  else
    {				/* Length of string is really 0!  */
      /* We always allocate *buffer.  */
      buffer->reset ((gdb_byte *) xmalloc (1));
      bufptr = buffer->get ();
      errcode = 0;
    }

  /* bufptr and addr now point immediately beyond the last byte which we
     consider part of the string (including a '\0' which ends the string).  */
  *bytes_read = bufptr - buffer->get ();

  return errcode;
}

/* See target/target.h.  */

gdb::unique_xmalloc_ptr<char>
target_read_string (CORE_ADDR memaddr, int len, int *bytes_read)
{
  gdb::unique_xmalloc_ptr<gdb_byte> buffer;

  int ignore;
  if (bytes_read == nullptr)
    bytes_read = &ignore;

  /* Note that the endian-ness does not matter here.  */
  int errcode = target_read_string (memaddr, -1, 1, len, &buffer, bytes_read);
  if (errcode != 0)
    return {};

  return gdb::unique_xmalloc_ptr<char> ((char *) buffer.release ());
}

/* See target/target.h.  */

std::string
to_string (gdb_thread_options options)
{
  static constexpr gdb_thread_options::string_mapping mapping[] = {
    MAP_ENUM_FLAG (GDB_THREAD_OPTION_CLONE),
    MAP_ENUM_FLAG (GDB_THREAD_OPTION_EXIT),
  };
  return options.to_string (mapping);
}
