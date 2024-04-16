/* GDB Notifications to Observers.

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

#include "defs.h"
#include "observable.h"
#include "command.h"
#include "gdbcmd.h"

namespace gdb
{

namespace observers
{

bool observer_debug = false;

#define DEFINE_OBSERVABLE(name) decltype (name) name (# name)

DEFINE_OBSERVABLE (normal_stop);
DEFINE_OBSERVABLE (signal_received);
DEFINE_OBSERVABLE (target_changed);
DEFINE_OBSERVABLE (executable_changed);
DEFINE_OBSERVABLE (inferior_created);
DEFINE_OBSERVABLE (inferior_execd);
DEFINE_OBSERVABLE (inferior_forked);
DEFINE_OBSERVABLE (solib_loaded);
DEFINE_OBSERVABLE (solib_unloaded);
DEFINE_OBSERVABLE (new_objfile);
DEFINE_OBSERVABLE (all_objfiles_removed);
DEFINE_OBSERVABLE (free_objfile);
DEFINE_OBSERVABLE (new_thread);
DEFINE_OBSERVABLE (thread_exit);
DEFINE_OBSERVABLE (thread_deleted);
DEFINE_OBSERVABLE (thread_stop_requested);
DEFINE_OBSERVABLE (target_resumed);
DEFINE_OBSERVABLE (about_to_proceed);
DEFINE_OBSERVABLE (breakpoint_created);
DEFINE_OBSERVABLE (breakpoint_deleted);
DEFINE_OBSERVABLE (breakpoint_modified);
DEFINE_OBSERVABLE (new_architecture);
DEFINE_OBSERVABLE (thread_ptid_changed);
DEFINE_OBSERVABLE (inferior_added);
DEFINE_OBSERVABLE (inferior_appeared);
DEFINE_OBSERVABLE (inferior_pre_detach);
DEFINE_OBSERVABLE (inferior_exit);
DEFINE_OBSERVABLE (inferior_removed);
DEFINE_OBSERVABLE (inferior_cloned);
DEFINE_OBSERVABLE (memory_changed);
DEFINE_OBSERVABLE (before_prompt);
DEFINE_OBSERVABLE (gdb_datadir_changed);
DEFINE_OBSERVABLE (inferior_call_pre);
DEFINE_OBSERVABLE (inferior_call_post);
DEFINE_OBSERVABLE (register_changed);
DEFINE_OBSERVABLE (user_selected_context_changed);
DEFINE_OBSERVABLE (styling_changed);
DEFINE_OBSERVABLE (current_source_symtab_and_line_changed);
DEFINE_OBSERVABLE (gdb_exiting);
DEFINE_OBSERVABLE (connection_removed);
DEFINE_OBSERVABLE (target_pre_wait);
DEFINE_OBSERVABLE (target_post_wait);
DEFINE_OBSERVABLE (new_program_space);
DEFINE_OBSERVABLE (free_program_space);

} /* namespace observers */
} /* namespace gdb */

static void
show_observer_debug (struct ui_file *file, int from_tty,
		     struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Observer debugging is %s.\n"), value);
}

void _initialize_observer ();
void
_initialize_observer ()
{
  add_setshow_boolean_cmd ("observer", class_maintenance,
			   &gdb::observers::observer_debug, _("\
Set observer debugging."), _("\
Show observer debugging."), _("\
When non-zero, observer debugging is enabled."),
			   NULL,
			   show_observer_debug,
			   &setdebuglist, &showdebuglist);
}
