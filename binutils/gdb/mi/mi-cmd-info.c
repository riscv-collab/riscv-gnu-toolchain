/* MI Command Set - information commands.
   Copyright (C) 2011-2024 Free Software Foundation, Inc.

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
#include "osdata.h"
#include "mi-cmds.h"
#include "ada-lang.h"
#include "arch-utils.h"

/* Implement the "-info-ada-exceptions" GDB/MI command.  */

void
mi_cmd_info_ada_exceptions (const char *command, const char *const *argv,
			    int argc)
{
  struct ui_out *uiout = current_uiout;
  struct gdbarch *gdbarch = get_current_arch ();
  const char *regexp;

  switch (argc)
    {
    case 0:
      regexp = NULL;
      break;
    case 1:
      regexp = argv[0];
      break;
    default:
      error (_("Usage: -info-ada-exceptions [REGEXP]"));
      break;
    }

  std::vector<ada_exc_info> exceptions = ada_exceptions_list (regexp);

  ui_out_emit_table table_emitter (uiout, 2,
				   exceptions.size (),
				   "ada-exceptions");
  uiout->table_header (1, ui_left, "name", "Name");
  uiout->table_header (1, ui_left, "address", "Address");
  uiout->table_body ();

  for (const ada_exc_info &info : exceptions)
    {
      ui_out_emit_tuple tuple_emitter (uiout, NULL);
      uiout->field_string ("name", info.name);
      uiout->field_core_addr ("address", gdbarch, info.addr);
    }
}

/* Implement the "-info-gdb-mi-command" GDB/MI command.  */

void
mi_cmd_info_gdb_mi_command (const char *command, const char *const *argv,
			    int argc)
{
  const char *cmd_name;
  mi_command *cmd;
  struct ui_out *uiout = current_uiout;

  /* This command takes exactly one argument.  */
  if (argc != 1)
    error (_("Usage: -info-gdb-mi-command MI_COMMAND_NAME"));
  cmd_name = argv[0];

  /* Normally, the command name (aka the "operation" in the GDB/MI
     grammar), does not include the leading '-' (dash).  But for
     the user's convenience, allow the user to specify the command
     name to be with or without that leading dash.  */
  if (cmd_name[0] == '-')
    cmd_name++;

  cmd = mi_cmd_lookup (cmd_name);

  ui_out_emit_tuple tuple_emitter (uiout, "command");
  uiout->field_string ("exists", cmd != NULL ? "true" : "false");
}

void
mi_cmd_info_os (const char *command, const char *const *argv, int argc)
{
  switch (argc)
    {
    case 0:
      info_osdata (NULL);
      break;
    case 1:
      info_osdata (argv[0]);
      break;
    default:
      error (_("Usage: -info-os [INFOTYPE]"));
      break;
    }
}
