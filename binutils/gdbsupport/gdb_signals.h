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

#ifndef COMMON_GDB_SIGNALS_H
#define COMMON_GDB_SIGNALS_H

#include "gdb/signals.h"

/* Predicate to gdb_signal_to_host(). Return non-zero if the enum
   targ_signal SIGNO has an equivalent ``host'' representation.  */
/* FIXME: cagney/1999-11-22: The name below was chosen in preference
   to the shorter gdb_signal_p() because it is far less ambiguous.
   In this context ``gdb_signal'' refers to GDB's internal
   representation of the target's set of signals while ``host signal''
   refers to the target operating system's signal.  Confused?  */
extern int gdb_signal_to_host_p (enum gdb_signal signo);

/* Convert between host signal numbers and enum gdb_signal's.
   gdb_signal_to_host() returns 0 and prints a warning() on GDB's
   console if SIGNO has no equivalent host representation.  */
/* FIXME: cagney/1999-11-22: Here ``host'' is used incorrectly, it is
   refering to the target operating system's signal numbering.
   Similarly, ``enum gdb_signal'' is named incorrectly, ``enum
   gdb_signal'' would probably be better as it is refering to GDB's
   internal representation of a target operating system's signal.  */
extern enum gdb_signal gdb_signal_from_host (int);
extern int gdb_signal_to_host (enum gdb_signal);

/* Return the enum symbol name of SIG as a string, to use in debug
   output.  */
extern const char *gdb_signal_to_symbol_string (enum gdb_signal sig);

/* Return the string for a signal.  */
extern const char *gdb_signal_to_string (enum gdb_signal);

/* Return the name (SIGHUP, etc.) for a signal.  */
extern const char *gdb_signal_to_name (enum gdb_signal);

/* Given a name (SIGHUP, etc.), return its signal.  */
enum gdb_signal gdb_signal_from_name (const char *);

#endif /* COMMON_GDB_SIGNALS_H */
