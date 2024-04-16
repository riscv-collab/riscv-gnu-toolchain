/* Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

/* Support for printing a backtrace when GDB hits an error.  This is not
   for printing backtraces of the inferior, but backtraces of GDB itself.  */

#ifndef BT_UTILS_H
#define BT_UTILS_H

#ifdef HAVE_LIBBACKTRACE
# include "backtrace.h"
# include "backtrace-supported.h"
# if BACKTRACE_SUPPORTED && (! BACKTRACE_USES_MALLOC)
#  define GDB_PRINT_INTERNAL_BACKTRACE
#  define GDB_PRINT_INTERNAL_BACKTRACE_USING_LIBBACKTRACE
# endif
#endif

#if defined HAVE_EXECINFO_H			\
  && defined HAVE_EXECINFO_BACKTRACE		\
  && !defined GDB_PRINT_INTERNAL_BACKTRACE_USING_LIBBACKTRACE
# include <execinfo.h>
# define GDB_PRINT_INTERNAL_BACKTRACE
# define GDB_PRINT_INTERNAL_BACKTRACE_USING_EXECINFO
#endif

/* Define GDB_PRINT_INTERNAL_BACKTRACE_INIT_ON.  This is a boolean value
   that can be used as an initial value for a set/show user setting, where
   the setting controls printing a GDB internal backtrace.

   If backtrace printing is supported then this will have the value true,
   but if backtrace printing is not supported then this has the value
   false.  */
#ifdef GDB_PRINT_INTERNAL_BACKTRACE
# define GDB_PRINT_INTERNAL_BACKTRACE_INIT_ON true
#else
# define GDB_PRINT_INTERNAL_BACKTRACE_INIT_ON false
#endif

/* Print a backtrace of the current GDB process to the current
   gdb_stderr.  The output is done in a signal async manner, so it is safe
   to call from signal handlers.  */

extern void gdb_internal_backtrace ();

/* A generic function that can be used as the set function for any set
   command that enables printing of an internal backtrace.  Command C must
   be a boolean set command.

   If GDB doesn't support printing a backtrace, and the user has tried to
   set the variable associated with command C to true, then the associated
   variable will be set back to false, and an error thrown.

   If GDB does support printing a backtrace then this function does
   nothing.  */

extern void gdb_internal_backtrace_set_cmd (const char *args, int from_tty,
					    cmd_list_element *c);

#endif /* BT_UTILS_H */
