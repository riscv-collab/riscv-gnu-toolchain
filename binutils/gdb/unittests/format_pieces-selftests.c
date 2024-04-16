/* Self tests for format_pieces for GDB, the GNU debugger.

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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
#include "gdbsupport/format.h"
#include "gdbsupport/selftest.h"

#if USE_PRINTF_I64
#define LL "I64"
#else
#define LL "ll"
#endif

namespace selftests {
namespace format_pieces {

/* Verify that parsing STR gives pieces equal to EXPECTED_PIECES.  */

static void
check (const char *str, const std::vector<format_piece> &expected_pieces,
       bool gdb_format = false)
{
  ::format_pieces pieces (&str, gdb_format);

  SELF_CHECK ((pieces.end () - pieces.begin ()) == expected_pieces.size ());
  SELF_CHECK (std::equal (pieces.begin (), pieces.end (),
			  expected_pieces.begin ()));
}

static void
test_escape_sequences ()
{
  check ("This is an escape sequence: \\e",
    {
      format_piece ("This is an escape sequence: \e", literal_piece, 0),
    });
}

static void
test_format_specifier ()
{
  /* The format string here ends with a % sequence, to ensure we don't
     see a trailing empty literal piece.  */
  check ("Hello\\t %d%llx%%d%d", /* ARI: %ll */
    {
      format_piece ("Hello\t ", literal_piece, 0),
      format_piece ("%d", int_arg, 0),
      format_piece ("%" LL "x", long_long_arg, 0),
      format_piece ("%%d", literal_piece, 0),
      format_piece ("%d", int_arg, 0),
    });
}

static void
test_gdb_formats ()
{
  check ("Hello\\t \"%p[%pF%ps%*.*d%p]\"",
    {
      format_piece ("Hello\\t \"", literal_piece, 0),
      format_piece ("%p[", ptr_arg, 0),
      format_piece ("%pF", ptr_arg, 0),
      format_piece ("%ps", ptr_arg, 0),
      format_piece ("%*.*d", int_arg, 2),
      format_piece ("%p]", ptr_arg, 0),
      format_piece ("\"", literal_piece, 0),
    }, true);
}

/* Test the different size modifiers that can be applied to an integer
   argument.  Test with different integer format specifiers too.  */

static void
test_format_int_sizes ()
{
  check ("Hello\\t %hu%lu%llu%zu", /* ARI: %ll */
    {
      format_piece ("Hello\t ", literal_piece, 0),
      format_piece ("%hu", int_arg, 0),
      format_piece ("%lu", long_arg, 0),
      format_piece ("%" LL "u", long_long_arg, 0),
      format_piece ("%zu", size_t_arg, 0)
    });

  check ("Hello\\t %hx%lx%llx%zx", /* ARI: %ll */
    {
      format_piece ("Hello\t ", literal_piece, 0),
      format_piece ("%hx", int_arg, 0),
      format_piece ("%lx", long_arg, 0),
      format_piece ("%" LL "x", long_long_arg, 0),
      format_piece ("%zx", size_t_arg, 0)
    });

  check ("Hello\\t %ho%lo%llo%zo", /* ARI: %ll */
    {
      format_piece ("Hello\t ", literal_piece, 0),
      format_piece ("%ho", int_arg, 0),
      format_piece ("%lo", long_arg, 0),
      format_piece ("%" LL "o", long_long_arg, 0),
      format_piece ("%zo", size_t_arg, 0)
    });

  check ("Hello\\t %hd%ld%lld%zd", /* ARI: %ll */
    {
      format_piece ("Hello\t ", literal_piece, 0),
      format_piece ("%hd", int_arg, 0),
      format_piece ("%ld", long_arg, 0),
      format_piece ("%" LL "d", long_long_arg, 0),
      format_piece ("%zd", size_t_arg, 0)
    });
}

static void
test_windows_formats ()
{
  check ("rc%I64d",
    {
     format_piece ("rc", literal_piece, 0),
     format_piece ("%I64d", long_long_arg, 0),
    });
}

static void
run_tests ()
{
  test_escape_sequences ();
  test_format_specifier ();
  test_gdb_formats ();
  test_format_int_sizes ();
  test_windows_formats ();
}

} /* namespace format_pieces */
} /* namespace selftests */

void _initialize_format_pieces_selftests ();
void
_initialize_format_pieces_selftests ()
{
  selftests::register_test ("format_pieces",
			    selftests::format_pieces::run_tests);
}
