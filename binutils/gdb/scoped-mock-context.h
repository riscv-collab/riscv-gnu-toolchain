/* RAII type to create a temporary mock context.

   Copyright (C) 2020-2024 Free Software Foundation, Inc.

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

#ifndef SCOPED_MOCK_CONTEXT_H
#define SCOPED_MOCK_CONTEXT_H

#include "inferior.h"
#include "gdbthread.h"
#include "progspace.h"
#include "progspace-and-thread.h"

#if GDB_SELF_TEST
namespace selftests {

/* RAII type to create (and switch to) a temporary mock context.  An
   inferior with a thread, with a process_stratum target pushed.  */

template<typename Target>
struct scoped_mock_context
{
  /* Order here is important.  */

  Target mock_target;
  ptid_t mock_ptid {1, 1};
  program_space mock_pspace {new_address_space ()};
  inferior mock_inferior {mock_ptid.pid ()};
  thread_info mock_thread {&mock_inferior, mock_ptid};

  scoped_restore_current_pspace_and_thread restore_pspace_thread;

  explicit scoped_mock_context (gdbarch *gdbarch)
  {
    /* Add the mock inferior to the inferior list so that look ups by
       target+ptid can find it.  */
    inferior_list.push_back (mock_inferior);

    mock_inferior.thread_list.push_back (mock_thread);
    mock_inferior.ptid_thread_map[mock_ptid] = &mock_thread;
    mock_inferior.set_arch (gdbarch);
    mock_inferior.aspace = mock_pspace.aspace;
    mock_inferior.pspace = &mock_pspace;

    /* Switch to the mock inferior.  */
    switch_to_inferior_no_thread (&mock_inferior);

    /* Push the process_stratum target so we can mock accessing
       registers.  */
    gdb_assert (mock_target.stratum () == process_stratum);
    mock_inferior.push_target (&mock_target);

    /* Switch to the mock thread.  */
    switch_to_thread (&mock_thread);
  }

  ~scoped_mock_context ()
  {
    inferior_list.erase (inferior_list.iterator_to (mock_inferior));
    mock_inferior.pop_all_targets_at_and_above (process_stratum);
  }
};

} // namespace selftests
#endif /* GDB_SELF_TEST */

#endif /* !defined (SCOPED_MOCK_CONTEXT_H) */
