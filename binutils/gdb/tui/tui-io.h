/* TUI support I/O functions.

   Copyright (C) 1998-2024 Free Software Foundation, Inc.

   Contributed by Hewlett-Packard Company.

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

#ifndef TUI_TUI_IO_H
#define TUI_TUI_IO_H

#include "gdb_curses.h"

struct ui_out;
class cli_ui_out;

/* Print the string in the given curses window.  If no window is
   provided, the command window is used.  */
extern void tui_puts (const char *, WINDOW * = nullptr);

/* Print LENGTH characters from the buffer pointed to by BUF to the
   curses command window.  */
extern void tui_write (const char *buf, size_t length);

/* Setup the IO for curses or non-curses mode.  */
extern void tui_setup_io (int mode);

/* Initialize the IO for gdb in curses mode.  */
extern void tui_initialize_io (void);

/* Readline callback.
   Redisplay the command line with its prompt after readline has
   changed the edited text.  */
extern void tui_redisplay_readline (void);

/* Enter/leave reverse video mode.  */
extern void tui_set_reverse_mode (WINDOW *w, bool reverse);

/* Apply STYLE to the window.  */
extern void tui_apply_style (WINDOW *w, ui_file_style style);

extern struct ui_out *tui_out;
extern cli_ui_out *tui_old_uiout;

/* This should be called when the user has entered a command line in tui
   mode.  Inject the newline into the output and move the cursor to the
   next line.  */
extern void tui_inject_newline_into_command_window ();

#endif /* TUI_TUI_IO_H */
