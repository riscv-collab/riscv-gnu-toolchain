/* Copyright (C) 2012-2024 Free Software Foundation, Inc.

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
#include "command.h"
#include "gdbcmd.h"
#include "target.h"
#include "gdbsupport/agent.h"
#include "observable.h"
#include "objfiles.h"

/* Enum strings for "set|show agent".  */

static const char can_use_agent_on[] = "on";
static const char can_use_agent_off[] = "off";
static const char * const can_use_agent_enum[] =
{
  can_use_agent_on,
  can_use_agent_off,
  NULL,
};

static const char *can_use_agent = can_use_agent_off;

static void
show_can_use_agent (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  gdb_printf (file,
	      _("Debugger's willingness to use agent in inferior "
		"as a helper is %s.\n"), value);
}

static void
set_can_use_agent (const char *args, int from_tty, struct cmd_list_element *c)
{
  bool can_use = (can_use_agent == can_use_agent_on);
  if (can_use && !agent_loaded_p ())
    {
      /* Since the setting was off, we may not have observed the objfiles and
	 therefore not looked up the required symbols.  Do so now.  */
      for (objfile *objfile : current_program_space->objfiles ())
	if (agent_look_up_symbols (objfile) == 0)
	  break;
    }
  if (target_use_agent (can_use) == 0)
    /* Something wrong during setting, set flag to default value.  */
    can_use_agent = can_use_agent_off;
}

static void
agent_new_objfile (struct objfile *objfile)
{
  if (agent_loaded_p ())
    return;

  if (can_use_agent == can_use_agent_off)
    return;

  agent_look_up_symbols (objfile);
}

void _initialize_agent ();
void
_initialize_agent ()
{
  gdb::observers::new_objfile.attach (agent_new_objfile,
				      "agent");

  add_setshow_enum_cmd ("agent", class_run,
			can_use_agent_enum,
			&can_use_agent, _("\
Set debugger's willingness to use agent as a helper."), _("\
Show debugger's willingness to use agent as a helper."), _("\
If on, GDB will delegate some of the debugging operations to the\n\
agent, if the target supports it.  This will speed up those\n\
operations that are supported by the agent.\n\
If off, GDB will not use agent, even if such is supported by the\n\
target."),
			set_can_use_agent,
			show_can_use_agent,
			&setlist, &showlist);
}
