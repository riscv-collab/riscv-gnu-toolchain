/* Base/prototype target for default child (native) targets.

   Copyright (C) 2004-2024 Free Software Foundation, Inc.

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

#ifndef INF_CHILD_H
#define INF_CHILD_H

#include "target.h"
#include "process-stratum-target.h"

/* A prototype child target.  The client can override it with local
   methods.  */

class inf_child_target
  : public memory_breakpoint_target<process_stratum_target>
{
public:
  inf_child_target () = default;
  ~inf_child_target () override = 0;

  const target_info &info () const override;

  void close () override;

  void disconnect (const char *, int) override;

  void fetch_registers (struct regcache *, int) override = 0;
  void store_registers (struct regcache *, int) override = 0;

  void prepare_to_store (struct regcache *) override;

  bool supports_terminal_ours () override;
  void terminal_init () override;
  void terminal_inferior () override;
  void terminal_save_inferior () override;
  void terminal_ours_for_output () override;
  void terminal_ours () override;
  void terminal_info (const char *, int) override;

  void interrupt () override;
  void pass_ctrlc () override;

  void follow_exec (inferior *follow_inf, ptid_t ptid,
		    const char *execd_pathname) override;

  void mourn_inferior () override;

  bool can_run () override;

  bool can_create_inferior () override;
  void create_inferior (const char *, const std::string &,
			char **, int) override = 0;

  bool can_attach () override;
  void attach (const char *, int) override = 0;

  void post_attach (int) override;

  const char *pid_to_exec_file (int pid) override;

  int fileio_open (struct inferior *inf, const char *filename,
		   int flags, int mode, int warn_if_slow,
		   fileio_error *target_errno) override;
  int fileio_pwrite (int fd, const gdb_byte *write_buf, int len,
		     ULONGEST offset, fileio_error *target_errno) override;
  int fileio_pread (int fd, gdb_byte *read_buf, int len,
		    ULONGEST offset, fileio_error *target_errno) override;
  int fileio_fstat (int fd, struct stat *sb, fileio_error *target_errno) override;
  int fileio_close (int fd, fileio_error *target_errno) override;
  int fileio_unlink (struct inferior *inf,
		     const char *filename,
		     fileio_error *target_errno) override;
  std::optional<std::string> fileio_readlink (struct inferior *inf,
					      const char *filename,
					      fileio_error *target_errno) override;
  bool use_agent (bool use) override;

  bool can_use_agent () override;

protected:
  /* Unpush the target if it wasn't explicitly open with "target native"
     and there are no live inferiors left.  Note: if calling this as a
     result of a mourn or detach, the current inferior shall already
     have its PID cleared, so it isn't counted as live.  That's usually
     done by calling either generic_mourn_inferior or
     detach_inferior.  */
  void maybe_unpush_target ();
};

/* Convert the host wait(2) status to a target_waitstatus.  */

extern target_waitstatus host_status_to_waitstatus (int hoststatus);

/* Register TARGET as native target and set it up to respond to the
   "target native" command.  */
extern void add_inf_child_target (inf_child_target *target);

/* target_open_ftype callback for inf-child targets.  Used by targets
   that want to register an alternative target_info object.  Most
   targets use add_inf_child_target instead.  */
extern void inf_child_open_target (const char *arg, int from_tty);

#endif
