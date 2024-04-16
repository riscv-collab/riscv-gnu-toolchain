/* Copyright (C) 2016-2024 Free Software Foundation, Inc.

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

#ifndef TUI_TUI_OUT_H
#define TUI_TUI_OUT_H

#include "cli-out.h"

/* A ui_out class for the TUI.  This is just like the CLI's ui_out,
   except that it overrides output methods to detect when a source
   line is being printed and show the source in the TUI's source
   window instead of printing the line in the console window.  */
class tui_ui_out : public cli_ui_out
{
public:

  explicit tui_ui_out (ui_file *stream);

protected:

  void do_field_signed (int fldno, int width, ui_align align, const char *fldname,
			LONGEST value) override;
  void do_field_string (int fldno, int width, ui_align align, const char *fldname,
			const char *string, const ui_file_style &style) override;
  void do_field_fmt (int fldno, int width, ui_align align, const char *fldname,
		     const ui_file_style &style,
		     const char *format, va_list args) override
    ATTRIBUTE_PRINTF (7, 0);
  void do_text (const char *string) override;

private:

  /* These fields are used to make print_source_lines show the source
     in the TUI's source window instead of in the console.
     M_START_OF_LINE is incremented whenever something is output to
     the ui_out.  If an integer field named "line" is printed on the
     ui_out, and nothing else has been printed yet (both
     M_START_OF_LINE and M_LINE are still 0), we assume
     print_source_lines is starting to print a source line, and thus
     record the line number in M_LINE.  Afterwards, when we see a
     string field named "fullname" being output, we take the fullname
     and the recorded line and show the source line in the TUI's
     source window.  tui_ui_out::do_text() suppresses text output
     until it sees an endline being printed, at which point these
     variables are reset back to 0.  */
  int m_line = 0;
  int m_start_of_line = 0;
};

#endif /* TUI_TUI_OUT_H */
