/* MI Internal Functions for GDB, the GNU debugger.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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

#ifndef MI_MI_MAIN_H
#define MI_MI_MAIN_H

struct ui_file;

extern void mi_load_progress (const char *section_name,
			      unsigned long sent_so_far,
			      unsigned long total_section,
			      unsigned long total_sent,
			      unsigned long grand_total);

extern void mi_print_timing_maybe (struct ui_file *file);

/* Whether MI is in async mode.  */

extern int mi_async_p (void);

struct mi_suppress_notification
{
  /* Breakpoint notification suppressed?  */
  int breakpoint;
  /* Command param changed notification suppressed?  */
  int cmd_param_changed;
  /* Traceframe changed notification suppressed?  */
  int traceframe;
  /* Memory changed notification suppressed?  */
  int memory;
  /* User selected context changed notification suppressed?  */
  int user_selected_context;
};
extern struct mi_suppress_notification mi_suppress_notification;

/* This is a hack so we can get some extra commands going, but has existed
   within GDB for many years now.  Ideally we don't want to channel things
   through the CLI, but implement all commands as pure MI commands with
   their own implementation.

   Execute the CLI command CMD, if ARGS_P is true then ARGS should be a
   non-nullptr string containing arguments to add after CMD.  If ARGS_P is
   false then ARGS must be nullptr.  */

extern void mi_execute_cli_command (const char *cmd, bool args_p,
				    const char *args);

/* Implementation of -fix-multi-location-breakpoint-output.  */

extern void mi_cmd_fix_multi_location_breakpoint_output (const char *command,
							 const char *const *argv,
							 int argc);

/* Implementation of -fix-breakpoint-script-output.  */

extern void mi_cmd_fix_breakpoint_script_output (const char *command,
						 const char *const *argv,
						 int argc);

/* Parse a thread-group-id from ID, and return the integer part of the
   ID.  A valid thread-group-id is the character 'i' followed by an
   integer that is greater than zero.  */

extern int mi_parse_thread_group_id (const char *id);

#endif /* MI_MI_MAIN_H */
