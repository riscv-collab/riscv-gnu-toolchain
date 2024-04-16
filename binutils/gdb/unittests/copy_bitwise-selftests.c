/* Self tests of the copy_bitwise routine for GDB, the GNU debugger.

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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
#include "gdbsupport/selftest.h"
#include "utils.h"

namespace selftests {

/* Helper function for the unit test of copy_bitwise.  Convert NBITS bits
   out of BITS, starting at OFFS, to the respective '0'/'1'-string.  MSB0
   specifies whether to assume big endian bit numbering.  Store the
   resulting (not null-terminated) string at STR.  */

static void
bits_to_str (char *str, const gdb_byte *bits, ULONGEST offs,
	     ULONGEST nbits, int msb0)
{
  unsigned int j;
  size_t i;

  for (i = offs / 8, j = offs % 8; nbits; i++, j = 0)
    {
      unsigned int ch = bits[i];
      for (; j < 8 && nbits; j++, nbits--)
	*str++ = (ch & (msb0 ? (1 << (7 - j)) : (1 << j))) ? '1' : '0';
    }
}

/* Check one invocation of copy_bitwise with the given parameters.  */

static void
check_copy_bitwise (const gdb_byte *dest, unsigned int dest_offset,
		    const gdb_byte *source, unsigned int source_offset,
		    unsigned int nbits, int msb0)
{
  size_t len = align_up (dest_offset + nbits, 8);
  char *expected = (char *) alloca (len + 1);
  char *actual = (char *) alloca (len + 1);
  gdb_byte *buf = (gdb_byte *) alloca (len / 8);

  /* Compose a '0'/'1'-string that represents the expected result of
     copy_bitwise below:
      Bits from [0, DEST_OFFSET) are filled from DEST.
      Bits from [DEST_OFFSET, DEST_OFFSET + NBITS) are filled from SOURCE.
      Bits from [DEST_OFFSET + NBITS, LEN) are filled from DEST.

     E.g., with:
      dest_offset: 4
      nbits:       2
      len:         8
      dest:        00000000
      source:      11111111

     We should end up with:
      buf:         00001100
		   DDDDSSDD (D=dest, S=source)
  */
  bits_to_str (expected, dest, 0, len, msb0);
  bits_to_str (expected + dest_offset, source, source_offset, nbits, msb0);

  /* Fill BUF with data from DEST, apply copy_bitwise, and convert the
     result to a '0'/'1'-string.  */
  memcpy (buf, dest, len / 8);
  copy_bitwise (buf, dest_offset, source, source_offset, nbits, msb0);
  bits_to_str (actual, buf, 0, len, msb0);

  /* Compare the resulting strings.  */
  expected[len] = actual[len] = '\0';
  if (strcmp (expected, actual) != 0)
    error (_("copy_bitwise %s != %s (%u+%u -> %u)"),
	   expected, actual, source_offset, nbits, dest_offset);
}

/* Unit test for copy_bitwise.  */

static void
copy_bitwise_tests (void)
{
  /* Data to be used as both source and destination buffers.  The two
     arrays below represent the lsb0- and msb0- encoded versions of the
     following bit string, respectively:
       00000000 00011111 11111111 01001000 10100101 11110010
     This pattern is chosen such that it contains:
     - constant 0- and 1- chunks of more than a full byte;
     - 0/1- and 1/0 transitions on all bit positions within a byte;
     - several sufficiently asymmetric bytes.
  */
  static const gdb_byte data_lsb0[] = {
    0x00, 0xf8, 0xff, 0x12, 0xa5, 0x4f
  };
  static const gdb_byte data_msb0[] = {
    0x00, 0x1f, 0xff, 0x48, 0xa5, 0xf2
  };

  constexpr size_t data_nbits = 8 * sizeof (data_lsb0);
  constexpr unsigned max_nbits = 24;

  /* Try all combinations of:
      lsb0/msb0 bit order (using the respective data array)
       X [0, MAX_NBITS] copy bit width
       X feasible source offsets for the given copy bit width
       X feasible destination offsets
  */
  for (int msb0 = 0; msb0 < 2; msb0++)
    {
      const gdb_byte *data = msb0 ? data_msb0 : data_lsb0;

      for (unsigned int nbits = 1; nbits <= max_nbits; nbits++)
	{
	  const unsigned int max_offset = data_nbits - nbits;

	  for (unsigned source_offset = 0;
	       source_offset <= max_offset;
	       source_offset++)
	    {
	      for (unsigned dest_offset = 0;
		   dest_offset <= max_offset;
		   dest_offset++)
		{
		  check_copy_bitwise (data + dest_offset / 8,
				      dest_offset % 8,
				      data + source_offset / 8,
				      source_offset % 8,
				      nbits, msb0);
		}
	    }
	}

      /* Special cases: copy all, copy nothing.  */
      check_copy_bitwise (data_lsb0, 0, data_msb0, 0, data_nbits, msb0);
      check_copy_bitwise (data_msb0, 0, data_lsb0, 0, data_nbits, msb0);
      check_copy_bitwise (data, data_nbits - 7, data, 9, 0, msb0);
    }
}

} /* namespace selftests */

void _initialize_copy_bitwise_utils_selftests ();
void
_initialize_copy_bitwise_utils_selftests ()
{
  selftests::register_test ("copy_bitwise", selftests::copy_bitwise_tests);
}
