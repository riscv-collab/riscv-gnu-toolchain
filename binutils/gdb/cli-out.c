/* Output generating routines for GDB CLI.

   Copyright (C) 1999-2024 Free Software Foundation, Inc.

   Contributed by Cygnus Solutions.
   Written by Fernando Nasser for Cygnus.

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
#include "ui-out.h"
#include "cli-out.h"
#include "completer.h"
#include "readline/readline.h"
#include "cli/cli-style.h"
#include "ui.h"

/* These are the CLI output functions */

/* Mark beginning of a table */

void
cli_ui_out::do_table_begin (int nbrofcols, int nr_rows, const char *tblid)
{
  if (nr_rows == 0)
    m_suppress_output = true;
  else
    /* Only the table suppresses the output and, fortunately, a table
       is not a recursive data structure.  */
    gdb_assert (!m_suppress_output);
}

/* Mark beginning of a table body */

void
cli_ui_out::do_table_body ()
{
  if (m_suppress_output)
    return;

  /* first, close the table header line */
  text ("\n");
}

/* Mark end of a table */

void
cli_ui_out::do_table_end ()
{
  m_suppress_output = false;
}

/* Specify table header */

void
cli_ui_out::do_table_header (int width, ui_align alignment,
			     const std::string &col_name,
			     const std::string &col_hdr)
{
  if (m_suppress_output)
    return;

  do_field_string (0, width, alignment, 0, col_hdr.c_str (),
		   ui_file_style ());
}

/* Mark beginning of a list */

void
cli_ui_out::do_begin (ui_out_type type, const char *id)
{
}

/* Mark end of a list */

void
cli_ui_out::do_end (ui_out_type type)
{
}

/* output an int field */

void
cli_ui_out::do_field_signed (int fldno, int width, ui_align alignment,
			     const char *fldname, LONGEST value)
{
  if (m_suppress_output)
    return;

  do_field_string (fldno, width, alignment, fldname, plongest (value),
		   ui_file_style ());
}

/* output an unsigned field */

void
cli_ui_out::do_field_unsigned (int fldno, int width, ui_align alignment,
			       const char *fldname, ULONGEST value)
{
  if (m_suppress_output)
    return;

  do_field_string (fldno, width, alignment, fldname, pulongest (value),
		   ui_file_style ());
}

/* used to omit a field */

void
cli_ui_out::do_field_skip (int fldno, int width, ui_align alignment,
			   const char *fldname)
{
  if (m_suppress_output)
    return;

  do_field_string (fldno, width, alignment, fldname, "",
		   ui_file_style ());
}

/* other specific cli_field_* end up here so alignment and field
   separators are both handled by cli_field_string */

void
cli_ui_out::do_field_string (int fldno, int width, ui_align align,
			     const char *fldname, const char *string,
			     const ui_file_style &style)
{
  int before = 0;
  int after = 0;

  if (m_suppress_output)
    return;

  if ((align != ui_noalign) && string)
    {
      before = width - strlen (string);
      if (before <= 0)
	before = 0;
      else
	{
	  if (align == ui_right)
	    after = 0;
	  else if (align == ui_left)
	    {
	      after = before;
	      before = 0;
	    }
	  else
	    /* ui_center */
	    {
	      after = before / 2;
	      before -= after;
	    }
	}
    }

  if (before)
    spaces (before);

  if (string)
    {
      ui_file *stream = m_streams.back ();
      stream->emit_style_escape (style);
      stream->puts (string);
      stream->emit_style_escape (ui_file_style ());
    }

  if (after)
    spaces (after);

  if (align != ui_noalign)
    field_separator ();
}

/* Output field containing ARGS using printf formatting in FORMAT.  */

void
cli_ui_out::do_field_fmt (int fldno, int width, ui_align align,
			  const char *fldname, const ui_file_style &style,
			  const char *format, va_list args)
{
  if (m_suppress_output)
    return;

  std::string str = string_vprintf (format, args);

  do_field_string (fldno, width, align, fldname, str.c_str (), style);
}

void
cli_ui_out::do_spaces (int numspaces)
{
  if (m_suppress_output)
    return;

  print_spaces (numspaces, m_streams.back ());
}

void
cli_ui_out::do_text (const char *string)
{
  if (m_suppress_output)
    return;

  gdb_puts (string, m_streams.back ());
}

void
cli_ui_out::do_message (const ui_file_style &style,
			const char *format, va_list args)
{
  if (m_suppress_output)
    return;

  std::string str = string_vprintf (format, args);
  if (!str.empty ())
    {
      ui_file *stream = m_streams.back ();
      stream->emit_style_escape (style);
      stream->puts (str.c_str ());
      stream->emit_style_escape (ui_file_style ());
    }
}

void
cli_ui_out::do_wrap_hint (int indent)
{
  if (m_suppress_output)
    return;

  m_streams.back ()->wrap_here (indent);
}

void
cli_ui_out::do_flush ()
{
  gdb_flush (m_streams.back ());
}

/* OUTSTREAM as non-NULL will push OUTSTREAM on the stack of output streams
   and make it therefore active.  OUTSTREAM as NULL will pop the last pushed
   output stream; it is an internal error if it does not exist.  */

void
cli_ui_out::do_redirect (ui_file *outstream)
{
  if (outstream != NULL)
    m_streams.push_back (outstream);
  else
    m_streams.pop_back ();
}

/* Initialize a progress update to be displayed with
   cli_ui_out::do_progress_notify.  */

void
cli_ui_out::do_progress_start ()
{
  m_progress_info.emplace_back ();
}

#define MIN_CHARS_PER_LINE 50
#define MAX_CHARS_PER_LINE 4096

/* Print a progress update.  MSG is a string to be printed on the line above
   the progress bar.  TOTAL is the size of the download whose progress is
   being displayed.  UNIT should be the unit of TOTAL (ex. "K"). If HOWMUCH
   is between 0.0 and 1.0, a progress bar is displayed indicating the percentage
   of completion and the download size.  If HOWMUCH is negative, a progress
   indicator will tick across the screen.  If the output stream is not a tty
   then only MSG is printed.

   - printed for tty, HOWMUCH between 0.0 and 1.0:
	<MSG
	[#########                  ]  HOWMUCH*100% (TOTAL UNIT)\r>
   - printed for tty, HOWMUCH < 0.0:
	<MSG
	[    ###                    ]\r>
   - printed for not-a-tty:
	<MSG...\n>
*/

void
cli_ui_out::do_progress_notify (const std::string &msg,
				const char *unit,
				double howmuch, double total)
{
  int chars_per_line = get_chars_per_line ();
  struct ui_file *stream = m_streams.back ();
  cli_progress_info &info (m_progress_info.back ());

  if (chars_per_line > MAX_CHARS_PER_LINE)
    chars_per_line = MAX_CHARS_PER_LINE;

  if (info.state == progress_update::START)
    {
      if (stream->isatty ()
	  && current_ui->input_interactive_p ()
	  && chars_per_line >= MIN_CHARS_PER_LINE)
	{
	  gdb_printf (stream, "%s\n", msg.c_str ());
	  info.state = progress_update::BAR;
	}
      else
	{
	  gdb_printf (stream, "%s...\n", msg.c_str ());
	  info.state = progress_update::WORKING;
	}
    }

  if (info.state != progress_update::BAR
      || chars_per_line < MIN_CHARS_PER_LINE)
    return;

  if (total > 0 && howmuch >= 0 && howmuch <= 1.0)
    {
      std::string progress = string_printf (" %3.f%% (%.2f %s)",
					    howmuch * 100, total,
					    unit);
      int width = chars_per_line - progress.size () - 4;
      int max = width * howmuch;

      std::string display = "\r[";

      for (int i = 0; i < width; ++i)
	if (i < max)
	  display += "#";
	else
	  display += " ";

      display += "]" + progress;
      gdb_printf (stream, "%s", display.c_str ());
      gdb_flush (stream);
    }
  else
    {
      using namespace std::chrono;
      milliseconds diff = duration_cast<milliseconds>
	(steady_clock::now () - info.last_update);

      /* Advance the progress indicator at a rate of 1 tick every
	 every 0.5 seconds.  */
      if (diff.count () >= 500)
	{
	  int width = chars_per_line - 4;

	  gdb_printf (stream, "\r[");
	  for (int i = 0; i < width; ++i)
	    {
	      if (i == info.pos % width
		  || i == (info.pos + 1) % width
		  || i == (info.pos + 2) % width)
		gdb_printf (stream, "#");
	      else
		gdb_printf (stream, " ");
	    }

	  gdb_printf (stream, "]");
	  gdb_flush (stream);
	  info.last_update = steady_clock::now ();
	  info.pos++;
	}
    }

  return;
}

/* Clear do_progress_notify output from the current line.  Overwrites the
   notification with whitespace.  */

void
cli_ui_out::clear_progress_notify ()
{
  struct ui_file *stream = m_streams.back ();
  int chars_per_line = get_chars_per_line ();

  scoped_restore save_pagination
    = make_scoped_restore (&pagination_enabled, false);

  if (!stream->isatty ()
      || !current_ui->input_interactive_p ()
      || chars_per_line < MIN_CHARS_PER_LINE)
    return;

  if (chars_per_line > MAX_CHARS_PER_LINE)
    chars_per_line = MAX_CHARS_PER_LINE;

  gdb_printf (stream, "\r");
  for (int i = 0; i < chars_per_line; ++i)
    gdb_printf (stream, " ");
  gdb_printf (stream, "\r");

  gdb_flush (stream);
}

/* Remove the most recent progress update from the progress_info stack
   and overwrite the current line with whitespace.  */

void
cli_ui_out::do_progress_end ()
{
  struct ui_file *stream = m_streams.back ();
  m_progress_info.pop_back ();

  if (stream->isatty ())
    clear_progress_notify ();
}

/* local functions */

void
cli_ui_out::field_separator ()
{
  gdb_putc (' ', m_streams.back ());
}

/* Constructor for cli_ui_out.  */

cli_ui_out::cli_ui_out (ui_file *stream, ui_out_flags flags)
: ui_out (flags),
  m_suppress_output (false)
{
  gdb_assert (stream != NULL);

  m_streams.push_back (stream);
}

cli_ui_out::~cli_ui_out ()
{
}

ui_file *
cli_ui_out::set_stream (struct ui_file *stream)
{
  ui_file *old;

  old = m_streams.back ();
  m_streams.back () = stream;

  return old;
}

bool
cli_ui_out::can_emit_style_escape () const
{
  return m_streams.back ()->can_emit_style_escape ();
}

/* CLI interface to display tab-completion matches.  */

/* CLI version of displayer.crlf.  */

static void
cli_mld_crlf (const struct match_list_displayer *displayer)
{
  rl_crlf ();
}

/* CLI version of displayer.putch.  */

static void
cli_mld_putch (const struct match_list_displayer *displayer, int ch)
{
  putc (ch, rl_outstream);
}

/* CLI version of displayer.puts.  */

static void
cli_mld_puts (const struct match_list_displayer *displayer, const char *s)
{
  fputs (s, rl_outstream);
}

/* CLI version of displayer.flush.  */

static void
cli_mld_flush (const struct match_list_displayer *displayer)
{
  fflush (rl_outstream);
}

extern "C" void _rl_erase_entire_line (void);

/* CLI version of displayer.erase_entire_line.  */

static void
cli_mld_erase_entire_line (const struct match_list_displayer *displayer)
{
  _rl_erase_entire_line ();
}

/* CLI version of displayer.beep.  */

static void
cli_mld_beep (const struct match_list_displayer *displayer)
{
  rl_ding ();
}

/* CLI version of displayer.read_key.  */

static int
cli_mld_read_key (const struct match_list_displayer *displayer)
{
  return rl_read_key ();
}

/* CLI version of rl_completion_display_matches_hook.
   See gdb_display_match_list for a description of the arguments.  */

void
cli_display_match_list (char **matches, int len, int max)
{
  struct match_list_displayer displayer;

  rl_get_screen_size (&displayer.height, &displayer.width);
  displayer.crlf = cli_mld_crlf;
  displayer.putch = cli_mld_putch;
  displayer.puts = cli_mld_puts;
  displayer.flush = cli_mld_flush;
  displayer.erase_entire_line = cli_mld_erase_entire_line;
  displayer.beep = cli_mld_beep;
  displayer.read_key = cli_mld_read_key;

  gdb_display_match_list (matches, len, max, &displayer);
  rl_forced_update_display ();
}
