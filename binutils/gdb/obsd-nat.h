/* Native-dependent code for OpenBSD.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

#ifndef OBSD_NAT_H
#define OBSD_NAT_H

#include "inf-ptrace.h"

class obsd_nat_target : public inf_ptrace_target
{
  /* Override some methods to support threads.  */
  std::string pid_to_str (ptid_t) override;
  void update_thread_list () override;
  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;

  void follow_fork (inferior *inf, ptid_t, target_waitkind, bool, bool) override;

  int insert_fork_catchpoint (int) override;

  int remove_fork_catchpoint (int) override;

  void post_attach (int) override;

protected:
  void post_startup_inferior (ptid_t) override;
};

#endif /* obsd-nat.h */
