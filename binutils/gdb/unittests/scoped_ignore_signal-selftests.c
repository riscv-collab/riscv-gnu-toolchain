/* Self tests for scoped_ignored_signal for GDB, the GNU debugger.

   Copyright (C) 2021-2024 Free Software Foundation, Inc.

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
#include "gdbsupport/scoped_ignore_signal.h"
#include "gdbsupport/selftest.h"
#include "gdbsupport/scope-exit.h"
#include <unistd.h>
#include <signal.h>

namespace selftests {
namespace scoped_ignore_sig {

#ifdef SIGPIPE

/* True if the SIGPIPE handler ran.  */
static volatile sig_atomic_t got_sigpipe = 0;

/* SIGPIPE handler for testing.  */

static void
handle_sigpipe (int)
{
  got_sigpipe = 1;
}

/* Test scoped_ignore_sigpipe.  */

static void
test_sigpipe ()
{
  auto *osig = signal (SIGPIPE, handle_sigpipe);
  SCOPE_EXIT { signal (SIGPIPE, osig); };

#ifdef HAVE_SIGPROCMASK
  /* Make sure SIGPIPE isn't blocked.  */
  sigset_t set, old_state;
  sigemptyset (&set);
  sigaddset (&set, SIGPIPE);
  sigprocmask (SIG_UNBLOCK, &set, &old_state);
  SCOPE_EXIT { sigprocmask (SIG_SETMASK, &old_state, nullptr); };
#endif

  /* Create pipe, and close read end so that writes to the pipe fail
     with EPIPE.  */

  int fd[2];
  char c = 0xff;
  int r;

  r = pipe (fd);
  SELF_CHECK (r == 0);

  close (fd[0]);
  SCOPE_EXIT { close (fd[1]); };

  /* Check that writing to the pipe results in EPIPE.  EXPECT_SIG
     indicates whether a SIGPIPE signal is expected.  */
  auto check_pipe_write = [&] (bool expect_sig)
  {
    got_sigpipe = 0;
    errno = 0;

    r = write (fd[1], &c, 1);
    SELF_CHECK (r == -1 && errno == EPIPE
		&& got_sigpipe == expect_sig);
  };

  /* Check that without a scoped_ignore_sigpipe in scope we indeed get
     a SIGPIPE signal.  */
  check_pipe_write (true);

  /* Now check that with a scoped_ignore_sigpipe in scope, SIGPIPE is
     ignored/blocked.  */
  {
    scoped_ignore_sigpipe ignore1;

    check_pipe_write (false);

    /* Check that scoped_ignore_sigpipe nests correctly.  */
    {
      scoped_ignore_sigpipe ignore2;

      check_pipe_write (false);
    }

    /* If nesting works correctly, this write results in no
       SIGPIPE.  */
    check_pipe_write (false);
  }

  /* No scoped_ignore_sigpipe is in scope anymore, so this should
     result in a SIGPIPE signal.  */
  check_pipe_write (true);
}

#endif /* SIGPIPE */

} /* namespace scoped_ignore_sig */
} /* namespace selftests */

void _initialize_scoped_ignore_signal_selftests ();
void
_initialize_scoped_ignore_signal_selftests ()
{
#ifdef SIGPIPE
  selftests::register_test ("scoped_ignore_sigpipe",
			    selftests::scoped_ignore_sig::test_sigpipe);
#endif
}
