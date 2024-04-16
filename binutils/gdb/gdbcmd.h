/* ***DEPRECATED***  The gdblib files must not be calling/using things in any
   of the possible command languages.  If necessary, a hook (that may be
   present or not) must be used and set to the appropriate routine by any
   command language that cares about it.  If you are having to include this
   file you are possibly doing things the old way.  This file will disappear.
   fnasser@redhat.com    */

/* Header file for GDB-specific command-line stuff.
   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#if !defined (GDBCMD_H)
#define GDBCMD_H 1

#include "command.h"
#include "ui-out.h"
#include "cli/cli-script.h"
#include "cli/cli-cmds.h"

extern void execute_command (const char *, int);

/* Run FN.  Sends its output to FILE, do not display it to the screen.
   The global BATCH_FLAG will be temporarily set to true.  */

extern void execute_fn_to_ui_file (struct ui_file *file, std::function<void(void)> fn);

/* Run FN.  Capture its output into the returned string, do not display it
   to the screen.  The global BATCH_FLAG will temporarily be set to true.
   When TERM_OUT is true the output is collected with terminal behaviour
   (e.g. with styling).  When TERM_OUT is false raw output will be collected
   (e.g. no styling).  */

extern void execute_fn_to_string (std::string &res,
				  std::function<void(void)> fn, bool term_out);

/* As execute_fn_to_ui_file, but run execute_command for P and FROM_TTY.  */

extern void execute_command_to_ui_file (struct ui_file *file,
					const char *p, int from_tty);

/* As execute_fn_to_string, but run execute_command for P and FROM_TTY.  */

extern void execute_command_to_string (std::string &res, const char *p,
				       int from_tty, bool term_out);

/* As execute_command_to_string, but ignore resulting string.  */

extern void execute_command_to_string (const char *p,
				       int from_tty, bool term_out);

extern void print_command_line (struct command_line *, unsigned int,
				struct ui_file *);
extern void print_command_lines (struct ui_out *,
				 struct command_line *, unsigned int);

/* Chains containing all defined "set/show style" subcommands.  */
extern struct cmd_list_element *style_set_list;
extern struct cmd_list_element *style_show_list;

#endif /* !defined (GDBCMD_H) */
