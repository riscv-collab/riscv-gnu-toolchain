/* Self tests for lookup_name_info for GDB, the GNU debugger.

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
#include "symtab.h"

namespace selftests {
namespace lookup_name {

/* Check that removing parameter info out of NAME produces EXPECTED.
   COMPLETION_MODE indicates whether we're testing normal and
   completion mode.  FILE and LINE are used to provide better test
   location information in case the check fails.  */

static void
check_make_paramless (const char *file, int line,
		      enum language lang,
		      const char *name, const char *expected,
		      bool completion_mode)
{
  lookup_name_info lookup_name (name, symbol_name_match_type::FULL,
				completion_mode, true /* ignore_parameters */);
  const char *result = lookup_name.language_lookup_name (lang);

  if (strcmp (result, expected) != 0)
    {
      error (_("%s:%d: make-paramless self-test failed: (completion=%d, lang=%d) "
	       "\"%s\" -> \"%s\", expected \"%s\""),
	     file, line, completion_mode, lang, name,
	     result, expected);
    }
}

static void
run_tests ()
{
  /* Helper for CHECK and CHECK_INCOMPL.  */
#define CHECK_1(INCOMPLETE, LANG, NAME, EXPECTED)			\
  do									\
    {									\
      check_make_paramless (__FILE__, __LINE__,				\
			    LANG, NAME,					\
			    (INCOMPLETE) ? "" : (EXPECTED), false);	\
      check_make_paramless (__FILE__, __LINE__,				\
			    LANG, NAME, EXPECTED, true);		\
    }									\
  while (0)

  /* Check that removing parameter info out of NAME produces EXPECTED.
     Checks both normal and completion modes.  */
#define CHECK(LANG, NAME, EXPECTED)		\
  CHECK_1(false, LANG, NAME, EXPECTED)

  /* Similar, but used when NAME is incomplete -- i.e., NAME has
     unbalanced parentheses.  In this case, looking for the exact name
     should fail / return empty.  */
#define CHECK_INCOMPL(LANG, NAME, EXPECTED)				\
  CHECK_1 (true, LANG, NAME, EXPECTED)

  /* None of these languages support function overloading like C++
     does, so building a nameless lookup name ends up being just the
     same as any other lookup name.  Mainly this serves as smoke test
     that C++-specific code doesn't mess up with other languages that
     use some other scoping character ('.' vs '::').  */
  CHECK (language_ada, "pck.ada_hello", "pck__ada_hello");
  CHECK (language_go, "pck.go_hello", "pck.go_hello");
  CHECK (language_fortran, "mod::func", "mod::func");

  /* D does support function overloading similar to C++, but we're
     missing support for stripping parameters.  At least make sure the
     input name is preserved unmodified.  */
  CHECK (language_d, "pck.d_hello", "pck.d_hello");

  /* Just a few basic tests to make sure
     lookup_name_info::make_paramless is well integrated with
     cp_remove_params_if_any.  gdb/cp-support.c has comprehensive
     testing of C++ specifics.  */
  CHECK (language_cplus, "function()", "function");
  CHECK (language_cplus, "function() const", "function");
  CHECK (language_cplus, "A::B::C()", "A::B::C");
  CHECK (language_cplus, "A::B::C", "A::B::C");

#undef CHECK
#undef CHECK_INCOMPL
}

}} // namespace selftests::lookup_name

void _initialize_lookup_name_info_selftests ();
void
_initialize_lookup_name_info_selftests ()
{
  selftests::register_test ("lookup_name_info",
			    selftests::lookup_name::run_tests);
}
