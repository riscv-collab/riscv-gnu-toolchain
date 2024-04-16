/* This test program is part of GDB, the GNU debugger.

   Copyright 2015-2024 Free Software Foundation, Inc.

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

/* This is the test program loaded into GDB by the py-unwind test.  */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void *
swap_value (void **location, void *new_value)
{
  void *old_value = *location;
  *location = new_value;
  return old_value;
}

static void
bad_layout(void **variable_ptr, void *fp)
{
  fprintf (stderr, "First variable should be allocated one word below "
           "the frame.  Got variable's address %p, frame at %p instead.\n",
           variable_ptr, fp);
  abort();
}

#define MY_FRAME (__builtin_frame_address (0))

static void
corrupt_frame_inner (void)
{
  /* Save outer frame address, then corrupt the unwind chain by
     setting the outer frame address in it to self.  This is
     ABI-specific: the first word of the frame contains previous frame
     address in amd64.  */
  void *previous_fp = swap_value ((void **) MY_FRAME, MY_FRAME);

  /* Verify the compiler allocates the first local variable one word
     below frame.  This is where the test unwinder expects to find the
     correct outer frame address.  */
  if (&previous_fp + 1 != (void **) MY_FRAME)
    bad_layout (&previous_fp + 1, MY_FRAME);

  /* Now restore it so that we can return.  The test sets the
     breakpoint just before this happens, and GDB will not be able to
     show the backtrace without JIT reader.  */
  swap_value ((void **) MY_FRAME, previous_fp); /* break backtrace-broken */
}

static void
corrupt_frame_outer (void)
{
  /* See above for the explanation of the code here.  This function
     corrupts its frame, too, and then calls the inner one.  */
  void *previous_fp = swap_value ((void **) MY_FRAME, MY_FRAME);
  if (&previous_fp + 1 != (void **) MY_FRAME)
    bad_layout (&previous_fp, MY_FRAME);
  corrupt_frame_inner ();
  swap_value ((void **) MY_FRAME, previous_fp);
}

int
main ()
{
  corrupt_frame_outer ();
  return 0;
}
