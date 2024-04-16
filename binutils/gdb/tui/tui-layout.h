/* TUI layout window management.

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

#ifndef TUI_TUI_LAYOUT_H
#define TUI_TUI_LAYOUT_H

#include "ui-file.h"

#include "tui/tui.h"
#include "tui/tui-data.h"
#include "gdbsupport/iterator-range.h"

#include <unordered_map>

/* Values that can be returned when handling a request to adjust a
   window's size.  */
enum tui_adjust_result
{
  /* Requested window was not found here.  */
  NOT_FOUND,
  /* Window was found but not handled.  */
  FOUND,
  /* Window was found and handled.  */
  HANDLED
};

/* The basic object in a TUI layout.  This represents a single piece
   of screen real estate.  Subclasses determine the exact
   behavior.  */
class tui_layout_base
{
public:

  DISABLE_COPY_AND_ASSIGN (tui_layout_base);

  virtual ~tui_layout_base () = default;

  /* Clone this object.  Ordinarily a layout is cloned before it is
     used, so that any necessary modifications do not affect the
     "skeleton" layout.  */
  virtual std::unique_ptr<tui_layout_base> clone () const = 0;

  /* Change the size and location of this layout.  When
     PRESERVE_CMD_WIN_SIZE_P is true the current size of the TUI_CMD_WIN
     is preserved, otherwise, the TUI_CMD_WIN will resize just like any
     other window.  */
  virtual void apply (int x, int y, int width, int height,
		      bool preserve_cmd_win_size_p) = 0;

  /* Return the minimum and maximum height or width of this layout.
     HEIGHT is true to fetch height, false to fetch width.  */
  virtual void get_sizes (bool height, int *min_value, int *max_value) = 0;

  /* True if the topmost (for vertical layouts), or the leftmost (for
     horizontal layouts) item in this layout is boxed.  */
  virtual bool first_edge_has_border_p () const = 0;

  /* True if the bottommost (for vertical layouts), or the rightmost (for
     horizontal layouts) item in this layout is boxed.  */
  virtual bool last_edge_has_border_p () const = 0;

  /* Return the name of this layout's window, or nullptr if this
     layout does not represent a single window.  */
  virtual const char *get_name () const
  {
    return nullptr;
  }

  /* Set the height of the window named NAME to NEW_HEIGHT, updating
     the sizes of the other windows around it.  */
  virtual tui_adjust_result set_height (const char *name, int new_height) = 0;

  /* Set the width of the window named NAME to NEW_WIDTH, updating
     the sizes of the other windows around it.  */
  virtual tui_adjust_result set_width (const char *name, int new_width) = 0;

  /* Remove some windows from the layout, leaving the command window
     and the window being passed in here.  */
  virtual void remove_windows (const char *name) = 0;

  /* Replace the window named NAME in the layout with the window named
     NEW_WINDOW.  */
  virtual void replace_window (const char *name, const char *new_window) = 0;

  /* Append the specification to this window to OUTPUT.  DEPTH is the
     depth of this layout in the hierarchy (zero-based).  */
  virtual void specification (ui_file *output, int depth) = 0;

  /* Return a FINGERPRINT string containing an abstract representation of
     the location of the cmd window in this layout.

     When called on a complete, top-level layout, the fingerprint will be a
     non-empty string made of 'V' and 'H' characters, followed by a single
     'C' character.  Each 'V' and 'H' represents a vertical or horizontal
     layout that must be passed through in order to find the cmd
     window.  A vertical or horizontal layout of just one window does not add
     a 'V' or 'H' character.

     Of course, layouts are built recursively, so, when called on a partial
     layout, if this object represents a single window, then either the
     empty string is returned (for non-cmd windows), or a string
     containing a single 'C' is returned.

     For object representing layouts, if the layout contains the cmd
     window then we will get back a valid fingerprint string (may contain 'V'
     and 'H', ends with 'C'), or, if this layout doesn't contain the cmd
     window, an empty string is returned.  */
  virtual std::string layout_fingerprint () const = 0;

  /* Add all windows to the WINDOWS vector.  */
  virtual void get_windows (std::vector<tui_win_info *> *windows) = 0;

  /* The most recent space allocation.  */
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;

protected:

  tui_layout_base () = default;
};

/* A TUI layout object that displays a single window.  The window is
   given by name.  */
class tui_layout_window : public tui_layout_base
{
public:

  explicit tui_layout_window (const char *name)
    : m_contents (name)
  {
  }

  DISABLE_COPY_AND_ASSIGN (tui_layout_window);

  std::unique_ptr<tui_layout_base> clone () const override;

  void apply (int x, int y, int width, int height,
	      bool preserve_cmd_win_size_p) override;

  const char *get_name () const override
  {
    return m_contents.c_str ();
  }

  tui_adjust_result set_height (const char *name, int new_height) override
  {
    return m_contents == name ? FOUND : NOT_FOUND;
  }

  tui_adjust_result set_width (const char *name, int new_width) override
  {
    return m_contents == name ? FOUND : NOT_FOUND;
  }

  bool first_edge_has_border_p () const override;

  bool last_edge_has_border_p () const override;

  void remove_windows (const char *name) override
  {
  }

  void replace_window (const char *name, const char *new_window) override;

  void specification (ui_file *output, int depth) override;

  std::string layout_fingerprint () const override;

  /* See tui_layout_base::get_windows.  */
  void get_windows (std::vector<tui_win_info *> *windows) override
  {
    if (m_window != nullptr && m_window->is_visible ())
      {
	/* Only get visible windows.  */
	windows->push_back (m_window);
      }
  }

protected:

  void get_sizes (bool height, int *min_value, int *max_value) override;

private:

  /* Type of content to display.  */
  std::string m_contents;

  /* When a layout is applied, this is updated to point to the window
     object.  */
  tui_win_info *m_window = nullptr;
};

/* A TUI layout that holds other layouts.  */
class tui_layout_split : public tui_layout_base
{
public:

  /* Create a new layout.  If VERTICAL is true, then windows in this
     layout will be arranged vertically.  */
  explicit tui_layout_split (bool vertical = true)
    : m_vertical (vertical)
  {
  }

  DISABLE_COPY_AND_ASSIGN (tui_layout_split);

  /* Add a new split layout to this layout.  WEIGHT is the desired
     size, which is relative to the other weights given in this
     layout.  */
  void add_split (std::unique_ptr<tui_layout_split> &&layout, int weight);

  /* Add a new window to this layout.  NAME is the name of the window
     to add.  WEIGHT is the desired size, which is relative to the
     other weights given in this layout.  */
  void add_window (const char *name, int weight);

  std::unique_ptr<tui_layout_base> clone () const override;

  void apply (int x, int y, int width, int height,
	      bool preserve_cmd_win_size_p) override;

  tui_adjust_result set_height (const char *name, int new_height) override
  {
    /* Pass false as the final argument to indicate change of height.  */
    return set_size (name, new_height, false);
  }

  tui_adjust_result set_width (const char *name, int new_width) override
  {
    /* Pass true as the final argument to indicate change of width.  */
    return set_size (name, new_width, true);
  }

  bool first_edge_has_border_p () const override;

  bool last_edge_has_border_p () const override;

  void remove_windows (const char *name) override;

  void replace_window (const char *name, const char *new_window) override;

  void specification (ui_file *output, int depth) override;

  std::string layout_fingerprint () const override;

  /* See tui_layout_base::get_windows.  */
  void get_windows (std::vector<tui_win_info *> *windows) override
  {
    for (auto &item : m_splits)
      item.layout->get_windows (windows);
  }

protected:

  void get_sizes (bool height, int *min_value, int *max_value) override;

private:

  /* Used to implement set_height and set_width member functions.  When
     SET_WIDTH_P is true, set the width, otherwise, set the height of the
     window named NAME to NEW_SIZE, updating the sizes of the other windows
     around it as needed.  The result indicates if the window NAME was
     found and had its size adjusted, was found but was not adjusted, or
     was not found at all.  */
  tui_adjust_result set_size (const char *name, int new_size,
			      bool set_width_p);

  /* Set the weights from the current heights (when m_vertical is true) or
     widths (when m_vertical is false).  */
  void set_weights_from_sizes ();

  /* Structure used when resizing, or applying a layout.  An instance of
     this structure is created for each sub-layout.  */
  struct size_info
  {
    /* The calculated size for this sub-layout.  */
    int size;

    /* The minimum and maximum sizes for this sub-layout, obtained by
       calling the get_sizes member function.  */
    int min_size;
    int max_size;

    /* True if this window will share a box border with the previous
       window in the list.  */
    bool share_box;
  };

  /* Used for debug, prints the contents of INFO using tui_debug_printf.
     Only call this when the global debug_tui is true.  */
  static void tui_debug_print_size_info (const std::vector<size_info> &info);

  /* Used for debug, returns a string describing the current weight of each
     sub-layout.  */
  std::string tui_debug_weights_to_string () const;

  struct split
  {
    /* The requested weight.  */
    int weight;
    /* The layout.  */
    std::unique_ptr<tui_layout_base> layout;
  };

  /* The splits.  */
  std::vector<split> m_splits;

  /* True if the windows in this split are arranged vertically.  */
  bool m_vertical;
};

/* Add the specified window to the layout in a logical way.  This
   means setting up the most logical layout given the window to be
   added.  Only the source or disassembly window can be added this
   way.  */
extern void tui_add_win_to_layout (enum tui_win_type);

/* Set the initial layout.  */
extern void tui_set_initial_layout ();

/* Switch to the next layout.  */
extern void tui_next_layout ();

/* Show the register window.  Like "layout regs".  */
extern void tui_regs_layout ();

/* Remove some windows from the layout, leaving only the focused
   window and the command window; if no window has the focus, then
   some other window is chosen to remain.  */
extern void tui_remove_some_windows ();

/* Apply the current layout.  When PRESERVE_CMD_WIN_SIZE_P is true the
   current size of the TUI_CMD_WIN is preserved, otherwise, the TUI_CMD_WIN
   will resize just like any other window.  */
extern void tui_apply_current_layout (bool);

/* Adjust the window height of WIN to NEW_HEIGHT.  */
extern void tui_adjust_window_height (struct tui_win_info *win,
				      int new_height);

/* Adjust the window width of WIN to NEW_WIDTH.  */
extern void tui_adjust_window_width (struct tui_win_info *win,
				     int new_width);

/* The type of a function that is used to create a TUI window.  */

typedef std::function<tui_win_info * (const char *name)> window_factory;

/* The type for a data structure that maps a window name to that window's
   factory function.  */
typedef std::unordered_map<std::string, window_factory> window_types_map;

/* Register a new TUI window type.  NAME is the name of the window
   type.  FACTORY is a function that can be called to instantiate the
   window.  */

extern void tui_register_window (const char *name, window_factory &&factory);

/* An iterator class that exposes just the window names from the
   known_window_types map in tui-layout.c.  This is just a wrapper around
   an iterator of the underlying known_window_types map, but this just
   exposes the window names.  */

struct known_window_names_iterator
{
  using known_window_types_iterator = window_types_map::iterator;

  known_window_names_iterator (known_window_types_iterator &&iter)
    : m_iter (std::move (iter))
  { /* Nothing.  */ }

  known_window_names_iterator &operator++ ()
  {
    ++m_iter;
    return *this;
  }

  const std::string &operator* () const
  { return (*m_iter).first; }

  bool operator!= (const known_window_names_iterator &other) const
  { return m_iter != other.m_iter; }

private:

  /* The underlying iterator.  */
  known_window_types_iterator m_iter;
};

/* A range adapter that makes it possible to iterate over the names of all
   known tui windows.  */

using known_window_names_range
  = iterator_range<known_window_names_iterator>;

/* Return a range that can be used to walk over the name of all known tui
   windows in a range-for loop.  */

extern known_window_names_range all_known_window_names ();

#endif /* TUI_TUI_LAYOUT_H */
