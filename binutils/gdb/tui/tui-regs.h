/* TUI display registers in window.

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

#ifndef TUI_TUI_REGS_H
#define TUI_TUI_REGS_H

#include "tui/tui-data.h"
#include "reggroups.h"

/* A data item window.  */

struct tui_data_item_window
{
  tui_data_item_window () = default;

  DISABLE_COPY_AND_ASSIGN (tui_data_item_window);

  tui_data_item_window (tui_data_item_window &&) = default;

  void rerender (WINDOW *handle, int field_width);

  /* Location.  */
  int x = 0;
  int y = 0;
  /* The register number.  */
  int regno = -1;
  bool highlight = false;
  bool visible = false;
  std::string content;
};

/* The TUI registers window.  */
struct tui_data_window : public tui_win_info
{
  tui_data_window () = default;

  DISABLE_COPY_AND_ASSIGN (tui_data_window);

  const char *name () const override
  {
    return DATA_NAME;
  }

  void check_register_values (frame_info_ptr frame);

  void show_registers (const reggroup *group);

  const reggroup *get_current_group () const
  {
    return m_current_group;
  }

protected:

  void do_scroll_vertical (int num_to_scroll) override;
  void do_scroll_horizontal (int num_to_scroll) override
  {
  }

  void rerender (bool toplevel);
  void rerender () override
  {
    rerender (true);
  }

private:

  /* Display the registers in the content from 'start_element_no'
     until the end of the register content or the end of the display
     height.  No checking for displaying past the end of the registers
     is done here.  */
  void display_registers_from (int start_element_no);

  /* Display the registers starting at line line_no in the data
     window.  Answers the line number that the display actually
     started from.  If nothing is displayed (-1) is returned.  */
  int display_registers_from_line (int line_no);

  /* Return the index of the first element displayed.  If none are
     displayed, then return -1.  */
  int first_data_item_displayed ();

  /* Display the registers in the content from 'start_element_no' on
     'start_line_no' until the end of the register content or the end
     of the display height.  This function checks that we won't
     display off the end of the register display.  */
  void display_reg_element_at_line (int start_element_no, int start_line_no);

  void show_register_group (const reggroup *group,
			    frame_info_ptr frame,
			    bool refresh_values_only);

  /* Answer the number of the last line in the regs display.  If there
     are no registers (-1) is returned.  */
  int last_regs_line_no () const;

  /* Answer the line number that the register element at element_no is
     on.  If element_no is greater than the number of register
     elements there are, -1 is returned.  */
  int line_from_reg_element_no (int element_no) const;

  /* Answer the index of the first element in line_no.  If line_no is
     past the register area (-1) is returned.  */
  int first_reg_element_no_inline (int line_no) const;

  /* Delete all the item windows in the data window.  This is usually
     done when the data window is scrolled.  */
  void delete_data_content_windows ();

  void erase_data_content (const char *prompt);

  /* Windows that are used to display registers.  */
  std::vector<tui_data_item_window> m_regs_content;
  int m_regs_column_count = 0;
  const reggroup *m_current_group = nullptr;

  /* Width of each register's display area.  */
  int m_item_width = 0;
};

#endif /* TUI_TUI_REGS_H */
