/* Self tests of the gmp-utils API.

   Copyright (C) 2019-2024 Free Software Foundation, Inc.

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
#include "gmp-utils.h"
#include "gdbsupport/selftest.h"

#include <math.h>

namespace selftests {

/* Perform a series of general tests of gdb_mpz's as_integer method.

   This function limits itself to values which are in range (out-of-range
   values will be tested separately).  In doing so, it tries to be reasonably
   exhaustive, by testing the edges, as well as a reasonable set of values
   including negative ones, zero, and positive values.  */

static void
gdb_mpz_as_integer ()
{
  /* Test a range of values, both as LONGEST and ULONGEST.  */
  gdb_mpz v;
  LONGEST l_expected;
  ULONGEST ul_expected;

  /* Start with the smallest LONGEST  */
  l_expected = (LONGEST) 1 << (sizeof (LONGEST) * 8 - 1);

  v = gdb_mpz::pow (2, sizeof (LONGEST) * 8 - 1);
  v.negate ();

  SELF_CHECK (v.as_integer<LONGEST> () == l_expected);

  /* Try with a small range of integers including negative, zero,
     and positive values.  */
  for (int i = -256; i <= 256; i++)
    {
      l_expected = (LONGEST) i;
      v = i;
      SELF_CHECK (v.as_integer<LONGEST> () == l_expected);

      if (i >= 0)
	{
	  ul_expected = (ULONGEST) i;
	  v = ul_expected;
	  SELF_CHECK (v.as_integer<ULONGEST> () == ul_expected);
	}
    }

  /* Try with LONGEST_MAX.  */
  l_expected = LONGEST_MAX;
  ul_expected = (ULONGEST) l_expected;

  v = gdb_mpz::pow (2, sizeof (LONGEST) * 8 - 1);
  v -= 1;

  SELF_CHECK (v.as_integer<LONGEST> () == l_expected);
  SELF_CHECK (v.as_integer<ULONGEST> () == ul_expected);

  /* Try with ULONGEST_MAX.  */
  ul_expected = ULONGEST_MAX;
  v = gdb_mpz::pow (2, sizeof (LONGEST) * 8);
  v -= 1;

  SELF_CHECK (v.as_integer<ULONGEST> () == ul_expected);
}

/* A helper function which calls the given gdb_mpz object's as_integer
   method with the given type T, and verifies that this triggers
   an error due to VAL's value being out of range for type T.  */

template<typename T, typename = gdb::Requires<std::is_integral<T>>>
static void
check_as_integer_raises_out_of_range_error (const gdb_mpz &val)
{
  try
    {
      val.as_integer<T> ();
    }
  catch (const gdb_exception_error &ex)
    {
      SELF_CHECK (ex.reason == RETURN_ERROR);
      SELF_CHECK (ex.error == GENERIC_ERROR);
      SELF_CHECK (strstr (ex.what (), "Cannot export value") != nullptr);
      return;
    }
  /* The expected exception did not get raised.  */
  SELF_CHECK (false);
}

/* Perform out-of-range tests of gdb_mpz's as_integer method.

   The goal of this function is to verify that gdb_mpz::as_integer
   handles out-of-range values correctly.  */

static void
gdb_mpz_as_integer_out_of_range ()
{
  gdb_mpz v;

  /* Try LONGEST_MIN minus 1.  */
  v = gdb_mpz::pow (2, sizeof (LONGEST) * 8 - 1);
  v.negate ();
  v -= 1;

  check_as_integer_raises_out_of_range_error<ULONGEST> (v);
  check_as_integer_raises_out_of_range_error<LONGEST> (v);

  /* Try negative one (-1). */
  v = -1;

  check_as_integer_raises_out_of_range_error<ULONGEST> (v);
  SELF_CHECK (v.as_integer<LONGEST> () == (LONGEST) -1);

  /* Try LONGEST_MAX plus 1.  */
  v = LONGEST_MAX;
  v += 1;

  SELF_CHECK (v.as_integer<ULONGEST> () == (ULONGEST) LONGEST_MAX + 1);
  check_as_integer_raises_out_of_range_error<LONGEST> (v);

  /* Try ULONGEST_MAX plus 1.  */
  v = ULONGEST_MAX;
  v += 1;

  check_as_integer_raises_out_of_range_error<ULONGEST> (v);
  check_as_integer_raises_out_of_range_error<LONGEST> (v);
}

/* A helper function to store the given integer value into a buffer,
   before reading it back into a gdb_mpz.  Sets ACTUAL to the value
   read back, while at the same time setting EXPECTED as the value
   we would expect to be read back.

   Note that this function does not perform the comparison between
   EXPECTED and ACTUAL.  The caller will do it inside a SELF_CHECK
   call, allowing the line information shown when the test fails
   to provide a bit more information about the kind of values
   that were used when the check failed.  This makes the writing
   of the tests a little more verbose, but the debugging in case
   of problems should hopefuly be easier.  */

template<typename T>
void
store_and_read_back (T val, size_t buf_len, enum bfd_endian byte_order,
		     gdb_mpz &expected, gdb_mpz &actual)
{
  gdb_byte *buf;

  expected = val;

  buf = (gdb_byte *) alloca (buf_len);
  store_integer (buf, buf_len, byte_order, val);

  /* Pre-initialize ACTUAL to something that's not the expected value.  */
  actual = expected;
  actual -= 500;

  actual.read ({buf, buf_len}, byte_order, !std::is_signed<T>::value);
}

/* Test the gdb_mpz::read method over a reasonable range of values.

   The testing is done by picking an arbitrary buffer length, after
   which we test every possible value that this buffer allows, both
   with signed numbers as well as unsigned ones.  */

static void
gdb_mpz_read_all_from_small ()
{
  /* Start with a type whose size is small enough that we can afford
     to check the complete range.  */

  int buf_len = 1;
  LONGEST l_min = -pow (2.0, buf_len * 8 - 1);
  LONGEST l_max = pow (2.0, buf_len * 8 - 1) - 1;

  for (LONGEST l = l_min; l <= l_max; l++)
    {
      gdb_mpz expected, actual;

      store_and_read_back (l, buf_len, BFD_ENDIAN_BIG, expected, actual);
      SELF_CHECK (actual ==  expected);

      store_and_read_back (l, buf_len, BFD_ENDIAN_LITTLE, expected, actual);
      SELF_CHECK (actual == expected);
    }

  /* Do the same as above, but with an unsigned type.  */
  ULONGEST ul_min = 0;
  ULONGEST ul_max = pow (2.0, buf_len * 8) - 1;

  for (ULONGEST ul = ul_min; ul <= ul_max; ul++)
    {
      gdb_mpz expected, actual;

      store_and_read_back (ul, buf_len, BFD_ENDIAN_BIG, expected, actual);
      SELF_CHECK (actual == expected);

      store_and_read_back (ul, buf_len, BFD_ENDIAN_LITTLE, expected, actual);
      SELF_CHECK (actual == expected);
    }
}

/* Test the gdb_mpz::read the extremes of LONGEST and ULONGEST.  */

static void
gdb_mpz_read_min_max ()
{
  gdb_mpz expected, actual;

  /* Start with the smallest LONGEST.  */

  LONGEST l_min = (LONGEST) 1 << (sizeof (LONGEST) * 8 - 1);

  store_and_read_back (l_min, sizeof (LONGEST), BFD_ENDIAN_BIG,
		       expected, actual);
  SELF_CHECK (actual == expected);

  store_and_read_back (l_min, sizeof (LONGEST), BFD_ENDIAN_LITTLE,
		       expected, actual);
  SELF_CHECK (actual == expected);

  /* Same with LONGEST_MAX.  */

  LONGEST l_max = LONGEST_MAX;

  store_and_read_back (l_max, sizeof (LONGEST), BFD_ENDIAN_BIG,
		       expected, actual);
  SELF_CHECK (actual == expected);

  store_and_read_back (l_max, sizeof (LONGEST), BFD_ENDIAN_LITTLE,
		       expected, actual);
  SELF_CHECK (actual == expected);

  /* Same with the smallest ULONGEST.  */

  ULONGEST ul_min = 0;

  store_and_read_back (ul_min, sizeof (ULONGEST), BFD_ENDIAN_BIG,
		       expected, actual);
  SELF_CHECK (actual == expected);

  store_and_read_back (ul_min, sizeof (ULONGEST), BFD_ENDIAN_LITTLE,
		       expected, actual);
  SELF_CHECK (actual == expected);

  /* Same with ULONGEST_MAX.  */

  ULONGEST ul_max = ULONGEST_MAX;

  store_and_read_back (ul_max, sizeof (ULONGEST), BFD_ENDIAN_BIG,
		       expected, actual);
  SELF_CHECK (actual == expected);

  store_and_read_back (ul_max, sizeof (ULONGEST), BFD_ENDIAN_LITTLE,
		       expected, actual);
  SELF_CHECK (actual == expected);
}

/* A helper function which creates a gdb_mpz object from the given
   integer VAL, and then writes it using its gdb_mpz::write method.

   The written value is then extracted from the buffer and returned,
   for comparison with the original.

   Note that this function does not perform the comparison between
   VAL and the returned value.  The caller will do it inside a SELF_CHECK
   call, allowing the line information shown when the test fails
   to provide a bit more information about the kind of values
   that were used when the check failed.  This makes the writing
   of the tests a little more verbose, but the debugging in case
   of problems should hopefuly be easier.  */

template<typename T>
T
write_and_extract (T val, size_t buf_len, enum bfd_endian byte_order)
{
  gdb_mpz v (val);

  SELF_CHECK (v.as_integer<T> () == val);

  gdb_byte *buf = (gdb_byte *) alloca (buf_len);
  v.write ({buf, buf_len}, byte_order, !std::is_signed<T>::value);

  return extract_integer<T> ({buf, buf_len}, byte_order);
}

/* Test the gdb_mpz::write method over a reasonable range of values.

   The testing is done by picking an arbitrary buffer length, after
   which we test every possible value that this buffer allows.  */

static void
gdb_mpz_write_all_from_small ()
{
  int buf_len = 1;
  LONGEST l_min = -pow (2.0, buf_len * 8 - 1);
  LONGEST l_max = pow (2.0, buf_len * 8 - 1) - 1;

  for (LONGEST l = l_min; l <= l_max; l++)
    {
      SELF_CHECK (write_and_extract (l, buf_len, BFD_ENDIAN_BIG) == l);
      SELF_CHECK (write_and_extract (l, buf_len, BFD_ENDIAN_LITTLE) == l);
    }

    /* Do the same as above, but with an unsigned type.  */
  ULONGEST ul_min = 0;
  ULONGEST ul_max = pow (2.0, buf_len * 8) - 1;

  for (ULONGEST ul = ul_min; ul <= ul_max; ul++)
    {
      SELF_CHECK (write_and_extract (ul, buf_len, BFD_ENDIAN_BIG) == ul);
      SELF_CHECK (write_and_extract (ul, buf_len, BFD_ENDIAN_LITTLE) == ul);
    }
}

/* Test the gdb_mpz::write the extremes of LONGEST and ULONGEST.  */

static void
gdb_mpz_write_min_max ()
{
  /* Start with the smallest LONGEST.  */

  LONGEST l_min = (LONGEST) 1 << (sizeof (LONGEST) * 8 - 1);
  SELF_CHECK (write_and_extract (l_min, sizeof (LONGEST), BFD_ENDIAN_BIG)
	      == l_min);
  SELF_CHECK (write_and_extract (l_min, sizeof (LONGEST), BFD_ENDIAN_LITTLE)
	      == l_min);

  /* Same with LONGEST_MAX.  */

  LONGEST l_max = LONGEST_MAX;
  SELF_CHECK (write_and_extract (l_max, sizeof (LONGEST), BFD_ENDIAN_BIG)
	      == l_max);
  SELF_CHECK (write_and_extract (l_max, sizeof (LONGEST), BFD_ENDIAN_LITTLE)
	      == l_max);

  /* Same with the smallest ULONGEST.  */

  ULONGEST ul_min = (ULONGEST) 1 << (sizeof (ULONGEST) * 8 - 1);
  SELF_CHECK (write_and_extract (ul_min, sizeof (ULONGEST), BFD_ENDIAN_BIG)
	      == ul_min);
  SELF_CHECK (write_and_extract (ul_min, sizeof (ULONGEST), BFD_ENDIAN_LITTLE)
	      == ul_min);

  /* Same with ULONGEST_MAX.  */

  ULONGEST ul_max = ULONGEST_MAX;
  SELF_CHECK (write_and_extract (ul_max, sizeof (ULONGEST), BFD_ENDIAN_BIG)
	      == ul_max);
  SELF_CHECK (write_and_extract (ul_max, sizeof (ULONGEST), BFD_ENDIAN_LITTLE)
	      == ul_max);
}

/* A helper function which stores the signed number, the unscaled value
   of a fixed point object, into a buffer, and then uses gdb_mpq's
   read_fixed_point to read it as a fixed_point value, with
   the given parameters.

   EXPECTED is set to the value we expected to get after the call
   to read_fixed_point.  ACTUAL is the value we actually do get.

   Note that this function does not perform the comparison between
   EXPECTED and ACTUAL.  The caller will do it inside a SELF_CHECK
   call, allowing the line information shown when the test fails
   to provide a bit more information about the kind of values
   that were used when the check failed.  This makes the writing
   of the tests a little more verbose, but the debugging in case
   of problems should hopefuly be easier.  */

static void
read_fp_test (int unscaled, const gdb_mpq &scaling_factor,
	      enum bfd_endian byte_order,
	      gdb_mpq &expected, gdb_mpq &actual)
{
  /* For this kind of testing, we'll use a buffer the same size as
     our unscaled parameter.  */
  const size_t len = sizeof (unscaled);
  gdb_byte buf[len];
  store_signed_integer (buf, len, byte_order, unscaled);

  actual.read_fixed_point ({buf, len}, byte_order, 0, scaling_factor);

  expected = gdb_mpq (unscaled, 1);
  expected *= scaling_factor;
}

/* Perform various tests of the gdb_mpq::read_fixed_point method.  */

static void
gdb_mpq_read_fixed_point ()
{
  gdb_mpq expected, actual;

  /* Pick an arbitrary scaling_factor; this operation is trivial enough
     thanks to GMP that the value we use isn't really important.  */
  gdb_mpq scaling_factor (3, 5);

  /* Try a few values, both negative and positive... */

  read_fp_test (-256, scaling_factor, BFD_ENDIAN_BIG, expected, actual);
  SELF_CHECK (actual == expected);
  read_fp_test (-256, scaling_factor, BFD_ENDIAN_LITTLE, expected, actual);
  SELF_CHECK (actual == expected);

  read_fp_test (-1, scaling_factor, BFD_ENDIAN_BIG, expected, actual);
  SELF_CHECK (actual == expected);
  read_fp_test (-1, scaling_factor, BFD_ENDIAN_LITTLE, expected, actual);
  SELF_CHECK (actual == expected);

  read_fp_test (0, scaling_factor, BFD_ENDIAN_BIG, expected, actual);
  SELF_CHECK (actual == expected);
  read_fp_test (0, scaling_factor, BFD_ENDIAN_LITTLE, expected, actual);
  SELF_CHECK (actual == expected);

  read_fp_test (1, scaling_factor, BFD_ENDIAN_BIG, expected, actual);
  SELF_CHECK (actual == expected);
  read_fp_test (1, scaling_factor, BFD_ENDIAN_LITTLE, expected, actual);
  SELF_CHECK (actual == expected);

  read_fp_test (1025, scaling_factor, BFD_ENDIAN_BIG, expected, actual);
  SELF_CHECK (actual == expected);
  read_fp_test (1025, scaling_factor, BFD_ENDIAN_LITTLE, expected, actual);
  SELF_CHECK (actual == expected);
}

/* A helper function which builds a gdb_mpq object from the given
   NUMERATOR and DENOMINATOR, and then calls gdb_mpq's write_fixed_point
   method to write it to a buffer.

   The value written into the buffer is then read back as is,
   and returned.  */

static LONGEST
write_fp_test (int numerator, unsigned int denominator,
	       const gdb_mpq &scaling_factor,
	       enum bfd_endian byte_order)
{
  /* For this testing, we'll use a buffer the size of LONGEST.
     This is really an arbitrary decision, as long as the buffer
     is long enough to hold the unscaled values that we'll be
     writing.  */
  const size_t len = sizeof (LONGEST);
  gdb_byte buf[len];
  memset (buf, 0, len);

  gdb_mpq v (numerator, denominator);
  v.write_fixed_point ({buf, len}, byte_order, 0, scaling_factor);

  return extract_unsigned_integer (buf, len, byte_order);
}

/* Perform various tests of the gdb_mpq::write_fixed_point method.  */

static void
gdb_mpq_write_fixed_point ()
{
  /* Pick an arbitrary factor; this operations is sufficiently trivial
     with the use of GMP that the value of this factor is not really
     all that important.  */
  gdb_mpq scaling_factor (1, 3);

  gdb_mpq vq;

  /* Try a few multiples of the scaling factor, both negative,
     and positive... */

  SELF_CHECK (write_fp_test (-8, 1, scaling_factor, BFD_ENDIAN_BIG) == -24);
  SELF_CHECK (write_fp_test (-8, 1, scaling_factor, BFD_ENDIAN_LITTLE) == -24);

  SELF_CHECK (write_fp_test (-2, 3, scaling_factor, BFD_ENDIAN_BIG) == -2);
  SELF_CHECK (write_fp_test (-2, 3, scaling_factor, BFD_ENDIAN_LITTLE) == -2);

  SELF_CHECK (write_fp_test (0, 3, scaling_factor, BFD_ENDIAN_BIG) == 0);
  SELF_CHECK (write_fp_test (0, 3, scaling_factor, BFD_ENDIAN_LITTLE) == 0);

  SELF_CHECK (write_fp_test (5, 3, scaling_factor, BFD_ENDIAN_BIG) == 5);
  SELF_CHECK (write_fp_test (5, 3, scaling_factor, BFD_ENDIAN_LITTLE) == 5);
}

}

void _initialize_gmp_utils_selftests ();

void
_initialize_gmp_utils_selftests ()
{
  selftests::register_test ("gdb_mpz_as_integer",
			    selftests::gdb_mpz_as_integer);
  selftests::register_test ("gdb_mpz_as_integer_out_of_range",
			    selftests::gdb_mpz_as_integer_out_of_range);
  selftests::register_test ("gdb_mpz_read_all_from_small",
			    selftests::gdb_mpz_read_all_from_small);
  selftests::register_test ("gdb_mpz_read_min_max",
			    selftests::gdb_mpz_read_min_max);
  selftests::register_test ("gdb_mpz_write_all_from_small",
			    selftests::gdb_mpz_write_all_from_small);
  selftests::register_test ("gdb_mpz_write_min_max",
			    selftests::gdb_mpz_write_min_max);
  selftests::register_test ("gdb_mpq_read_fixed_point",
			    selftests::gdb_mpq_read_fixed_point);
  selftests::register_test ("gdb_mpq_write_fixed_point",
			    selftests::gdb_mpq_write_fixed_point);
}
