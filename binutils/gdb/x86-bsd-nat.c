/* Native-dependent code for X86 BSD's.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "gdbthread.h"

/* We include <signal.h> to make sure `struct fxsave64' is defined on
   NetBSD, since NetBSD's <machine/reg.h> needs it.  */
#include <signal.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "x86-nat.h"
#include "x86-bsd-nat.h"
#include "inf-ptrace.h"


/* Support for debug registers.  */

#ifdef HAVE_PT_GETDBREGS

static PTRACE_TYPE_RET
gdb_ptrace (PTRACE_TYPE_ARG1 request, ptid_t ptid, PTRACE_TYPE_ARG3 addr)
{
#ifdef __NetBSD__
  /* Support for NetBSD threads: unlike other ptrace implementations in this
     file, NetBSD requires that we pass both the pid and lwp.  */
  return ptrace (request, ptid.pid (), addr, ptid.lwp ());
#else
  pid_t pid = get_ptrace_pid (ptid);
  return ptrace (request, pid, addr, 0);
#endif
}

/* Helper macro to access debug register X.  FreeBSD/amd64 and modern
   versions of FreeBSD/i386 provide this macro in system headers.  Define
   a local version for systems that do not provide it.  */
#ifndef DBREG_DRX
#ifdef __NetBSD__
#define DBREG_DRX(d, x) ((d)->dr[x])
#else
#define DBREG_DRX(d, x) ((&d->dr0)[x])
#endif
#endif

static unsigned long
x86bsd_dr_get (ptid_t ptid, int regnum)
{
  struct dbreg dbregs;

  if (gdb_ptrace (PT_GETDBREGS, ptid, (PTRACE_TYPE_ARG3) &dbregs) == -1)
    perror_with_name (_("Couldn't read debug registers"));

  return DBREG_DRX ((&dbregs), regnum);
}

static void
x86bsd_dr_set (ptid_t ptid, int regnum, unsigned long value)
{
  struct dbreg dbregs;

  if (gdb_ptrace (PT_GETDBREGS, ptid, (PTRACE_TYPE_ARG3) &dbregs) == -1)
    perror_with_name (_("Couldn't get debug registers"));

  /* For some mysterious reason, some of the reserved bits in the
     debug control register get set.  Mask these off, otherwise the
     ptrace call below will fail.  */
  DBREG_DRX ((&dbregs), 7) &= ~(0xffffffff0000fc00);

  DBREG_DRX ((&dbregs), regnum) = value;

  for (thread_info *thread : current_inferior ()->non_exited_threads ())
    {
      if (gdb_ptrace (PT_SETDBREGS, thread->ptid,
		      (PTRACE_TYPE_ARG3) &dbregs) == -1)
	perror_with_name (_("Couldn't write debug registers"));
    }
}

static void
x86bsd_dr_set_control (unsigned long control)
{
  x86bsd_dr_set (inferior_ptid, 7, control);
}

static void
x86bsd_dr_set_addr (int regnum, CORE_ADDR addr)
{
  gdb_assert (regnum >= 0 && regnum <= 4);

  x86bsd_dr_set (inferior_ptid, regnum, addr);
}

static CORE_ADDR
x86bsd_dr_get_addr (int regnum)
{
  return x86bsd_dr_get (inferior_ptid, regnum);
}

static unsigned long
x86bsd_dr_get_status (void)
{
  return x86bsd_dr_get (inferior_ptid, 6);
}

static unsigned long
x86bsd_dr_get_control (void)
{
  return x86bsd_dr_get (inferior_ptid, 7);
}

#endif /* PT_GETDBREGS */

void _initialize_x86_bsd_nat ();
void
_initialize_x86_bsd_nat ()
{
#ifdef HAVE_PT_GETDBREGS
  x86_dr_low.set_control = x86bsd_dr_set_control;
  x86_dr_low.set_addr = x86bsd_dr_set_addr;
  x86_dr_low.get_addr = x86bsd_dr_get_addr;
  x86_dr_low.get_status = x86bsd_dr_get_status;
  x86_dr_low.get_control = x86bsd_dr_get_control;
  x86_set_debug_register_length (sizeof (void *));
#endif /* HAVE_PT_GETDBREGS */
}
