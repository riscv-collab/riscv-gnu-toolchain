/* Code dealing with dummy stack frames, for GDB, the GNU debugger.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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

#if !defined (DUMMY_FRAME_H)
#define DUMMY_FRAME_H 1

#include "frame.h"

struct infcall_suspend_state;
struct frame_unwind;
class thread_info;

/* Push the information needed to identify, and unwind from, a dummy
   frame onto the dummy frame stack.  */

/* NOTE: cagney/2004-08-02: In the case of ABIs using push_dummy_code
   containing more than one instruction, this interface many need to
   be expanded so that it knowns the lower/upper extent of the dummy
   frame's code.  */

extern void dummy_frame_push (infcall_suspend_state *caller_state,
			      const frame_id *dummy_id,
			      thread_info *thread);

/* Pop the dummy frame DUMMY_ID, restoring program state to that before the
   frame was created.
   On return reinit_frame_cache has been called.
   If the frame isn't found, flag an internal error.

   NOTE: This can only pop the one frame, even if it is in the middle of the
   stack, because the other frames may be for different threads, and there's
   currently no way to tell which stack frame is for which thread.  */

extern void dummy_frame_pop (frame_id dummy_id, thread_info *thread);

extern void dummy_frame_discard (frame_id dummy_id, thread_info *thread);

/* If the PC falls in a dummy frame, return a dummy frame
   unwinder.  */

extern const struct frame_unwind dummy_frame_unwind;

/* Destructor for dummy_frame.  DATA is supplied by registrant.
   REGISTERS_VALID is 1 for dummy_frame_pop, 0 for dummy_frame_discard.  */
typedef void (dummy_frame_dtor_ftype) (void *data, int registers_valid);

/* Call DTOR with DTOR_DATA when DUMMY_ID frame of thread THREAD gets
   discarded.  Dummy frame with DUMMY_ID must exist.  Multiple
   destructors may be registered, they will be called in the reverse
   order of registrations (LIFO).  */
extern void register_dummy_frame_dtor (frame_id dummy_id,
				       thread_info *thread,
				       dummy_frame_dtor_ftype *dtor,
				       void *dtor_data);

/* Return 1 if there exists any dummy frame with any of its registered
   destructors equal to both DTOR and DTOR_DATA.  Return 0 otherwise.  */
extern int find_dummy_frame_dtor (dummy_frame_dtor_ftype *dtor,
				  void *dtor_data);

/* Default implementation of gdbarch_dummy_id.  Generate a dummy frame_id
   for THIS_FRAME assuming that the frame is a dummy frame.  */

extern struct frame_id default_dummy_id (struct gdbarch *gdbarch,
					 frame_info_ptr this_frame);

#endif /* !defined (DUMMY_FRAME_H)  */
