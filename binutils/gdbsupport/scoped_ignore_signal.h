/* Support for ignoring signals.

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

#ifndef SCOPED_IGNORE_SIGNAL_H
#define SCOPED_IGNORE_SIGNAL_H

#include <signal.h>

/* RAII class used to ignore a signal in a scope.  If sigprocmask is
   supported, then the signal is only ignored by the calling thread.
   Otherwise, the signal disposition is set to SIG_IGN, which affects
   the whole process.  If ConsumePending is true, the destructor
   consumes a pending Sig.  SIGPIPE for example is queued on the
   thread even if blocked at the time the pipe is written to.  SIGTTOU
   OTOH is not raised at all if the thread writing to the terminal has
   it blocked.  Because SIGTTOU is sent to the whole process instead
   of to a specific thread, consuming a pending SIGTTOU in the
   destructor could consume a signal raised due to actions done by
   some other thread.  */

template <int Sig, bool ConsumePending>
class scoped_ignore_signal
{
public:
  scoped_ignore_signal ()
  {
#ifdef HAVE_SIGPROCMASK
    sigset_t set, old_state;

    sigemptyset (&set);
    sigaddset (&set, Sig);
    sigprocmask (SIG_BLOCK, &set, &old_state);
    m_was_blocked = sigismember (&old_state, Sig);
#else
    m_osig = signal (Sig, SIG_IGN);
#endif
  }

  ~scoped_ignore_signal ()
  {
#ifdef HAVE_SIGPROCMASK
    if (!m_was_blocked)
      {
	sigset_t set;

	sigemptyset (&set);
	sigaddset (&set, Sig);

	/* If we got a pending Sig signal, consume it before
	   unblocking.  */
	if (ConsumePending)
	  {
#ifdef HAVE_SIGTIMEDWAIT
	    const timespec zero_timeout = {};

	    sigtimedwait (&set, nullptr, &zero_timeout);
#else
	    sigset_t pending;

	    sigpending (&pending);
	    if (sigismember (&pending, Sig))
	      {
		int sig_found;

		sigwait (&set, &sig_found);
		gdb_assert (sig_found == Sig);
	      }
#endif
	  }

	sigprocmask (SIG_UNBLOCK, &set, nullptr);
      }
#else
    signal (Sig, m_osig);
#endif
  }

  DISABLE_COPY_AND_ASSIGN (scoped_ignore_signal);

private:
#ifdef HAVE_SIGPROCMASK
  bool m_was_blocked;
#else
  sighandler_t m_osig;
#endif
};

struct scoped_ignore_signal_nop
{
  /* Note, these can't both be "= default", because otherwise the
     compiler warns that variables of this type are not used.  */
  scoped_ignore_signal_nop ()
  {}
  ~scoped_ignore_signal_nop ()
  {}
  DISABLE_COPY_AND_ASSIGN (scoped_ignore_signal_nop);
};

#ifdef SIGPIPE
using scoped_ignore_sigpipe = scoped_ignore_signal<SIGPIPE, true>;
#else
using scoped_ignore_sigpipe = scoped_ignore_signal_nop;
#endif

#endif /* SCOPED_IGNORE_SIGNAL_H */
