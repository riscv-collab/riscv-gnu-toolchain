/* Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

/* See gmp-utils.h.  */

std::string
gmp_string_printf (const char *fmt, ...)
{
  va_list vp;

  va_start (vp, fmt);
  int size = gmp_vsnprintf (NULL, 0, fmt, vp);
  va_end (vp);

  std::string str (size, '\0');

  /* C++11 and later guarantee std::string uses contiguous memory and
     always includes the terminating '\0'.  */
  va_start (vp, fmt);
  gmp_vsprintf (&str[0], fmt, vp);
  va_end (vp);

  return str;
}

/* See gmp-utils.h.  */

void
gdb_mpz::read (gdb::array_view<const gdb_byte> buf, enum bfd_endian byte_order,
	       bool unsigned_p)
{
  mpz_import (m_val, 1 /* count */, -1 /* order */, buf.size () /* size */,
	      byte_order == BFD_ENDIAN_BIG ? 1 : -1 /* endian */,
	      0 /* nails */, buf.data () /* op */);

  if (!unsigned_p)
    {
      /* The value was imported as if it was a positive value,
	 as mpz_import does not handle signs. If the original value
	 was in fact negative, we need to adjust VAL accordingly.  */
      gdb_mpz max;

      mpz_ui_pow_ui (max.m_val, 2, buf.size () * HOST_CHAR_BIT - 1);
      if (mpz_cmp (m_val, max.m_val) >= 0)
	mpz_submul_ui (m_val, max.m_val, 2);
    }
}

/* See gmp-utils.h.  */

void
gdb_mpz::export_bits (gdb::array_view<gdb_byte> buf, int endian, bool unsigned_p,
		      bool safe) const
{
  int sign = mpz_sgn (m_val);
  if (sign == 0)
    {
      /* Our value is zero, so no need to call mpz_export to do the work,
	 especially since mpz_export's documentation explicitly says
	 that the function is a noop in this case.  Just write zero to
	 BUF ourselves, if it is non-empty.  In some languages, a
	 zero-bit type can exist and this is also fine.  */
      if (buf.size () > 0)
	memset (buf.data (), 0, buf.size ());
      return;
    }

  gdb_assert (buf.size () > 0);

  if (safe)
    {
      /* Determine the maximum range of values that our buffer can
	 hold, and verify that VAL is within that range.  */

      gdb_mpz lo, hi;
      const size_t max_usable_bits = buf.size () * HOST_CHAR_BIT;
      if (unsigned_p)
	{
	  lo = 0;

	  mpz_ui_pow_ui (hi.m_val, 2, max_usable_bits);
	  mpz_sub_ui (hi.m_val, hi.m_val, 1);
	}
      else
	{
	  mpz_ui_pow_ui (lo.m_val, 2, max_usable_bits - 1);
	  mpz_neg (lo.m_val, lo.m_val);

	  mpz_ui_pow_ui (hi.m_val, 2, max_usable_bits - 1);
	  mpz_sub_ui (hi.m_val, hi.m_val, 1);
	}

      if (mpz_cmp (m_val, lo.m_val) < 0 || mpz_cmp (m_val, hi.m_val) > 0)
	error (_("Cannot export value %s as %zu-bits %s integer"
		 " (must be between %s and %s)"),
	       this->str ().c_str (),
	       max_usable_bits,
	       unsigned_p ? _("unsigned") : _("signed"),
	       lo.str ().c_str (),
	       hi.str ().c_str ());
    }

  const gdb_mpz *exported_val = this;
  gdb_mpz un_signed;
  if (sign < 0)
    {
      /* mpz_export does not handle signed values, so create a positive
	 value whose bit representation as an unsigned of the same length
	 would be the same as our negative value.  */
      gdb_mpz neg_offset = gdb_mpz::pow (2, buf.size () * HOST_CHAR_BIT);
      un_signed = *exported_val + neg_offset;
      exported_val = &un_signed;
    }

  /* If the value is too large, truncate it.  */
  if (!safe
      && mpz_sizeinbase (exported_val->m_val, 2) > buf.size () * HOST_CHAR_BIT)
    {
      /* If we don't already have a copy, make it now.  */
      if (exported_val != &un_signed)
	{
	  un_signed = *exported_val;
	  exported_val = &un_signed;
	}

      un_signed.mask (buf.size () * HOST_CHAR_BIT);
    }

  /* It's possible that one of the above results in zero, which has to
     be handled specially.  */
  if (exported_val->sgn () == 0)
    {
      memset (buf.data (), 0, buf.size ());
      return;
    }

  /* Do the export into a buffer allocated by GMP itself; that way,
     we can detect cases where BUF is not large enough to export
     our value, and thus avoid a buffer overflow.  Normally, this should
     never happen, since we verified earlier that the buffer is large
     enough to accommodate our value, but doing this allows us to be
     extra safe with the export.

     After verification that the export behaved as expected, we will
     copy the data over to BUF.  */

  size_t word_countp;
  gdb::unique_xmalloc_ptr<void> exported
    (mpz_export (NULL, &word_countp, -1 /* order */, buf.size () /* size */,
		 endian, 0 /* nails */, exported_val->m_val));

  gdb_assert (word_countp == 1);

  memcpy (buf.data (), exported.get (), buf.size ());
}

/* See gmp-utils.h.  */

gdb_mpz
gdb_mpq::get_rounded () const
{
  /* Work with a positive number so as to make the "floor" rounding
     always round towards zero.  */

  gdb_mpq abs_val (m_val);
  mpq_abs (abs_val.m_val, abs_val.m_val);

  /* Convert our rational number into a quotient and remainder,
     with "floor" rounding, which in our case means rounding
     towards zero.  */

  gdb_mpz quotient, remainder;
  mpz_fdiv_qr (quotient.m_val, remainder.m_val,
	       mpq_numref (abs_val.m_val), mpq_denref (abs_val.m_val));

  /* Multiply the remainder by 2, and see if it is greater or equal
     to abs_val's denominator.  If yes, round to the next integer.  */

  mpz_mul_ui (remainder.m_val, remainder.m_val, 2);
  if (mpz_cmp (remainder.m_val, mpq_denref (abs_val.m_val)) >= 0)
    mpz_add_ui (quotient.m_val, quotient.m_val, 1);

  /* Re-apply the sign if needed.  */
  if (mpq_sgn (m_val) < 0)
    mpz_neg (quotient.m_val, quotient.m_val);

  return quotient;
}

/* See gmp-utils.h.  */

void
gdb_mpq::read_fixed_point (gdb::array_view<const gdb_byte> buf,
			   enum bfd_endian byte_order, bool unsigned_p,
			   const gdb_mpq &scaling_factor)
{
  gdb_mpz vz;
  vz.read (buf, byte_order, unsigned_p);

  mpq_set_z (m_val, vz.m_val);
  mpq_mul (m_val, m_val, scaling_factor.m_val);
}

/* See gmp-utils.h.  */

void
gdb_mpq::write_fixed_point (gdb::array_view<gdb_byte> buf,
			    enum bfd_endian byte_order, bool unsigned_p,
			    const gdb_mpq &scaling_factor) const
{
  gdb_mpq unscaled (m_val);

  mpq_div (unscaled.m_val, unscaled.m_val, scaling_factor.m_val);

  gdb_mpz unscaled_z = unscaled.get_rounded ();
  unscaled_z.write (buf, byte_order, unsigned_p);
}

/* A wrapper around xrealloc that we can then register with GMP
   as the "realloc" function.  */

static void *
xrealloc_for_gmp (void *ptr, size_t old_size, size_t new_size)
{
  return xrealloc (ptr, new_size);
}

/* A wrapper around xfree that we can then register with GMP
   as the "free" function.  */

static void
xfree_for_gmp (void *ptr, size_t size)
{
  xfree (ptr);
}

void _initialize_gmp_utils ();

void
_initialize_gmp_utils ()
{
  /* Tell GMP to use GDB's memory management routines.  */
  mp_set_memory_functions (xmalloc, xrealloc_for_gmp, xfree_for_gmp);
}
