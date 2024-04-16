/* TUI data manipulation routines.

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

#ifndef TUI_TUI_DATA_H
#define TUI_TUI_DATA_H

#include "tui/tui.h"
#include "gdb_curses.h"
#include "observable.h"
#include "gdbsupport/gdb-checked-static-cast.h"

/* A deleter that calls delwin.  */
struct curses_deleter
{
  void operator() (WINDOW *win) const
  {
    delwin (win);
  }
};

#define MIN_WIN_HEIGHT          3

/* Generic window information.  */
struct tui_win_info
{
protected:

  tui_win_info () = default;
  DISABLE_COPY_AND_ASSIGN (tui_win_info);

  /* This is called after the window is resized, and should update the
     window's contents.  */
  virtual void rerender ();

  virtual void make_window ();

public:
  tui_win_info (tui_win_info &&) = default;
  virtual ~tui_win_info () = default;

  /* Call to refresh this window.  */
  virtual void refresh_window ();

  /* Make this window visible or invisible.  */
  virtual void make_visible (bool visible);

  /* Return the name of this type of window.  */
  virtual const char *name () const = 0;

  /* Compute the maximum height of this window.  */
  virtual int max_height () const;

  /* Compute the minimum height of this window.  */
  virtual int min_height () const
  {
    return MIN_WIN_HEIGHT;
  }

  /* Compute the maximum width of this window.  */
  int max_width () const;

  /* Compute the minimum width of this window.  */
  int min_width () const
  {
    return 3;
  }

  /* Return true if this window can be boxed.  */
  virtual bool can_box () const
  {
    return true;
  }

  /* Return the width of the box.  */
  int box_width () const
  {
    return can_box () ? 1 : 0;
  }

  /* Return the size of the box.  */
  int box_size () const
  {
    return 2 * box_width ();
  }

  /* Resize this window.  The parameters are used to set the window's
     size and position.  */
  virtual void resize (int height, int width,
		       int origin_x, int origin_y);

  /* Return true if this window is visible.  */
  bool is_visible () const
  {
    return handle != nullptr && tui_active;
  }

  /* Return true if this window can accept the focus.  */
  virtual bool can_focus () const
  {
    return true;
  }

  /* Disable output until the next call to doupdate.  */
  void no_refresh ()
  {
    if (handle != nullptr)
      wnoutrefresh (handle.get ());
  }

  /* Called after the tab width has been changed.  */
  virtual void update_tab_width ()
  {
  }

  /* Set whether this window is highlighted.  */
  void set_highlight (bool highlight)
  {
    is_highlighted = highlight;
  }

  /* Methods to scroll the contents of this window.  Note that they
     are named with "_scroll" coming at the end because the more
     obvious "scroll_forward" is defined as a macro in term.h.  */
  void forward_scroll (int num_to_scroll);
  void backward_scroll (int num_to_scroll);
  void left_scroll (int num_to_scroll);
  void right_scroll (int num_to_scroll);

  /* Return true if this window can be scrolled, false otherwise.  */
  virtual bool can_scroll () const
  {
    return true;
  }

  /* Called for each mouse click inside this window.  Coordinates MOUSE_X
     and MOUSE_Y are 0-based relative to the window, and MOUSE_BUTTON can
     be 1 (left), 2 (middle), or 3 (right).  */
  virtual void click (int mouse_x, int mouse_y, int mouse_button)
  {
  }

  void check_and_display_highlight_if_needed ();

  /* A helper function to change the title and then redraw the
     surrounding box, if needed.  */
  void set_title (std::string &&new_title);

  /* Return a reference to the current window title.  */
  const std::string &title () const
  { return m_title; }

  /* Display string STR in the window at position (Y,X), abbreviated if
     necessary.  */
  void display_string (int y, int x, const char *str) const;

  /* Display string STR in the window at the current cursor position,
     abbreviated if necessary.  */
  void display_string (const char *str) const;

  /* Window handle.  */
  std::unique_ptr<WINDOW, curses_deleter> handle;
  /* Window width.  */
  int width = 0;
  /* Window height.  */
  int height = 0;
  /* Origin of window.  */
  int x = 0;
  int y = 0;

  /* Is this window highlighted?  */
  bool is_highlighted = false;

protected:

  /* Scroll the contents vertically.  This is only called via
     forward_scroll and backward_scroll.  */
  virtual void do_scroll_vertical (int num_to_scroll) = 0;

  /* Scroll the contents horizontally.  This is only called via
     left_scroll and right_scroll.  */
  virtual void do_scroll_horizontal (int num_to_scroll) = 0;

private:
  /* Window title to display.  */
  std::string m_title;
};

/* A TUI window that doesn't scroll.  */

struct tui_noscroll_window : public virtual tui_win_info
{
public:
  virtual bool can_scroll () const final override
  {
    return false;
  }

protected:
  virtual void do_scroll_vertical (int num_to_scroll) final override
  {
  }

  /* Scroll the contents horizontally.  This is only called via
     left_scroll and right_scroll.  */
  virtual void do_scroll_horizontal (int num_to_scroll) final override
  {
  }
};

/* A TUI window that cannot have focus.  */

struct tui_nofocus_window : public virtual tui_win_info
{
public:
  virtual bool can_focus () const final override
  {
    return false;
  }
};

/* A TUI window that occupies a single line.  */

struct tui_oneline_window : public virtual tui_win_info
{
  int max_height () const final override
  {
    return 1;
  }

  int min_height () const final override
  {
    return 1;
  }
};

/* A TUI window that has no border.  */

struct tui_nobox_window : public virtual tui_win_info
{
  bool can_box () const final override
  {
    return false;
  }
};

/* A TUI window that is not refreshed.  */

struct tui_norefresh_window : public virtual tui_win_info
{
  virtual void refresh_window () final override
  {
  }
};

/* A TUI window that is always visible.  */

struct tui_always_visible_window : public virtual tui_win_info
{
  virtual void make_visible (bool visible) final override
  {
  }
};

/* Constant definitions.  */
#define SRC_NAME                "src"
#define CMD_NAME                "cmd"
#define DATA_NAME               "regs"
#define DISASSEM_NAME           "asm"
#define STATUS_NAME		"status"

/* Global Data.  */
extern struct tui_win_info *tui_win_list[MAX_MAJOR_WINDOWS];

#define TUI_SRC_WIN \
  (gdb::checked_static_cast<tui_source_window *> (tui_win_list[SRC_WIN]))
#define TUI_DISASM_WIN \
  (gdb::checked_static_cast<tui_disasm_window *> (tui_win_list[DISASSEM_WIN]))
#define TUI_DATA_WIN \
  (gdb::checked_static_cast<tui_data_window *> (tui_win_list[DATA_WIN]))
#define TUI_CMD_WIN \
  (gdb::checked_static_cast<tui_cmd_window *> (tui_win_list[CMD_WIN]))
#define TUI_STATUS_WIN \
  (gdb::checked_static_cast<tui_status_window *> (tui_win_list[STATUS_WIN]))

/* All the windows that are currently instantiated, in layout
   order.  */
extern std::vector<tui_win_info *> tui_windows;

/* Return a range adapter for iterating over TUI windows.  */
static inline std::vector<tui_win_info *> &
all_tui_windows ()
{
  return tui_windows;
}

/* Data Manipulation Functions.  */
extern int tui_term_height (void);
extern void tui_set_term_height_to (int);
extern int tui_term_width (void);
extern void tui_set_term_width_to (int);
extern struct tui_win_info *tui_win_with_focus (void);
extern bool tui_win_resized ();
extern void tui_set_win_resized_to (bool);

extern struct tui_win_info *tui_next_win (struct tui_win_info *);
extern struct tui_win_info *tui_prev_win (struct tui_win_info *);

extern unsigned int tui_tab_width;

#endif /* TUI_TUI_DATA_H */
