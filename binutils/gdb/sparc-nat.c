/* Native-dependent code for SPARC.

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
#include "regcache.h"
#include "target.h"

#include <signal.h>
#include <sys/ptrace.h>
#include "gdbsupport/gdb_wait.h"
#ifdef HAVE_MACHINE_REG_H
#include <machine/reg.h>
#endif

#include "sparc-tdep.h"
#include "sparc-nat.h"
#include "inf-ptrace.h"

/* With some trickery we can use the code in this file for most (if
   not all) ptrace(2) based SPARC systems, which includes SunOS 4,
   GNU/Linux and the various SPARC BSD's.

   First, we need a data structure for use with ptrace(2).  SunOS has
   `struct regs' and `struct fp_status' in <machine/reg.h>.  BSD's
   have `struct reg' and `struct fpreg' in <machine/reg.h>.  GNU/Linux
   has the same structures as SunOS 4, but they're in <asm/reg.h>,
   which is a kernel header.  As a general rule we avoid including
   GNU/Linux kernel headers.  Fortunately GNU/Linux has a `gregset_t'
   and a `fpregset_t' that are equivalent to `struct regs' and `struct
   fp_status' in <sys/ucontext.h>, which is automatically included by
   <signal.h>.  Settling on using the `gregset_t' and `fpregset_t'
   typedefs, providing them for the other systems, therefore solves
   the puzzle.  */

#ifdef HAVE_MACHINE_REG_H
#ifdef HAVE_STRUCT_REG
typedef struct reg gregset_t;
typedef struct fpreg fpregset_t;
#else 
typedef struct regs gregset_t;
typedef struct fp_status fpregset_t;
#endif
#endif

/* Second, we need to remap the BSD ptrace(2) requests to their SunOS
   equivalents.  GNU/Linux already follows SunOS here.  */

#ifndef PTRACE_GETREGS
#define PTRACE_GETREGS PT_GETREGS
#endif

#ifndef PTRACE_SETREGS
#define PTRACE_SETREGS PT_SETREGS
#endif

#ifndef PTRACE_GETFPREGS
#define PTRACE_GETFPREGS PT_GETFPREGS
#endif

#ifndef PTRACE_SETFPREGS
#define PTRACE_SETFPREGS PT_SETFPREGS
#endif

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

/* Register set description.  */
const struct sparc_gregmap *sparc_gregmap;
const struct sparc_fpregmap *sparc_fpregmap;
void (*sparc_supply_gregset) (const struct sparc_gregmap *,
			      struct regcache *, int , const void *);
void (*sparc_collect_gregset) (const struct sparc_gregmap *,
			       const struct regcache *, int, void *);
void (*sparc_supply_fpregset) (const struct sparc_fpregmap *,
			       struct regcache *, int , const void *);
void (*sparc_collect_fpregset) (const struct sparc_fpregmap *,
				const struct regcache *, int , void *);
int (*sparc_gregset_supplies_p) (struct gdbarch *, int);
int (*sparc_fpregset_supplies_p) (struct gdbarch *, int);

/* Determine whether `gregset_t' contains register REGNUM.  */

int
sparc32_gregset_supplies_p (struct gdbarch *gdbarch, int regnum)
{
  /* Integer registers.  */
  if ((regnum >= SPARC_G1_REGNUM && regnum <= SPARC_G7_REGNUM)
      || (regnum >= SPARC_O0_REGNUM && regnum <= SPARC_O7_REGNUM)
      || (regnum >= SPARC_L0_REGNUM && regnum <= SPARC_L7_REGNUM)
      || (regnum >= SPARC_I0_REGNUM && regnum <= SPARC_I7_REGNUM))
    return 1;

  /* Control registers.  */
  if (regnum == SPARC32_PC_REGNUM
      || regnum == SPARC32_NPC_REGNUM
      || regnum == SPARC32_PSR_REGNUM
      || regnum == SPARC32_Y_REGNUM)
    return 1;

  return 0;
}

/* Determine whether `fpregset_t' contains register REGNUM.  */

int
sparc32_fpregset_supplies_p (struct gdbarch *gdbarch, int regnum)
{
  /* Floating-point registers.  */
  if (regnum >= SPARC_F0_REGNUM && regnum <= SPARC_F31_REGNUM)
    return 1;

  /* Control registers.  */
  if (regnum == SPARC32_FSR_REGNUM)
    return 1;

  return 0;
}

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers (including the floating-point registers).  */

void
sparc_fetch_inferior_registers (process_stratum_target *proc_target,
				regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  ptid_t ptid = regcache->ptid ();

  if (regnum == SPARC_G0_REGNUM)
    {
      gdb_byte zero[8] = { 0 };

      regcache->raw_supply (SPARC_G0_REGNUM, &zero);
      return;
    }

  if (regnum == -1 || sparc_gregset_supplies_p (gdbarch, regnum))
    {
      gregset_t regs;

      if (gdb_ptrace (PTRACE_GETREGS, ptid, (PTRACE_TYPE_ARG3) &regs) == -1)
	perror_with_name (_("Couldn't get registers"));

      /* Deep down, sparc_supply_rwindow reads memory, so needs the global
	 thread context to be set.  */
      scoped_restore restore_inferior_ptid
	= make_scoped_restore (&inferior_ptid, ptid);

      sparc_supply_gregset (sparc_gregmap, regcache, -1, &regs);
      if (regnum != -1)
	return;
    }

  if (regnum == -1 || sparc_fpregset_supplies_p (gdbarch, regnum))
    {
      fpregset_t fpregs;

      if (gdb_ptrace (PTRACE_GETFPREGS, ptid, (PTRACE_TYPE_ARG3) &fpregs) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      sparc_supply_fpregset (sparc_fpregmap, regcache, -1, &fpregs);
    }
}

void
sparc_store_inferior_registers (process_stratum_target *proc_target,
				regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  ptid_t ptid = regcache->ptid ();

  if (regnum == -1 || sparc_gregset_supplies_p (gdbarch, regnum))
    {
      gregset_t regs;

      if (gdb_ptrace (PTRACE_GETREGS, ptid, (PTRACE_TYPE_ARG3) &regs) == -1)
	perror_with_name (_("Couldn't get registers"));

      sparc_collect_gregset (sparc_gregmap, regcache, regnum, &regs);

      if (gdb_ptrace (PTRACE_SETREGS, ptid, (PTRACE_TYPE_ARG3) &regs) == -1)
	perror_with_name (_("Couldn't write registers"));

      /* Deal with the stack regs.  */
      if (regnum == -1 || regnum == SPARC_SP_REGNUM
	  || (regnum >= SPARC_L0_REGNUM && regnum <= SPARC_I7_REGNUM))
	{
	  ULONGEST sp;

	  regcache_cooked_read_unsigned (regcache, SPARC_SP_REGNUM, &sp);

	  /* Deep down, sparc_collect_rwindow writes memory, so needs the global
	     thread context to be set.  */
	  scoped_restore restore_inferior_ptid
	    = make_scoped_restore (&inferior_ptid, ptid);

	  sparc_collect_rwindow (regcache, sp, regnum);
	}

      if (regnum != -1)
	return;
    }

  if (regnum == -1 || sparc_fpregset_supplies_p (gdbarch, regnum))
    {
      fpregset_t fpregs, saved_fpregs;

      if (gdb_ptrace (PTRACE_GETFPREGS, ptid, (PTRACE_TYPE_ARG3) &fpregs) == -1)
	perror_with_name (_("Couldn't get floating-point registers"));

      memcpy (&saved_fpregs, &fpregs, sizeof (fpregs));
      sparc_collect_fpregset (sparc_fpregmap, regcache, regnum, &fpregs);

      /* Writing the floating-point registers will fail on NetBSD with
	 EINVAL if the inferior process doesn't have an FPU state
	 (i.e. if it didn't use the FPU yet).  Therefore we don't try
	 to write the registers if nothing changed.  */
      if (memcmp (&saved_fpregs, &fpregs, sizeof (fpregs)) != 0)
	{
	  if (gdb_ptrace (PTRACE_SETFPREGS, ptid,
			  (PTRACE_TYPE_ARG3) &fpregs) == -1)
	    perror_with_name (_("Couldn't write floating-point registers"));
	}

      if (regnum != -1)
	return;
    }
}


/* Implement the to_xfer_partial target_ops method for
   TARGET_OBJECT_WCOOKIE.  Fetch StackGhost Per-Process XOR cookie.  */

enum target_xfer_status
sparc_xfer_wcookie (enum target_object object,
		    const char *annex, gdb_byte *readbuf,
		    const gdb_byte *writebuf, ULONGEST offset, ULONGEST len,
		    ULONGEST *xfered_len)
{
  unsigned long wcookie = 0;
  char *buf = (char *)&wcookie;

  gdb_assert (object == TARGET_OBJECT_WCOOKIE);
  gdb_assert (readbuf && writebuf == NULL);

  if (offset == sizeof (unsigned long))
    return TARGET_XFER_EOF;			/* Signal EOF.  */
  if (offset > sizeof (unsigned long))
    return TARGET_XFER_E_IO;

#ifdef PT_WCOOKIE
  /* If PT_WCOOKIE is defined (by <sys/ptrace.h>), assume we're
     running on an OpenBSD release that uses StackGhost (3.1 or
     later).  Since release 3.6, OpenBSD uses a fully randomized
     cookie.  */
  {
    int pid = inferior_ptid.pid ();

    /* Sanity check.  The proper type for a cookie is register_t, but
       we can't assume that this type exists on all systems supported
       by the code in this file.  */
    gdb_assert (sizeof (wcookie) == sizeof (register_t));

    /* Fetch the cookie.  */
    if (ptrace (PT_WCOOKIE, pid, (PTRACE_TYPE_ARG3) &wcookie, 0) == -1)
      {
	if (errno != EINVAL)
	  perror_with_name (_("Couldn't get StackGhost cookie"));

	/* Although PT_WCOOKIE is defined on OpenBSD 3.1 and later,
	   the request wasn't implemented until after OpenBSD 3.4.  If
	   the kernel doesn't support the PT_WCOOKIE request, assume
	   we're running on a kernel that uses non-randomized cookies.  */
	wcookie = 0x3;
      }
  }
#endif /* PT_WCOOKIE */

  if (len > sizeof (unsigned long) - offset)
    len = sizeof (unsigned long) - offset;

  memcpy (readbuf, buf + offset, len);
  *xfered_len = (ULONGEST) len;
  return TARGET_XFER_OK;
}


void _initialize_sparc_nat ();
void
_initialize_sparc_nat ()
{
  /* Default to using SunOS 4 register sets.  */
  if (sparc_gregmap == NULL)
    sparc_gregmap = &sparc32_sunos4_gregmap;
  if (sparc_fpregmap == NULL)
    sparc_fpregmap = &sparc32_sunos4_fpregmap;
  if (sparc_supply_gregset == NULL)
    sparc_supply_gregset = sparc32_supply_gregset;
  if (sparc_collect_gregset == NULL)
    sparc_collect_gregset = sparc32_collect_gregset;
  if (sparc_supply_fpregset == NULL)
    sparc_supply_fpregset = sparc32_supply_fpregset;
  if (sparc_collect_fpregset == NULL)
    sparc_collect_fpregset = sparc32_collect_fpregset;
  if (sparc_gregset_supplies_p == NULL)
    sparc_gregset_supplies_p = sparc32_gregset_supplies_p;
  if (sparc_fpregset_supplies_p == NULL)
    sparc_fpregset_supplies_p = sparc32_fpregset_supplies_p;
}
