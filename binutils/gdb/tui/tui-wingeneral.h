/* General window behavior.

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

#ifndef TUI_TUI_WINGENERAL_H
#define TUI_TUI_WINGENERAL_H

#include "gdb_curses.h"

struct tui_win_info;

extern void tui_unhighlight_win (struct tui_win_info *);
extern void tui_highlight_win (struct tui_win_info *);
extern void tui_refresh_all ();

/* An RAII class that suppresses output on construction (calling
   wnoutrefresh on the existing windows), and then flushes the output
   (via doupdate) when destroyed.  */

class tui_suppress_output
{
public:

  tui_suppress_output ();
  ~tui_suppress_output ();

  DISABLE_COPY_AND_ASSIGN (tui_suppress_output);

private:

  /* Save the state of the suppression global.  */
  bool m_saved_suppress;
};

/* Call wrefresh on the given window.  However, if output is being
   suppressed via tui_suppress_output, do not call wrefresh.  */
extern void tui_wrefresh (WINDOW *win);

#endif /* TUI_TUI_WINGENERAL_H */
