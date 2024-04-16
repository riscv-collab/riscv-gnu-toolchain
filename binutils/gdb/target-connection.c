/* List of target connections for GDB.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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
#include "target-connection.h"

#include <map>

#include "inferior.h"
#include "target.h"
#include "observable.h"

/* A map between connection number and representative process_stratum
   target.  */
static std::map<int, process_stratum_target *> process_targets;

/* The highest connection number ever given to a target.  */
static int highest_target_connection_num;

/* See target-connection.h.  */

void
connection_list_add (process_stratum_target *t)
{
  if (t->connection_number == 0)
    {
      t->connection_number = ++highest_target_connection_num;
      process_targets[t->connection_number] = t;
    }
}

/* See target-connection.h.  */

void
connection_list_remove (process_stratum_target *t)
{
  /* Notify about the connection being removed before we reset the
     connection number to zero.  */
  gdb::observers::connection_removed.notify (t);
  process_targets.erase (t->connection_number);
  t->connection_number = 0;
}

/* See target-connection.h.  */

std::string
make_target_connection_string (process_stratum_target *t)
{
  if (t->connection_string () != NULL)
    return string_printf ("%s %s", t->shortname (),
			  t->connection_string ());
  else
    return t->shortname ();
}

/* Prints the list of target connections and their details on UIOUT.

   If REQUESTED_CONNECTIONS is not NULL, it's a list of GDB ids of the
   target connections that should be printed.  Otherwise, all target
   connections are printed.  */

static void
print_connection (struct ui_out *uiout, const char *requested_connections)
{
  int count = 0;
  size_t what_len = 0;

  /* Compute number of lines we will print.  */
  for (const auto &it : process_targets)
    {
      if (!number_is_in_list (requested_connections, it.first))
	continue;

      ++count;

      process_stratum_target *t = it.second;

      size_t l = make_target_connection_string (t).length ();
      if (l > what_len)
	what_len = l;
    }

  if (count == 0)
    {
      uiout->message (_("No connections.\n"));
      return;
    }

  ui_out_emit_table table_emitter (uiout, 4, process_targets.size (),
				   "connections");

  uiout->table_header (1, ui_left, "current", "");
  uiout->table_header (4, ui_left, "number", "Num");
  /* The text in the "what" column may include spaces.  Add one extra
     space to visually separate the What and Description columns a
     little better.  Compare:
      "* 1    remote :9999 Remote serial target in gdb-specific protocol"
      "* 1    remote :9999  Remote serial target in gdb-specific protocol"
  */
  uiout->table_header (what_len + 1, ui_left, "what", "What");
  uiout->table_header (17, ui_left, "description", "Description");

  uiout->table_body ();

  for (const auto &it : process_targets)
    {
      process_stratum_target *t = it.second;

      if (!number_is_in_list (requested_connections, t->connection_number))
	continue;

      ui_out_emit_tuple tuple_emitter (uiout, NULL);

      if (current_inferior ()->process_target () == t)
	uiout->field_string ("current", "*");
      else
	uiout->field_skip ("current");

      uiout->field_signed ("number", t->connection_number);

      uiout->field_string ("what", make_target_connection_string (t));

      uiout->field_string ("description", t->longname ());

      uiout->text ("\n");
    }
}

/* The "info connections" command.  */

static void
info_connections_command (const char *args, int from_tty)
{
  print_connection (current_uiout, args);
}

void _initialize_target_connection ();

void
_initialize_target_connection ()
{
  add_info ("connections", info_connections_command,
	    _("\
Target connections in use.\n\
Shows the list of target connections currently in use."));
}
