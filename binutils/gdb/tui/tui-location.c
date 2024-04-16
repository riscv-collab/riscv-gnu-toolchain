/* Copyright (C) 2021-2024 Free Software Foundation, Inc.

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
#include "tui/tui.h"
#include "tui/tui-status.h"
#include "tui/tui-data.h"
#include "tui/tui-location.h"
#include "symtab.h"
#include "source.h"

/* See tui/tui-location.h.  */

tui_location_tracker tui_location;

/* See tui/tui-location.h.  */

bool
tui_location_tracker::set_location (struct gdbarch *gdbarch,
				    const struct symtab_and_line &sal,
				    const char *procname)
{
  gdb_assert (procname != nullptr);

  bool location_changed_p = set_fullname (sal.symtab);
  location_changed_p |= procname != m_proc_name;
  location_changed_p |= sal.line != m_line_no;
  location_changed_p |= sal.pc != m_addr;
  location_changed_p |= gdbarch != m_gdbarch;

  m_proc_name = procname;
  m_line_no = sal.line;
  m_addr = sal.pc;
  m_gdbarch = gdbarch;

  if (location_changed_p)
    tui_show_status_content ();

  return location_changed_p;
}

/* See tui/tui-location.h.  */

bool
tui_location_tracker::set_location (struct symtab *symtab)
{
  bool location_changed_p = set_fullname (symtab);

  if (location_changed_p)
    tui_show_status_content ();

  return location_changed_p;
}

/* See tui/tui-location.h.  */

bool
tui_location_tracker::set_fullname (struct symtab *symtab)
{
  const char *fullname = (symtab == nullptr
			  ? "??"
			  : symtab_to_fullname (symtab));
  bool location_changed_p = fullname != m_full_name;
  m_full_name = std::string (fullname);

  return location_changed_p;
}
