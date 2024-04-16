/* TUI Interpreter definitions for GDB, the GNU debugger.

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
#include "cli/cli-interp.h"
#include "interps.h"
#include "ui.h"
#include "event-top.h"
#include "gdbsupport/event-loop.h"
#include "ui-out.h"
#include "cli-out.h"
#include "tui/tui-data.h"
#include "tui/tui-win.h"
#include "tui/tui.h"
#include "tui/tui-io.h"
#include "infrun.h"
#include "observable.h"
#include "gdbthread.h"
#include "inferior.h"
#include "main.h"

/* Set to true when the TUI mode must be activated when we first start
   gdb.  */
static bool tui_start_enabled = false;

class tui_interp final : public cli_interp_base
{
public:
  explicit tui_interp (const char *name)
    : cli_interp_base (name)
  {}

  void init (bool top_level) override;
  void resume () override;
  void suspend () override;
  void exec (const char *command_str) override;
  ui_out *interp_ui_out () override;
};

/* Cleanup the tui before exiting.  */

static void
tui_exit (void)
{
  /* Disable the tui.  Curses mode is left leaving the screen in a
     clean state (see endwin()).  */
  tui_disable ();
}

/* These implement the TUI interpreter.  */

void
tui_interp::init (bool top_level)
{
  /* Install exit handler to leave the screen in a good shape.  */
  atexit (tui_exit);

  tui_initialize_io ();
  if (gdb_stdout->isatty ())
    {
      tui_ensure_readline_initialized ();

      /* This installs the SIGWINCH signal handler.  The handler needs to do
	 readline calls (to rl_resize_terminal), so it must not be installed
	 unless readline is properly initialized.  */
      tui_initialize_win ();
    }
}

/* Used as the command handler for the tui.  */

static void
tui_command_line_handler (gdb::unique_xmalloc_ptr<char> &&rl)
{
  /* When a tui enabled GDB is running in either tui mode or cli mode then
     it is always the tui interpreter that is in use.  As a result we end
     up in here even in standard cli mode.

     We only need to do any special actions when the tui is in use
     though.  When the tui is active the users return is not echoed to the
     screen as a result the display will not automatically move us to the
     next line.  Here we manually insert a newline character and move the
     cursor.  */
  if (tui_active)
    tui_inject_newline_into_command_window ();

  /* Now perform GDB's standard CLI command line handling.  */
  command_line_handler (std::move (rl));
}

void
tui_interp::resume ()
{
  struct ui *ui = current_ui;
  struct ui_file *stream;

  /* gdb_setup_readline will change gdb_stdout.  If the TUI was
     previously writing to gdb_stdout, then set it to the new
     gdb_stdout afterwards.  */

  stream = tui_old_uiout->set_stream (gdb_stdout);
  if (stream != gdb_stdout)
    {
      tui_old_uiout->set_stream (stream);
      stream = NULL;
    }

  gdb_setup_readline (1);

  ui->input_handler = tui_command_line_handler;

  if (stream != NULL)
    tui_old_uiout->set_stream (gdb_stdout);

  if (tui_start_enabled)
    tui_enable ();
}

void
tui_interp::suspend ()
{
  gdb_disable_readline ();
  tui_start_enabled = tui_active;
  tui_disable ();
}

ui_out *
tui_interp::interp_ui_out ()
{
  if (tui_active)
    return tui_out;
  else
    return tui_old_uiout;
}

void
tui_interp::exec (const char *command_str)
{
  internal_error (_("tui_exec called"));
}


/* Factory for TUI interpreters.  */

static struct interp *
tui_interp_factory (const char *name)
{
  return new tui_interp (name);
}

void _initialize_tui_interp ();
void
_initialize_tui_interp ()
{
  interp_factory_register (INTERP_TUI, tui_interp_factory);

  if (interpreter_p == INTERP_TUI)
    tui_start_enabled = true;

  if (interpreter_p == INTERP_CONSOLE)
    interpreter_p = INTERP_TUI;

  /* There are no observers here because the CLI interpreter's
     observers work for the TUI interpreter as well.  See
     cli-interp.c.  */
}
