/* Native-dependent code for x86 (i386 and x86-64).

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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
#include "x86-nat.h"
#include "gdbcmd.h"
#include "inferior.h"

#include <unordered_map>

/* Support for hardware watchpoints and breakpoints using the x86
   debug registers.

   This provides several functions for inserting and removing
   hardware-assisted breakpoints and watchpoints, testing if one or
   more of the watchpoints triggered and at what address, checking
   whether a given region can be watched, etc.

   The functions below implement debug registers sharing by reference
   counts, and allow to watch regions up to 16 bytes long.  */

/* Low-level function vector.  */
struct x86_dr_low_type x86_dr_low;

/* Hash table storing per-process data.  We don't bind this to a
   per-inferior registry because of targets like x86 GNU/Linux that
   need to keep track of processes that aren't bound to any inferior
   (e.g., fork children, checkpoints).  */

static std::unordered_map<pid_t,
			  struct x86_debug_reg_state> x86_debug_process_state;

/* See x86-nat.h.  */

struct x86_debug_reg_state *
x86_lookup_debug_reg_state (pid_t pid)
{
  auto it = x86_debug_process_state.find (pid);
  if (it != x86_debug_process_state.end ())
    return &it->second;

  return nullptr;
}

/* Get debug registers state for process PID.  */

struct x86_debug_reg_state *
x86_debug_reg_state (pid_t pid)
{
  return &x86_debug_process_state[pid];
}

/* See declaration in x86-nat.h.  */

void
x86_forget_process (pid_t pid)
{
  x86_debug_process_state.erase (pid);
}

/* Clear the reference counts and forget everything we knew about the
   debug registers.  */

void
x86_cleanup_dregs (void)
{
  /* Starting from scratch has the same effect.  */
  x86_forget_process (inferior_ptid.pid ());
}

/* Insert a watchpoint to watch a memory region which starts at
   address ADDR and whose length is LEN bytes.  Watch memory accesses
   of the type TYPE.  Return 0 on success, -1 on failure.  */

int
x86_insert_watchpoint (CORE_ADDR addr, int len,
		       enum target_hw_bp_type type, struct expression *cond)
{
  struct x86_debug_reg_state *state
    = x86_debug_reg_state (inferior_ptid.pid ());

  return x86_dr_insert_watchpoint (state, type, addr, len);
}

/* Remove a watchpoint that watched the memory region which starts at
   address ADDR, whose length is LEN bytes, and for accesses of the
   type TYPE.  Return 0 on success, -1 on failure.  */
int
x86_remove_watchpoint (CORE_ADDR addr, int len,
		       enum target_hw_bp_type type, struct expression *cond)
{
  struct x86_debug_reg_state *state
    = x86_debug_reg_state (inferior_ptid.pid ());

  return x86_dr_remove_watchpoint (state, type, addr, len);
}

/* Return non-zero if we can watch a memory region that starts at
   address ADDR and whose length is LEN bytes.  */

int
x86_region_ok_for_hw_watchpoint (CORE_ADDR addr, int len)
{
  struct x86_debug_reg_state *state
    = x86_debug_reg_state (inferior_ptid.pid ());

  return x86_dr_region_ok_for_watchpoint (state, addr, len);
}

/* If the inferior has some break/watchpoint that triggered, set the
   address associated with that break/watchpoint and return non-zero.
   Otherwise, return zero.  */

int
x86_stopped_data_address (CORE_ADDR *addr_p)
{
  struct x86_debug_reg_state *state
    = x86_debug_reg_state (inferior_ptid.pid ());

  return x86_dr_stopped_data_address (state, addr_p);
}

/* Return non-zero if the inferior has some watchpoint that triggered.
   Otherwise return zero.  */

int
x86_stopped_by_watchpoint ()
{
  struct x86_debug_reg_state *state
    = x86_debug_reg_state (inferior_ptid.pid ());

  return x86_dr_stopped_by_watchpoint (state);
}

/* Insert a hardware-assisted breakpoint at BP_TGT->reqstd_address.
   Return 0 on success, EBUSY on failure.  */

int
x86_insert_hw_breakpoint (struct gdbarch *gdbarch, struct bp_target_info *bp_tgt)
{
  struct x86_debug_reg_state *state
    = x86_debug_reg_state (inferior_ptid.pid ());

  bp_tgt->placed_address = bp_tgt->reqstd_address;
  return x86_dr_insert_watchpoint (state, hw_execute,
				   bp_tgt->placed_address, 1) ? EBUSY : 0;
}

/* Remove a hardware-assisted breakpoint at BP_TGT->placed_address.
   Return 0 on success, -1 on failure.  */

int
x86_remove_hw_breakpoint (struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt)
{
  struct x86_debug_reg_state *state
    = x86_debug_reg_state (inferior_ptid.pid ());

  return x86_dr_remove_watchpoint (state, hw_execute,
				   bp_tgt->placed_address, 1);
}

/* Returns the number of hardware watchpoints of type TYPE that we can
   set.  Value is positive if we can set CNT watchpoints, zero if
   setting watchpoints of type TYPE is not supported, and negative if
   CNT is more than the maximum number of watchpoints of type TYPE
   that we can support.  TYPE is one of bp_hardware_watchpoint,
   bp_read_watchpoint, bp_write_watchpoint, or bp_hardware_breakpoint.
   CNT is the number of such watchpoints used so far (including this
   one).  OTHERTYPE is non-zero if other types of watchpoints are
   currently enabled.

   We always return 1 here because we don't have enough information
   about possible overlap of addresses that they want to watch.  As an
   extreme example, consider the case where all the watchpoints watch
   the same address and the same region length: then we can handle a
   virtually unlimited number of watchpoints, due to debug register
   sharing implemented via reference counts in x86-nat.c.  */

int
x86_can_use_hw_breakpoint (enum bptype type, int cnt, int othertype)
{
  return 1;
}

/* Return non-zero if the inferior has some breakpoint that triggered.
   Otherwise return zero.  */

int
x86_stopped_by_hw_breakpoint ()
{
  struct x86_debug_reg_state *state
    = x86_debug_reg_state (inferior_ptid.pid ());

  return x86_dr_stopped_by_hw_breakpoint (state);
}

static void
add_show_debug_regs_command (void)
{
  /* A maintenance command to enable printing the internal DRi mirror
     variables.  */
  add_setshow_boolean_cmd ("show-debug-regs", class_maintenance,
			   &show_debug_regs, _("\
Set whether to show variables that mirror the x86 debug registers."), _("\
Show whether to show variables that mirror the x86 debug registers."), _("\
Use \"on\" to enable, \"off\" to disable.\n\
If enabled, the debug registers values are shown when GDB inserts\n\
or removes a hardware breakpoint or watchpoint, and when the inferior\n\
triggers a breakpoint or watchpoint."),
			   NULL,
			   NULL,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);
}

/* See x86-nat.h.  */

void
x86_set_debug_register_length (int len)
{
  /* This function should be called only once for each native target.  */
  gdb_assert (x86_dr_low.debug_register_length == 0);
  gdb_assert (len == 4 || len == 8);
  x86_dr_low.debug_register_length = len;
  add_show_debug_regs_command ();
}
