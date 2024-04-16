/* Displaced stepping related things.

   Copyright (C) 2020-2024 Free Software Foundation, Inc.

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
#include "displaced-stepping.h"

#include "cli/cli-cmds.h"
#include "command.h"
#include "gdbarch.h"
#include "gdbcore.h"
#include "gdbthread.h"
#include "inferior.h"
#include "regcache.h"
#include "target/target.h"

/* Default destructor for displaced_step_copy_insn_closure.  */

displaced_step_copy_insn_closure::~displaced_step_copy_insn_closure ()
  = default;

bool debug_displaced = false;

static void
show_debug_displaced (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Displace stepping debugging is %s.\n"), value);
}

displaced_step_prepare_status
displaced_step_buffers::prepare (thread_info *thread, CORE_ADDR &displaced_pc)
{
  gdb_assert (!thread->displaced_step_state.in_progress ());

  /* Sanity check: the thread should not be using a buffer at this point.  */
  for (displaced_step_buffer &buf : m_buffers)
    gdb_assert (buf.current_thread != thread);

  regcache *regcache = get_thread_regcache (thread);
  gdbarch *arch = regcache->arch ();
  ULONGEST len = gdbarch_displaced_step_buffer_length (arch);

  /* Search for an unused buffer.  */
  displaced_step_buffer *buffer = nullptr;
  displaced_step_prepare_status fail_status
    = DISPLACED_STEP_PREPARE_STATUS_CANT;

  for (displaced_step_buffer &candidate : m_buffers)
    {
      bool bp_in_range = breakpoint_in_range_p (thread->inf->aspace.get (),
						candidate.addr, len);
      bool is_free = candidate.current_thread == nullptr;

      if (!bp_in_range)
	{
	  if (is_free)
	    {
	      buffer = &candidate;
	      break;
	    }
	  else
	    {
	      /* This buffer would be suitable, but it's used right now.  */
	      fail_status = DISPLACED_STEP_PREPARE_STATUS_UNAVAILABLE;
	    }
	}
      else
	{
	  /* There's a breakpoint set in the scratch pad location range
	     (which is usually around the entry point).  We'd either
	     install it before resuming, which would overwrite/corrupt the
	     scratch pad, or if it was already inserted, this displaced
	     step would overwrite it.  The latter is OK in the sense that
	     we already assume that no thread is going to execute the code
	     in the scratch pad range (after initial startup) anyway, but
	     the former is unacceptable.  Simply punt and fallback to
	     stepping over this breakpoint in-line.  */
	  displaced_debug_printf ("breakpoint set in displaced stepping "
				  "buffer at %s, can't use.",
				  paddress (arch, candidate.addr));
	}
    }

  if (buffer == nullptr)
    return fail_status;

  displaced_debug_printf ("selected buffer at %s",
			  paddress (arch, buffer->addr));

  /* Save the original PC of the thread.  */
  buffer->original_pc = regcache_read_pc (regcache);

  /* Return displaced step buffer address to caller.  */
  displaced_pc = buffer->addr;

  /* Save the original contents of the displaced stepping buffer.  */
  buffer->saved_copy.resize (len);

  int status = target_read_memory (buffer->addr,
				    buffer->saved_copy.data (), len);
  if (status != 0)
    throw_error (MEMORY_ERROR,
		 _("Error accessing memory address %s (%s) for "
		   "displaced-stepping scratch space."),
		 paddress (arch, buffer->addr), safe_strerror (status));

  displaced_debug_printf ("saved %s: %s",
			  paddress (arch, buffer->addr),
			  bytes_to_string (buffer->saved_copy).c_str ());

  /* Save this in a local variable first, so it's released if code below
     throws.  */
  displaced_step_copy_insn_closure_up copy_insn_closure
    = gdbarch_displaced_step_copy_insn (arch, buffer->original_pc,
					buffer->addr, regcache);

  if (copy_insn_closure == nullptr)
    {
      /* The architecture doesn't know how or want to displaced step
	 this instruction or instruction sequence.  Fallback to
	 stepping over the breakpoint in-line.  */
      return DISPLACED_STEP_PREPARE_STATUS_CANT;
    }

  /* This marks the buffer as being in use.  */
  buffer->current_thread = thread;

  /* Save this, now that we know everything went fine.  */
  buffer->copy_insn_closure = std::move (copy_insn_closure);

  /* Reset the displaced step buffer state if we failed to write PC.
     Otherwise we will prevent this buffer from being used, as it will
     always have a thread in buffer->current_thread.  */
  auto reset_buffer = make_scope_exit
    ([buffer] ()
      {
	buffer->current_thread = nullptr;
	buffer->copy_insn_closure.reset ();
      });

  /* Adjust the PC so it points to the displaced step buffer address that will
     be used.  This needs to be done after we save the copy_insn_closure, as
     some architectures (Arm, for one) need that information so they can adjust
     other data as needed.  In particular, Arm needs to know if the instruction
     being executed in the displaced step buffer is thumb or not.  Without that
     information, things will be very wrong in a random way.  */
  regcache_write_pc (regcache, buffer->addr);

  /* PC update successful.  Discard the displaced step state rollback.  */
  reset_buffer.release ();

  /* Tell infrun not to try preparing a displaced step again for this inferior if
     all buffers are taken.  */
  thread->inf->displaced_step_state.unavailable = true;
  for (const displaced_step_buffer &buf : m_buffers)
    {
      if (buf.current_thread == nullptr)
	{
	  thread->inf->displaced_step_state.unavailable = false;
	  break;
	}
    }

  return DISPLACED_STEP_PREPARE_STATUS_OK;
}

static void
write_memory_ptid (ptid_t ptid, CORE_ADDR memaddr,
		   const gdb_byte *myaddr, int len)
{
  scoped_restore save_inferior_ptid = make_scoped_restore (&inferior_ptid);

  inferior_ptid = ptid;
  write_memory (memaddr, myaddr, len);
}

static bool
displaced_step_instruction_executed_successfully
  (gdbarch *arch, const target_waitstatus &status)
{
  if (status.kind () == TARGET_WAITKIND_STOPPED
      && status.sig () != GDB_SIGNAL_TRAP)
    return false;

  /* All other (thread event) waitkinds can only happen if the
     instruction fully executed.  For example, a fork, or a syscall
     entry can only happen if the syscall instruction actually
     executed.  */

  if (target_stopped_by_watchpoint ())
    {
      if (gdbarch_have_nonsteppable_watchpoint (arch)
	  || target_have_steppable_watchpoint ())
	return false;
    }

  return true;
}

displaced_step_finish_status
displaced_step_buffers::finish (gdbarch *arch, thread_info *thread,
				const target_waitstatus &status)
{
  gdb_assert (thread->displaced_step_state.in_progress ());

  /* Find the buffer this thread was using.  */
  displaced_step_buffer *buffer = nullptr;

  for (displaced_step_buffer &candidate : m_buffers)
    {
      if (candidate.current_thread == thread)
	{
	  buffer = &candidate;
	  break;
	}
    }

  gdb_assert (buffer != nullptr);

  /* Move this to a local variable so it's released in case something goes
     wrong.  */
  displaced_step_copy_insn_closure_up copy_insn_closure
    = std::move (buffer->copy_insn_closure);
  gdb_assert (copy_insn_closure != nullptr);

  /* Reset BUFFER->CURRENT_THREAD immediately to mark the buffer as available,
     in case something goes wrong below.  */
  buffer->current_thread = nullptr;

  /* Now that a buffer gets freed, tell infrun it can ask us to prepare a displaced
     step again for this inferior.  Do that here in case something goes wrong
     below.  */
  thread->inf->displaced_step_state.unavailable = false;

  ULONGEST len = gdbarch_displaced_step_buffer_length (arch);

  /* Restore memory of the buffer.  */
  write_memory_ptid (thread->ptid, buffer->addr,
		     buffer->saved_copy.data (), len);

  displaced_debug_printf ("restored %s %s",
			  thread->ptid.to_string ().c_str (),
			  paddress (arch, buffer->addr));

  /* If the thread exited while stepping, we are done.  The code above
     made the buffer available again, and we restored the bytes in the
     buffer.  We don't want to run the fixup: since the thread is now
     dead there's nothing to adjust.  */
  if (status.kind () == TARGET_WAITKIND_THREAD_EXITED)
    return DISPLACED_STEP_FINISH_STATUS_OK;

  regcache *rc = get_thread_regcache (thread);

  bool instruction_executed_successfully
    = displaced_step_instruction_executed_successfully (arch, status);

  gdbarch_displaced_step_fixup (arch, copy_insn_closure.get (),
				buffer->original_pc, buffer->addr,
				rc, instruction_executed_successfully);

  return (instruction_executed_successfully
	  ? DISPLACED_STEP_FINISH_STATUS_OK
	  : DISPLACED_STEP_FINISH_STATUS_NOT_EXECUTED);
}

const displaced_step_copy_insn_closure *
displaced_step_buffers::copy_insn_closure_by_addr (CORE_ADDR addr)
{
  for (const displaced_step_buffer &buffer : m_buffers)
    {
      /* Make sure we have active buffers to compare to.  */
      if (buffer.current_thread != nullptr && addr == buffer.addr)
      {
	/* The closure information should always be available. */
	gdb_assert (buffer.copy_insn_closure.get () != nullptr);
	return buffer.copy_insn_closure.get ();
      }
    }

  return nullptr;
}

void
displaced_step_buffers::restore_in_ptid (ptid_t ptid)
{
  for (const displaced_step_buffer &buffer : m_buffers)
    {
      if (buffer.current_thread == nullptr)
	continue;

      regcache *regcache = get_thread_regcache (buffer.current_thread);
      gdbarch *arch = regcache->arch ();
      ULONGEST len = gdbarch_displaced_step_buffer_length (arch);

      write_memory_ptid (ptid, buffer.addr, buffer.saved_copy.data (), len);

      displaced_debug_printf ("restored in ptid %s %s",
			      ptid.to_string ().c_str (),
			      paddress (arch, buffer.addr));
    }
}

void _initialize_displaced_stepping ();
void
_initialize_displaced_stepping ()
{
  add_setshow_boolean_cmd ("displaced", class_maintenance,
			   &debug_displaced, _("\
Set displaced stepping debugging."), _("\
Show displaced stepping debugging."), _("\
When non-zero, displaced stepping specific debugging is enabled."),
			    NULL,
			    show_debug_displaced,
			    &setdebuglist, &showdebuglist);
}
