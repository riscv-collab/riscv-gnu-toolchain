/* Native-dependent code for NetBSD.

   Copyright (C) 2006-2024 Free Software Foundation, Inc.

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

#ifndef NBSD_NAT_H
#define NBSD_NAT_H

#include "inf-ptrace.h"

struct thread_info;

/* A prototype NetBSD target.  */

struct nbsd_nat_target : public inf_ptrace_target
{
  const char *pid_to_exec_file (int pid) override;

  bool thread_alive (ptid_t ptid) override;
  const char *thread_name (struct thread_info *thr) override;
  void post_attach (int pid) override;
  void update_thread_list () override;
  std::string pid_to_str (ptid_t ptid) override;

  int find_memory_regions (find_memory_region_ftype func, void *data) override;
  bool info_proc (const char *, enum info_proc_what) override;

  void resume (ptid_t, int, enum gdb_signal) override;
  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;
  int insert_exec_catchpoint (int pid) override;
  int remove_exec_catchpoint (int pid) override;
  int set_syscall_catchpoint (int pid, bool needed, int any_count,
			      gdb::array_view<const int> syscall_counts)
    override;

  bool supports_multi_process () override;
  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;
  bool supports_dumpcore () override;
  void dumpcore (const char *filename) override;

protected:
  void post_startup_inferior (ptid_t ptid) override;
};

#endif /* netbsd-nat.h */
