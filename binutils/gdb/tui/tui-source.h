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

#ifndef TUI_TUI_SOURCE_H
#define TUI_TUI_SOURCE_H

#include "tui/tui-data.h"
#include "tui-winsource.h"

struct symtab;

/* A TUI source window.  */

struct tui_source_window : public tui_source_window_base
{
  tui_source_window () = default;

  DISABLE_COPY_AND_ASSIGN (tui_source_window);

  const char *name () const override
  {
    return SRC_NAME;
  }

  /* Return true if the location LOC corresponds to the line number
     LINE_NO in this source window; false otherwise.  */
  bool location_matches_p (struct bp_location *loc, int line_no) override;

  bool showing_source_p (const char *filename) const;

  void maybe_update (frame_info_ptr fi, symtab_and_line sal) override;

  void erase_source_content () override
  {
    do_erase_source_content (_("[ No Source Available ]"));
  }

  void display_start_addr (struct gdbarch **gdbarch_p,
			   CORE_ADDR *addr_p) override;

protected:

  void do_scroll_vertical (int num_to_scroll) override;

  bool set_contents (struct gdbarch *gdbarch,
		     const struct symtab_and_line &sal) override;

  int extra_margin () const override
  {
    return m_digits;
  }

  void show_line_number (int lineno) const override;

private:

  /* Answer whether a particular line number or address is displayed
     in the current source window.  */
  bool line_is_displayed (int line) const;

  /* How many digits to use when formatting the line number.  This
     includes the trailing space.  */
  int m_digits;

  /* It is the resolved form as returned by symtab_to_fullname.  */
  gdb::unique_xmalloc_ptr<char> m_fullname;
};

#endif /* TUI_TUI_SOURCE_H */
