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

#ifndef TUI_TUI_LOCATION_H
#define TUI_TUI_LOCATION_H

#include "tui/tui.h"
#include "tui/tui.h"
#include "gdb_curses.h"
#include "observable.h"

/* Class used to track the current location that the TUI is displaying.  An
   instance of this class will be created; as events occur within GDB the
   location information within this instance will be updated.  As windows
   like the status window, source window, or disassembler window need to
   update themselves, they will ask this instance which location they
   should be displaying.  */

struct tui_location_tracker
{
  /* Update the current location with the provided arguments.  Returns
     true if any of the status window's fields were actually changed,
     and false otherwise.  */
  bool set_location (struct gdbarch *gdbarch,
		     const struct symtab_and_line &sal,
		     const char *procname);

  /* Update the current location with the with the provided argument.
     Return true if any of the fields actually changed, otherwise false.  */
  bool set_location (struct symtab *symtab);

  /* Return the address of the current location.  */
  CORE_ADDR addr () const
  { return m_addr; }

  /* Return the architecture for the current location.  */
  struct gdbarch *gdbarch () const
  { return m_gdbarch; }

  /* Return the full name of the file containing the current location.  */
  const std::string &full_name () const
  { return m_full_name; }

  /* Return the name of the function containing the current location.  */
  const std::string &proc_name () const
  { return m_proc_name; }

  /* Return the line number for the current location.  */
  int line_no () const
  { return m_line_no; }

private:

  /* Update M_FULL_NAME from SYMTAB.   Return true if M_FULL_NAME actually
     changed, otherwise, return false.  */
  bool set_fullname (struct symtab *symtab);

  /* The full name for the file containing the current location.  */
  std::string m_full_name;

  /* The name of the function we're currently within.  */
  std::string m_proc_name;

  /* The line number for the current location.  */
  int m_line_no = 0;

  /* The address of the current location.  */
  CORE_ADDR m_addr = 0;

  /* Architecture associated with code at this location.  */
  struct gdbarch *m_gdbarch = nullptr;
};

/* The single global instance of the location tracking class.  Tracks the
   current location that the TUI windows are displaying.  */

extern tui_location_tracker tui_location;

#endif /* TUI_TUI_LOCATION_H */
