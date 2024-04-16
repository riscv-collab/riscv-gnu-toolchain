/* Functions and data responsible for forking the inferior process.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#ifndef NAT_FORK_INFERIOR_H
#define NAT_FORK_INFERIOR_H

#include <string>
#include "gdbsupport/function-view.h"

struct process_stratum_target;

/* Number of traps that happen between exec'ing the shell to run an
   inferior and when we finally get to the inferior code, not counting
   the exec for the shell.  This is 1 on all supported
   implementations.  */
#define START_INFERIOR_TRAPS_EXPECTED 1

/* Start an inferior Unix child process and sets inferior_ptid to its
   pid.  EXEC_FILE is the file to run.  ALLARGS is a string containing
   the arguments to the program.  ENV is the environment vector to
   pass.  SHELL_FILE is the shell file, or NULL if we should pick
   one.  EXEC_FUN is the exec(2) function to use, or NULL for the default
   one.  */

/* This function is NOT reentrant.  Some of the variables have been
   made static to ensure that they survive the vfork call.  */
extern pid_t fork_inferior (const char *exec_file_arg,
			    const std::string &allargs,
			    char **env, void (*traceme_fun) (),
			    gdb::function_view<void (int)> init_trace_fun,
			    void (*pre_trace_fun) (),
			    const char *shell_file_arg,
			    void (*exec_fun) (const char *file,
					      char * const *argv,
					      char * const *env));

/* Accept NTRAPS traps from the inferior.

   Return the ptid of the inferior being started.  */
extern ptid_t startup_inferior (process_stratum_target *proc_target,
				pid_t pid, int ntraps,
				struct target_waitstatus *mystatus,
				ptid_t *myptid);

/* Perform any necessary tasks before a fork/vfork takes place.  ARGS
   is a string containing all the arguments received by the inferior.
   This function is mainly used by fork_inferior.  */
extern void prefork_hook (const char *args);

/* Perform any necessary tasks after a fork/vfork takes place.  This
   function is mainly used by fork_inferior.  */
extern void postfork_hook (pid_t pid);

/* Perform any necessary tasks *on the child* after a fork/vfork takes
   place.  This function is mainly used by fork_inferior.  */
extern void postfork_child_hook ();

/* Flush both stdout and stderr.  This function needs to be
   implemented differently on GDB and GDBserver.  */
extern void gdb_flush_out_err ();

/* Report an error that happened when starting to trace the inferior
   (i.e., when the "traceme_fun" callback is called on fork_inferior)
   and bail out.  This function does not return.  */
extern void trace_start_error (const char *fmt, ...)
  ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (1, 2);

/* Like "trace_start_error", but the error message is constructed by
   combining STRING with the system error message for errno.  This
   function does not return.  */
extern void trace_start_error_with_name (const char *string)
  ATTRIBUTE_NORETURN;

#endif /* NAT_FORK_INFERIOR_H */
