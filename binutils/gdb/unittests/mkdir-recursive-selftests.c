/* Self tests for scoped_fd for GDB, the GNU debugger.

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

#include "gdbsupport/filestuff.h"
#include "gdbsupport/selftest.h"
#include "gdbsupport/byte-vector.h"
#include "gdbsupport/pathstuff.h"

namespace selftests {
namespace mkdir_recursive {

/* Try to create DIR using mkdir_recursive and make sure it exists.  */

static bool
create_dir_and_check (const char *dir)
{
  ::mkdir_recursive (dir);

  struct stat st;
  if (stat (dir, &st) != 0)
    perror_with_name (("stat"));

  return (st.st_mode & S_IFDIR) != 0;
}

/* Test mkdir_recursive.  */

static void
test ()
{
  std::string tmp = get_standard_temp_dir () + "/gdb-selftests";
  gdb::char_vector base = make_temp_filename (tmp);

  if (mkdtemp (base.data ()) == NULL)
    perror_with_name (("mkdtemp"));

  /* Try not to leave leftover directories.  */
  struct cleanup_dirs {
    cleanup_dirs (const char *base)
      : m_base (base)
    {}

    ~cleanup_dirs () {
      rmdir (string_printf ("%s/a/b/c/d/e", m_base).c_str ());
      rmdir (string_printf ("%s/a/b/c/d", m_base).c_str ());
      rmdir (string_printf ("%s/a/b/c", m_base).c_str ());
      rmdir (string_printf ("%s/a/b", m_base).c_str ());
      rmdir (string_printf ("%s/a", m_base).c_str ());
      rmdir (m_base);
    }

  private:
    const char *m_base;
  } cleanup_dirs (base.data ());

  std::string dir = string_printf ("%s/a/b", base.data ());
  SELF_CHECK (create_dir_and_check (dir.c_str ()));

  dir = string_printf ("%s/a/b/c//d/e/", base.data ());
  SELF_CHECK (create_dir_and_check (dir.c_str ()));
}

}
}

void _initialize_mkdir_recursive_selftests ();
void
_initialize_mkdir_recursive_selftests ()
{
  selftests::register_test ("mkdir_recursive",
			    selftests::mkdir_recursive::test);
}

