/* Self tests for ui_file_style

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
#include "gdbsupport/selftest.h"
#include "ui-style.h"

namespace selftests {
namespace style {

#define CHECK_RGB(R, G, B) \
  SELF_CHECK (rgb[0] == (R) && rgb[1] == (G) && rgb[2] == (B))

static void
run_tests ()
{
  ui_file_style style;
  size_t n_read;
  uint8_t rgb[3];

  SELF_CHECK (style.parse ("\033[m", &n_read));
  SELF_CHECK (n_read == 3);
  SELF_CHECK (style.get_foreground ().is_none ());
  SELF_CHECK (style.get_background ().is_none ());
  SELF_CHECK (style.get_intensity () == ui_file_style::NORMAL);
  SELF_CHECK (!style.is_reverse ());
  SELF_CHECK (style.to_ansi () == "\033[m");

  style = ui_file_style ();
  SELF_CHECK (style.parse ("\033[0m", &n_read));
  SELF_CHECK (n_read == 4);
  SELF_CHECK (style.get_foreground ().is_none ());
  SELF_CHECK (style.get_background ().is_none ());
  SELF_CHECK (style.get_intensity () == ui_file_style::NORMAL);
  SELF_CHECK (!style.is_reverse ());
  /* This particular case does not round-trip identically, but the
     difference is unimportant.  */
  SELF_CHECK (style.to_ansi () == "\033[m");

  SELF_CHECK (style.parse ("\033[7m", &n_read));
  SELF_CHECK (n_read == 4);
  SELF_CHECK (style.get_foreground ().is_none ());
  SELF_CHECK (style.get_background ().is_none ());
  SELF_CHECK (style.get_intensity () == ui_file_style::NORMAL);
  SELF_CHECK (style.is_reverse ());
  SELF_CHECK (style.to_ansi () == "\033[7m");

  style = ui_file_style ();
  SELF_CHECK (style.parse ("\033[32;1m", &n_read));
  SELF_CHECK (n_read == 7);
  SELF_CHECK (style.get_foreground ().is_basic ());
  SELF_CHECK (style.get_foreground ().get_value () == ui_file_style::GREEN);
  SELF_CHECK (style.get_background ().is_none ());
  SELF_CHECK (style.get_intensity () == ui_file_style::BOLD);
  SELF_CHECK (!style.is_reverse ());
  SELF_CHECK (style.to_ansi () == "\033[32;1m");

  style = ui_file_style ();
  SELF_CHECK (style.parse ("\033[38;5;112;48;5;249m", &n_read));
  SELF_CHECK (n_read == 20);
  SELF_CHECK (!style.get_foreground ().is_basic ());
  style.get_foreground ().get_rgb (rgb);
  CHECK_RGB (0x87, 0xd7, 0);
  SELF_CHECK (!style.get_background ().is_basic ());
  style.get_background ().get_rgb (rgb);
  CHECK_RGB (0xb2, 0xb2, 0xb2);
  SELF_CHECK (style.get_intensity () == ui_file_style::NORMAL);
  SELF_CHECK (!style.is_reverse ());
  SELF_CHECK (style.to_ansi () == "\033[38;5;112;48;5;249m");

  style = ui_file_style ();
  SELF_CHECK (style.parse ("\033[38;2;83;84;85;48;2;0;1;254;2;7m", &n_read));
  SELF_CHECK (n_read == 33);
  SELF_CHECK (!style.get_foreground ().is_basic ());
  style.get_foreground ().get_rgb (rgb);
  CHECK_RGB (83, 84, 85);
  SELF_CHECK (!style.get_background ().is_basic ());
  style.get_background ().get_rgb (rgb);
  CHECK_RGB (0, 1, 254);
  SELF_CHECK (style.get_intensity () == ui_file_style::DIM);
  SELF_CHECK (style.is_reverse ());
  SELF_CHECK (style.to_ansi () == "\033[38;2;83;84;85;48;2;0;1;254;2;7m");
}

} /* namespace style */
} /* namespace selftests */

void _initialize_style_selftest ();
void
_initialize_style_selftest ()
{
  selftests::register_test ("style",
			    selftests::style::run_tests);
}
