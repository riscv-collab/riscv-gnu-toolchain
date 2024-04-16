/* This test file is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

/* Return true if address P is ALIGNMENT-byte aligned.  */

static int
is_aligned (void *p, size_t alignment)
{
  size_t mask = (alignment - 1);
  return ((uintptr_t)p & mask) == 0;
}

/* Allocate SIZE memory with ALIGNMENT, and return it.  If FREE_POINTER,
   return in it the corresponding pointer to be passed to free.

   Do the alignment precisely, in other words, if an alignment of 4 is
   requested, make sure the pointer is 4-byte aligned, but not 8-byte
   aligned.  In other words, make sure the pointer is not overaligned.

   The benefit of using precise alignment is that accidentally specifying
   a too low alignment will not be compensated by accidental
   overalignment.  */

static void *
precise_aligned_alloc (size_t alignment, size_t size, void **free_pointer)
{
  /* Allocate extra to compensate for "p += alignment".  */
  size_t alloc_size = size + alignment;

  /* Align extra, to be able to do precise align.  */
  void *p = aligned_alloc (alignment * 2, alloc_size);
  assert (p != NULL);
  void *p_orig = p;
  void *p_end = p + alloc_size;

  /* Make p precisely aligned.  */
  p += alignment;

  /* Verify p is without bounds, and points to large enough area.  */
  assert (p >= p_orig);
  assert (p + size <= p_end);

  /* Verify required alignment.  */
  assert (is_aligned (p, alignment));

  /* Verify required alignment is precise.  */
  assert (! is_aligned (p, 2 * alignment));

  if (free_pointer != NULL)
    *free_pointer = p_orig;

  return p;
}

/* Duplicate data SRC of size SIZE to a newly allocated, precisely aligned
   location with alignment ALIGNMENT.  */

static void *
precise_aligned_dup (size_t alignment, size_t size, void **free_pointer,
		     void *src)
{
  void *p = precise_aligned_alloc (alignment, size, free_pointer);

  memcpy (p, src, size);

  return p;
}
