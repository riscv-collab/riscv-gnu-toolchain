/* Debugging routines for the remote server for GDB.
   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_DEBUG_H
#define GDBSERVER_DEBUG_H

#if !defined (IN_PROCESS_AGENT)
extern bool remote_debug;

/* Print a "remote" debug statement.  */

#define remote_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (remote_debug, \
			      "remote", fmt, ##__VA_ARGS__)

/* Switch all debug output to DEBUG_FILE.  If DEBUG_FILE is nullptr or an
   empty string, or if the file cannot be opened, then debug output is sent to
   stderr.  */
void debug_set_output (const char *debug_file);
#endif

extern int using_threads;

/* Enable miscellaneous debugging output.  The name is historical - it
   was originally used to debug LinuxThreads support.  */

extern bool debug_threads;

/* Print a "threads" debug statement.  */

#define threads_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (debug_threads, \
			      "threads", fmt, ##__VA_ARGS__)

/* Print "threads" enter/exit debug statements.  */

#define THREADS_SCOPED_DEBUG_ENTER_EXIT \
  scoped_debug_enter_exit (debug_threads, "threads")

extern int debug_timestamp;

void debug_flush (void);

/* Async signal safe debug output function that calls write directly.  */
ssize_t debug_write (const void *buf, size_t nbyte);

#endif /* GDBSERVER_DEBUG_H */
