/* Functions to deal with the inferior being executed on GDB or
   GDBserver.

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

#ifndef COMMON_COMMON_INFERIOR_H
#define COMMON_COMMON_INFERIOR_H

#include "gdbsupport/array-view.h"

/* Return the exec wrapper to be used when starting the inferior, or NULL
   otherwise.  */
extern const char *get_exec_wrapper ();

/* Return the name of the executable file as a string.
   ERR nonzero means get error if there is none specified;
   otherwise return 0 in that case.  */
extern const char *get_exec_file (int err);

/* Return the inferior's current working directory.

   If it is not set, the string is empty.  */
extern const std::string &get_inferior_cwd ();

/* Whether to start up the debuggee under a shell.

   If startup-with-shell is set, GDB's "run" will attempt to start up
   the debuggee under a shell.  This also happens when using GDBserver
   under extended remote mode.

   This is in order for argument-expansion to occur.  E.g.,

   (gdb) run *

   The "*" gets expanded by the shell into a list of files.

   While this is a nice feature, it may be handy to bypass the shell
   in some cases.  To disable this feature, do "set startup-with-shell
   false".

   The catch-exec traps expected during start-up will be one more if
   the target is started up with a shell.  */
extern bool startup_with_shell;

/* Compute command-line string given argument vector. This does the
   same shell processing as fork_inferior.  */
extern std::string
construct_inferior_arguments (gdb::array_view<char * const>);

#endif /* COMMON_COMMON_INFERIOR_H */
