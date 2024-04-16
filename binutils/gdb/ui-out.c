/* Output generating routines for GDB.

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
#include "expression.h"
#include "language.h"
#include "ui-out.h"
#include "gdbsupport/format.h"
#include "cli/cli-style.h"
#include "diagnostics.h"

#include <vector>
#include <memory>
#include <string>

namespace {

/* A header of a ui_out_table.  */

class ui_out_hdr
{
 public:

  explicit ui_out_hdr (int number, int min_width, ui_align alignment,
		       const std::string &name, const std::string &header)
  : m_number (number),
    m_min_width (min_width),
    m_alignment (alignment),
    m_name (name),
    m_header (header)
  {
  }

  int number () const
  {
    return m_number;
  }

  int min_width () const
  {
    return m_min_width;
  }

  ui_align alignment () const
  {
    return m_alignment;
  }

  const std::string &header () const
  {
    return m_header;
  }

  const std::string &name () const
  {
    return m_name;
  }

 private:

  /* The number of the table column this header represents, 1-based.  */
  int m_number;

  /* Minimal column width in characters.  May or may not be applicable,
     depending on the actual implementation of ui_out.  */
  int m_min_width;

  /* Alignment of the content in the column.  May or may not be applicable,
     depending on the actual implementation of ui_out.  */
  ui_align m_alignment;

  /* Internal column name, used to internally refer to the column.  */
  std::string m_name;

  /* Printed header text of the column.  */
  std::string m_header;
};

} // namespace

/* A level of nesting (either a list or a tuple) in a ui_out output.  */

class ui_out_level
{
 public:

  explicit ui_out_level (ui_out_type type)
  : m_type (type),
    m_field_count (0)
  {
  }

  ui_out_type type () const
  {
    return m_type;
  }

  int field_count () const
  {
    return m_field_count;
  }

  void inc_field_count ()
  {
    m_field_count++;
  }

 private:

  /* The type of this level.  */
  ui_out_type m_type;

  /* Count each field; the first element is for non-list fields.  */
  int m_field_count;
};

/* Tables are special.  Maintain a separate structure that tracks
   their state.  At present an output can only contain a single table
   but that restriction might eventually be lifted.  */

class ui_out_table
{
 public:

  /* States (steps) of a table generation.  */

  enum class state
  {
    /* We are generating the table headers.  */
    HEADERS,

    /* We are generating the table body.  */
    BODY,
  };

  explicit ui_out_table (int entry_level, int nr_cols, const std::string &id)
  : m_state (state::HEADERS),
    m_entry_level (entry_level),
    m_nr_cols (nr_cols),
    m_id (id)
  {
  }

  /* Start building the body of the table.  */

  void start_body ();

  /* Add a new header to the table.  */

  void append_header (int width, ui_align alignment,
		      const std::string &col_name, const std::string &col_hdr);

  void start_row ();

  /* Extract the format information for the next header and advance
     the header iterator.  Return false if there was no next header.  */

  bool get_next_header (int *colno, int *width, ui_align *alignment,
		       const char **col_hdr);

  bool query_field (int colno, int *width, int *alignment,
		    const char **col_name) const;

  state current_state () const;

  int entry_level () const;

 private:

  state m_state;

  /* The level at which each entry of the table is to be found.  A row
     (a tuple) is made up of entries.  Consequently ENTRY_LEVEL is one
     above that of the table.  */
  int m_entry_level;

  /* Number of table columns (as specified in the table_begin call).  */
  int m_nr_cols;

  /* String identifying the table (as specified in the table_begin
     call).  */
  std::string m_id;

  /* Pointers to the column headers.  */
  std::vector<std::unique_ptr<ui_out_hdr>> m_headers;

  /* Iterator over the headers vector, used when printing successive fields.  */
  std::vector<std::unique_ptr<ui_out_hdr>>::const_iterator m_headers_iterator;
};

/* See ui-out.h.  */

void ui_out_table::start_body ()
{
  if (m_state != state::HEADERS)
    internal_error (_("extra table_body call not allowed; there must be only "
		      "one table_body after a table_begin and before a "
		      "table_end."));

  /* Check if the number of defined headers matches the number of expected
     columns.  */
  if (m_headers.size () != m_nr_cols)
    internal_error (_("number of headers differ from number of table "
		      "columns."));

  m_state = state::BODY;
  m_headers_iterator = m_headers.begin ();
}

/* See ui-out.h.  */

void ui_out_table::append_header (int width, ui_align alignment,
				  const std::string &col_name,
				  const std::string &col_hdr)
{
  if (m_state != state::HEADERS)
    internal_error (_("table header must be specified after table_begin and "
		      "before table_body."));

  auto header = std::make_unique<ui_out_hdr> (m_headers.size () + 1,
					      width, alignment,
					      col_name, col_hdr);

  m_headers.push_back (std::move (header));
}

/* See ui-out.h.  */

void ui_out_table::start_row ()
{
  m_headers_iterator = m_headers.begin ();
}

/* See ui-out.h.  */

bool ui_out_table::get_next_header (int *colno, int *width, ui_align *alignment,
				    const char **col_hdr)
{
  /* There may be no headers at all or we may have used all columns.  */
  if (m_headers_iterator == m_headers.end ())
    return false;

  ui_out_hdr *hdr = m_headers_iterator->get ();

  *colno = hdr->number ();
  *width = hdr->min_width ();
  *alignment = hdr->alignment ();
  *col_hdr = hdr->header ().c_str ();

  /* Advance the header pointer to the next entry.  */
  m_headers_iterator++;

  return true;
}

/* See ui-out.h.  */

bool ui_out_table::query_field (int colno, int *width, int *alignment,
				const char **col_name) const
{
  /* Column numbers are 1-based, so convert to 0-based index.  */
  int index = colno - 1;

  if (index >= 0 && index < m_headers.size ())
    {
      ui_out_hdr *hdr = m_headers[index].get ();

      gdb_assert (colno == hdr->number ());

      *width = hdr->min_width ();
      *alignment = hdr->alignment ();
      *col_name = hdr->name ().c_str ();

      return true;
    }
  else
    return false;
}

/* See ui-out.h.  */

ui_out_table::state ui_out_table::current_state () const
{
  return m_state;
}

/* See ui-out.h.  */

int ui_out_table::entry_level () const
{
  return m_entry_level;
}

int
ui_out::level () const
{
  return m_levels.size ();
}

/* The current (inner most) level.  */

ui_out_level *
ui_out::current_level () const
{
  return m_levels.back ().get ();
}

/* Create a new level, of TYPE.  */
void
ui_out::push_level (ui_out_type type)
{
  auto level = std::make_unique<ui_out_level> (type);

  m_levels.push_back (std::move (level));
}

/* Discard the current level.  TYPE is the type of the level being
   discarded.  */
void
ui_out::pop_level (ui_out_type type)
{
  /* We had better not underflow the buffer.  */
  gdb_assert (m_levels.size () > 0);
  gdb_assert (current_level ()->type () == type);

  m_levels.pop_back ();
}

/* Mark beginning of a table.  */

void
ui_out::table_begin (int nr_cols, int nr_rows, const std::string &tblid)
{
  if (m_table_up != nullptr)
    internal_error (_("tables cannot be nested; table_begin found before \
previous table_end."));

  m_table_up.reset (new ui_out_table (level () + 1, nr_cols, tblid));

  do_table_begin (nr_cols, nr_rows, tblid.c_str ());
}

void
ui_out::table_header (int width, ui_align alignment,
		      const std::string &col_name, const std::string &col_hdr)
{
  if (m_table_up == nullptr)
    internal_error (_("table_header outside a table is not valid; it must be \
after a table_begin and before a table_body."));

  m_table_up->append_header (width, alignment, col_name, col_hdr);

  do_table_header (width, alignment, col_name, col_hdr);
}

void
ui_out::table_body ()
{
  if (m_table_up == nullptr)
    internal_error (_("table_body outside a table is not valid; it must be "
		      "after a table_begin and before a table_end."));

  m_table_up->start_body ();

  do_table_body ();
}

void
ui_out::table_end ()
{
  if (m_table_up == nullptr)
    internal_error (_("misplaced table_end or missing table_begin."));

  do_table_end ();

  m_table_up = nullptr;
}

void
ui_out::begin (ui_out_type type, const char *id)
{
  /* Be careful to verify the ``field'' before the new tuple/list is
     pushed onto the stack.  That way the containing list/table/row is
     verified and not the newly created tuple/list.  This verification
     is needed (at least) for the case where a table row entry
     contains either a tuple/list.  For that case bookkeeping such as
     updating the column count or advancing to the next heading still
     needs to be performed.  */
  {
    int fldno;
    int width;
    ui_align align;

    verify_field (&fldno, &width, &align);
  }

  push_level (type);

  /* If the push puts us at the same level as a table row entry, we've
     got a new table row.  Put the header pointer back to the start.  */
  if (m_table_up != nullptr
      && m_table_up->current_state () == ui_out_table::state::BODY
      && m_table_up->entry_level () == level ())
    m_table_up->start_row ();

  do_begin (type, id);
}

void
ui_out::end (ui_out_type type)
{
  pop_level (type);

  do_end (type);
}

void
ui_out::field_signed (const char *fldname, LONGEST value)
{
  int fldno;
  int width;
  ui_align align;

  verify_field (&fldno, &width, &align);

  do_field_signed (fldno, width, align, fldname, value);
}

void
ui_out::field_fmt_signed (int input_width, ui_align input_align,
			  const char *fldname, LONGEST value)
{
  int fldno;
  int width;
  ui_align align;

  verify_field (&fldno, &width, &align);

  do_field_signed (fldno, input_width, input_align, fldname, value);
}

/* See ui-out.h.  */

void
ui_out::field_unsigned (const char *fldname, ULONGEST value)
{
  int fldno;
  int width;
  ui_align align;

  verify_field (&fldno, &width, &align);

  do_field_unsigned (fldno, width, align, fldname, value);
}

/* Documented in ui-out.h.  */

void
ui_out::field_core_addr (const char *fldname, struct gdbarch *gdbarch,
			 CORE_ADDR address)
{
  field_string (fldname, print_core_address (gdbarch, address),
		address_style.style ());
}

void
ui_out::field_stream (const char *fldname, string_file &stream,
		      const ui_file_style &style)
{
  if (!stream.empty ())
    field_string (fldname, stream.c_str (), style);
  else
    field_skip (fldname);
  stream.clear ();
}

/* Used to omit a field.  */

void
ui_out::field_skip (const char *fldname)
{
  int fldno;
  int width;
  ui_align align;

  verify_field (&fldno, &width, &align);

  do_field_skip (fldno, width, align, fldname);
}

void
ui_out::field_string (const char *fldname, const char *string,
		      const ui_file_style &style)
{
  int fldno;
  int width;
  ui_align align;

  verify_field (&fldno, &width, &align);

  do_field_string (fldno, width, align, fldname, string, style);
}

/* VARARGS */
void
ui_out::field_fmt (const char *fldname, const char *format, ...)
{
  va_list args;
  int fldno;
  int width;
  ui_align align;

  verify_field (&fldno, &width, &align);

  va_start (args, format);

  do_field_fmt (fldno, width, align, fldname, ui_file_style (), format, args);

  va_end (args);
}

void
ui_out::field_fmt (const char *fldname, const ui_file_style &style,
		   const char *format, ...)
{
  va_list args;
  int fldno;
  int width;
  ui_align align;

  verify_field (&fldno, &width, &align);

  va_start (args, format);

  do_field_fmt (fldno, width, align, fldname, style, format, args);

  va_end (args);
}

void
ui_out::spaces (int numspaces)
{
  do_spaces (numspaces);
}

void
ui_out::text (const char *string)
{
  do_text (string);
}

void
ui_out::call_do_message (const ui_file_style &style, const char *format,
			 ...)
{
  va_list args;

  va_start (args, format);

  /* Since call_do_message is only used as a helper of vmessage, silence the
     warning here once instead of at all call sites in vmessage, if we were
     to put a "format" attribute on call_do_message.  */
  DIAGNOSTIC_PUSH
  DIAGNOSTIC_IGNORE_FORMAT_NONLITERAL
  do_message (style, format, args);
  DIAGNOSTIC_POP

  va_end (args);
}

void
ui_out::vmessage (const ui_file_style &in_style, const char *format,
		  va_list args)
{
  format_pieces fpieces (&format, true);

  ui_file_style style = in_style;

  for (auto &&piece : fpieces)
    {
      const char *current_substring = piece.string;

      gdb_assert (piece.n_int_args >= 0 && piece.n_int_args <= 2);
      int intvals[2] = { 0, 0 };
      for (int i = 0; i < piece.n_int_args; ++i)
	intvals[i] = va_arg (args, int);

      /* The only ones we support for now.  */
      gdb_assert (piece.n_int_args == 0
		  || piece.argclass == string_arg
		  || piece.argclass == int_arg
		  || piece.argclass == long_arg);

      switch (piece.argclass)
	{
	case string_arg:
	  {
	    const char *str = va_arg (args, const char *);
	    switch (piece.n_int_args)
	      {
	      case 0:
		call_do_message (style, current_substring, str);
		break;
	      case 1:
		call_do_message (style, current_substring, intvals[0], str);
		break;
	      case 2:
		call_do_message (style, current_substring,
				 intvals[0], intvals[1], str);
		break;
	      }
	  }
	  break;
	case wide_string_arg:
	  gdb_assert_not_reached ("wide_string_arg not supported in vmessage");
	  break;
	case wide_char_arg:
	  gdb_assert_not_reached ("wide_char_arg not supported in vmessage");
	  break;
	case long_long_arg:
	  call_do_message (style, current_substring, va_arg (args, long long));
	  break;
	case int_arg:
	  {
	    int val = va_arg (args, int);
	    switch (piece.n_int_args)
	      {
	      case 0:
		call_do_message (style, current_substring, val);
		break;
	      case 1:
		call_do_message (style, current_substring, intvals[0], val);
		break;
	      case 2:
		call_do_message (style, current_substring,
				 intvals[0], intvals[1], val);
		break;
	      }
	  }
	  break;
	case long_arg:
	  {
	    long val = va_arg (args, long);
	    switch (piece.n_int_args)
	      {
	      case 0:
		call_do_message (style, current_substring, val);
		break;
	      case 1:
		call_do_message (style, current_substring, intvals[0], val);
		break;
	      case 2:
		call_do_message (style, current_substring,
				 intvals[0], intvals[1], val);
		break;
	      }
	  }
	  break;
	case size_t_arg:
	  {
	    size_t val = va_arg (args, size_t);
	    switch (piece.n_int_args)
	      {
	      case 0:
		call_do_message (style, current_substring, val);
		break;
	      case 1:
		call_do_message (style, current_substring, intvals[0], val);
		break;
	      case 2:
		call_do_message (style, current_substring,
				 intvals[0], intvals[1], val);
		break;
	      }
	  }
	  break;
	case double_arg:
	  call_do_message (style, current_substring, va_arg (args, double));
	  break;
	case long_double_arg:
	  gdb_assert_not_reached ("long_double_arg not supported in vmessage");
	  break;
	case dec32float_arg:
	  gdb_assert_not_reached ("dec32float_arg not supported in vmessage");
	  break;
	case dec64float_arg:
	  gdb_assert_not_reached ("dec64float_arg not supported in vmessage");
	  break;
	case dec128float_arg:
	  gdb_assert_not_reached ("dec128float_arg not supported in vmessage");
	  break;
	case ptr_arg:
	  switch (current_substring[2])
	    {
	    case 'F':
	      {
		gdb_assert (!test_flags (disallow_ui_out_field));
		base_field_s *bf = va_arg (args, base_field_s *);
		switch (bf->kind)
		  {
		  case field_kind::FIELD_SIGNED:
		    {
		      auto *f = (signed_field_s *) bf;
		      field_signed (f->name, f->val);
		    }
		    break;
		  case field_kind::FIELD_STRING:
		    {
		      auto *f = (string_field_s *) bf;
		      field_string (f->name, f->str);
		    }
		    break;
		  }
	      }
	      break;
	    case 's':
	      {
		styled_string_s *ss = va_arg (args, styled_string_s *);
		call_do_message (ss->style, "%s", ss->str);
	      }
	      break;
	    case '[':
	      style = *va_arg (args, const ui_file_style *);
	      break;
	    case ']':
	      {
		void *arg = va_arg (args, void *);
		gdb_assert (arg == nullptr);

		style = {};
	      }
	      break;
	    default:
	      call_do_message (style, current_substring, va_arg (args, void *));
	      break;
	    }
	  break;
	case literal_piece:
	  /* Print a portion of the format string that has no
	     directives.  Note that this will not include any ordinary
	     %-specs, but it might include "%%".  That is why we use
	     call_do_message here.  Also, we pass a dummy argument
	     because some platforms have modified GCC to include
	     -Wformat-security by default, which will warn here if
	     there is no argument.  */
	  call_do_message (style, current_substring, 0);
	  break;
	default:
	  internal_error (_("failed internal consistency check"));
	}
    }
}

void
ui_out::message (const char *format, ...)
{
  va_list args;
  va_start (args, format);

  vmessage (ui_file_style (), format, args);

  va_end (args);
}

void
ui_out::wrap_hint (int indent)
{
  do_wrap_hint (indent);
}

void
ui_out::flush ()
{
  do_flush ();
}

void
ui_out::redirect (ui_file *outstream)
{
  do_redirect (outstream);
}

/* Test the flags against the mask given.  */
ui_out_flags
ui_out::test_flags (ui_out_flags mask)
{
  return m_flags & mask;
}

bool
ui_out::is_mi_like_p () const
{
  return do_is_mi_like_p ();
}

/* Verify that the field/tuple/list is correctly positioned.  Return
   the field number and corresponding alignment (if
   available/applicable).  */

void
ui_out::verify_field (int *fldno, int *width, ui_align *align)
{
  ui_out_level *current = current_level ();
  const char *text;

  if (m_table_up != nullptr
      && m_table_up->current_state () != ui_out_table::state::BODY)
    {
      internal_error (_("table_body missing; table fields must be \
specified after table_body and inside a list."));
    }

  current->inc_field_count ();

  if (m_table_up != nullptr
      && m_table_up->current_state () == ui_out_table::state::BODY
      && m_table_up->entry_level () == level ()
      && m_table_up->get_next_header (fldno, width, align, &text))
    {
      if (*fldno != current->field_count ())
	internal_error (_("ui-out internal error in handling headers."));
    }
  else
    {
      *width = 0;
      *align = ui_noalign;
      *fldno = current->field_count ();
    }
}

/* Access table field parameters.  */

bool
ui_out::query_table_field (int colno, int *width, int *alignment,
			   const char **col_name)
{
  if (m_table_up == nullptr)
    return false;

  return m_table_up->query_field (colno, width, alignment, col_name);
}

/* The constructor.  */

ui_out::ui_out (ui_out_flags flags)
: m_flags (flags)
{
  /* Create the ui-out level #1, the default level.  */
  push_level (ui_out_type_tuple);
}

ui_out::~ui_out ()
{
}
