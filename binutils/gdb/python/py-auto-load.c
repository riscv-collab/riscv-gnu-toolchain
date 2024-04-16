/* GDB routines for supporting auto-loaded scripts.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.

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

#include "defs.h"
#include "top.h"
#include "gdbcmd.h"
#include "objfiles.h"
#include "python.h"
#include "auto-load.h"
#include "python-internal.h"

/* User-settable option to enable/disable auto-loading of Python scripts:
   set auto-load python-scripts on|off
   This is true if we should auto-load associated Python scripts when an
   objfile is opened, false otherwise.  */
static bool auto_load_python_scripts = true;

/* "show" command for the auto_load_python_scripts configuration variable.  */

static void
show_auto_load_python_scripts (struct ui_file *file, int from_tty,
			       struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Auto-loading of Python scripts is %s.\n"), value);
}

/* See python-internal.h.  */

bool
gdbpy_auto_load_enabled (const struct extension_language_defn *extlang)
{
  return auto_load_python_scripts;
}

/* Wrapper for "info auto-load python-scripts".  */

static void
info_auto_load_python_scripts (const char *pattern, int from_tty)
{
  auto_load_info_scripts (current_program_space, pattern, from_tty,
			  &extension_language_python);
}

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_auto_load (void)
{
  add_setshow_boolean_cmd ("python-scripts", class_support,
			   &auto_load_python_scripts, _("\
Set the debugger's behaviour regarding auto-loaded Python scripts."), _("\
Show the debugger's behaviour regarding auto-loaded Python scripts."), _("\
If enabled, auto-loaded Python scripts are loaded when the debugger reads\n\
an executable or shared library.\n\
This options has security implications for untrusted inferiors."),
			   NULL, show_auto_load_python_scripts,
			   auto_load_set_cmdlist_get (),
			   auto_load_show_cmdlist_get ());

  set_show_commands auto_load_scripts_cmds
    = add_setshow_boolean_cmd ("auto-load-scripts", class_support,
			       &auto_load_python_scripts, _("\
Set the debugger's behaviour regarding auto-loaded Python scripts, "
								 "deprecated."),
			       _("\
Show the debugger's behaviour regarding auto-loaded Python scripts, "
								 "deprecated."),
			       NULL, NULL, show_auto_load_python_scripts,
			       &setlist, &showlist);
  deprecate_cmd (auto_load_scripts_cmds.set, "set auto-load python-scripts");
  deprecate_cmd (auto_load_scripts_cmds.show, "show auto-load python-scripts");

  add_cmd ("python-scripts", class_info, info_auto_load_python_scripts,
	   _("Print the list of automatically loaded Python scripts.\n\
Usage: info auto-load python-scripts [REGEXP]"),
	   auto_load_info_cmdlist_get ());

  cmd_list_element *info_auto_load_scripts_cmd
    = add_info ("auto-load-scripts", info_auto_load_python_scripts, _("\
Print the list of automatically loaded Python scripts, deprecated."));
  deprecate_cmd (info_auto_load_scripts_cmd, "info auto-load python-scripts");

  return 0;
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_auto_load);
