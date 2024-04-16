/* Parse a printf-style format string.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_FORMAT_H
#define COMMON_FORMAT_H

#include <string_view>

#if defined(__MINGW32__) && !defined(PRINTF_HAS_LONG_LONG)
# define USE_PRINTF_I64 1
# define PRINTF_HAS_LONG_LONG
#else
# define USE_PRINTF_I64 0
#endif

/* The argclass represents the general type of data that goes with a
   format directive; int_arg for %d, long_arg for %l, and so forth.
   Note that these primarily distinguish types by size and need for
   special handling, so for instance %u and %x are (at present) also
   classed as int_arg.  */

enum argclass
  {
    literal_piece,
    int_arg, long_arg, long_long_arg, size_t_arg, ptr_arg,
    string_arg, wide_string_arg, wide_char_arg,
    double_arg, long_double_arg,
    dec32float_arg, dec64float_arg, dec128float_arg,
    value_arg
  };

/* A format piece is a section of the format string that may include a
   single print directive somewhere in it, and the associated class
   for the argument.  */

struct format_piece
{
  format_piece (const char *str, enum argclass argc, int n)
    : string (str),
      argclass (argc),
      n_int_args (n)
  {
    gdb_assert (str != nullptr);
  }

  bool operator== (const format_piece &other) const
  {
    return (this->argclass == other.argclass
	    && std::string_view (this->string) == other.string);
  }

  const char *string;
  enum argclass argclass;
  /* Count the number of preceding 'int' arguments that must be passed
     along.  This is used for a width or precision of '*'.  Note that
     this feature is only available in "gdb_extensions" mode.  */
  int n_int_args;
};

class format_pieces
{
public:

  format_pieces (const char **arg, bool gdb_extensions = false,
		 bool value_extension = false);
  ~format_pieces () = default;

  DISABLE_COPY_AND_ASSIGN (format_pieces);

  typedef std::vector<format_piece>::iterator iterator;

  iterator begin ()
  {
    return m_pieces.begin ();
  }

  iterator end ()
  {
    return m_pieces.end ();
  }

private:

  std::vector<format_piece> m_pieces;
  gdb::unique_xmalloc_ptr<char> m_storage;
};

#endif /* COMMON_FORMAT_H */
