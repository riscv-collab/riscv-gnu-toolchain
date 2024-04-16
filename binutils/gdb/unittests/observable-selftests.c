/* Self tests for gdb::observers, GDB notifications to observers.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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
#include "gdbsupport/observable.h"

namespace selftests {
namespace observers {

static gdb::observers::observable<int> test_notification ("test_notification");

static int test_first_observer = 0;
static int test_second_observer = 0;
static int test_third_observer = 0;

/* Counters for observers used for dependency tests.  */
static std::vector<int> dependency_test_counters;

/* Tokens for observers used for dependency tests.  */
static gdb::observers::token observer_token0;
static gdb::observers::token observer_token1;
static gdb::observers::token observer_token2;
static gdb::observers::token observer_token3;
static gdb::observers::token observer_token4;
static gdb::observers::token observer_token5;

/* Data for one observer used for checking that dependencies work as expected;
   dependencies are specified using their indices into the 'test_observers'
   vector here for simplicity and mapped to corresponding tokens later.  */
struct dependency_observer_data
{
  gdb::observers::token *token;

  /* Name of the observer to use on attach.  */
  const char *name;

  /* Indices of observers that this one directly depends on.  */
  std::vector<int> direct_dependencies;

  /* Indices for all dependencies, including transitive ones.  */
  std::vector<int> all_dependencies;

  /* Function to attach to the observable for this observer.  */
  std::function<void (int)> callback;
};

static void observer_dependency_test_callback (size_t index);

/* Data for observers to use for dependency tests, using some sample
   dependencies between the observers.  */
static std::vector<dependency_observer_data> test_observers = {
  {&observer_token0, "test0", {}, {},
   [] (int) { observer_dependency_test_callback (0); }},
  {&observer_token1, "test1", {0}, {0},
   [] (int) { observer_dependency_test_callback (1); }},
  {&observer_token2, "test2", {1}, {0, 1},
   [] (int) { observer_dependency_test_callback (2); }},
  {&observer_token3, "test3", {1}, {0, 1},
   [] (int) { observer_dependency_test_callback (3); }},
  {&observer_token4, "test4", {2, 3, 5}, {0, 1, 2, 3, 5},
   [] (int) { observer_dependency_test_callback (4); }},
  {&observer_token5, "test5", {0}, {0},
   [] (int) { observer_dependency_test_callback (5); }},
  {nullptr, "test6", {4}, {0, 1, 2, 3, 4, 5},
   [] (int) { observer_dependency_test_callback (6); }},
  {nullptr, "test7", {0}, {0},
   [] (int) { observer_dependency_test_callback (7); }},
};

static void
test_first_notification_function (int arg)
{
  test_first_observer++;
}

static void
test_second_notification_function (int arg)
{
  test_second_observer++;
}

static void
test_third_notification_function (int arg)
{
  test_third_observer++;
}

static void
notify_check_counters (int one, int two, int three)
{
  /* Reset.  */
  test_first_observer = 0;
  test_second_observer = 0;
  test_third_observer = 0;
  /* Notify.  */
  test_notification.notify (0);
  /* Check.  */
  SELF_CHECK (one == test_first_observer);
  SELF_CHECK (two == test_second_observer);
  SELF_CHECK (three == test_third_observer);
}

/* Function for each observer to run when being notified during the dependency
   tests.  Verify that the observer's dependencies have been notified before the
   observer itself by checking their counters, then increase the observer's own
   counter.  */
static void
observer_dependency_test_callback (size_t index)
{
  /* Check that dependencies have already been notified.  */
  for (int i : test_observers[index].all_dependencies)
    SELF_CHECK (dependency_test_counters[i] == 1);

  /* Increase own counter.  */
  dependency_test_counters[index]++;
}

/* Run a dependency test by attaching the observers in the specified order
   then notifying them.  */
static void
run_dependency_test (std::vector<int> insertion_order)
{
  gdb::observers::observable<int> dependency_test_notification
    ("dependency_test_notification");

  /* Reset counters.  */
  dependency_test_counters = std::vector<int> (test_observers.size (), 0);

  /* Attach all observers in the given order, specifying dependencies.  */
  for (int i : insertion_order)
    {
      const dependency_observer_data &o = test_observers[i];

      /* Get tokens for dependencies using their indices.  */
      std::vector<const gdb::observers::token *> dependency_tokens;
      for (int index : o.all_dependencies)
	dependency_tokens.emplace_back (test_observers[index].token);

      if (o.token != nullptr)
	dependency_test_notification.attach
	  (o.callback, *o.token, o.name, dependency_tokens);
      else
	dependency_test_notification.attach (o.callback, o.name,
					     dependency_tokens);
    }

  /* Notify observers, they check that their dependencies were notified.  */
  dependency_test_notification.notify (1);
}

static void
test_dependency ()
{
  /* Run dependency tests with different insertion orders.  */
  run_dependency_test ({0, 1, 2, 3, 4, 5, 6, 7});
  run_dependency_test ({7, 6, 5, 4, 3, 2, 1, 0});
  run_dependency_test ({0, 3, 2, 1, 7, 6, 4, 5});
}

static void
run_tests ()
{
  /* First, try sending a notification without any observer
     attached.  */
  notify_check_counters (0, 0, 0);

  const gdb::observers::token token1 {}, token2 {} , token3 {};

  /* Now, attach one observer, and send a notification.  */
  test_notification.attach (&test_second_notification_function, token2, "test");
  notify_check_counters (0, 1, 0);

  /* Remove the observer, and send a notification.  */
  test_notification.detach (token2);
  notify_check_counters (0, 0, 0);

  /* With a new observer.  */
  test_notification.attach (&test_first_notification_function, token1, "test");
  notify_check_counters (1, 0, 0);

  /* With 2 observers.  */
  test_notification.attach (&test_second_notification_function, token2, "test");
  notify_check_counters (1, 1, 0);

  /* With 3 observers.  */
  test_notification.attach (&test_third_notification_function, token3, "test");
  notify_check_counters (1, 1, 1);

  /* Remove middle observer.  */
  test_notification.detach (token2);
  notify_check_counters (1, 0, 1);

  /* Remove first observer.  */
  test_notification.detach (token1);
  notify_check_counters (0, 0, 1);

  /* Remove last observer.  */
  test_notification.detach (token3);
  notify_check_counters (0, 0, 0);

  /* Go back to 3 observers, and remove them in a different
     order...  */
  test_notification.attach (&test_first_notification_function, token1, "test");
  test_notification.attach (&test_second_notification_function, token2, "test");
  test_notification.attach (&test_third_notification_function, token3, "test");
  notify_check_counters (1, 1, 1);

  /* Remove the third observer.  */
  test_notification.detach (token3);
  notify_check_counters (1, 1, 0);

  /* Remove the second observer.  */
  test_notification.detach (token2);
  notify_check_counters (1, 0, 0);

  /* Remove first observer, no more observers.  */
  test_notification.detach (token1);
  notify_check_counters (0, 0, 0);
}

} /* namespace observers */
} /* namespace selftests */

void _initialize_observer_selftest ();
void
_initialize_observer_selftest ()
{
  selftests::register_test ("gdb::observers",
			    selftests::observers::run_tests);
  selftests::register_test ("gdb::observers dependency",
			    selftests::observers::test_dependency);
}
