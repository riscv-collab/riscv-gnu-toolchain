/* Copyright (C) 2023-2024 Free Software Foundation, Inc.

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
#include "ui.h"

#include "cli/cli-cmds.h"
#include "event-top.h"
#include "gdbsupport/buildargv.h"
#include "gdbsupport/filestuff.h"
#include "gdbsupport/gdb_file.h"
#include "gdbsupport/scoped_fd.h"
#include "interps.h"
#include "pager.h"
#include "main.h"
#include "top.h"

/* See top.h.  */

struct ui *main_ui;
struct ui *current_ui;
struct ui *ui_list;

/* The highest UI number ever assigned.  */

static int highest_ui_num;

/* See top.h.  */

ui::ui (FILE *instream_, FILE *outstream_, FILE *errstream_)
  : num (++highest_ui_num),
    stdin_stream (instream_),
    instream (instream_),
    outstream (outstream_),
    errstream (errstream_),
    input_fd (fileno (instream)),
    m_input_interactive_p (ISATTY (instream)),
    m_gdb_stdout (new pager_file (new stdio_file (outstream))),
    m_gdb_stdin (new stdio_file (instream)),
    m_gdb_stderr (new stderr_file (errstream)),
    m_gdb_stdlog (new timestamped_file (m_gdb_stderr))
{
  unbuffer_stream (instream_);

  if (ui_list == NULL)
    ui_list = this;
  else
    {
      struct ui *last;

      for (last = ui_list; last->next != NULL; last = last->next)
	;
      last->next = this;
    }
}

ui::~ui ()
{
  struct ui *ui, *uiprev;

  uiprev = NULL;

  for (ui = ui_list; ui != NULL; uiprev = ui, ui = ui->next)
    if (ui == this)
      break;

  gdb_assert (ui != NULL);

  if (uiprev != NULL)
    uiprev->next = next;
  else
    ui_list = next;

  delete m_gdb_stdin;
  delete m_gdb_stdout;
  delete m_gdb_stderr;
}


/* Returns whether GDB is running on an interactive terminal.  */

bool
ui::input_interactive_p () const
{
  if (batch_flag)
    return false;

  if (interactive_mode != AUTO_BOOLEAN_AUTO)
    return interactive_mode == AUTO_BOOLEAN_TRUE;

  return m_input_interactive_p;
}


/* When there is an event ready on the stdin file descriptor, instead
   of calling readline directly throught the callback function, or
   instead of calling gdb_readline_no_editing_callback, give gdb a
   chance to detect errors and do something.  */

static void
stdin_event_handler (int error, gdb_client_data client_data)
{
  struct ui *ui = (struct ui *) client_data;

  if (error)
    {
      /* Switch to the main UI, so diagnostics always go there.  */
      current_ui = main_ui;

      ui->unregister_file_handler ();
      if (main_ui == ui)
	{
	  /* If stdin died, we may as well kill gdb.  */
	  gdb_printf (gdb_stderr, _("error detected on stdin\n"));
	  quit_command ((char *) 0, 0);
	}
      else
	{
	  /* Simply delete the UI.  */
	  delete ui;
	}
    }
  else
    {
      /* Switch to the UI whose input descriptor woke up the event
	 loop.  */
      current_ui = ui;

      /* This makes sure a ^C immediately followed by further input is
	 always processed in that order.  E.g,. with input like
	 "^Cprint 1\n", the SIGINT handler runs, marks the async
	 signal handler, and then select/poll may return with stdin
	 ready, instead of -1/EINTR.  The
	 gdb.base/double-prompt-target-event-error.exp test exercises
	 this.  */
      QUIT;

      do
	{
	  call_stdin_event_handler_again_p = 0;
	  ui->call_readline (client_data);
	}
      while (call_stdin_event_handler_again_p != 0);
    }
}

/* See top.h.  */

void
ui::register_file_handler ()
{
  if (input_fd != -1)
    add_file_handler (input_fd, stdin_event_handler, this,
		      string_printf ("ui-%d", num), true);
}

/* See top.h.  */

void
ui::unregister_file_handler ()
{
  if (input_fd != -1)
    delete_file_handler (input_fd);
}

/* Open file named NAME for read/write, making sure not to make it the
   controlling terminal.  */

static gdb_file_up
open_terminal_stream (const char *name)
{
  scoped_fd fd = gdb_open_cloexec (name, O_RDWR | O_NOCTTY, 0);
  if (fd.get () < 0)
    perror_with_name  (_("opening terminal failed"));

  return fd.to_file ("w+");
}

/* Implementation of the "new-ui" command.  */

static void
new_ui_command (const char *args, int from_tty)
{
  int argc;
  const char *interpreter_name;
  const char *tty_name;

  dont_repeat ();

  gdb_argv argv (args);
  argc = argv.count ();

  if (argc < 2)
    error (_("Usage: new-ui INTERPRETER TTY"));

  interpreter_name = argv[0];
  tty_name = argv[1];

  {
    scoped_restore save_ui = make_scoped_restore (&current_ui);

    /* Open specified terminal.  Note: we used to open it three times,
       once for each of stdin/stdout/stderr, but that does not work
       with Windows named pipes.  */
    gdb_file_up stream = open_terminal_stream (tty_name);

    std::unique_ptr<ui> ui
      (new struct ui (stream.get (), stream.get (), stream.get ()));

    ui->async = 1;

    current_ui = ui.get ();

    set_top_level_interpreter (interpreter_name);

    top_level_interpreter ()->pre_command_loop ();

    /* Make sure the file is not closed.  */
    stream.release ();

    ui.release ();
  }

  gdb_printf ("New UI allocated\n");
}

void _initialize_ui ();
void
_initialize_ui ()
{
  cmd_list_element *c = add_cmd ("new-ui", class_support, new_ui_command, _("\
Create a new UI.\n\
Usage: new-ui INTERPRETER TTY\n\
The first argument is the name of the interpreter to run.\n\
The second argument is the terminal the UI runs on."), &cmdlist);
  set_cmd_completer (c, interpreter_completer);
}
