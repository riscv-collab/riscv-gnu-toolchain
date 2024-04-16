/* GDB self-test for each gdbarch.
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
#include <functional>

#if GDB_SELF_TEST
#include "gdbsupport/selftest.h"
#include "selftest-arch.h"
#include "arch-utils.h"

namespace selftests {

static bool skip_arch (const char *arch)
{
  if (strcmp ("powerpc:EC603e", arch) == 0
      || strcmp ("powerpc:e500mc", arch) == 0
      || strcmp ("powerpc:e500mc64", arch) == 0
      || strcmp ("powerpc:titan", arch) == 0
      || strcmp ("powerpc:vle", arch) == 0
      || strcmp ("powerpc:e5500", arch) == 0
      || strcmp ("powerpc:e6500", arch) == 0)
    {
      /* PR 19797 */
      return true;
    }

  return false;
}

/* Generate a selftest for each gdbarch known to GDB.  */

static std::vector<selftest>
foreach_arch_test_generator (const std::string &name,
			     self_test_foreach_arch_function *function)
{
  std::vector<selftest> tests;
  std::vector<const char *> arches = gdbarch_printable_names ();
  tests.reserve (arches.size ());
  for (const char *arch : arches)
    {
      if (skip_arch (arch))
	continue;

      struct gdbarch_info info;
      info.bfd_arch_info = bfd_scan_arch (arch);
      info.osabi = GDB_OSABI_NONE;

      auto test_fn
	= ([=] ()
	   {
	     struct gdbarch *gdbarch = gdbarch_find_by_info (info);
	     SELF_CHECK (gdbarch != NULL);
	     function (gdbarch);
	     reset ();
	   });

      std::string id;

      bool has_sep = strchr (arch, ':') != nullptr;
      if (has_sep)
	/* Avoid avr::avr:1.  */
	id = arch;
      else if (strncasecmp (info.bfd_arch_info->arch_name, arch,
			    strlen (info.bfd_arch_info->arch_name)) == 0)
	/* Avoid arm::arm.  */
	id = arch;
      else
	/* Use arc::A6 instead of A6.  This still leaves us with an unfortunate
	   redundant id like am33_2::am33-2, but that doesn't seem worth the
	   effort to avoid.  */
	id = string_printf ("%s::%s", info.bfd_arch_info->arch_name, arch);

      id = string_printf ("%s::%s", name.c_str (), id.c_str ());
      tests.emplace_back (id, test_fn);
    }
  return tests;
}

/* See selftest-arch.h.  */

void
register_test_foreach_arch (const std::string &name,
			    self_test_foreach_arch_function *function)
{
  add_lazy_generator ([=] ()
		      { return foreach_arch_test_generator (name, function); });
}

void
reset ()
{
  /* Clear GDB internal state.  */
  registers_changed ();
  reinit_frame_cache ();
}
} // namespace selftests
#endif /* GDB_SELF_TEST */
