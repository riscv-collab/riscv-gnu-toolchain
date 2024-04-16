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

#ifndef UI_H
#define UI_H

#include "gdbsupport/event-loop.h"
#include "gdbsupport/intrusive_list.h"
#include "gdbsupport/next-iterator.h"

struct interp;

/* Prompt state.  */

enum prompt_state
{
  /* The command line is blocked simulating synchronous execution.
     This is used to implement the foreground execution commands
     ('run', 'continue', etc.).  We won't display the prompt and
     accept further commands until the execution is actually over.  */
  PROMPT_BLOCKED,

  /* The command finished; display the prompt before returning back to
     the top level.  */
  PROMPT_NEEDED,

  /* We've displayed the prompt already, ready for input.  */
  PROMPTED,
};

/* All about a user interface instance.  Each user interface has its
   own I/O files/streams, readline state, its own top level
   interpreter (for the main UI, this is the interpreter specified
   with -i on the command line) and secondary interpreters (for
   interpreter-exec ...), etc.  There's always one UI associated with
   stdin/stdout/stderr, but the user can create secondary UIs, for
   example, to create a separate MI channel on its own stdio
   streams.  */

struct ui
{
  /* Create a new UI.  */
  ui (FILE *instream, FILE *outstream, FILE *errstream);
  ~ui ();

  DISABLE_COPY_AND_ASSIGN (ui);

  /* Pointer to next in singly-linked list.  */
  struct ui *next = nullptr;

  /* Convenient handle (UI number).  Unique across all UIs.  */
  int num;

  /* The UI's command line buffer.  This is to used to accumulate
     input until we have a whole command line.  */
  std::string line_buffer;

  /* The callback used by the event loop whenever an event is detected
     on the UI's input file descriptor.  This function incrementally
     builds a buffer where it accumulates the line read up to the
     point of invocation.  In the special case in which the character
     read is newline, the function invokes the INPUT_HANDLER callback
     (see below).  */
  void (*call_readline) (gdb_client_data) = nullptr;

  /* The function to invoke when a complete line of input is ready for
     processing.  */
  void (*input_handler) (gdb::unique_xmalloc_ptr<char> &&) = nullptr;

  /* True if this UI is using the readline library for command
     editing; false if using GDB's own simple readline emulation, with
     no editing support.  */
  int command_editing = 0;

  /* Each UI has its own independent set of interpreters.  */
  intrusive_list<interp> interp_list;
  interp *current_interpreter = nullptr;
  interp *top_level_interpreter = nullptr;

  /* The interpreter that is active while `interp_exec' is active, NULL
     at all other times.  */
  interp *command_interpreter = nullptr;

  /* True if the UI is in async mode, false if in sync mode.  If in
     sync mode, a synchronous execution command (e.g, "next") does not
     return until the command is finished.  If in async mode, then
     running a synchronous command returns right after resuming the
     target.  Waiting for the command's completion is later done on
     the top event loop.  For the main UI, this starts out disabled,
     until all the explicit command line arguments (e.g., `gdb -ex
     "start" -ex "next"') are processed.  */
  int async = 0;

  /* The number of nested readline secondary prompts that are
     currently active.  */
  int secondary_prompt_depth = 0;

  /* The UI's stdin.  Set to stdin for the main UI.  */
  FILE *stdin_stream;

  /* stdio stream that command input is being read from.  Set to stdin
     normally.  Set by source_command to the file we are sourcing.
     Set to NULL if we are executing a user-defined command or
     interacting via a GUI.  */
  FILE *instream;
  /* Standard output stream.  */
  FILE *outstream;
  /* Standard error stream.  */
  FILE *errstream;

  /* The file descriptor for the input stream, so that we can register
     it with the event loop.  This can be set to -1 to prevent this
     registration.  */
  int input_fd;

  /* Whether ISATTY returns true on input_fd.  Cached here because
     quit_force needs to know this _after_ input_fd might be
     closed.  */
  bool m_input_interactive_p;

  /* See enum prompt_state's description.  */
  enum prompt_state prompt_state = PROMPT_NEEDED;

  /* The fields below that start with "m_" are "private".  They're
     meant to be accessed through wrapper macros that make them look
     like globals.  */

  /* The ui_file streams.  */
  /* Normal results */
  struct ui_file *m_gdb_stdout;
  /* Input stream */
  struct ui_file *m_gdb_stdin;
  /* Serious error notifications */
  struct ui_file *m_gdb_stderr;
  /* Log/debug/trace messages that should bypass normal stdout/stderr
     filtering.  */
  struct ui_file *m_gdb_stdlog;

  /* The current ui_out.  */
  struct ui_out *m_current_uiout = nullptr;

  /* Register the UI's input file descriptor in the event loop.  */
  void register_file_handler ();

  /* Unregister the UI's input file descriptor from the event loop.  */
  void unregister_file_handler ();

  /* Return true if this UI's input fd is a tty.  */
  bool input_interactive_p () const;
};

/* The main UI.  This is the UI that is bound to stdin/stdout/stderr.
   It always exists and is created automatically when GDB starts
   up.  */
extern struct ui *main_ui;

/* The current UI.  */
extern struct ui *current_ui;

/* The list of all UIs.  */
extern struct ui *ui_list;

/* State for SWITCH_THRU_ALL_UIS.  */
class switch_thru_all_uis
{
public:

  switch_thru_all_uis () : m_iter (ui_list), m_save_ui (&current_ui)
  {
    current_ui = ui_list;
  }

  DISABLE_COPY_AND_ASSIGN (switch_thru_all_uis);

  /* If done iterating, return true; otherwise return false.  */
  bool done () const
  {
    return m_iter == NULL;
  }

  /* Move to the next UI, setting current_ui if iteration is not yet
     complete.  */
  void next ()
  {
    m_iter = m_iter->next;
    if (m_iter != NULL)
      current_ui = m_iter;
  }

 private:

  /* Used to iterate through the UIs.  */
  struct ui *m_iter;

  /* Save and restore current_ui.  */
  scoped_restore_tmpl<struct ui *> m_save_ui;
};

  /* Traverse through all UI, and switch the current UI to the one
     being iterated.  */
#define SWITCH_THRU_ALL_UIS()		\
  for (switch_thru_all_uis stau_state; !stau_state.done (); stau_state.next ())

using ui_range = next_range<ui>;

/* An adapter that can be used to traverse over all UIs.  */
static inline
ui_range all_uis ()
{
  return ui_range (ui_list);
}

#endif /* UI_H */
