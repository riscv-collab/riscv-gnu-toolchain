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

#include "defs.h"
#include "arch-utils.h"
#include "command.h"
#include "symtab.h"
#include "frame.h"
#include "source.h"
#include "cli/cli-cmds.h"
#include "cli/cli-decode.h"
#include "cli/cli-utils.h"
#include <ctype.h>
#include <unordered_set>

#include "tui/tui.h"
#include "tui/tui-command.h"
#include "tui/tui-data.h"
#include "tui/tui-wingeneral.h"
#include "tui/tui-status.h"
#include "tui/tui-regs.h"
#include "tui/tui-win.h"
#include "tui/tui-winsource.h"
#include "tui/tui-disasm.h"
#include "tui/tui-layout.h"
#include "tui/tui-source.h"
#include "gdb_curses.h"
#include "gdbsupport/gdb-safe-ctype.h"

/* The layouts.  */
static std::vector<std::unique_ptr<tui_layout_split>> layouts;

/* The layout that is currently applied.  */
static std::unique_ptr<tui_layout_base> applied_layout;

/* The "skeleton" version of the layout that is currently applied.  */
static tui_layout_split *applied_skeleton;

/* The two special "regs" layouts.  Note that these aren't registered
   as commands and so can never be deleted.  */
static tui_layout_split *src_regs_layout;
static tui_layout_split *asm_regs_layout;

/* See tui-data.h.  */
std::vector<tui_win_info *> tui_windows;

/* See tui-layout.h.  */

void
tui_apply_current_layout (bool preserve_cmd_win_size_p)
{
  for (tui_win_info *win_info : tui_windows)
    win_info->make_visible (false);

  applied_layout->apply (0, 0, tui_term_width (), tui_term_height (),
			 preserve_cmd_win_size_p);

  /* Keep the list of internal windows up-to-date.  */
  for (int win_type = SRC_WIN; (win_type < MAX_MAJOR_WINDOWS); win_type++)
    if (tui_win_list[win_type] != nullptr
	&& !tui_win_list[win_type]->is_visible ())
      tui_win_list[win_type] = nullptr;

  /* This should always be made visible by a layout.  */
  gdb_assert (TUI_CMD_WIN != nullptr);
  gdb_assert (TUI_CMD_WIN->is_visible ());

  /* Get the new list of currently visible windows.  */
  std::vector<tui_win_info *> new_tui_windows;
  applied_layout->get_windows (&new_tui_windows);

  /* Now delete any window that was not re-applied.  */
  tui_win_info *focus = tui_win_with_focus ();
  for (tui_win_info *win_info : tui_windows)
    {
      if (!win_info->is_visible ())
	{
	  if (focus == win_info)
	    tui_set_win_focus_to (new_tui_windows[0]);
	  delete win_info;
	}
    }

  /* Replace the global list of active windows.  */
  tui_windows = std::move (new_tui_windows);
}

/* See tui-layout.  */

void
tui_adjust_window_height (struct tui_win_info *win, int new_height)
{
  applied_layout->set_height (win->name (), new_height);
}

/* See tui-layout.  */

void
tui_adjust_window_width (struct tui_win_info *win, int new_width)
{
  applied_layout->set_width (win->name (), new_width);
}

/* Set the current layout to LAYOUT.  */

static void
tui_set_layout (tui_layout_split *layout)
{
  std::string old_fingerprint;
  if (applied_layout != nullptr)
    old_fingerprint = applied_layout->layout_fingerprint ();

  applied_skeleton = layout;
  applied_layout = layout->clone ();

  std::string new_fingerprint = applied_layout->layout_fingerprint ();
  bool preserve_command_window_size
    = (TUI_CMD_WIN != nullptr && old_fingerprint == new_fingerprint);

  tui_apply_current_layout (preserve_command_window_size);
}

/* See tui-layout.h.  */

void
tui_add_win_to_layout (enum tui_win_type type)
{
  gdb_assert (type == SRC_WIN || type == DISASSEM_WIN);

  /* If the window already exists, no need to add it.  */
  if (tui_win_list[type] != nullptr)
    return;

  /* If the window we are trying to replace doesn't exist, we're
     done.  */
  enum tui_win_type other = type == SRC_WIN ? DISASSEM_WIN : SRC_WIN;
  if (tui_win_list[other] == nullptr)
    return;

  const char *name = type == SRC_WIN ? SRC_NAME : DISASSEM_NAME;
  applied_layout->replace_window (tui_win_list[other]->name (), name);
  tui_apply_current_layout (true);
}

/* Find LAYOUT in the "layouts" global and return its index.  */

static size_t
find_layout (tui_layout_split *layout)
{
  for (size_t i = 0; i < layouts.size (); ++i)
    {
      if (layout == layouts[i].get ())
	return i;
    }
  gdb_assert_not_reached ("layout not found!?");
}

/* Function to set the layout. */

static void
tui_apply_layout (const char *args, int from_tty, cmd_list_element *command)
{
  tui_layout_split *layout = (tui_layout_split *) command->context ();

  /* Make sure the curses mode is enabled.  */
  tui_enable ();
  tui_set_layout (layout);
}

/* See tui-layout.h.  */

void
tui_next_layout ()
{
  size_t index = find_layout (applied_skeleton);
  ++index;
  if (index == layouts.size ())
    index = 0;
  tui_set_layout (layouts[index].get ());
}

/* Implement the "layout next" command.  */

static void
tui_next_layout_command (const char *arg, int from_tty)
{
  tui_enable ();
  tui_next_layout ();
}

/* See tui-layout.h.  */

void
tui_set_initial_layout ()
{
  tui_set_layout (layouts[0].get ());
}

/* Implement the "layout prev" command.  */

static void
tui_prev_layout_command (const char *arg, int from_tty)
{
  tui_enable ();
  size_t index = find_layout (applied_skeleton);
  if (index == 0)
    index = layouts.size ();
  --index;
  tui_set_layout (layouts[index].get ());
}


/* See tui-layout.h.  */

void
tui_regs_layout ()
{
  /* If there's already a register window, we're done.  */
  if (TUI_DATA_WIN != nullptr)
    return;

  tui_set_layout (TUI_DISASM_WIN != nullptr
		  ? asm_regs_layout
		  : src_regs_layout);
}

/* Implement the "layout regs" command.  */

static void
tui_regs_layout_command (const char *arg, int from_tty)
{
  tui_enable ();
  tui_regs_layout ();
}

/* See tui-layout.h.  */

void
tui_remove_some_windows ()
{
  tui_win_info *focus = tui_win_with_focus ();

  if (strcmp (focus->name (), CMD_NAME) == 0)
    {
      /* Try leaving the source or disassembly window.  If neither
	 exists, just do nothing.  */
      focus = TUI_SRC_WIN;
      if (focus == nullptr)
	focus = TUI_DISASM_WIN;
      if (focus == nullptr)
	return;
    }

  applied_layout->remove_windows (focus->name ());
  tui_apply_current_layout (true);
}

void
tui_win_info::resize (int height_, int width_,
		      int origin_x_, int origin_y_)
{
  if (width == width_ && height == height_
      && x == origin_x_ && y == origin_y_
      && handle != nullptr)
    return;

  width = width_;
  height = height_;
  x = origin_x_;
  y = origin_y_;

  if (handle != nullptr)
    {
#ifdef HAVE_WRESIZE
      wresize (handle.get (), height, width);
      mvwin (handle.get (), y, x);
      wmove (handle.get (), 0, 0);
#else
      handle.reset (nullptr);
#endif
    }

  if (handle == nullptr)
    make_window ();

  rerender ();
}



/* Helper function to create one of the built-in (non-status)
   windows.  */

template<enum tui_win_type V, class T>
static tui_win_info *
make_standard_window (const char *)
{
  if (tui_win_list[V] == nullptr)
    tui_win_list[V] = new T ();
  return tui_win_list[V];
}

/* A map holding all the known window types, keyed by name.  */

static window_types_map known_window_types;

/* See tui-layout.h.  */

known_window_names_range
all_known_window_names ()
{
  auto begin = known_window_names_iterator (known_window_types.begin ());
  auto end = known_window_names_iterator (known_window_types.end ());
  return known_window_names_range (begin, end);
}

/* Helper function that returns a TUI window, given its name.  */

static tui_win_info *
tui_get_window_by_name (const std::string &name)
{
  for (tui_win_info *window : tui_windows)
    if (name == window->name ())
      return window;

  auto iter = known_window_types.find (name);
  if (iter == known_window_types.end ())
    error (_("Unknown window type \"%s\""), name.c_str ());

  tui_win_info *result = iter->second (name.c_str ());
  if (result == nullptr)
    error (_("Could not create window \"%s\""), name.c_str ());
  return result;
}

/* Initialize the known window types.  */

static void
initialize_known_windows ()
{
  known_window_types.emplace (SRC_NAME,
			       make_standard_window<SRC_WIN,
						    tui_source_window>);
  known_window_types.emplace (CMD_NAME,
			       make_standard_window<CMD_WIN, tui_cmd_window>);
  known_window_types.emplace (DATA_NAME,
			       make_standard_window<DATA_WIN,
						    tui_data_window>);
  known_window_types.emplace (DISASSEM_NAME,
			       make_standard_window<DISASSEM_WIN,
						    tui_disasm_window>);
  known_window_types.emplace (STATUS_NAME,
			       make_standard_window<STATUS_WIN,
						    tui_status_window>);
}

/* See tui-layout.h.  */

void
tui_register_window (const char *name, window_factory &&factory)
{
  std::string name_copy = name;

  if (name_copy == SRC_NAME || name_copy == CMD_NAME || name_copy == DATA_NAME
      || name_copy == DISASSEM_NAME || name_copy == STATUS_NAME)
    error (_("Window type \"%s\" is built-in"), name);

  for (const char &c : name_copy)
    {
      if (ISSPACE (c))
	error (_("invalid whitespace character in window name"));

      if (!ISALNUM (c) && strchr ("-_.", c) == nullptr)
	error (_("invalid character '%c' in window name"), c);
    }

  if (!ISALPHA (name_copy[0]))
    error (_("window name must start with a letter, not '%c'"), name_copy[0]);

  /* We already check above for all the builtin window names.  If we get
     this far then NAME must be a user defined window.  Remove any existing
     factory and replace it with this new version.  */

  auto iter = known_window_types.find (name);
  if (iter != known_window_types.end ())
    known_window_types.erase (iter);

  known_window_types.emplace (std::move (name_copy),
			       std::move (factory));
}

/* See tui-layout.h.  */

std::unique_ptr<tui_layout_base>
tui_layout_window::clone () const
{
  tui_layout_window *result = new tui_layout_window (m_contents.c_str ());
  return std::unique_ptr<tui_layout_base> (result);
}

/* See tui-layout.h.  */

void
tui_layout_window::apply (int x_, int y_, int width_, int height_,
			  bool preserve_cmd_win_size_p)
{
  x = x_;
  y = y_;
  width = width_;
  height = height_;
  gdb_assert (m_window != nullptr);
  if (width == 0 || height == 0)
    {
      /* The window was dropped, so it's going to be deleted, reset the
	 soon to be dangling pointer.  */
      m_window = nullptr;
      return;
    }
  m_window->resize (height, width, x, y);
}

/* See tui-layout.h.  */

void
tui_layout_window::get_sizes (bool height, int *min_value, int *max_value)
{
  TUI_SCOPED_DEBUG_ENTER_EXIT;

  if (m_window == nullptr)
    m_window = tui_get_window_by_name (m_contents);

  tui_debug_printf ("window = %s, getting %s",
		    m_window->name (), (height ? "height" : "width"));

  if (height)
    {
      *min_value = m_window->min_height ();
      *max_value = m_window->max_height ();
    }
  else
    {
      *min_value = m_window->min_width ();
      *max_value = m_window->max_width ();
    }

  tui_debug_printf ("min = %d, max = %d", *min_value, *max_value);
}

/* See tui-layout.h.  */

bool
tui_layout_window::first_edge_has_border_p () const
{
  gdb_assert (m_window != nullptr);
  return m_window->can_box ();
}

/* See tui-layout.h.  */

bool
tui_layout_window::last_edge_has_border_p () const
{
  gdb_assert (m_window != nullptr);
  return m_window->can_box ();
}

/* See tui-layout.h.  */

void
tui_layout_window::replace_window (const char *name, const char *new_window)
{
  if (m_contents == name)
    {
      m_contents = new_window;
      if (m_window != nullptr)
	{
	  m_window->make_visible (false);
	  m_window = tui_get_window_by_name (m_contents);
	}
    }
}

/* See tui-layout.h.  */

void
tui_layout_window::specification (ui_file *output, int depth)
{
  gdb_puts (get_name (), output);
}

/* See tui-layout.h.  */

std::string
tui_layout_window::layout_fingerprint () const
{
  if (strcmp (get_name (), "cmd") == 0)
    return "C";
  else
    return "";
}

/* See tui-layout.h.  */

void
tui_layout_split::add_split (std::unique_ptr<tui_layout_split> &&layout,
			     int weight)
{
  split s = {weight, std::move (layout)};
  m_splits.push_back (std::move (s));
}

/* See tui-layout.h.  */

void
tui_layout_split::add_window (const char *name, int weight)
{
  tui_layout_window *result = new tui_layout_window (name);
  split s = {weight, std::unique_ptr<tui_layout_base> (result)};
  m_splits.push_back (std::move (s));
}

/* See tui-layout.h.  */

std::unique_ptr<tui_layout_base>
tui_layout_split::clone () const
{
  tui_layout_split *result = new tui_layout_split (m_vertical);
  for (const split &item : m_splits)
    {
      std::unique_ptr<tui_layout_base> next = item.layout->clone ();
      split s = {item.weight, std::move (next)};
      result->m_splits.push_back (std::move (s));
    }
  return std::unique_ptr<tui_layout_base> (result);
}

/* See tui-layout.h.  */

void
tui_layout_split::get_sizes (bool height, int *min_value, int *max_value)
{
  TUI_SCOPED_DEBUG_ENTER_EXIT;

  *min_value = 0;
  *max_value = 0;
  bool first_time = true;
  for (const split &item : m_splits)
    {
      int new_min, new_max;
      item.layout->get_sizes (height, &new_min, &new_max);
      /* For the mismatch case, the first time through we want to set
	 the min and max to the computed values -- the "first_time"
	 check here is just a funny way of doing that.  */
      if (height == m_vertical || first_time)
	{
	  *min_value += new_min;
	  *max_value += new_max;
	}
      else
	{
	  *min_value = std::max (*min_value, new_min);
	  *max_value = std::min (*max_value, new_max);
	}
      first_time = false;
    }

  tui_debug_printf ("min_value = %d, max_value = %d", *min_value, *max_value);
}

/* See tui-layout.h.  */

bool
tui_layout_split::first_edge_has_border_p () const
{
  if (m_splits.empty ())
    return false;
  return m_splits[0].layout->first_edge_has_border_p ();
}

/* See tui-layout.h.  */

bool
tui_layout_split::last_edge_has_border_p () const
{
  if (m_splits.empty ())
    return false;
  return m_splits.back ().layout->last_edge_has_border_p ();
}

/* See tui-layout.h.  */

void
tui_layout_split::set_weights_from_sizes ()
{
  for (int i = 0; i < m_splits.size (); ++i)
    m_splits[i].weight
      = m_vertical ? m_splits[i].layout->height : m_splits[i].layout->width;
}

/* See tui-layout.h.  */

std::string
tui_layout_split::tui_debug_weights_to_string () const
{
  std::string str;

  for (int i = 0; i < m_splits.size (); ++i)
    {
      if (i > 0)
       str += ", ";
      str += string_printf ("[%d] %d", i, m_splits[i].weight);
    }

  return str;
}

/* See tui-layout.h.  */

void
tui_layout_split::tui_debug_print_size_info
  (const std::vector<tui_layout_split::size_info> &info)
{
  gdb_assert (debug_tui);

  tui_debug_printf ("current size info data:");
  for (int i = 0; i < info.size (); ++i)
    tui_debug_printf ("  [%d] { size = %d, min = %d, max = %d, share_box = %d }",
		      i, info[i].size, info[i].min_size,
		      info[i].max_size, info[i].share_box);
}

/* See tui-layout.h.  */

tui_adjust_result
tui_layout_split::set_size (const char *name, int new_size, bool set_width_p)
{
  TUI_SCOPED_DEBUG_ENTER_EXIT;

  tui_debug_printf ("this = %p, name = %s, new_size = %d",
		    this, name, new_size);

  /* Look through the children.  If one is a layout holding the named
     window, we're done; or if one actually is the named window,
     update it.  */
  int found_index = -1;
  for (int i = 0; i < m_splits.size (); ++i)
    {
      tui_adjust_result adjusted;
      if (set_width_p)
	adjusted = m_splits[i].layout->set_width (name, new_size);
      else
	adjusted = m_splits[i].layout->set_height (name, new_size);
      if (adjusted == HANDLED)
	return HANDLED;
      if (adjusted == FOUND)
	{
	  if (set_width_p ? m_vertical : !m_vertical)
	    return FOUND;
	  found_index = i;
	  break;
	}
    }

  if (found_index == -1)
    return NOT_FOUND;
  int curr_size = (set_width_p
		   ? m_splits[found_index].layout->width
		   : m_splits[found_index].layout->height);
  if (curr_size == new_size)
    return HANDLED;

  tui_debug_printf ("found window %s at index %d", name, found_index);

  set_weights_from_sizes ();
  int delta = m_splits[found_index].weight - new_size;
  m_splits[found_index].weight = new_size;

  tui_debug_printf ("before delta (%d) distribution, weights: %s",
		    delta, tui_debug_weights_to_string ().c_str ());

  /* Distribute the "delta" over all other windows, while respecting their
     min/max sizes.  We grow each window by 1 line at a time continually
     looping over all the windows.  However, skip the window that the user
     just resized, obviously we don't want to readjust that window.  */
  bool found_window_that_can_grow_p = true;
  for (int i = 0; delta != 0; i = (i + 1) % m_splits.size ())
    {
      int index = (found_index + 1 + i) % m_splits.size ();
      if (index == found_index)
	{
	  if (!found_window_that_can_grow_p)
	    break;
	  found_window_that_can_grow_p = false;
	  continue;
	}

      int new_min, new_max;
      m_splits[index].layout->get_sizes (m_vertical, &new_min, &new_max);

      if (delta < 0)
	{
	  /* The primary window grew, so we are trying to shrink other
	     windows.  */
	  if (m_splits[index].weight > new_min)
	    {
	      m_splits[index].weight -= 1;
	      delta += 1;
	      found_window_that_can_grow_p = true;
	    }
	}
      else
	{
	  /* The primary window shrank, so we are trying to grow other
	     windows.  */
	  if (m_splits[index].weight < new_max)
	    {
	      m_splits[index].weight += 1;
	      delta -= 1;
	      found_window_that_can_grow_p = true;
	    }
	}

      tui_debug_printf ("index = %d, weight now: %d",
			index, m_splits[index].weight);
    }

  tui_debug_printf ("after delta (%d) distribution, weights: %s",
		    delta, tui_debug_weights_to_string ().c_str ());

  if (delta != 0)
    {
      if (set_width_p)
	warning (_("Invalid window width specified"));
      else
	warning (_("Invalid window height specified"));
      /* Effectively undo any modifications made here.  */
      set_weights_from_sizes ();
    }
  else
    {
      /* Simply re-apply the updated layout.  We pass false here so that
	 the cmd window can be resized.  However, we should have already
	 resized everything above to be "just right", so the apply call
	 here should not end up changing the sizes at all.  */
      apply (x, y, width, height, false);
    }

  return HANDLED;
}

/* See tui-layout.h.  */

void
tui_layout_split::apply (int x_, int y_, int width_, int height_,
			 bool preserve_cmd_win_size_p)
{
  TUI_SCOPED_DEBUG_ENTER_EXIT;

  x = x_;
  y = y_;
  width = width_;
  height = height_;

  /* In some situations we fix the size of the cmd window.  However,
     occasionally this turns out to be a mistake.  This struct is used to
     hold the original information about the cmd window, so we can restore
     it if needed.  */
  struct old_size_info
  {
    /* Constructor.  */
    old_size_info (int index_, int min_size_, int max_size_)
      : index (index_),
	min_size (min_size_),
	max_size (max_size_)
    { /* Nothing.  */ }

    /* The index in m_splits where the cmd window was found.  */
    int index;

    /* The previous min/max size.  */
    int min_size;
    int max_size;
  };

  /* This is given a value only if we fix the size of the cmd window.  */
  std::optional<old_size_info> old_cmd_info;

  std::vector<size_info> info (m_splits.size ());

  tui_debug_printf ("weights are: %s",
		    tui_debug_weights_to_string ().c_str ());

  /* Step 1: Find the min and max size of each sub-layout.
     Fixed-sized layouts are given their desired size, and then the
     remaining space is distributed among the remaining windows
     according to the weights given.  */
  int available_size = m_vertical ? height : width;
  int last_index = -1;
  int total_weight = 0;
  int prev = -1;
  for (int i = 0; i < m_splits.size (); ++i)
    {
      bool cmd_win_already_exists = TUI_CMD_WIN != nullptr;

      /* Always call get_sizes, to ensure that the window is
	 instantiated.  This is a bit gross but less gross than adding
	 special cases for this in other places.  */
      m_splits[i].layout->get_sizes (m_vertical, &info[i].min_size,
				     &info[i].max_size);

      if (preserve_cmd_win_size_p
	  && cmd_win_already_exists
	  && m_splits[i].layout->get_name () != nullptr
	  && strcmp (m_splits[i].layout->get_name (), "cmd") == 0)
	{
	  /* Save the old cmd window information, in case we need to
	     restore it later.  */
	  old_cmd_info.emplace (i, info[i].min_size, info[i].max_size);

	  /* If this layout has never been applied, then it means the
	     user just changed the layout.  In this situation, it's
	     desirable to keep the size of the command window the
	     same.  Setting the min and max sizes this way ensures
	     that the resizing step, below, does the right thing with
	     this window.  */
	  info[i].min_size = (m_vertical
			      ? TUI_CMD_WIN->height
			      : TUI_CMD_WIN->width);
	  info[i].max_size = info[i].min_size;
	}

      if (info[i].min_size > info[i].max_size)
	{
	  /* There is not enough room for this window, drop it.  */
	  info[i].min_size = 0;
	  info[i].max_size = 0;
	  continue;
	}

      /* Two adjacent boxed windows will share a border.  */
      if (prev != -1
	  && m_splits[prev].layout->last_edge_has_border_p ()
	  && m_splits[i].layout->first_edge_has_border_p ())
	info[i].share_box = true;

      if (info[i].min_size == info[i].max_size)
	{
	  available_size -= info[i].min_size;
	  if (info[i].share_box)
	    {
	      /* A shared border makes a bit more size available.  */
	      ++available_size;
	    }
	}
      else
	{
	  last_index = i;
	  total_weight += m_splits[i].weight;
	}

      prev = i;
    }

  /* If last_index is set then we have a window that is not of a fixed
     size.  This window will have its size calculated below, which requires
     that the total_weight not be zero (we divide by total_weight, so don't
     want a floating-point exception).  */
  gdb_assert (last_index == -1 || total_weight > 0);

  /* Step 2: Compute the size of each sub-layout.  Fixed-sized items
     are given their fixed size, while others are resized according to
     their weight.  */
  int used_size = 0;
  for (int i = 0; i < m_splits.size (); ++i)
    {
      if (info[i].min_size != info[i].max_size)
	{
	  /* Compute the height and clamp to the allowable range.  */
	  info[i].size = available_size * m_splits[i].weight / total_weight;
	  if (info[i].size > info[i].max_size)
	    info[i].size = info[i].max_size;
	  if (info[i].size < info[i].min_size)
	    info[i].size = info[i].min_size;
	  /* Keep a total of all the size we've used so far (we gain some
	     size back if this window can share a border with a preceding
	     window).  Any unused space will be distributed between all of
	     the other windows (while respecting min/max sizes) later in
	     this function.  */
	  used_size += info[i].size;
	  if (info[i].share_box)
	    {
	      /* A shared border makes a bit more size available.  */
	      --used_size;
	    }
	}
      else
	info[i].size = info[i].min_size;
    }

  if (debug_tui)
    {
      tui_debug_printf ("after initial size calculation");
      tui_debug_printf ("available_size = %d, used_size = %d",
			available_size, used_size);
      tui_debug_printf ("total_weight = %d, last_index = %d",
			total_weight, last_index);
      tui_debug_print_size_info (info);
    }

  /* If we didn't find any sub-layouts that were of a non-fixed size, but
     we did find the cmd window, then we can consider that a sort-of
     non-fixed size sub-layout.

     The cmd window might, initially, be of a fixed size (see above), but,
     we are willing to relax this constraint if required to correctly apply
     this layout (see below).  */
  if (last_index == -1 && old_cmd_info.has_value ())
    last_index = old_cmd_info->index;

  /* Allocate any leftover size.  */
  if (available_size != used_size && last_index != -1)
    {
      /* Loop over all windows until the amount of used space is equal to
	 the amount of available space.  There's an escape hatch within
	 the loop in case we can't find any sub-layouts to resize.  */
      bool found_window_that_can_grow_p = true;
      for (int idx = last_index;
	   available_size != used_size;
	   idx = (idx + 1) % m_splits.size ())
	{
	  /* Every time we get back to last_index, which is where the loop
	     started, we check to make sure that we did assign some space
	     to a window, bringing used_size closer to available_size.

	     If we didn't, but the cmd window is of a fixed size, then we
	     can make the console window non-fixed-size, and continue
	     around the loop, hopefully, this will allow the layout to be
	     applied correctly.

	     If we still make it around the loop without moving used_size
	     closer to available_size, then there's nothing more we can do,
	     and we break out of the loop.  */
	  if (idx == last_index)
	    {
	      /* If the used_size is greater than the available_size then
		 this indicates that the fixed-sized sub-layouts claimed
		 more space than is available.  This layout is not going to
		 work.  Our only hope at this point is to make the cmd
		 window non-fixed-size (if possible), and hope we can
		 shrink this enough to fit the rest of the sub-layouts in.

		 Alternatively, we've made it around the loop without
		 adjusting any window's size.  This likely means all
		 windows have hit their min or max size.  Again, our only
		 hope is to make the cmd window non-fixed-size, and hope
		 this fixes all our problems.  */
	      if (old_cmd_info.has_value ()
		  && ((available_size < used_size)
		      || !found_window_that_can_grow_p))
		{
		  info[old_cmd_info->index].min_size = old_cmd_info->min_size;
		  info[old_cmd_info->index].max_size = old_cmd_info->max_size;
		  tui_debug_printf
		    ("restoring index %d (cmd) size limits, min = %d, max = %d",
		     old_cmd_info->index, old_cmd_info->min_size,
		     old_cmd_info->max_size);
		  old_cmd_info.reset ();
		}
	      else if (!found_window_that_can_grow_p)
		break;
	      found_window_that_can_grow_p = false;
	    }

	  if (available_size > used_size
	      && info[idx].size < info[idx].max_size)
	    {
	      found_window_that_can_grow_p = true;
	      info[idx].size += 1;
	      used_size += 1;
	    }
	  else if (available_size < used_size
		   && info[idx].size > info[idx].min_size)
	    {
	      found_window_that_can_grow_p = true;
	      info[idx].size -= 1;
	      used_size -= 1;
	    }
	}

      if (debug_tui)
	{
	  tui_debug_printf ("after final size calculation");
	  tui_debug_printf ("available_size = %d, used_size = %d",
			    available_size, used_size);
	  tui_debug_printf ("total_weight = %d, last_index = %d",
			    total_weight, last_index);
	  tui_debug_print_size_info (info);
	}
    }

  /* Step 3: Resize.  */
  int size_accum = 0;
  const int maximum = m_vertical ? height : width;
  for (int i = 0; i < m_splits.size (); ++i)
    {
      /* If we fall off the bottom, just make allocations overlap.
	 GIGO.  */
      if (size_accum + info[i].size > maximum)
	size_accum = maximum - info[i].size;
      else if (info[i].share_box)
	--size_accum;
      if (m_vertical)
	m_splits[i].layout->apply (x, y + size_accum, width, info[i].size,
				   preserve_cmd_win_size_p);
      else
	m_splits[i].layout->apply (x + size_accum, y, info[i].size, height,
				   preserve_cmd_win_size_p);
      size_accum += info[i].size;
    }
}

/* See tui-layout.h.  */

void
tui_layout_split::remove_windows (const char *name)
{
  for (int i = 0; i < m_splits.size (); ++i)
    {
      const char *this_name = m_splits[i].layout->get_name ();
      if (this_name == nullptr)
	m_splits[i].layout->remove_windows (name);
      else if (strcmp (this_name, name) == 0
	       || strcmp (this_name, CMD_NAME) == 0
	       || strcmp (this_name, STATUS_NAME) == 0)
	{
	  /* Keep.  */
	}
      else
	{
	  m_splits.erase (m_splits.begin () + i);
	  --i;
	}
    }
}

/* See tui-layout.h.  */

void
tui_layout_split::replace_window (const char *name, const char *new_window)
{
  for (auto &item : m_splits)
    item.layout->replace_window (name, new_window);
}

/* See tui-layout.h.  */

void
tui_layout_split::specification (ui_file *output, int depth)
{
  if (depth > 0)
    gdb_puts ("{", output);

  if (!m_vertical)
    gdb_puts ("-horizontal ", output);

  bool first = true;
  for (auto &item : m_splits)
    {
      if (!first)
	gdb_puts (" ", output);
      first = false;
      item.layout->specification (output, depth + 1);
      gdb_printf (output, " %d", item.weight);
    }

  if (depth > 0)
    gdb_puts ("}", output);
}

/* See tui-layout.h.  */

std::string
tui_layout_split::layout_fingerprint () const
{
  for (auto &item : m_splits)
    {
      std::string fp = item.layout->layout_fingerprint ();
      if (!fp.empty () && m_splits.size () != 1)
	return std::string (m_vertical ? "V" : "H") + fp;
    }

  return "";
}

/* Destroy the layout associated with SELF.  */

static void
destroy_layout (struct cmd_list_element *self, void *context)
{
  tui_layout_split *layout = (tui_layout_split *) context;
  size_t index = find_layout (layout);
  layouts.erase (layouts.begin () + index);
}

/* List holding the sub-commands of "layout".  */

static struct cmd_list_element *layout_list;

/* Called to implement 'tui layout'.  */

static void
tui_layout_command (const char *args, int from_tty)
{
  help_list (layout_list, "tui layout ", all_commands, gdb_stdout);
}

/* Add a "layout" command with name NAME that switches to LAYOUT.  */

static struct cmd_list_element *
add_layout_command (const char *name, tui_layout_split *layout)
{
  struct cmd_list_element *cmd;

  string_file spec;
  layout->specification (&spec, 0);

  gdb::unique_xmalloc_ptr<char> doc
    = xstrprintf (_("Apply the \"%s\" layout.\n\
This layout was created using:\n\
  tui new-layout %s %s"),
		  name, name, spec.c_str ());

  cmd = add_cmd (name, class_tui, nullptr, doc.get (), &layout_list);
  cmd->set_context (layout);
  /* There is no API to set this.  */
  cmd->func = tui_apply_layout;
  cmd->destroyer = destroy_layout;
  cmd->doc_allocated = 1;
  doc.release ();
  layouts.emplace_back (layout);

  return cmd;
}

/* Initialize the standard layouts.  */

static void
initialize_layouts ()
{
  tui_layout_split *layout;

  layout = new tui_layout_split ();
  layout->add_window (SRC_NAME, 2);
  layout->add_window (STATUS_NAME, 0);
  layout->add_window (CMD_NAME, 1);
  add_layout_command (SRC_NAME, layout);

  layout = new tui_layout_split ();
  layout->add_window (DISASSEM_NAME, 2);
  layout->add_window (STATUS_NAME, 0);
  layout->add_window (CMD_NAME, 1);
  add_layout_command (DISASSEM_NAME, layout);

  layout = new tui_layout_split ();
  layout->add_window (SRC_NAME, 1);
  layout->add_window (DISASSEM_NAME, 1);
  layout->add_window (STATUS_NAME, 0);
  layout->add_window (CMD_NAME, 1);
  add_layout_command ("split", layout);

  layout = new tui_layout_split ();
  layout->add_window (DATA_NAME, 1);
  layout->add_window (SRC_NAME, 1);
  layout->add_window (STATUS_NAME, 0);
  layout->add_window (CMD_NAME, 1);
  layouts.emplace_back (layout);
  src_regs_layout = layout;

  layout = new tui_layout_split ();
  layout->add_window (DATA_NAME, 1);
  layout->add_window (DISASSEM_NAME, 1);
  layout->add_window (STATUS_NAME, 0);
  layout->add_window (CMD_NAME, 1);
  layouts.emplace_back (layout);
  asm_regs_layout = layout;
}



/* A helper function that returns true if NAME is the name of an
   available window.  */

static bool
validate_window_name (const std::string &name)
{
  auto iter = known_window_types.find (name);
  return iter != known_window_types.end ();
}

/* Implementation of the "tui new-layout" command.  */

static void
tui_new_layout_command (const char *spec, int from_tty)
{
  std::string new_name = extract_arg (&spec);
  if (new_name.empty ())
    error (_("No layout name specified"));
  if (new_name[0] == '-')
    error (_("Layout name cannot start with '-'"));

  bool is_vertical = true;
  spec = skip_spaces (spec);
  if (check_for_argument (&spec, "-horizontal"))
    is_vertical = false;

  std::vector<std::unique_ptr<tui_layout_split>> splits;
  splits.emplace_back (new tui_layout_split (is_vertical));
  std::unordered_set<std::string> seen_windows;
  while (true)
    {
      spec = skip_spaces (spec);
      if (spec[0] == '\0')
	break;

      if (spec[0] == '{')
	{
	  is_vertical = true;
	  spec = skip_spaces (spec + 1);
	  if (check_for_argument (&spec, "-horizontal"))
	    is_vertical = false;
	  splits.emplace_back (new tui_layout_split (is_vertical));
	  continue;
	}

      bool is_close = false;
      std::string name;
      if (spec[0] == '}')
	{
	  is_close = true;
	  ++spec;
	  if (splits.size () == 1)
	    error (_("Extra '}' in layout specification"));
	}
      else
	{
	  name = extract_arg (&spec);
	  if (name.empty ())
	    break;
	  if (!validate_window_name (name))
	    error (_("Unknown window \"%s\""), name.c_str ());
	  if (seen_windows.find (name) != seen_windows.end ())
	    error (_("Window \"%s\" seen twice in layout"), name.c_str ());
	}

      ULONGEST weight = get_ulongest (&spec, '}');
      if ((int) weight != weight)
	error (_("Weight out of range: %s"), pulongest (weight));
      if (is_close)
	{
	  std::unique_ptr<tui_layout_split> last_split
	    = std::move (splits.back ());
	  splits.pop_back ();
	  splits.back ()->add_split (std::move (last_split), weight);
	}
      else
	{
	  splits.back ()->add_window (name.c_str (), weight);
	  seen_windows.insert (name);
	}
    }
  if (splits.size () > 1)
    error (_("Missing '}' in layout specification"));
  if (seen_windows.empty ())
    error (_("New layout does not contain any windows"));
  if (seen_windows.find (CMD_NAME) == seen_windows.end ())
    error (_("New layout does not contain the \"" CMD_NAME "\" window"));

  gdb::unique_xmalloc_ptr<char> cmd_name
    = make_unique_xstrdup (new_name.c_str ());
  std::unique_ptr<tui_layout_split> new_layout = std::move (splits.back ());
  struct cmd_list_element *cmd
    = add_layout_command (cmd_name.get (), new_layout.get ());
  cmd->name_allocated = 1;
  cmd_name.release ();
  new_layout.release ();
}

/* Function to initialize gdb commands, for tui window layout
   manipulation.  */

void _initialize_tui_layout ();
void
_initialize_tui_layout ()
{
  struct cmd_list_element *layout_cmd
    = add_prefix_cmd ("layout", class_tui, tui_layout_command, _("\
Change the layout of windows.\n\
Usage: tui layout prev | next | LAYOUT-NAME"),
		      &layout_list, 0, tui_get_cmd_list ());
  add_com_alias ("layout", layout_cmd, class_tui, 0);

  add_cmd ("next", class_tui, tui_next_layout_command,
	   _("Apply the next TUI layout."),
	   &layout_list);
  add_cmd ("prev", class_tui, tui_prev_layout_command,
	   _("Apply the previous TUI layout."),
	   &layout_list);
  add_cmd ("regs", class_tui, tui_regs_layout_command,
	   _("Apply the TUI register layout."),
	   &layout_list);

  add_cmd ("new-layout", class_tui, tui_new_layout_command,
	   _("Create a new TUI layout.\n\
Usage: tui new-layout [-horizontal] NAME WINDOW WEIGHT [WINDOW WEIGHT]...\n\
Create a new TUI layout.  The new layout will be named NAME,\n\
and can be accessed using \"layout NAME\".\n\
The windows will be displayed in the specified order.\n\
A WINDOW can also be of the form:\n\
  { [-horizontal] NAME WEIGHT [NAME WEIGHT]... }\n\
This form indicates a sub-frame.\n\
Each WEIGHT is an integer, which holds the relative size\n\
to be allocated to the window."),
	   tui_get_cmd_list ());

  initialize_layouts ();
  initialize_known_windows ();
}
