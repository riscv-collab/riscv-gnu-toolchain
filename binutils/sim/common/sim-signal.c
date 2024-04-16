/* Simulator signal support
   Copyright (C) 1997-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Support

This file is part of the GNU Simulators.

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

#include "sim/sim.h"
#include "sim-io.h"
#include "sim-signal.h"

/* Convert SIM_SIGFOO to SIGFOO.
   What to do when the host doesn't have SIGFOO is handled on a case by case
   basis.  Generally, in the case of passing a value back to gdb, we want gdb
   to not think the process has died (so it can be debugged at the point of
   failure).  */

#ifdef _WIN32
#ifndef SIGTRAP
#define SIGTRAP 5
#endif
#ifndef SIGBUS
#define SIGBUS 10
#endif
#ifndef SIGQUIT
#define SIGQUIT 3
#endif
#endif

int
sim_signal_to_host (SIM_DESC sd, SIM_SIGNAL sig)
{
  switch (sig)
    {
    case SIM_SIGINT :
      return SIGINT;

    case SIM_SIGABRT :
      return SIGABRT;

    case SIM_SIGILL :
#ifdef SIGILL
      return SIGILL;
#else
      return SIGSEGV;
#endif

    case SIM_SIGTRAP :
      return SIGTRAP;

    case SIM_SIGBUS :
#ifdef SIGBUS
      return SIGBUS;
#else
      return SIGSEGV;
#endif

    case SIM_SIGSEGV :
      return SIGSEGV;

    case SIM_SIGXCPU :
#ifdef SIGXCPU
      return SIGXCPU;
#endif
      break;

    case SIM_SIGFPE:
#ifdef SIGFPE
      return SIGFPE;
#endif
      break;

    case SIM_SIGNONE:
      return 0;
      break;
    }

  sim_io_eprintf (sd, "sim_signal_to_host: unknown signal: %d\n", sig);
#ifdef SIGHUP
  return SIGHUP;  /* FIXME: Suggestions?  */
#else
  return 1;
#endif
}

enum gdb_signal
sim_signal_to_gdb_signal (SIM_DESC sd, SIM_SIGNAL sig)
{
  switch (sig)
    {
    case SIM_SIGINT :
      return GDB_SIGNAL_INT;

    case SIM_SIGABRT :
      return GDB_SIGNAL_ABRT;

    case SIM_SIGILL :
      return GDB_SIGNAL_ILL;

    case SIM_SIGTRAP :
      return GDB_SIGNAL_TRAP;

    case SIM_SIGBUS :
      return GDB_SIGNAL_BUS;

    case SIM_SIGSEGV :
      return GDB_SIGNAL_SEGV;

    case SIM_SIGXCPU :
      return GDB_SIGNAL_XCPU;

    case SIM_SIGFPE:
      return GDB_SIGNAL_FPE;
      break;

    case SIM_SIGNONE:
      return GDB_SIGNAL_0;
      break;
    }

  sim_io_eprintf (sd, "sim_signal_to_host: unknown signal: %d\n", sig);
  return GDB_SIGNAL_HUP;
}
