/* Specific command window processing.

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

#ifndef TUI_TUI_COMMAND_H
#define TUI_TUI_COMMAND_H

#include "tui/tui-data.h"

/* The TUI command window.  */
struct tui_cmd_window
  : public tui_noscroll_window, tui_nobox_window, tui_norefresh_window,
    tui_always_visible_window
{
  tui_cmd_window () = default;

  DISABLE_COPY_AND_ASSIGN (tui_cmd_window);

  const char *name () const override
  {
    return CMD_NAME;
  }

  void resize (int height, int width, int origin_x, int origin_y) override;

  /* Compute the minimum height of this window.  */
  virtual int min_height () const override
  {
    int preferred_min = tui_win_info::min_height ();
    int max = max_height ();
    /* If there is enough space to accommodate the preferred minimum height,
       use it.  Otherwise, use as much as possible.  */
    return (preferred_min <= max
	    ? preferred_min
	    : max);
  }

  int start_line = 0;
};

/* Refresh the command window.  */
extern void tui_refresh_cmd_win (void);

#endif /* TUI_TUI_COMMAND_H */
