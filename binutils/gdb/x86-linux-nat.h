/* Native-dependent code for GNU/Linux x86 (i386 and x86-64).

   Copyright (C) 1999-2024 Free Software Foundation, Inc.

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

#ifndef X86_LINUX_NAT_H
#define X86_LINUX_NAT_H 1

#include "gdb_proc_service.h"
#include "linux-nat.h"
#include "gdbsupport/x86-xstate.h"
#include "x86-nat.h"
#include "nat/x86-linux.h"

struct x86_linux_nat_target : public x86_nat_target<linux_nat_target>
{
  virtual ~x86_linux_nat_target () override = 0;

  /* Add the description reader.  */
  const struct target_desc *read_description () override;

  struct btrace_target_info *enable_btrace (thread_info *tp,
					    const struct btrace_config *conf) override;
  void disable_btrace (struct btrace_target_info *tinfo) override;
  void teardown_btrace (struct btrace_target_info *tinfo) override;
  enum btrace_error read_btrace (struct btrace_data *data,
				 struct btrace_target_info *btinfo,
				 enum btrace_read_type type) override;
  const struct btrace_config *btrace_conf (const struct btrace_target_info *) override;

  x86_xsave_layout fetch_x86_xsave_layout () override
  { return m_xsave_layout; }

  /* These two are rewired to low_ versions.  linux-nat.c queries
     stopped-by-watchpoint info as soon as an lwp stops (via the low_
     methods) and caches the result, to be returned via the normal
     non-low methods.  */
  bool stopped_by_watchpoint () override
  { return linux_nat_target::stopped_by_watchpoint (); }

  bool stopped_data_address (CORE_ADDR *addr_p) override
  { return linux_nat_target::stopped_data_address (addr_p); }

  bool low_stopped_by_watchpoint () override
  { return x86_nat_target::stopped_by_watchpoint (); }

  bool low_stopped_data_address (CORE_ADDR *addr_p) override
  { return x86_nat_target::stopped_data_address (addr_p); }

  void low_new_fork (struct lwp_info *parent, pid_t child_pid) override;

  void low_forget_process (pid_t pid) override
  { x86_forget_process (pid); }

  void low_prepare_to_resume (struct lwp_info *lwp) override
  { x86_linux_prepare_to_resume (lwp); }

  void low_new_thread (struct lwp_info *lwp) override
  { x86_linux_new_thread (lwp); }

  void low_delete_thread (struct arch_lwp_info *lwp) override
  { x86_linux_delete_thread (lwp); }

protected:
  /* Override the GNU/Linux inferior startup hook.  */
  void post_startup_inferior (ptid_t) override;

private:
  x86_xsave_layout m_xsave_layout;
};



/* Helper for ps_get_thread_area.  Sets BASE_ADDR to a pointer to
   the thread local storage (or its descriptor) and returns PS_OK
   on success.  Returns PS_ERR on failure.  */

extern ps_err_e x86_linux_get_thread_area (pid_t pid, void *addr,
					   unsigned int *base_addr);

#endif
