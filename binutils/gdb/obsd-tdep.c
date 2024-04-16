/* Target-dependent code for OpenBSD.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.

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
#include "auxv.h"
#include "frame.h"
#include "symtab.h"
#include "objfiles.h"

#include "obsd-tdep.h"

CORE_ADDR
obsd_skip_solib_resolver (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct bound_minimal_symbol msym;

  msym = lookup_minimal_symbol("_dl_bind", NULL, NULL);
  if (msym.minsym && msym.value_address () == pc)
    return frame_unwind_caller_pc (get_current_frame ());
  else
    return find_solib_trampoline_target (get_current_frame (), pc);
}

/* OpenBSD signal numbers.  From <sys/signal.h>. */

enum
  {
    OBSD_SIGHUP = 1,
    OBSD_SIGINT = 2,
    OBSD_SIGQUIT = 3,
    OBSD_SIGILL = 4,
    OBSD_SIGTRAP = 5,
    OBSD_SIGABRT = 6,
    OBSD_SIGEMT = 7,
    OBSD_SIGFPE = 8,
    OBSD_SIGKILL = 9,
    OBSD_SIGBUS = 10,
    OBSD_SIGSEGV = 11,
    OBSD_SIGSYS = 12,
    OBSD_SIGPIPE = 13,
    OBSD_SIGALRM = 14,
    OBSD_SIGTERM = 15,
    OBSD_SIGURG = 16,
    OBSD_SIGSTOP = 17,
    OBSD_SIGTSTP = 18,
    OBSD_SIGCONT = 19,
    OBSD_SIGCHLD = 20,
    OBSD_SIGTTIN = 21,
    OBSD_SIGTTOU = 22,
    OBSD_SIGIO = 23,
    OBSD_SIGXCPU = 24,
    OBSD_SIGXFSZ = 25,
    OBSD_SIGVTALRM = 26,
    OBSD_SIGPROF = 27,
    OBSD_SIGWINCH = 28,
    OBSD_SIGINFO = 29,
    OBSD_SIGUSR1 = 30,
    OBSD_SIGUSR2 = 31,
    OBSD_SIGTHR = 32,
  };

/* Implement the "gdb_signal_from_target" gdbarch method.  */

static enum gdb_signal
obsd_gdb_signal_from_target (struct gdbarch *gdbarch, int signal)
{
  switch (signal)
    {
    case 0:
      return GDB_SIGNAL_0;

    case OBSD_SIGHUP:
      return GDB_SIGNAL_HUP;

    case OBSD_SIGINT:
      return GDB_SIGNAL_INT;

    case OBSD_SIGQUIT:
      return GDB_SIGNAL_QUIT;

    case OBSD_SIGILL:
      return GDB_SIGNAL_ILL;

    case OBSD_SIGTRAP:
      return GDB_SIGNAL_TRAP;

    case OBSD_SIGABRT:
      return GDB_SIGNAL_ABRT;

    case OBSD_SIGEMT:
      return GDB_SIGNAL_EMT;

    case OBSD_SIGFPE:
      return GDB_SIGNAL_FPE;

    case OBSD_SIGKILL:
      return GDB_SIGNAL_KILL;

    case OBSD_SIGBUS:
      return GDB_SIGNAL_BUS;

    case OBSD_SIGSEGV:
      return GDB_SIGNAL_SEGV;

    case OBSD_SIGSYS:
      return GDB_SIGNAL_SYS;

    case OBSD_SIGPIPE:
      return GDB_SIGNAL_PIPE;

    case OBSD_SIGALRM:
      return GDB_SIGNAL_ALRM;

    case OBSD_SIGTERM:
      return GDB_SIGNAL_TERM;

    case OBSD_SIGURG:
      return GDB_SIGNAL_URG;

    case OBSD_SIGSTOP:
      return GDB_SIGNAL_STOP;

    case OBSD_SIGTSTP:
      return GDB_SIGNAL_TSTP;

    case OBSD_SIGCONT:
      return GDB_SIGNAL_CONT;

    case OBSD_SIGCHLD:
      return GDB_SIGNAL_CHLD;

    case OBSD_SIGTTIN:
      return GDB_SIGNAL_TTIN;

    case OBSD_SIGTTOU:
      return GDB_SIGNAL_TTOU;

    case OBSD_SIGIO:
      return GDB_SIGNAL_IO;

    case OBSD_SIGXCPU:
      return GDB_SIGNAL_XCPU;

    case OBSD_SIGXFSZ:
      return GDB_SIGNAL_XFSZ;

    case OBSD_SIGVTALRM:
      return GDB_SIGNAL_VTALRM;

    case OBSD_SIGPROF:
      return GDB_SIGNAL_PROF;

    case OBSD_SIGWINCH:
      return GDB_SIGNAL_WINCH;

    case OBSD_SIGINFO:
      return GDB_SIGNAL_INFO;

    case OBSD_SIGUSR1:
      return GDB_SIGNAL_USR1;

    case OBSD_SIGUSR2:
      return GDB_SIGNAL_USR2;
    }

  return GDB_SIGNAL_UNKNOWN;
}

/* Implement the "gdb_signal_to_target" gdbarch method.  */

static int
obsd_gdb_signal_to_target (struct gdbarch *gdbarch,
			   enum gdb_signal signal)
{
  switch (signal)
    {
    case GDB_SIGNAL_0:
      return 0;

    case GDB_SIGNAL_HUP:
      return OBSD_SIGHUP;

    case GDB_SIGNAL_INT:
      return OBSD_SIGINT;

    case GDB_SIGNAL_QUIT:
      return OBSD_SIGQUIT;

    case GDB_SIGNAL_ILL:
      return OBSD_SIGILL;

    case GDB_SIGNAL_TRAP:
      return OBSD_SIGTRAP;

    case GDB_SIGNAL_ABRT:
      return OBSD_SIGABRT;

    case GDB_SIGNAL_EMT:
      return OBSD_SIGEMT;

    case GDB_SIGNAL_FPE:
      return OBSD_SIGFPE;

    case GDB_SIGNAL_KILL:
      return OBSD_SIGKILL;

    case GDB_SIGNAL_BUS:
      return OBSD_SIGBUS;

    case GDB_SIGNAL_SEGV:
      return OBSD_SIGSEGV;

    case GDB_SIGNAL_SYS:
      return OBSD_SIGSYS;

    case GDB_SIGNAL_PIPE:
      return OBSD_SIGPIPE;

    case GDB_SIGNAL_ALRM:
      return OBSD_SIGALRM;

    case GDB_SIGNAL_TERM:
      return OBSD_SIGTERM;

    case GDB_SIGNAL_URG:
      return OBSD_SIGURG;

    case GDB_SIGNAL_STOP:
      return OBSD_SIGSTOP;

    case GDB_SIGNAL_TSTP:
      return OBSD_SIGTSTP;

    case GDB_SIGNAL_CONT:
      return OBSD_SIGCONT;

    case GDB_SIGNAL_CHLD:
      return OBSD_SIGCHLD;

    case GDB_SIGNAL_TTIN:
      return OBSD_SIGTTIN;

    case GDB_SIGNAL_TTOU:
      return OBSD_SIGTTOU;

    case GDB_SIGNAL_IO:
      return OBSD_SIGIO;

    case GDB_SIGNAL_XCPU:
      return OBSD_SIGXCPU;

    case GDB_SIGNAL_XFSZ:
      return OBSD_SIGXFSZ;

    case GDB_SIGNAL_VTALRM:
      return OBSD_SIGVTALRM;

    case GDB_SIGNAL_PROF:
      return OBSD_SIGPROF;

    case GDB_SIGNAL_WINCH:
      return OBSD_SIGWINCH;

    case GDB_SIGNAL_USR1:
      return OBSD_SIGUSR1;

    case GDB_SIGNAL_USR2:
      return OBSD_SIGUSR2;

    case GDB_SIGNAL_INFO:
      return OBSD_SIGINFO;
    }

  return -1;
}

void
obsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  set_gdbarch_gdb_signal_from_target (gdbarch,
				      obsd_gdb_signal_from_target);
  set_gdbarch_gdb_signal_to_target (gdbarch,
				    obsd_gdb_signal_to_target);

  set_gdbarch_auxv_parse (gdbarch, svr4_auxv_parse);
}
