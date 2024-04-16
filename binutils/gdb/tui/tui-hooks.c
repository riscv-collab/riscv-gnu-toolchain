/* GDB hooks for TUI.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "inferior.h"
#include "command.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "target.h"
#include "gdbcore.h"
#include "gdbsupport/event-loop.h"
#include "event-top.h"
#include "frame.h"
#include "breakpoint.h"
#include "ui-out.h"
#include "top.h"
#include "observable.h"
#include "source.h"
#include <unistd.h>
#include <fcntl.h>

#include "tui/tui.h"
#include "tui/tui-hooks.h"
#include "tui/tui-data.h"
#include "tui/tui-layout.h"
#include "tui/tui-io.h"
#include "tui/tui-regs.h"
#include "tui/tui-win.h"
#include "tui/tui-status.h"
#include "tui/tui-winsource.h"

#include "gdb_curses.h"

static void tui_on_objfiles_changed ()
{
  if (tui_active)
    tui_display_main ();
}

static void
tui_new_objfile_hook (struct objfile* objfile)
{ tui_on_objfiles_changed (); }

static void
tui_all_objfiles_removed (program_space *pspace)
{ tui_on_objfiles_changed (); }

/* Prevent recursion of deprecated_register_changed_hook().  */
static bool tui_refreshing_registers = false;

/* Observer for the register_changed notification.  */

static void
tui_register_changed (frame_info_ptr frame, int regno)
{
  frame_info_ptr fi;

  if (!tui_is_window_visible (DATA_WIN))
    return;

  /* The frame of the register that was changed may differ from the selected
     frame, but we only want to show the register values of the selected frame.
     And even if the frames differ a register change made in one can still show
     up in the other.  So we always use the selected frame here, and ignore
     FRAME.  */
  fi = get_selected_frame (NULL);
  if (!tui_refreshing_registers)
    {
      tui_refreshing_registers = true;
      TUI_DATA_WIN->check_register_values (fi);
      tui_refreshing_registers = false;
    }
}

/* Breakpoint creation hook.
   Update the screen to show the new breakpoint.  */
static void
tui_event_create_breakpoint (struct breakpoint *b)
{
  tui_update_all_breakpoint_info (nullptr);
}

/* Breakpoint deletion hook.
   Refresh the screen to update the breakpoint marks.  */
static void
tui_event_delete_breakpoint (struct breakpoint *b)
{
  tui_update_all_breakpoint_info (b);
}

static void
tui_event_modify_breakpoint (struct breakpoint *b)
{
  tui_update_all_breakpoint_info (nullptr);
}

/* This is set to true if the next window refresh should come from the
   current stack frame.  */

static bool from_stack;

/* This is set to true if the next window refresh should come from the
   current source symtab.  */

static bool from_source_symtab;

/* Refresh TUI's frame and register information.  This is a hook intended to be
   used to update the screen after potential frame and register changes.  */

static void
tui_refresh_frame_and_register_information ()
{
  if (!from_stack && !from_source_symtab)
    return;

  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  if (from_stack && has_stack_frames ())
    {
      frame_info_ptr fi = get_selected_frame (NULL);

      /* Display the frame position (even if there is no symbols or
	 the PC is not known).  */
      bool frame_info_changed_p = tui_show_frame_info (fi);

      /* Refresh the register window if it's visible.  */
      if (tui_is_window_visible (DATA_WIN)
	  && (frame_info_changed_p || from_stack))
	{
	  tui_refreshing_registers = true;
	  TUI_DATA_WIN->check_register_values (fi);
	  tui_refreshing_registers = false;
	}
    }
  else if (!from_stack)
    {
      /* Make sure that the source window is displayed.  */
      tui_add_win_to_layout (SRC_WIN);

      struct symtab_and_line sal = get_current_source_symtab_and_line ();
      tui_update_source_windows_with_line (sal);
    }
}

/* Dummy callback for deprecated_print_frame_info_listing_hook which is called
   from print_frame_info.  */

static void
tui_dummy_print_frame_info_listing_hook (struct symtab *s,
					 int line,
					 int stopline, 
					 int noerror)
{
}

/* Perform all necessary cleanups regarding our module's inferior data
   that is required after the inferior INF just exited.  */

static void
tui_inferior_exit (struct inferior *inf)
{
  /* Leave the SingleKey mode to make sure the gdb prompt is visible.  */
  tui_set_key_mode (TUI_COMMAND_MODE);
  tui_show_frame_info (0);
  tui_display_main ();
}

/* Observer for the before_prompt notification.  */

static void
tui_before_prompt (const char *current_gdb_prompt)
{
  tui_refresh_frame_and_register_information ();
  from_stack = false;
  from_source_symtab = false;
}

/* Observer for the normal_stop notification.  */

static void
tui_normal_stop (struct bpstat *bs, int print_frame)
{
  from_stack = true;
}

/* Observer for user_selected_context_changed.  */

static void
tui_context_changed (user_selected_what ignore)
{
  from_stack = true;
}

/* Observer for current_source_symtab_and_line_changed.  */

static void
tui_symtab_changed ()
{
  from_source_symtab = true;
}

/* Token associated with observers registered while TUI hooks are
   installed.  */
static const gdb::observers::token tui_observers_token {};

/* Attach or detach a single observer, according to ATTACH.  */

template<typename T>
static void
attach_or_detach (T &observable, typename T::func_type func, bool attach)
{
  if (attach)
    observable.attach (func, tui_observers_token, "tui-hooks");
  else
    observable.detach (tui_observers_token);
}

/* Attach or detach TUI observers, according to ATTACH.  */

static void
tui_attach_detach_observers (bool attach)
{
  attach_or_detach (gdb::observers::breakpoint_created,
		    tui_event_create_breakpoint, attach);
  attach_or_detach (gdb::observers::breakpoint_deleted,
		    tui_event_delete_breakpoint, attach);
  attach_or_detach (gdb::observers::breakpoint_modified,
		    tui_event_modify_breakpoint, attach);
  attach_or_detach (gdb::observers::inferior_exit,
		    tui_inferior_exit, attach);
  attach_or_detach (gdb::observers::before_prompt,
		    tui_before_prompt, attach);
  attach_or_detach (gdb::observers::normal_stop,
		    tui_normal_stop, attach);
  attach_or_detach (gdb::observers::register_changed,
		    tui_register_changed, attach);
  attach_or_detach (gdb::observers::user_selected_context_changed,
		    tui_context_changed, attach);
  attach_or_detach (gdb::observers::current_source_symtab_and_line_changed,
		    tui_symtab_changed, attach);
}

/* Install the TUI specific hooks.  */
void
tui_install_hooks (void)
{
  /* If this hook is not set to something then print_frame_info will
     assume that the CLI, not the TUI, is active, and will print the frame info
     for us in such a way that we are not prepared to handle.  This hook is
     otherwise effectively obsolete.  */
  deprecated_print_frame_info_listing_hook
    = tui_dummy_print_frame_info_listing_hook;

  /* Install the event hooks.  */
  tui_attach_detach_observers (true);
}

/* Remove the TUI specific hooks.  */
void
tui_remove_hooks (void)
{
  deprecated_print_frame_info_listing_hook = 0;

  /* Remove our observers.  */
  tui_attach_detach_observers (false);
}

void _initialize_tui_hooks ();
void
_initialize_tui_hooks ()
{
  /* Install the permanent hooks.  */
  gdb::observers::new_objfile.attach (tui_new_objfile_hook, "tui-hooks");
  gdb::observers::all_objfiles_removed.attach (tui_all_objfiles_removed,
					       "tui-hooks");
}
