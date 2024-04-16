/* Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

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

#include "gdbsupport/common-defs.h"
#include "gdbsupport/break-common.h"
#include "gdbsupport/common-regcache.h"
#include "nat/linux-nat.h"
#include "aarch64-linux-hw-point.h"

#include <sys/uio.h>

/* The order in which <sys/ptrace.h> and <asm/ptrace.h> are included
   can be important.  <sys/ptrace.h> often declares various PTRACE_*
   enums.  <asm/ptrace.h> often defines preprocessor constants for
   these very same symbols.  When that's the case, build errors will
   result when <asm/ptrace.h> is included before <sys/ptrace.h>.  */
#include <sys/ptrace.h>
#include <asm/ptrace.h>

#include <elf.h>

/* See aarch64-linux-hw-point.h  */

bool kernel_supports_any_contiguous_range = true;

/* Helper for aarch64_notify_debug_reg_change.  Records the
   information about the change of one hardware breakpoint/watchpoint
   setting for the thread LWP.
   N.B.  The actual updating of hardware debug registers is not
   carried out until the moment the thread is resumed.  */

static int
debug_reg_change_callback (struct lwp_info *lwp, int is_watchpoint,
			   unsigned int idx)
{
  int tid = ptid_of_lwp (lwp).lwp ();
  struct arch_lwp_info *info = lwp_arch_private_info (lwp);
  dr_changed_t *dr_changed_ptr;
  dr_changed_t dr_changed;

  if (info == NULL)
    {
      info = XCNEW (struct arch_lwp_info);
      lwp_set_arch_private_info (lwp, info);
    }

  if (show_debug_regs)
    {
      debug_printf ("debug_reg_change_callback: \n\tOn entry:\n");
      debug_printf ("\ttid%d, dr_changed_bp=0x%s, "
		    "dr_changed_wp=0x%s\n", tid,
		    phex (info->dr_changed_bp, 8),
		    phex (info->dr_changed_wp, 8));
    }

  dr_changed_ptr = is_watchpoint ? &info->dr_changed_wp
    : &info->dr_changed_bp;
  dr_changed = *dr_changed_ptr;

  gdb_assert (idx >= 0
	      && (idx <= (is_watchpoint ? aarch64_num_wp_regs
			  : aarch64_num_bp_regs)));

  /* The actual update is done later just before resuming the lwp,
     we just mark that one register pair needs updating.  */
  DR_MARK_N_CHANGED (dr_changed, idx);
  *dr_changed_ptr = dr_changed;

  /* If the lwp isn't stopped, force it to momentarily pause, so
     we can update its debug registers.  */
  if (!lwp_is_stopped (lwp))
    linux_stop_lwp (lwp);

  if (show_debug_regs)
    {
      debug_printf ("\tOn exit:\n\ttid%d, dr_changed_bp=0x%s, "
		    "dr_changed_wp=0x%s\n", tid,
		    phex (info->dr_changed_bp, 8),
		    phex (info->dr_changed_wp, 8));
    }

  return 0;
}

/* Notify each thread that their IDXth breakpoint/watchpoint register
   pair needs to be updated.  The message will be recorded in each
   thread's arch-specific data area, the actual updating will be done
   when the thread is resumed.  */

void
aarch64_notify_debug_reg_change (ptid_t ptid,
				 int is_watchpoint, unsigned int idx)
{
  ptid_t pid_ptid = ptid_t (ptid.pid ());

  iterate_over_lwps (pid_ptid, [=] (struct lwp_info *info)
			       {
				 return debug_reg_change_callback (info,
								   is_watchpoint,
								   idx);
			       });
}

/* Reconfigure STATE to be compatible with Linux kernels with the PR
   external/20207 bug.  This is called when
   KERNEL_SUPPORTS_ANY_CONTIGUOUS_RANGE transitions to false.  Note we
   don't try to support combining watchpoints with matching (and thus
   shared) masks, as it's too late when we get here.  On buggy
   kernels, GDB will try to first setup the perfect matching ranges,
   which will run out of registers before this function can merge
   them.  It doesn't look like worth the effort to improve that, given
   eventually buggy kernels will be phased out.  */

static void
aarch64_downgrade_regs (struct aarch64_debug_reg_state *state)
{
  for (int i = 0; i < aarch64_num_wp_regs; ++i)
    if ((state->dr_ctrl_wp[i] & 1) != 0)
      {
	gdb_assert (state->dr_ref_count_wp[i] != 0);
	uint8_t mask_orig = (state->dr_ctrl_wp[i] >> 5) & 0xff;
	gdb_assert (mask_orig != 0);
	static const uint8_t old_valid[] = { 0x01, 0x03, 0x0f, 0xff };
	uint8_t mask = 0;
	for (const uint8_t old_mask : old_valid)
	  if (mask_orig <= old_mask)
	    {
	      mask = old_mask;
	      break;
	    }
	gdb_assert (mask != 0);

	/* No update needed for this watchpoint?  */
	if (mask == mask_orig)
	  continue;
	state->dr_ctrl_wp[i] |= mask << 5;
	state->dr_addr_wp[i]
	  = align_down (state->dr_addr_wp[i], AARCH64_HWP_ALIGNMENT);

	/* Try to match duplicate entries.  */
	for (int j = 0; j < i; ++j)
	  if ((state->dr_ctrl_wp[j] & 1) != 0
	      && state->dr_addr_wp[j] == state->dr_addr_wp[i]
	      && state->dr_addr_orig_wp[j] == state->dr_addr_orig_wp[i]
	      && state->dr_ctrl_wp[j] == state->dr_ctrl_wp[i])
	    {
	      state->dr_ref_count_wp[j] += state->dr_ref_count_wp[i];
	      state->dr_ref_count_wp[i] = 0;
	      state->dr_addr_wp[i] = 0;
	      state->dr_addr_orig_wp[i] = 0;
	      state->dr_ctrl_wp[i] &= ~1;
	      break;
	    }

	aarch64_notify_debug_reg_change (current_lwp_ptid (),
					 1 /* is_watchpoint */, i);
      }
}

/* Call ptrace to set the thread TID's hardware breakpoint/watchpoint
   registers with data from *STATE.  */

void
aarch64_linux_set_debug_regs (struct aarch64_debug_reg_state *state,
			      int tid, int watchpoint)
{
  int i, count;
  struct iovec iov;
  struct user_hwdebug_state regs;
  const CORE_ADDR *addr;
  const unsigned int *ctrl;

  memset (&regs, 0, sizeof (regs));
  iov.iov_base = &regs;
  count = watchpoint ? aarch64_num_wp_regs : aarch64_num_bp_regs;
  addr = watchpoint ? state->dr_addr_wp : state->dr_addr_bp;
  ctrl = watchpoint ? state->dr_ctrl_wp : state->dr_ctrl_bp;
  if (count == 0)
    return;
  iov.iov_len = (offsetof (struct user_hwdebug_state, dbg_regs)
		 + count * sizeof (regs.dbg_regs[0]));

  for (i = 0; i < count; i++)
    {
      regs.dbg_regs[i].addr = addr[i];
      regs.dbg_regs[i].ctrl = ctrl[i];
    }

  if (ptrace (PTRACE_SETREGSET, tid,
	      watchpoint ? NT_ARM_HW_WATCH : NT_ARM_HW_BREAK,
	      (void *) &iov))
    {
      /* Handle Linux kernels with the PR external/20207 bug.  */
      if (watchpoint && errno == EINVAL
	  && kernel_supports_any_contiguous_range)
	{
	  kernel_supports_any_contiguous_range = false;
	  aarch64_downgrade_regs (state);
	  aarch64_linux_set_debug_regs (state, tid, watchpoint);
	  return;
	}
      error (_("Unexpected error setting hardware debug registers"));
    }
}

/* Return true if debug arch level is compatible for hw watchpoints
   and breakpoints.  */

static bool
compatible_debug_arch (unsigned int debug_arch)
{
  if (debug_arch == AARCH64_DEBUG_ARCH_V8)
    return true;
  if (debug_arch == AARCH64_DEBUG_ARCH_V8_1)
    return true;
  if (debug_arch == AARCH64_DEBUG_ARCH_V8_2)
    return true;
  if (debug_arch == AARCH64_DEBUG_ARCH_V8_4)
    return true;
  if (debug_arch == AARCH64_DEBUG_ARCH_V8_8)
    return true;
  if (debug_arch == AARCH64_DEBUG_ARCH_V8_9)
    return true;

  return false;
}

/* Get the hardware debug register capacity information from the
   process represented by TID.  */

void
aarch64_linux_get_debug_reg_capacity (int tid)
{
  struct iovec iov;
  struct user_hwdebug_state dreg_state;

  iov.iov_base = &dreg_state;
  iov.iov_len = sizeof (dreg_state);

  /* Get hardware watchpoint register info.  */
  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_HW_WATCH, &iov) == 0
      && compatible_debug_arch (AARCH64_DEBUG_ARCH (dreg_state.dbg_info)))
    {
      aarch64_num_wp_regs = AARCH64_DEBUG_NUM_SLOTS (dreg_state.dbg_info);
      if (aarch64_num_wp_regs > AARCH64_HWP_MAX_NUM)
	{
	  warning (_("Unexpected number of hardware watchpoint registers"
		     " reported by ptrace, got %d, expected %d."),
		   aarch64_num_wp_regs, AARCH64_HWP_MAX_NUM);
	  aarch64_num_wp_regs = AARCH64_HWP_MAX_NUM;
	}
    }
  else
    {
      warning (_("Unable to determine the number of hardware watchpoints"
		 " available."));
      aarch64_num_wp_regs = 0;
    }

  /* Get hardware breakpoint register info.  */
  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_HW_BREAK, &iov) == 0
      && compatible_debug_arch (AARCH64_DEBUG_ARCH (dreg_state.dbg_info)))
    {
      aarch64_num_bp_regs = AARCH64_DEBUG_NUM_SLOTS (dreg_state.dbg_info);
      if (aarch64_num_bp_regs > AARCH64_HBP_MAX_NUM)
	{
	  warning (_("Unexpected number of hardware breakpoint registers"
		     " reported by ptrace, got %d, expected %d."),
		   aarch64_num_bp_regs, AARCH64_HBP_MAX_NUM);
	  aarch64_num_bp_regs = AARCH64_HBP_MAX_NUM;
	}
    }
  else
    {
      warning (_("Unable to determine the number of hardware breakpoints"
		 " available."));
      aarch64_num_bp_regs = 0;
    }
}
