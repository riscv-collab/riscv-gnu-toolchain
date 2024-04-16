/* Low-level DWARF 2 reading code

   Copyright (C) 1994-2024 Free Software Foundation, Inc.

   Adapted by Gary Funck (gary@intrepid.com), Intrepid Technology,
   Inc.  with support from Florida State University (under contract
   with the Ada Joint Program Office), and Silicon Graphics, Inc.
   Initial contribution by Brent Benson, Harris Computer Systems, Inc.,
   based on Fred Fish's (Cygnus Support) implementation of DWARF 1
   support.

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

#ifndef GDB_DWARF2_LEB_H
#define GDB_DWARF2_LEB_H

/* Read dwarf information from a buffer.  */

static inline unsigned int
read_1_byte (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_8 (abfd, buf);
}

static inline int
read_1_signed_byte (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_signed_8 (abfd, buf);
}

static inline unsigned int
read_2_bytes (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_16 (abfd, buf);
}

static inline int
read_2_signed_bytes (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_signed_16 (abfd, buf);
}

/* Read the next three bytes (little-endian order) as an unsigned integer.  */
static inline unsigned int
read_3_bytes (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_24 (abfd, buf);
}

static inline unsigned int
read_4_bytes (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_32 (abfd, buf);
}

static inline int
read_4_signed_bytes (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_signed_32 (abfd, buf);
}

static inline ULONGEST
read_8_bytes (bfd *abfd, const gdb_byte *buf)
{
  return bfd_get_64 (abfd, buf);
}

extern LONGEST read_signed_leb128 (bfd *, const gdb_byte *, unsigned int *);

extern ULONGEST read_unsigned_leb128 (bfd *, const gdb_byte *, unsigned int *);

/* Read the initial length from a section.  The (draft) DWARF 3
   specification allows the initial length to take up either 4 bytes
   or 12 bytes.  If the first 4 bytes are 0xffffffff, then the next 8
   bytes describe the length and all offsets will be 8 bytes in length
   instead of 4.

   An older, non-standard 64-bit format is also handled by this
   function.  The older format in question stores the initial length
   as an 8-byte quantity without an escape value.  Lengths greater
   than 2^32 aren't very common which means that the initial 4 bytes
   is almost always zero.  Since a length value of zero doesn't make
   sense for the 32-bit format, this initial zero can be considered to
   be an escape value which indicates the presence of the older 64-bit
   format.  As written, the code can't detect (old format) lengths
   greater than 4GB.  If it becomes necessary to handle lengths
   somewhat larger than 4GB, we could allow other small values (such
   as the non-sensical values of 1, 2, and 3) to also be used as
   escape values indicating the presence of the old format.

   The value returned via bytes_read should be used to increment the
   relevant pointer after calling read_initial_length().

   [ Note:  read_initial_length() and read_offset() are based on the
     document entitled "DWARF Debugging Information Format", revision
     3, draft 8, dated November 19, 2001.  This document was obtained
     from:

	http://reality.sgiweb.org/davea/dwarf3-draft8-011125.pdf

     This document is only a draft and is subject to change.  (So beware.)

     Details regarding the older, non-standard 64-bit format were
     determined empirically by examining 64-bit ELF files produced by
     the SGI toolchain on an IRIX 6.5 machine.

     - Kevin, July 16, 2002
   ] */
extern LONGEST read_initial_length (bfd *abfd, const gdb_byte *buf,
				    unsigned int *bytes_read,
				    bool handle_nonstd = true);

/* Read an offset from the data stream.  */
extern LONGEST read_offset (bfd *abfd, const gdb_byte *buf,
			    unsigned int offset_size);

static inline const gdb_byte *
read_n_bytes (bfd *abfd, const gdb_byte *buf, unsigned int size)
{
  /* If the size of a host char is 8 bits, we can return a pointer
     to the buffer, otherwise we have to copy the data to a buffer
     allocated on the temporary obstack.  */
  gdb_assert (HOST_CHAR_BIT == 8);
  return buf;
}

static inline const char *
read_direct_string (bfd *abfd, const gdb_byte *buf,
		    unsigned int *bytes_read_ptr)
{
  /* If the size of a host char is 8 bits, we can return a pointer
     to the string, otherwise we have to copy the string to a buffer
     allocated on the temporary obstack.  */
  gdb_assert (HOST_CHAR_BIT == 8);
  if (*buf == '\0')
    {
      *bytes_read_ptr = 1;
      return NULL;
    }
  *bytes_read_ptr = strlen ((const char *) buf) + 1;
  return (const char *) buf;
}

#endif /* GDB_DWARF2_LEB_H */
