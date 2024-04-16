/* Low level packing and unpacking of values for GDB, the GNU Debugger.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "value.h"
#include "gdbcore.h"
#include "command.h"
#include "gdbcmd.h"
#include "target.h"
#include "language.h"
#include "demangle.h"
#include "regcache.h"
#include "block.h"
#include "target-float.h"
#include "objfiles.h"
#include "valprint.h"
#include "cli/cli-decode.h"
#include "extension.h"
#include <ctype.h>
#include "tracepoint.h"
#include "cp-abi.h"
#include "user-regs.h"
#include <algorithm>
#include <iterator>
#include <map>
#include <utility>
#include <vector>
#include "completer.h"
#include "gdbsupport/selftest.h"
#include "gdbsupport/array-view.h"
#include "cli/cli-style.h"
#include "expop.h"
#include "inferior.h"
#include "varobj.h"

/* Definition of a user function.  */
struct internal_function
{
  /* The name of the function.  It is a bit odd to have this in the
     function itself -- the user might use a differently-named
     convenience variable to hold the function.  */
  char *name;

  /* The handler.  */
  internal_function_fn handler;

  /* User data for the handler.  */
  void *cookie;
};

/* Returns true if the ranges defined by [offset1, offset1+len1) and
   [offset2, offset2+len2) overlap.  */

static bool
ranges_overlap (LONGEST offset1, ULONGEST len1,
		LONGEST offset2, ULONGEST len2)
{
  LONGEST h, l;

  l = std::max (offset1, offset2);
  h = std::min (offset1 + len1, offset2 + len2);
  return (l < h);
}

/* Returns true if RANGES contains any range that overlaps [OFFSET,
   OFFSET+LENGTH).  */

static bool
ranges_contain (const std::vector<range> &ranges, LONGEST offset,
		ULONGEST length)
{
  range what;

  what.offset = offset;
  what.length = length;

  /* We keep ranges sorted by offset and coalesce overlapping and
     contiguous ranges, so to check if a range list contains a given
     range, we can do a binary search for the position the given range
     would be inserted if we only considered the starting OFFSET of
     ranges.  We call that position I.  Since we also have LENGTH to
     care for (this is a range afterall), we need to check if the
     _previous_ range overlaps the I range.  E.g.,

	 R
	 |---|
       |---|    |---|  |------| ... |--|
       0        1      2            N

       I=1

     In the case above, the binary search would return `I=1', meaning,
     this OFFSET should be inserted at position 1, and the current
     position 1 should be pushed further (and before 2).  But, `0'
     overlaps with R.

     Then we need to check if the I range overlaps the I range itself.
     E.g.,

	      R
	      |---|
       |---|    |---|  |-------| ... |--|
       0        1      2             N

       I=1
  */


  auto i = std::lower_bound (ranges.begin (), ranges.end (), what);

  if (i > ranges.begin ())
    {
      const struct range &bef = *(i - 1);

      if (ranges_overlap (bef.offset, bef.length, offset, length))
	return true;
    }

  if (i < ranges.end ())
    {
      const struct range &r = *i;

      if (ranges_overlap (r.offset, r.length, offset, length))
	return true;
    }

  return false;
}

static struct cmd_list_element *functionlist;

value::~value ()
{
  if (this->lval () == lval_computed)
    {
      const struct lval_funcs *funcs = m_location.computed.funcs;

      if (funcs->free_closure)
	funcs->free_closure (this);
    }
  else if (this->lval () == lval_xcallable)
    delete m_location.xm_worker;
}

/* See value.h.  */

struct gdbarch *
value::arch () const
{
  return type ()->arch ();
}

bool
value::bits_available (LONGEST offset, ULONGEST length) const
{
  gdb_assert (!m_lazy);

  /* Don't pretend we have anything available there in the history beyond
     the boundaries of the value recorded.  It's not like inferior memory
     where there is actual stuff underneath.  */
  ULONGEST val_len = TARGET_CHAR_BIT * enclosing_type ()->length ();
  return !((m_in_history
	    && (offset < 0 || offset + length > val_len))
	   || ranges_contain (m_unavailable, offset, length));
}

bool
value::bytes_available (LONGEST offset, ULONGEST length) const
{
  ULONGEST sign = (1ULL << (sizeof (ULONGEST) * 8 - 1)) / TARGET_CHAR_BIT;
  ULONGEST mask = (sign << 1) - 1;

  if (offset != ((offset & mask) ^ sign) - sign
      || length != ((length & mask) ^ sign) - sign
      || (length > 0 && (~offset & (offset + length - 1) & sign) != 0))
    error (_("Integer overflow in data location calculation"));

  return bits_available (offset * TARGET_CHAR_BIT, length * TARGET_CHAR_BIT);
}

bool
value::bits_any_optimized_out (int bit_offset, int bit_length) const
{
  gdb_assert (!m_lazy);

  return ranges_contain (m_optimized_out, bit_offset, bit_length);
}

bool
value::entirely_available ()
{
  /* We can only tell whether the whole value is available when we try
     to read it.  */
  if (m_lazy)
    fetch_lazy ();

  if (m_unavailable.empty ())
    return true;
  return false;
}

/* See value.h.  */

bool
value::entirely_covered_by_range_vector (const std::vector<range> &ranges)
{
  /* We can only tell whether the whole value is optimized out /
     unavailable when we try to read it.  */
  if (m_lazy)
    fetch_lazy ();

  if (ranges.size () == 1)
    {
      const struct range &t = ranges[0];

      if (t.offset == 0
	  && t.length == TARGET_CHAR_BIT * enclosing_type ()->length ())
	return true;
    }

  return false;
}

/* Insert into the vector pointed to by VECTORP the bit range starting of
   OFFSET bits, and extending for the next LENGTH bits.  */

static void
insert_into_bit_range_vector (std::vector<range> *vectorp,
			      LONGEST offset, ULONGEST length)
{
  range newr;

  /* Insert the range sorted.  If there's overlap or the new range
     would be contiguous with an existing range, merge.  */

  newr.offset = offset;
  newr.length = length;

  /* Do a binary search for the position the given range would be
     inserted if we only considered the starting OFFSET of ranges.
     Call that position I.  Since we also have LENGTH to care for
     (this is a range afterall), we need to check if the _previous_
     range overlaps the I range.  E.g., calling R the new range:

       #1 - overlaps with previous

	   R
	   |-...-|
	 |---|     |---|  |------| ... |--|
	 0         1      2            N

	 I=1

     In the case #1 above, the binary search would return `I=1',
     meaning, this OFFSET should be inserted at position 1, and the
     current position 1 should be pushed further (and become 2).  But,
     note that `0' overlaps with R, so we want to merge them.

     A similar consideration needs to be taken if the new range would
     be contiguous with the previous range:

       #2 - contiguous with previous

	    R
	    |-...-|
	 |--|       |---|  |------| ... |--|
	 0          1      2            N

	 I=1

     If there's no overlap with the previous range, as in:

       #3 - not overlapping and not contiguous

	       R
	       |-...-|
	  |--|         |---|  |------| ... |--|
	  0            1      2            N

	 I=1

     or if I is 0:

       #4 - R is the range with lowest offset

	  R
	 |-...-|
		 |--|       |---|  |------| ... |--|
		 0          1      2            N

	 I=0

     ... we just push the new range to I.

     All the 4 cases above need to consider that the new range may
     also overlap several of the ranges that follow, or that R may be
     contiguous with the following range, and merge.  E.g.,

       #5 - overlapping following ranges

	  R
	 |------------------------|
		 |--|       |---|  |------| ... |--|
		 0          1      2            N

	 I=0

       or:

	    R
	    |-------|
	 |--|       |---|  |------| ... |--|
	 0          1      2            N

	 I=1

  */

  auto i = std::lower_bound (vectorp->begin (), vectorp->end (), newr);
  if (i > vectorp->begin ())
    {
      struct range &bef = *(i - 1);

      if (ranges_overlap (bef.offset, bef.length, offset, length))
	{
	  /* #1 */
	  LONGEST l = std::min (bef.offset, offset);
	  LONGEST h = std::max (bef.offset + bef.length, offset + length);

	  bef.offset = l;
	  bef.length = h - l;
	  i--;
	}
      else if (offset == bef.offset + bef.length)
	{
	  /* #2 */
	  bef.length += length;
	  i--;
	}
      else
	{
	  /* #3 */
	  i = vectorp->insert (i, newr);
	}
    }
  else
    {
      /* #4 */
      i = vectorp->insert (i, newr);
    }

  /* Check whether the ranges following the one we've just added or
     touched can be folded in (#5 above).  */
  if (i != vectorp->end () && i + 1 < vectorp->end ())
    {
      int removed = 0;
      auto next = i + 1;

      /* Get the range we just touched.  */
      struct range &t = *i;
      removed = 0;

      i = next;
      for (; i < vectorp->end (); i++)
	{
	  struct range &r = *i;
	  if (r.offset <= t.offset + t.length)
	    {
	      LONGEST l, h;

	      l = std::min (t.offset, r.offset);
	      h = std::max (t.offset + t.length, r.offset + r.length);

	      t.offset = l;
	      t.length = h - l;

	      removed++;
	    }
	  else
	    {
	      /* If we couldn't merge this one, we won't be able to
		 merge following ones either, since the ranges are
		 always sorted by OFFSET.  */
	      break;
	    }
	}

      if (removed != 0)
	vectorp->erase (next, next + removed);
    }
}

void
value::mark_bits_unavailable (LONGEST offset, ULONGEST length)
{
  insert_into_bit_range_vector (&m_unavailable, offset, length);
}

void
value::mark_bytes_unavailable (LONGEST offset, ULONGEST length)
{
  mark_bits_unavailable (offset * TARGET_CHAR_BIT,
			 length * TARGET_CHAR_BIT);
}

/* Find the first range in RANGES that overlaps the range defined by
   OFFSET and LENGTH, starting at element POS in the RANGES vector,
   Returns the index into RANGES where such overlapping range was
   found, or -1 if none was found.  */

static int
find_first_range_overlap (const std::vector<range> *ranges, int pos,
			  LONGEST offset, LONGEST length)
{
  int i;

  for (i = pos; i < ranges->size (); i++)
    {
      const range &r = (*ranges)[i];
      if (ranges_overlap (r.offset, r.length, offset, length))
	return i;
    }

  return -1;
}

/* Compare LENGTH_BITS of memory at PTR1 + OFFSET1_BITS with the memory at
   PTR2 + OFFSET2_BITS.  Return 0 if the memory is the same, otherwise
   return non-zero.

   It must always be the case that:
     OFFSET1_BITS % TARGET_CHAR_BIT == OFFSET2_BITS % TARGET_CHAR_BIT

   It is assumed that memory can be accessed from:
     PTR + (OFFSET_BITS / TARGET_CHAR_BIT)
   to:
     PTR + ((OFFSET_BITS + LENGTH_BITS + TARGET_CHAR_BIT - 1)
	    / TARGET_CHAR_BIT)  */
static int
memcmp_with_bit_offsets (const gdb_byte *ptr1, size_t offset1_bits,
			 const gdb_byte *ptr2, size_t offset2_bits,
			 size_t length_bits)
{
  gdb_assert (offset1_bits % TARGET_CHAR_BIT
	      == offset2_bits % TARGET_CHAR_BIT);

  if (offset1_bits % TARGET_CHAR_BIT != 0)
    {
      size_t bits;
      gdb_byte mask, b1, b2;

      /* The offset from the base pointers PTR1 and PTR2 is not a complete
	 number of bytes.  A number of bits up to either the next exact
	 byte boundary, or LENGTH_BITS (which ever is sooner) will be
	 compared.  */
      bits = TARGET_CHAR_BIT - offset1_bits % TARGET_CHAR_BIT;
      gdb_assert (bits < sizeof (mask) * TARGET_CHAR_BIT);
      mask = (1 << bits) - 1;

      if (length_bits < bits)
	{
	  mask &= ~(gdb_byte) ((1 << (bits - length_bits)) - 1);
	  bits = length_bits;
	}

      /* Now load the two bytes and mask off the bits we care about.  */
      b1 = *(ptr1 + offset1_bits / TARGET_CHAR_BIT) & mask;
      b2 = *(ptr2 + offset2_bits / TARGET_CHAR_BIT) & mask;

      if (b1 != b2)
	return 1;

      /* Now update the length and offsets to take account of the bits
	 we've just compared.  */
      length_bits -= bits;
      offset1_bits += bits;
      offset2_bits += bits;
    }

  if (length_bits % TARGET_CHAR_BIT != 0)
    {
      size_t bits;
      size_t o1, o2;
      gdb_byte mask, b1, b2;

      /* The length is not an exact number of bytes.  After the previous
	 IF.. block then the offsets are byte aligned, or the
	 length is zero (in which case this code is not reached).  Compare
	 a number of bits at the end of the region, starting from an exact
	 byte boundary.  */
      bits = length_bits % TARGET_CHAR_BIT;
      o1 = offset1_bits + length_bits - bits;
      o2 = offset2_bits + length_bits - bits;

      gdb_assert (bits < sizeof (mask) * TARGET_CHAR_BIT);
      mask = ((1 << bits) - 1) << (TARGET_CHAR_BIT - bits);

      gdb_assert (o1 % TARGET_CHAR_BIT == 0);
      gdb_assert (o2 % TARGET_CHAR_BIT == 0);

      b1 = *(ptr1 + o1 / TARGET_CHAR_BIT) & mask;
      b2 = *(ptr2 + o2 / TARGET_CHAR_BIT) & mask;

      if (b1 != b2)
	return 1;

      length_bits -= bits;
    }

  if (length_bits > 0)
    {
      /* We've now taken care of any stray "bits" at the start, or end of
	 the region to compare, the remainder can be covered with a simple
	 memcmp.  */
      gdb_assert (offset1_bits % TARGET_CHAR_BIT == 0);
      gdb_assert (offset2_bits % TARGET_CHAR_BIT == 0);
      gdb_assert (length_bits % TARGET_CHAR_BIT == 0);

      return memcmp (ptr1 + offset1_bits / TARGET_CHAR_BIT,
		     ptr2 + offset2_bits / TARGET_CHAR_BIT,
		     length_bits / TARGET_CHAR_BIT);
    }

  /* Length is zero, regions match.  */
  return 0;
}

/* Helper struct for find_first_range_overlap_and_match and
   value_contents_bits_eq.  Keep track of which slot of a given ranges
   vector have we last looked at.  */

struct ranges_and_idx
{
  /* The ranges.  */
  const std::vector<range> *ranges;

  /* The range we've last found in RANGES.  Given ranges are sorted,
     we can start the next lookup here.  */
  int idx;
};

/* Helper function for value_contents_bits_eq.  Compare LENGTH bits of
   RP1's ranges starting at OFFSET1 bits with LENGTH bits of RP2's
   ranges starting at OFFSET2 bits.  Return true if the ranges match
   and fill in *L and *H with the overlapping window relative to
   (both) OFFSET1 or OFFSET2.  */

static int
find_first_range_overlap_and_match (struct ranges_and_idx *rp1,
				    struct ranges_and_idx *rp2,
				    LONGEST offset1, LONGEST offset2,
				    ULONGEST length, ULONGEST *l, ULONGEST *h)
{
  rp1->idx = find_first_range_overlap (rp1->ranges, rp1->idx,
				       offset1, length);
  rp2->idx = find_first_range_overlap (rp2->ranges, rp2->idx,
				       offset2, length);

  if (rp1->idx == -1 && rp2->idx == -1)
    {
      *l = length;
      *h = length;
      return 1;
    }
  else if (rp1->idx == -1 || rp2->idx == -1)
    return 0;
  else
    {
      const range *r1, *r2;
      ULONGEST l1, h1;
      ULONGEST l2, h2;

      r1 = &(*rp1->ranges)[rp1->idx];
      r2 = &(*rp2->ranges)[rp2->idx];

      /* Get the unavailable windows intersected by the incoming
	 ranges.  The first and last ranges that overlap the argument
	 range may be wider than said incoming arguments ranges.  */
      l1 = std::max (offset1, r1->offset);
      h1 = std::min (offset1 + length, r1->offset + r1->length);

      l2 = std::max (offset2, r2->offset);
      h2 = std::min (offset2 + length, offset2 + r2->length);

      /* Make them relative to the respective start offsets, so we can
	 compare them for equality.  */
      l1 -= offset1;
      h1 -= offset1;

      l2 -= offset2;
      h2 -= offset2;

      /* Different ranges, no match.  */
      if (l1 != l2 || h1 != h2)
	return 0;

      *h = h1;
      *l = l1;
      return 1;
    }
}

/* Helper function for value_contents_eq.  The only difference is that
   this function is bit rather than byte based.

   Compare LENGTH bits of VAL1's contents starting at OFFSET1 bits
   with LENGTH bits of VAL2's contents starting at OFFSET2 bits.
   Return true if the available bits match.  */

bool
value::contents_bits_eq (int offset1, const struct value *val2, int offset2,
			 int length) const
{
  /* Each array element corresponds to a ranges source (unavailable,
     optimized out).  '1' is for VAL1, '2' for VAL2.  */
  struct ranges_and_idx rp1[2], rp2[2];

  /* See function description in value.h.  */
  gdb_assert (!m_lazy && !val2->m_lazy);

  /* We shouldn't be trying to compare past the end of the values.  */
  gdb_assert (offset1 + length
	      <= m_enclosing_type->length () * TARGET_CHAR_BIT);
  gdb_assert (offset2 + length
	      <= val2->m_enclosing_type->length () * TARGET_CHAR_BIT);

  memset (&rp1, 0, sizeof (rp1));
  memset (&rp2, 0, sizeof (rp2));
  rp1[0].ranges = &m_unavailable;
  rp2[0].ranges = &val2->m_unavailable;
  rp1[1].ranges = &m_optimized_out;
  rp2[1].ranges = &val2->m_optimized_out;

  while (length > 0)
    {
      ULONGEST l = 0, h = 0; /* init for gcc -Wall */
      int i;

      for (i = 0; i < 2; i++)
	{
	  ULONGEST l_tmp, h_tmp;

	  /* The contents only match equal if the invalid/unavailable
	     contents ranges match as well.  */
	  if (!find_first_range_overlap_and_match (&rp1[i], &rp2[i],
						   offset1, offset2, length,
						   &l_tmp, &h_tmp))
	    return false;

	  /* We're interested in the lowest/first range found.  */
	  if (i == 0 || l_tmp < l)
	    {
	      l = l_tmp;
	      h = h_tmp;
	    }
	}

      /* Compare the available/valid contents.  */
      if (memcmp_with_bit_offsets (m_contents.get (), offset1,
				   val2->m_contents.get (), offset2, l) != 0)
	return false;

      length -= h;
      offset1 += h;
      offset2 += h;
    }

  return true;
}

/* See value.h.  */

bool
value::contents_eq (LONGEST offset1,
		    const struct value *val2, LONGEST offset2,
		    LONGEST length) const
{
  return contents_bits_eq (offset1 * TARGET_CHAR_BIT,
			   val2, offset2 * TARGET_CHAR_BIT,
			   length * TARGET_CHAR_BIT);
}

/* See value.h.  */

bool
value::contents_eq (const struct value *val2) const
{
  ULONGEST len1 = check_typedef (enclosing_type ())->length ();
  ULONGEST len2 = check_typedef (val2->enclosing_type ())->length ();
  if (len1 != len2)
    return false;
  return contents_eq (0, val2, 0, len1);
}

/* The value-history records all the values printed by print commands
   during this session.  */

static std::vector<value_ref_ptr> value_history;


/* List of all value objects currently allocated
   (except for those released by calls to release_value)
   This is so they can be freed after each command.  */

static std::vector<value_ref_ptr> all_values;

/* See value.h.  */

struct value *
value::allocate_lazy (struct type *type)
{
  struct value *val;

  /* Call check_typedef on our type to make sure that, if TYPE
     is a TYPE_CODE_TYPEDEF, its length is set to the length
     of the target type instead of zero.  However, we do not
     replace the typedef type by the target type, because we want
     to keep the typedef in order to be able to set the VAL's type
     description correctly.  */
  check_typedef (type);

  val = new struct value (type);

  /* Values start out on the all_values chain.  */
  all_values.emplace_back (val);

  return val;
}

/* The maximum size, in bytes, that GDB will try to allocate for a value.
   The initial value of 64k was not selected for any specific reason, it is
   just a reasonable starting point.  */

static int max_value_size = 65536; /* 64k bytes */

/* It is critical that the MAX_VALUE_SIZE is at least as big as the size of
   LONGEST, otherwise GDB will not be able to parse integer values from the
   CLI; for example if the MAX_VALUE_SIZE could be set to 1 then GDB would
   be unable to parse "set max-value-size 2".

   As we want a consistent GDB experience across hosts with different sizes
   of LONGEST, this arbitrary minimum value was selected, so long as this
   is bigger than LONGEST on all GDB supported hosts we're fine.  */

#define MIN_VALUE_FOR_MAX_VALUE_SIZE 16
static_assert (sizeof (LONGEST) <= MIN_VALUE_FOR_MAX_VALUE_SIZE);

/* Implement the "set max-value-size" command.  */

static void
set_max_value_size (const char *args, int from_tty,
		    struct cmd_list_element *c)
{
  gdb_assert (max_value_size == -1 || max_value_size >= 0);

  if (max_value_size > -1 && max_value_size < MIN_VALUE_FOR_MAX_VALUE_SIZE)
    {
      max_value_size = MIN_VALUE_FOR_MAX_VALUE_SIZE;
      error (_("max-value-size set too low, increasing to %d bytes"),
	     max_value_size);
    }
}

/* Implement the "show max-value-size" command.  */

static void
show_max_value_size (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  if (max_value_size == -1)
    gdb_printf (file, _("Maximum value size is unlimited.\n"));
  else
    gdb_printf (file, _("Maximum value size is %d bytes.\n"),
		max_value_size);
}

/* Called before we attempt to allocate or reallocate a buffer for the
   contents of a value.  TYPE is the type of the value for which we are
   allocating the buffer.  If the buffer is too large (based on the user
   controllable setting) then throw an error.  If this function returns
   then we should attempt to allocate the buffer.  */

static void
check_type_length_before_alloc (const struct type *type)
{
  ULONGEST length = type->length ();

  if (exceeds_max_value_size (length))
    {
      if (type->name () != NULL)
	error (_("value of type `%s' requires %s bytes, which is more "
		 "than max-value-size"), type->name (), pulongest (length));
      else
	error (_("value requires %s bytes, which is more than "
		 "max-value-size"), pulongest (length));
    }
}

/* See value.h.  */

bool
exceeds_max_value_size (ULONGEST length)
{
  return max_value_size > -1 && length > max_value_size;
}

/* When this has a value, it is used to limit the number of array elements
   of an array that are loaded into memory when an array value is made
   non-lazy.  */
static std::optional<int> array_length_limiting_element_count;

/* See value.h.  */
scoped_array_length_limiting::scoped_array_length_limiting (int elements)
{
  m_old_value = array_length_limiting_element_count;
  array_length_limiting_element_count.emplace (elements);
}

/* See value.h.  */
scoped_array_length_limiting::~scoped_array_length_limiting ()
{
  array_length_limiting_element_count = m_old_value;
}

/* Find the inner element type for ARRAY_TYPE.  */

static struct type *
find_array_element_type (struct type *array_type)
{
  array_type = check_typedef (array_type);
  gdb_assert (array_type->code () == TYPE_CODE_ARRAY);

  if (current_language->la_language == language_fortran)
    while (array_type->code () == TYPE_CODE_ARRAY)
      {
	array_type = array_type->target_type ();
	array_type = check_typedef (array_type);
      }
  else
    {
      array_type = array_type->target_type ();
      array_type = check_typedef (array_type);
    }

  return array_type;
}

/* Return the limited length of ARRAY_TYPE, which must be of
   TYPE_CODE_ARRAY.  This function can only be called when the global
   ARRAY_LENGTH_LIMITING_ELEMENT_COUNT has a value.

   The limited length of an array is the smallest of either (1) the total
   size of the array type, or (2) the array target type multiplies by the
   array_length_limiting_element_count.  */

static ULONGEST
calculate_limited_array_length (struct type *array_type)
{
  gdb_assert (array_length_limiting_element_count.has_value ());

  array_type = check_typedef (array_type);
  gdb_assert (array_type->code () == TYPE_CODE_ARRAY);

  struct type *elm_type = find_array_element_type (array_type);
  ULONGEST len = (elm_type->length ()
		  * (*array_length_limiting_element_count));
  len = std::min (len, array_type->length ());

  return len;
}

/* See value.h.  */

bool
value::set_limited_array_length ()
{
  ULONGEST limit = m_limited_length;
  ULONGEST len = type ()->length ();

  if (array_length_limiting_element_count.has_value ())
    len = calculate_limited_array_length (type ());

  if (limit != 0 && len > limit)
    len = limit;
  if (len > max_value_size)
    return false;

  m_limited_length = max_value_size;
  return true;
}

/* See value.h.  */

void
value::allocate_contents (bool check_size)
{
  if (!m_contents)
    {
      struct type *enc_type = enclosing_type ();
      ULONGEST len = enc_type->length ();

      if (check_size)
	{
	  /* If we are allocating the contents of an array, which
	     is greater in size than max_value_size, and there is
	     an element limit in effect, then we can possibly try
	     to load only a sub-set of the array contents into
	     GDB's memory.  */
	  if (type () == enc_type
	      && type ()->code () == TYPE_CODE_ARRAY
	      && len > max_value_size
	      && set_limited_array_length ())
	    len = m_limited_length;
	  else
	    check_type_length_before_alloc (enc_type);
	}

      m_contents.reset ((gdb_byte *) xzalloc (len));
    }
}

/* Allocate a value and its contents for type TYPE.  If CHECK_SIZE is true,
   then apply the usual max-value-size checks.  */

struct value *
value::allocate (struct type *type, bool check_size)
{
  struct value *val = value::allocate_lazy (type);

  val->allocate_contents (check_size);
  val->m_lazy = false;
  return val;
}

/* Allocate a value and its contents for type TYPE.  */

struct value *
value::allocate (struct type *type)
{
  return allocate (type, true);
}

/* See value.h  */

value *
value::allocate_register_lazy (frame_info_ptr next_frame, int regnum,
			       struct type *type)
{
  if (type == nullptr)
    type = register_type (frame_unwind_arch (next_frame), regnum);

  value *result = value::allocate_lazy (type);

  result->set_lval (lval_register);
  result->m_location.reg.regnum = regnum;

  /* If this register value is created during unwind (while computing a frame
     id), and NEXT_FRAME is a frame inlined in the frame being unwound, then
     NEXT_FRAME will not have a valid frame id yet.  Find the next non-inline
     frame (possibly the sentinel frame).  This is where registers are unwound
     from anyway.  */
  while (get_frame_type (next_frame) == INLINE_FRAME)
    next_frame = get_next_frame_sentinel_okay (next_frame);

  result->m_location.reg.next_frame_id = get_frame_id (next_frame);

  /* We should have a next frame with a valid id.  */
  gdb_assert (frame_id_p (result->m_location.reg.next_frame_id));

  return result;
}

/* See value.h  */

value *
value::allocate_register (frame_info_ptr next_frame, int regnum,
			  struct type *type)
{
  value *result = value::allocate_register_lazy (next_frame, regnum, type);
  result->set_lazy (false);
  return result;
}

/* Allocate a  value  that has the correct length
   for COUNT repetitions of type TYPE.  */

struct value *
allocate_repeat_value (struct type *type, int count)
{
  /* Despite the fact that we are really creating an array of TYPE here, we
     use the string lower bound as the array lower bound.  This seems to
     work fine for now.  */
  int low_bound = current_language->string_lower_bound ();
  /* FIXME-type-allocation: need a way to free this type when we are
     done with it.  */
  struct type *array_type
    = lookup_array_range_type (type, low_bound, count + low_bound - 1);

  return value::allocate (array_type);
}

struct value *
value::allocate_computed (struct type *type,
			  const struct lval_funcs *funcs,
			  void *closure)
{
  struct value *v = value::allocate_lazy (type);

  v->set_lval (lval_computed);
  v->m_location.computed.funcs = funcs;
  v->m_location.computed.closure = closure;

  return v;
}

/* See value.h.  */

struct value *
value::allocate_optimized_out (struct type *type)
{
  struct value *retval = value::allocate_lazy (type);

  retval->mark_bytes_optimized_out (0, type->length ());
  retval->set_lazy (false);
  return retval;
}

/* Accessor methods.  */

gdb::array_view<gdb_byte>
value::contents_raw ()
{
  int unit_size = gdbarch_addressable_memory_unit_size (arch ());

  allocate_contents (true);

  ULONGEST length = type ()->length ();
  return gdb::make_array_view
    (m_contents.get () + m_embedded_offset * unit_size, length);
}

gdb::array_view<gdb_byte>
value::contents_all_raw ()
{
  allocate_contents (true);

  ULONGEST length = enclosing_type ()->length ();
  return gdb::make_array_view (m_contents.get (), length);
}

/* Look at value.h for description.  */

struct type *
value_actual_type (struct value *value, int resolve_simple_types,
		   int *real_type_found)
{
  struct value_print_options opts;
  struct type *result;

  get_user_print_options (&opts);

  if (real_type_found)
    *real_type_found = 0;
  result = value->type ();
  if (opts.objectprint)
    {
      /* If result's target type is TYPE_CODE_STRUCT, proceed to
	 fetch its rtti type.  */
      if (result->is_pointer_or_reference ()
	  && (check_typedef (result->target_type ())->code ()
	      == TYPE_CODE_STRUCT)
	  && !value->optimized_out ())
	{
	  struct type *real_type;

	  real_type = value_rtti_indirect_type (value, NULL, NULL, NULL);
	  if (real_type)
	    {
	      if (real_type_found)
		*real_type_found = 1;
	      result = real_type;
	    }
	}
      else if (resolve_simple_types)
	{
	  if (real_type_found)
	    *real_type_found = 1;
	  result = value->enclosing_type ();
	}
    }

  return result;
}

void
error_value_optimized_out (void)
{
  throw_error (OPTIMIZED_OUT_ERROR, _("value has been optimized out"));
}

void
value::require_not_optimized_out () const
{
  if (!m_optimized_out.empty ())
    {
      if (m_lval == lval_register)
	throw_error (OPTIMIZED_OUT_ERROR,
		     _("register has not been saved in frame"));
      else
	error_value_optimized_out ();
    }
}

void
value::require_available () const
{
  if (!m_unavailable.empty ())
    throw_error (NOT_AVAILABLE_ERROR, _("value is not available"));
}

gdb::array_view<const gdb_byte>
value::contents_for_printing ()
{
  if (m_lazy)
    fetch_lazy ();

  ULONGEST length = enclosing_type ()->length ();
  return gdb::make_array_view (m_contents.get (), length);
}

gdb::array_view<const gdb_byte>
value::contents_for_printing () const
{
  gdb_assert (!m_lazy);

  ULONGEST length = enclosing_type ()->length ();
  return gdb::make_array_view (m_contents.get (), length);
}

gdb::array_view<const gdb_byte>
value::contents_all ()
{
  gdb::array_view<const gdb_byte> result = contents_for_printing ();
  require_not_optimized_out ();
  require_available ();
  return result;
}

/* Copy ranges in SRC_RANGE that overlap [SRC_BIT_OFFSET,
   SRC_BIT_OFFSET+BIT_LENGTH) ranges into *DST_RANGE, adjusted.  */

static void
ranges_copy_adjusted (std::vector<range> *dst_range, int dst_bit_offset,
		      const std::vector<range> &src_range, int src_bit_offset,
		      unsigned int bit_length)
{
  for (const range &r : src_range)
    {
      LONGEST h, l;

      l = std::max (r.offset, (LONGEST) src_bit_offset);
      h = std::min ((LONGEST) (r.offset + r.length),
		    (LONGEST) src_bit_offset + bit_length);

      if (l < h)
	insert_into_bit_range_vector (dst_range,
				      dst_bit_offset + (l - src_bit_offset),
				      h - l);
    }
}

/* See value.h.  */

void
value::ranges_copy_adjusted (struct value *dst, int dst_bit_offset,
			     int src_bit_offset, int bit_length) const
{
  ::ranges_copy_adjusted (&dst->m_unavailable, dst_bit_offset,
			  m_unavailable, src_bit_offset,
			  bit_length);
  ::ranges_copy_adjusted (&dst->m_optimized_out, dst_bit_offset,
			  m_optimized_out, src_bit_offset,
			  bit_length);
}

/* See value.h.  */

void
value::contents_copy_raw (struct value *dst, LONGEST dst_offset,
			  LONGEST src_offset, LONGEST length)
{
  LONGEST src_bit_offset, dst_bit_offset, bit_length;
  int unit_size = gdbarch_addressable_memory_unit_size (arch ());

  /* A lazy DST would make that this copy operation useless, since as
     soon as DST's contents were un-lazied (by a later value_contents
     call, say), the contents would be overwritten.  A lazy SRC would
     mean we'd be copying garbage.  */
  gdb_assert (!dst->m_lazy && !m_lazy);

  ULONGEST copy_length = length;
  ULONGEST limit = m_limited_length;
  if (limit > 0 && src_offset + length > limit)
    copy_length = src_offset > limit ? 0 : limit - src_offset;

  /* The overwritten DST range gets unavailability ORed in, not
     replaced.  Make sure to remember to implement replacing if it
     turns out actually necessary.  */
  gdb_assert (dst->bytes_available (dst_offset, length));
  gdb_assert (!dst->bits_any_optimized_out (TARGET_CHAR_BIT * dst_offset,
					    TARGET_CHAR_BIT * length));

  /* Copy the data.  */
  gdb::array_view<gdb_byte> dst_contents
    = dst->contents_all_raw ().slice (dst_offset * unit_size,
				      copy_length * unit_size);
  gdb::array_view<const gdb_byte> src_contents
    = contents_all_raw ().slice (src_offset * unit_size,
				 copy_length * unit_size);
  gdb::copy (src_contents, dst_contents);

  /* Copy the meta-data, adjusted.  */
  src_bit_offset = src_offset * unit_size * HOST_CHAR_BIT;
  dst_bit_offset = dst_offset * unit_size * HOST_CHAR_BIT;
  bit_length = length * unit_size * HOST_CHAR_BIT;

  ranges_copy_adjusted (dst, dst_bit_offset,
			src_bit_offset, bit_length);
}

/* See value.h.  */

void
value::contents_copy_raw_bitwise (struct value *dst, LONGEST dst_bit_offset,
				  LONGEST src_bit_offset,
				  LONGEST bit_length)
{
  /* A lazy DST would make that this copy operation useless, since as
     soon as DST's contents were un-lazied (by a later value_contents
     call, say), the contents would be overwritten.  A lazy SRC would
     mean we'd be copying garbage.  */
  gdb_assert (!dst->m_lazy && !m_lazy);

  ULONGEST copy_bit_length = bit_length;
  ULONGEST bit_limit = m_limited_length * TARGET_CHAR_BIT;
  if (bit_limit > 0 && src_bit_offset + bit_length > bit_limit)
    copy_bit_length = (src_bit_offset > bit_limit ? 0
		       : bit_limit - src_bit_offset);

  /* The overwritten DST range gets unavailability ORed in, not
     replaced.  Make sure to remember to implement replacing if it
     turns out actually necessary.  */
  LONGEST dst_offset = dst_bit_offset / TARGET_CHAR_BIT;
  LONGEST length = bit_length / TARGET_CHAR_BIT;
  gdb_assert (dst->bytes_available (dst_offset, length));
  gdb_assert (!dst->bits_any_optimized_out (dst_bit_offset,
					    bit_length));

  /* Copy the data.  */
  gdb::array_view<gdb_byte> dst_contents = dst->contents_all_raw ();
  gdb::array_view<const gdb_byte> src_contents = contents_all_raw ();
  copy_bitwise (dst_contents.data (), dst_bit_offset,
		src_contents.data (), src_bit_offset,
		copy_bit_length,
		type_byte_order (type ()) == BFD_ENDIAN_BIG);

  /* Copy the meta-data.  */
  ranges_copy_adjusted (dst, dst_bit_offset, src_bit_offset, bit_length);
}

/* See value.h.  */

void
value::contents_copy (struct value *dst, LONGEST dst_offset,
		      LONGEST src_offset, LONGEST length)
{
  if (m_lazy)
    fetch_lazy ();

  contents_copy_raw (dst, dst_offset, src_offset, length);
}

gdb::array_view<const gdb_byte>
value::contents ()
{
  gdb::array_view<const gdb_byte> result = contents_writeable ();
  require_not_optimized_out ();
  require_available ();
  return result;
}

gdb::array_view<gdb_byte>
value::contents_writeable ()
{
  if (m_lazy)
    fetch_lazy ();
  return contents_raw ();
}

bool
value::optimized_out ()
{
  if (m_lazy)
    {
      /* See if we can compute the result without fetching the
	 value.  */
      if (this->lval () == lval_memory)
	return false;
      else if (this->lval () == lval_computed)
	{
	  const struct lval_funcs *funcs = m_location.computed.funcs;

	  if (funcs->is_optimized_out != nullptr)
	    return funcs->is_optimized_out (this);
	}

      /* Fall back to fetching.  */
      try
	{
	  fetch_lazy ();
	}
      catch (const gdb_exception_error &ex)
	{
	  switch (ex.error)
	    {
	    case MEMORY_ERROR:
	    case OPTIMIZED_OUT_ERROR:
	    case NOT_AVAILABLE_ERROR:
	      /* These can normally happen when we try to access an
		 optimized out or unavailable register, either in a
		 physical register or spilled to memory.  */
	      break;
	    default:
	      throw;
	    }
	}
    }

  return !m_optimized_out.empty ();
}

/* Mark contents of VALUE as optimized out, starting at OFFSET bytes, and
   the following LENGTH bytes.  */

void
value::mark_bytes_optimized_out (int offset, int length)
{
  mark_bits_optimized_out (offset * TARGET_CHAR_BIT,
			   length * TARGET_CHAR_BIT);
}

/* See value.h.  */

void
value::mark_bits_optimized_out (LONGEST offset, LONGEST length)
{
  insert_into_bit_range_vector (&m_optimized_out, offset, length);
}

bool
value::bits_synthetic_pointer (LONGEST offset, LONGEST length) const
{
  if (m_lval != lval_computed
      || !m_location.computed.funcs->check_synthetic_pointer)
    return false;
  return m_location.computed.funcs->check_synthetic_pointer (this, offset,
							     length);
}

const struct lval_funcs *
value::computed_funcs () const
{
  gdb_assert (m_lval == lval_computed);

  return m_location.computed.funcs;
}

void *
value::computed_closure () const
{
  gdb_assert (m_lval == lval_computed);

  return m_location.computed.closure;
}

CORE_ADDR
value::address () const
{
  if (m_lval != lval_memory)
    return 0;
  if (m_parent != NULL)
    return m_parent->address () + m_offset;
  if (NULL != TYPE_DATA_LOCATION (type ()))
    {
      gdb_assert (TYPE_DATA_LOCATION (type ())->is_constant ());
      return TYPE_DATA_LOCATION_ADDR (type ());
    }

  return m_location.address + m_offset;
}

CORE_ADDR
value::raw_address () const
{
  if (m_lval != lval_memory)
    return 0;
  return m_location.address;
}

void
value::set_address (CORE_ADDR addr)
{
  gdb_assert (m_lval == lval_memory);
  m_location.address = addr;
}

/* Return a mark in the value chain.  All values allocated after the
   mark is obtained (except for those released) are subject to being freed
   if a subsequent value_free_to_mark is passed the mark.  */
struct value *
value_mark (void)
{
  if (all_values.empty ())
    return nullptr;
  return all_values.back ().get ();
}

/* Release a reference to VAL, which was acquired with value_incref.
   This function is also called to deallocate values from the value
   chain.  */

void
value::decref ()
{
  gdb_assert (m_reference_count > 0);
  m_reference_count--;
  if (m_reference_count == 0)
    delete this;
}

/* Free all values allocated since MARK was obtained by value_mark
   (except for those released).  */
void
value_free_to_mark (const struct value *mark)
{
  auto iter = std::find (all_values.begin (), all_values.end (), mark);
  if (iter == all_values.end ())
    all_values.clear ();
  else
    all_values.erase (iter + 1, all_values.end ());
}

/* Remove VAL from the chain all_values
   so it will not be freed automatically.  */

value_ref_ptr
release_value (struct value *val)
{
  if (val == nullptr)
    return value_ref_ptr ();

  std::vector<value_ref_ptr>::reverse_iterator iter;
  for (iter = all_values.rbegin (); iter != all_values.rend (); ++iter)
    {
      if (*iter == val)
	{
	  value_ref_ptr result = *iter;
	  all_values.erase (iter.base () - 1);
	  return result;
	}
    }

  /* We must always return an owned reference.  Normally this happens
     because we transfer the reference from the value chain, but in
     this case the value was not on the chain.  */
  return value_ref_ptr::new_reference (val);
}

/* See value.h.  */

std::vector<value_ref_ptr>
value_release_to_mark (const struct value *mark)
{
  std::vector<value_ref_ptr> result;

  auto iter = std::find (all_values.begin (), all_values.end (), mark);
  if (iter == all_values.end ())
    std::swap (result, all_values);
  else
    {
      std::move (iter + 1, all_values.end (), std::back_inserter (result));
      all_values.erase (iter + 1, all_values.end ());
    }
  std::reverse (result.begin (), result.end ());
  return result;
}

/* See value.h.  */

struct value *
value::copy () const
{
  struct type *encl_type = enclosing_type ();
  struct value *val;

  val = value::allocate_lazy (encl_type);
  val->m_type = m_type;
  val->set_lval (m_lval);
  val->m_location = m_location;
  val->m_offset = m_offset;
  val->m_bitpos = m_bitpos;
  val->m_bitsize = m_bitsize;
  val->m_lazy = m_lazy;
  val->m_embedded_offset = embedded_offset ();
  val->m_pointed_to_offset = m_pointed_to_offset;
  val->m_modifiable = m_modifiable;
  val->m_stack = m_stack;
  val->m_is_zero = m_is_zero;
  val->m_in_history = m_in_history;
  val->m_initialized = m_initialized;
  val->m_unavailable = m_unavailable;
  val->m_optimized_out = m_optimized_out;
  val->m_parent = m_parent;
  val->m_limited_length = m_limited_length;

  if (!val->lazy ()
      && !(val->entirely_optimized_out ()
	   || val->entirely_unavailable ()))
    {
      ULONGEST length = val->m_limited_length;
      if (length == 0)
	length = val->enclosing_type ()->length ();

      gdb_assert (m_contents != nullptr);
      const auto &arg_view
	= gdb::make_array_view (m_contents.get (), length);

      val->allocate_contents (false);
      gdb::array_view<gdb_byte> val_contents
	= val->contents_all_raw ().slice (0, length);

      gdb::copy (arg_view, val_contents);
    }

  if (val->lval () == lval_computed)
    {
      const struct lval_funcs *funcs = val->m_location.computed.funcs;

      if (funcs->copy_closure)
	val->m_location.computed.closure = funcs->copy_closure (val);
    }
  return val;
}

/* Return a "const" and/or "volatile" qualified version of the value V.
   If CNST is true, then the returned value will be qualified with
   "const".
   if VOLTL is true, then the returned value will be qualified with
   "volatile".  */

struct value *
make_cv_value (int cnst, int voltl, struct value *v)
{
  struct type *val_type = v->type ();
  struct type *m_enclosing_type = v->enclosing_type ();
  struct value *cv_val = v->copy ();

  cv_val->deprecated_set_type (make_cv_type (cnst, voltl, val_type, NULL));
  cv_val->set_enclosing_type (make_cv_type (cnst, voltl, m_enclosing_type, NULL));

  return cv_val;
}

/* See value.h.  */

struct value *
value::non_lval ()
{
  if (this->lval () != not_lval)
    {
      struct type *enc_type = enclosing_type ();
      struct value *val = value::allocate (enc_type);

      gdb::copy (contents_all (), val->contents_all_raw ());
      val->m_type = m_type;
      val->set_embedded_offset (embedded_offset ());
      val->set_pointed_to_offset (pointed_to_offset ());
      return val;
    }
  return this;
}

/* See value.h.  */

void
value::force_lval (CORE_ADDR addr)
{
  gdb_assert (this->lval () == not_lval);

  write_memory (addr, contents_raw ().data (), type ()->length ());
  m_lval = lval_memory;
  m_location.address = addr;
}

void
value::set_component_location (const struct value *whole)
{
  struct type *type;

  gdb_assert (whole->m_lval != lval_xcallable);

  if (whole->m_lval == lval_internalvar)
    m_lval = lval_internalvar_component;
  else
    m_lval = whole->m_lval;

  m_location = whole->m_location;
  if (whole->m_lval == lval_computed)
    {
      const struct lval_funcs *funcs = whole->m_location.computed.funcs;

      if (funcs->copy_closure)
	m_location.computed.closure = funcs->copy_closure (whole);
    }

  /* If the WHOLE value has a dynamically resolved location property then
     update the address of the COMPONENT.  */
  type = whole->type ();
  if (NULL != TYPE_DATA_LOCATION (type)
      && TYPE_DATA_LOCATION (type)->is_constant ())
    set_address (TYPE_DATA_LOCATION_ADDR (type));

  /* Similarly, if the COMPONENT value has a dynamically resolved location
     property then update its address.  */
  type = this->type ();
  if (NULL != TYPE_DATA_LOCATION (type)
      && TYPE_DATA_LOCATION (type)->is_constant ())
    {
      /* If the COMPONENT has a dynamic location, and is an
	 lval_internalvar_component, then we change it to a lval_memory.

	 Usually a component of an internalvar is created non-lazy, and has
	 its content immediately copied from the parent internalvar.
	 However, for components with a dynamic location, the content of
	 the component is not contained within the parent, but is instead
	 accessed indirectly.  Further, the component will be created as a
	 lazy value.

	 By changing the type of the component to lval_memory we ensure
	 that value_fetch_lazy can successfully load the component.

	 This solution isn't ideal, but a real fix would require values to
	 carry around both the parent value contents, and the contents of
	 any dynamic fields within the parent.  This is a substantial
	 change to how values work in GDB.  */
      if (this->lval () == lval_internalvar_component)
	{
	  gdb_assert (lazy ());
	  m_lval = lval_memory;
	}
      else
	gdb_assert (this->lval () == lval_memory);
      set_address (TYPE_DATA_LOCATION_ADDR (type));
    }
}

/* Access to the value history.  */

/* Record a new value in the value history.
   Returns the absolute history index of the entry.  */

int
value::record_latest ()
{
  /* We don't want this value to have anything to do with the inferior anymore.
     In particular, "set $1 = 50" should not affect the variable from which
     the value was taken, and fast watchpoints should be able to assume that
     a value on the value history never changes.  */
  if (lazy ())
    {
      /* We know that this is a _huge_ array, any attempt to fetch this
	 is going to cause GDB to throw an error.  However, to allow
	 the array to still be displayed we fetch its contents up to
	 `max_value_size' and mark anything beyond "unavailable" in
	 the history.  */
      if (m_type->code () == TYPE_CODE_ARRAY
	  && m_type->length () > max_value_size
	  && array_length_limiting_element_count.has_value ()
	  && m_enclosing_type == m_type
	  && calculate_limited_array_length (m_type) <= max_value_size)
	m_limited_length = max_value_size;

      fetch_lazy ();
    }

  ULONGEST limit = m_limited_length;
  if (limit != 0)
    mark_bytes_unavailable (limit, m_enclosing_type->length () - limit);

  /* Mark the value as recorded in the history for the availability check.  */
  m_in_history = true;

  /* We preserve VALUE_LVAL so that the user can find out where it was fetched
     from.  This is a bit dubious, because then *&$1 does not just return $1
     but the current contents of that location.  c'est la vie...  */
  set_modifiable (false);

  value_history.push_back (release_value (this));

  return value_history.size ();
}

/* Return a copy of the value in the history with sequence number NUM.  */

struct value *
access_value_history (int num)
{
  int absnum = num;

  if (absnum <= 0)
    absnum += value_history.size ();

  if (absnum <= 0)
    {
      if (num == 0)
	error (_("The history is empty."));
      else if (num == 1)
	error (_("There is only one value in the history."));
      else
	error (_("History does not go back to $$%d."), -num);
    }
  if (absnum > value_history.size ())
    error (_("History has not yet reached $%d."), absnum);

  absnum--;

  return value_history[absnum]->copy ();
}

/* See value.h.  */

ULONGEST
value_history_count ()
{
  return value_history.size ();
}

static void
show_values (const char *num_exp, int from_tty)
{
  int i;
  struct value *val;
  static int num = 1;

  if (num_exp)
    {
      /* "show values +" should print from the stored position.
	 "show values <exp>" should print around value number <exp>.  */
      if (num_exp[0] != '+' || num_exp[1] != '\0')
	num = parse_and_eval_long (num_exp) - 5;
    }
  else
    {
      /* "show values" means print the last 10 values.  */
      num = value_history.size () - 9;
    }

  if (num <= 0)
    num = 1;

  for (i = num; i < num + 10 && i <= value_history.size (); i++)
    {
      struct value_print_options opts;

      val = access_value_history (i);
      gdb_printf (("$%d = "), i);
      get_user_print_options (&opts);
      value_print (val, gdb_stdout, &opts);
      gdb_printf (("\n"));
    }

  /* The next "show values +" should start after what we just printed.  */
  num += 10;

  /* Hitting just return after this command should do the same thing as
     "show values +".  If num_exp is null, this is unnecessary, since
     "show values +" is not useful after "show values".  */
  if (from_tty && num_exp)
    set_repeat_arguments ("+");
}

enum internalvar_kind
{
  /* The internal variable is empty.  */
  INTERNALVAR_VOID,

  /* The value of the internal variable is provided directly as
     a GDB value object.  */
  INTERNALVAR_VALUE,

  /* A fresh value is computed via a call-back routine on every
     access to the internal variable.  */
  INTERNALVAR_MAKE_VALUE,

  /* The internal variable holds a GDB internal convenience function.  */
  INTERNALVAR_FUNCTION,

  /* The variable holds an integer value.  */
  INTERNALVAR_INTEGER,

  /* The variable holds a GDB-provided string.  */
  INTERNALVAR_STRING,
};

union internalvar_data
{
  /* A value object used with INTERNALVAR_VALUE.  */
  struct value *value;

  /* The call-back routine used with INTERNALVAR_MAKE_VALUE.  */
  struct
  {
    /* The functions to call.  */
    const struct internalvar_funcs *functions;

    /* The function's user-data.  */
    void *data;
  } make_value;

  /* The internal function used with INTERNALVAR_FUNCTION.  */
  struct
  {
    struct internal_function *function;
    /* True if this is the canonical name for the function.  */
    int canonical;
  } fn;

  /* An integer value used with INTERNALVAR_INTEGER.  */
  struct
  {
    /* If type is non-NULL, it will be used as the type to generate
       a value for this internal variable.  If type is NULL, a default
       integer type for the architecture is used.  */
    struct type *type;
    LONGEST val;
  } integer;

  /* A string value used with INTERNALVAR_STRING.  */
  char *string;
};

/* Internal variables.  These are variables within the debugger
   that hold values assigned by debugger commands.
   The user refers to them with a '$' prefix
   that does not appear in the variable names stored internally.  */

struct internalvar
{
  internalvar (std::string name)
    : name (std::move (name))
  {}

  std::string name;

  /* We support various different kinds of content of an internal variable.
     enum internalvar_kind specifies the kind, and union internalvar_data
     provides the data associated with this particular kind.  */

  enum internalvar_kind kind = INTERNALVAR_VOID;

  union internalvar_data u {};
};

/* Use std::map, a sorted container, to make the order of iteration (and
   therefore the output of "show convenience") stable.  */

static std::map<std::string, internalvar> internalvars;

/* If the variable does not already exist create it and give it the
   value given.  If no value is given then the default is zero.  */
static void
init_if_undefined_command (const char* args, int from_tty)
{
  struct internalvar *intvar = nullptr;

  /* Parse the expression - this is taken from set_command().  */
  expression_up expr = parse_expression (args);

  /* Validate the expression.
     Was the expression an assignment?
     Or even an expression at all?  */
  if (expr->first_opcode () != BINOP_ASSIGN)
    error (_("Init-if-undefined requires an assignment expression."));

  /* Extract the variable from the parsed expression.  */
  expr::assign_operation *assign
    = dynamic_cast<expr::assign_operation *> (expr->op.get ());
  if (assign != nullptr)
    {
      expr::operation *lhs = assign->get_lhs ();
      expr::internalvar_operation *ivarop
	= dynamic_cast<expr::internalvar_operation *> (lhs);
      if (ivarop != nullptr)
	intvar = ivarop->get_internalvar ();
    }

  if (intvar == nullptr)
    error (_("The first parameter to init-if-undefined "
	     "should be a GDB variable."));

  /* Only evaluate the expression if the lvalue is void.
     This may still fail if the expression is invalid.  */
  if (intvar->kind == INTERNALVAR_VOID)
    expr->evaluate ();
}


/* Look up an internal variable with name NAME.  NAME should not
   normally include a dollar sign.

   If the specified internal variable does not exist,
   the return value is NULL.  */

struct internalvar *
lookup_only_internalvar (const char *name)
{
  auto it = internalvars.find (name);
  if (it == internalvars.end ())
    return nullptr;

  return &it->second;
}

/* Complete NAME by comparing it to the names of internal
   variables.  */

void
complete_internalvar (completion_tracker &tracker, const char *name)
{
  int len = strlen (name);

  for (auto &pair : internalvars)
    {
      const internalvar &var = pair.second;

      if (var.name.compare (0, len, name) == 0)
	tracker.add_completion (make_unique_xstrdup (var.name.c_str ()));
    }
}

/* Create an internal variable with name NAME and with a void value.
   NAME should not normally include a dollar sign.

   An internal variable with that name must not exist already.  */

struct internalvar *
create_internalvar (const char *name)
{
  auto pair = internalvars.emplace (std::make_pair (name, internalvar (name)));
  gdb_assert (pair.second);

  return &pair.first->second;
}

/* Create an internal variable with name NAME and register FUN as the
   function that value_of_internalvar uses to create a value whenever
   this variable is referenced.  NAME should not normally include a
   dollar sign.  DATA is passed uninterpreted to FUN when it is
   called.  CLEANUP, if not NULL, is called when the internal variable
   is destroyed.  It is passed DATA as its only argument.  */

struct internalvar *
create_internalvar_type_lazy (const char *name,
			      const struct internalvar_funcs *funcs,
			      void *data)
{
  struct internalvar *var = create_internalvar (name);

  var->kind = INTERNALVAR_MAKE_VALUE;
  var->u.make_value.functions = funcs;
  var->u.make_value.data = data;
  return var;
}

/* See documentation in value.h.  */

int
compile_internalvar_to_ax (struct internalvar *var,
			   struct agent_expr *expr,
			   struct axs_value *value)
{
  if (var->kind != INTERNALVAR_MAKE_VALUE
      || var->u.make_value.functions->compile_to_ax == NULL)
    return 0;

  var->u.make_value.functions->compile_to_ax (var, expr, value,
					      var->u.make_value.data);
  return 1;
}

/* Look up an internal variable with name NAME.  NAME should not
   normally include a dollar sign.

   If the specified internal variable does not exist,
   one is created, with a void value.  */

struct internalvar *
lookup_internalvar (const char *name)
{
  struct internalvar *var;

  var = lookup_only_internalvar (name);
  if (var)
    return var;

  return create_internalvar (name);
}

/* Return current value of internal variable VAR.  For variables that
   are not inherently typed, use a value type appropriate for GDBARCH.  */

struct value *
value_of_internalvar (struct gdbarch *gdbarch, struct internalvar *var)
{
  struct value *val;
  struct trace_state_variable *tsv;

  /* If there is a trace state variable of the same name, assume that
     is what we really want to see.  */
  tsv = find_trace_state_variable (var->name.c_str ());
  if (tsv)
    {
      tsv->value_known = target_get_trace_state_variable_value (tsv->number,
								&(tsv->value));
      if (tsv->value_known)
	val = value_from_longest (builtin_type (gdbarch)->builtin_int64,
				  tsv->value);
      else
	val = value::allocate (builtin_type (gdbarch)->builtin_void);
      return val;
    }

  switch (var->kind)
    {
    case INTERNALVAR_VOID:
      val = value::allocate (builtin_type (gdbarch)->builtin_void);
      break;

    case INTERNALVAR_FUNCTION:
      val = value::allocate (builtin_type (gdbarch)->internal_fn);
      break;

    case INTERNALVAR_INTEGER:
      if (!var->u.integer.type)
	val = value_from_longest (builtin_type (gdbarch)->builtin_int,
				  var->u.integer.val);
      else
	val = value_from_longest (var->u.integer.type, var->u.integer.val);
      break;

    case INTERNALVAR_STRING:
      val = current_language->value_string (gdbarch,
					    var->u.string,
					    strlen (var->u.string));
      break;

    case INTERNALVAR_VALUE:
      val = var->u.value->copy ();
      if (val->lazy ())
	val->fetch_lazy ();
      break;

    case INTERNALVAR_MAKE_VALUE:
      val = (*var->u.make_value.functions->make_value) (gdbarch, var,
							var->u.make_value.data);
      break;

    default:
      internal_error (_("bad kind"));
    }

  /* Change the VALUE_LVAL to lval_internalvar so that future operations
     on this value go back to affect the original internal variable.

     Do not do this for INTERNALVAR_MAKE_VALUE variables, as those have
     no underlying modifiable state in the internal variable.

     Likewise, if the variable's value is a computed lvalue, we want
     references to it to produce another computed lvalue, where
     references and assignments actually operate through the
     computed value's functions.

     This means that internal variables with computed values
     behave a little differently from other internal variables:
     assignments to them don't just replace the previous value
     altogether.  At the moment, this seems like the behavior we
     want.  */

  if (var->kind != INTERNALVAR_MAKE_VALUE
      && val->lval () != lval_computed)
    {
      val->set_lval (lval_internalvar);
      VALUE_INTERNALVAR (val) = var;
    }

  return val;
}

int
get_internalvar_integer (struct internalvar *var, LONGEST *result)
{
  if (var->kind == INTERNALVAR_INTEGER)
    {
      *result = var->u.integer.val;
      return 1;
    }

  if (var->kind == INTERNALVAR_VALUE)
    {
      struct type *type = check_typedef (var->u.value->type ());

      if (type->code () == TYPE_CODE_INT)
	{
	  *result = value_as_long (var->u.value);
	  return 1;
	}
    }

  if (var->kind == INTERNALVAR_MAKE_VALUE)
    {
      struct gdbarch *gdbarch = get_current_arch ();
      struct value *val
	= (*var->u.make_value.functions->make_value) (gdbarch, var,
						      var->u.make_value.data);
      struct type *type = check_typedef (val->type ());

      if (type->code () == TYPE_CODE_INT)
	{
	  *result = value_as_long (val);
	  return 1;
	}
    }

  return 0;
}

static int
get_internalvar_function (struct internalvar *var,
			  struct internal_function **result)
{
  switch (var->kind)
    {
    case INTERNALVAR_FUNCTION:
      *result = var->u.fn.function;
      return 1;

    default:
      return 0;
    }
}

void
set_internalvar_component (struct internalvar *var,
			   LONGEST offset, LONGEST bitpos,
			   LONGEST bitsize, struct value *newval)
{
  gdb_byte *addr;
  struct gdbarch *gdbarch;
  int unit_size;

  switch (var->kind)
    {
    case INTERNALVAR_VALUE:
      addr = var->u.value->contents_writeable ().data ();
      gdbarch = var->u.value->arch ();
      unit_size = gdbarch_addressable_memory_unit_size (gdbarch);

      if (bitsize)
	modify_field (var->u.value->type (), addr + offset,
		      value_as_long (newval), bitpos, bitsize);
      else
	memcpy (addr + offset * unit_size, newval->contents ().data (),
		newval->type ()->length ());
      break;

    default:
      /* We can never get a component of any other kind.  */
      internal_error (_("set_internalvar_component"));
    }
}

void
set_internalvar (struct internalvar *var, struct value *val)
{
  enum internalvar_kind new_kind;
  union internalvar_data new_data = { 0 };

  if (var->kind == INTERNALVAR_FUNCTION && var->u.fn.canonical)
    error (_("Cannot overwrite convenience function %s"), var->name.c_str ());

  /* Prepare new contents.  */
  switch (check_typedef (val->type ())->code ())
    {
    case TYPE_CODE_VOID:
      new_kind = INTERNALVAR_VOID;
      break;

    case TYPE_CODE_INTERNAL_FUNCTION:
      gdb_assert (val->lval () == lval_internalvar);
      new_kind = INTERNALVAR_FUNCTION;
      get_internalvar_function (VALUE_INTERNALVAR (val),
				&new_data.fn.function);
      /* Copies created here are never canonical.  */
      break;

    default:
      new_kind = INTERNALVAR_VALUE;
      struct value *copy = val->copy ();
      copy->set_modifiable (true);

      /* Force the value to be fetched from the target now, to avoid problems
	 later when this internalvar is referenced and the target is gone or
	 has changed.  */
      if (copy->lazy ())
	copy->fetch_lazy ();

      /* Release the value from the value chain to prevent it from being
	 deleted by free_all_values.  From here on this function should not
	 call error () until new_data is installed into the var->u to avoid
	 leaking memory.  */
      new_data.value = release_value (copy).release ();

      /* Internal variables which are created from values with a dynamic
	 location don't need the location property of the origin anymore.
	 The resolved dynamic location is used prior then any other address
	 when accessing the value.
	 If we keep it, we would still refer to the origin value.
	 Remove the location property in case it exist.  */
      new_data.value->type ()->remove_dyn_prop (DYN_PROP_DATA_LOCATION);

      break;
    }

  /* Clean up old contents.  */
  clear_internalvar (var);

  /* Switch over.  */
  var->kind = new_kind;
  var->u = new_data;
  /* End code which must not call error().  */
}

void
set_internalvar_integer (struct internalvar *var, LONGEST l)
{
  /* Clean up old contents.  */
  clear_internalvar (var);

  var->kind = INTERNALVAR_INTEGER;
  var->u.integer.type = NULL;
  var->u.integer.val = l;
}

void
set_internalvar_string (struct internalvar *var, const char *string)
{
  /* Clean up old contents.  */
  clear_internalvar (var);

  var->kind = INTERNALVAR_STRING;
  var->u.string = xstrdup (string);
}

static void
set_internalvar_function (struct internalvar *var, struct internal_function *f)
{
  /* Clean up old contents.  */
  clear_internalvar (var);

  var->kind = INTERNALVAR_FUNCTION;
  var->u.fn.function = f;
  var->u.fn.canonical = 1;
  /* Variables installed here are always the canonical version.  */
}

void
clear_internalvar (struct internalvar *var)
{
  /* Clean up old contents.  */
  switch (var->kind)
    {
    case INTERNALVAR_VALUE:
      var->u.value->decref ();
      break;

    case INTERNALVAR_STRING:
      xfree (var->u.string);
      break;

    default:
      break;
    }

  /* Reset to void kind.  */
  var->kind = INTERNALVAR_VOID;
}

const char *
internalvar_name (const struct internalvar *var)
{
  return var->name.c_str ();
}

static struct internal_function *
create_internal_function (const char *name,
			  internal_function_fn handler, void *cookie)
{
  struct internal_function *ifn = XNEW (struct internal_function);

  ifn->name = xstrdup (name);
  ifn->handler = handler;
  ifn->cookie = cookie;
  return ifn;
}

const char *
value_internal_function_name (struct value *val)
{
  struct internal_function *ifn;
  int result;

  gdb_assert (val->lval () == lval_internalvar);
  result = get_internalvar_function (VALUE_INTERNALVAR (val), &ifn);
  gdb_assert (result);

  return ifn->name;
}

struct value *
call_internal_function (struct gdbarch *gdbarch,
			const struct language_defn *language,
			struct value *func, int argc, struct value **argv)
{
  struct internal_function *ifn;
  int result;

  gdb_assert (func->lval () == lval_internalvar);
  result = get_internalvar_function (VALUE_INTERNALVAR (func), &ifn);
  gdb_assert (result);

  return (*ifn->handler) (gdbarch, language, ifn->cookie, argc, argv);
}

/* The 'function' command.  This does nothing -- it is just a
   placeholder to let "help function NAME" work.  This is also used as
   the implementation of the sub-command that is created when
   registering an internal function.  */
static void
function_command (const char *command, int from_tty)
{
  /* Do nothing.  */
}

/* Helper function that does the work for add_internal_function.  */

static struct cmd_list_element *
do_add_internal_function (const char *name, const char *doc,
			  internal_function_fn handler, void *cookie)
{
  struct internal_function *ifn;
  struct internalvar *var = lookup_internalvar (name);

  ifn = create_internal_function (name, handler, cookie);
  set_internalvar_function (var, ifn);

  return add_cmd (name, no_class, function_command, doc, &functionlist);
}

/* See value.h.  */

void
add_internal_function (const char *name, const char *doc,
		       internal_function_fn handler, void *cookie)
{
  do_add_internal_function (name, doc, handler, cookie);
}

/* See value.h.  */

void
add_internal_function (gdb::unique_xmalloc_ptr<char> &&name,
		       gdb::unique_xmalloc_ptr<char> &&doc,
		       internal_function_fn handler, void *cookie)
{
  struct cmd_list_element *cmd
    = do_add_internal_function (name.get (), doc.get (), handler, cookie);

  /* Manually transfer the ownership of the doc and name strings to CMD by
     setting the appropriate flags.  */
  (void) doc.release ();
  cmd->doc_allocated = 1;
  (void) name.release ();
  cmd->name_allocated = 1;
}

void
value::preserve (struct objfile *objfile, htab_t copied_types)
{
  if (m_type->objfile_owner () == objfile)
    m_type = copy_type_recursive (m_type, copied_types);

  if (m_enclosing_type->objfile_owner () == objfile)
    m_enclosing_type = copy_type_recursive (m_enclosing_type, copied_types);
}

/* Likewise for internal variable VAR.  */

static void
preserve_one_internalvar (struct internalvar *var, struct objfile *objfile,
			  htab_t copied_types)
{
  switch (var->kind)
    {
    case INTERNALVAR_INTEGER:
      if (var->u.integer.type
	  && var->u.integer.type->objfile_owner () == objfile)
	var->u.integer.type
	  = copy_type_recursive (var->u.integer.type, copied_types);
      break;

    case INTERNALVAR_VALUE:
      var->u.value->preserve (objfile, copied_types);
      break;
    }
}

/* Make sure that all types and values referenced by VAROBJ are updated before
   OBJFILE is discarded.  COPIED_TYPES is used to prevent cycles and
   duplicates.  */

static void
preserve_one_varobj (struct varobj *varobj, struct objfile *objfile,
		     htab_t copied_types)
{
  if (varobj->type->is_objfile_owned ()
      && varobj->type->objfile_owner () == objfile)
    {
      varobj->type
	= copy_type_recursive (varobj->type, copied_types);
    }

  if (varobj->value != nullptr)
    varobj->value->preserve (objfile, copied_types);
}

/* Update the internal variables and value history when OBJFILE is
   discarded; we must copy the types out of the objfile.  New global types
   will be created for every convenience variable which currently points to
   this objfile's types, and the convenience variables will be adjusted to
   use the new global types.  */

void
preserve_values (struct objfile *objfile)
{
  /* Create the hash table.  We allocate on the objfile's obstack, since
     it is soon to be deleted.  */
  htab_up copied_types = create_copied_types_hash ();

  for (const value_ref_ptr &item : value_history)
    item->preserve (objfile, copied_types.get ());

  for (auto &pair : internalvars)
    preserve_one_internalvar (&pair.second, objfile, copied_types.get ());

  /* For the remaining varobj, check that none has type owned by OBJFILE.  */
  all_root_varobjs ([&copied_types, objfile] (struct varobj *varobj)
    {
      preserve_one_varobj (varobj, objfile,
			   copied_types.get ());
    });

  preserve_ext_lang_values (objfile, copied_types.get ());
}

static void
show_convenience (const char *ignore, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();
  int varseen = 0;
  struct value_print_options opts;

  get_user_print_options (&opts);
  for (auto &pair : internalvars)
    {
      internalvar &var = pair.second;

      if (!varseen)
	{
	  varseen = 1;
	}
      gdb_printf (("$%s = "), var.name.c_str ());

      try
	{
	  struct value *val;

	  val = value_of_internalvar (gdbarch, &var);
	  value_print (val, gdb_stdout, &opts);
	}
      catch (const gdb_exception_error &ex)
	{
	  fprintf_styled (gdb_stdout, metadata_style.style (),
			  _("<error: %s>"), ex.what ());
	}

      gdb_printf (("\n"));
    }
  if (!varseen)
    {
      /* This text does not mention convenience functions on purpose.
	 The user can't create them except via Python, and if Python support
	 is installed this message will never be printed ($_streq will
	 exist).  */
      gdb_printf (_("No debugger convenience variables now defined.\n"
		    "Convenience variables have "
		    "names starting with \"$\";\n"
		    "use \"set\" as in \"set "
		    "$foo = 5\" to define them.\n"));
    }
}


/* See value.h.  */

struct value *
value::from_xmethod (xmethod_worker_up &&worker)
{
  struct value *v;

  v = value::allocate (builtin_type (current_inferior ()->arch ())->xmethod);
  v->m_lval = lval_xcallable;
  v->m_location.xm_worker = worker.release ();
  v->m_modifiable = false;

  return v;
}

/* See value.h.  */

struct type *
value::result_type_of_xmethod (gdb::array_view<value *> argv)
{
  gdb_assert (type ()->code () == TYPE_CODE_XMETHOD
	      && m_lval == lval_xcallable && !argv.empty ());

  return m_location.xm_worker->get_result_type (argv[0], argv.slice (1));
}

/* See value.h.  */

struct value *
value::call_xmethod (gdb::array_view<value *> argv)
{
  gdb_assert (type ()->code () == TYPE_CODE_XMETHOD
	      && m_lval == lval_xcallable && !argv.empty ());

  return m_location.xm_worker->invoke (argv[0], argv.slice (1));
}

/* Extract a value as a C number (either long or double).
   Knows how to convert fixed values to double, or
   floating values to long.
   Does not deallocate the value.  */

LONGEST
value_as_long (struct value *val)
{
  /* This coerces arrays and functions, which is necessary (e.g.
     in disassemble_command).  It also dereferences references, which
     I suspect is the most logical thing to do.  */
  val = coerce_array (val);
  return unpack_long (val->type (), val->contents ().data ());
}

/* See value.h.  */

gdb_mpz
value_as_mpz (struct value *val)
{
  val = coerce_array (val);
  struct type *type = check_typedef (val->type ());

  switch (type->code ())
    {
    case TYPE_CODE_ENUM:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_INT:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_RANGE:
      break;

    default:
      return gdb_mpz (value_as_long (val));
    }

  gdb_mpz result;

  gdb::array_view<const gdb_byte> valbytes = val->contents ();
  enum bfd_endian byte_order = type_byte_order (type);

  /* Handle integers that are either not a multiple of the word size,
     or that are stored at some bit offset.  */
  unsigned bit_off = 0, bit_size = 0;
  if (type->bit_size_differs_p ())
    {
      bit_size = type->bit_size ();
      if (bit_size == 0)
	{
	  /* We can just handle this immediately.  */
	  return result;
	}

      bit_off = type->bit_offset ();

      unsigned n_bytes = ((bit_off % 8) + bit_size + 7) / 8;
      valbytes = valbytes.slice (bit_off / 8, n_bytes);

      if (byte_order == BFD_ENDIAN_BIG)
	bit_off = (n_bytes * 8 - bit_off % 8 - bit_size);
      else
	bit_off %= 8;
    }

  result.read (val->contents (), byte_order, type->is_unsigned ());

  /* Shift off any low bits, if needed.  */
  if (bit_off != 0)
    result >>= bit_off;

  /* Mask off any high bits, if needed.  */
  if (bit_size)
    result.mask (bit_size);

  /* Now handle any range bias.  */
  if (type->code () == TYPE_CODE_RANGE && type->bounds ()->bias != 0)
    {
      /* Unfortunately we have to box here, because LONGEST is
	 probably wider than long.  */
      result += gdb_mpz (type->bounds ()->bias);
    }

  return result;
}

/* Extract a value as a C pointer.  */

CORE_ADDR
value_as_address (struct value *val)
{
  struct gdbarch *gdbarch = val->type ()->arch ();

  /* Assume a CORE_ADDR can fit in a LONGEST (for now).  Not sure
     whether we want this to be true eventually.  */
#if 0
  /* gdbarch_addr_bits_remove is wrong if we are being called for a
     non-address (e.g. argument to "signal", "info break", etc.), or
     for pointers to char, in which the low bits *are* significant.  */
  return gdbarch_addr_bits_remove (gdbarch, value_as_long (val));
#else

  /* There are several targets (IA-64, PowerPC, and others) which
     don't represent pointers to functions as simply the address of
     the function's entry point.  For example, on the IA-64, a
     function pointer points to a two-word descriptor, generated by
     the linker, which contains the function's entry point, and the
     value the IA-64 "global pointer" register should have --- to
     support position-independent code.  The linker generates
     descriptors only for those functions whose addresses are taken.

     On such targets, it's difficult for GDB to convert an arbitrary
     function address into a function pointer; it has to either find
     an existing descriptor for that function, or call malloc and
     build its own.  On some targets, it is impossible for GDB to
     build a descriptor at all: the descriptor must contain a jump
     instruction; data memory cannot be executed; and code memory
     cannot be modified.

     Upon entry to this function, if VAL is a value of type `function'
     (that is, TYPE_CODE (val->type ()) == TYPE_CODE_FUNC), then
     val->address () is the address of the function.  This is what
     you'll get if you evaluate an expression like `main'.  The call
     to COERCE_ARRAY below actually does all the usual unary
     conversions, which includes converting values of type `function'
     to `pointer to function'.  This is the challenging conversion
     discussed above.  Then, `unpack_pointer' will convert that pointer
     back into an address.

     So, suppose the user types `disassemble foo' on an architecture
     with a strange function pointer representation, on which GDB
     cannot build its own descriptors, and suppose further that `foo'
     has no linker-built descriptor.  The address->pointer conversion
     will signal an error and prevent the command from running, even
     though the next step would have been to convert the pointer
     directly back into the same address.

     The following shortcut avoids this whole mess.  If VAL is a
     function, just return its address directly.  */
  if (val->type ()->code () == TYPE_CODE_FUNC
      || val->type ()->code () == TYPE_CODE_METHOD)
    return val->address ();

  val = coerce_array (val);

  /* Some architectures (e.g. Harvard), map instruction and data
     addresses onto a single large unified address space.  For
     instance: An architecture may consider a large integer in the
     range 0x10000000 .. 0x1000ffff to already represent a data
     addresses (hence not need a pointer to address conversion) while
     a small integer would still need to be converted integer to
     pointer to address.  Just assume such architectures handle all
     integer conversions in a single function.  */

  /* JimB writes:

     I think INTEGER_TO_ADDRESS is a good idea as proposed --- but we
     must admonish GDB hackers to make sure its behavior matches the
     compiler's, whenever possible.

     In general, I think GDB should evaluate expressions the same way
     the compiler does.  When the user copies an expression out of
     their source code and hands it to a `print' command, they should
     get the same value the compiler would have computed.  Any
     deviation from this rule can cause major confusion and annoyance,
     and needs to be justified carefully.  In other words, GDB doesn't
     really have the freedom to do these conversions in clever and
     useful ways.

     AndrewC pointed out that users aren't complaining about how GDB
     casts integers to pointers; they are complaining that they can't
     take an address from a disassembly listing and give it to `x/i'.
     This is certainly important.

     Adding an architecture method like integer_to_address() certainly
     makes it possible for GDB to "get it right" in all circumstances
     --- the target has complete control over how things get done, so
     people can Do The Right Thing for their target without breaking
     anyone else.  The standard doesn't specify how integers get
     converted to pointers; usually, the ABI doesn't either, but
     ABI-specific code is a more reasonable place to handle it.  */

  if (!val->type ()->is_pointer_or_reference ()
      && gdbarch_integer_to_address_p (gdbarch))
    return gdbarch_integer_to_address (gdbarch, val->type (),
				       val->contents ().data ());

  return unpack_pointer (val->type (), val->contents ().data ());
#endif
}

/* Unpack raw data (copied from debugee, target byte order) at VALADDR
   as a long, or as a double, assuming the raw data is described
   by type TYPE.  Knows how to convert different sizes of values
   and can convert between fixed and floating point.  We don't assume
   any alignment for the raw data.  Return value is in host byte order.

   If you want functions and arrays to be coerced to pointers, and
   references to be dereferenced, call value_as_long() instead.

   C++: It is assumed that the front-end has taken care of
   all matters concerning pointers to members.  A pointer
   to member which reaches here is considered to be equivalent
   to an INT (or some size).  After all, it is only an offset.  */

LONGEST
unpack_long (struct type *type, const gdb_byte *valaddr)
{
  if (is_fixed_point_type (type))
    type = type->fixed_point_type_base_type ();

  enum bfd_endian byte_order = type_byte_order (type);
  enum type_code code = type->code ();
  int len = type->length ();
  int nosign = type->is_unsigned ();

  switch (code)
    {
    case TYPE_CODE_TYPEDEF:
      return unpack_long (check_typedef (type), valaddr);
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_INT:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_MEMBERPTR:
      {
	LONGEST result;

	if (type->bit_size_differs_p ())
	  {
	    unsigned bit_off = type->bit_offset ();
	    unsigned bit_size = type->bit_size ();
	    if (bit_size == 0)
	      {
		/* unpack_bits_as_long doesn't handle this case the
		   way we'd like, so handle it here.  */
		result = 0;
	      }
	    else
	      result = unpack_bits_as_long (type, valaddr, bit_off, bit_size);
	  }
	else
	  {
	    if (nosign)
	      result = extract_unsigned_integer (valaddr, len, byte_order);
	    else
	      result = extract_signed_integer (valaddr, len, byte_order);
	  }
	if (code == TYPE_CODE_RANGE)
	  result += type->bounds ()->bias;
	return result;
      }

    case TYPE_CODE_FLT:
    case TYPE_CODE_DECFLOAT:
      return target_float_to_longest (valaddr, type);

    case TYPE_CODE_FIXED_POINT:
      {
	gdb_mpq vq;
	vq.read_fixed_point (gdb::make_array_view (valaddr, len),
			     byte_order, nosign,
			     type->fixed_point_scaling_factor ());

	gdb_mpz vz = vq.as_integer ();
	return vz.as_integer<LONGEST> ();
      }

    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
      /* Assume a CORE_ADDR can fit in a LONGEST (for now).  Not sure
	 whether we want this to be true eventually.  */
      return extract_typed_address (valaddr, type);

    default:
      error (_("Value can't be converted to integer."));
    }
}

/* Unpack raw data (copied from debugee, target byte order) at VALADDR
   as a CORE_ADDR, assuming the raw data is described by type TYPE.
   We don't assume any alignment for the raw data.  Return value is in
   host byte order.

   If you want functions and arrays to be coerced to pointers, and
   references to be dereferenced, call value_as_address() instead.

   C++: It is assumed that the front-end has taken care of
   all matters concerning pointers to members.  A pointer
   to member which reaches here is considered to be equivalent
   to an INT (or some size).  After all, it is only an offset.  */

CORE_ADDR
unpack_pointer (struct type *type, const gdb_byte *valaddr)
{
  /* Assume a CORE_ADDR can fit in a LONGEST (for now).  Not sure
     whether we want this to be true eventually.  */
  return unpack_long (type, valaddr);
}

bool
is_floating_value (struct value *val)
{
  struct type *type = check_typedef (val->type ());

  if (is_floating_type (type))
    {
      if (!target_float_is_valid (val->contents ().data (), type))
	error (_("Invalid floating value found in program."));
      return true;
    }

  return false;
}


/* Get the value of the FIELDNO'th field (which must be static) of
   TYPE.  */

struct value *
value_static_field (struct type *type, int fieldno)
{
  struct value *retval;

  switch (type->field (fieldno).loc_kind ())
    {
    case FIELD_LOC_KIND_PHYSADDR:
      retval = value_at_lazy (type->field (fieldno).type (),
			      type->field (fieldno).loc_physaddr ());
      break;
    case FIELD_LOC_KIND_PHYSNAME:
    {
      const char *phys_name = type->field (fieldno).loc_physname ();
      /* type->field (fieldno).name (); */
      struct block_symbol sym = lookup_symbol (phys_name, 0, VAR_DOMAIN, 0);

      if (sym.symbol == NULL)
	{
	  /* With some compilers, e.g. HP aCC, static data members are
	     reported as non-debuggable symbols.  */
	  struct bound_minimal_symbol msym
	    = lookup_minimal_symbol (phys_name, NULL, NULL);
	  struct type *field_type = type->field (fieldno).type ();

	  if (!msym.minsym)
	    retval = value::allocate_optimized_out (field_type);
	  else
	    retval = value_at_lazy (field_type, msym.value_address ());
	}
      else
	retval = value_of_variable (sym.symbol, sym.block);
      break;
    }
    default:
      gdb_assert_not_reached ("unexpected field location kind");
    }

  return retval;
}

/* Change the enclosing type of a value object VAL to NEW_ENCL_TYPE.
   You have to be careful here, since the size of the data area for the value
   is set by the length of the enclosing type.  So if NEW_ENCL_TYPE is bigger
   than the old enclosing type, you have to allocate more space for the
   data.  */

void
value::set_enclosing_type (struct type *new_encl_type)
{
  if (new_encl_type->length () > enclosing_type ()->length ())
    {
      check_type_length_before_alloc (new_encl_type);
      m_contents.reset ((gdb_byte *) xrealloc (m_contents.release (),
					       new_encl_type->length ()));
    }

  m_enclosing_type = new_encl_type;
}

/* See value.h.  */

struct value *
value::primitive_field (LONGEST offset, int fieldno, struct type *arg_type)
{
  struct value *v;
  struct type *type;
  int unit_size = gdbarch_addressable_memory_unit_size (arch ());

  arg_type = check_typedef (arg_type);
  type = arg_type->field (fieldno).type ();

  /* Call check_typedef on our type to make sure that, if TYPE
     is a TYPE_CODE_TYPEDEF, its length is set to the length
     of the target type instead of zero.  However, we do not
     replace the typedef type by the target type, because we want
     to keep the typedef in order to be able to print the type
     description correctly.  */
  check_typedef (type);

  if (arg_type->field (fieldno).bitsize ())
    {
      /* Handle packed fields.

	 Create a new value for the bitfield, with bitpos and bitsize
	 set.  If possible, arrange offset and bitpos so that we can
	 do a single aligned read of the size of the containing type.
	 Otherwise, adjust offset to the byte containing the first
	 bit.  Assume that the address, offset, and embedded offset
	 are sufficiently aligned.  */

      LONGEST bitpos = arg_type->field (fieldno).loc_bitpos ();
      LONGEST container_bitsize = type->length () * 8;

      v = value::allocate_lazy (type);
      v->set_bitsize (arg_type->field (fieldno).bitsize ());
      if ((bitpos % container_bitsize) + v->bitsize () <= container_bitsize
	  && type->length () <= (int) sizeof (LONGEST))
	v->set_bitpos (bitpos % container_bitsize);
      else
	v->set_bitpos (bitpos % 8);
      v->set_offset ((embedded_offset ()
		      + offset
		      + (bitpos - v->bitpos ()) / 8));
      v->set_parent (this);
      if (!lazy ())
	v->fetch_lazy ();
    }
  else if (fieldno < TYPE_N_BASECLASSES (arg_type))
    {
      /* This field is actually a base subobject, so preserve the
	 entire object's contents for later references to virtual
	 bases, etc.  */
      LONGEST boffset;

      /* Lazy register values with offsets are not supported.  */
      if (this->lval () == lval_register && lazy ())
	fetch_lazy ();

      /* We special case virtual inheritance here because this
	 requires access to the contents, which we would rather avoid
	 for references to ordinary fields of unavailable values.  */
      if (BASETYPE_VIA_VIRTUAL (arg_type, fieldno))
	boffset = baseclass_offset (arg_type, fieldno,
				    contents ().data (),
				    embedded_offset (),
				    address (),
				    this);
      else
	boffset = arg_type->field (fieldno).loc_bitpos () / 8;

      if (lazy ())
	v = value::allocate_lazy (enclosing_type ());
      else
	{
	  v = value::allocate (enclosing_type ());
	  contents_copy_raw (v, 0, 0, enclosing_type ()->length ());
	}
      v->deprecated_set_type (type);
      v->set_offset (this->offset ());
      v->set_embedded_offset (offset + embedded_offset () + boffset);
    }
  else if (NULL != TYPE_DATA_LOCATION (type))
    {
      /* Field is a dynamic data member.  */

      gdb_assert (0 == offset);
      /* We expect an already resolved data location.  */
      gdb_assert (TYPE_DATA_LOCATION (type)->is_constant ());
      /* For dynamic data types defer memory allocation
	 until we actual access the value.  */
      v = value::allocate_lazy (type);
    }
  else
    {
      /* Plain old data member */
      offset += (arg_type->field (fieldno).loc_bitpos ()
		 / (HOST_CHAR_BIT * unit_size));

      /* Lazy register values with offsets are not supported.  */
      if (this->lval () == lval_register && lazy ())
	fetch_lazy ();

      if (lazy ())
	v = value::allocate_lazy (type);
      else
	{
	  v = value::allocate (type);
	  contents_copy_raw (v, v->embedded_offset (),
			     embedded_offset () + offset,
			     type_length_units (type));
	}
      v->set_offset (this->offset () + offset + embedded_offset ());
    }
  v->set_component_location (this);
  return v;
}

/* Given a value ARG1 of a struct or union type,
   extract and return the value of one of its (non-static) fields.
   FIELDNO says which field.  */

struct value *
value_field (struct value *arg1, int fieldno)
{
  return arg1->primitive_field (0, fieldno, arg1->type ());
}

/* Return a non-virtual function as a value.
   F is the list of member functions which contains the desired method.
   J is an index into F which provides the desired method.

   We only use the symbol for its address, so be happy with either a
   full symbol or a minimal symbol.  */

struct value *
value_fn_field (struct value **arg1p, struct fn_field *f,
		int j, struct type *type,
		LONGEST offset)
{
  struct value *v;
  struct type *ftype = TYPE_FN_FIELD_TYPE (f, j);
  const char *physname = TYPE_FN_FIELD_PHYSNAME (f, j);
  struct symbol *sym;
  struct bound_minimal_symbol msym;

  sym = lookup_symbol (physname, 0, VAR_DOMAIN, 0).symbol;
  if (sym == nullptr)
    {
      msym = lookup_bound_minimal_symbol (physname);
      if (msym.minsym == NULL)
	return NULL;
    }

  v = value::allocate (ftype);
  v->set_lval (lval_memory);
  if (sym)
    {
      v->set_address (sym->value_block ()->entry_pc ());
    }
  else
    {
      /* The minimal symbol might point to a function descriptor;
	 resolve it to the actual code address instead.  */
      struct objfile *objfile = msym.objfile;
      struct gdbarch *gdbarch = objfile->arch ();

      v->set_address (gdbarch_convert_from_func_ptr_addr
		      (gdbarch, msym.value_address (),
		       current_inferior ()->top_target ()));
    }

  if (arg1p)
    {
      if (type != (*arg1p)->type ())
	*arg1p = value_ind (value_cast (lookup_pointer_type (type),
					value_addr (*arg1p)));

      /* Move the `this' pointer according to the offset.
	 (*arg1p)->offset () += offset; */
    }

  return v;
}



/* See value.h.  */

LONGEST
unpack_bits_as_long (struct type *field_type, const gdb_byte *valaddr,
		     LONGEST bitpos, LONGEST bitsize)
{
  enum bfd_endian byte_order = type_byte_order (field_type);
  ULONGEST val;
  ULONGEST valmask;
  int lsbcount;
  LONGEST bytes_read;
  LONGEST read_offset;

  /* Read the minimum number of bytes required; there may not be
     enough bytes to read an entire ULONGEST.  */
  field_type = check_typedef (field_type);
  if (bitsize)
    bytes_read = ((bitpos % 8) + bitsize + 7) / 8;
  else
    {
      bytes_read = field_type->length ();
      bitsize = 8 * bytes_read;
    }

  read_offset = bitpos / 8;

  val = extract_unsigned_integer (valaddr + read_offset,
				  bytes_read, byte_order);

  /* Extract bits.  See comment above.  */

  if (byte_order == BFD_ENDIAN_BIG)
    lsbcount = (bytes_read * 8 - bitpos % 8 - bitsize);
  else
    lsbcount = (bitpos % 8);
  val >>= lsbcount;

  /* If the field does not entirely fill a LONGEST, then zero the sign bits.
     If the field is signed, and is negative, then sign extend.  */

  if (bitsize < 8 * (int) sizeof (val))
    {
      valmask = (((ULONGEST) 1) << bitsize) - 1;
      val &= valmask;
      if (!field_type->is_unsigned ())
	{
	  if (val & (valmask ^ (valmask >> 1)))
	    {
	      val |= ~valmask;
	    }
	}
    }

  return val;
}

/* Unpack a field FIELDNO of the specified TYPE, from the object at
   VALADDR + EMBEDDED_OFFSET.  VALADDR points to the contents of
   ORIGINAL_VALUE, which must not be NULL.  See
   unpack_value_bits_as_long for more details.  */

int
unpack_value_field_as_long (struct type *type, const gdb_byte *valaddr,
			    LONGEST embedded_offset, int fieldno,
			    const struct value *val, LONGEST *result)
{
  int bitpos = type->field (fieldno).loc_bitpos ();
  int bitsize = type->field (fieldno).bitsize ();
  struct type *field_type = type->field (fieldno).type ();
  int bit_offset;

  gdb_assert (val != NULL);

  bit_offset = embedded_offset * TARGET_CHAR_BIT + bitpos;
  if (val->bits_any_optimized_out (bit_offset, bitsize)
      || !val->bits_available (bit_offset, bitsize))
    return 0;

  *result = unpack_bits_as_long (field_type, valaddr + embedded_offset,
				 bitpos, bitsize);
  return 1;
}

/* Unpack a field FIELDNO of the specified TYPE, from the anonymous
   object at VALADDR.  See unpack_bits_as_long for more details.  */

LONGEST
unpack_field_as_long (struct type *type, const gdb_byte *valaddr, int fieldno)
{
  int bitpos = type->field (fieldno).loc_bitpos ();
  int bitsize = type->field (fieldno).bitsize ();
  struct type *field_type = type->field (fieldno).type ();

  return unpack_bits_as_long (field_type, valaddr, bitpos, bitsize);
}

/* See value.h.  */

void
value::unpack_bitfield (struct value *dest_val,
			LONGEST bitpos, LONGEST bitsize,
			const gdb_byte *valaddr, LONGEST embedded_offset)
  const
{
  enum bfd_endian byte_order;
  int src_bit_offset;
  int dst_bit_offset;
  struct type *field_type = dest_val->type ();

  byte_order = type_byte_order (field_type);

  /* First, unpack and sign extend the bitfield as if it was wholly
     valid.  Optimized out/unavailable bits are read as zero, but
     that's OK, as they'll end up marked below.  If the VAL is
     wholly-invalid we may have skipped allocating its contents,
     though.  See value::allocate_optimized_out.  */
  if (valaddr != NULL)
    {
      LONGEST num;

      num = unpack_bits_as_long (field_type, valaddr + embedded_offset,
				 bitpos, bitsize);
      store_signed_integer (dest_val->contents_raw ().data (),
			    field_type->length (), byte_order, num);
    }

  /* Now copy the optimized out / unavailability ranges to the right
     bits.  */
  src_bit_offset = embedded_offset * TARGET_CHAR_BIT + bitpos;
  if (byte_order == BFD_ENDIAN_BIG)
    dst_bit_offset = field_type->length () * TARGET_CHAR_BIT - bitsize;
  else
    dst_bit_offset = 0;
  ranges_copy_adjusted (dest_val, dst_bit_offset, src_bit_offset, bitsize);
}

/* Return a new value with type TYPE, which is FIELDNO field of the
   object at VALADDR + EMBEDDEDOFFSET.  VALADDR points to the contents
   of VAL.  If the VAL's contents required to extract the bitfield
   from are unavailable/optimized out, the new value is
   correspondingly marked unavailable/optimized out.  */

struct value *
value_field_bitfield (struct type *type, int fieldno,
		      const gdb_byte *valaddr,
		      LONGEST embedded_offset, const struct value *val)
{
  int bitpos = type->field (fieldno).loc_bitpos ();
  int bitsize = type->field (fieldno).bitsize ();
  struct value *res_val = value::allocate (type->field (fieldno).type ());

  val->unpack_bitfield (res_val, bitpos, bitsize, valaddr, embedded_offset);

  return res_val;
}

/* Modify the value of a bitfield.  ADDR points to a block of memory in
   target byte order; the bitfield starts in the byte pointed to.  FIELDVAL
   is the desired value of the field, in host byte order.  BITPOS and BITSIZE
   indicate which bits (in target bit order) comprise the bitfield.
   Requires 0 < BITSIZE <= lbits, 0 <= BITPOS % 8 + BITSIZE <= lbits, and
   0 <= BITPOS, where lbits is the size of a LONGEST in bits.  */

void
modify_field (struct type *type, gdb_byte *addr,
	      LONGEST fieldval, LONGEST bitpos, LONGEST bitsize)
{
  enum bfd_endian byte_order = type_byte_order (type);
  ULONGEST oword;
  ULONGEST mask = (ULONGEST) -1 >> (8 * sizeof (ULONGEST) - bitsize);
  LONGEST bytesize;

  /* Normalize BITPOS.  */
  addr += bitpos / 8;
  bitpos %= 8;

  /* If a negative fieldval fits in the field in question, chop
     off the sign extension bits.  */
  if ((~fieldval & ~(mask >> 1)) == 0)
    fieldval &= mask;

  /* Warn if value is too big to fit in the field in question.  */
  if (0 != (fieldval & ~mask))
    {
      /* FIXME: would like to include fieldval in the message, but
	 we don't have a sprintf_longest.  */
      warning (_("Value does not fit in %s bits."), plongest (bitsize));

      /* Truncate it, otherwise adjoining fields may be corrupted.  */
      fieldval &= mask;
    }

  /* Ensure no bytes outside of the modified ones get accessed as it may cause
     false valgrind reports.  */

  bytesize = (bitpos + bitsize + 7) / 8;
  oword = extract_unsigned_integer (addr, bytesize, byte_order);

  /* Shifting for bit field depends on endianness of the target machine.  */
  if (byte_order == BFD_ENDIAN_BIG)
    bitpos = bytesize * 8 - bitpos - bitsize;

  oword &= ~(mask << bitpos);
  oword |= fieldval << bitpos;

  store_unsigned_integer (addr, bytesize, byte_order, oword);
}

/* Pack NUM into BUF using a target format of TYPE.  */

void
pack_long (gdb_byte *buf, struct type *type, LONGEST num)
{
  enum bfd_endian byte_order = type_byte_order (type);
  LONGEST len;

  type = check_typedef (type);
  len = type->length ();

  switch (type->code ())
    {
    case TYPE_CODE_RANGE:
      num -= type->bounds ()->bias;
      [[fallthrough]];
    case TYPE_CODE_INT:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_MEMBERPTR:
      if (type->bit_size_differs_p ())
	{
	  unsigned bit_off = type->bit_offset ();
	  unsigned bit_size = type->bit_size ();
	  num &= ((ULONGEST) 1 << bit_size) - 1;
	  num <<= bit_off;
	}
      store_signed_integer (buf, len, byte_order, num);
      break;

    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
    case TYPE_CODE_PTR:
      store_typed_address (buf, type, (CORE_ADDR) num);
      break;

    case TYPE_CODE_FLT:
    case TYPE_CODE_DECFLOAT:
      target_float_from_longest (buf, type, num);
      break;

    default:
      error (_("Unexpected type (%d) encountered for integer constant."),
	     type->code ());
    }
}


/* Pack NUM into BUF using a target format of TYPE.  */

static void
pack_unsigned_long (gdb_byte *buf, struct type *type, ULONGEST num)
{
  LONGEST len;
  enum bfd_endian byte_order;

  type = check_typedef (type);
  len = type->length ();
  byte_order = type_byte_order (type);

  switch (type->code ())
    {
    case TYPE_CODE_INT:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_MEMBERPTR:
      if (type->bit_size_differs_p ())
	{
	  unsigned bit_off = type->bit_offset ();
	  unsigned bit_size = type->bit_size ();
	  num &= ((ULONGEST) 1 << bit_size) - 1;
	  num <<= bit_off;
	}
      store_unsigned_integer (buf, len, byte_order, num);
      break;

    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
    case TYPE_CODE_PTR:
      store_typed_address (buf, type, (CORE_ADDR) num);
      break;

    case TYPE_CODE_FLT:
    case TYPE_CODE_DECFLOAT:
      target_float_from_ulongest (buf, type, num);
      break;

    default:
      error (_("Unexpected type (%d) encountered "
	       "for unsigned integer constant."),
	     type->code ());
    }
}

/* See value.h.  */

struct value *
value::zero (struct type *type, enum lval_type lv)
{
  struct value *val = value::allocate_lazy (type);

  val->set_lval (lv == lval_computed ? not_lval : lv);
  val->m_is_zero = true;
  return val;
}

/* Convert C numbers into newly allocated values.  */

struct value *
value_from_longest (struct type *type, LONGEST num)
{
  struct value *val = value::allocate (type);

  pack_long (val->contents_raw ().data (), type, num);
  return val;
}


/* Convert C unsigned numbers into newly allocated values.  */

struct value *
value_from_ulongest (struct type *type, ULONGEST num)
{
  struct value *val = value::allocate (type);

  pack_unsigned_long (val->contents_raw ().data (), type, num);

  return val;
}

/* See value.h.  */

struct value *
value_from_mpz (struct type *type, const gdb_mpz &v)
{
  struct type *real_type = check_typedef (type);

  const gdb_mpz *val = &v;
  gdb_mpz storage;
  if (real_type->code () == TYPE_CODE_RANGE && type->bounds ()->bias != 0)
    {
      storage = *val;
      val = &storage;
      storage -= type->bounds ()->bias;
    }

  if (type->bit_size_differs_p ())
    {
      unsigned bit_off = type->bit_offset ();
      unsigned bit_size = type->bit_size ();

      if (val != &storage)
	{
	  storage = *val;
	  val = &storage;
	}

      storage.mask (bit_size);
      storage <<= bit_off;
    }

  struct value *result = value::allocate (type);
  val->truncate (result->contents_raw (), type_byte_order (type),
		 type->is_unsigned ());
  return result;
}

/* Create a value representing a pointer of type TYPE to the address
   ADDR.  */

struct value *
value_from_pointer (struct type *type, CORE_ADDR addr)
{
  struct value *val = value::allocate (type);

  store_typed_address (val->contents_raw ().data (),
		       check_typedef (type), addr);
  return val;
}

/* Create and return a value object of TYPE containing the value D.  The
   TYPE must be of TYPE_CODE_FLT, and must be large enough to hold D once
   it is converted to target format.  */

struct value *
value_from_host_double (struct type *type, double d)
{
  struct value *value = value::allocate (type);
  gdb_assert (type->code () == TYPE_CODE_FLT);
  target_float_from_host_double (value->contents_raw ().data (),
				 value->type (), d);
  return value;
}

/* Create a value of type TYPE whose contents come from VALADDR, if it
   is non-null, and whose memory address (in the inferior) is
   ADDRESS.  The type of the created value may differ from the passed
   type TYPE.  Make sure to retrieve values new type after this call.
   Note that TYPE is not passed through resolve_dynamic_type; this is
   a special API intended for use only by Ada.  */

struct value *
value_from_contents_and_address_unresolved (struct type *type,
					    const gdb_byte *valaddr,
					    CORE_ADDR address)
{
  struct value *v;

  if (valaddr == NULL)
    v = value::allocate_lazy (type);
  else
    v = value_from_contents (type, valaddr);
  v->set_lval (lval_memory);
  v->set_address (address);
  return v;
}

/* Create a value of type TYPE whose contents come from VALADDR, if it
   is non-null, and whose memory address (in the inferior) is
   ADDRESS.  The type of the created value may differ from the passed
   type TYPE.  Make sure to retrieve values new type after this call.  */

struct value *
value_from_contents_and_address (struct type *type,
				 const gdb_byte *valaddr,
				 CORE_ADDR address,
				 frame_info_ptr frame)
{
  gdb::array_view<const gdb_byte> view;
  if (valaddr != nullptr)
    view = gdb::make_array_view (valaddr, type->length ());
  struct type *resolved_type = resolve_dynamic_type (type, view, address,
						     &frame);
  struct type *resolved_type_no_typedef = check_typedef (resolved_type);
  struct value *v;

  if (valaddr == NULL)
    v = value::allocate_lazy (resolved_type);
  else
    v = value_from_contents (resolved_type, valaddr);
  if (TYPE_DATA_LOCATION (resolved_type_no_typedef) != NULL
      && TYPE_DATA_LOCATION (resolved_type_no_typedef)->is_constant ())
    address = TYPE_DATA_LOCATION_ADDR (resolved_type_no_typedef);
  v->set_lval (lval_memory);
  v->set_address (address);
  return v;
}

/* Create a value of type TYPE holding the contents CONTENTS.
   The new value is `not_lval'.  */

struct value *
value_from_contents (struct type *type, const gdb_byte *contents)
{
  struct value *result;

  result = value::allocate (type);
  memcpy (result->contents_raw ().data (), contents, type->length ());
  return result;
}

/* Extract a value from the history file.  Input will be of the form
   $digits or $$digits.  See block comment above 'write_dollar_variable'
   for details.  */

struct value *
value_from_history_ref (const char *h, const char **endp)
{
  int index, len;

  if (h[0] == '$')
    len = 1;
  else
    return NULL;

  if (h[1] == '$')
    len = 2;

  /* Find length of numeral string.  */
  for (; isdigit (h[len]); len++)
    ;

  /* Make sure numeral string is not part of an identifier.  */
  if (h[len] == '_' || isalpha (h[len]))
    return NULL;

  /* Now collect the index value.  */
  if (h[1] == '$')
    {
      if (len == 2)
	{
	  /* For some bizarre reason, "$$" is equivalent to "$$1", 
	     rather than to "$$0" as it ought to be!  */
	  index = -1;
	  *endp += len;
	}
      else
	{
	  char *local_end;

	  index = -strtol (&h[2], &local_end, 10);
	  *endp = local_end;
	}
    }
  else
    {
      if (len == 1)
	{
	  /* "$" is equivalent to "$0".  */
	  index = 0;
	  *endp += len;
	}
      else
	{
	  char *local_end;

	  index = strtol (&h[1], &local_end, 10);
	  *endp = local_end;
	}
    }

  return access_value_history (index);
}

/* Get the component value (offset by OFFSET bytes) of a struct or
   union WHOLE.  Component's type is TYPE.  */

struct value *
value_from_component (struct value *whole, struct type *type, LONGEST offset)
{
  struct value *v;

  if (whole->lval () == lval_memory && whole->lazy ())
    v = value::allocate_lazy (type);
  else
    {
      v = value::allocate (type);
      whole->contents_copy (v, v->embedded_offset (),
			    whole->embedded_offset () + offset,
			    type_length_units (type));
    }
  v->set_offset (whole->offset () + offset + whole->embedded_offset ());
  v->set_component_location (whole);

  return v;
}

/* See value.h.  */

struct value *
value::from_component_bitsize (struct type *type,
			       LONGEST bit_offset, LONGEST bit_length)
{
  gdb_assert (!lazy ());

  /* Preserve lvalue-ness if possible.  This is needed to avoid
     array-printing failures (including crashes) when printing Ada
     arrays in programs compiled with -fgnat-encodings=all.  */
  if ((bit_offset % TARGET_CHAR_BIT) == 0
      && (bit_length % TARGET_CHAR_BIT) == 0
      && bit_length == TARGET_CHAR_BIT * type->length ())
    return value_from_component (this, type, bit_offset / TARGET_CHAR_BIT);

  struct value *v = value::allocate (type);

  LONGEST dst_offset = TARGET_CHAR_BIT * v->embedded_offset ();
  if (is_scalar_type (type) && type_byte_order (type) == BFD_ENDIAN_BIG)
    dst_offset += TARGET_CHAR_BIT * type->length () - bit_length;

  contents_copy_raw_bitwise (v, dst_offset,
			     TARGET_CHAR_BIT
			     * embedded_offset ()
			     + bit_offset,
			     bit_length);
  return v;
}

struct value *
coerce_ref_if_computed (const struct value *arg)
{
  const struct lval_funcs *funcs;

  if (!TYPE_IS_REFERENCE (check_typedef (arg->type ())))
    return NULL;

  if (arg->lval () != lval_computed)
    return NULL;

  funcs = arg->computed_funcs ();
  if (funcs->coerce_ref == NULL)
    return NULL;

  return funcs->coerce_ref (arg);
}

/* Look at value.h for description.  */

struct value *
readjust_indirect_value_type (struct value *value, struct type *enc_type,
			      const struct type *original_type,
			      struct value *original_value,
			      CORE_ADDR original_value_address)
{
  gdb_assert (original_type->is_pointer_or_reference ());

  struct type *original_target_type = original_type->target_type ();
  gdb::array_view<const gdb_byte> view;
  struct type *resolved_original_target_type
    = resolve_dynamic_type (original_target_type, view,
			    original_value_address);

  /* Re-adjust type.  */
  value->deprecated_set_type (resolved_original_target_type);

  /* Add embedding info.  */
  value->set_enclosing_type (enc_type);
  value->set_embedded_offset (original_value->pointed_to_offset ());

  /* We may be pointing to an object of some derived type.  */
  return value_full_object (value, NULL, 0, 0, 0);
}

struct value *
coerce_ref (struct value *arg)
{
  struct type *value_type_arg_tmp = check_typedef (arg->type ());
  struct value *retval;
  struct type *enc_type;

  retval = coerce_ref_if_computed (arg);
  if (retval)
    return retval;

  if (!TYPE_IS_REFERENCE (value_type_arg_tmp))
    return arg;

  enc_type = check_typedef (arg->enclosing_type ());
  enc_type = enc_type->target_type ();

  CORE_ADDR addr = unpack_pointer (arg->type (), arg->contents ().data ());
  retval = value_at_lazy (enc_type, addr);
  enc_type = retval->type ();
  return readjust_indirect_value_type (retval, enc_type, value_type_arg_tmp,
				       arg, addr);
}

struct value *
coerce_array (struct value *arg)
{
  struct type *type;

  arg = coerce_ref (arg);
  type = check_typedef (arg->type ());

  switch (type->code ())
    {
    case TYPE_CODE_ARRAY:
      if (!type->is_vector () && current_language->c_style_arrays_p ())
	arg = value_coerce_array (arg);
      break;
    case TYPE_CODE_FUNC:
      arg = value_coerce_function (arg);
      break;
    }
  return arg;
}


/* Return the return value convention that will be used for the
   specified type.  */

enum return_value_convention
struct_return_convention (struct gdbarch *gdbarch,
			  struct value *function, struct type *value_type)
{
  enum type_code code = value_type->code ();

  if (code == TYPE_CODE_ERROR)
    error (_("Function return type unknown."));

  /* Probe the architecture for the return-value convention.  */
  return gdbarch_return_value_as_value (gdbarch, function, value_type,
					NULL, NULL, NULL);
}

/* Return true if the function returning the specified type is using
   the convention of returning structures in memory (passing in the
   address as a hidden first parameter).  */

int
using_struct_return (struct gdbarch *gdbarch,
		     struct value *function, struct type *value_type)
{
  if (value_type->code () == TYPE_CODE_VOID)
    /* A void return value is never in memory.  See also corresponding
       code in "print_return_value".  */
    return 0;

  return (struct_return_convention (gdbarch, function, value_type)
	  != RETURN_VALUE_REGISTER_CONVENTION);
}

/* See value.h.  */

void
value::fetch_lazy_bitfield ()
{
  gdb_assert (bitsize () != 0);

  /* To read a lazy bitfield, read the entire enclosing value.  This
     prevents reading the same block of (possibly volatile) memory once
     per bitfield.  It would be even better to read only the containing
     word, but we have no way to record that just specific bits of a
     value have been fetched.  */
  struct value *parent = this->parent ();

  if (parent->lazy ())
    parent->fetch_lazy ();

  parent->unpack_bitfield (this, bitpos (), bitsize (),
			   parent->contents_for_printing ().data (),
			   offset ());
}

/* See value.h.  */

void
value::fetch_lazy_memory ()
{
  gdb_assert (m_lval == lval_memory);

  CORE_ADDR addr = address ();
  struct type *type = check_typedef (enclosing_type ());

  /* Figure out how much we should copy from memory.  Usually, this is just
     the size of the type, but, for arrays, we might only be loading a
     small part of the array (this is only done for very large arrays).  */
  int len = 0;
  if (m_limited_length > 0)
    {
      gdb_assert (this->type ()->code () == TYPE_CODE_ARRAY);
      len = m_limited_length;
    }
  else if (type->length () > 0)
    len = type_length_units (type);

  gdb_assert (len >= 0);

  if (len > 0)
    read_value_memory (this, 0, stack (), addr,
		       contents_all_raw ().data (), len);
}

/* See value.h.  */

void
value::fetch_lazy_register ()
{
  struct type *type = check_typedef (this->type ());
  struct value *new_val = this;

  scoped_value_mark mark;

  /* Offsets are not supported here; lazy register values must
     refer to the entire register.  */
  gdb_assert (offset () == 0);

  while (new_val->lval () == lval_register && new_val->lazy ())
    {
      frame_id next_frame_id = new_val->next_frame_id ();
      frame_info_ptr next_frame = frame_find_by_id (next_frame_id);
      gdb_assert (next_frame != NULL);

      int regnum = new_val->regnum ();

      /* Convertible register routines are used for multi-register
	 values and for interpretation in different types
	 (e.g. float or int from a double register).  Lazy
	 register values should have the register's natural type,
	 so they do not apply.  */
      gdb_assert (!gdbarch_convert_register_p (get_frame_arch (next_frame),
					       regnum, type));

      new_val = frame_unwind_register_value (next_frame, regnum);

      /* If we get another lazy lval_register value, it means the
	 register is found by reading it from NEXT_FRAME's next frame.
	 frame_unwind_register_value should never return a value with
	 the frame id pointing to NEXT_FRAME.  If it does, it means we
	 either have two consecutive frames with the same frame id
	 in the frame chain, or some code is trying to unwind
	 behind get_prev_frame's back (e.g., a frame unwind
	 sniffer trying to unwind), bypassing its validations.  In
	 any case, it should always be an internal error to end up
	 in this situation.  */
      if (new_val->lval () == lval_register
	  && new_val->lazy ()
	  && new_val->next_frame_id () == next_frame_id)
	internal_error (_("infinite loop while fetching a register"));
    }

  /* If it's still lazy (for instance, a saved register on the
     stack), fetch it.  */
  if (new_val->lazy ())
    new_val->fetch_lazy ();

  /* Copy the contents and the unavailability/optimized-out
     meta-data from NEW_VAL to VAL.  */
  set_lazy (false);
  new_val->contents_copy (this, embedded_offset (),
			  new_val->embedded_offset (),
			  type_length_units (type));

  if (frame_debug)
    {
      frame_info_ptr frame = frame_find_by_id (this->next_frame_id ());
      frame = get_prev_frame_always (frame);
      int regnum = this->regnum ();
      gdbarch *gdbarch = get_frame_arch (frame);

      string_file debug_file;
      gdb_printf (&debug_file,
		  "(frame=%d, regnum=%d(%s), ...) ",
		  frame_relative_level (frame), regnum,
		  user_reg_map_regnum_to_name (gdbarch, regnum));

      gdb_printf (&debug_file, "->");
      if (new_val->optimized_out ())
	{
	  gdb_printf (&debug_file, " ");
	  val_print_optimized_out (new_val, &debug_file);
	}
      else
	{
	  int i;
	  gdb::array_view<const gdb_byte> buf = new_val->contents ();

	  if (new_val->lval () == lval_register)
	    gdb_printf (&debug_file, " register=%d", new_val->regnum ());
	  else if (new_val->lval () == lval_memory)
	    gdb_printf (&debug_file, " address=%s",
			paddress (gdbarch,
				  new_val->address ()));
	  else
	    gdb_printf (&debug_file, " computed");

	  gdb_printf (&debug_file, " bytes=");
	  gdb_printf (&debug_file, "[");
	  for (i = 0; i < register_size (gdbarch, regnum); i++)
	    gdb_printf (&debug_file, "%02x", buf[i]);
	  gdb_printf (&debug_file, "]");
	}

      frame_debug_printf ("%s", debug_file.c_str ());
    }
}

/* See value.h.  */

void
value::fetch_lazy ()
{
  gdb_assert (lazy ());
  allocate_contents (true);
  /* A value is either lazy, or fully fetched.  The
     availability/validity is only established as we try to fetch a
     value.  */
  gdb_assert (m_optimized_out.empty ());
  gdb_assert (m_unavailable.empty ());
  if (m_is_zero)
    {
      /* Nothing.  */
    }
  else if (bitsize ())
    fetch_lazy_bitfield ();
  else if (this->lval () == lval_memory)
    fetch_lazy_memory ();
  else if (this->lval () == lval_register)
    fetch_lazy_register ();
  else if (this->lval () == lval_computed
	   && computed_funcs ()->read != NULL)
    computed_funcs ()->read (this);
  else
    internal_error (_("Unexpected lazy value type."));

  set_lazy (false);
}

/* See value.h.  */

value *
pseudo_from_raw_part (frame_info_ptr next_frame, int pseudo_reg_num,
		      int raw_reg_num, int raw_offset)
{
  value *pseudo_reg_val
    = value::allocate_register (next_frame, pseudo_reg_num);
  value *raw_reg_val = value_of_register (raw_reg_num, next_frame);
  raw_reg_val->contents_copy (pseudo_reg_val, 0, raw_offset,
			      pseudo_reg_val->type ()->length ());
  return pseudo_reg_val;
}

/* See value.h.  */

void
pseudo_to_raw_part (frame_info_ptr next_frame,
		    gdb::array_view<const gdb_byte> pseudo_buf,
		    int raw_reg_num, int raw_offset)
{
  int raw_reg_size
    = register_size (frame_unwind_arch (next_frame), raw_reg_num);

  /* When overflowing a register, put_frame_register_bytes writes to the
     subsequent registers.  We don't want that behavior here, so make sure
     the write is wholly within register RAW_REG_NUM.  */
  gdb_assert (raw_offset + pseudo_buf.size () <= raw_reg_size);
  put_frame_register_bytes (next_frame, raw_reg_num, raw_offset, pseudo_buf);
}

/* See value.h.  */

value *
pseudo_from_concat_raw (frame_info_ptr next_frame, int pseudo_reg_num,
			int raw_reg_1_num, int raw_reg_2_num)
{
  value *pseudo_reg_val
    = value::allocate_register (next_frame, pseudo_reg_num);
  int dst_offset = 0;

  value *raw_reg_1_val = value_of_register (raw_reg_1_num, next_frame);
  raw_reg_1_val->contents_copy (pseudo_reg_val, dst_offset, 0,
				raw_reg_1_val->type ()->length ());
  dst_offset += raw_reg_1_val->type ()->length ();

  value *raw_reg_2_val = value_of_register (raw_reg_2_num, next_frame);
  raw_reg_2_val->contents_copy (pseudo_reg_val, dst_offset, 0,
				raw_reg_2_val->type ()->length ());
  dst_offset += raw_reg_2_val->type ()->length ();

  gdb_assert (dst_offset == pseudo_reg_val->type ()->length ());

  return pseudo_reg_val;
}

/* See value.h. */

void
pseudo_to_concat_raw (frame_info_ptr next_frame,
		      gdb::array_view<const gdb_byte> pseudo_buf,
		      int raw_reg_1_num, int raw_reg_2_num)
{
  int src_offset = 0;
  gdbarch *arch = frame_unwind_arch (next_frame);

  int raw_reg_1_size = register_size (arch, raw_reg_1_num);
  put_frame_register (next_frame, raw_reg_1_num,
		      pseudo_buf.slice (src_offset, raw_reg_1_size));
  src_offset += raw_reg_1_size;

  int raw_reg_2_size = register_size (arch, raw_reg_2_num);
  put_frame_register (next_frame, raw_reg_2_num,
		      pseudo_buf.slice (src_offset, raw_reg_2_size));
  src_offset += raw_reg_2_size;

  gdb_assert (src_offset == pseudo_buf.size ());
}

/* See value.h.  */

value *
pseudo_from_concat_raw (frame_info_ptr next_frame, int pseudo_reg_num,
			int raw_reg_1_num, int raw_reg_2_num,
			int raw_reg_3_num)
{
  value *pseudo_reg_val
    = value::allocate_register (next_frame, pseudo_reg_num);
  int dst_offset = 0;

  value *raw_reg_1_val = value_of_register (raw_reg_1_num, next_frame);
  raw_reg_1_val->contents_copy (pseudo_reg_val, dst_offset, 0,
				raw_reg_1_val->type ()->length ());
  dst_offset += raw_reg_1_val->type ()->length ();

  value *raw_reg_2_val = value_of_register (raw_reg_2_num, next_frame);
  raw_reg_2_val->contents_copy (pseudo_reg_val, dst_offset, 0,
				raw_reg_2_val->type ()->length ());
  dst_offset += raw_reg_2_val->type ()->length ();

  value *raw_reg_3_val = value_of_register (raw_reg_3_num, next_frame);
  raw_reg_3_val->contents_copy (pseudo_reg_val, dst_offset, 0,
				raw_reg_3_val->type ()->length ());
  dst_offset += raw_reg_3_val->type ()->length ();

  gdb_assert (dst_offset == pseudo_reg_val->type ()->length ());

  return pseudo_reg_val;
}

/* See value.h. */

void
pseudo_to_concat_raw (frame_info_ptr next_frame,
		      gdb::array_view<const gdb_byte> pseudo_buf,
		      int raw_reg_1_num, int raw_reg_2_num, int raw_reg_3_num)
{
  int src_offset = 0;
  gdbarch *arch = frame_unwind_arch (next_frame);

  int raw_reg_1_size = register_size (arch, raw_reg_1_num);
  put_frame_register (next_frame, raw_reg_1_num,
		      pseudo_buf.slice (src_offset, raw_reg_1_size));
  src_offset += raw_reg_1_size;

  int raw_reg_2_size = register_size (arch, raw_reg_2_num);
  put_frame_register (next_frame, raw_reg_2_num,
		      pseudo_buf.slice (src_offset, raw_reg_2_size));
  src_offset += raw_reg_2_size;

  int raw_reg_3_size = register_size (arch, raw_reg_3_num);
  put_frame_register (next_frame, raw_reg_3_num,
		      pseudo_buf.slice (src_offset, raw_reg_3_size));
  src_offset += raw_reg_3_size;

  gdb_assert (src_offset == pseudo_buf.size ());
}

/* Implementation of the convenience function $_isvoid.  */

static struct value *
isvoid_internal_fn (struct gdbarch *gdbarch,
		    const struct language_defn *language,
		    void *cookie, int argc, struct value **argv)
{
  int ret;

  if (argc != 1)
    error (_("You must provide one argument for $_isvoid."));

  ret = argv[0]->type ()->code () == TYPE_CODE_VOID;

  return value_from_longest (builtin_type (gdbarch)->builtin_int, ret);
}

/* Implementation of the convenience function $_creal.  Extracts the
   real part from a complex number.  */

static struct value *
creal_internal_fn (struct gdbarch *gdbarch,
		   const struct language_defn *language,
		   void *cookie, int argc, struct value **argv)
{
  if (argc != 1)
    error (_("You must provide one argument for $_creal."));

  value *cval = argv[0];
  type *ctype = check_typedef (cval->type ());
  if (ctype->code () != TYPE_CODE_COMPLEX)
    error (_("expected a complex number"));
  return value_real_part (cval);
}

/* Implementation of the convenience function $_cimag.  Extracts the
   imaginary part from a complex number.  */

static struct value *
cimag_internal_fn (struct gdbarch *gdbarch,
		   const struct language_defn *language,
		   void *cookie, int argc,
		   struct value **argv)
{
  if (argc != 1)
    error (_("You must provide one argument for $_cimag."));

  value *cval = argv[0];
  type *ctype = check_typedef (cval->type ());
  if (ctype->code () != TYPE_CODE_COMPLEX)
    error (_("expected a complex number"));
  return value_imaginary_part (cval);
}

#if GDB_SELF_TEST
namespace selftests
{

/* Test the ranges_contain function.  */

static void
test_ranges_contain ()
{
  std::vector<range> ranges;
  range r;

  /* [10, 14] */
  r.offset = 10;
  r.length = 5;
  ranges.push_back (r);

  /* [20, 24] */
  r.offset = 20;
  r.length = 5;
  ranges.push_back (r);

  /* [2, 6] */
  SELF_CHECK (!ranges_contain (ranges, 2, 5));
  /* [9, 13] */
  SELF_CHECK (ranges_contain (ranges, 9, 5));
  /* [10, 11] */
  SELF_CHECK (ranges_contain (ranges, 10, 2));
  /* [10, 14] */
  SELF_CHECK (ranges_contain (ranges, 10, 5));
  /* [13, 18] */
  SELF_CHECK (ranges_contain (ranges, 13, 6));
  /* [14, 18] */
  SELF_CHECK (ranges_contain (ranges, 14, 5));
  /* [15, 18] */
  SELF_CHECK (!ranges_contain (ranges, 15, 4));
  /* [16, 19] */
  SELF_CHECK (!ranges_contain (ranges, 16, 4));
  /* [16, 21] */
  SELF_CHECK (ranges_contain (ranges, 16, 6));
  /* [21, 21] */
  SELF_CHECK (ranges_contain (ranges, 21, 1));
  /* [21, 25] */
  SELF_CHECK (ranges_contain (ranges, 21, 5));
  /* [26, 28] */
  SELF_CHECK (!ranges_contain (ranges, 26, 3));
}

/* Check that RANGES contains the same ranges as EXPECTED.  */

static bool
check_ranges_vector (gdb::array_view<const range> ranges,
		     gdb::array_view<const range> expected)
{
  return ranges == expected;
}

/* Test the insert_into_bit_range_vector function.  */

static void
test_insert_into_bit_range_vector ()
{
  std::vector<range> ranges;

  /* [10, 14] */
  {
    insert_into_bit_range_vector (&ranges, 10, 5);
    static const range expected[] = {
      {10, 5}
    };
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }

  /* [10, 14] */
  {
    insert_into_bit_range_vector (&ranges, 11, 4);
    static const range expected = {10, 5};
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }

  /* [10, 14] [20, 24] */
  {
    insert_into_bit_range_vector (&ranges, 20, 5);
    static const range expected[] = {
      {10, 5},
      {20, 5},
    };
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }

  /* [10, 14] [17, 24] */
  {
    insert_into_bit_range_vector (&ranges, 17, 5);
    static const range expected[] = {
      {10, 5},
      {17, 8},
    };
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }

  /* [2, 8] [10, 14] [17, 24] */
  {
    insert_into_bit_range_vector (&ranges, 2, 7);
    static const range expected[] = {
      {2, 7},
      {10, 5},
      {17, 8},
    };
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }

  /* [2, 14] [17, 24] */
  {
    insert_into_bit_range_vector (&ranges, 9, 1);
    static const range expected[] = {
      {2, 13},
      {17, 8},
    };
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }

  /* [2, 14] [17, 24] */
  {
    insert_into_bit_range_vector (&ranges, 9, 1);
    static const range expected[] = {
      {2, 13},
      {17, 8},
    };
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }

  /* [2, 33] */
  {
    insert_into_bit_range_vector (&ranges, 4, 30);
    static const range expected = {2, 32};
    SELF_CHECK (check_ranges_vector (ranges, expected));
  }
}

static void
test_value_copy ()
{
  type *type = builtin_type (current_inferior ()->arch ())->builtin_int;

  /* Verify that we can copy an entirely optimized out value, that may not have
     its contents allocated.  */
  value_ref_ptr val = release_value (value::allocate_optimized_out (type));
  value_ref_ptr copy = release_value (val->copy ());

  SELF_CHECK (val->entirely_optimized_out ());
  SELF_CHECK (copy->entirely_optimized_out ());
}

} /* namespace selftests */
#endif /* GDB_SELF_TEST */

void _initialize_values ();
void
_initialize_values ()
{
  cmd_list_element *show_convenience_cmd
    = add_cmd ("convenience", no_class, show_convenience, _("\
Debugger convenience (\"$foo\") variables and functions.\n\
Convenience variables are created when you assign them values;\n\
thus, \"set $foo=1\" gives \"$foo\" the value 1.  Values may be any type.\n\
\n\
A few convenience variables are given values automatically:\n\
\"$_\"holds the last address examined with \"x\" or \"info lines\",\n\
\"$__\" holds the contents of the last address examined with \"x\"."
#ifdef HAVE_PYTHON
"\n\n\
Convenience functions are defined via the Python API."
#endif
	   ), &showlist);
  add_alias_cmd ("conv", show_convenience_cmd, no_class, 1, &showlist);

  add_cmd ("values", no_set_class, show_values, _("\
Elements of value history around item number IDX (or last ten)."),
	   &showlist);

  add_com ("init-if-undefined", class_vars, init_if_undefined_command, _("\
Initialize a convenience variable if necessary.\n\
init-if-undefined VARIABLE = EXPRESSION\n\
Set an internal VARIABLE to the result of the EXPRESSION if it does not\n\
exist or does not contain a value.  The EXPRESSION is not evaluated if the\n\
VARIABLE is already initialized."));

  add_prefix_cmd ("function", no_class, function_command, _("\
Placeholder command for showing help on convenience functions."),
		  &functionlist, 0, &cmdlist);

  add_internal_function ("_isvoid", _("\
Check whether an expression is void.\n\
Usage: $_isvoid (expression)\n\
Return 1 if the expression is void, zero otherwise."),
			 isvoid_internal_fn, NULL);

  add_internal_function ("_creal", _("\
Extract the real part of a complex number.\n\
Usage: $_creal (expression)\n\
Return the real part of a complex number, the type depends on the\n\
type of a complex number."),
			 creal_internal_fn, NULL);

  add_internal_function ("_cimag", _("\
Extract the imaginary part of a complex number.\n\
Usage: $_cimag (expression)\n\
Return the imaginary part of a complex number, the type depends on the\n\
type of a complex number."),
			 cimag_internal_fn, NULL);

  add_setshow_zuinteger_unlimited_cmd ("max-value-size",
				       class_support, &max_value_size, _("\
Set maximum sized value gdb will load from the inferior."), _("\
Show maximum sized value gdb will load from the inferior."), _("\
Use this to control the maximum size, in bytes, of a value that gdb\n\
will load from the inferior.  Setting this value to 'unlimited'\n\
disables checking.\n\
Setting this does not invalidate already allocated values, it only\n\
prevents future values, larger than this size, from being allocated."),
			    set_max_value_size,
			    show_max_value_size,
			    &setlist, &showlist);
  set_show_commands vsize_limit
    = add_setshow_zuinteger_unlimited_cmd ("varsize-limit", class_support,
					   &max_value_size, _("\
Set the maximum number of bytes allowed in a variable-size object."), _("\
Show the maximum number of bytes allowed in a variable-size object."), _("\
Attempts to access an object whose size is not a compile-time constant\n\
and exceeds this limit will cause an error."),
					   NULL, NULL, &setlist, &showlist);
  deprecate_cmd (vsize_limit.set, "set max-value-size");

#if GDB_SELF_TEST
  selftests::register_test ("ranges_contain", selftests::test_ranges_contain);
  selftests::register_test ("insert_into_bit_range_vector",
			    selftests::test_insert_into_bit_range_vector);
  selftests::register_test ("value_copy", selftests::test_value_copy);
#endif
}

/* See value.h.  */

void
finalize_values ()
{
  all_values.clear ();
}
