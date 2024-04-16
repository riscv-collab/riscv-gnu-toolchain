/* Self tests for gdb_tilde_expand

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

#include "gdbsupport/common-defs.h"
#include "gdbsupport/selftest.h"
#include <cstdlib>

#include "gdbsupport/gdb_tilde_expand.h"

namespace selftests {
namespace gdb_tilde_expand_tests {

static void
do_test ()
{
  /* Home directory of the user running the test.  */
  const char *c_home = std::getenv ("HOME");

  /* Skip the test if $HOME is not available in the environment.  */
  if (c_home == nullptr)
    return;

  const std::string home (c_home);

  /* Basic situation.  */
  SELF_CHECK (home == gdb_tilde_expand ("~"));

  /* When given a path that begins by a tilde and refers to a file that
     does not exist, gdb_tilde expand must still be able to do the tilde
     expansion.  */
  SELF_CHECK (gdb_tilde_expand ("~/non/existent/directory")
	      == home + "/non/existent/directory");

  /* gdb_tilde_expand only expands tilde and does not try to do any other
     substitution.  */
  SELF_CHECK (gdb_tilde_expand ("~/*/a.out") == home + "/*/a.out");

  /* gdb_tilde_expand does no modification to a non tilde leading path.  */
  SELF_CHECK (gdb_tilde_expand ("/some/abs/path") == "/some/abs/path");
  SELF_CHECK (gdb_tilde_expand ("relative/path") == "relative/path");

  /* If $USER is available in the env variables, check the '~user'
     expansion.  */
  const char *c_user = std::getenv ("USER");
  if (c_user != nullptr)
    {
      const std::string user (c_user);
      SELF_CHECK (gdb_tilde_expand (("~" + user).c_str ()) == home);
      SELF_CHECK (gdb_tilde_expand (("~" + user + "/a/b").c_str ())
		  == home + "/a/b");
    }

  /* Check that an error is thrown when trying to expand home of a unknown
     user.  */
  try
    {
      gdb_tilde_expand ("~no_one_should_have_that_login/a");
      SELF_CHECK (false);
    }
  catch (const gdb_exception_error &e)
    {
      SELF_CHECK (e.error == GENERIC_ERROR);
      SELF_CHECK
	(*e.message
	 == "Could not find a match for '~no_one_should_have_that_login'.");
    }
}

} /* namespace gdb_tilde_expand_tests */
} /* namespace selftests */

void _initialize_gdb_tilde_expand_selftests ();
void
_initialize_gdb_tilde_expand_selftests ()
{
  selftests::register_test
    ("gdb_tilde_expand", selftests::gdb_tilde_expand_tests::do_test);
}
