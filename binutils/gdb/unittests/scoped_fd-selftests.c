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
#include "gdbsupport/scoped_fd.h"
#include "config.h"
#include "gdbsupport/selftest.h"

namespace selftests {
namespace scoped_fd {

/* Test that the file descriptor is closed.  */
static void
test_destroy ()
{
  char filename[] = "scoped_fd-selftest-XXXXXX";
  int fd = gdb_mkostemp_cloexec (filename).release ();
  SELF_CHECK (fd >= 0);

  unlink (filename);
  errno = 0;
  {
    ::scoped_fd sfd (fd);

    SELF_CHECK (sfd.get () == fd);
  }

  SELF_CHECK (close (fd) == -1 && errno == EBADF);
}

/* Test that the file descriptor can be released.  */
static void
test_release ()
{
  char filename[] = "scoped_fd-selftest-XXXXXX";
  int fd = gdb_mkostemp_cloexec (filename).release ();
  SELF_CHECK (fd >= 0);

  unlink (filename);
  errno = 0;
  {
    ::scoped_fd sfd (fd);

    SELF_CHECK (sfd.release () == fd);
  }

  SELF_CHECK (close (fd) == 0 || errno != EBADF);
}

/* Test that the file descriptor can be converted to a FILE *.  */
static void
test_to_file ()
{
  char filename[] = "scoped_fd-selftest-XXXXXX";

  ::scoped_fd sfd = gdb_mkostemp_cloexec (filename);
  SELF_CHECK (sfd.get () >= 0);

  unlink (filename);
  
  gdb_file_up file = sfd.to_file ("rw");
  SELF_CHECK (file != nullptr);
  SELF_CHECK (sfd.get () == -1);
}

/* Run selftests.  */
static void
run_tests ()
{
  test_destroy ();
  test_release ();
  test_to_file ();
}

} /* namespace scoped_fd */
} /* namespace selftests */

void _initialize_scoped_fd_selftests ();
void
_initialize_scoped_fd_selftests ()
{
  selftests::register_test ("scoped_fd",
			    selftests::scoped_fd::run_tests);
}
