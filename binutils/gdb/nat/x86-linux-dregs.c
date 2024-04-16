/* Low-level debug register code for GNU/Linux x86 (i386 and x86-64).

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

#include "gdbsupport/common-defs.h"
#include "nat/gdb_ptrace.h"
#include <sys/user.h>
#include "target/waitstatus.h"
#include "nat/x86-linux.h"
#include "nat/x86-dregs.h"
#include "nat/x86-linux-dregs.h"

/* Return the offset of REGNUM in the u_debugreg field of struct
   user.  */

static int
u_debugreg_offset (int regnum)
{
  return (offsetof (struct user, u_debugreg)
	  + sizeof (((struct user *) 0)->u_debugreg[0]) * regnum);
}

/* Get debug register REGNUM value from the LWP specified by PTID.  */

static unsigned long
x86_linux_dr_get (ptid_t ptid, int regnum)
{
  int tid;
  unsigned long value;

  gdb_assert (ptid.lwp_p ());
  tid = ptid.lwp ();

  errno = 0;
  value = ptrace (PTRACE_PEEKUSER, tid, u_debugreg_offset (regnum), 0);
  if (errno != 0)
    perror_with_name (_("Couldn't read debug register"));

  return value;
}

/* Set debug register REGNUM to VALUE in the LWP specified by PTID.  */

static void
x86_linux_dr_set (ptid_t ptid, int regnum, unsigned long value)
{
  int tid;

  gdb_assert (ptid.lwp_p ());
  tid = ptid.lwp ();

  errno = 0;
  ptrace (PTRACE_POKEUSER, tid, u_debugreg_offset (regnum), value);
  if (errno != 0)
    perror_with_name (_("Couldn't write debug register"));
}

/* Callback for iterate_over_lwps.  Mark that our local mirror of
   LWP's debug registers has been changed, and cause LWP to stop if
   it isn't already.  Values are written from our local mirror to
   the actual debug registers immediately prior to LWP resuming.  */

static int
update_debug_registers_callback (struct lwp_info *lwp)
{
  lwp_set_debug_registers_changed (lwp, 1);

  if (!lwp_is_stopped (lwp))
    linux_stop_lwp (lwp);

  /* Continue the iteration.  */
  return 0;
}

/* See nat/x86-linux-dregs.h.  */

CORE_ADDR
x86_linux_dr_get_addr (int regnum)
{
  gdb_assert (DR_FIRSTADDR <= regnum && regnum <= DR_LASTADDR);

  return x86_linux_dr_get (current_lwp_ptid (), regnum);
}

/* See nat/x86-linux-dregs.h.  */

void
x86_linux_dr_set_addr (int regnum, CORE_ADDR addr)
{
  ptid_t pid_ptid = ptid_t (current_lwp_ptid ().pid ());

  gdb_assert (DR_FIRSTADDR <= regnum && regnum <= DR_LASTADDR);

  iterate_over_lwps (pid_ptid, update_debug_registers_callback);
}

/* See nat/x86-linux-dregs.h.  */

unsigned long
x86_linux_dr_get_control (void)
{
  return x86_linux_dr_get (current_lwp_ptid (), DR_CONTROL);
}

/* See nat/x86-linux-dregs.h.  */

void
x86_linux_dr_set_control (unsigned long control)
{
  ptid_t pid_ptid = ptid_t (current_lwp_ptid ().pid ());

  iterate_over_lwps (pid_ptid, update_debug_registers_callback);
}

/* See nat/x86-linux-dregs.h.  */

unsigned long
x86_linux_dr_get_status (void)
{
  return x86_linux_dr_get (current_lwp_ptid (), DR_STATUS);
}

/* See nat/x86-linux-dregs.h.  */

void
x86_linux_update_debug_registers (struct lwp_info *lwp)
{
  ptid_t ptid = ptid_of_lwp (lwp);
  int clear_status = 0;

  gdb_assert (lwp_is_stopped (lwp));

  if (lwp_debug_registers_changed (lwp))
    {
      struct x86_debug_reg_state *state
	= x86_debug_reg_state (ptid.pid ());
      int i;

      /* Prior to Linux kernel 2.6.33 commit
	 72f674d203cd230426437cdcf7dd6f681dad8b0d, setting DR0-3 to
	 a value that did not match what was enabled in DR_CONTROL
	 resulted in EINVAL.  To avoid this we zero DR_CONTROL before
	 writing address registers, only writing DR_CONTROL's actual
	 value once all the addresses are in place.  */
      x86_linux_dr_set (ptid, DR_CONTROL, 0);

      ALL_DEBUG_ADDRESS_REGISTERS (i)
	if (state->dr_ref_count[i] > 0)
	  {
	    x86_linux_dr_set (ptid, i, state->dr_mirror[i]);

	    /* If we're setting a watchpoint, any change the inferior
	       has made to its debug registers needs to be discarded
	       to avoid x86_stopped_data_address getting confused.  */
	    clear_status = 1;
	  }

      /* If DR_CONTROL is supposed to be zero then it's already set.  */
      if (state->dr_control_mirror != 0)
	x86_linux_dr_set (ptid, DR_CONTROL, state->dr_control_mirror);

      lwp_set_debug_registers_changed (lwp, 0);
    }

  if (clear_status
      || lwp_stop_reason (lwp) == TARGET_STOPPED_BY_WATCHPOINT)
    x86_linux_dr_set (ptid, DR_STATUS, 0);
}
