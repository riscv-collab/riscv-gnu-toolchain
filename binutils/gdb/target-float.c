/* Floating point routines for GDB, the GNU debugger.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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
#include "gdbtypes.h"
#include "floatformat.h"
#include "target-float.h"
#include "gdbarch.h"

/* Target floating-point operations.

   We provide multiple implementations of those operations, which differ
   by the host-side intermediate format they perform computations in.

   Those multiple implementations all derive from the following abstract
   base class, which specifies the set of operations to be implemented.  */

class target_float_ops
{
public:
  virtual std::string to_string (const gdb_byte *addr, const struct type *type,
				 const char *format) const = 0;
  virtual bool from_string (gdb_byte *addr, const struct type *type,
			    const std::string &string) const = 0;

  virtual LONGEST to_longest (const gdb_byte *addr,
			      const struct type *type) const = 0;
  virtual void from_longest (gdb_byte *addr, const struct type *type,
			     LONGEST val) const = 0;
  virtual void from_ulongest (gdb_byte *addr, const struct type *type,
			      ULONGEST val) const = 0;
  virtual double to_host_double (const gdb_byte *addr,
				 const struct type *type) const = 0;
  virtual void from_host_double (gdb_byte *addr, const struct type *type,
				 double val) const = 0;
  virtual void convert (const gdb_byte *from, const struct type *from_type,
			gdb_byte *to, const struct type *to_type) const = 0;

  virtual void binop (enum exp_opcode opcode,
		      const gdb_byte *x, const struct type *type_x,
		      const gdb_byte *y, const struct type *type_y,
		      gdb_byte *res, const struct type *type_res) const = 0;
  virtual int compare (const gdb_byte *x, const struct type *type_x,
		       const gdb_byte *y, const struct type *type_y) const = 0;
};


/* Helper routines operating on binary floating-point data.  */

#include <cmath>
#include <limits>

/* Different kinds of floatformat numbers recognized by
   floatformat_classify.  To avoid portability issues, we use local
   values instead of the C99 macros (FP_NAN et cetera).  */
enum float_kind {
  float_nan,
  float_infinite,
  float_zero,
  float_normal,
  float_subnormal
};

/* The odds that CHAR_BIT will be anything but 8 are low enough that I'm not
   going to bother with trying to muck around with whether it is defined in
   a system header, what we do if not, etc.  */
#define FLOATFORMAT_CHAR_BIT 8

/* The number of bytes that the largest floating-point type that we
   can convert to doublest will need.  */
#define FLOATFORMAT_LARGEST_BYTES 16

/* Return the floatformat's total size in host bytes.  */
static size_t
floatformat_totalsize_bytes (const struct floatformat *fmt)
{
  return ((fmt->totalsize + FLOATFORMAT_CHAR_BIT - 1)
	  / FLOATFORMAT_CHAR_BIT);
}

/* Return the precision of the floating point format FMT.  */
static int
floatformat_precision (const struct floatformat *fmt)
{
  /* Assume the precision of and IBM long double is twice the precision
     of the underlying double.  This matches what GCC does.  */
  if (fmt->split_half)
    return 2 * floatformat_precision (fmt->split_half);

  /* Otherwise, the precision is the size of mantissa in bits,
     including the implicit bit if present.  */
  int prec = fmt->man_len;
  if (fmt->intbit == floatformat_intbit_no)
    prec++;

  return prec;
}

/* Normalize the byte order of FROM into TO.  If no normalization is
   needed then FMT->byteorder is returned and TO is not changed;
   otherwise the format of the normalized form in TO is returned.  */
static enum floatformat_byteorders
floatformat_normalize_byteorder (const struct floatformat *fmt,
				 const void *from, void *to)
{
  const unsigned char *swapin;
  unsigned char *swapout;
  int words;

  if (fmt->byteorder == floatformat_little
      || fmt->byteorder == floatformat_big)
    return fmt->byteorder;

  words = fmt->totalsize / FLOATFORMAT_CHAR_BIT;
  words >>= 2;

  swapout = (unsigned char *)to;
  swapin = (const unsigned char *)from;

  if (fmt->byteorder == floatformat_vax)
    {
      while (words-- > 0)
	{
	  *swapout++ = swapin[1];
	  *swapout++ = swapin[0];
	  *swapout++ = swapin[3];
	  *swapout++ = swapin[2];
	  swapin += 4;
	}
      /* This may look weird, since VAX is little-endian, but it is
	 easier to translate to big-endian than to little-endian.  */
      return floatformat_big;
    }
  else
    {
      gdb_assert (fmt->byteorder == floatformat_littlebyte_bigword);

      while (words-- > 0)
	{
	  *swapout++ = swapin[3];
	  *swapout++ = swapin[2];
	  *swapout++ = swapin[1];
	  *swapout++ = swapin[0];
	  swapin += 4;
	}
      return floatformat_big;
    }
}

/* Extract a field which starts at START and is LEN bytes long.  DATA and
   TOTAL_LEN are the thing we are extracting it from, in byteorder ORDER.  */
static unsigned long
get_field (const bfd_byte *data, enum floatformat_byteorders order,
	   unsigned int total_len, unsigned int start, unsigned int len)
{
  unsigned long result;
  unsigned int cur_byte;
  int cur_bitshift;

  /* Caller must byte-swap words before calling this routine.  */
  gdb_assert (order == floatformat_little || order == floatformat_big);

  /* Start at the least significant part of the field.  */
  if (order == floatformat_little)
    {
      /* We start counting from the other end (i.e, from the high bytes
	 rather than the low bytes).  As such, we need to be concerned
	 with what happens if bit 0 doesn't start on a byte boundary.
	 I.e, we need to properly handle the case where total_len is
	 not evenly divisible by 8.  So we compute ``excess'' which
	 represents the number of bits from the end of our starting
	 byte needed to get to bit 0.  */
      int excess = FLOATFORMAT_CHAR_BIT - (total_len % FLOATFORMAT_CHAR_BIT);

      cur_byte = (total_len / FLOATFORMAT_CHAR_BIT)
		 - ((start + len + excess) / FLOATFORMAT_CHAR_BIT);
      cur_bitshift = ((start + len + excess) % FLOATFORMAT_CHAR_BIT)
		     - FLOATFORMAT_CHAR_BIT;
    }
  else
    {
      cur_byte = (start + len) / FLOATFORMAT_CHAR_BIT;
      cur_bitshift =
	((start + len) % FLOATFORMAT_CHAR_BIT) - FLOATFORMAT_CHAR_BIT;
    }
  if (cur_bitshift > -FLOATFORMAT_CHAR_BIT)
    result = *(data + cur_byte) >> (-cur_bitshift);
  else
    result = 0;
  cur_bitshift += FLOATFORMAT_CHAR_BIT;
  if (order == floatformat_little)
    ++cur_byte;
  else
    --cur_byte;

  /* Move towards the most significant part of the field.  */
  while (cur_bitshift < len)
    {
      result |= (unsigned long)*(data + cur_byte) << cur_bitshift;
      cur_bitshift += FLOATFORMAT_CHAR_BIT;
      switch (order)
	{
	case floatformat_little:
	  ++cur_byte;
	  break;
	case floatformat_big:
	  --cur_byte;
	  break;
	}
    }
  if (len < sizeof(result) * FLOATFORMAT_CHAR_BIT)
    /* Mask out bits which are not part of the field.  */
    result &= ((1UL << len) - 1);
  return result;
}

/* Set a field which starts at START and is LEN bytes long.  DATA and
   TOTAL_LEN are the thing we are extracting it from, in byteorder ORDER.  */
static void
put_field (unsigned char *data, enum floatformat_byteorders order,
	   unsigned int total_len, unsigned int start, unsigned int len,
	   unsigned long stuff_to_put)
{
  unsigned int cur_byte;
  int cur_bitshift;

  /* Caller must byte-swap words before calling this routine.  */
  gdb_assert (order == floatformat_little || order == floatformat_big);

  /* Start at the least significant part of the field.  */
  if (order == floatformat_little)
    {
      int excess = FLOATFORMAT_CHAR_BIT - (total_len % FLOATFORMAT_CHAR_BIT);

      cur_byte = (total_len / FLOATFORMAT_CHAR_BIT)
		 - ((start + len + excess) / FLOATFORMAT_CHAR_BIT);
      cur_bitshift = ((start + len + excess) % FLOATFORMAT_CHAR_BIT)
		     - FLOATFORMAT_CHAR_BIT;
    }
  else
    {
      cur_byte = (start + len) / FLOATFORMAT_CHAR_BIT;
      cur_bitshift =
	((start + len) % FLOATFORMAT_CHAR_BIT) - FLOATFORMAT_CHAR_BIT;
    }
  if (cur_bitshift > -FLOATFORMAT_CHAR_BIT)
    {
      *(data + cur_byte) &=
	~(((1 << ((start + len) % FLOATFORMAT_CHAR_BIT)) - 1)
	  << (-cur_bitshift));
      *(data + cur_byte) |=
	(stuff_to_put & ((1 << FLOATFORMAT_CHAR_BIT) - 1)) << (-cur_bitshift);
    }
  cur_bitshift += FLOATFORMAT_CHAR_BIT;
  if (order == floatformat_little)
    ++cur_byte;
  else
    --cur_byte;

  /* Move towards the most significant part of the field.  */
  while (cur_bitshift < len)
    {
      if (len - cur_bitshift < FLOATFORMAT_CHAR_BIT)
	{
	  /* This is the last byte.  */
	  *(data + cur_byte) &=
	    ~((1 << (len - cur_bitshift)) - 1);
	  *(data + cur_byte) |= (stuff_to_put >> cur_bitshift);
	}
      else
	*(data + cur_byte) = ((stuff_to_put >> cur_bitshift)
			      & ((1 << FLOATFORMAT_CHAR_BIT) - 1));
      cur_bitshift += FLOATFORMAT_CHAR_BIT;
      if (order == floatformat_little)
	++cur_byte;
      else
	--cur_byte;
    }
}

/* Check if VAL (which is assumed to be a floating point number whose
   format is described by FMT) is negative.  */
static int
floatformat_is_negative (const struct floatformat *fmt,
			 const bfd_byte *uval)
{
  enum floatformat_byteorders order;
  unsigned char newfrom[FLOATFORMAT_LARGEST_BYTES];

  gdb_assert (fmt != NULL);
  gdb_assert (fmt->totalsize
	      <= FLOATFORMAT_LARGEST_BYTES * FLOATFORMAT_CHAR_BIT);

  /* An IBM long double (a two element array of double) always takes the
     sign of the first double.  */
  if (fmt->split_half)
    fmt = fmt->split_half;

  order = floatformat_normalize_byteorder (fmt, uval, newfrom);

  if (order != fmt->byteorder)
    uval = newfrom;

  return get_field (uval, order, fmt->totalsize, fmt->sign_start, 1);
}

/* Check if VAL is "not a number" (NaN) for FMT.  */
static enum float_kind
floatformat_classify (const struct floatformat *fmt,
		      const bfd_byte *uval)
{
  long exponent;
  unsigned long mant;
  unsigned int mant_bits, mant_off;
  int mant_bits_left;
  enum floatformat_byteorders order;
  unsigned char newfrom[FLOATFORMAT_LARGEST_BYTES];
  int mant_zero;

  gdb_assert (fmt != NULL);
  gdb_assert (fmt->totalsize
	      <= FLOATFORMAT_LARGEST_BYTES * FLOATFORMAT_CHAR_BIT);

  /* An IBM long double (a two element array of double) can be classified
     by looking at the first double.  inf and nan are specified as
     ignoring the second double.  zero and subnormal will always have
     the second double 0.0 if the long double is correctly rounded.  */
  if (fmt->split_half)
    fmt = fmt->split_half;

  order = floatformat_normalize_byteorder (fmt, uval, newfrom);

  if (order != fmt->byteorder)
    uval = newfrom;

  exponent = get_field (uval, order, fmt->totalsize, fmt->exp_start,
			fmt->exp_len);

  mant_bits_left = fmt->man_len;
  mant_off = fmt->man_start;

  mant_zero = 1;
  while (mant_bits_left > 0)
    {
      mant_bits = std::min (mant_bits_left, 32);

      mant = get_field (uval, order, fmt->totalsize, mant_off, mant_bits);

      /* If there is an explicit integer bit, mask it off.  */
      if (mant_off == fmt->man_start
	  && fmt->intbit == floatformat_intbit_yes)
	mant &= ~(1 << (mant_bits - 1));

      if (mant)
	{
	  mant_zero = 0;
	  break;
	}

      mant_off += mant_bits;
      mant_bits_left -= mant_bits;
    }

  /* If exp_nan is not set, assume that inf, NaN, and subnormals are not
     supported.  */
  if (! fmt->exp_nan)
    {
      if (mant_zero)
	return float_zero;
      else
	return float_normal;
    }

  if (exponent == 0)
    {
      if (mant_zero)
	return float_zero;
      else
	return float_subnormal;
    }

  if (exponent == fmt->exp_nan)
    {
      if (mant_zero)
	return float_infinite;
      else
	return float_nan;
    }

  return float_normal;
}

/* Convert the mantissa of VAL (which is assumed to be a floating
   point number whose format is described by FMT) into a hexadecimal
   and store it in a static string.  Return a pointer to that string.  */
static const char *
floatformat_mantissa (const struct floatformat *fmt,
		      const bfd_byte *val)
{
  unsigned char *uval = (unsigned char *) val;
  unsigned long mant;
  unsigned int mant_bits, mant_off;
  int mant_bits_left;
  static char res[50];
  char buf[9];
  int len;
  enum floatformat_byteorders order;
  unsigned char newfrom[FLOATFORMAT_LARGEST_BYTES];

  gdb_assert (fmt != NULL);
  gdb_assert (fmt->totalsize
	      <= FLOATFORMAT_LARGEST_BYTES * FLOATFORMAT_CHAR_BIT);

  /* For IBM long double (a two element array of double), return the
     mantissa of the first double.  The problem with returning the
     actual mantissa from both doubles is that there can be an
     arbitrary number of implied 0's or 1's between the mantissas
     of the first and second double.  In any case, this function
     is only used for dumping out nans, and a nan is specified to
     ignore the value in the second double.  */
  if (fmt->split_half)
    fmt = fmt->split_half;

  order = floatformat_normalize_byteorder (fmt, uval, newfrom);

  if (order != fmt->byteorder)
    uval = newfrom;

  if (! fmt->exp_nan)
    return 0;

  /* Make sure we have enough room to store the mantissa.  */
  gdb_assert (sizeof res > ((fmt->man_len + 7) / 8) * 2);

  mant_off = fmt->man_start;
  mant_bits_left = fmt->man_len;
  mant_bits = (mant_bits_left % 32) > 0 ? mant_bits_left % 32 : 32;

  mant = get_field (uval, order, fmt->totalsize, mant_off, mant_bits);

  len = xsnprintf (res, sizeof res, "%lx", mant);

  mant_off += mant_bits;
  mant_bits_left -= mant_bits;

  while (mant_bits_left > 0)
    {
      mant = get_field (uval, order, fmt->totalsize, mant_off, 32);

      xsnprintf (buf, sizeof buf, "%08lx", mant);
      gdb_assert (len + strlen (buf) <= sizeof res);
      strcat (res, buf);

      mant_off += 32;
      mant_bits_left -= 32;
    }

  return res;
}

/* Convert printf format string FORMAT to the otherwise equivalent string
   which may be used to print a host floating-point number using the length
   modifier LENGTH (which may be 0 if none is needed).  If FORMAT is null,
   return a format appropriate to print the full precision of a target
   floating-point number of format FMT.  */
static std::string
floatformat_printf_format (const struct floatformat *fmt,
			   const char *format, char length)
{
  std::string host_format;
  char conversion;

  if (format == nullptr)
    {
      /* If no format was specified, print the number using a format string
	 where the precision is set to the DECIMAL_DIG value for the given
	 floating-point format.  This value is computed as

		ceil(1 + p * log10(b)),

	 where p is the precision of the floating-point format in bits, and
	 b is the base (which is always 2 for the formats we support).  */
      const double log10_2 = .30102999566398119521;
      double d_decimal_dig = 1 + floatformat_precision (fmt) * log10_2;
      int decimal_dig = d_decimal_dig;
      if (decimal_dig < d_decimal_dig)
	decimal_dig++;

      host_format = string_printf ("%%.%d", decimal_dig);
      conversion = 'g';
    }
  else
    {
      /* Use the specified format, stripping out the conversion character
	 and length modifier, if present.  */
      size_t len = strlen (format);
      gdb_assert (len > 1);
      conversion = format[--len];
      gdb_assert (conversion == 'e' || conversion == 'f' || conversion == 'g'
		  || conversion == 'E' || conversion == 'G');
      if (format[len - 1] == 'L')
	len--;

      host_format = std::string (format, len);
    }

  /* Add the length modifier and conversion character appropriate for
     handling the appropriate host floating-point type.  */
  if (length)
    host_format += length;
  host_format += conversion;

  return host_format;
}

/* Implementation of target_float_ops using the host floating-point type T
   as intermediate type.  */

template<typename T> class host_float_ops : public target_float_ops
{
public:
  std::string to_string (const gdb_byte *addr, const struct type *type,
			 const char *format) const override;
  bool from_string (gdb_byte *addr, const struct type *type,
		    const std::string &string) const override;

  LONGEST to_longest (const gdb_byte *addr,
		      const struct type *type) const override;
  void from_longest (gdb_byte *addr, const struct type *type,
		     LONGEST val) const override;
  void from_ulongest (gdb_byte *addr, const struct type *type,
		      ULONGEST val) const override;
  double to_host_double (const gdb_byte *addr,
			 const struct type *type) const override;
  void from_host_double (gdb_byte *addr, const struct type *type,
			 double val) const override;
  void convert (const gdb_byte *from, const struct type *from_type,
		gdb_byte *to, const struct type *to_type) const override;

  void binop (enum exp_opcode opcode,
	      const gdb_byte *x, const struct type *type_x,
	      const gdb_byte *y, const struct type *type_y,
	      gdb_byte *res, const struct type *type_res) const override;
  int compare (const gdb_byte *x, const struct type *type_x,
	       const gdb_byte *y, const struct type *type_y) const override;

private:
  void from_target (const struct floatformat *fmt,
		    const gdb_byte *from, T *to) const;
  void from_target (const struct type *type,
		    const gdb_byte *from, T *to) const;

  void to_target (const struct type *type,
		  const T *from, gdb_byte *to) const;
  void to_target (const struct floatformat *fmt,
		  const T *from, gdb_byte *to) const;
};


/* Convert TO/FROM target to the host floating-point format T.

   If the host and target formats agree, we just copy the raw data
   into the appropriate type of variable and return, letting the host
   increase precision as necessary.  Otherwise, we call the conversion
   routine and let it do the dirty work.  Note that even if the target
   and host floating-point formats match, the length of the types
   might still be different, so the conversion routines must make sure
   to not overrun any buffers.  For example, on x86, long double is
   the 80-bit extended precision type on both 32-bit and 64-bit ABIs,
   but by default it is stored as 12 bytes on 32-bit, and 16 bytes on
   64-bit, for alignment reasons.  See comment in store_typed_floating
   for a discussion about zeroing out remaining bytes in the target
   buffer.  */

static const struct floatformat *host_float_format = GDB_HOST_FLOAT_FORMAT;
static const struct floatformat *host_double_format = GDB_HOST_DOUBLE_FORMAT;
static const struct floatformat *host_long_double_format
  = GDB_HOST_LONG_DOUBLE_FORMAT;

/* Convert target floating-point value at FROM in format FMT to host
   floating-point format of type T.  */
template<typename T> void
host_float_ops<T>::from_target (const struct floatformat *fmt,
				const gdb_byte *from, T *to) const
{
  gdb_assert (fmt != NULL);

  if (fmt == host_float_format)
    {
      float val = 0;

      memcpy (&val, from, floatformat_totalsize_bytes (fmt));
      *to = val;
      return;
    }
  else if (fmt == host_double_format)
    {
      double val = 0;

      memcpy (&val, from, floatformat_totalsize_bytes (fmt));
      *to = val;
      return;
    }
  else if (fmt == host_long_double_format)
    {
      long double val = 0;

      memcpy (&val, from, floatformat_totalsize_bytes (fmt));
      *to = val;
      return;
    }

  unsigned char *ufrom = (unsigned char *) from;
  long exponent;
  unsigned long mant;
  unsigned int mant_bits, mant_off;
  int mant_bits_left;
  int special_exponent;		/* It's a NaN, denorm or zero.  */
  enum floatformat_byteorders order;
  unsigned char newfrom[FLOATFORMAT_LARGEST_BYTES];
  enum float_kind kind;

  gdb_assert (fmt->totalsize
	      <= FLOATFORMAT_LARGEST_BYTES * FLOATFORMAT_CHAR_BIT);

  /* For non-numbers, reuse libiberty's logic to find the correct
     format.  We do not lose any precision in this case by passing
     through a double.  */
  kind = floatformat_classify (fmt, (const bfd_byte *) from);
  if (kind == float_infinite || kind == float_nan)
    {
      double dto;

      floatformat_to_double	/* ARI: floatformat_to_double */
	(fmt->split_half ? fmt->split_half : fmt, from, &dto);
      *to = (T) dto;
      return;
    }

  order = floatformat_normalize_byteorder (fmt, ufrom, newfrom);

  if (order != fmt->byteorder)
    ufrom = newfrom;

  if (fmt->split_half)
    {
      T dtop, dbot;

      from_target (fmt->split_half, ufrom, &dtop);
      /* Preserve the sign of 0, which is the sign of the top
	 half.  */
      if (dtop == 0.0)
	{
	  *to = dtop;
	  return;
	}
      from_target (fmt->split_half,
		   ufrom + fmt->totalsize / FLOATFORMAT_CHAR_BIT / 2, &dbot);
      *to = dtop + dbot;
      return;
    }

  exponent = get_field (ufrom, order, fmt->totalsize, fmt->exp_start,
			fmt->exp_len);
  /* Note that if exponent indicates a NaN, we can't really do anything useful
     (not knowing if the host has NaN's, or how to build one).  So it will
     end up as an infinity or something close; that is OK.  */

  mant_bits_left = fmt->man_len;
  mant_off = fmt->man_start;
  T dto = 0.0;

  special_exponent = exponent == 0 || exponent == fmt->exp_nan;

  /* Don't bias NaNs.  Use minimum exponent for denorms.  For
     simplicity, we don't check for zero as the exponent doesn't matter.
     Note the cast to int; exp_bias is unsigned, so it's important to
     make sure the operation is done in signed arithmetic.  */
  if (!special_exponent)
    exponent -= fmt->exp_bias;
  else if (exponent == 0)
    exponent = 1 - fmt->exp_bias;

  /* Build the result algebraically.  Might go infinite, underflow, etc;
     who cares.  */

  /* If this format uses a hidden bit, explicitly add it in now.  Otherwise,
     increment the exponent by one to account for the integer bit.  */

  if (!special_exponent)
    {
      if (fmt->intbit == floatformat_intbit_no)
	dto = ldexp (1.0, exponent);
      else
	exponent++;
    }

  while (mant_bits_left > 0)
    {
      mant_bits = std::min (mant_bits_left, 32);

      mant = get_field (ufrom, order, fmt->totalsize, mant_off, mant_bits);

      dto += ldexp ((T) mant, exponent - mant_bits);
      exponent -= mant_bits;
      mant_off += mant_bits;
      mant_bits_left -= mant_bits;
    }

  /* Negate it if negative.  */
  if (get_field (ufrom, order, fmt->totalsize, fmt->sign_start, 1))
    dto = -dto;
  *to = dto;
}

template<typename T> void
host_float_ops<T>::from_target (const struct type *type,
				const gdb_byte *from, T *to) const
{
  from_target (floatformat_from_type (type), from, to);
}

/* Convert host floating-point value of type T to target floating-point
   value in format FMT and store at TO.  */
template<typename T> void
host_float_ops<T>::to_target (const struct floatformat *fmt,
			      const T *from, gdb_byte *to) const
{
  gdb_assert (fmt != NULL);

  if (fmt == host_float_format)
    {
      float val = *from;

      memcpy (to, &val, floatformat_totalsize_bytes (fmt));
      return;
    }
  else if (fmt == host_double_format)
    {
      double val = *from;

      memcpy (to, &val, floatformat_totalsize_bytes (fmt));
      return;
    }
  else if (fmt == host_long_double_format)
    {
      long double val = *from;

      memcpy (to, &val, floatformat_totalsize_bytes (fmt));
      return;
    }

  T dfrom;
  int exponent;
  T mant;
  unsigned int mant_bits, mant_off;
  int mant_bits_left;
  unsigned char *uto = (unsigned char *) to;
  enum floatformat_byteorders order = fmt->byteorder;
  unsigned char newto[FLOATFORMAT_LARGEST_BYTES];

  if (order != floatformat_little)
    order = floatformat_big;

  if (order != fmt->byteorder)
    uto = newto;

  memcpy (&dfrom, from, sizeof (dfrom));
  memset (uto, 0, floatformat_totalsize_bytes (fmt));

  if (fmt->split_half)
    {
      /* Use static volatile to ensure that any excess precision is
	 removed via storing in memory, and so the top half really is
	 the result of converting to double.  */
      static volatile double dtop, dbot;
      T dtopnv, dbotnv;

      dtop = (double) dfrom;
      /* If the rounded top half is Inf, the bottom must be 0 not NaN
	 or Inf.  */
      if (dtop + dtop == dtop && dtop != 0.0)
	dbot = 0.0;
      else
	dbot = (double) (dfrom - (T) dtop);
      dtopnv = dtop;
      dbotnv = dbot;
      to_target (fmt->split_half, &dtopnv, uto);
      to_target (fmt->split_half, &dbotnv,
		 uto + fmt->totalsize / FLOATFORMAT_CHAR_BIT / 2);
      return;
    }

  if (dfrom == 0)
    goto finalize_byteorder;	/* Result is zero */
  if (dfrom != dfrom)		/* Result is NaN */
    {
      /* From is NaN */
      put_field (uto, order, fmt->totalsize, fmt->exp_start,
		 fmt->exp_len, fmt->exp_nan);
      /* Be sure it's not infinity, but NaN value is irrel.  */
      put_field (uto, order, fmt->totalsize, fmt->man_start,
		 fmt->man_len, 1);
      goto finalize_byteorder;
    }

  /* If negative, set the sign bit.  */
  if (dfrom < 0)
    {
      put_field (uto, order, fmt->totalsize, fmt->sign_start, 1, 1);
      dfrom = -dfrom;
    }

  if (dfrom + dfrom == dfrom && dfrom != 0.0)	/* Result is Infinity.  */
    {
      /* Infinity exponent is same as NaN's.  */
      put_field (uto, order, fmt->totalsize, fmt->exp_start,
		 fmt->exp_len, fmt->exp_nan);
      /* Infinity mantissa is all zeroes.  */
      put_field (uto, order, fmt->totalsize, fmt->man_start,
		 fmt->man_len, 0);
      goto finalize_byteorder;
    }

  mant = frexp (dfrom, &exponent);

  if (exponent + fmt->exp_bias <= 0)
    {
      /* The value is too small to be expressed in the destination
	 type (not enough bits in the exponent.  Treat as 0.  */
      put_field (uto, order, fmt->totalsize, fmt->exp_start,
		 fmt->exp_len, 0);
      put_field (uto, order, fmt->totalsize, fmt->man_start,
		 fmt->man_len, 0);
      goto finalize_byteorder;
    }

  if (exponent + fmt->exp_bias >= (1 << fmt->exp_len))
    {
      /* The value is too large to fit into the destination.
	 Treat as infinity.  */
      put_field (uto, order, fmt->totalsize, fmt->exp_start,
		 fmt->exp_len, fmt->exp_nan);
      put_field (uto, order, fmt->totalsize, fmt->man_start,
		 fmt->man_len, 0);
      goto finalize_byteorder;
    }

  put_field (uto, order, fmt->totalsize, fmt->exp_start, fmt->exp_len,
	     exponent + fmt->exp_bias - 1);

  mant_bits_left = fmt->man_len;
  mant_off = fmt->man_start;
  while (mant_bits_left > 0)
    {
      unsigned long mant_long;

      mant_bits = mant_bits_left < 32 ? mant_bits_left : 32;

      mant *= 4294967296.0;
      mant_long = ((unsigned long) mant) & 0xffffffffL;
      mant -= mant_long;

      /* If the integer bit is implicit, then we need to discard it.
	 If we are discarding a zero, we should be (but are not) creating
	 a denormalized number which means adjusting the exponent
	 (I think).  */
      if (mant_bits_left == fmt->man_len
	  && fmt->intbit == floatformat_intbit_no)
	{
	  mant_long <<= 1;
	  mant_long &= 0xffffffffL;
	  /* If we are processing the top 32 mantissa bits of a doublest
	     so as to convert to a float value with implied integer bit,
	     we will only be putting 31 of those 32 bits into the
	     final value due to the discarding of the top bit.  In the
	     case of a small float value where the number of mantissa
	     bits is less than 32, discarding the top bit does not alter
	     the number of bits we will be adding to the result.  */
	  if (mant_bits == 32)
	    mant_bits -= 1;
	}

      if (mant_bits < 32)
	{
	  /* The bits we want are in the most significant MANT_BITS bits of
	     mant_long.  Move them to the least significant.  */
	  mant_long >>= 32 - mant_bits;
	}

      put_field (uto, order, fmt->totalsize,
		 mant_off, mant_bits, mant_long);
      mant_off += mant_bits;
      mant_bits_left -= mant_bits;
    }

 finalize_byteorder:
  /* Do we need to byte-swap the words in the result?  */
  if (order != fmt->byteorder)
    floatformat_normalize_byteorder (fmt, newto, to);
}

template<typename T> void
host_float_ops<T>::to_target (const struct type *type,
			      const T *from, gdb_byte *to) const
{
  /* Ensure possible padding bytes in the target buffer are zeroed out.  */
  memset (to, 0, type->length ());

  to_target (floatformat_from_type (type), from, to);
}

/* Convert the byte-stream ADDR, interpreted as floating-point type TYPE,
   to a string, optionally using the print format FORMAT.  */
template<typename T> struct printf_length_modifier
{
  static constexpr char value = 0;
};
template<> struct printf_length_modifier<long double>
{
  static constexpr char value = 'L';
};
template<typename T> std::string
host_float_ops<T>::to_string (const gdb_byte *addr, const struct type *type,
			      const char *format) const
{
  /* Determine the format string to use on the host side.  */
  constexpr char length = printf_length_modifier<T>::value;
  const struct floatformat *fmt = floatformat_from_type (type);
  std::string host_format = floatformat_printf_format (fmt, format, length);

  T host_float;
  from_target (type, addr, &host_float);

  DIAGNOSTIC_PUSH
  DIAGNOSTIC_IGNORE_FORMAT_NONLITERAL
  return string_printf (host_format.c_str (), host_float);
  DIAGNOSTIC_POP
}

/* Parse string IN into a target floating-number of type TYPE and
   store it as byte-stream ADDR.  Return whether parsing succeeded.  */
template<typename T> struct scanf_length_modifier
{
  static constexpr char value = 0;
};
template<> struct scanf_length_modifier<double>
{
  static constexpr char value = 'l';
};
template<> struct scanf_length_modifier<long double>
{
  static constexpr char value = 'L';
};
template<typename T> bool
host_float_ops<T>::from_string (gdb_byte *addr, const struct type *type,
				const std::string &in) const
{
  T host_float;
  int n, num;

  std::string scan_format = "%";
  if (scanf_length_modifier<T>::value)
    scan_format += scanf_length_modifier<T>::value;
  scan_format += "g%n";

  DIAGNOSTIC_PUSH
  DIAGNOSTIC_IGNORE_FORMAT_NONLITERAL
  num = sscanf (in.c_str (), scan_format.c_str(), &host_float, &n);
  DIAGNOSTIC_POP

  /* The sscanf man page suggests not making any assumptions on the effect
     of %n on the result, so we don't.
     That is why we simply test num == 0.  */
  if (num == 0)
    return false;

  /* We only accept the whole string.  */
  if (in[n])
    return false;

  to_target (type, &host_float, addr);
  return true;
}

/* Convert the byte-stream ADDR, interpreted as floating-point type TYPE,
   to an integer value (rounding towards zero).  */
template<typename T> LONGEST
host_float_ops<T>::to_longest (const gdb_byte *addr,
			       const struct type *type) const
{
  T host_float;
  from_target (type, addr, &host_float);
  T min_possible_range = static_cast<T>(std::numeric_limits<LONGEST>::min());
  T max_possible_range = -min_possible_range;
  /* host_float can be converted to an integer as long as it's in
     the range [min_possible_range, max_possible_range). If not, it is either
     too large, or too small, or is NaN; in this case return the maximum or
     minimum possible value.  */
  if (host_float < max_possible_range && host_float >= min_possible_range)
    return static_cast<LONGEST> (host_float);
  if (host_float < min_possible_range)
    return std::numeric_limits<LONGEST>::min();
  /* This line will be executed if host_float is NaN.  */
  return std::numeric_limits<LONGEST>::max();
}

/* Convert signed integer VAL to a target floating-number of type TYPE
   and store it as byte-stream ADDR.  */
template<typename T> void
host_float_ops<T>::from_longest (gdb_byte *addr, const struct type *type,
				 LONGEST val) const
{
  T host_float = (T) val;
  to_target (type, &host_float, addr);
}

/* Convert unsigned integer VAL to a target floating-number of type TYPE
   and store it as byte-stream ADDR.  */
template<typename T> void
host_float_ops<T>::from_ulongest (gdb_byte *addr, const struct type *type,
				  ULONGEST val) const
{
  T host_float = (T) val;
  to_target (type, &host_float, addr);
}

/* Convert the byte-stream ADDR, interpreted as floating-point type TYPE,
   to a floating-point value in the host "double" format.  */
template<typename T> double
host_float_ops<T>::to_host_double (const gdb_byte *addr,
				   const struct type *type) const
{
  T host_float;
  from_target (type, addr, &host_float);
  return (double) host_float;
}

/* Convert floating-point value VAL in the host "double" format to a target
   floating-number of type TYPE and store it as byte-stream ADDR.  */
template<typename T> void
host_float_ops<T>::from_host_double (gdb_byte *addr, const struct type *type,
				     double val) const
{
  T host_float = (T) val;
  to_target (type, &host_float, addr);
}

/* Convert a floating-point number of type FROM_TYPE from the target
   byte-stream FROM to a floating-point number of type TO_TYPE, and
   store it to the target byte-stream TO.  */
template<typename T> void
host_float_ops<T>::convert (const gdb_byte *from,
			    const struct type *from_type,
			    gdb_byte *to,
			    const struct type *to_type) const
{
  T host_float;
  from_target (from_type, from, &host_float);
  to_target (to_type, &host_float, to);
}

/* Perform the binary operation indicated by OPCODE, using as operands the
   target byte streams X and Y, interpreted as floating-point numbers of
   types TYPE_X and TYPE_Y, respectively.  Convert the result to format
   TYPE_RES and store it into the byte-stream RES.  */
template<typename T> void
host_float_ops<T>::binop (enum exp_opcode op,
			  const gdb_byte *x, const struct type *type_x,
			  const gdb_byte *y, const struct type *type_y,
			  gdb_byte *res, const struct type *type_res) const
{
  T v1, v2, v = 0;

  from_target (type_x, x, &v1);
  from_target (type_y, y, &v2);

  switch (op)
    {
      case BINOP_ADD:
	v = v1 + v2;
	break;

      case BINOP_SUB:
	v = v1 - v2;
	break;

      case BINOP_MUL:
	v = v1 * v2;
	break;

      case BINOP_DIV:
	v = v1 / v2;
	break;

      case BINOP_EXP:
	errno = 0;
	v = pow (v1, v2);
	if (errno)
	  error (_("Cannot perform exponentiation: %s"),
		 safe_strerror (errno));
	break;

      case BINOP_MIN:
	v = v1 < v2 ? v1 : v2;
	break;

      case BINOP_MAX:
	v = v1 > v2 ? v1 : v2;
	break;

      default:
	error (_("Integer-only operation on floating point number."));
	break;
    }

  to_target (type_res, &v, res);
}

/* Compare the two target byte streams X and Y, interpreted as floating-point
   numbers of types TYPE_X and TYPE_Y, respectively.  Return zero if X and Y
   are equal, -1 if X is less than Y, and 1 otherwise.  */
template<typename T> int
host_float_ops<T>::compare (const gdb_byte *x, const struct type *type_x,
			    const gdb_byte *y, const struct type *type_y) const
{
  T v1, v2;

  from_target (type_x, x, &v1);
  from_target (type_y, y, &v2);

  if (v1 == v2)
    return 0;
  if (v1 < v2)
    return -1;
  return 1;
}


/* Implementation of target_float_ops using the MPFR library
   mpfr_t as intermediate type.  */

#define MPFR_USE_INTMAX_T

#include <mpfr.h>

class mpfr_float_ops : public target_float_ops
{
public:
  std::string to_string (const gdb_byte *addr, const struct type *type,
			 const char *format) const override;
  bool from_string (gdb_byte *addr, const struct type *type,
		    const std::string &string) const override;

  LONGEST to_longest (const gdb_byte *addr,
		      const struct type *type) const override;
  void from_longest (gdb_byte *addr, const struct type *type,
		     LONGEST val) const override;
  void from_ulongest (gdb_byte *addr, const struct type *type,
		      ULONGEST val) const override;
  double to_host_double (const gdb_byte *addr,
			 const struct type *type) const override;
  void from_host_double (gdb_byte *addr, const struct type *type,
			 double val) const override;
  void convert (const gdb_byte *from, const struct type *from_type,
		gdb_byte *to, const struct type *to_type) const override;

  void binop (enum exp_opcode opcode,
	      const gdb_byte *x, const struct type *type_x,
	      const gdb_byte *y, const struct type *type_y,
	      gdb_byte *res, const struct type *type_res) const override;
  int compare (const gdb_byte *x, const struct type *type_x,
	       const gdb_byte *y, const struct type *type_y) const override;

private:
  /* Local wrapper class to handle mpfr_t initialization and cleanup.  */
  class gdb_mpfr
  {
  public:
    mpfr_t val;

    gdb_mpfr (const struct type *type)
    {
      const struct floatformat *fmt = floatformat_from_type (type);
      mpfr_init2 (val, floatformat_precision (fmt));
    }

    gdb_mpfr (const gdb_mpfr &source)
    {
      mpfr_init2 (val, mpfr_get_prec (source.val));
    }

    ~gdb_mpfr ()
    {
      mpfr_clear (val);
    }
  };

  void from_target (const struct floatformat *fmt,
		const gdb_byte *from, gdb_mpfr &to) const;
  void from_target (const struct type *type,
		const gdb_byte *from, gdb_mpfr &to) const;

  void to_target (const struct type *type,
		  const gdb_mpfr &from, gdb_byte *to) const;
  void to_target (const struct floatformat *fmt,
		  const gdb_mpfr &from, gdb_byte *to) const;
};


/* Convert TO/FROM target floating-point format to mpfr_t.  */

void
mpfr_float_ops::from_target (const struct floatformat *fmt,
			     const gdb_byte *orig_from, gdb_mpfr &to) const
{
  const gdb_byte *from = orig_from;
  mpfr_exp_t exponent;
  unsigned long mant;
  unsigned int mant_bits, mant_off;
  int mant_bits_left;
  int special_exponent;		/* It's a NaN, denorm or zero.  */
  enum floatformat_byteorders order;
  unsigned char newfrom[FLOATFORMAT_LARGEST_BYTES];
  enum float_kind kind;

  gdb_assert (fmt->totalsize
	      <= FLOATFORMAT_LARGEST_BYTES * FLOATFORMAT_CHAR_BIT);

  /* Handle non-numbers.  */
  kind = floatformat_classify (fmt, from);
  if (kind == float_infinite)
    {
      mpfr_set_inf (to.val, floatformat_is_negative (fmt, from) ? -1 : 1);
      return;
    }
  if (kind == float_nan)
    {
      mpfr_set_nan (to.val);
      return;
    }

  order = floatformat_normalize_byteorder (fmt, from, newfrom);

  if (order != fmt->byteorder)
    from = newfrom;

  if (fmt->split_half)
    {
      gdb_mpfr top (to), bot (to);

      from_target (fmt->split_half, from, top);
      /* Preserve the sign of 0, which is the sign of the top half.  */
      if (mpfr_zero_p (top.val))
	{
	  mpfr_set (to.val, top.val, MPFR_RNDN);
	  return;
	}
      from_target (fmt->split_half,
	       from + fmt->totalsize / FLOATFORMAT_CHAR_BIT / 2, bot);
      mpfr_add (to.val, top.val, bot.val, MPFR_RNDN);
      return;
    }

  exponent = get_field (from, order, fmt->totalsize, fmt->exp_start,
			fmt->exp_len);
  /* Note that if exponent indicates a NaN, we can't really do anything useful
     (not knowing if the host has NaN's, or how to build one).  So it will
     end up as an infinity or something close; that is OK.  */

  mant_bits_left = fmt->man_len;
  mant_off = fmt->man_start;
  mpfr_set_zero (to.val, 0);

  special_exponent = exponent == 0 || exponent == fmt->exp_nan;

  /* Don't bias NaNs.  Use minimum exponent for denorms.  For
     simplicity, we don't check for zero as the exponent doesn't matter.
     Note the cast to int; exp_bias is unsigned, so it's important to
     make sure the operation is done in signed arithmetic.  */
  if (!special_exponent)
    exponent -= fmt->exp_bias;
  else if (exponent == 0)
    exponent = 1 - fmt->exp_bias;

  /* Build the result algebraically.  Might go infinite, underflow, etc;
     who cares.  */

  /* If this format uses a hidden bit, explicitly add it in now.  Otherwise,
     increment the exponent by one to account for the integer bit.  */

  if (!special_exponent)
    {
      if (fmt->intbit == floatformat_intbit_no)
	mpfr_set_ui_2exp (to.val, 1, exponent, MPFR_RNDN);
      else
	exponent++;
    }

  gdb_mpfr tmp (to);

  while (mant_bits_left > 0)
    {
      mant_bits = std::min (mant_bits_left, 32);

      mant = get_field (from, order, fmt->totalsize, mant_off, mant_bits);

      mpfr_set_ui (tmp.val, mant, MPFR_RNDN);
      mpfr_mul_2si (tmp.val, tmp.val, exponent - mant_bits, MPFR_RNDN);
      mpfr_add (to.val, to.val, tmp.val, MPFR_RNDN);
      exponent -= mant_bits;
      mant_off += mant_bits;
      mant_bits_left -= mant_bits;
    }

  /* Negate it if negative.  */
  if (get_field (from, order, fmt->totalsize, fmt->sign_start, 1))
    mpfr_neg (to.val, to.val, MPFR_RNDN);
}

void
mpfr_float_ops::from_target (const struct type *type,
			     const gdb_byte *from, gdb_mpfr &to) const
{
  from_target (floatformat_from_type (type), from, to);
}

void
mpfr_float_ops::to_target (const struct floatformat *fmt,
			   const gdb_mpfr &from, gdb_byte *orig_to) const
{
  unsigned char *to = orig_to;
  mpfr_exp_t exponent;
  unsigned int mant_bits, mant_off;
  int mant_bits_left;
  enum floatformat_byteorders order = fmt->byteorder;
  unsigned char newto[FLOATFORMAT_LARGEST_BYTES];

  if (order != floatformat_little)
    order = floatformat_big;

  if (order != fmt->byteorder)
    to = newto;

  memset (to, 0, floatformat_totalsize_bytes (fmt));

  if (fmt->split_half)
    {
      gdb_mpfr top (from), bot (from);

      mpfr_set (top.val, from.val, MPFR_RNDN);
      /* If the rounded top half is Inf, the bottom must be 0 not NaN
	 or Inf.  */
      if (mpfr_inf_p (top.val))
	mpfr_set_zero (bot.val, 0);
      else
	mpfr_sub (bot.val, from.val, top.val, MPFR_RNDN);

      to_target (fmt->split_half, top, to);
      to_target (fmt->split_half, bot,
		 to + fmt->totalsize / FLOATFORMAT_CHAR_BIT / 2);
      return;
    }

  gdb_mpfr tmp (from);

  if (mpfr_zero_p (from.val))
    goto finalize_byteorder;	/* Result is zero */

  mpfr_set (tmp.val, from.val, MPFR_RNDN);

  if (mpfr_nan_p (tmp.val))	/* Result is NaN */
    {
      /* From is NaN */
      put_field (to, order, fmt->totalsize, fmt->exp_start,
		 fmt->exp_len, fmt->exp_nan);
      /* Be sure it's not infinity, but NaN value is irrel.  */
      put_field (to, order, fmt->totalsize, fmt->man_start,
		 fmt->man_len, 1);
      goto finalize_byteorder;
    }

  /* If negative, set the sign bit.  */
  if (mpfr_sgn (tmp.val) < 0)
    {
      put_field (to, order, fmt->totalsize, fmt->sign_start, 1, 1);
      mpfr_neg (tmp.val, tmp.val, MPFR_RNDN);
    }

  if (mpfr_inf_p (tmp.val))		/* Result is Infinity.  */
    {
      /* Infinity exponent is same as NaN's.  */
      put_field (to, order, fmt->totalsize, fmt->exp_start,
		 fmt->exp_len, fmt->exp_nan);
      /* Infinity mantissa is all zeroes.  */
      put_field (to, order, fmt->totalsize, fmt->man_start,
		 fmt->man_len, 0);
      goto finalize_byteorder;
    }

  mpfr_frexp (&exponent, tmp.val, tmp.val, MPFR_RNDN);

  if (exponent + fmt->exp_bias <= 0)
    {
      /* The value is too small to be expressed in the destination
	 type (not enough bits in the exponent.  Treat as 0.  */
      put_field (to, order, fmt->totalsize, fmt->exp_start,
		 fmt->exp_len, 0);
      put_field (to, order, fmt->totalsize, fmt->man_start,
		 fmt->man_len, 0);
      goto finalize_byteorder;
    }

  if (exponent + fmt->exp_bias >= (1 << fmt->exp_len))
    {
      /* The value is too large to fit into the destination.
	 Treat as infinity.  */
      put_field (to, order, fmt->totalsize, fmt->exp_start,
		 fmt->exp_len, fmt->exp_nan);
      put_field (to, order, fmt->totalsize, fmt->man_start,
		 fmt->man_len, 0);
      goto finalize_byteorder;
    }

  put_field (to, order, fmt->totalsize, fmt->exp_start, fmt->exp_len,
	     exponent + fmt->exp_bias - 1);

  mant_bits_left = fmt->man_len;
  mant_off = fmt->man_start;
  while (mant_bits_left > 0)
    {
      unsigned long mant_long;

      mant_bits = mant_bits_left < 32 ? mant_bits_left : 32;

      mpfr_mul_2ui (tmp.val, tmp.val, 32, MPFR_RNDN);
      mant_long = mpfr_get_ui (tmp.val, MPFR_RNDZ) & 0xffffffffL;
      mpfr_sub_ui (tmp.val, tmp.val, mant_long, MPFR_RNDZ);

      /* If the integer bit is implicit, then we need to discard it.
	 If we are discarding a zero, we should be (but are not) creating
	 a denormalized number which means adjusting the exponent
	 (I think).  */
      if (mant_bits_left == fmt->man_len
	  && fmt->intbit == floatformat_intbit_no)
	{
	  mant_long <<= 1;
	  mant_long &= 0xffffffffL;
	  /* If we are processing the top 32 mantissa bits of a doublest
	     so as to convert to a float value with implied integer bit,
	     we will only be putting 31 of those 32 bits into the
	     final value due to the discarding of the top bit.  In the
	     case of a small float value where the number of mantissa
	     bits is less than 32, discarding the top bit does not alter
	     the number of bits we will be adding to the result.  */
	  if (mant_bits == 32)
	    mant_bits -= 1;
	}

      if (mant_bits < 32)
	{
	  /* The bits we want are in the most significant MANT_BITS bits of
	     mant_long.  Move them to the least significant.  */
	  mant_long >>= 32 - mant_bits;
	}

      put_field (to, order, fmt->totalsize,
		 mant_off, mant_bits, mant_long);
      mant_off += mant_bits;
      mant_bits_left -= mant_bits;
    }

 finalize_byteorder:
  /* Do we need to byte-swap the words in the result?  */
  if (order != fmt->byteorder)
    floatformat_normalize_byteorder (fmt, newto, orig_to);
}

void
mpfr_float_ops::to_target (const struct type *type,
			   const gdb_mpfr &from, gdb_byte *to) const
{
  /* Ensure possible padding bytes in the target buffer are zeroed out.  */
  memset (to, 0, type->length ());

  to_target (floatformat_from_type (type), from, to);
}

/* Convert the byte-stream ADDR, interpreted as floating-point type TYPE,
   to a string, optionally using the print format FORMAT.  */
std::string
mpfr_float_ops::to_string (const gdb_byte *addr,
			   const struct type *type,
			   const char *format) const
{
  const struct floatformat *fmt = floatformat_from_type (type);

  /* Unless we need to adhere to a specific format, provide special
     output for certain cases.  */
  if (format == nullptr)
    {
      /* Detect invalid representations.  */
      if (!floatformat_is_valid (fmt, addr))
	return "<invalid float value>";

      /* Handle NaN and Inf.  */
      enum float_kind kind = floatformat_classify (fmt, addr);
      if (kind == float_nan)
	{
	  const char *sign = floatformat_is_negative (fmt, addr)? "-" : "";
	  const char *mantissa = floatformat_mantissa (fmt, addr);
	  return string_printf ("%snan(0x%s)", sign, mantissa);
	}
      else if (kind == float_infinite)
	{
	  const char *sign = floatformat_is_negative (fmt, addr)? "-" : "";
	  return string_printf ("%sinf", sign);
	}
    }

  /* Determine the format string to use on the host side.  */
  std::string host_format = floatformat_printf_format (fmt, format, 'R');

  gdb_mpfr tmp (type);
  from_target (type, addr, tmp);

  int size = mpfr_snprintf (NULL, 0, host_format.c_str (), tmp.val);
  std::string str (size, '\0');
  mpfr_sprintf (&str[0], host_format.c_str (), tmp.val);

  return str;
}

/* Parse string STRING into a target floating-number of type TYPE and
   store it as byte-stream ADDR.  Return whether parsing succeeded.  */
bool
mpfr_float_ops::from_string (gdb_byte *addr,
			     const struct type *type,
			     const std::string &in) const
{
  gdb_mpfr tmp (type);

  char *endptr;
  mpfr_strtofr (tmp.val, in.c_str (), &endptr, 0, MPFR_RNDN);

  /* We only accept the whole string.  */
  if (*endptr)
    return false;

  to_target (type, tmp, addr);
  return true;
}

/* Convert the byte-stream ADDR, interpreted as floating-point type TYPE,
   to an integer value (rounding towards zero).  */
LONGEST
mpfr_float_ops::to_longest (const gdb_byte *addr,
			    const struct type *type) const
{
  gdb_mpfr tmp (type);
  from_target (type, addr, tmp);
  return mpfr_get_sj (tmp.val, MPFR_RNDZ);
}

/* Convert signed integer VAL to a target floating-number of type TYPE
   and store it as byte-stream ADDR.  */
void
mpfr_float_ops::from_longest (gdb_byte *addr,
			      const struct type *type,
			      LONGEST val) const
{
  gdb_mpfr tmp (type);
  mpfr_set_sj (tmp.val, val, MPFR_RNDN);
  to_target (type, tmp, addr);
}

/* Convert unsigned integer VAL to a target floating-number of type TYPE
   and store it as byte-stream ADDR.  */
void
mpfr_float_ops::from_ulongest (gdb_byte *addr,
			       const struct type *type,
			       ULONGEST val) const
{
  gdb_mpfr tmp (type);
  mpfr_set_uj (tmp.val, val, MPFR_RNDN);
  to_target (type, tmp, addr);
}

/* Convert the byte-stream ADDR, interpreted as floating-point type TYPE,
   to a floating-point value in the host "double" format.  */
double
mpfr_float_ops::to_host_double (const gdb_byte *addr,
				const struct type *type) const
{
  gdb_mpfr tmp (type);
  from_target (type, addr, tmp);
  return mpfr_get_d (tmp.val, MPFR_RNDN);
}

/* Convert floating-point value VAL in the host "double" format to a target
   floating-number of type TYPE and store it as byte-stream ADDR.  */
void
mpfr_float_ops::from_host_double (gdb_byte *addr,
				  const struct type *type,
				  double val) const
{
  gdb_mpfr tmp (type);
  mpfr_set_d (tmp.val, val, MPFR_RNDN);
  to_target (type, tmp, addr);
}

/* Convert a floating-point number of type FROM_TYPE from the target
   byte-stream FROM to a floating-point number of type TO_TYPE, and
   store it to the target byte-stream TO.  */
void
mpfr_float_ops::convert (const gdb_byte *from,
			 const struct type *from_type,
			 gdb_byte *to,
			 const struct type *to_type) const
{
  gdb_mpfr from_tmp (from_type), to_tmp (to_type);
  from_target (from_type, from, from_tmp);
  mpfr_set (to_tmp.val, from_tmp.val, MPFR_RNDN);
  to_target (to_type, to_tmp, to);
}

/* Perform the binary operation indicated by OPCODE, using as operands the
   target byte streams X and Y, interpreted as floating-point numbers of
   types TYPE_X and TYPE_Y, respectively.  Convert the result to type
   TYPE_RES and store it into the byte-stream RES.  */
void
mpfr_float_ops::binop (enum exp_opcode op,
		       const gdb_byte *x, const struct type *type_x,
		       const gdb_byte *y, const struct type *type_y,
		       gdb_byte *res, const struct type *type_res) const
{
  gdb_mpfr x_tmp (type_x), y_tmp (type_y), tmp (type_res);

  from_target (type_x, x, x_tmp);
  from_target (type_y, y, y_tmp);

  switch (op)
    {
      case BINOP_ADD:
	mpfr_add (tmp.val, x_tmp.val, y_tmp.val, MPFR_RNDN);
	break;

      case BINOP_SUB:
	mpfr_sub (tmp.val, x_tmp.val, y_tmp.val, MPFR_RNDN);
	break;

      case BINOP_MUL:
	mpfr_mul (tmp.val, x_tmp.val, y_tmp.val, MPFR_RNDN);
	break;

      case BINOP_DIV:
	mpfr_div (tmp.val, x_tmp.val, y_tmp.val, MPFR_RNDN);
	break;

      case BINOP_EXP:
	mpfr_pow (tmp.val, x_tmp.val, y_tmp.val, MPFR_RNDN);
	break;

      case BINOP_MIN:
	mpfr_min (tmp.val, x_tmp.val, y_tmp.val, MPFR_RNDN);
	break;

      case BINOP_MAX:
	mpfr_max (tmp.val, x_tmp.val, y_tmp.val, MPFR_RNDN);
	break;

      default:
	error (_("Integer-only operation on floating point number."));
	break;
    }

  to_target (type_res, tmp, res);
}

/* Compare the two target byte streams X and Y, interpreted as floating-point
   numbers of types TYPE_X and TYPE_Y, respectively.  Return zero if X and Y
   are equal, -1 if X is less than Y, and 1 otherwise.  */
int
mpfr_float_ops::compare (const gdb_byte *x, const struct type *type_x,
			 const gdb_byte *y, const struct type *type_y) const
{
  gdb_mpfr x_tmp (type_x), y_tmp (type_y);

  from_target (type_x, x, x_tmp);
  from_target (type_y, y, y_tmp);

  if (mpfr_equal_p (x_tmp.val, y_tmp.val))
    return 0;
  else if (mpfr_less_p (x_tmp.val, y_tmp.val))
    return -1;
  else
    return 1;
}


/* Helper routines operating on decimal floating-point data.  */

/* Decimal floating point is one of the extension to IEEE 754, which is
   described in http://grouper.ieee.org/groups/754/revision.html and
   http://www2.hursley.ibm.com/decimal/.  It completes binary floating
   point by representing floating point more exactly.  */

/* The order of the following headers is important for making sure
   decNumber structure is large enough to hold decimal128 digits.  */

#include "dpd/decimal128.h"
#include "dpd/decimal64.h"
#include "dpd/decimal32.h"

/* When using decimal128, this is the maximum string length + 1
   (value comes from libdecnumber's DECIMAL128_String constant).  */
#define MAX_DECIMAL_STRING  43

/* In GDB, we are using an array of gdb_byte to represent decimal values.
   They are stored in host byte order.  This routine does the conversion if
   the target byte order is different.  */
static void
match_endianness (const gdb_byte *from, const struct type *type, gdb_byte *to)
{
  gdb_assert (type->code () == TYPE_CODE_DECFLOAT);

  int len = type->length ();
  int i;

#if WORDS_BIGENDIAN
#define OPPOSITE_BYTE_ORDER BFD_ENDIAN_LITTLE
#else
#define OPPOSITE_BYTE_ORDER BFD_ENDIAN_BIG
#endif

  if (type_byte_order (type) == OPPOSITE_BYTE_ORDER)
    for (i = 0; i < len; i++)
      to[i] = from[len - i - 1];
  else
    for (i = 0; i < len; i++)
      to[i] = from[i];

  return;
}

/* Helper function to get the appropriate libdecnumber context for each size
   of decimal float.  */
static void
set_decnumber_context (decContext *ctx, const struct type *type)
{
  gdb_assert (type->code () == TYPE_CODE_DECFLOAT);

  switch (type->length ())
    {
      case 4:
	decContextDefault (ctx, DEC_INIT_DECIMAL32);
	break;
      case 8:
	decContextDefault (ctx, DEC_INIT_DECIMAL64);
	break;
      case 16:
	decContextDefault (ctx, DEC_INIT_DECIMAL128);
	break;
    }

  ctx->traps = 0;
}

/* Check for errors signaled in the decimal context structure.  */
static void
decimal_check_errors (decContext *ctx)
{
  /* An error here could be a division by zero, an overflow, an underflow or
     an invalid operation (from the DEC_Errors constant in decContext.h).
     Since GDB doesn't complain about division by zero, overflow or underflow
     errors for binary floating, we won't complain about them for decimal
     floating either.  */
  if (ctx->status & DEC_IEEE_854_Invalid_operation)
    {
      /* Leave only the error bits in the status flags.  */
      ctx->status &= DEC_IEEE_854_Invalid_operation;
      error (_("Cannot perform operation: %s"),
	     decContextStatusToString (ctx));
    }
}

/* Helper function to convert from libdecnumber's appropriate representation
   for computation to each size of decimal float.  */
static void
decimal_from_number (const decNumber *from,
		     gdb_byte *to, const struct type *type)
{
  gdb_byte dec[16];

  decContext set;

  set_decnumber_context (&set, type);

  switch (type->length ())
    {
      case 4:
	decimal32FromNumber ((decimal32 *) dec, from, &set);
	break;
      case 8:
	decimal64FromNumber ((decimal64 *) dec, from, &set);
	break;
      case 16:
	decimal128FromNumber ((decimal128 *) dec, from, &set);
	break;
      default:
	error (_("Unknown decimal floating point type."));
	break;
    }

  match_endianness (dec, type, to);
}

/* Helper function to convert each size of decimal float to libdecnumber's
   appropriate representation for computation.  */
static void
decimal_to_number (const gdb_byte *addr, const struct type *type,
		   decNumber *to)
{
  gdb_byte dec[16];
  match_endianness (addr, type, dec);

  switch (type->length ())
    {
      case 4:
	decimal32ToNumber ((decimal32 *) dec, to);
	break;
      case 8:
	decimal64ToNumber ((decimal64 *) dec, to);
	break;
      case 16:
	decimal128ToNumber ((decimal128 *) dec, to);
	break;
      default:
	error (_("Unknown decimal floating point type."));
	break;
    }
}

/* Returns true if ADDR (which is of type TYPE) is the number zero.  */
static bool
decimal_is_zero (const gdb_byte *addr, const struct type *type)
{
  decNumber number;

  decimal_to_number (addr, type, &number);

  return decNumberIsZero (&number);
}


/* Implementation of target_float_ops using the libdecnumber decNumber type
   as intermediate format.  */

class decimal_float_ops : public target_float_ops
{
public:
  std::string to_string (const gdb_byte *addr, const struct type *type,
			 const char *format) const override;
  bool from_string (gdb_byte *addr, const struct type *type,
		    const std::string &string) const override;

  LONGEST to_longest (const gdb_byte *addr,
		      const struct type *type) const override;
  void from_longest (gdb_byte *addr, const struct type *type,
		     LONGEST val) const override;
  void from_ulongest (gdb_byte *addr, const struct type *type,
		      ULONGEST val) const override;
  double to_host_double (const gdb_byte *addr,
			 const struct type *type) const override
  {
    /* We don't support conversions between target decimal floating-point
       types and the host double type.  */
    gdb_assert_not_reached ("invalid operation on decimal float");
  }
  void from_host_double (gdb_byte *addr, const struct type *type,
			 double val) const override
  {
    /* We don't support conversions between target decimal floating-point
       types and the host double type.  */
    gdb_assert_not_reached ("invalid operation on decimal float");
  }
  void convert (const gdb_byte *from, const struct type *from_type,
		gdb_byte *to, const struct type *to_type) const override;

  void binop (enum exp_opcode opcode,
	      const gdb_byte *x, const struct type *type_x,
	      const gdb_byte *y, const struct type *type_y,
	      gdb_byte *res, const struct type *type_res) const override;
  int compare (const gdb_byte *x, const struct type *type_x,
	       const gdb_byte *y, const struct type *type_y) const override;
};

/* Convert decimal type to its string representation.  LEN is the length
   of the decimal type, 4 bytes for decimal32, 8 bytes for decimal64 and
   16 bytes for decimal128.  */
std::string
decimal_float_ops::to_string (const gdb_byte *addr, const struct type *type,
			      const char *format = nullptr) const
{
  gdb_byte dec[16];

  match_endianness (addr, type, dec);

  if (format != nullptr)
    {
      /* We don't handle format strings (yet).  If the host printf supports
	 decimal floating point types, just use this.  Otherwise, fall back
	 to printing the number while ignoring the format string.  */
#if defined (PRINTF_HAS_DECFLOAT)
      /* FIXME: This makes unwarranted assumptions about the host ABI!  */
      return string_printf (format, dec);
#endif
    }

  std::string result;
  result.resize (MAX_DECIMAL_STRING);

  switch (type->length ())
    {
      case 4:
	decimal32ToString ((decimal32 *) dec, &result[0]);
	break;
      case 8:
	decimal64ToString ((decimal64 *) dec, &result[0]);
	break;
      case 16:
	decimal128ToString ((decimal128 *) dec, &result[0]);
	break;
      default:
	error (_("Unknown decimal floating point type."));
	break;
    }

  return result;
}

/* Convert the string form of a decimal value to its decimal representation.
   LEN is the length of the decimal type, 4 bytes for decimal32, 8 bytes for
   decimal64 and 16 bytes for decimal128.  */
bool
decimal_float_ops::from_string (gdb_byte *addr, const struct type *type,
				const std::string &string) const
{
  decContext set;
  gdb_byte dec[16];

  set_decnumber_context (&set, type);

  switch (type->length ())
    {
      case 4:
	decimal32FromString ((decimal32 *) dec, string.c_str (), &set);
	break;
      case 8:
	decimal64FromString ((decimal64 *) dec, string.c_str (), &set);
	break;
      case 16:
	decimal128FromString ((decimal128 *) dec, string.c_str (), &set);
	break;
      default:
	error (_("Unknown decimal floating point type."));
	break;
    }

  match_endianness (dec, type, addr);

  /* Check for errors in the DFP operation.  */
  decimal_check_errors (&set);

  return true;
}

/* Converts a LONGEST to a decimal float of specified LEN bytes.  */
void
decimal_float_ops::from_longest (gdb_byte *addr, const struct type *type,
				 LONGEST from) const
{
  decNumber number;

  if ((int32_t) from != from)
    /* libdecnumber can convert only 32-bit integers.  */
    error (_("Conversion of large integer to a "
	     "decimal floating type is not supported."));

  decNumberFromInt32 (&number, (int32_t) from);

  decimal_from_number (&number, addr, type);
}

/* Converts a ULONGEST to a decimal float of specified LEN bytes.  */
void
decimal_float_ops::from_ulongest (gdb_byte *addr, const struct type *type,
				  ULONGEST from) const
{
  decNumber number;

  if ((uint32_t) from != from)
    /* libdecnumber can convert only 32-bit integers.  */
    error (_("Conversion of large integer to a "
	     "decimal floating type is not supported."));

  decNumberFromUInt32 (&number, (uint32_t) from);

  decimal_from_number (&number, addr, type);
}

/* Converts a decimal float of LEN bytes to a LONGEST.  */
LONGEST
decimal_float_ops::to_longest (const gdb_byte *addr,
			       const struct type *type) const
{
  /* libdecnumber has a function to convert from decimal to integer, but
     it doesn't work when the decimal number has a fractional part.  */
  std::string str = to_string (addr, type);
  return strtoll (str.c_str (), NULL, 10);
}

/* Perform operation OP with operands X and Y with sizes LEN_X and LEN_Y
   and byte orders BYTE_ORDER_X and BYTE_ORDER_Y, and store value in
   RESULT with size LEN_RESULT and byte order BYTE_ORDER_RESULT.  */
void
decimal_float_ops::binop (enum exp_opcode op,
			  const gdb_byte *x, const struct type *type_x,
			  const gdb_byte *y, const struct type *type_y,
			  gdb_byte *res, const struct type *type_res) const
{
  decContext set;
  decNumber number1, number2, number3;

  decimal_to_number (x, type_x, &number1);
  decimal_to_number (y, type_y, &number2);

  set_decnumber_context (&set, type_res);

  switch (op)
    {
      case BINOP_ADD:
	decNumberAdd (&number3, &number1, &number2, &set);
	break;
      case BINOP_SUB:
	decNumberSubtract (&number3, &number1, &number2, &set);
	break;
      case BINOP_MUL:
	decNumberMultiply (&number3, &number1, &number2, &set);
	break;
      case BINOP_DIV:
	decNumberDivide (&number3, &number1, &number2, &set);
	break;
      case BINOP_EXP:
	decNumberPower (&number3, &number1, &number2, &set);
	break;
     default:
	error (_("Operation not valid for decimal floating point number."));
	break;
    }

  /* Check for errors in the DFP operation.  */
  decimal_check_errors (&set);

  decimal_from_number (&number3, res, type_res);
}

/* Compares two numbers numerically.  If X is less than Y then the return value
   will be -1.  If they are equal, then the return value will be 0.  If X is
   greater than the Y then the return value will be 1.  */
int
decimal_float_ops::compare (const gdb_byte *x, const struct type *type_x,
			    const gdb_byte *y, const struct type *type_y) const
{
  decNumber number1, number2, result;
  decContext set;
  const struct type *type_result;

  decimal_to_number (x, type_x, &number1);
  decimal_to_number (y, type_y, &number2);

  /* Perform the comparison in the larger of the two sizes.  */
  type_result = type_x->length () > type_y->length () ? type_x : type_y;
  set_decnumber_context (&set, type_result);

  decNumberCompare (&result, &number1, &number2, &set);

  /* Check for errors in the DFP operation.  */
  decimal_check_errors (&set);

  if (decNumberIsNaN (&result))
    error (_("Comparison with an invalid number (NaN)."));
  else if (decNumberIsZero (&result))
    return 0;
  else if (decNumberIsNegative (&result))
    return -1;
  else
    return 1;
}

/* Convert a decimal value from a decimal type with LEN_FROM bytes to a
   decimal type with LEN_TO bytes.  */
void
decimal_float_ops::convert (const gdb_byte *from, const struct type *from_type,
			    gdb_byte *to, const struct type *to_type) const
{
  decNumber number;

  decimal_to_number (from, from_type, &number);
  decimal_from_number (&number, to, to_type);
}


/* Typed floating-point routines.  These routines operate on floating-point
   values in target format, represented by a byte buffer interpreted as a
   "struct type", which may be either a binary or decimal floating-point
   type (TYPE_CODE_FLT or TYPE_CODE_DECFLOAT).  */

/* Return whether TYPE1 and TYPE2 are of the same category (binary or
   decimal floating-point).  */
static bool
target_float_same_category_p (const struct type *type1,
			      const struct type *type2)
{
  return type1->code () == type2->code ();
}

/* Return whether TYPE1 and TYPE2 use the same floating-point format.  */
static bool
target_float_same_format_p (const struct type *type1,
			    const struct type *type2)
{
  if (!target_float_same_category_p (type1, type2))
    return false;

  switch (type1->code ())
    {
      case TYPE_CODE_FLT:
	return floatformat_from_type (type1) == floatformat_from_type (type2);

      case TYPE_CODE_DECFLOAT:
	return (type1->length () == type2->length ()
		&& (type_byte_order (type1)
		    == type_byte_order (type2)));

      default:
	gdb_assert_not_reached ("unexpected type code");
    }
}

/* Return the size (without padding) of the target floating-point
   format used by TYPE.  */
static int
target_float_format_length (const struct type *type)
{
  switch (type->code ())
    {
      case TYPE_CODE_FLT:
	return floatformat_totalsize_bytes (floatformat_from_type (type));

      case TYPE_CODE_DECFLOAT:
	return type->length ();

      default:
	gdb_assert_not_reached ("unexpected type code");
    }
}

/* Identifiers of available host-side intermediate formats.  These must
   be sorted so the that the more "general" kinds come later.  */
enum target_float_ops_kind
{
  /* Target binary floating-point formats that match a host format.  */
  host_float = 0,
  host_double,
  host_long_double,
  /* Any other target binary floating-point format.  */
  binary,
  /* Any target decimal floating-point format.  */
  decimal
};

/* Given a target type TYPE, choose the best host-side intermediate format
   to perform operations on TYPE in.  */
static enum target_float_ops_kind
get_target_float_ops_kind (const struct type *type)
{
  switch (type->code ())
    {
      case TYPE_CODE_FLT:
	{
	  const struct floatformat *fmt = floatformat_from_type (type);

	  /* Binary floating-point formats matching a host format.  */
	  if (fmt == host_float_format)
	    return target_float_ops_kind::host_float;
	  if (fmt == host_double_format)
	    return target_float_ops_kind::host_double;
	  if (fmt == host_long_double_format)
	    return target_float_ops_kind::host_long_double;

	  /* Any other binary floating-point format.  */
	  return target_float_ops_kind::binary;
	}

      case TYPE_CODE_DECFLOAT:
	{
	  /* Any decimal floating-point format.  */
	  return target_float_ops_kind::decimal;
	}

      default:
	gdb_assert_not_reached ("unexpected type code");
    }
}

/* Return target_float_ops to perform operations for KIND.  */
static const target_float_ops *
get_target_float_ops (enum target_float_ops_kind kind)
{
  switch (kind)
    {
      /* If the type format matches one of the host floating-point
	 types, use that type as intermediate format.  */
      case target_float_ops_kind::host_float:
	{
	  static host_float_ops<float> host_float_ops_float;
	  return &host_float_ops_float;
	}

      case target_float_ops_kind::host_double:
	{
	  static host_float_ops<double> host_float_ops_double;
	  return &host_float_ops_double;
	}

      case target_float_ops_kind::host_long_double:
	{
	  static host_float_ops<long double> host_float_ops_long_double;
	  return &host_float_ops_long_double;
	}

      /* For binary floating-point formats that do not match any host format,
	 use mpfr_t as intermediate format to provide precise target-floating
	 point emulation.  However, if the MPFR library is not available,
	 use the largest host floating-point type as intermediate format.  */
      case target_float_ops_kind::binary:
	{
	  static mpfr_float_ops binary_float_ops;
	  return &binary_float_ops;
	}

      /* For decimal floating-point types, always use the libdecnumber
	 decNumber type as intermediate format.  */
      case target_float_ops_kind::decimal:
	{
	  static decimal_float_ops decimal_float_ops;
	  return &decimal_float_ops;
	}

      default:
	gdb_assert_not_reached ("unexpected target_float_ops_kind");
    }
}

/* Given a target type TYPE, determine the best host-side intermediate format
   to perform operations on TYPE in.  */
static const target_float_ops *
get_target_float_ops (const struct type *type)
{
  enum target_float_ops_kind kind = get_target_float_ops_kind (type);
  return get_target_float_ops (kind);
}

/* The same for operations involving two target types TYPE1 and TYPE2.  */
static const target_float_ops *
get_target_float_ops (const struct type *type1, const struct type *type2)
{
  gdb_assert (type1->code () == type2->code ());

  enum target_float_ops_kind kind1 = get_target_float_ops_kind (type1);
  enum target_float_ops_kind kind2 = get_target_float_ops_kind (type2);

  /* Given the way the kinds are sorted, we simply choose the larger one;
     this will be able to hold values of either type.  */
  return get_target_float_ops (std::max (kind1, kind2));
}

/* Return whether the byte-stream ADDR holds a valid value of
   floating-point type TYPE.  */
bool
target_float_is_valid (const gdb_byte *addr, const struct type *type)
{
  if (type->code () == TYPE_CODE_FLT)
    return floatformat_is_valid (floatformat_from_type (type), addr);

  if (type->code () == TYPE_CODE_DECFLOAT)
    return true;

  gdb_assert_not_reached ("unexpected type code");
}

/* Return whether the byte-stream ADDR, interpreted as floating-point
   type TYPE, is numerically equal to zero (of either sign).  */
bool
target_float_is_zero (const gdb_byte *addr, const struct type *type)
{
  if (type->code () == TYPE_CODE_FLT)
    return (floatformat_classify (floatformat_from_type (type), addr)
	    == float_zero);

  if (type->code () == TYPE_CODE_DECFLOAT)
    return decimal_is_zero (addr, type);

  gdb_assert_not_reached ("unexpected type code");
}

/* Convert the byte-stream ADDR, interpreted as floating-point type TYPE,
   to a string, optionally using the print format FORMAT.  */
std::string
target_float_to_string (const gdb_byte *addr, const struct type *type,
			const char *format)
{
  /* Unless we need to adhere to a specific format, provide special
     output for special cases of binary floating-point numbers.  */
  if (format == nullptr && type->code () == TYPE_CODE_FLT)
    {
      const struct floatformat *fmt = floatformat_from_type (type);

      /* Detect invalid representations.  */
      if (!floatformat_is_valid (fmt, addr))
	return "<invalid float value>";

      /* Handle NaN and Inf.  */
      enum float_kind kind = floatformat_classify (fmt, addr);
      if (kind == float_nan)
	{
	  const char *sign = floatformat_is_negative (fmt, addr)? "-" : "";
	  const char *mantissa = floatformat_mantissa (fmt, addr);
	  return string_printf ("%snan(0x%s)", sign, mantissa);
	}
      else if (kind == float_infinite)
	{
	  const char *sign = floatformat_is_negative (fmt, addr)? "-" : "";
	  return string_printf ("%sinf", sign);
	}
    }

  const target_float_ops *ops = get_target_float_ops (type);
  return ops->to_string (addr, type, format);
}

/* Parse string STRING into a target floating-number of type TYPE and
   store it as byte-stream ADDR.  Return whether parsing succeeded.  */
bool
target_float_from_string (gdb_byte *addr, const struct type *type,
			  const std::string &string)
{
  const target_float_ops *ops = get_target_float_ops (type);
  return ops->from_string (addr, type, string);
}

/* Convert the byte-stream ADDR, interpreted as floating-point type TYPE,
   to an integer value (rounding towards zero).  */
LONGEST
target_float_to_longest (const gdb_byte *addr, const struct type *type)
{
  const target_float_ops *ops = get_target_float_ops (type);
  return ops->to_longest (addr, type);
}

/* Convert signed integer VAL to a target floating-number of type TYPE
   and store it as byte-stream ADDR.  */
void
target_float_from_longest (gdb_byte *addr, const struct type *type,
			   LONGEST val)
{
  const target_float_ops *ops = get_target_float_ops (type);
  ops->from_longest (addr, type, val);
}

/* Convert unsigned integer VAL to a target floating-number of type TYPE
   and store it as byte-stream ADDR.  */
void
target_float_from_ulongest (gdb_byte *addr, const struct type *type,
			    ULONGEST val)
{
  const target_float_ops *ops = get_target_float_ops (type);
  ops->from_ulongest (addr, type, val);
}

/* Convert the byte-stream ADDR, interpreted as floating-point type TYPE,
   to a floating-point value in the host "double" format.  */
double
target_float_to_host_double (const gdb_byte *addr,
			     const struct type *type)
{
  const target_float_ops *ops = get_target_float_ops (type);
  return ops->to_host_double (addr, type);
}

/* Convert floating-point value VAL in the host "double" format to a target
   floating-number of type TYPE and store it as byte-stream ADDR.  */
void
target_float_from_host_double (gdb_byte *addr, const struct type *type,
			       double val)
{
  const target_float_ops *ops = get_target_float_ops (type);
  ops->from_host_double (addr, type, val);
}

/* Convert a floating-point number of type FROM_TYPE from the target
   byte-stream FROM to a floating-point number of type TO_TYPE, and
   store it to the target byte-stream TO.  */
void
target_float_convert (const gdb_byte *from, const struct type *from_type,
		      gdb_byte *to, const struct type *to_type)
{
  /* We cannot directly convert between binary and decimal floating-point
     types, so go via an intermediary string.  */
  if (!target_float_same_category_p (from_type, to_type))
    {
      std::string str = target_float_to_string (from, from_type);
      target_float_from_string (to, to_type, str);
      return;
    }

  /* Convert between two different formats in the same category.  */
  if (!target_float_same_format_p (from_type, to_type))
    {
      const target_float_ops *ops = get_target_float_ops (from_type, to_type);
      ops->convert (from, from_type, to, to_type);
      return;
    }

  /* The floating-point formats match, so we simply copy the data, ensuring
     possible padding bytes in the target buffer are zeroed out.  */
  memset (to, 0, to_type->length ());
  memcpy (to, from, target_float_format_length (to_type));
}

/* Perform the binary operation indicated by OPCODE, using as operands the
   target byte streams X and Y, interpreted as floating-point numbers of
   types TYPE_X and TYPE_Y, respectively.  Convert the result to type
   TYPE_RES and store it into the byte-stream RES.

   The three types must either be all binary floating-point types, or else
   all decimal floating-point types.  Binary and decimal floating-point
   types cannot be mixed within a single operation.  */
void
target_float_binop (enum exp_opcode opcode,
		    const gdb_byte *x, const struct type *type_x,
		    const gdb_byte *y, const struct type *type_y,
		    gdb_byte *res, const struct type *type_res)
{
  gdb_assert (target_float_same_category_p (type_x, type_res));
  gdb_assert (target_float_same_category_p (type_y, type_res));

  const target_float_ops *ops = get_target_float_ops (type_x, type_y);
  ops->binop (opcode, x, type_x, y, type_y, res, type_res);
}

/* Compare the two target byte streams X and Y, interpreted as floating-point
   numbers of types TYPE_X and TYPE_Y, respectively.  Return zero if X and Y
   are equal, -1 if X is less than Y, and 1 otherwise.

   The two types must either both be binary floating-point types, or else
   both be decimal floating-point types.  Binary and decimal floating-point
   types cannot compared directly against each other.  */
int
target_float_compare (const gdb_byte *x, const struct type *type_x,
		      const gdb_byte *y, const struct type *type_y)
{
  gdb_assert (target_float_same_category_p (type_x, type_y));

  const target_float_ops *ops = get_target_float_ops (type_x, type_y);
  return ops->compare (x, type_x, y, type_y);
}

