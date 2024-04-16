/* Target errno mappings for newlib/libgloss environment.
   Copyright 1995-2024 Free Software Foundation, Inc.
   Contributed by Mike Frysinger.

   This file is part of simulators.

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

/* This must come before any other includes.  */
#include "defs.h"

#include <signal.h>

#include "sim/callback.h"

/* This file is kept up-to-date via the gennltvals.py script.  Do not edit
   anything between the START & END comment blocks below.  */

CB_TARGET_DEFS_MAP cb_init_signal_map[] = {
  /* gennltvals: START */
#ifdef SIGABRT
  { "SIGABRT", SIGABRT, 6 },
#endif
#ifdef SIGALRM
  { "SIGALRM", SIGALRM, 14 },
#endif
#ifdef SIGBUS
  { "SIGBUS", SIGBUS, 10 },
#endif
#ifdef SIGCHLD
  { "SIGCHLD", SIGCHLD, 20 },
#endif
#ifdef SIGCLD
  { "SIGCLD", SIGCLD, 20 },
#endif
#ifdef SIGCONT
  { "SIGCONT", SIGCONT, 19 },
#endif
#ifdef SIGEMT
  { "SIGEMT", SIGEMT, 7 },
#endif
#ifdef SIGFPE
  { "SIGFPE", SIGFPE, 8 },
#endif
#ifdef SIGHUP
  { "SIGHUP", SIGHUP, 1 },
#endif
#ifdef SIGILL
  { "SIGILL", SIGILL, 4 },
#endif
#ifdef SIGINT
  { "SIGINT", SIGINT, 2 },
#endif
#ifdef SIGIO
  { "SIGIO", SIGIO, 23 },
#endif
#ifdef SIGIOT
  { "SIGIOT", SIGIOT, 6 },
#endif
#ifdef SIGKILL
  { "SIGKILL", SIGKILL, 9 },
#endif
#ifdef SIGLOST
  { "SIGLOST", SIGLOST, 29 },
#endif
#ifdef SIGPIPE
  { "SIGPIPE", SIGPIPE, 13 },
#endif
#ifdef SIGPOLL
  { "SIGPOLL", SIGPOLL, 23 },
#endif
#ifdef SIGPROF
  { "SIGPROF", SIGPROF, 27 },
#endif
#ifdef SIGQUIT
  { "SIGQUIT", SIGQUIT, 3 },
#endif
#ifdef SIGSEGV
  { "SIGSEGV", SIGSEGV, 11 },
#endif
#ifdef SIGSTOP
  { "SIGSTOP", SIGSTOP, 17 },
#endif
#ifdef SIGSYS
  { "SIGSYS", SIGSYS, 12 },
#endif
#ifdef SIGTERM
  { "SIGTERM", SIGTERM, 15 },
#endif
#ifdef SIGTRAP
  { "SIGTRAP", SIGTRAP, 5 },
#endif
#ifdef SIGTSTP
  { "SIGTSTP", SIGTSTP, 18 },
#endif
#ifdef SIGTTIN
  { "SIGTTIN", SIGTTIN, 21 },
#endif
#ifdef SIGTTOU
  { "SIGTTOU", SIGTTOU, 22 },
#endif
#ifdef SIGURG
  { "SIGURG", SIGURG, 16 },
#endif
#ifdef SIGUSR1
  { "SIGUSR1", SIGUSR1, 30 },
#endif
#ifdef SIGUSR2
  { "SIGUSR2", SIGUSR2, 31 },
#endif
#ifdef SIGVTALRM
  { "SIGVTALRM", SIGVTALRM, 26 },
#endif
#ifdef SIGWINCH
  { "SIGWINCH", SIGWINCH, 28 },
#endif
#ifdef SIGXCPU
  { "SIGXCPU", SIGXCPU, 24 },
#endif
#ifdef SIGXFSZ
  { "SIGXFSZ", SIGXFSZ, 25 },
#endif
  /* gennltvals: END */
  { NULL, -1, -1 },
};
