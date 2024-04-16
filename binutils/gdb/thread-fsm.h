/* Thread command's finish-state machine, for GDB, the GNU debugger.
   Copyright (C) 2015-2024 Free Software Foundation, Inc.

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

#ifndef THREAD_FSM_H
#define THREAD_FSM_H

#include "mi/mi-common.h"

struct return_value_info;
struct thread_fsm_ops;

/* A thread finite-state machine structure contains the necessary info
   and callbacks to manage the state machine protocol of a thread's
   execution command.  */

struct thread_fsm
{
  explicit thread_fsm (struct interp *cmd_interp)
    : command_interp (cmd_interp)
  {
  }

  /* The destructor.  This should simply free heap allocated data
     structures.  Cleaning up target resources (like, e.g.,
     breakpoints) should be done in the clean_up method.  */
  virtual ~thread_fsm () = default;

  DISABLE_COPY_AND_ASSIGN (thread_fsm);

  /* Called to clean up target resources after the FSM.  E.g., if the
     FSM created internal breakpoints, this is where they should be
     deleted.  */
  virtual void clean_up (struct thread_info *thread)
  {
  }

  /* Called after handle_inferior_event decides the target is done
     (that is, after stop_waiting).  The FSM is given a chance to
     decide whether the command is done and thus the target should
     stop, or whether there's still more to do and thus the thread
     should be re-resumed.  This is a good place to cache target data
     too.  For example, the "finish" command saves the just-finished
     function's return value here.  */
  virtual bool should_stop (struct thread_info *thread) = 0;

  /* If this FSM saved a function's return value, you can use this
     method to retrieve it.  Otherwise, this returns NULL.  */
  virtual struct return_value_info *return_value ()
  {
    return nullptr;
  }

  enum async_reply_reason async_reply_reason ()
  {
    /* If we didn't finish, then the stop reason must come from
       elsewhere.  E.g., a breakpoint hit or a signal intercepted.  */
    gdb_assert (finished_p ());
    return do_async_reply_reason ();
  }

  /* Whether the stop should be notified to the user/frontend.  */
  virtual bool should_notify_stop ()
  {
    return true;
  }

  void set_finished ()
  {
    finished = true;
  }

  bool finished_p () const
  {
    return finished;
  }

  /* The interpreter that issued the execution command that caused
     this thread to resume.  If the top level interpreter is MI/async,
     and the execution command was a CLI command (next/step/etc.),
     we'll want to print stop event output to the MI console channel
     (the stepped-to line, etc.), as if the user entered the execution
     command on a real GDB console.  */
  struct interp *command_interp = nullptr;

protected:

  /* Whether the FSM is done successfully.  */
  bool finished = false;

  /* The async_reply_reason that is broadcast to MI clients if this
     FSM finishes successfully.  */
  virtual enum async_reply_reason do_async_reply_reason ()
  {
    gdb_assert_not_reached ("should not call async_reply_reason here");
  }
};

#endif /* THREAD_FSM_H */
