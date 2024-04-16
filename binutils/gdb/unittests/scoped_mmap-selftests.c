/* Self tests for scoped_mmap for GDB, the GNU debugger.

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
#include "gdbsupport/scoped_mmap.h"
#include "config.h"

#if defined(HAVE_SYS_MMAN_H)

#include "gdbsupport/selftest.h"
#include "gdbsupport/gdb_unlinker.h"

#include <unistd.h>

namespace selftests {
namespace scoped_mmap {

/* Test that the file is unmapped.  */
static void
test_destroy ()
{
  void *mem;

  errno = 0;
  {
    ::scoped_mmap smmap (nullptr, sysconf (_SC_PAGESIZE), PROT_WRITE,
			 MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);

    mem = smmap.get ();
    SELF_CHECK (mem != nullptr);
  }

  SELF_CHECK (msync (mem, sysconf (_SC_PAGESIZE), 0) == -1 && errno == ENOMEM);
}

/* Test that the memory can be released.  */
static void
test_release ()
{
  void *mem;

  errno = 0;
  {
    ::scoped_mmap smmap (nullptr, sysconf (_SC_PAGESIZE), PROT_WRITE,
			 MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);

    mem = smmap.release ();
    SELF_CHECK (mem != nullptr);
  }

  SELF_CHECK (msync (mem, sysconf (_SC_PAGESIZE), 0) == 0 || errno != ENOMEM);

  munmap (mem, sysconf (_SC_PAGESIZE));
}

/* Run selftests.  */
static void
run_tests ()
{
  test_destroy ();
  test_release ();
}

} /* namespace scoped_mmap */

namespace mmap_file
{

/* Test the standard usage of mmap_file.  */
static void
test_normal ()
{
  char filename[] = "scoped_mmapped_file-selftest-XXXXXX";
  {
    scoped_fd fd = gdb_mkostemp_cloexec (filename);
    SELF_CHECK (fd.get () >= 0);

    SELF_CHECK (write (fd.get (), "Hello!", 7) == 7);
  }

  gdb::unlinker unlink_test_file (filename);

  {
    ::scoped_mmap m = ::mmap_file (filename);

    SELF_CHECK (m.get () != MAP_FAILED);
    SELF_CHECK (m.size () == 7);
    SELF_CHECK (0 == strcmp ((char *) m.get (), "Hello!"));
  }
}

/* Calling mmap_file with a non-existent file should throw an exception.  */
static void
test_invalid_filename ()
{
  bool threw = false;

  try {
      ::scoped_mmap m = ::mmap_file ("/this/file/should/not/exist");
  } catch (gdb_exception &e) {
      threw = true;
  }

  SELF_CHECK (threw);
}


/* Run selftests.  */
static void
run_tests ()
{
  test_normal ();
  test_invalid_filename ();
}

} /* namespace mmap_file */
} /* namespace selftests */

#endif /* !defined(HAVE_SYS_MMAN_H) */

void _initialize_scoped_mmap_selftests ();
void
_initialize_scoped_mmap_selftests ()
{
#if defined(HAVE_SYS_MMAN_H)
  selftests::register_test ("scoped_mmap",
			    selftests::scoped_mmap::run_tests);
  selftests::register_test ("mmap_file",
			    selftests::mmap_file::run_tests);
#endif
}
