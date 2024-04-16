/* Handling of inferior events for the event loop for GDB, the GNU debugger.
   Copyright (C) 1999-2024 Free Software Foundation, Inc.
   Written by Elena Zannoni <ezannoni@cygnus.com> of Cygnus Solutions.

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
#include "inferior.h"
#include "infrun.h"
#include "gdbsupport/event-loop.h"
#include "event-top.h"
#include "inf-loop.h"
#include "remote.h"
#include "language.h"
#include "gdbthread.h"
#include "interps.h"
#include "top.h"
#include "ui.h"
#include "observable.h"

/* General function to handle events in the inferior.  */

void
inferior_event_handler (enum inferior_event_type event_type)
{
  switch (event_type)
    {
    case INF_REG_EVENT:
      fetch_inferior_event ();
      break;

    case INF_EXEC_COMPLETE:
      if (!non_stop)
	{
	  /* Unregister the inferior from the event loop.  This is done
	     so that when the inferior is not running we don't get
	     distracted by spurious inferior output.  */
	  if (target_has_execution () && target_can_async_p ())
	    target_async (false);
	}

      /* Do all continuations associated with the whole inferior (not
	 a particular thread).  */
      if (inferior_ptid != null_ptid)
	current_inferior ()->do_all_continuations ();

      /* When running a command list (from a user command, say), these
	 are only run when the command list is all done.  */
      if (current_ui->async)
	{
	  check_frame_language_change ();

	  /* Don't propagate breakpoint commands errors.  Either we're
	     stopping or some command resumes the inferior.  The user will
	     be informed.  */
	  try
	    {
	      bpstat_do_actions ();
	    }
	  catch (const gdb_exception_error &e)
	    {
	      /* If the user was running a foreground execution
		 command, then propagate the error so that the prompt
		 can be reenabled.  Otherwise, the user already has
		 the prompt and is typing some unrelated command, so
		 just inform the user and swallow the exception.  */
	      if (current_ui->prompt_state == PROMPT_BLOCKED)
		throw;
	      else
		exception_print (gdb_stderr, e);
	    }
	}
      break;

    default:
      gdb_printf (gdb_stderr, _("Event type not recognized.\n"));
      break;
    }
}
