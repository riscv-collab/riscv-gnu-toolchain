/* Common code for targets with the none ABI (bare-metal), but where the
   BFD library is build with ELF support.

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

#include "defs.h"
#include "elf-none-tdep.h"
#include "regset.h"
#include "elf-bfd.h"
#include "inferior.h"
#include "regcache.h"
#include "gdbarch.h"
#include "gcore.h"
#include "gcore-elf.h"

/* Build the note section for a corefile, and return it in a malloc
   buffer.  Currently this just dumps all available registers for each
   thread.  */

static gdb::unique_xmalloc_ptr<char>
elf_none_make_corefile_notes (struct gdbarch *gdbarch, bfd *obfd,
			      int *note_size)
{
  gdb::unique_xmalloc_ptr<char> note_data;

  /* Add note information about the executable and its arguments.  */
  std::string fname;
  std::string psargs;
  static const size_t fname_len = 16;
  static const size_t psargs_len = 80;
  if (get_exec_file (0))
    {
      const char *exe = get_exec_file (0);
      fname = lbasename (exe);
      psargs = std::string (exe);

      const std::string &infargs = current_inferior ()->args ();
      if (!infargs.empty ())
	psargs += ' ' + infargs;

      /* All existing targets that handle writing out prpsinfo expect the
	 fname and psargs strings to be at least 16 and 80 characters long
	 respectively, including a null terminator at the end.  Resize to
	 the expected length minus one to ensure there is a null within the
	 required length.  */
      fname.resize (fname_len - 1);
      psargs.resize (psargs_len - 1);
    }

  /* Resize the buffers up to their required lengths.  This will fill any
     remaining space with the null character.  */
  fname.resize (fname_len);
  psargs.resize (psargs_len);

  /* Now write out the prpsinfo structure.  */
  note_data.reset (elfcore_write_prpsinfo (obfd, note_data.release (),
					   note_size, fname.c_str (),
					   psargs.c_str ()));
  if (note_data == nullptr)
    return nullptr;

  /* Thread register information.  */
  try
    {
      update_thread_list ();
    }
  catch (const gdb_exception_error &e)
    {
      exception_print (gdb_stderr, e);
    }

  /* Like the Linux kernel, prefer dumping the signalled thread first.
     "First thread" is what tools use to infer the signalled thread.  */
  thread_info *signalled_thr = gcore_find_signalled_thread ();

  /* All threads are reported as having been stopped by the same signal
     that stopped SIGNALLED_THR.  */
  gdb_signal stop_signal;
  if (signalled_thr != nullptr)
    stop_signal = signalled_thr->stop_signal ();
  else
    stop_signal = GDB_SIGNAL_0;

  if (signalled_thr != nullptr)
    gcore_elf_build_thread_register_notes (gdbarch, signalled_thr,
					   stop_signal, obfd, &note_data,
					   note_size);
  for (thread_info *thr : current_inferior ()->non_exited_threads ())
    {
      if (thr == signalled_thr)
	continue;

      gcore_elf_build_thread_register_notes (gdbarch, thr, stop_signal, obfd,
					     &note_data, note_size);
    }


  /* Include the target description when possible.  Some architectures
     allow for per-thread gdbarch so we should really be emitting a tdesc
     per-thread, however, we don't currently support reading in a
     per-thread tdesc, so just emit the tdesc for the signalled thread.  */
  gdbarch = target_thread_architecture (signalled_thr->ptid);
  gcore_elf_make_tdesc_note (gdbarch, obfd, &note_data, note_size);

  return note_data;
}

/* See none-tdep.h.  */

void
elf_none_init_abi (struct gdbarch *gdbarch)
{
  /* Default core file support.  */
  set_gdbarch_make_corefile_notes (gdbarch, elf_none_make_corefile_notes);
}
