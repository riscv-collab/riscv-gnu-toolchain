/* TUI window generic functions.

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

#ifndef TUI_TUI_WIN_H
#define TUI_TUI_WIN_H

#include "tui/tui-data.h"

extern void tui_set_win_focus_to (struct tui_win_info *);
extern void tui_resize_all (void);
extern void tui_refresh_all_win (void);
extern void tui_rehighlight_all (void);

extern chtype tui_border_ulcorner;
extern chtype tui_border_urcorner;
extern chtype tui_border_lrcorner;
extern chtype tui_border_llcorner;
extern chtype tui_border_vline;
extern chtype tui_border_hline;
extern int tui_border_attrs;
extern int tui_active_border_attrs;

extern bool tui_update_variables ();

extern void tui_initialize_win (void);

/* Update gdb's knowledge of the terminal size.  */
extern void tui_update_gdb_sizes (void);

/* Create or get the TUI command list.  */
struct cmd_list_element **tui_get_cmd_list (void);

/* Whether compact source display should be used.  */
extern bool compact_source;

/* Whether the TUI should intercept terminal mouse events.  */
extern bool tui_enable_mouse;

/* Whether to style the source and assembly code highlighted by the TUI's
   current position indicator.  */
extern bool style_tui_current_position;

/* Whether to replace the spaces in the left margin with '_' and '0'.  */
extern bool tui_left_margin_verbose;

#endif /* TUI_TUI_WIN_H */
