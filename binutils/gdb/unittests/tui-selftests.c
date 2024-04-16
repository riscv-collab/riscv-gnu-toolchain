/* Self tests for the TUI

   Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

#ifdef TUI

#include "tui/tui-winsource.h"

namespace selftests {
namespace tui {

static void
run_tests ()
{
  const char *text = "hello";
  std::string result = tui_copy_source_line (&text);
  SELF_CHECK (result == "hello");
  SELF_CHECK (*text == '\0');

  text = "hello\n";
  result = tui_copy_source_line (&text);
  SELF_CHECK (result == "hello");
  SELF_CHECK (*text == '\0');
}

} /* namespace tui*/
} /* namespace selftests */

#endif /* TUI */

void _initialize_tui_selftest ();
void
_initialize_tui_selftest ()
{
#ifdef TUI
  selftests::register_test ("tui", selftests::tui::run_tests);
#endif
}
