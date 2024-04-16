/* TUI display source/assembly window.

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
#include <ctype.h>
#include "symtab.h"
#include "frame.h"
#include "breakpoint.h"
#include "value.h"
#include "source.h"
#include "objfiles.h"
#include "filenames.h"
#include "gdbsupport/gdb-safe-ctype.h"

#include "tui/tui.h"
#include "tui/tui-data.h"
#include "tui/tui-io.h"
#include "tui/tui-status.h"
#include "tui/tui-win.h"
#include "tui/tui-wingeneral.h"
#include "tui/tui-winsource.h"
#include "tui/tui-source.h"
#include "tui/tui-disasm.h"
#include "tui/tui-location.h"
#include "gdb_curses.h"

/* Function to display the "main" routine.  */
void
tui_display_main ()
{
  auto adapter = tui_source_windows ();
  if (adapter.begin () != adapter.end ())
    {
      struct gdbarch *gdbarch;
      CORE_ADDR addr;

      tui_get_begin_asm_address (&gdbarch, &addr);
      if (addr != (CORE_ADDR) 0)
	{
	  struct symtab *s;

	  tui_update_source_windows_with_addr (gdbarch, addr);
	  s = find_pc_line_symtab (addr);
	  tui_location.set_location (s);
	}
    }
}

/* See tui-winsource.h.  */

std::string
tui_copy_source_line (const char **ptr, int *length)
{
  const char *lineptr = *ptr;

  /* Init the line with the line number.  */
  std::string result;

  int column = 0;
  char c;
  do
    {
      int skip_bytes;

      c = *lineptr;
      if (c == '\033' && skip_ansi_escape (lineptr, &skip_bytes))
	{
	  /* We always have to preserve escapes.  */
	  result.append (lineptr, lineptr + skip_bytes);
	  lineptr += skip_bytes;
	  continue;
	}
      if (c == '\0')
	break;

      ++lineptr;
      ++column;

      auto process_tab = [&] ()
	{
	  int max_tab_len = tui_tab_width;

	  --column;
	  for (int j = column % max_tab_len;
	       j < max_tab_len;
	       column++, j++)
	    result.push_back (' ');
	};

      if (c == '\n' || c == '\r' || c == '\0')
	{
	  /* Nothing.  */
	}
      else if (c == '\t')
	process_tab ();
      else if (ISCNTRL (c))
	{
	  result.push_back ('^');
	  result.push_back (c + 0100);
	  ++column;
	}
      else if (c == 0177)
	{
	  result.push_back ('^');
	  result.push_back ('?');
	  ++column;
	}
      else
	result.push_back (c);
    }
  while (c != '\0' && c != '\n' && c != '\r');

  if (c == '\r' && *lineptr == '\n')
    ++lineptr;
  *ptr = lineptr;

  if (length != nullptr)
    *length = column;

  return result;
}

void
tui_source_window_base::style_changed ()
{
  if (tui_active && is_visible ())
    refill ();
}

/* Function to display source in the source window.  This function
   initializes the horizontal scroll to 0.  */
void
tui_source_window_base::update_source_window
  (struct gdbarch *gdbarch,
   const struct symtab_and_line &sal)
{
  m_horizontal_offset = 0;
  update_source_window_as_is (gdbarch, sal);
}


/* Function to display source in the source/asm window.  This function
   shows the source as specified by the horizontal offset.  */
void
tui_source_window_base::update_source_window_as_is
  (struct gdbarch *gdbarch,
   const struct symtab_and_line &sal)
{
  bool ret = set_contents (gdbarch, sal);

  if (!ret)
    erase_source_content ();
  else
    {
      validate_scroll_offsets ();
      update_breakpoint_info (nullptr, false);
      update_exec_info (false);
      show_source_content ();
    }
}


/* See tui-winsource.h.  */
void
tui_source_window_base::update_source_window_with_addr (struct gdbarch *gdbarch,
							CORE_ADDR addr)
{
  struct symtab_and_line sal {};
  if (addr != 0)
    sal = find_pc_line (addr, 0);

  update_source_window (gdbarch, sal);
}

/* Function to ensure that the source and/or disassembly windows
   reflect the input address.  */
void
tui_update_source_windows_with_addr (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  struct symtab_and_line sal {};
  if (addr != 0)
    sal = find_pc_line (addr, 0);

  for (struct tui_source_window_base *win_info : tui_source_windows ())
    win_info->update_source_window (gdbarch, sal);
}

/* Function to ensure that the source and/or disassembly windows
   reflect the symtab and line.  */
void
tui_update_source_windows_with_line (struct symtab_and_line sal)
{
  struct gdbarch *gdbarch = nullptr;
  if (sal.symtab != nullptr)
    {
      find_line_pc (sal.symtab, sal.line, &sal.pc);
      gdbarch = sal.symtab->compunit ()->objfile ()->arch ();
    }

  for (struct tui_source_window_base *win_info : tui_source_windows ())
    win_info->update_source_window (gdbarch, sal);
}

void
tui_source_window_base::do_erase_source_content (const char *str)
{
  int x_pos;
  int half_width = (width - box_size ()) / 2;

  m_content.clear ();
  if (handle != NULL)
    {
      werase (handle.get ());
      check_and_display_highlight_if_needed ();

      if (strlen (str) >= half_width)
	x_pos = 1;
      else
	x_pos = half_width - strlen (str);
      display_string (height / 2, x_pos, str);

      refresh_window ();
    }
}

/* See tui-winsource.h.  */

void
tui_source_window_base::puts_to_pad_with_skip (const char *string, int skip)
{
  gdb_assert (m_pad.get () != nullptr);
  WINDOW *w = m_pad.get ();

  while (skip > 0)
    {
      const char *next = strpbrk (string, "\033");

      /* Print the plain text prefix.  */
      size_t n_chars = next == nullptr ? strlen (string) : next - string;
      if (n_chars > 0)
	{
	  if (skip > 0)
	    {
	      if (skip < n_chars)
		{
		  string += skip;
		  n_chars -= skip;
		  skip = 0;
		}
	      else
		{
		  skip -= n_chars;
		  string += n_chars;
		  n_chars = 0;
		}
	    }

	  if (n_chars > 0)
	    {
	      std::string copy (string, n_chars);
	      tui_puts (copy.c_str (), w);
	    }
	}

      /* We finished.  */
      if (next == nullptr)
	break;

      gdb_assert (*next == '\033');

      int n_read;
      if (skip_ansi_escape (next, &n_read))
	{
	  std::string copy (next, n_read);
	  tui_puts (copy.c_str (), w);
	  next += n_read;
	}
      else
	gdb_assert_not_reached ("unhandled escape");

      string = next;
    }

  if (*string != '\0')
    tui_puts (string, w);
}

/* Redraw the complete line of a source or disassembly window.  */
void
tui_source_window_base::show_source_line (int lineno)
{
  struct tui_source_element *line;

  line = &m_content[lineno];
  if (line->is_exec_point)
    tui_set_reverse_mode (m_pad.get (), true);

  wmove (m_pad.get (), lineno, 0);
  puts_to_pad_with_skip (line->line.c_str (), m_pad_offset);

  if (line->is_exec_point)
    tui_set_reverse_mode (m_pad.get (), false);
}

/* See tui-winsource.h.  */

void
tui_source_window_base::refresh_window ()
{
  TUI_SCOPED_DEBUG_START_END ("window `%s`", name ());

  /* tui_win_info::refresh_window would draw the empty background window to
     the screen, potentially creating a flicker.  */
  wnoutrefresh (handle.get ());

  int pad_width = getmaxx (m_pad.get ());
  int left_margin = this->left_margin ();
  int view_width = this->view_width ();
  int content_width = m_max_length;
  int pad_x = m_horizontal_offset - m_pad_offset;

  tui_debug_printf ("pad_width = %d, left_margin = %d, view_width = %d",
		    pad_width, left_margin, view_width);
  tui_debug_printf ("content_width = %d, pad_x = %d, m_horizontal_offset = %d",
		    content_width, pad_x, m_horizontal_offset);
  tui_debug_printf ("m_pad_offset = %d", m_pad_offset);

  gdb_assert (m_pad_offset >= 0);
  gdb_assert (m_horizontal_offset + view_width
	      <= std::max (content_width, view_width));
  gdb_assert (pad_x >= 0);
  gdb_assert (m_horizontal_offset >= 0);

  /* This function can be called before the pad has been allocated, this
     should only occur during the initial startup.  In this case the first
     condition in the following asserts will not be true, but the nullptr
     check will.  */
  gdb_assert (pad_width > 0 || m_pad.get () == nullptr);
  gdb_assert (pad_x + view_width <= pad_width || m_pad.get () == nullptr);

  int sminrow = y + box_width ();
  int smincol = x + box_width () + left_margin;
  int smaxrow = sminrow + m_content.size () - 1;
  int smaxcol = smincol + view_width - 1;
  prefresh (m_pad.get (), 0, pad_x, sminrow, smincol, smaxrow, smaxcol);
}

void
tui_source_window_base::show_source_content ()
{
  TUI_SCOPED_DEBUG_START_END ("window `%s`", name ());

  gdb_assert (!m_content.empty ());

  /* The pad should be at least as wide as the window, but ideally, as wide
     as the content, however, for some very wide content this might not be
     possible.  */
  int required_pad_width = std::max (m_max_length, width);
  int required_pad_height = m_content.size ();

  /* If the required pad width is wider than the previously requested pad
     width, then we might want to grow the pad.  */
  if (required_pad_width > m_pad_requested_width
      || required_pad_height > getmaxy (m_pad.get ()))
    {
      /* The current pad width.  */
      int pad_width = m_pad == nullptr ? 0 : getmaxx (m_pad.get ());

      gdb_assert (pad_width <= m_pad_requested_width);

      /* If the current pad width is smaller than the previously requested
	 pad width, then this means we previously failed to allocate a
	 bigger pad.  There's no point asking again, so we'll just make so
	 with the pad we currently have.  */
      if (pad_width == m_pad_requested_width
	  || required_pad_height > getmaxy (m_pad.get ()))
	{
	  pad_width = required_pad_width;

	  do
	    {
	      /* Try to allocate a new pad.  */
	      m_pad.reset (newpad (required_pad_height, pad_width));

	      if (m_pad == nullptr)
		{
		  int reduced_width = std::max (pad_width / 2, width);
		  if (reduced_width == pad_width)
		    error (_("failed to setup source window"));
		  pad_width = reduced_width;
		}
	    }
	  while (m_pad == nullptr);
	}

      m_pad_requested_width = required_pad_width;
      tui_debug_printf ("requested width %d, allocated width %d",
			required_pad_width, getmaxx (m_pad.get ()));
    }

  gdb_assert (m_pad != nullptr);
  werase (m_pad.get ());
  for (int lineno = 0; lineno < m_content.size (); lineno++)
    show_source_line (lineno);

  if (can_box ())
    {
      /* Calling check_and_display_highlight_if_needed will call refresh_window
	 (so long as the current window can be boxed), which will ensure that
	 the newly loaded window content is copied to the screen.  */
      check_and_display_highlight_if_needed ();
    }
  else
    refresh_window ();
}

tui_source_window_base::tui_source_window_base ()
{
  m_start_line_or_addr.loa = LOA_ADDRESS;
  m_start_line_or_addr.u.addr = 0;

  gdb::observers::styling_changed.attach
    (std::bind (&tui_source_window::style_changed, this),
     m_observable, "tui-winsource");
}

tui_source_window_base::~tui_source_window_base ()
{
  gdb::observers::styling_changed.detach (m_observable);
}

/* See tui-data.h.  */

void
tui_source_window_base::update_tab_width ()
{
  werase (handle.get ());
  rerender ();
}

void
tui_source_window_base::rerender ()
{
  TUI_SCOPED_DEBUG_START_END ("window `%s`", name ());

  if (!m_content.empty ())
    {
      struct symtab_and_line cursal
	= get_current_source_symtab_and_line ();

      if (m_start_line_or_addr.loa == LOA_LINE)
	cursal.line = m_start_line_or_addr.u.line_no;
      else
	cursal.pc = m_start_line_or_addr.u.addr;
      update_source_window (m_gdbarch, cursal);
    }
  else if (deprecated_safe_get_selected_frame () != NULL)
    {
      struct symtab_and_line cursal
	= get_current_source_symtab_and_line ();
      frame_info_ptr frame = deprecated_safe_get_selected_frame ();
      struct gdbarch *gdbarch = get_frame_arch (frame);

      struct symtab *s = find_pc_line_symtab (get_frame_pc (frame));
      if (this != TUI_SRC_WIN)
	find_line_pc (s, cursal.line, &cursal.pc);
      update_source_window (gdbarch, cursal);
    }
  else
    {
      CORE_ADDR addr;
      struct gdbarch *gdbarch;
      tui_get_begin_asm_address (&gdbarch, &addr);
      if (addr == 0)
	erase_source_content ();
      else
	update_source_window_with_addr (gdbarch, addr);
    }
}

/* See tui-data.h.  */

void
tui_source_window_base::refill ()
{
  symtab_and_line sal {};

  if (this == TUI_SRC_WIN)
    {
      sal = get_current_source_symtab_and_line ();
      if (sal.symtab == NULL)
	{
	  frame_info_ptr fi = deprecated_safe_get_selected_frame ();
	  if (fi != nullptr)
	    sal = find_pc_line (get_frame_pc (fi), 0);
	}
    }

  if (sal.pspace == nullptr)
    sal.pspace = current_program_space;

  if (m_start_line_or_addr.loa == LOA_LINE)
    sal.line = m_start_line_or_addr.u.line_no;
  else
    sal.pc = m_start_line_or_addr.u.addr;

  update_source_window_as_is (m_gdbarch, sal);
}

/* See tui-winsource.h.  */

bool
tui_source_window_base::validate_scroll_offsets ()
{
  TUI_SCOPED_DEBUG_START_END ("window `%s`", name ());

  int original_pad_offset = m_pad_offset;

  if (m_horizontal_offset < 0)
    m_horizontal_offset = 0;

  int content_width = m_max_length;
  int pad_width = getmaxx (m_pad.get ());
  int view_width = this->view_width ();

  tui_debug_printf ("pad_width = %d, view_width = %d, content_width = %d",
		    pad_width, view_width, content_width);
  tui_debug_printf ("original_pad_offset = %d, m_horizontal_offset = %d",
		    original_pad_offset, m_horizontal_offset);

  if (m_horizontal_offset + view_width > content_width)
    m_horizontal_offset = std::max (content_width - view_width, 0);

  if ((m_horizontal_offset + view_width) > (m_pad_offset + pad_width))
    {
      m_pad_offset = std::min (m_horizontal_offset, content_width - pad_width);
      m_pad_offset = std::max (m_pad_offset, 0);
    }
  else if (m_horizontal_offset < m_pad_offset)
    m_pad_offset = std::max (m_horizontal_offset + view_width - pad_width, 0);

  gdb_assert (m_pad_offset >= 0);
  return (original_pad_offset != m_pad_offset);
}

/* Scroll the source forward or backward horizontally.  */

void
tui_source_window_base::do_scroll_horizontal (int num_to_scroll)
{
  if (!m_content.empty ())
    {
      m_horizontal_offset += num_to_scroll;

      if (validate_scroll_offsets ())
	show_source_content ();

      refresh_window ();
    }
}


/* Set or clear the is_exec_point flag in the line whose line is
   line_no.  */

void
tui_source_window_base::set_is_exec_point_at (struct tui_line_or_address l)
{
  bool changed = false;
  int i;

  i = 0;
  while (i < m_content.size ())
    {
      bool new_state;
      struct tui_line_or_address content_loa =
	m_content[i].line_or_addr;

      if (content_loa.loa == l.loa
	  && ((l.loa == LOA_LINE && content_loa.u.line_no == l.u.line_no)
	      || (l.loa == LOA_ADDRESS && content_loa.u.addr == l.u.addr)))
	new_state = true;
      else
	new_state = false;
      if (new_state != m_content[i].is_exec_point)
	{
	  changed = true;
	  m_content[i].is_exec_point = new_state;
	}
      i++;
    }
  if (changed)
    refill ();
}

/* See tui-winsource.h.  */

void
tui_update_all_breakpoint_info (struct breakpoint *being_deleted)
{
  for (tui_source_window_base *win : tui_source_windows ())
    {
      if (win->update_breakpoint_info (being_deleted, false))
	win->update_exec_info ();
    }
}


/* Scan the source window and the breakpoints to update the break_mode
   information for each line.

   Returns true if something changed and the execution window must be
   refreshed.  */

bool
tui_source_window_base::update_breakpoint_info
  (struct breakpoint *being_deleted, bool current_only)
{
  int i;
  bool need_refresh = false;

  for (i = 0; i < m_content.size (); i++)
    {
      struct tui_source_element *line;

      line = &m_content[i];
      if (current_only && !line->is_exec_point)
	 continue;

      /* Scan each breakpoint to see if the current line has something to
	 do with it.  Identify enable/disabled breakpoints as well as
	 those that we already hit.  */
      tui_bp_flags mode = 0;
      for (breakpoint &bp : all_breakpoints ())
	{
	  if (&bp == being_deleted)
	    continue;

	  for (bp_location &loc : bp.locations ())
	    {
	      if (location_matches_p (&loc, i))
		{
		  if (bp.enable_state == bp_disabled)
		    mode |= TUI_BP_DISABLED;
		  else
		    mode |= TUI_BP_ENABLED;
		  if (bp.hit_count)
		    mode |= TUI_BP_HIT;
		  if (bp.first_loc ().cond)
		    mode |= TUI_BP_CONDITIONAL;
		  if (bp.type == bp_hardware_breakpoint)
		    mode |= TUI_BP_HARDWARE;
		}
	    }
	}

      if (line->break_mode != mode)
	{
	  line->break_mode = mode;
	  need_refresh = true;
	}
    }
  return need_refresh;
}

/* See tui-winsource.h.  */

void
tui_source_window_base::update_exec_info (bool refresh_p)
{
  update_breakpoint_info (nullptr, true);
  for (int i = 0; i < m_content.size (); i++)
    {
      struct tui_source_element *src_element = &m_content[i];
      /* Add 1 for '\0'.  */
      char element[TUI_EXECINFO_SIZE + 1];
      /* Initialize all but last element.  */
      char space = tui_left_margin_verbose ? '_' : ' ';
      memset (element, space, TUI_EXECINFO_SIZE);
      /* Initialize last element.  */
      element[TUI_EXECINFO_SIZE] = '\0';

      /* Now update the exec info content based upon the state
	 of each line as indicated by the source content.  */
      tui_bp_flags mode = src_element->break_mode;
      if (mode & TUI_BP_HIT)
	element[TUI_BP_HIT_POS] = (mode & TUI_BP_HARDWARE) ? 'H' : 'B';
      else if (mode & (TUI_BP_ENABLED | TUI_BP_DISABLED))
	element[TUI_BP_HIT_POS] = (mode & TUI_BP_HARDWARE) ? 'h' : 'b';

      if (mode & TUI_BP_ENABLED)
	element[TUI_BP_BREAK_POS] = '+';
      else if (mode & TUI_BP_DISABLED)
	element[TUI_BP_BREAK_POS] = '-';

      if (src_element->is_exec_point)
	element[TUI_EXEC_POS] = '>';

      display_string (i + box_width (), box_width (), element);

      show_line_number (i);
    }
  if (refresh_p)
    refresh_window ();
}
