/* Self tests for gdb_environ for GDB, the GNU debugger.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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
#include "gdbsupport/environ.h"
#include "diagnostics.h"

static const char gdb_selftest_env_var[] = "GDB_SELFTEST_ENVIRON";

static bool
set_contains (const std::set<std::string> &set, std::string key)
{
  return set.find (key) != set.end ();
}

namespace selftests {
namespace gdb_environ_tests {

/* Test if the vector is initialized in a correct way.  */

static void
test_vector_initialization ()
{
  gdb_environ env;

  /* When the vector is initialized, there should always be one NULL
     element in it.  */
  SELF_CHECK (env.envp ()[0] == NULL);
  SELF_CHECK (env.user_set_env ().size () == 0);
  SELF_CHECK (env.user_unset_env ().size () == 0);

  /* Make sure that there is no other element.  */
  SELF_CHECK (env.get ("PWD") == NULL);
}

/* Test initialization using the host's environ.  */

static void
test_init_from_host_environ ()
{
  /* Initialize the environment vector using the host's environ.  */
  gdb_environ env = gdb_environ::from_host_environ ();

  /* The user-set and user-unset lists must be empty.  */
  SELF_CHECK (env.user_set_env ().size () == 0);
  SELF_CHECK (env.user_unset_env ().size () == 0);

  /* Our test environment variable should be present at the
     vector.  */
  SELF_CHECK (strcmp (env.get (gdb_selftest_env_var), "1") == 0);
}

/* Test reinitialization using the host's environ.  */

static void
test_reinit_from_host_environ ()
{
  /* Reinitialize our environ vector using the host environ.  We
     should be able to see one (and only one) instance of the test
     variable.  */
  gdb_environ env = gdb_environ::from_host_environ ();
  env = gdb_environ::from_host_environ ();
  char **envp = env.envp ();
  int num_found = 0;

  for (size_t i = 0; envp[i] != NULL; ++i)
    if (strcmp (envp[i], "GDB_SELFTEST_ENVIRON=1") == 0)
      ++num_found;
  SELF_CHECK (num_found == 1);
}

/* Test the case when we set a variable A, then set a variable B,
   then unset A, and make sure that we cannot find A in the environ
   vector, but can still find B.  */

static void
test_set_A_unset_B_unset_A_cannot_find_A_can_find_B ()
{
  gdb_environ env;

  env.set ("GDB_SELFTEST_ENVIRON_1", "aaa");
  SELF_CHECK (strcmp (env.get ("GDB_SELFTEST_ENVIRON_1"), "aaa") == 0);
  /* User-set environ var list must contain one element.  */
  SELF_CHECK (env.user_set_env ().size () == 1);
  SELF_CHECK (set_contains (env.user_set_env (),
			    std::string ("GDB_SELFTEST_ENVIRON_1=aaa")));

  env.set ("GDB_SELFTEST_ENVIRON_2", "bbb");
  SELF_CHECK (strcmp (env.get ("GDB_SELFTEST_ENVIRON_2"), "bbb") == 0);

  env.unset ("GDB_SELFTEST_ENVIRON_1");
  SELF_CHECK (env.get ("GDB_SELFTEST_ENVIRON_1") == NULL);
  SELF_CHECK (strcmp (env.get ("GDB_SELFTEST_ENVIRON_2"), "bbb") == 0);

  /* The user-set environ var list must contain only one element
     now.  */
  SELF_CHECK (set_contains (env.user_set_env (),
			    std::string ("GDB_SELFTEST_ENVIRON_2=bbb")));
  SELF_CHECK (env.user_set_env ().size () == 1);
}

/* Check if unset followed by a set in an empty vector works.  */

static void
test_unset_set_empty_vector ()
{
  gdb_environ env;

  env.set ("PWD", "test");
  SELF_CHECK (strcmp (env.get ("PWD"), "test") == 0);
  SELF_CHECK (set_contains (env.user_set_env (), std::string ("PWD=test")));
  SELF_CHECK (env.user_unset_env ().size () == 0);
  /* The second element must be NULL.  */
  SELF_CHECK (env.envp ()[1] == NULL);
  SELF_CHECK (env.user_set_env ().size () == 1);
  env.unset ("PWD");
  SELF_CHECK (env.envp ()[0] == NULL);
  SELF_CHECK (env.user_set_env ().size () == 0);
  SELF_CHECK (env.user_unset_env ().size () == 1);
  SELF_CHECK (set_contains (env.user_unset_env (), std::string ("PWD")));
}

/* When we clear our environ vector, there should be only one
   element on it (NULL), and we shouldn't be able to get our test
   variable.  */

static void
test_vector_clear ()
{
  gdb_environ env;

  env.set (gdb_selftest_env_var, "1");

  env.clear ();
  SELF_CHECK (env.envp ()[0] == NULL);
  SELF_CHECK (env.user_set_env ().size () == 0);
  SELF_CHECK (env.user_unset_env ().size () == 0);
  SELF_CHECK (env.get (gdb_selftest_env_var) == NULL);
}

/* Test that after a std::move the moved-from object is left at a
   valid state (i.e., its only element is NULL).  */

static void
test_std_move ()
{
  gdb_environ env;
  gdb_environ env2;

  env.set ("A", "1");
  SELF_CHECK (strcmp (env.get ("A"), "1") == 0);
  SELF_CHECK (set_contains (env.user_set_env (), std::string ("A=1")));
  SELF_CHECK (env.user_set_env ().size () == 1);

  env2 = std::move (env);
  SELF_CHECK (env.envp ()[0] == NULL);
  SELF_CHECK (strcmp (env2.get ("A"), "1") == 0);
  SELF_CHECK (env2.envp ()[1] == NULL);
  SELF_CHECK (env.user_set_env ().size () == 0);
  SELF_CHECK (set_contains (env2.user_set_env (), std::string ("A=1")));
  SELF_CHECK (env2.user_set_env ().size () == 1);
  env.set ("B", "2");
  SELF_CHECK (strcmp (env.get ("B"), "2") == 0);
  SELF_CHECK (env.envp ()[1] == NULL);
}

/* Test that the move constructor leaves everything at a valid
   state.  */

static void
test_move_constructor ()
{
  gdb_environ env;

  env.set ("A", "1");
  SELF_CHECK (strcmp (env.get ("A"), "1") == 0);
  SELF_CHECK (set_contains (env.user_set_env (), std::string ("A=1")));

  gdb_environ env2 = std::move (env);
  SELF_CHECK (env.envp ()[0] == NULL);
  SELF_CHECK (env.user_set_env ().size () == 0);
  SELF_CHECK (strcmp (env2.get ("A"), "1") == 0);
  SELF_CHECK (env2.envp ()[1] == NULL);
  SELF_CHECK (set_contains (env2.user_set_env (), std::string ("A=1")));
  SELF_CHECK (env2.user_set_env ().size () == 1);

  env.set ("B", "2");
  SELF_CHECK (strcmp (env.get ("B"), "2") == 0);
  SELF_CHECK (env.envp ()[1] == NULL);
  SELF_CHECK (set_contains (env.user_set_env (), std::string ("B=2")));
  SELF_CHECK (env.user_set_env ().size () == 1);
}

/* Test that self-moving works.  */

static void
test_self_move ()
{
  gdb_environ env;

  /* Test self-move.  */
  env.set ("A", "1");
  SELF_CHECK (strcmp (env.get ("A"), "1") == 0);
  SELF_CHECK (set_contains (env.user_set_env (), std::string ("A=1")));
  SELF_CHECK (env.user_set_env ().size () == 1);

  /* Some compilers warn about moving to self, but that's precisely what we want
     to test here, so turn this warning off.  */
  DIAGNOSTIC_PUSH
  DIAGNOSTIC_IGNORE_SELF_MOVE
  env = std::move (env);
  DIAGNOSTIC_POP

  SELF_CHECK (strcmp (env.get ("A"), "1") == 0);
  SELF_CHECK (strcmp (env.envp ()[0], "A=1") == 0);
  SELF_CHECK (env.envp ()[1] == NULL);
  SELF_CHECK (set_contains (env.user_set_env (), std::string ("A=1")));
  SELF_CHECK (env.user_set_env ().size () == 1);
}

/* Test if set/unset/reset works.  */

static void
test_set_unset_reset ()
{
  gdb_environ env = gdb_environ::from_host_environ ();

  /* Our test variable should already be present in the host's environment.  */
  SELF_CHECK (env.get ("GDB_SELFTEST_ENVIRON") != NULL);

  /* Set our test variable to another value.  */
  env.set ("GDB_SELFTEST_ENVIRON", "test");
  SELF_CHECK (strcmp (env.get ("GDB_SELFTEST_ENVIRON"), "test") == 0);
  SELF_CHECK (env.user_set_env ().size () == 1);
  SELF_CHECK (env.user_unset_env ().size () == 0);

  /* And unset our test variable.  The variable still exists in the
     host's environment, but doesn't exist in our vector.  */
  env.unset ("GDB_SELFTEST_ENVIRON");
  SELF_CHECK (env.get ("GDB_SELFTEST_ENVIRON") == NULL);
  SELF_CHECK (env.user_set_env ().size () == 0);
  SELF_CHECK (env.user_unset_env ().size () == 1);
  SELF_CHECK (set_contains (env.user_unset_env (),
			    std::string ("GDB_SELFTEST_ENVIRON")));

  /* Re-set the test variable.  */
  env.set ("GDB_SELFTEST_ENVIRON", "1");
  SELF_CHECK (strcmp (env.get ("GDB_SELFTEST_ENVIRON"), "1") == 0);
}

static void
run_tests ()
{
  /* Set a test environment variable.  */
  if (setenv ("GDB_SELFTEST_ENVIRON", "1", 1) != 0)
    error (_("Could not set environment variable for testing."));

  test_vector_initialization ();

  test_unset_set_empty_vector ();

  test_init_from_host_environ ();

  test_set_unset_reset ();

  test_vector_clear ();

  test_reinit_from_host_environ ();

  /* Get rid of our test variable.  We won't need it anymore.  */
  unsetenv ("GDB_SELFTEST_ENVIRON");

  test_set_A_unset_B_unset_A_cannot_find_A_can_find_B ();

  test_std_move ();

  test_move_constructor ();

  test_self_move ();
}
} /* namespace gdb_environ */
} /* namespace selftests */

void _initialize_environ_selftests ();
void
_initialize_environ_selftests ()
{
  selftests::register_test ("gdb_environ",
			    selftests::gdb_environ_tests::run_tests);
}
