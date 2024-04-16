/* Code dealing with dummy stack frames, for GDB, the GNU debugger.

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
#include "dummy-frame.h"
#include "regcache.h"
#include "frame.h"
#include "inferior.h"
#include "frame-unwind.h"
#include "command.h"
#include "gdbcmd.h"
#include "observable.h"
#include "gdbthread.h"
#include "infcall.h"
#include "gdbarch.h"

struct dummy_frame_id
{
  /* This frame's ID.  Must match the value returned by
     gdbarch_dummy_id.  */
  struct frame_id id;

  /* The thread this dummy_frame relates to.  */
  thread_info *thread;
};

/* Return whether dummy_frame_id *ID1 and *ID2 are equal.  */

static int
dummy_frame_id_eq (struct dummy_frame_id *id1,
		   struct dummy_frame_id *id2)
{
  return id1->id == id2->id && id1->thread == id2->thread;
}

/* List of dummy_frame destructors.  */

struct dummy_frame_dtor_list
{
  /* Next element in the list or NULL if this is the last element.  */
  struct dummy_frame_dtor_list *next;

  /* If non-NULL, a destructor that is run when this dummy frame is freed.  */
  dummy_frame_dtor_ftype *dtor;

  /* Arbitrary data that is passed to DTOR.  */
  void *dtor_data;
};

/* Dummy frame.  This saves the processor state just prior to setting
   up the inferior function call.  Older targets save the registers
   on the target stack (but that really slows down function calls).  */

struct dummy_frame
{
  struct dummy_frame *next;

  /* An id represents a dummy frame.  */
  struct dummy_frame_id id;

  /* The caller's state prior to the call.  */
  struct infcall_suspend_state *caller_state;

  /* First element of destructors list or NULL if there are no
     destructors registered for this dummy_frame.  */
  struct dummy_frame_dtor_list *dtor_list;
};

static struct dummy_frame *dummy_frame_stack = NULL;

/* Push the caller's state, along with the dummy frame info, onto the
   dummy-frame stack.  */

void
dummy_frame_push (struct infcall_suspend_state *caller_state,
		  const frame_id *dummy_id, thread_info *thread)
{
  struct dummy_frame *dummy_frame;

  dummy_frame = XCNEW (struct dummy_frame);
  dummy_frame->caller_state = caller_state;
  dummy_frame->id.id = (*dummy_id);
  dummy_frame->id.thread = thread;
  dummy_frame->next = dummy_frame_stack;
  dummy_frame_stack = dummy_frame;
}

/* Remove *DUMMY_PTR from the dummy frame stack.  */

static void
remove_dummy_frame (struct dummy_frame **dummy_ptr)
{
  struct dummy_frame *dummy = *dummy_ptr;

  while (dummy->dtor_list != NULL)
    {
      struct dummy_frame_dtor_list *list = dummy->dtor_list;

      dummy->dtor_list = list->next;
      list->dtor (list->dtor_data, 0);
      xfree (list);
    }

  *dummy_ptr = dummy->next;
  discard_infcall_suspend_state (dummy->caller_state);
  xfree (dummy);
}

/* Delete any breakpoint B which is a momentary breakpoint for return from
   inferior call matching DUMMY_VOIDP.  */

static bool
pop_dummy_frame_bpt (struct breakpoint *b, struct dummy_frame *dummy)
{
  if (b->thread == dummy->id.thread->global_num
      && b->disposition == disp_del && b->frame_id == dummy->id.id)
    {
      while (b->related_breakpoint != b)
	delete_breakpoint (b->related_breakpoint);

      delete_breakpoint (b);

      /* Stop the traversal.  */
      return true;
    }

  /* Continue the traversal.  */
  return false;
}

/* Pop *DUMMY_PTR, restoring program state to that before the
   frame was created.  */

static void
pop_dummy_frame (struct dummy_frame **dummy_ptr)
{
  struct dummy_frame *dummy = *dummy_ptr;

  gdb_assert (dummy->id.thread == inferior_thread ());

  while (dummy->dtor_list != NULL)
    {
      struct dummy_frame_dtor_list *list = dummy->dtor_list;

      dummy->dtor_list = list->next;
      list->dtor (list->dtor_data, 1);
      xfree (list);
    }

  restore_infcall_suspend_state (dummy->caller_state);

  for (breakpoint &bp : all_breakpoints_safe ())
    if (pop_dummy_frame_bpt (&bp, dummy))
      break;

  /* restore_infcall_control_state frees inf_state,
     all that remains is to pop *dummy_ptr.  */
  *dummy_ptr = dummy->next;
  xfree (dummy);

  /* We've made right mess of GDB's local state, just discard
     everything.  */
  reinit_frame_cache ();
}

/* Look up DUMMY_ID.
   Return NULL if not found.  */

static struct dummy_frame **
lookup_dummy_frame (struct dummy_frame_id *dummy_id)
{
  struct dummy_frame **dp;

  for (dp = &dummy_frame_stack; *dp != NULL; dp = &(*dp)->next)
    {
      if (dummy_frame_id_eq (&(*dp)->id, dummy_id))
	return dp;
    }

  return NULL;
}

/* Find the dummy frame by DUMMY_ID and THREAD, and pop it, restoring
   program state to that before the frame was created.
   On return reinit_frame_cache has been called.
   If the frame isn't found, flag an internal error.  */

void
dummy_frame_pop (frame_id dummy_id, thread_info *thread)
{
  struct dummy_frame **dp;
  struct dummy_frame_id id = { dummy_id, thread };

  dp = lookup_dummy_frame (&id);
  gdb_assert (dp != NULL);

  pop_dummy_frame (dp);
}

/* Find the dummy frame by DUMMY_ID and PTID and drop it.  Do nothing
   if it is not found.  Do not restore its state into inferior, just
   free its memory.  */

void
dummy_frame_discard (struct frame_id dummy_id, thread_info *thread)
{
  struct dummy_frame **dp;
  struct dummy_frame_id id = { dummy_id, thread };

  dp = lookup_dummy_frame (&id);
  if (dp)
    remove_dummy_frame (dp);
}

/* See dummy-frame.h.  */

void
register_dummy_frame_dtor (frame_id dummy_id, thread_info *thread,
			   dummy_frame_dtor_ftype *dtor, void *dtor_data)
{
  struct dummy_frame_id id = { dummy_id, thread };
  struct dummy_frame **dp, *d;
  struct dummy_frame_dtor_list *list;

  dp = lookup_dummy_frame (&id);
  gdb_assert (dp != NULL);
  d = *dp;
  list = XNEW (struct dummy_frame_dtor_list);
  list->next = d->dtor_list;
  d->dtor_list = list;
  list->dtor = dtor;
  list->dtor_data = dtor_data;
}

/* See dummy-frame.h.  */

int
find_dummy_frame_dtor (dummy_frame_dtor_ftype *dtor, void *dtor_data)
{
  struct dummy_frame *d;

  for (d = dummy_frame_stack; d != NULL; d = d->next)
    {
      struct dummy_frame_dtor_list *list;

      for (list = d->dtor_list; list != NULL; list = list->next)
	if (list->dtor == dtor && list->dtor_data == dtor_data)
	  return 1;
    }
  return 0;
}

/* There may be stale dummy frames, perhaps left over from when an uncaught
   longjmp took us out of a function that was called by the debugger.  Clean
   them up at least once whenever we start a new inferior.  */

static void
cleanup_dummy_frames (inferior *inf)
{
  while (dummy_frame_stack != NULL)
    remove_dummy_frame (&dummy_frame_stack);
}

/* Return the dummy frame cache, it contains both the ID, and a
   pointer to the regcache.  */
struct dummy_frame_cache
{
  struct frame_id this_id;
  readonly_detached_regcache *prev_regcache;
};

static int
dummy_frame_sniffer (const struct frame_unwind *self,
		     frame_info_ptr this_frame,
		     void **this_prologue_cache)
{
  /* When unwinding a normal frame, the stack structure is determined
     by analyzing the frame's function's code (be it using brute force
     prologue analysis, or the dwarf2 CFI).  In the case of a dummy
     frame, that simply isn't possible.  The PC is either the program
     entry point, or some random address on the stack.  Trying to use
     that PC to apply standard frame ID unwind techniques is just
     asking for trouble.  */
  
  /* Don't bother unless there is at least one dummy frame.  */
  if (dummy_frame_stack != NULL)
    {
      struct dummy_frame *dummyframe;
      /* Use an architecture specific method to extract this frame's
	 dummy ID, assuming it is a dummy frame.  */
      struct frame_id this_id
	= gdbarch_dummy_id (get_frame_arch (this_frame), this_frame);
      struct dummy_frame_id dummy_id = { this_id, inferior_thread () };

      /* Use that ID to find the corresponding cache entry.  */
      for (dummyframe = dummy_frame_stack;
	   dummyframe != NULL;
	   dummyframe = dummyframe->next)
	{
	  if (dummy_frame_id_eq (&dummyframe->id, &dummy_id))
	    {
	      struct dummy_frame_cache *cache;

	      cache = FRAME_OBSTACK_ZALLOC (struct dummy_frame_cache);
	      cache->prev_regcache = get_infcall_suspend_state_regcache
						   (dummyframe->caller_state);
	      cache->this_id = this_id;
	      (*this_prologue_cache) = cache;
	      return 1;
	    }
	}
    }
  return 0;
}

/* Given a call-dummy dummy-frame, return the registers.  Here the
   register value is taken from the local copy of the register buffer.  */

static struct value *
dummy_frame_prev_register (frame_info_ptr this_frame,
			   void **this_prologue_cache,
			   int regnum)
{
  struct dummy_frame_cache *cache
    = (struct dummy_frame_cache *) *this_prologue_cache;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct value *reg_val;

  /* The dummy-frame sniffer always fills in the cache.  */
  gdb_assert (cache != NULL);

  /* Describe the register's location.  Generic dummy frames always
     have the register value in an ``expression''.  */
  reg_val = value::zero (register_type (gdbarch, regnum), not_lval);

  /* Use the regcache_cooked_read() method so that it, on the fly,
     constructs either a raw or pseudo register from the raw
     register cache.  */
  cache->prev_regcache->cooked_read
    (regnum, reg_val->contents_writeable ().data ());
  return reg_val;
}

/* Assuming that THIS_FRAME is a dummy, return its ID.  That ID is
   determined by examining the NEXT frame's unwound registers using
   the method dummy_id().  As a side effect, THIS dummy frame's
   dummy cache is located and saved in THIS_PROLOGUE_CACHE.  */

static void
dummy_frame_this_id (frame_info_ptr this_frame,
		     void **this_prologue_cache,
		     struct frame_id *this_id)
{
  /* The dummy-frame sniffer always fills in the cache.  */
  struct dummy_frame_cache *cache
    = (struct dummy_frame_cache *) *this_prologue_cache;

  gdb_assert (cache != NULL);
  (*this_id) = cache->this_id;
}

const struct frame_unwind dummy_frame_unwind =
{
  "dummy",
  DUMMY_FRAME,
  default_frame_unwind_stop_reason,
  dummy_frame_this_id,
  dummy_frame_prev_register,
  NULL,
  dummy_frame_sniffer,
};

/* See dummy-frame.h.  */

struct frame_id
default_dummy_id (struct gdbarch *gdbarch, frame_info_ptr this_frame)
{
  CORE_ADDR sp, pc;

  sp = get_frame_sp (this_frame);
  pc = get_frame_pc (this_frame);
  return frame_id_build (sp, pc);
}

static void
fprint_dummy_frames (struct ui_file *file)
{
  struct dummy_frame *s;

  for (s = dummy_frame_stack; s != NULL; s = s->next)
    gdb_printf (file, "%s: id=%s, ptid=%s\n",
		host_address_to_string (s),
		s->id.id.to_string ().c_str (),
		s->id.thread->ptid.to_string ().c_str ());
}

static void
maintenance_print_dummy_frames (const char *args, int from_tty)
{
  if (args == NULL)
    fprint_dummy_frames (gdb_stdout);
  else
    {
      stdio_file file;

      if (!file.open (args, "w"))
	perror_with_name (_("maintenance print dummy-frames"));
      fprint_dummy_frames (&file);
    }
}

void _initialize_dummy_frame ();
void
_initialize_dummy_frame ()
{
  add_cmd ("dummy-frames", class_maintenance, maintenance_print_dummy_frames,
	   _("Print the contents of the internal dummy-frame stack."),
	   &maintenanceprintlist);

  gdb::observers::inferior_created.attach (cleanup_dummy_frames, "dummy-frame");
}
