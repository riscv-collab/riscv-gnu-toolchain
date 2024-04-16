/* MI Command Set - MI output generating routines for GDB.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions (a Red Hat company).

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

#ifndef MI_MI_OUT_H
#define MI_MI_OUT_H

#include <vector>

struct ui_out;
struct ui_file;


class mi_ui_out : public ui_out
{
public:

  explicit mi_ui_out (int mi_version);
  virtual ~mi_ui_out ();

  /* MI-specific */
  void rewind ();
  void put (struct ui_file *stream);

  /* Return the version number of the current MI.  */
  int version ();

  bool can_emit_style_escape () const override
  {
    return false;
  }

protected:

  virtual void do_table_begin (int nbrofcols, int nr_rows, const char *tblid)
    override;
  virtual void do_table_body () override;
  virtual void do_table_header (int width, ui_align align,
			     const std::string &col_name,
			     const std::string &col_hdr) override;
  virtual void do_table_end () override;

  virtual void do_begin (ui_out_type type, const char *id) override;
  virtual void do_end (ui_out_type type) override;
  virtual void do_field_signed (int fldno, int width, ui_align align,
				const char *fldname, LONGEST value) override;
  virtual void do_field_unsigned (int fldno, int width, ui_align align,
				  const char *fldname, ULONGEST value)
    override;
  virtual void do_field_skip (int fldno, int width, ui_align align,
			   const char *fldname) override;
  virtual void do_field_string (int fldno, int width, ui_align align,
				const char *fldname, const char *string,
				const ui_file_style &style) override;
  virtual void do_field_fmt (int fldno, int width, ui_align align,
			     const char *fldname, const ui_file_style &style,
			     const char *format, va_list args)
    override ATTRIBUTE_PRINTF (7,0);
  virtual void do_spaces (int numspaces) override;
  virtual void do_text (const char *string) override;
  virtual void do_message (const ui_file_style &style,
			   const char *format, va_list args) override
    ATTRIBUTE_PRINTF (3,0);
  virtual void do_wrap_hint (int indent) override;
  virtual void do_flush () override;
  virtual void do_redirect (struct ui_file *outstream) override;

  virtual bool do_is_mi_like_p () const override
  { return true; }

  virtual void do_progress_start () override;
  virtual void do_progress_notify (const std::string &, const char *,
				   double, double) override;

  virtual void do_progress_end () override;

private:

  void field_separator ();
  void open (const char *name, ui_out_type type);
  void close (ui_out_type type);

  /* The state of a recent progress_update.  */
  struct mi_progress_info
  {
    /* The current state.  */
    progress_update::state state;

    mi_progress_info ()
      : state (progress_update::START)
    {}
  };

  /* Stack of progress info.  */
  std::vector<mi_progress_info> m_progress_info;

  /* Convenience method that returns the MI out's string stream cast
     to its appropriate type.  Assumes/asserts that output was not
     redirected.  */
  string_file *main_stream ();

  /* Helper for the constructor, deduce ui_out_flags for the given
     MI_VERSION.  */
  static ui_out_flags make_flags (int mi_version)
  {
    ui_out_flags flags = 0;

    /* In MI version 2 and below, multi-location breakpoints had a wrong
       syntax.  It is fixed in version 3.  */
    if (mi_version >= 3)
      flags |= fix_multi_location_breakpoint_output;

    /* In MI version 3 and below, the "script" field in breakpoint output
       had a wrong syntax.  It is fixed in version 4.  */
    if (mi_version >= 4)
      flags |= fix_breakpoint_script_output;

    return flags;
  }

  bool m_suppress_field_separator;
  bool m_suppress_output;
  int m_mi_version;
  std::vector<ui_file *> m_streams;
};

/* Create an MI ui-out object with MI version MI_VERSION, which should be equal
   to one of the INTERP_MI* constants (see interps.h).

   Return nullptr if an invalid version is provided.  */
std::unique_ptr<mi_ui_out> mi_out_new (const char *mi_version);

void mi_out_put (ui_out *uiout, struct ui_file *stream);
void mi_out_rewind (ui_out *uiout);

#endif /* MI_MI_OUT_H */
