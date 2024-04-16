/* Definitions for inline frame support.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

#if !defined (INLINE_FRAME_H)
#define INLINE_FRAME_H 1

class frame_info_ptr;
struct frame_unwind;
struct bpstat;
struct process_stratum_target;

/* The inline frame unwinder.  */

extern const struct frame_unwind inline_frame_unwind;

/* Skip all inlined functions whose call sites are at the current PC.

   If non-NULL, STOP_CHAIN is used to determine whether a stop was caused by
   a user breakpoint.  In that case, do not skip that inlined frame.  This
   allows the inlined frame to be treated as if it were non-inlined from the
   user's perspective.  GDB will stop "in" the inlined frame instead of
   the caller.  */

void skip_inline_frames (thread_info *thread, struct bpstat *stop_chain);

/* Forget about any hidden inlined functions in PTID, which is new or
   about to be resumed.  PTID may be minus_one_ptid (all processes of
   TARGET) or a PID (all threads in this process of TARGET).  */

void clear_inline_frame_state (process_stratum_target *target, ptid_t ptid);

/* Forget about any hidden inlined functions in THREAD, which is new
   or about to be resumed.  */

void clear_inline_frame_state (thread_info *thread);

/* Step into an inlined function by unhiding it.  */

void step_into_inline_frame (thread_info *thread);

/* Return the number of hidden functions inlined into the current
   frame.  */

int inline_skipped_frames (thread_info *thread);

/* If one or more inlined functions are hidden, return the symbol for
   the function inlined into the current frame.  */

struct symbol *inline_skipped_symbol (thread_info *thread);

/* Return the number of functions inlined into THIS_FRAME.  Some of
   the callees may not have associated frames (see
   skip_inline_frames).  */

int frame_inlined_callees (frame_info_ptr this_frame);

#endif /* !defined (INLINE_FRAME_H) */
