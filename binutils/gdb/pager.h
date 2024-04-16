/* Output pager for gdb
   Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

#ifndef GDB_PAGER_H
#define GDB_PAGER_H

#include "ui-file.h"

/* A ui_file that implements output paging and unfiltered output.  */

class pager_file : public wrapped_file
{
public:
  /* Create a new pager_file.  The new object takes ownership of
     STREAM.  */
  explicit pager_file (ui_file *stream)
    : wrapped_file (stream)
  {
  }

  ~pager_file ()
  {
    delete m_stream;
  }

  DISABLE_COPY_AND_ASSIGN (pager_file);

  void write (const char *buf, long length_buf) override;

  void puts (const char *str) override;

  void write_async_safe (const char *buf, long length_buf) override
  {
    m_stream->write_async_safe (buf, length_buf);
  }

  void emit_style_escape (const ui_file_style &style) override;
  void reset_style () override;

  void flush () override;

  void wrap_here (int indent) override;

  void puts_unfiltered (const char *str) override
  {
    flush_wrap_buffer ();
    m_stream->puts_unfiltered (str);
  }

private:

  void prompt_for_continue ();

  /* Flush the wrap buffer to STREAM, if necessary.  */
  void flush_wrap_buffer ();

  /* Contains characters which are waiting to be output (they have
     already been counted in chars_printed).  */
  std::string m_wrap_buffer;

  /* Amount to indent by if the wrap occurs.  */
  int m_wrap_indent = 0;

  /* Column number on the screen where wrap_buffer begins, or 0 if
     wrapping is not in effect.  */
  int m_wrap_column = 0;

  /* The style applied at the time that wrap_here was called.  */
  ui_file_style m_wrap_style;

  /* This is temporarily set when paging.  This will cause some
     methods to change their behavior to ignore the wrap buffer.  */
  bool m_paging = false;
};

#endif /* GDB_PAGER_H */
