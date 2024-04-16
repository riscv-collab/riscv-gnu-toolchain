/* Native-dependent code for AArch64.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.

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

#ifndef AARCH64_NAT_H
#define AARCH64_NAT_H

#include "breakpoint.h"
#include "nat/aarch64-hw-point.h"
#include "target.h"

/* Hardware-assisted breakpoints and watchpoints.  */

/* Initialize platform-independent state for hardware-assisted
   breakpoints and watchpoints.  */

void aarch64_initialize_hw_point ();

/* Return the debug register state for process PID.  If no existing
   state is found for this process, return nullptr.  */

struct aarch64_debug_reg_state *aarch64_lookup_debug_reg_state (pid_t pid);

/* Return the debug register state for process PID.  If no existing
   state is found for this process, create new state.  */

struct aarch64_debug_reg_state *aarch64_get_debug_reg_state (pid_t pid);

/* Remove any existing per-process debug state for process PID.  */

void aarch64_remove_debug_reg_state (pid_t pid);

/* Helper for the "stopped_data_address" target method.  Returns TRUE
   if a hardware watchpoint trap at ADDR_TRAP matches a set
   watchpoint.  The address of the matched watchpoint is returned in
   *ADDR_P.  */

bool aarch64_stopped_data_address (const struct aarch64_debug_reg_state *state,
				   CORE_ADDR addr_trap, CORE_ADDR *addr_p);

/* Helper functions used by aarch64_nat_target below.  See their
   definitions.  */

int aarch64_can_use_hw_breakpoint (enum bptype type, int cnt, int othertype);
int aarch64_insert_watchpoint (CORE_ADDR addr, int len,
			       enum target_hw_bp_type type,
			       struct expression *cond);
int aarch64_remove_watchpoint (CORE_ADDR addr, int len,
			       enum target_hw_bp_type type,
			       struct expression *cond);
int aarch64_insert_hw_breakpoint (struct gdbarch *gdbarch,
				  struct bp_target_info *bp_tgt);
int aarch64_remove_hw_breakpoint (struct gdbarch *gdbarch,
				  struct bp_target_info *bp_tgt);
int aarch64_stopped_by_hw_breakpoint ();

/* Convenience template mixin used to add aarch64 watchpoints support to a
   target.  */

template <typename BaseTarget>
struct aarch64_nat_target : public BaseTarget
{
  /* Hook in common aarch64 hardware watchpoints/breakpoints support.  */

  int can_use_hw_breakpoint (enum bptype type, int cnt, int othertype) override
  { return aarch64_can_use_hw_breakpoint (type, cnt, othertype); }

  int region_ok_for_hw_watchpoint (CORE_ADDR addr, int len) override
  { return aarch64_region_ok_for_watchpoint (addr, len); }

  int insert_watchpoint (CORE_ADDR addr, int len,
			 enum target_hw_bp_type type,
			 struct expression *cond) override
  { return aarch64_insert_watchpoint (addr, len, type, cond); }

  int remove_watchpoint (CORE_ADDR addr, int len,
			 enum target_hw_bp_type type,
			 struct expression *cond) override
  { return aarch64_remove_watchpoint (addr, len, type, cond); }

  int insert_hw_breakpoint (struct gdbarch *gdbarch,
			    struct bp_target_info *bp_tgt) override
  { return aarch64_insert_hw_breakpoint (gdbarch, bp_tgt); }

  int remove_hw_breakpoint (struct gdbarch *gdbarch,
			    struct bp_target_info *bp_tgt) override
  { return aarch64_remove_hw_breakpoint (gdbarch, bp_tgt); }

  bool watchpoint_addr_within_range (CORE_ADDR addr, CORE_ADDR start,
				     int length) override
  { return start <= addr && start + length - 1 >= addr; }
};

#endif /* AARCH64_NAT_H */
