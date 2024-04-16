/* Target signal translation functions for GDB.
   Copyright (C) 1990-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Support.

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

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "gdb_signals.h"

struct gdbarch;

/* Always use __SIGRTMIN if it's available.  SIGRTMIN is the lowest
   _available_ realtime signal, not the lowest supported; glibc takes
   several for its own use.  */

#ifndef REALTIME_LO
# if defined(__SIGRTMIN)
#  define REALTIME_LO __SIGRTMIN
#  define REALTIME_HI (__SIGRTMAX + 1)
# elif defined(SIGRTMIN)
#  define REALTIME_LO SIGRTMIN
#  define REALTIME_HI (SIGRTMAX + 1)
# endif
#endif

/* This table must match in order and size the signals in enum
   gdb_signal.  */

static const struct {
  const char *symbol;
  const char *name;
  const char *string;
  } signals [] =
{
#define SET(symbol, constant, name, string) { #symbol, name, string },
#include "gdb/signals.def"
#undef SET
};

const char *
gdb_signal_to_symbol_string (enum gdb_signal sig)
{
  gdb_assert ((int) sig >= GDB_SIGNAL_FIRST && (int) sig <= GDB_SIGNAL_LAST);

  return signals[sig].symbol;
}

/* Return the string for a signal.  */
const char *
gdb_signal_to_string (enum gdb_signal sig)
{
  if ((int) sig >= GDB_SIGNAL_FIRST && (int) sig <= GDB_SIGNAL_LAST)
    return signals[sig].string;
  else
    return signals[GDB_SIGNAL_UNKNOWN].string;
}

/* Return the name for a signal.  */
const char *
gdb_signal_to_name (enum gdb_signal sig)
{
  if ((int) sig >= GDB_SIGNAL_FIRST && (int) sig <= GDB_SIGNAL_LAST
      && signals[sig].name != NULL)
    return signals[sig].name;
  else
    /* I think the code which prints this will always print it along
       with the string, so no need to be verbose (very old comment).  */
    return "?";
}

/* Given a name, return its signal.  */
enum gdb_signal
gdb_signal_from_name (const char *name)
{
  enum gdb_signal sig;

  /* It's possible we also should allow "SIGCLD" as well as "SIGCHLD"
     for GDB_SIGNAL_SIGCHLD.  SIGIOT, on the other hand, is more
     questionable; seems like by now people should call it SIGABRT
     instead.  */

  /* This ugly cast brought to you by the native VAX compiler.  */
  for (sig = GDB_SIGNAL_HUP;
       sig < GDB_SIGNAL_LAST;
       sig = (enum gdb_signal) ((int) sig + 1))
    if (signals[sig].name != NULL
	&& strcmp (name, signals[sig].name) == 0)
      return sig;
  return GDB_SIGNAL_UNKNOWN;
}

/* The following functions are to help certain targets deal
   with the signal/waitstatus stuff.  They could just as well be in
   a file called native-utils.c or unixwaitstatus-utils.c or whatever.  */

/* Convert host signal to our signals.  */
enum gdb_signal
gdb_signal_from_host (int hostsig)
{
  /* A switch statement would make sense but would require special
     kludges to deal with the cases where more than one signal has the
     same number.  Signals are ordered ANSI-standard signals first,
     other signals second, with signals in each block ordered by their
     numerical values on a typical POSIX platform.  */

  if (hostsig == 0)
    return GDB_SIGNAL_0;

  /* SIGINT, SIGILL, SIGABRT, SIGFPE, SIGSEGV and SIGTERM
     are ANSI-standard signals and are always available.  */
  if (hostsig == SIGINT)
    return GDB_SIGNAL_INT;
  if (hostsig == SIGILL)
    return GDB_SIGNAL_ILL;
  if (hostsig == SIGABRT)
    return GDB_SIGNAL_ABRT;
  if (hostsig == SIGFPE)
    return GDB_SIGNAL_FPE;
  if (hostsig == SIGSEGV)
    return GDB_SIGNAL_SEGV;
  if (hostsig == SIGTERM)
    return GDB_SIGNAL_TERM;

  /* All other signals need preprocessor conditionals.  */
#if defined (SIGHUP)
  if (hostsig == SIGHUP)
    return GDB_SIGNAL_HUP;
#endif
#if defined (SIGQUIT)
  if (hostsig == SIGQUIT)
    return GDB_SIGNAL_QUIT;
#endif
#if defined (SIGTRAP)
  if (hostsig == SIGTRAP)
    return GDB_SIGNAL_TRAP;
#endif
#if defined (SIGEMT)
  if (hostsig == SIGEMT)
    return GDB_SIGNAL_EMT;
#endif
#if defined (SIGKILL)
  if (hostsig == SIGKILL)
    return GDB_SIGNAL_KILL;
#endif
#if defined (SIGBUS)
  if (hostsig == SIGBUS)
    return GDB_SIGNAL_BUS;
#endif
#if defined (SIGSYS)
  if (hostsig == SIGSYS)
    return GDB_SIGNAL_SYS;
#endif
#if defined (SIGPIPE)
  if (hostsig == SIGPIPE)
    return GDB_SIGNAL_PIPE;
#endif
#if defined (SIGALRM)
  if (hostsig == SIGALRM)
    return GDB_SIGNAL_ALRM;
#endif
#if defined (SIGUSR1)
  if (hostsig == SIGUSR1)
    return GDB_SIGNAL_USR1;
#endif
#if defined (SIGUSR2)
  if (hostsig == SIGUSR2)
    return GDB_SIGNAL_USR2;
#endif
#if defined (SIGCLD)
  if (hostsig == SIGCLD)
    return GDB_SIGNAL_CHLD;
#endif
#if defined (SIGCHLD)
  if (hostsig == SIGCHLD)
    return GDB_SIGNAL_CHLD;
#endif
#if defined (SIGPWR)
  if (hostsig == SIGPWR)
    return GDB_SIGNAL_PWR;
#endif
#if defined (SIGWINCH)
  if (hostsig == SIGWINCH)
    return GDB_SIGNAL_WINCH;
#endif
#if defined (SIGURG)
  if (hostsig == SIGURG)
    return GDB_SIGNAL_URG;
#endif
#if defined (SIGIO)
  if (hostsig == SIGIO)
    return GDB_SIGNAL_IO;
#endif
#if defined (SIGPOLL)
  if (hostsig == SIGPOLL)
    return GDB_SIGNAL_POLL;
#endif
#if defined (SIGSTOP)
  if (hostsig == SIGSTOP)
    return GDB_SIGNAL_STOP;
#endif
#if defined (SIGTSTP)
  if (hostsig == SIGTSTP)
    return GDB_SIGNAL_TSTP;
#endif
#if defined (SIGCONT)
  if (hostsig == SIGCONT)
    return GDB_SIGNAL_CONT;
#endif
#if defined (SIGTTIN)
  if (hostsig == SIGTTIN)
    return GDB_SIGNAL_TTIN;
#endif
#if defined (SIGTTOU)
  if (hostsig == SIGTTOU)
    return GDB_SIGNAL_TTOU;
#endif
#if defined (SIGVTALRM)
  if (hostsig == SIGVTALRM)
    return GDB_SIGNAL_VTALRM;
#endif
#if defined (SIGPROF)
  if (hostsig == SIGPROF)
    return GDB_SIGNAL_PROF;
#endif
#if defined (SIGXCPU)
  if (hostsig == SIGXCPU)
    return GDB_SIGNAL_XCPU;
#endif
#if defined (SIGXFSZ)
  if (hostsig == SIGXFSZ)
    return GDB_SIGNAL_XFSZ;
#endif
#if defined (SIGWIND)
  if (hostsig == SIGWIND)
    return GDB_SIGNAL_WIND;
#endif
#if defined (SIGPHONE)
  if (hostsig == SIGPHONE)
    return GDB_SIGNAL_PHONE;
#endif
#if defined (SIGLOST)
  if (hostsig == SIGLOST)
    return GDB_SIGNAL_LOST;
#endif
#if defined (SIGWAITING)
  if (hostsig == SIGWAITING)
    return GDB_SIGNAL_WAITING;
#endif
#if defined (SIGCANCEL)
  if (hostsig == SIGCANCEL)
    return GDB_SIGNAL_CANCEL;
#endif
#if defined (SIGLWP)
  if (hostsig == SIGLWP)
    return GDB_SIGNAL_LWP;
#endif
#if defined (SIGDANGER)
  if (hostsig == SIGDANGER)
    return GDB_SIGNAL_DANGER;
#endif
#if defined (SIGGRANT)
  if (hostsig == SIGGRANT)
    return GDB_SIGNAL_GRANT;
#endif
#if defined (SIGRETRACT)
  if (hostsig == SIGRETRACT)
    return GDB_SIGNAL_RETRACT;
#endif
#if defined (SIGMSG)
  if (hostsig == SIGMSG)
    return GDB_SIGNAL_MSG;
#endif
#if defined (SIGSOUND)
  if (hostsig == SIGSOUND)
    return GDB_SIGNAL_SOUND;
#endif
#if defined (SIGSAK)
  if (hostsig == SIGSAK)
    return GDB_SIGNAL_SAK;
#endif
#if defined (SIGPRIO)
  if (hostsig == SIGPRIO)
    return GDB_SIGNAL_PRIO;
#endif

  /* Mach exceptions.  Assumes that the values for EXC_ are positive! */
#if defined (EXC_BAD_ACCESS) && defined (_NSIG)
  if (hostsig == _NSIG + EXC_BAD_ACCESS)
    return GDB_EXC_BAD_ACCESS;
#endif
#if defined (EXC_BAD_INSTRUCTION) && defined (_NSIG)
  if (hostsig == _NSIG + EXC_BAD_INSTRUCTION)
    return GDB_EXC_BAD_INSTRUCTION;
#endif
#if defined (EXC_ARITHMETIC) && defined (_NSIG)
  if (hostsig == _NSIG + EXC_ARITHMETIC)
    return GDB_EXC_ARITHMETIC;
#endif
#if defined (EXC_EMULATION) && defined (_NSIG)
  if (hostsig == _NSIG + EXC_EMULATION)
    return GDB_EXC_EMULATION;
#endif
#if defined (EXC_SOFTWARE) && defined (_NSIG)
  if (hostsig == _NSIG + EXC_SOFTWARE)
    return GDB_EXC_SOFTWARE;
#endif
#if defined (EXC_BREAKPOINT) && defined (_NSIG)
  if (hostsig == _NSIG + EXC_BREAKPOINT)
    return GDB_EXC_BREAKPOINT;
#endif

#if defined (SIGINFO)
  if (hostsig == SIGINFO)
    return GDB_SIGNAL_INFO;
#endif
#if defined (SIGLIBRT)
  if (hostsig == SIGLIBRT)
    return GDB_SIGNAL_LIBRT;
#endif

#if defined (REALTIME_LO)
  if (hostsig >= REALTIME_LO && hostsig < REALTIME_HI)
    {
      /* This block of GDB_SIGNAL_REALTIME value is in order.  */
      if (33 <= hostsig && hostsig <= 63)
	return (enum gdb_signal)
	  (hostsig - 33 + (int) GDB_SIGNAL_REALTIME_33);
      else if (hostsig == 32)
	return GDB_SIGNAL_REALTIME_32;
      else if (64 <= hostsig && hostsig <= 127)
	return (enum gdb_signal)
	  (hostsig - 64 + (int) GDB_SIGNAL_REALTIME_64);
      else
	error (_("GDB bug: target.c (gdb_signal_from_host): "
	       "unrecognized real-time signal"));
    }
#endif

  return GDB_SIGNAL_UNKNOWN;
}

/* Convert a OURSIG (an enum gdb_signal) to the form used by the
   target operating system (referred to as the ``host'') or zero if the
   equivalent host signal is not available.  Set/clear OURSIG_OK
   accordingly. */

static int
do_gdb_signal_to_host (enum gdb_signal oursig,
			  int *oursig_ok)
{
  int retsig;
  /* Silence the 'not used' warning, for targets that
     do not support signals.  */
  (void) retsig;

  /* Signals are ordered ANSI-standard signals first, other signals
     second, with signals in each block ordered by their numerical
     values on a typical POSIX platform.  */

  *oursig_ok = 1;
  switch (oursig)
    {
    case GDB_SIGNAL_0:
      return 0;

      /* SIGINT, SIGILL, SIGABRT, SIGFPE, SIGSEGV and SIGTERM
	 are ANSI-standard signals and are always available.  */
    case GDB_SIGNAL_INT:
      return SIGINT;
    case GDB_SIGNAL_ILL:
      return SIGILL;
    case GDB_SIGNAL_ABRT:
      return SIGABRT;
    case GDB_SIGNAL_FPE:
      return SIGFPE;
    case GDB_SIGNAL_SEGV:
      return SIGSEGV;
    case GDB_SIGNAL_TERM:
      return SIGTERM;

      /* All other signals need preprocessor conditionals.  */
#if defined (SIGHUP)
    case GDB_SIGNAL_HUP:
      return SIGHUP;
#endif
#if defined (SIGQUIT)
    case GDB_SIGNAL_QUIT:
      return SIGQUIT;
#endif
#if defined (SIGTRAP)
    case GDB_SIGNAL_TRAP:
      return SIGTRAP;
#endif
#if defined (SIGEMT)
    case GDB_SIGNAL_EMT:
      return SIGEMT;
#endif
#if defined (SIGKILL)
    case GDB_SIGNAL_KILL:
      return SIGKILL;
#endif
#if defined (SIGBUS)
    case GDB_SIGNAL_BUS:
      return SIGBUS;
#endif
#if defined (SIGSYS)
    case GDB_SIGNAL_SYS:
      return SIGSYS;
#endif
#if defined (SIGPIPE)
    case GDB_SIGNAL_PIPE:
      return SIGPIPE;
#endif
#if defined (SIGALRM)
    case GDB_SIGNAL_ALRM:
      return SIGALRM;
#endif
#if defined (SIGUSR1)
    case GDB_SIGNAL_USR1:
      return SIGUSR1;
#endif
#if defined (SIGUSR2)
    case GDB_SIGNAL_USR2:
      return SIGUSR2;
#endif
#if defined (SIGCHLD) || defined (SIGCLD)
    case GDB_SIGNAL_CHLD:
#if defined (SIGCHLD)
      return SIGCHLD;
#else
      return SIGCLD;
#endif
#endif /* SIGCLD or SIGCHLD */
#if defined (SIGPWR)
    case GDB_SIGNAL_PWR:
      return SIGPWR;
#endif
#if defined (SIGWINCH)
    case GDB_SIGNAL_WINCH:
      return SIGWINCH;
#endif
#if defined (SIGURG)
    case GDB_SIGNAL_URG:
      return SIGURG;
#endif
#if defined (SIGIO)
    case GDB_SIGNAL_IO:
      return SIGIO;
#endif
#if defined (SIGPOLL)
    case GDB_SIGNAL_POLL:
      return SIGPOLL;
#endif
#if defined (SIGSTOP)
    case GDB_SIGNAL_STOP:
      return SIGSTOP;
#endif
#if defined (SIGTSTP)
    case GDB_SIGNAL_TSTP:
      return SIGTSTP;
#endif
#if defined (SIGCONT)
    case GDB_SIGNAL_CONT:
      return SIGCONT;
#endif
#if defined (SIGTTIN)
    case GDB_SIGNAL_TTIN:
      return SIGTTIN;
#endif
#if defined (SIGTTOU)
    case GDB_SIGNAL_TTOU:
      return SIGTTOU;
#endif
#if defined (SIGVTALRM)
    case GDB_SIGNAL_VTALRM:
      return SIGVTALRM;
#endif
#if defined (SIGPROF)
    case GDB_SIGNAL_PROF:
      return SIGPROF;
#endif
#if defined (SIGXCPU)
    case GDB_SIGNAL_XCPU:
      return SIGXCPU;
#endif
#if defined (SIGXFSZ)
    case GDB_SIGNAL_XFSZ:
      return SIGXFSZ;
#endif
#if defined (SIGWIND)
    case GDB_SIGNAL_WIND:
      return SIGWIND;
#endif
#if defined (SIGPHONE)
    case GDB_SIGNAL_PHONE:
      return SIGPHONE;
#endif
#if defined (SIGLOST)
    case GDB_SIGNAL_LOST:
      return SIGLOST;
#endif
#if defined (SIGWAITING)
    case GDB_SIGNAL_WAITING:
      return SIGWAITING;
#endif
#if defined (SIGCANCEL)
    case GDB_SIGNAL_CANCEL:
      return SIGCANCEL;
#endif
#if defined (SIGLWP)
    case GDB_SIGNAL_LWP:
      return SIGLWP;
#endif
#if defined (SIGDANGER)
    case GDB_SIGNAL_DANGER:
      return SIGDANGER;
#endif
#if defined (SIGGRANT)
    case GDB_SIGNAL_GRANT:
      return SIGGRANT;
#endif
#if defined (SIGRETRACT)
    case GDB_SIGNAL_RETRACT:
      return SIGRETRACT;
#endif
#if defined (SIGMSG)
    case GDB_SIGNAL_MSG:
      return SIGMSG;
#endif
#if defined (SIGSOUND)
    case GDB_SIGNAL_SOUND:
      return SIGSOUND;
#endif
#if defined (SIGSAK)
    case GDB_SIGNAL_SAK:
      return SIGSAK;
#endif
#if defined (SIGPRIO)
    case GDB_SIGNAL_PRIO:
      return SIGPRIO;
#endif

      /* Mach exceptions.  Assumes that the values for EXC_ are positive! */
#if defined (EXC_BAD_ACCESS) && defined (_NSIG)
    case GDB_EXC_BAD_ACCESS:
      return _NSIG + EXC_BAD_ACCESS;
#endif
#if defined (EXC_BAD_INSTRUCTION) && defined (_NSIG)
    case GDB_EXC_BAD_INSTRUCTION:
      return _NSIG + EXC_BAD_INSTRUCTION;
#endif
#if defined (EXC_ARITHMETIC) && defined (_NSIG)
    case GDB_EXC_ARITHMETIC:
      return _NSIG + EXC_ARITHMETIC;
#endif
#if defined (EXC_EMULATION) && defined (_NSIG)
    case GDB_EXC_EMULATION:
      return _NSIG + EXC_EMULATION;
#endif
#if defined (EXC_SOFTWARE) && defined (_NSIG)
    case GDB_EXC_SOFTWARE:
      return _NSIG + EXC_SOFTWARE;
#endif
#if defined (EXC_BREAKPOINT) && defined (_NSIG)
    case GDB_EXC_BREAKPOINT:
      return _NSIG + EXC_BREAKPOINT;
#endif

#if defined (SIGINFO)
    case GDB_SIGNAL_INFO:
      return SIGINFO;
#endif
#if defined (SIGLIBRT)
    case GDB_SIGNAL_LIBRT:
      return SIGLIBRT;
#endif

    default:
#if defined (REALTIME_LO)
      retsig = 0;

      if (oursig >= GDB_SIGNAL_REALTIME_33
	  && oursig <= GDB_SIGNAL_REALTIME_63)
	{
	  /* This block of signals is continuous, and
	     GDB_SIGNAL_REALTIME_33 is 33 by definition.  */
	  retsig = (int) oursig - (int) GDB_SIGNAL_REALTIME_33 + 33;
	}
      else if (oursig == GDB_SIGNAL_REALTIME_32)
	{
	  /* GDB_SIGNAL_REALTIME_32 isn't contiguous with
	     GDB_SIGNAL_REALTIME_33.  It is 32 by definition.  */
	  retsig = 32;
	}
      else if (oursig >= GDB_SIGNAL_REALTIME_64
	  && oursig <= GDB_SIGNAL_REALTIME_127)
	{
	  /* This block of signals is continuous, and
	     GDB_SIGNAL_REALTIME_64 is 64 by definition.  */
	  retsig = (int) oursig - (int) GDB_SIGNAL_REALTIME_64 + 64;
	}

      if (retsig >= REALTIME_LO && retsig < REALTIME_HI)
	return retsig;
#endif

      *oursig_ok = 0;
      return 0;
    }
}

int
gdb_signal_to_host_p (enum gdb_signal oursig)
{
  int oursig_ok;
  do_gdb_signal_to_host (oursig, &oursig_ok);
  return oursig_ok;
}

int
gdb_signal_to_host (enum gdb_signal oursig)
{
  int oursig_ok;
  int targ_signo = do_gdb_signal_to_host (oursig, &oursig_ok);
  if (!oursig_ok)
    {
      /* The user might be trying to do "signal SIGSAK" where this system
	 doesn't have SIGSAK.  */
      warning (_("Signal %s does not exist on this system."),
	       gdb_signal_to_name (oursig));
      return 0;
    }
  else
    return targ_signo;
}
