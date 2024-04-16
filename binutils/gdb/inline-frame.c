/* Inline frame unwinder for GDB.

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

#include "defs.h"
#include "breakpoint.h"
#include "inline-frame.h"
#include "addrmap.h"
#include "block.h"
#include "frame-unwind.h"
#include "inferior.h"
#include "gdbthread.h"
#include "regcache.h"
#include "symtab.h"
#include "frame.h"
#include <algorithm>

/* We need to save a few variables for every thread stopped at the
   virtual call site of an inlined function.  If there was always a
   "struct thread_info", we could hang it off that; in the mean time,
   keep our own list.  */
struct inline_state
{
  inline_state (thread_info *thread_, int skipped_frames_, CORE_ADDR saved_pc_,
		std::vector<symbol *> &&skipped_symbols_)
    : thread (thread_), skipped_frames (skipped_frames_), saved_pc (saved_pc_),
      skipped_symbols (std::move (skipped_symbols_))
  {}

  /* The thread this data relates to.  It should be a currently
     stopped thread.  */
  thread_info *thread;

  /* The number of inlined functions we are skipping.  Each of these
     functions can be stepped in to.  */
  int skipped_frames;

  /* Only valid if SKIPPED_FRAMES is non-zero.  This is the PC used
     when calculating SKIPPED_FRAMES; used to check whether we have
     moved to a new location by user request.  If so, we invalidate
     any skipped frames.  */
  CORE_ADDR saved_pc;

  /* Only valid if SKIPPED_FRAMES is non-zero.  This is the list of all
     function symbols that have been skipped, from inner most to outer
     most.  It is used to find the call site of the current frame.  */
  std::vector<struct symbol *> skipped_symbols;
};

static std::vector<inline_state> inline_states;

/* Locate saved inlined frame state for THREAD, if it exists and is
   valid.  */

static struct inline_state *
find_inline_frame_state (thread_info *thread)
{
  auto state_it = std::find_if (inline_states.begin (), inline_states.end (),
				[thread] (const inline_state &state)
				  {
				    return state.thread == thread;
				  });

  if (state_it == inline_states.end ())
    return nullptr;

  inline_state &state = *state_it;
  struct regcache *regcache = get_thread_regcache (thread);
  CORE_ADDR current_pc = regcache_read_pc (regcache);

  if (current_pc != state.saved_pc)
    {
      /* PC has changed - this context is invalid.  Use the
	 default behavior.  */

      unordered_remove (inline_states, state_it);
      return nullptr;
    }

  return &state;
}

/* See inline-frame.h.  */

void
clear_inline_frame_state (process_stratum_target *target, ptid_t filter_ptid)
{
  gdb_assert (target != NULL);

  if (filter_ptid == minus_one_ptid || filter_ptid.is_pid ())
    {
      auto matcher = [target, &filter_ptid] (const inline_state &state)
	{
	  thread_info *t = state.thread;
	  return (t->inf->process_target () == target
		  && t->ptid.matches (filter_ptid));
	};

      auto it = std::remove_if (inline_states.begin (), inline_states.end (),
				matcher);

      inline_states.erase (it, inline_states.end ());

      return;
    }


  auto matcher = [target, &filter_ptid] (const inline_state &state)
    {
      thread_info *t = state.thread;
      return (t->inf->process_target () == target
	      && filter_ptid == t->ptid);
    };

  auto it = std::find_if (inline_states.begin (), inline_states.end (),
			  matcher);

  if (it != inline_states.end ())
    unordered_remove (inline_states, it);
}

/* See inline-frame.h.  */

void
clear_inline_frame_state (thread_info *thread)
{
  auto it = std::find_if (inline_states.begin (), inline_states.end (),
			  [thread] (const inline_state &state)
			    {
			      return thread == state.thread;
			    });

  if (it != inline_states.end ())
    unordered_remove (inline_states, it);
}

static void
inline_frame_this_id (frame_info_ptr this_frame,
		      void **this_cache,
		      struct frame_id *this_id)
{
  struct symbol *func;

  /* In order to have a stable frame ID for a given inline function,
     we must get the stack / special addresses from the underlying
     real frame's this_id method.  So we must call
     get_prev_frame_always.  Because we are inlined into some
     function, there must be previous frames, so this is safe - as
     long as we're careful not to create any cycles.  See related
     comments in get_prev_frame_always_1.  */
  frame_info_ptr prev_frame = get_prev_frame_always (this_frame);
  if (prev_frame == nullptr)
    error (_("failed to find previous frame when computing inline frame id"));
  *this_id = get_frame_id (prev_frame);

  /* We need a valid frame ID, so we need to be based on a valid
     frame.  FSF submission NOTE: this would be a good assertion to
     apply to all frames, all the time.  That would fix the ambiguity
     of null_frame_id (between "no/any frame" and "the outermost
     frame").  This will take work.  */
  gdb_assert (frame_id_p (*this_id));

  /* Future work NOTE: Alexandre Oliva applied a patch to GCC 4.3
     which generates DW_AT_entry_pc for inlined functions when
     possible.  If this attribute is available, we should use it
     in the frame ID (and eventually, to set breakpoints).  */
  func = get_frame_function (this_frame);
  gdb_assert (func != NULL);
  (*this_id).code_addr = func->value_block ()->entry_pc ();
  (*this_id).artificial_depth++;
}

static struct value *
inline_frame_prev_register (frame_info_ptr this_frame, void **this_cache,
			    int regnum)
{
  /* Use get_frame_register_value instead of
     frame_unwind_got_register, to avoid requiring this frame's ID.
     This frame's ID depends on the previous frame's ID (unusual), and
     the previous frame's ID depends on this frame's unwound
     registers.  If unwinding registers from this frame called
     get_frame_id, there would be a loop.

     Do not copy this code into any other unwinder!  Inlined functions
     are special; other unwinders must not have a dependency on the
     previous frame's ID, and therefore can and should use
     frame_unwind_got_register instead.  */
  return get_frame_register_value (this_frame, regnum);
}

/* Check whether we are at an inlining site that does not already
   have an associated frame.  */

static int
inline_frame_sniffer (const struct frame_unwind *self,
		      frame_info_ptr this_frame,
		      void **this_cache)
{
  CORE_ADDR this_pc;
  const struct block *frame_block, *cur_block;
  int depth;
  frame_info_ptr next_frame;
  struct inline_state *state = find_inline_frame_state (inferior_thread ());

  this_pc = get_frame_address_in_block (this_frame);
  frame_block = block_for_pc (this_pc);
  if (frame_block == NULL)
    return 0;

  /* Calculate DEPTH, the number of inlined functions at this
     location.  */
  depth = 0;
  cur_block = frame_block;
  while (cur_block->superblock ())
    {
      if (cur_block->inlined_p ())
	depth++;
      else if (cur_block->function () != NULL)
	break;

      cur_block = cur_block->superblock ();
    }

  /* Check how many inlined functions already have frames.  */
  for (next_frame = get_next_frame (this_frame);
       next_frame && get_frame_type (next_frame) == INLINE_FRAME;
       next_frame = get_next_frame (next_frame))
    {
      gdb_assert (depth > 0);
      depth--;
    }

  /* If this is the topmost frame, or all frames above us are inlined,
     then check whether we were requested to skip some frames (so they
     can be stepped into later).  */
  if (state != NULL && state->skipped_frames > 0 && next_frame == NULL)
    {
      gdb_assert (depth >= state->skipped_frames);
      depth -= state->skipped_frames;
    }

  /* If all the inlined functions here already have frames, then pass
     to the normal unwinder for this PC.  */
  if (depth == 0)
    return 0;

  /* If the next frame is an inlined function, but not the outermost, then
     we are the next outer.  If it is not an inlined function, then we
     are the innermost inlined function of a different real frame.  */
  return 1;
}

const struct frame_unwind inline_frame_unwind = {
  "inline",
  INLINE_FRAME,
  default_frame_unwind_stop_reason,
  inline_frame_this_id,
  inline_frame_prev_register,
  NULL,
  inline_frame_sniffer
};

/* Return non-zero if BLOCK, an inlined function block containing PC,
   has a group of contiguous instructions starting at PC (but not
   before it).  */

static int
block_starting_point_at (CORE_ADDR pc, const struct block *block)
{
  const struct blockvector *bv;
  const struct block *new_block;

  bv = blockvector_for_pc (pc, NULL);
  if (bv->map () == nullptr)
    return 0;

  new_block = (const struct block *) bv->map ()->find (pc - 1);
  if (new_block == NULL)
    return 1;

  if (new_block == block || block->contains (new_block))
    return 0;

  /* The immediately preceding address belongs to a different block,
     which is not a child of this one.  Treat this as an entrance into
     BLOCK.  */
  return 1;
}

/* Loop over the stop chain and determine if execution stopped in an
   inlined frame because of a breakpoint with a user-specified location
   set at FRAME_BLOCK.  */

static bool
stopped_by_user_bp_inline_frame (const block *frame_block, bpstat *stop_chain)
{
  for (bpstat *s = stop_chain; s != nullptr; s = s->next)
    {
      struct breakpoint *bpt = s->breakpoint_at;

      if (bpt != NULL
	  && (user_breakpoint_p (bpt) || bpt->type == bp_until))
	{
	  bp_location *loc = s->bp_location_at.get ();
	  enum bp_loc_type t = loc->loc_type;

	  if (t == bp_loc_software_breakpoint
	      || t == bp_loc_hardware_breakpoint)
	    {
	      /* If the location has a function symbol, check whether
		 the frame was for that inlined function.  If it has
		 no function symbol, then assume it is.  I.e., default
		 to presenting the stop at the innermost inline
		 function.  */
	      if (loc->symbol == nullptr
		  || frame_block == loc->symbol->value_block ())
		return true;
	    }
	}
    }

  return false;
}

/* See inline-frame.h.  */

void
skip_inline_frames (thread_info *thread, bpstat *stop_chain)
{
  const struct block *frame_block, *cur_block;
  std::vector<struct symbol *> skipped_syms;
  int skip_count = 0;

  /* This function is called right after reinitializing the frame
     cache.  We try not to do more unwinding than absolutely
     necessary, for performance.  */
  CORE_ADDR this_pc = get_frame_pc (get_current_frame ());
  frame_block = block_for_pc (this_pc);

  if (frame_block != NULL)
    {
      cur_block = frame_block;
      while (cur_block->superblock ())
	{
	  if (cur_block->inlined_p ())
	    {
	      /* See comments in inline_frame_this_id about this use
		 of BLOCK_ENTRY_PC.  */
	      if (cur_block->entry_pc () == this_pc
		  || block_starting_point_at (this_pc, cur_block))
		{
		  /* Do not skip the inlined frame if execution
		     stopped in an inlined frame because of a user
		     breakpoint for this inline function.  */
		  if (stopped_by_user_bp_inline_frame (cur_block, stop_chain))
		    break;

		  skip_count++;
		  skipped_syms.push_back (cur_block->function ());
		}
	      else
		break;
	    }
	  else if (cur_block->function () != NULL)
	    break;

	  cur_block = cur_block->superblock ();
	}
    }

  gdb_assert (find_inline_frame_state (thread) == NULL);
  inline_states.emplace_back (thread, skip_count, this_pc,
			      std::move (skipped_syms));

  if (skip_count != 0)
    reinit_frame_cache ();
}

/* Step into an inlined function by unhiding it.  */

void
step_into_inline_frame (thread_info *thread)
{
  inline_state *state = find_inline_frame_state (thread);

  gdb_assert (state != NULL && state->skipped_frames > 0);
  state->skipped_frames--;
  reinit_frame_cache ();
}

/* Return the number of hidden functions inlined into the current
   frame.  */

int
inline_skipped_frames (thread_info *thread)
{
  inline_state *state = find_inline_frame_state (thread);

  if (state == NULL)
    return 0;
  else
    return state->skipped_frames;
}

/* If one or more inlined functions are hidden, return the symbol for
   the function inlined into the current frame.  */

struct symbol *
inline_skipped_symbol (thread_info *thread)
{
  inline_state *state = find_inline_frame_state (thread);
  gdb_assert (state != NULL);

  /* This should only be called when we are skipping at least one frame,
     hence SKIPPED_FRAMES will be greater than zero when we get here.
     We initialise SKIPPED_FRAMES at the same time as we build
     SKIPPED_SYMBOLS, hence it should be true that SKIPPED_FRAMES never
     indexes outside of the SKIPPED_SYMBOLS vector.  */
  gdb_assert (state->skipped_frames > 0);
  gdb_assert (state->skipped_frames <= state->skipped_symbols.size ());
  return state->skipped_symbols[state->skipped_frames - 1];
}

/* Return the number of functions inlined into THIS_FRAME.  Some of
   the callees may not have associated frames (see
   skip_inline_frames).  */

int
frame_inlined_callees (frame_info_ptr this_frame)
{
  frame_info_ptr next_frame;
  int inline_count = 0;

  /* First count how many inlined functions at this PC have frames
     above FRAME (are inlined into FRAME).  */
  for (next_frame = get_next_frame (this_frame);
       next_frame && get_frame_type (next_frame) == INLINE_FRAME;
       next_frame = get_next_frame (next_frame))
    inline_count++;

  /* Simulate some most-inner inlined frames which were suppressed, so
     they can be stepped into later.  If we are unwinding already
     outer frames from some non-inlined frame this does not apply.  */
  if (next_frame == NULL)
    inline_count += inline_skipped_frames (inferior_thread ());

  return inline_count;
}
