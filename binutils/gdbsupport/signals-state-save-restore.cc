/* Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#include "common-defs.h"
#include "signals-state-save-restore.h"
#include "gdbsupport/gdb-sigmask.h"

#include <signal.h>

/* The original signal actions and mask.  */

#ifdef HAVE_SIGACTION
static struct sigaction original_signal_actions[NSIG];

static sigset_t original_signal_mask;
#endif

/* See signals-state-save-restore.h.   */

void
save_original_signals_state (bool quiet)
{
#ifdef HAVE_SIGACTION
  int i;
  int res;

  res = gdb_sigmask (0,  NULL, &original_signal_mask);
  if (res == -1)
    perror_with_name (("sigprocmask"));

  bool found_preinstalled = false;

  for (i = 1; i < NSIG; i++)
    {
      struct sigaction *oldact = &original_signal_actions[i];

      res = sigaction (i, NULL, oldact);
      if (res == -1 && errno == EINVAL)
	{
	  /* Some signal numbers in the range are invalid.  */
	  continue;
	}
      else if (res == -1)
	perror_with_name (("sigaction"));

      /* If we find a custom signal handler already installed, then
	 this function was called too late.  This is a warning instead
	 of an internal error because this can also happen if you
	 LD_PRELOAD a library that installs a signal handler early via
	 __attribute__((constructor)), like libSegFault.so.  */
      if (!quiet
	  && oldact->sa_handler != SIG_DFL
	  && oldact->sa_handler != SIG_IGN)
	{
	  found_preinstalled = true;

	  /* Use raw fprintf here because we're being called in early
	     startup, before GDB's filtered streams are created.  */
	  fprintf (stderr,
		   _("warning: Found custom handler for signal "
		     "%d (%s) preinstalled.\n"), i,
		   strsignal (i));
	}
    }

  if (found_preinstalled)
    {
      fprintf (stderr, _("\
Some signal dispositions inherited from the environment (SIG_DFL/SIG_IGN)\n\
won't be propagated to spawned programs.\n"));
    }
#endif
}

/* See signals-state-save-restore.h.   */

void
restore_original_signals_state (void)
{
#ifdef HAVE_SIGACTION
  int i;
  int res;

  for (i = 1; i < NSIG; i++)
    {
      res = sigaction (i, &original_signal_actions[i], NULL);
      if (res == -1 && errno == EINVAL)
	{
	  /* Some signal numbers in the range are invalid.  */
	  continue;
	}
      else if (res == -1)
	perror_with_name (("sigaction"));
    }

  res = gdb_sigmask (SIG_SETMASK,  &original_signal_mask, NULL);
  if (res == -1)
    perror_with_name (("sigprocmask"));
#endif
}
