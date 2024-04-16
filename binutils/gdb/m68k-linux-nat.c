/* Motorola m68k native support for GNU/Linux.

   Copyright (C) 1996-2024 Free Software Foundation, Inc.

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
#include "frame.h"
#include "inferior.h"
#include "language.h"
#include "gdbcore.h"
#include "regcache.h"
#include "target.h"
#include "linux-nat.h"
#include "gdbarch.h"

#include "m68k-tdep.h"

#include <sys/dir.h>
#include <signal.h>
#include "nat/gdb_ptrace.h"
#include <sys/user.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/procfs.h>

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#include <sys/file.h>
#include <sys/stat.h>

#include "floatformat.h"

/* Prototypes for supply_gregset etc.  */
#include "gregset.h"

/* Defines ps_err_e, struct ps_prochandle.  */
#include "gdb_proc_service.h"

#include "inf-ptrace.h"

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 25
#endif


class m68k_linux_nat_target final : public linux_nat_target
{
public:
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static m68k_linux_nat_target the_m68k_linux_nat_target;

/* This table must line up with gdbarch_register_name in "m68k-tdep.c".  */
static const int regmap[] =
{
  PT_D0, PT_D1, PT_D2, PT_D3, PT_D4, PT_D5, PT_D6, PT_D7,
  PT_A0, PT_A1, PT_A2, PT_A3, PT_A4, PT_A5, PT_A6, PT_USP,
  PT_SR, PT_PC,
  /* PT_FP0, ..., PT_FP7 */
  21, 24, 27, 30, 33, 36, 39, 42,
  /* PT_FPCR, PT_FPSR, PT_FPIAR */
  45, 46, 47
};

/* Which ptrace request retrieves which registers?
   These apply to the corresponding SET requests as well.  */
#define NUM_GREGS (18)
#define MAX_NUM_REGS (NUM_GREGS + 11)

static int
getregs_supplies (int regno)
{
  return 0 <= regno && regno < NUM_GREGS;
}

static int
getfpregs_supplies (int regno)
{
  return M68K_FP0_REGNUM <= regno && regno <= M68K_FPI_REGNUM;
}

/* Does the current host support the GETREGS request?  */
static int have_ptrace_getregs =
#ifdef HAVE_PTRACE_GETREGS
  1
#else
  0
#endif
;



/* Fetching registers directly from the U area, one at a time.  */

/* Fetch one register.  */

static void
fetch_register (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  long regaddr, val;
  int i;
  gdb_byte buf[M68K_MAX_REGISTER_SIZE];
  pid_t tid = get_ptrace_pid (regcache->ptid ());

  regaddr = 4 * regmap[regno];
  for (i = 0; i < register_size (gdbarch, regno); i += sizeof (long))
    {
      errno = 0;
      val = ptrace (PTRACE_PEEKUSER, tid, regaddr, 0);
      memcpy (&buf[i], &val, sizeof (long));
      regaddr += sizeof (long);
      if (errno != 0)
	error (_("Couldn't read register %s (#%d): %s."), 
	       gdbarch_register_name (gdbarch, regno),
	       regno, safe_strerror (errno));
    }
  regcache->raw_supply (regno, buf);
}

/* Fetch register values from the inferior.
   If REGNO is negative, do this for all registers.
   Otherwise, REGNO specifies which register (so we can save time).  */

static void
old_fetch_inferior_registers (struct regcache *regcache, int regno)
{
  if (regno >= 0)
    {
      fetch_register (regcache, regno);
    }
  else
    {
      for (regno = 0;
	   regno < gdbarch_num_regs (regcache->arch ());
	   regno++)
	{
	  fetch_register (regcache, regno);
	}
    }
}

/* Store one register.  */

static void
store_register (const struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  long regaddr, val;
  int i;
  gdb_byte buf[M68K_MAX_REGISTER_SIZE];
  pid_t tid = get_ptrace_pid (regcache->ptid ());

  regaddr = 4 * regmap[regno];

  /* Put the contents of regno into a local buffer.  */
  regcache->raw_collect (regno, buf);

  /* Store the local buffer into the inferior a chunk at the time.  */
  for (i = 0; i < register_size (gdbarch, regno); i += sizeof (long))
    {
      errno = 0;
      memcpy (&val, &buf[i], sizeof (long));
      ptrace (PTRACE_POKEUSER, tid, regaddr, val);
      regaddr += sizeof (long);
      if (errno != 0)
	error (_("Couldn't write register %s (#%d): %s."),
	       gdbarch_register_name (gdbarch, regno),
	       regno, safe_strerror (errno));
    }
}

/* Store our register values back into the inferior.
   If REGNO is negative, do this for all registers.
   Otherwise, REGNO specifies which register (so we can save time).  */

static void
old_store_inferior_registers (const struct regcache *regcache, int regno)
{
  if (regno >= 0)
    {
      store_register (regcache, regno);
    }
  else
    {
      for (regno = 0;
	   regno < gdbarch_num_regs (regcache->arch ());
	   regno++)
	{
	  store_register (regcache, regno);
	}
    }
}

/*  Given a pointer to a general register set in /proc format
   (elf_gregset_t *), unpack the register contents and supply
   them as gdb's idea of the current register values.  */

void
supply_gregset (struct regcache *regcache, const elf_gregset_t *gregsetp)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const elf_greg_t *regp = (const elf_greg_t *) gregsetp;
  int regi;

  for (regi = M68K_D0_REGNUM;
       regi <= gdbarch_sp_regnum (gdbarch);
       regi++)
    regcache->raw_supply (regi, &regp[regmap[regi]]);
  regcache->raw_supply (gdbarch_ps_regnum (gdbarch), &regp[PT_SR]);
  regcache->raw_supply (gdbarch_pc_regnum (gdbarch), &regp[PT_PC]);
}

/* Fill register REGNO (if it is a general-purpose register) in
   *GREGSETPS with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */
void
fill_gregset (const struct regcache *regcache,
	      elf_gregset_t *gregsetp, int regno)
{
  elf_greg_t *regp = (elf_greg_t *) gregsetp;
  int i;

  for (i = 0; i < NUM_GREGS; i++)
    if (regno == -1 || regno == i)
      regcache->raw_collect (i, regp + regmap[i]);
}

#ifdef HAVE_PTRACE_GETREGS

/* Fetch all general-purpose registers from process/thread TID and
   store their values in GDB's register array.  */

static void
fetch_regs (struct regcache *regcache, int tid)
{
  elf_gregset_t regs;

  if (ptrace (PTRACE_GETREGS, tid, 0, (int) &regs) < 0)
    {
      if (errno == EIO)
	{
	  /* The kernel we're running on doesn't support the GETREGS
	     request.  Reset `have_ptrace_getregs'.  */
	  have_ptrace_getregs = 0;
	  return;
	}

      perror_with_name (_("Couldn't get registers"));
    }

  supply_gregset (regcache, (const elf_gregset_t *) &regs);
}

/* Store all valid general-purpose registers in GDB's register array
   into the process/thread specified by TID.  */

static void
store_regs (const struct regcache *regcache, int tid, int regno)
{
  elf_gregset_t regs;

  if (ptrace (PTRACE_GETREGS, tid, 0, (int) &regs) < 0)
    perror_with_name (_("Couldn't get registers"));

  fill_gregset (regcache, &regs, regno);

  if (ptrace (PTRACE_SETREGS, tid, 0, (int) &regs) < 0)
    perror_with_name (_("Couldn't write registers"));
}

#else

static void fetch_regs (struct regcache *regcache, int tid)
{
}

static void store_regs (const struct regcache *regcache, int tid, int regno)
{
}

#endif


/* Transfering floating-point registers between GDB, inferiors and cores.  */

/* What is the address of fpN within the floating-point register set F?  */
#define FPREG_ADDR(f, n) (&(f)->fpregs[(n) * 3])

/* Fill GDB's register array with the floating-point register values in
   *FPREGSETP.  */

void
supply_fpregset (struct regcache *regcache, const elf_fpregset_t *fpregsetp)
{
  struct gdbarch *gdbarch = regcache->arch ();
  int regi;

  for (regi = gdbarch_fp0_regnum (gdbarch);
       regi < gdbarch_fp0_regnum (gdbarch) + 8; regi++)
    regcache->raw_supply
      (regi, FPREG_ADDR (fpregsetp, regi - gdbarch_fp0_regnum (gdbarch)));
  regcache->raw_supply (M68K_FPC_REGNUM, &fpregsetp->fpcntl[0]);
  regcache->raw_supply (M68K_FPS_REGNUM, &fpregsetp->fpcntl[1]);
  regcache->raw_supply (M68K_FPI_REGNUM, &fpregsetp->fpcntl[2]);
}

/* Fill register REGNO (if it is a floating-point register) in
   *FPREGSETP with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */

void
fill_fpregset (const struct regcache *regcache,
	       elf_fpregset_t *fpregsetp, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  int i;

  /* Fill in the floating-point registers.  */
  for (i = gdbarch_fp0_regnum (gdbarch);
       i < gdbarch_fp0_regnum (gdbarch) + 8; i++)
    if (regno == -1 || regno == i)
      regcache->raw_collect
	(i, FPREG_ADDR (fpregsetp, i - gdbarch_fp0_regnum (gdbarch)));

  /* Fill in the floating-point control registers.  */
  for (i = M68K_FPC_REGNUM; i <= M68K_FPI_REGNUM; i++)
    if (regno == -1 || regno == i)
      regcache->raw_collect (i, &fpregsetp->fpcntl[i - M68K_FPC_REGNUM]);
}

#ifdef HAVE_PTRACE_GETREGS

/* Fetch all floating-point registers from process/thread TID and store
   thier values in GDB's register array.  */

static void
fetch_fpregs (struct regcache *regcache, int tid)
{
  elf_fpregset_t fpregs;

  if (ptrace (PTRACE_GETFPREGS, tid, 0, (int) &fpregs) < 0)
    perror_with_name (_("Couldn't get floating point status"));

  supply_fpregset (regcache, (const elf_fpregset_t *) &fpregs);
}

/* Store all valid floating-point registers in GDB's register array
   into the process/thread specified by TID.  */

static void
store_fpregs (const struct regcache *regcache, int tid, int regno)
{
  elf_fpregset_t fpregs;

  if (ptrace (PTRACE_GETFPREGS, tid, 0, (int) &fpregs) < 0)
    perror_with_name (_("Couldn't get floating point status"));

  fill_fpregset (regcache, &fpregs, regno);

  if (ptrace (PTRACE_SETFPREGS, tid, 0, (int) &fpregs) < 0)
    perror_with_name (_("Couldn't write floating point status"));
}

#else

static void fetch_fpregs (struct regcache *regcache, int tid)
{
}

static void store_fpregs (const struct regcache *regcache, int tid, int regno)
{
}

#endif

/* Transferring arbitrary registers between GDB and inferior.  */

/* Fetch register REGNO from the child process.  If REGNO is -1, do
   this for all registers (including the floating point and SSE
   registers).  */

void
m68k_linux_nat_target::fetch_registers (struct regcache *regcache, int regno)
{
  pid_t tid;

  /* Use the old method of peeking around in `struct user' if the
     GETREGS request isn't available.  */
  if (! have_ptrace_getregs)
    {
      old_fetch_inferior_registers (regcache, regno);
      return;
    }

  tid = get_ptrace_pid (regcache->ptid ());

  /* Use the PTRACE_GETFPXREGS request whenever possible, since it
     transfers more registers in one system call, and we'll cache the
     results.  But remember that fetch_fpxregs can fail, and return
     zero.  */
  if (regno == -1)
    {
      fetch_regs (regcache, tid);

      /* The call above might reset `have_ptrace_getregs'.  */
      if (! have_ptrace_getregs)
	{
	  old_fetch_inferior_registers (regcache, -1);
	  return;
	}

      fetch_fpregs (regcache, tid);
      return;
    }

  if (getregs_supplies (regno))
    {
      fetch_regs (regcache, tid);
      return;
    }

  if (getfpregs_supplies (regno))
    {
      fetch_fpregs (regcache, tid);
      return;
    }

  internal_error (_("Got request for bad register number %d."), regno);
}

/* Store register REGNO back into the child process.  If REGNO is -1,
   do this for all registers (including the floating point and SSE
   registers).  */
void
m68k_linux_nat_target::store_registers (struct regcache *regcache, int regno)
{
  pid_t tid;

  /* Use the old method of poking around in `struct user' if the
     SETREGS request isn't available.  */
  if (! have_ptrace_getregs)
    {
      old_store_inferior_registers (regcache, regno);
      return;
    }

  tid = get_ptrace_pid (regcache->ptid ());

  /* Use the PTRACE_SETFPREGS requests whenever possible, since it
     transfers more registers in one system call.  But remember that
     store_fpregs can fail, and return zero.  */
  if (regno == -1)
    {
      store_regs (regcache, tid, regno);
      store_fpregs (regcache, tid, regno);
      return;
    }

  if (getregs_supplies (regno))
    {
      store_regs (regcache, tid, regno);
      return;
    }

  if (getfpregs_supplies (regno))
    {
      store_fpregs (regcache, tid, regno);
      return;
    }

  internal_error (_("Got request to store bad register number %d."), regno);
}


/* Fetch the thread-local storage pointer for libthread_db.  */

ps_err_e
ps_get_thread_area (struct ps_prochandle *ph, 
		    lwpid_t lwpid, int idx, void **base)
{
  if (ptrace (PTRACE_GET_THREAD_AREA, lwpid, NULL, base) < 0)
    return PS_ERR;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  *base = (char *) *base - idx;

  return PS_OK;
}

void _initialize_m68k_linux_nat ();
void
_initialize_m68k_linux_nat ()
{
  /* Register the target.  */
  linux_target = &the_m68k_linux_nat_target;
  add_inf_child_target (&the_m68k_linux_nat_target);
}
