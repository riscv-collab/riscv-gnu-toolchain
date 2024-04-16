/* TUI display source window.

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
#include <math.h>
#include <ctype.h>
#include "symtab.h"
#include "frame.h"
#include "breakpoint.h"
#include "source.h"
#include "objfiles.h"
#include "filenames.h"
#include "source-cache.h"

#include "tui/tui.h"
#include "tui/tui-data.h"
#include "tui/tui-io.h"
#include "tui/tui-status.h"
#include "tui/tui-win.h"
#include "tui/tui-winsource.h"
#include "tui/tui-source.h"
#include "tui/tui-location.h"
#include "gdb_curses.h"

/* Function to display source in the source window.  */
bool
tui_source_window::set_contents (struct gdbarch *arch,
				 const struct symtab_and_line &sal)
{
  struct symtab *s = sal.symtab;
  int line_no = sal.line;

  if (s == NULL)
    return false;

  /* Take hilite (window border) into account, when
     calculating the number of lines.  */
  int nlines = height - box_size ();

  std::string srclines;
  const std::vector<off_t> *offsets;
  if (!g_source_cache.get_source_lines (s, line_no, line_no + nlines,
					&srclines)
      || !g_source_cache.get_line_charpos (s, &offsets))
    return false;

  int cur_line_no, cur_line;
  const char *s_filename = symtab_to_filename_for_display (s);

  set_title (s_filename);

  m_fullname = make_unique_xstrdup (symtab_to_fullname (s));

  cur_line = 0;
  m_gdbarch = s->compunit ()->objfile ()->arch ();
  m_start_line_or_addr.loa = LOA_LINE;
  cur_line_no = m_start_line_or_addr.u.line_no = line_no;

  m_digits = 7;
  if (compact_source)
    {
      /* Solaris 11+gcc 5.5 has ambiguous overloads of log10, so we
	 cast to double to get the right one.  */
      int lines_in_file = offsets->size ();
      int max_line_nr = lines_in_file;
      int digits_needed = 1 + (int)log10 ((double) max_line_nr);
      int trailing_space = 1;
      m_digits = digits_needed + trailing_space;
    }

  m_max_length = -1;
  const char *iter = srclines.c_str ();
  m_content.resize (nlines);
  while (cur_line < nlines)
    {
      struct tui_source_element *element = &m_content[cur_line];

      std::string text;
      if (*iter != '\0')
	{
	  int line_len;
	  text = tui_copy_source_line (&iter, &line_len);
	  m_max_length = std::max (m_max_length, line_len);
	}
      else
	{
	  /* Line not in source file.  */
	  cur_line_no = -1;
	}

      /* Set whether element is the execution point
	 and whether there is a break point on it.  */
      element->line_or_addr.loa = LOA_LINE;
      element->line_or_addr.u.line_no = cur_line_no;
      element->is_exec_point
	= (filename_cmp (tui_location.full_name ().c_str (),
			 symtab_to_fullname (s)) == 0
	   && cur_line_no == tui_location.line_no ());

      m_content[cur_line].line = std::move (text);

      cur_line++;
      cur_line_no++;
    }

  return true;
}


/* Answer whether the source is currently displayed in the source
   window.  */
bool
tui_source_window::showing_source_p (const char *fullname) const
{
  return (!m_content.empty ()
	  && (filename_cmp (tui_location.full_name ().c_str (),
			    fullname) == 0));
}


/* Scroll the source forward or backward vertically.  */
void
tui_source_window::do_scroll_vertical (int num_to_scroll)
{
  if (!m_content.empty ())
    {
      struct symtab *s;
      struct symtab_and_line cursal = get_current_source_symtab_and_line ();
      struct gdbarch *arch = m_gdbarch;

      if (cursal.symtab == NULL)
	{
	  frame_info_ptr fi = get_selected_frame (NULL);
	  s = find_pc_line_symtab (get_frame_pc (fi));
	  arch = get_frame_arch (fi);
	}
      else
	s = cursal.symtab;

      int line_no = m_start_line_or_addr.u.line_no + num_to_scroll;
      const std::vector<off_t> *offsets;
      if (g_source_cache.get_line_charpos (s, &offsets)
	  && line_no > offsets->size ())
	line_no = m_start_line_or_addr.u.line_no;
      if (line_no <= 0)
	line_no = 1;

      cursal.line = line_no;
      find_line_pc (cursal.symtab, cursal.line, &cursal.pc);
      for (struct tui_source_window_base *win_info : tui_source_windows ())
	win_info->update_source_window_as_is (arch, cursal);
    }
}

bool
tui_source_window::location_matches_p (struct bp_location *loc, int line_no)
{
  return (m_content[line_no].line_or_addr.loa == LOA_LINE
	  && m_content[line_no].line_or_addr.u.line_no == loc->line_number
	  && loc->symtab != NULL
	  && filename_cmp (m_fullname.get (),
			   symtab_to_fullname (loc->symtab)) == 0);
}

/* See tui-source.h.  */

bool
tui_source_window::line_is_displayed (int line) const
{
  if (m_content.size () < SCROLL_THRESHOLD)
    return false;

  for (size_t i = 0; i < m_content.size () - SCROLL_THRESHOLD; ++i)
    {
      if (m_content[i].line_or_addr.loa == LOA_LINE
	  && m_content[i].line_or_addr.u.line_no == line)
	return true;
    }

  return false;
}

void
tui_source_window::maybe_update (frame_info_ptr fi, symtab_and_line sal)
{
  int start_line = (sal.line - ((height - box_size ()) / 2)) + 1;
  if (start_line <= 0)
    start_line = 1;

  bool source_already_displayed = (sal.symtab != 0
				   && showing_source_p (m_fullname.get ()));

  if (!(source_already_displayed && line_is_displayed (sal.line)))
    {
      sal.line = start_line;
      update_source_window (get_frame_arch (fi), sal);
    }
  else
    {
      struct tui_line_or_address l;

      l.loa = LOA_LINE;
      l.u.line_no = sal.line;
      set_is_exec_point_at (l);
    }
}

void
tui_source_window::display_start_addr (struct gdbarch **gdbarch_p,
				       CORE_ADDR *addr_p)
{
  struct symtab_and_line cursal = get_current_source_symtab_and_line ();

  *gdbarch_p = m_gdbarch;
  find_line_pc (cursal.symtab, m_start_line_or_addr.u.line_no, addr_p);
}

/* See tui-winsource.h.  */

void
tui_source_window::show_line_number (int offset) const
{
  int lineno = m_content[offset].line_or_addr.u.line_no;
  char text[20];
  char space = tui_left_margin_verbose ? '_' : ' ';
  if (lineno == -1)
    {
      /* Line not in source file, don't show line number.  */
      for (int i = 0; i <= m_digits; ++i)
	text[i] = (i == m_digits) ? '\0' : space;
    }
  else
    {
      xsnprintf (text, sizeof (text),
		 tui_left_margin_verbose ? "%0*d%c" : "%*d%c", m_digits - 1,
		 lineno, space);
    }
  display_string (text);
}
