/* Producer string parsers for GDB.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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
#include "producer.h"
#include "gdbsupport/selftest.h"
#include "gdbsupport/gdb_regex.h"

/* See producer.h.  */

int
producer_is_gcc_ge_4 (const char *producer)
{
  int major, minor;

  if (! producer_is_gcc (producer, &major, &minor))
    return -1;
  if (major < 4)
    return -1;
  if (major > 4)
    return INT_MAX;
  return minor;
}

/* See producer.h.  */

int
producer_is_gcc (const char *producer, int *major, int *minor)
{
  const char *cs;

  if (producer != NULL && startswith (producer, "GNU "))
    {
      int maj, min;

      if (major == NULL)
	major = &maj;
      if (minor == NULL)
	minor = &min;

      /* Skip GNU.  */
      cs = &producer[strlen ("GNU ")];

      /* Bail out for GNU AS.  */
      if (startswith (cs, "AS "))
	return 0;

      /* Skip any identifier after "GNU " - such as "C11" "C++" or "Java".
	 A full producer string might look like:
	 "GNU C 4.7.2"
	 "GNU Fortran 4.8.2 20140120 (Red Hat 4.8.2-16) -mtune=generic ..."
	 "GNU C++14 5.0.0 20150123 (experimental)"
      */
      while (*cs && !isspace (*cs))
	cs++;
      if (*cs && isspace (*cs))
	cs++;
      if (sscanf (cs, "%d.%d", major, minor) == 2)
	return 1;
    }

  /* Not recognized as GCC.  */
  return 0;
}

/* See producer.h.  */

bool
producer_is_gas (const char *producer, int *major, int *minor)
{
  if (producer == nullptr)
    {
      /* No producer, don't know.  */
      return false;
    }

  /* Detect prefix.  */
  const char prefix[] = "GNU AS ";
  if (!startswith (producer, prefix))
    {
      /* Producer is not gas.  */
      return false;
    }

  /* Skip prefix.  */
  const char *cs = &producer[strlen (prefix)];

  /* Ensure that major/minor are not nullptrs.  */
  int maj, min;
  if (major == nullptr)
    major = &maj;
  if (minor == nullptr)
    minor = &min;

  int scanned = sscanf (cs, "%d.%d", major, minor);
  if (scanned != 2)
    {
      /* Unable to scan major/minor version.  */
      return false;
    }

  return true;
}

  /* See producer.h.  */

bool
producer_is_icc_ge_19 (const char *producer)
{
  int major, minor;

  if (! producer_is_icc (producer, &major, &minor))
    return false;

  return major >= 19;
}

/* See producer.h.  */

bool
producer_is_icc (const char *producer, int *major, int *minor)
{
  compiled_regex i_re ("Intel(R)", 0, "producer_is_icc");
  if (producer == nullptr || i_re.exec (producer, 0, nullptr, 0) != 0)
    return false;

  /* Prepare the used fields.  */
  int maj, min;
  if (major == nullptr)
    major = &maj;
  if (minor == nullptr)
    minor = &min;

  *minor = 0;
  *major = 0;

  compiled_regex re ("[0-9]+\\.[0-9]+", REG_EXTENDED, "producer_is_icc");
  regmatch_t version[1];
  if (re.exec (producer, ARRAY_SIZE (version), version, 0) == 0
      && version[0].rm_so != -1)
    {
      const char *version_str = producer + version[0].rm_so;
      sscanf (version_str, "%d.%d", major, minor);
      return true;
    }

  return false;
}

/* See producer.h.  */

bool
producer_is_llvm (const char *producer)
{
  return ((producer != NULL) && (startswith (producer, "clang ")
				 || startswith (producer, " F90 Flang ")));
}

/* See producer.h.  */

bool
producer_is_clang (const char *producer, int *major, int *minor)
{
  if (producer != nullptr && startswith (producer, "clang version "))
    {
      int maj, min;
      if (major == nullptr)
	major = &maj;
      if (minor == nullptr)
	minor = &min;

      /* The full producer string will look something like
	 "clang version XX.X.X ..."
	 So we can safely ignore all characters before the first digit.  */
      const char *cs = producer + strlen ("clang version ");

      if (sscanf (cs, "%d.%d", major, minor) == 2)
	return true;
    }
  return false;
}

#if defined GDB_SELF_TEST
namespace selftests {
namespace producer {

static void
producer_parsing_tests ()
{
  {
    /* Check that we don't crash if "Version" is not found in what
       looks like an ICC producer string.  */
    static const char icc_no_version[] = "Intel(R) foo bar";

    int major = 0, minor = 0;
    SELF_CHECK (!producer_is_icc (icc_no_version, &major, &minor));
    SELF_CHECK (!producer_is_gcc (icc_no_version, &major, &minor));
  }

  {
    static const char extern_f_14_0[] = "\
Intel(R) Fortran Intel(R) 64 Compiler XE for applications running on \
Intel(R) 64, \
Version 14.0.1.074 Build 20130716";

    int major = 0, minor = 0;
    SELF_CHECK (producer_is_icc (extern_f_14_0, &major, &minor)
		&& major == 14 && minor == 0);
    SELF_CHECK (!producer_is_gcc (extern_f_14_0, &major, &minor));
  }

  {
    static const char intern_f_14[] = "\
Intel(R) Fortran Intel(R) 64 Compiler XE for applications running on \
Intel(R) 64, \
Version 14.0";

    int major = 0, minor = 0;
    SELF_CHECK (producer_is_icc (intern_f_14, &major, &minor)
		&& major == 14 && minor == 0);
    SELF_CHECK (!producer_is_gcc (intern_f_14, &major, &minor));
  }

  {
    static const char intern_c_14[] = "\
Intel(R) C++ Intel(R) 64 Compiler XE for applications running on \
Intel(R) 64, \
Version 14.0";
    int major = 0, minor = 0;
    SELF_CHECK (producer_is_icc (intern_c_14, &major, &minor)
		&& major == 14 && minor == 0);
    SELF_CHECK (!producer_is_gcc (intern_c_14, &major, &minor));
  }

  {
    static const char intern_c_18[] = "\
Intel(R) C++ Intel(R) 64 Compiler for applications running on \
Intel(R) 64, \
Version 18.0 Beta";
    int major = 0, minor = 0;
    SELF_CHECK (producer_is_icc (intern_c_18, &major, &minor)
		&& major == 18 && minor == 0);
  }

  {
    static const char gnu[] = "GNU C 4.7.2";
    SELF_CHECK (!producer_is_icc (gnu, NULL, NULL));

    int major = 0, minor = 0;
    SELF_CHECK (producer_is_gcc (gnu, &major, &minor)
		&& major == 4 && minor == 7);
  }

  {
    static const char gnu_exp[] = "GNU C++14 5.0.0 20150123 (experimental)";
    int major = 0, minor = 0;
    SELF_CHECK (!producer_is_icc (gnu_exp, NULL, NULL));
    SELF_CHECK (producer_is_gcc (gnu_exp, &major, &minor)
		&& major == 5 && minor == 0);
  }

  {
    static const char clang_llvm_exp[] = "clang version 12.0.0 (CLANG: bld#8)";
    int major = 0, minor = 0;
    SELF_CHECK (!producer_is_icc (clang_llvm_exp, NULL, NULL));
    SELF_CHECK (!producer_is_gcc (clang_llvm_exp, &major, &minor));
    SELF_CHECK (producer_is_llvm (clang_llvm_exp));
  }

  {
    static const char flang_llvm_exp[] = " F90 Flang - 1.5 2017-05-01";
    int major = 0, minor = 0;
    SELF_CHECK (!producer_is_icc (flang_llvm_exp, NULL, NULL));
    SELF_CHECK (!producer_is_gcc (flang_llvm_exp, &major, &minor));
    SELF_CHECK (producer_is_llvm (flang_llvm_exp));
  }

  {
    static const char gas_exp[] = "GNU AS 2.39.0";
    int major = 0, minor = 0;
    SELF_CHECK (!producer_is_gcc (gas_exp, &major, &minor));
    SELF_CHECK (producer_is_gas (gas_exp, &major, &minor));
    SELF_CHECK (major == 2 && minor == 39);

    static const char gas_incomplete_exp[] = "GNU AS ";
    SELF_CHECK (!producer_is_gas (gas_incomplete_exp, &major, &minor));
    SELF_CHECK (!producer_is_gcc (gas_incomplete_exp, &major, &minor));

    static const char gas_incomplete_exp_2[] = "GNU AS 2";
    SELF_CHECK (!producer_is_gas (gas_incomplete_exp_2, &major, &minor));
    SELF_CHECK (!producer_is_gcc (gas_incomplete_exp_2, &major, &minor));
  }

}
}
}
#endif

void _initialize_producer ();
void
_initialize_producer ()
{
#if defined GDB_SELF_TEST
  selftests::register_test
    ("producer-parser", selftests::producer::producer_parsing_tests);
#endif
}
