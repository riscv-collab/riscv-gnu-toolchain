/* Header file for GDB CLI command implementation library.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#ifndef CLI_CLI_CMDS_H
#define CLI_CLI_CMDS_H

#include "gdbsupport/filestuff.h"
#include <optional>
#include "completer.h"

/* Chain containing all defined commands.  */

extern struct cmd_list_element *cmdlist;

/* Chain containing all defined info subcommands.  */

extern struct cmd_list_element *infolist;

/* Chain containing all defined enable subcommands.  */

extern struct cmd_list_element *enablelist;

/* Chain containing all defined disable subcommands.  */

extern struct cmd_list_element *disablelist;

/* Chain containing all defined delete subcommands.  */

extern struct cmd_list_element *deletelist;

/* Chain containing all defined detach subcommands.  */

extern struct cmd_list_element *detachlist;

/* Chain containing all defined kill subcommands.  */

extern struct cmd_list_element *killlist;

/* Chain containing all defined stop subcommands.  */

extern struct cmd_list_element *stoplist;

/* Chain containing all defined set subcommands */

extern struct cmd_list_element *setlist;

/* Chain containing all defined unset subcommands */

extern struct cmd_list_element *unsetlist;

/* Chain containing all defined show subcommands.  */

extern struct cmd_list_element *showlist;

/* Chain containing all defined \"set history\".  */

extern struct cmd_list_element *sethistlist;

/* Chain containing all defined \"show history\".  */

extern struct cmd_list_element *showhistlist;

/* Chain containing all defined \"unset history\".  */

extern struct cmd_list_element *unsethistlist;

/* Chain containing all defined maintenance subcommands.  */

extern struct cmd_list_element *maintenancelist;

/* Chain containing all defined "maintenance info" subcommands.  */

extern struct cmd_list_element *maintenanceinfolist;

/* Chain containing all defined "maintenance print" subcommands.  */

extern struct cmd_list_element *maintenanceprintlist;

/* Chain containing all defined "maintenance flush" subcommands.  */

extern struct cmd_list_element *maintenanceflushlist;

/* Chain containing all defined "maintenance check" subcommands.  */

extern struct cmd_list_element *maintenancechecklist;

/* Chain containing all defined "maintenance set" subcommands.  */

extern struct cmd_list_element *maintenance_set_cmdlist;

/* Chain containing all defined "maintenance show" subcommands.  */

extern struct cmd_list_element *maintenance_show_cmdlist;

extern struct cmd_list_element *setprintlist;

extern struct cmd_list_element *showprintlist;

/* Chain containing all defined "set print raw" subcommands.  */

extern struct cmd_list_element *setprintrawlist;

/* Chain containing all defined "show print raw" subcommands.  */

extern struct cmd_list_element *showprintrawlist;

/* Chain containing all defined "set print type" subcommands.  */

extern struct cmd_list_element *setprinttypelist;

/* Chain containing all defined "show print type" subcommands.  */

extern struct cmd_list_element *showprinttypelist;

extern struct cmd_list_element *setdebuglist;

extern struct cmd_list_element *showdebuglist;

extern struct cmd_list_element *setchecklist;

extern struct cmd_list_element *showchecklist;

/* Chain containing all defined "save" subcommands.  */

extern struct cmd_list_element *save_cmdlist;

/* Chain containing all defined "set source" subcommands.  */

extern struct cmd_list_element *setsourcelist;

/* Chain containing all defined "show source" subcommands.  */

extern struct cmd_list_element *showsourcelist;

/* Limit the call depth of user-defined commands */

extern unsigned int max_user_call_depth;

/* Exported to gdb/top.c */

int is_complete_command (struct cmd_list_element *cmd);

/* Exported to gdb/main.c */

extern void cd_command (const char *, int);

/* Exported to gdb/top.c and gdb/main.c */

extern void quit_command (const char *, int);

extern void source_script (const char *, int);

/* Exported to objfiles.c.  */

/* The script that was opened.  */
struct open_script
{
  gdb_file_up stream;
  gdb::unique_xmalloc_ptr<char> full_path;

  open_script (gdb_file_up &&stream_,
	       gdb::unique_xmalloc_ptr<char> &&full_path_)
    : stream (std::move (stream_)),
      full_path (std::move (full_path_))
  {
  }
};

extern std::optional<open_script>
    find_and_open_script (const char *file, int search_path);

/* Command tracing state.  */

extern int source_verbose;
extern bool trace_commands;

/* Common code for the "with" and "maintenance with" commands.
   SET_CMD_PREFIX is the spelling of the corresponding "set" command
   prefix: i.e., "set " or "maintenance set ".  SETLIST is the command
   element for the same "set" command prefix.  */
extern void with_command_1 (const char *set_cmd_prefix,
			    cmd_list_element *setlist,
			    const char *args, int from_tty);

/* Common code for the completers of the "with" and "maintenance with"
   commands.  SET_CMD_PREFIX is the spelling of the corresponding
   "set" command prefix: i.e., "set " or "maintenance set ".  */
extern void with_command_completer_1 (const char *set_cmd_prefix,
				      completion_tracker &tracker,
				      const char *text);

#endif /* CLI_CLI_CMDS_H */
