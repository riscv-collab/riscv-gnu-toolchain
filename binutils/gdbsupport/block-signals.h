/* Block signals used by gdb

   Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

#ifndef GDBSUPPORT_BLOCK_SIGNALS_H
#define GDBSUPPORT_BLOCK_SIGNALS_H

#include <signal.h>

#include "gdbsupport/gdb-sigmask.h"

namespace gdb
{

/* This is an RAII class that temporarily blocks the signals needed by
   gdb.  This can be used before starting a new thread to ensure that
   this thread starts with the appropriate signals blocked.  */
class block_signals
{
public:
  block_signals ()
  {
#ifdef HAVE_SIGPROCMASK
    sigset_t mask;
    sigemptyset (&mask);
    sigaddset (&mask, SIGINT);
    sigaddset (&mask, SIGCHLD);
    sigaddset (&mask, SIGALRM);
    sigaddset (&mask, SIGWINCH);
    sigaddset (&mask, SIGTERM);
    gdb_sigmask (SIG_BLOCK, &mask, &m_old_mask);
#endif
  }

  ~block_signals ()
  {
#ifdef HAVE_SIGPROCMASK
    gdb_sigmask (SIG_SETMASK, &m_old_mask, nullptr);
#endif
  }

  DISABLE_COPY_AND_ASSIGN (block_signals);

private:

#ifdef HAVE_SIGPROCMASK
  sigset_t m_old_mask;
#endif
};

}

#endif /* GDBSUPPORT_BLOCK_SIGNALS_H */
