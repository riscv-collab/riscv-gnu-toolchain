/* IBM RS/6000 native-dependent code for GDB, the GNU debugger.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include "target.h"
#include "gdbcore.h"
#include "symfile.h"
#include "objfiles.h"
#include "bfd.h"
#include "gdb-stabs.h"
#include "regcache.h"
#include "arch-utils.h"
#include "inf-child.h"
#include "inf-ptrace.h"
#include "ppc-tdep.h"
#include "rs6000-aix-tdep.h"
#include "exec.h"
#include "observable.h"
#include "xcoffread.h"

#include <sys/ptrace.h>
#include <sys/reg.h>

#include <sys/dir.h>
#include <sys/user.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <a.out.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "gdb_bfd.h"
#include <sys/core.h>
#define __LDINFO_PTRACE32__	/* for __ld_info32 */
#define __LDINFO_PTRACE64__	/* for __ld_info64 */
#include <sys/ldr.h>
#include <sys/systemcfg.h>

/* Header files for getting ppid in AIX of a child process.  */
#include <procinfo.h>
#include <sys/types.h>

/* Header files for alti-vec reg.  */
#include <sys/context.h>

/* On AIX4.3+, sys/ldr.h provides different versions of struct ld_info for
   debugging 32-bit and 64-bit processes.  Define a typedef and macros for
   accessing fields in the appropriate structures.  */

/* In 32-bit compilation mode (which is the only mode from which ptrace()
   works on 4.3), __ld_info32 is #defined as equivalent to ld_info.  */

#if defined (__ld_info32) || defined (__ld_info64)
# define ARCH3264
#endif

/* Return whether the current architecture is 64-bit.  */

#ifndef ARCH3264
# define ARCH64() 0
#else
# define ARCH64() (register_size (current_inferior ()->arch (), 0) == 8)
#endif

class rs6000_nat_target final : public inf_ptrace_target
{
public:
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  void create_inferior (const char *, const std::string &,
			char **, int) override;

  ptid_t wait (ptid_t, struct target_waitstatus *, target_wait_flags) override;

  /* Fork detection related functions, For adding multi process debugging
     support.  */
  void follow_fork (inferior *, ptid_t, target_waitkind, bool, bool) override;

  const struct target_desc *read_description ()  override;

  int insert_fork_catchpoint (int) override;
  int remove_fork_catchpoint (int) override;

protected:

  void post_startup_inferior (ptid_t ptid) override;

private:
  enum target_xfer_status
    xfer_shared_libraries (enum target_object object,
			   const char *annex, gdb_byte *readbuf,
			   const gdb_byte *writebuf,
			   ULONGEST offset, ULONGEST len,
			   ULONGEST *xfered_len);
};

static rs6000_nat_target the_rs6000_nat_target;

/* The below declaration is to track number of times, parent has
   reported fork event before its children.  */

static std::list<pid_t> aix_pending_parent;

/* The below declaration is for a child process event that
   is reported before its corresponding parent process in
   the event of a fork ().  */

static std::list<pid_t> aix_pending_children;

static void
aix_remember_child (pid_t pid)
{
  aix_pending_children.push_front (pid);
}

static void
aix_remember_parent (pid_t pid)
{
  aix_pending_parent.push_front (pid);
}

/* This function returns a parent of a child process.  */

static pid_t
find_my_aix_parent (pid_t child_pid)
{
  struct procsinfo ProcessBuffer1;

  if (getprocs (&ProcessBuffer1, sizeof (ProcessBuffer1),
		NULL, 0, &child_pid, 1) != 1)
    return 0;
  else
    return ProcessBuffer1.pi_ppid;
}

/* In the below function we check if there was any child
   process pending.  If it exists we return it from the
   list, otherwise we return a null.  */

static pid_t
has_my_aix_child_reported (pid_t parent_pid)
{
  pid_t child = 0;
  auto it = std::find_if (aix_pending_children.begin (),
			  aix_pending_children.end (),
			  [=] (pid_t child_pid)
			  {
			    return find_my_aix_parent (child_pid) == parent_pid;
			  });
  if (it != aix_pending_children.end ())
    {
      child = *it;
      aix_pending_children.erase (it);
    }
  return child;
}

/* In the below function we check if there was any parent
   process pending.  If it exists we return it from the
   list, otherwise we return a null.  */

static pid_t
has_my_aix_parent_reported (pid_t child_pid)
{
  pid_t my_parent = find_my_aix_parent (child_pid);
  auto it = std::find (aix_pending_parent.begin (),
		       aix_pending_parent.end (),
		       my_parent);
  if (it != aix_pending_parent.end ())
    {
      aix_pending_parent.erase (it);
      return my_parent;
    }
  return 0;
}

/* Given REGNO, a gdb register number, return the corresponding
   number suitable for use as a ptrace() parameter.  Return -1 if
   there's no suitable mapping.  Also, set the int pointed to by
   ISFLOAT to indicate whether REGNO is a floating point register.  */

static int
regmap (struct gdbarch *gdbarch, int regno, int *isfloat)
{
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  *isfloat = 0;
  if (tdep->ppc_gp0_regnum <= regno
      && regno < tdep->ppc_gp0_regnum + ppc_num_gprs)
    return regno;
  else if (tdep->ppc_fp0_regnum >= 0
	   && tdep->ppc_fp0_regnum <= regno
	   && regno < tdep->ppc_fp0_regnum + ppc_num_fprs)
    {
      *isfloat = 1;
      return regno - tdep->ppc_fp0_regnum + FPR0;
    }
  else if (regno == gdbarch_pc_regnum (gdbarch))
    return IAR;
  else if (regno == tdep->ppc_ps_regnum)
    return MSR;
  else if (regno == tdep->ppc_cr_regnum)
    return CR;
  else if (regno == tdep->ppc_lr_regnum)
    return LR;
  else if (regno == tdep->ppc_ctr_regnum)
    return CTR;
  else if (regno == tdep->ppc_xer_regnum)
    return XER;
  else if (tdep->ppc_fpscr_regnum >= 0
	   && regno == tdep->ppc_fpscr_regnum)
    return FPSCR;
  else if (tdep->ppc_mq_regnum >= 0 && regno == tdep->ppc_mq_regnum)
    return MQ;
  else
    return -1;
}

/* Call ptrace(REQ, ID, ADDR, DATA, BUF).  */

static int
rs6000_ptrace32 (int req, int id, int *addr, int data, int *buf)
{
#ifdef HAVE_PTRACE64
  int ret = ptrace64 (req, id, (uintptr_t) addr, data, buf);
#else
  int ret = ptrace (req, id, (int *)addr, data, buf);
#endif
#if 0
  printf ("rs6000_ptrace32 (%d, %d, 0x%x, %08x, 0x%x) = 0x%x\n",
	  req, id, (unsigned int)addr, data, (unsigned int)buf, ret);
#endif
  return ret;
}

/* Call ptracex(REQ, ID, ADDR, DATA, BUF).  */

static int
rs6000_ptrace64 (int req, int id, long long addr, int data, void *buf)
{
#ifdef ARCH3264
#  ifdef HAVE_PTRACE64
  int ret = ptrace64 (req, id, addr, data, (PTRACE_TYPE_ARG5) buf);
#  else
  int ret = ptracex (req, id, addr, data, (PTRACE_TYPE_ARG5) buf);
#  endif
#else
  int ret = 0;
#endif
#if 0
  printf ("rs6000_ptrace64 (%d, %d, %s, %08x, 0x%x) = 0x%x\n",
	  req, id, hex_string (addr), data, (unsigned int)buf, ret);
#endif
  return ret;
}

/* Store the vsx registers.  */

static void
store_vsx_register_aix (struct regcache *regcache, int regno)
{
  int ret;
  struct gdbarch *gdbarch = regcache->arch ();
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  struct thrdentry64 thrdentry;
  __vsx_context_t vsx;
  pid_t pid = inferior_ptid.pid ();
  tid64_t thrd_i = 0;

  if (getthrds64(pid, &thrdentry, sizeof(struct thrdentry64),
		 &thrd_i, 1) == 1)
    thrd_i = thrdentry.ti_tid;

  memset(&vsx, 0, sizeof(__vsx_context_t));
  if (__power_vsx() && thrd_i > 0)
    {
      if (ARCH64 ())
	ret = rs6000_ptrace64 (PTT_READ_VSX, thrd_i, (long long) &vsx, 0, 0);
      else
	ret = rs6000_ptrace32 (PTT_READ_VSX, thrd_i, (int *)&vsx, 0, 0);
      if (ret < 0)
	return;

      regcache->raw_collect (regno, &(vsx.__vsr_dw1[0])+
			     regno - tdep->ppc_vsr0_upper_regnum);

      if (ARCH64 ())
	ret = rs6000_ptrace64 (PTT_WRITE_VSX, thrd_i, (long long) &vsx, 0, 0);
      else
	ret = rs6000_ptrace32 (PTT_WRITE_VSX, thrd_i, (int *) &vsx, 0, 0);

      if (ret < 0)
	perror_with_name (_("Unable to write VSX registers after reading it"));
    }
}

/* Store Altivec registers.  */

static void
store_altivec_register_aix (struct regcache *regcache, int regno)
{
  int ret;
  struct gdbarch *gdbarch = regcache->arch ();
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  struct thrdentry64 thrdentry;
  __vmx_context_t vmx;
  pid_t pid = inferior_ptid.pid ();
  tid64_t  thrd_i = 0;

  if (getthrds64(pid, &thrdentry, sizeof(struct thrdentry64),
		 &thrd_i, 1) == 1)
    thrd_i = thrdentry.ti_tid;

  memset(&vmx, 0, sizeof(__vmx_context_t));
  if (__power_vmx() && thrd_i > 0)
    {
      if (ARCH64 ())
	ret = rs6000_ptrace64 (PTT_READ_VEC, thrd_i, (long long) &vmx, 0, 0);
      else
	ret = rs6000_ptrace32 (PTT_READ_VEC, thrd_i, (int *) &vmx, 0, 0);
      if (ret < 0)
	return;

      regcache->raw_collect (regno, &(vmx.__vr[0]) + regno
			     - tdep->ppc_vr0_regnum);

      if (ARCH64 ())
	ret = rs6000_ptrace64 (PTT_WRITE_VEC, thrd_i, (long long) &vmx, 0, 0);
      else
	ret = rs6000_ptrace32 (PTT_WRITE_VEC, thrd_i, (int *) &vmx, 0, 0);
      if (ret < 0)
	perror_with_name (_("Unable to store AltiVec register after reading it"));
    }
}

/* Supply altivec registers.  */

static void
supply_vrregset_aix (struct regcache *regcache, __vmx_context_t *vmx)
{
  int i;
  struct gdbarch *gdbarch = regcache->arch ();
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  int num_of_vrregs = tdep->ppc_vrsave_regnum - tdep->ppc_vr0_regnum + 1;

  for (i = 0; i < num_of_vrregs; i++)
    regcache->raw_supply (tdep->ppc_vr0_regnum + i,
			  &(vmx->__vr[i]));
  regcache->raw_supply (tdep->ppc_vrsave_regnum, &(vmx->__vrsave));
  regcache->raw_supply (tdep->ppc_vrsave_regnum - 1, &(vmx->__vscr));
}

/* Fetch altivec register.  */

static void
fetch_altivec_registers_aix (struct regcache *regcache)
{
  struct thrdentry64 thrdentry;
  __vmx_context_t vmx;
  pid_t pid = current_inferior ()->pid;
  tid64_t  thrd_i = 0;

  if (getthrds64(pid, &thrdentry, sizeof(struct thrdentry64),
		 &thrd_i, 1) == 1)
    thrd_i = thrdentry.ti_tid;

  memset(&vmx, 0, sizeof(__vmx_context_t));
  if (__power_vmx() && thrd_i > 0)
    {
      if (ARCH64 ())
	rs6000_ptrace64 (PTT_READ_VEC, thrd_i, (long long) &vmx, 0, 0);
      else
	rs6000_ptrace32 (PTT_READ_VEC, thrd_i, (int *) &vmx, 0, 0);
      supply_vrregset_aix (regcache, &vmx);
    }
}

/* supply vsx register.  */

static void
supply_vsxregset_aix (struct regcache *regcache, __vsx_context_t *vsx)
{
  int i;
  struct gdbarch *gdbarch = regcache->arch ();
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

  for (i = 0; i < ppc_num_vshrs; i++)
   regcache->raw_supply (tdep->ppc_vsr0_upper_regnum + i,
			 &(vsx->__vsr_dw1[i]));
}

/* Fetch vsx registers.  */
static void
fetch_vsx_registers_aix (struct regcache *regcache)
{
  struct thrdentry64 thrdentry;
  __vsx_context_t vsx;
  pid_t pid = current_inferior ()->pid;
  tid64_t  thrd_i = 0;

  if (getthrds64(pid, &thrdentry, sizeof(struct thrdentry64),
		 &thrd_i, 1) == 1)
    thrd_i = thrdentry.ti_tid;

  memset(&vsx, 0, sizeof(__vsx_context_t));
  if (__power_vsx() && thrd_i > 0)
    {
      if (ARCH64 ())
	rs6000_ptrace64 (PTT_READ_VSX, thrd_i, (long long) &vsx, 0, 0);
      else
	rs6000_ptrace32 (PTT_READ_VSX, thrd_i, (int *) &vsx, 0, 0);
      supply_vsxregset_aix (regcache, &vsx);
    }
}

void rs6000_nat_target::post_startup_inferior (ptid_t ptid)
{

  /* In AIX to turn on multi process debugging in ptrace
     PT_MULTI is the option to be passed,
     with the process ID which can fork () and
     the data parameter [fourth parameter] must be 1.  */

  if (!ARCH64 ())
    rs6000_ptrace32 (PT_MULTI, ptid.pid(), 0, 1, 0);
  else
    rs6000_ptrace64 (PT_MULTI, ptid.pid(), 0, 1, 0);
}

void
rs6000_nat_target::follow_fork (inferior *child_inf, ptid_t child_ptid,
				target_waitkind fork_kind, bool follow_child,
				bool detach_fork)
{

  /* Once the fork event is detected the infrun.c code
     calls the target_follow_fork to take care of
     follow child and detach the child activity which is
     done using the function below.  */

  inf_ptrace_target::follow_fork (child_inf, child_ptid, fork_kind,
				  follow_child, detach_fork);

  /* If we detach fork and follow child we do not want the child
     process to generate events that ptrace can trace.  Hence we
     detach it.  */

  if (detach_fork && !follow_child)
  {
    if (ARCH64 ())
      rs6000_ptrace64 (PT_DETACH, child_ptid.pid (), 0, 0, 0);
    else
      rs6000_ptrace32 (PT_DETACH, child_ptid.pid (), 0, 0, 0);
  }
}

/* Functions for catchpoint in AIX.  */
int
rs6000_nat_target::insert_fork_catchpoint (int pid)
{
  return 0;
}

int
rs6000_nat_target::remove_fork_catchpoint (int pid)
{
  return 0;
}

/* Fetch register REGNO from the inferior.  */

static void
fetch_register (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  int addr[PPC_MAX_REGISTER_SIZE];
  int nr, isfloat;
  pid_t pid = regcache->ptid ().pid ();

  /* Retrieved values may be -1, so infer errors from errno.  */
  errno = 0;

  /* Alti-vec register.  */
  if (altivec_register_p (gdbarch, regno))
    {
      fetch_altivec_registers_aix (regcache);
      return;
    }

  /* VSX register.  */
  if (vsx_register_p (gdbarch, regno))
    {
      fetch_vsx_registers_aix (regcache);
      return;
    }

  nr = regmap (gdbarch, regno, &isfloat);

  /* Floating-point registers.  */
  if (isfloat)
    rs6000_ptrace32 (PT_READ_FPR, pid, addr, nr, 0);

  /* Bogus register number.  */
  else if (nr < 0)
    {
      if (regno >= gdbarch_num_regs (gdbarch))
	gdb_printf (gdb_stderr,
		    "gdb error: register no %d not implemented.\n",
		    regno);
      return;
    }

  /* Fixed-point registers.  */
  else
    {
      if (!ARCH64 ())
	*addr = rs6000_ptrace32 (PT_READ_GPR, pid, (int *) nr, 0, 0);
      else
	{
	  /* PT_READ_GPR requires the buffer parameter to point to long long,
	     even if the register is really only 32 bits.  */
	  long long buf;
	  rs6000_ptrace64 (PT_READ_GPR, pid, nr, 0, &buf);
	  if (register_size (gdbarch, regno) == 8)
	    memcpy (addr, &buf, 8);
	  else
	    *addr = buf;
	}
    }

  if (!errno)
    regcache->raw_supply (regno, (char *) addr);
  else
    {
#if 0
      /* FIXME: this happens 3 times at the start of each 64-bit program.  */
      perror (_("ptrace read"));
#endif
      errno = 0;
    }
}

/* Store register REGNO back into the inferior.  */

static void
store_register (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  int addr[PPC_MAX_REGISTER_SIZE];
  int nr, isfloat;
  pid_t pid = regcache->ptid ().pid ();

  /* Fetch the register's value from the register cache.  */
  regcache->raw_collect (regno, addr);

  /* -1 can be a successful return value, so infer errors from errno.  */
  errno = 0;

  if (altivec_register_p (gdbarch, regno))
    {
      store_altivec_register_aix (regcache, regno);
      return;
    }

  if (vsx_register_p (gdbarch, regno))
    {
      store_vsx_register_aix (regcache, regno);
      return;
    }

  nr = regmap (gdbarch, regno, &isfloat);

  /* Floating-point registers.  */
  if (isfloat)
    rs6000_ptrace32 (PT_WRITE_FPR, pid, addr, nr, 0);

  /* Bogus register number.  */
  else if (nr < 0)
    {
      if (regno >= gdbarch_num_regs (gdbarch))
	gdb_printf (gdb_stderr,
		    "gdb error: register no %d not implemented.\n",
		    regno);
    }

  /* Fixed-point registers.  */
  else
    {
      /* The PT_WRITE_GPR operation is rather odd.  For 32-bit inferiors,
	 the register's value is passed by value, but for 64-bit inferiors,
	 the address of a buffer containing the value is passed.  */
      if (!ARCH64 ())
	rs6000_ptrace32 (PT_WRITE_GPR, pid, (int *) nr, *addr, 0);
      else
	{
	  /* PT_WRITE_GPR requires the buffer parameter to point to an 8-byte
	     area, even if the register is really only 32 bits.  */
	  long long buf;
	  if (register_size (gdbarch, regno) == 8)
	    memcpy (&buf, addr, 8);
	  else
	    buf = *addr;
	  rs6000_ptrace64 (PT_WRITE_GPR, pid, nr, 0, &buf);
	}
    }

  if (errno)
    {
      perror (_("ptrace write"));
      errno = 0;
    }
}

/* Read from the inferior all registers if REGNO == -1 and just register
   REGNO otherwise.  */

void
rs6000_nat_target::fetch_registers (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  if (regno != -1)
    fetch_register (regcache, regno);

  else
    {
      ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

      /* Read 32 general purpose registers.  */
      for (regno = tdep->ppc_gp0_regnum;
	   regno < tdep->ppc_gp0_regnum + ppc_num_gprs;
	   regno++)
	{
	  fetch_register (regcache, regno);
	}

      /* Read general purpose floating point registers.  */
      if (tdep->ppc_fp0_regnum >= 0)
	for (regno = 0; regno < ppc_num_fprs; regno++)
	  fetch_register (regcache, tdep->ppc_fp0_regnum + regno);

      if (tdep->ppc_vr0_regnum != -1 && tdep->ppc_vrsave_regnum != -1)
	fetch_altivec_registers_aix (regcache);

      if (tdep->ppc_vsr0_upper_regnum != -1)
	fetch_vsx_registers_aix (regcache);

      /* Read special registers.  */
      fetch_register (regcache, gdbarch_pc_regnum (gdbarch));
      fetch_register (regcache, tdep->ppc_ps_regnum);
      fetch_register (regcache, tdep->ppc_cr_regnum);
      fetch_register (regcache, tdep->ppc_lr_regnum);
      fetch_register (regcache, tdep->ppc_ctr_regnum);
      fetch_register (regcache, tdep->ppc_xer_regnum);
      if (tdep->ppc_fpscr_regnum >= 0)
	fetch_register (regcache, tdep->ppc_fpscr_regnum);
      if (tdep->ppc_mq_regnum >= 0)
	fetch_register (regcache, tdep->ppc_mq_regnum);
    }
}

const struct target_desc *
rs6000_nat_target::read_description ()
{
   if (ARCH64())
     {
       if (__power_vsx ())
	 return tdesc_powerpc_vsx64;
       else if (__power_vmx ())
	 return tdesc_powerpc_altivec64;
     }
   else
     {
       if (__power_vsx ())
	 return tdesc_powerpc_vsx32;
       else if (__power_vmx ())
	 return tdesc_powerpc_altivec32;
     }
   return NULL;
}

/* Store our register values back into the inferior.
   If REGNO is -1, do this for all registers.
   Otherwise, REGNO specifies which register (so we can save time).  */

void
rs6000_nat_target::store_registers (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  if (regno != -1)
    store_register (regcache, regno);

  else
    {
      ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);

      /* Write general purpose registers first.  */
      for (regno = tdep->ppc_gp0_regnum;
	   regno < tdep->ppc_gp0_regnum + ppc_num_gprs;
	   regno++)
	{
	  store_register (regcache, regno);
	}

      /* Write floating point registers.  */
      if (tdep->ppc_fp0_regnum >= 0)
	for (regno = 0; regno < ppc_num_fprs; regno++)
	  store_register (regcache, tdep->ppc_fp0_regnum + regno);

      /* Write special registers.  */
      store_register (regcache, gdbarch_pc_regnum (gdbarch));
      store_register (regcache, tdep->ppc_ps_regnum);
      store_register (regcache, tdep->ppc_cr_regnum);
      store_register (regcache, tdep->ppc_lr_regnum);
      store_register (regcache, tdep->ppc_ctr_regnum);
      store_register (regcache, tdep->ppc_xer_regnum);
      if (tdep->ppc_fpscr_regnum >= 0)
	store_register (regcache, tdep->ppc_fpscr_regnum);
      if (tdep->ppc_mq_regnum >= 0)
	store_register (regcache, tdep->ppc_mq_regnum);
    }
}

/* Implement the to_xfer_partial target_ops method.  */

enum target_xfer_status
rs6000_nat_target::xfer_partial (enum target_object object,
				 const char *annex, gdb_byte *readbuf,
				 const gdb_byte *writebuf,
				 ULONGEST offset, ULONGEST len,
				 ULONGEST *xfered_len)
{
  pid_t pid = inferior_ptid.pid ();
  int arch64 = ARCH64 ();

  switch (object)
    {
    case TARGET_OBJECT_LIBRARIES_AIX:
      return xfer_shared_libraries (object, annex,
				    readbuf, writebuf,
				    offset, len, xfered_len);
    case TARGET_OBJECT_MEMORY:
      {
	union
	{
	  PTRACE_TYPE_RET word;
	  gdb_byte byte[sizeof (PTRACE_TYPE_RET)];
	} buffer;
	ULONGEST rounded_offset;
	LONGEST partial_len;

	/* Round the start offset down to the next long word
	   boundary.  */
	rounded_offset = offset & -(ULONGEST) sizeof (PTRACE_TYPE_RET);

	/* Since ptrace will transfer a single word starting at that
	   rounded_offset the partial_len needs to be adjusted down to
	   that (remember this function only does a single transfer).
	   Should the required length be even less, adjust it down
	   again.  */
	partial_len = (rounded_offset + sizeof (PTRACE_TYPE_RET)) - offset;
	if (partial_len > len)
	  partial_len = len;

	if (writebuf)
	  {
	    /* If OFFSET:PARTIAL_LEN is smaller than
	       ROUNDED_OFFSET:WORDSIZE then a read/modify write will
	       be needed.  Read in the entire word.  */
	    if (rounded_offset < offset
		|| (offset + partial_len
		    < rounded_offset + sizeof (PTRACE_TYPE_RET)))
	      {
		/* Need part of initial word -- fetch it.  */
		if (arch64)
		  buffer.word = rs6000_ptrace64 (PT_READ_I, pid,
						 rounded_offset, 0, NULL);
		else
		  buffer.word = rs6000_ptrace32 (PT_READ_I, pid,
						 (int *) (uintptr_t)
						 rounded_offset,
						 0, NULL);
	      }

	    /* Copy data to be written over corresponding part of
	       buffer.  */
	    memcpy (buffer.byte + (offset - rounded_offset),
		    writebuf, partial_len);

	    errno = 0;
	    if (arch64)
	      rs6000_ptrace64 (PT_WRITE_D, pid,
			       rounded_offset, buffer.word, NULL);
	    else
	      rs6000_ptrace32 (PT_WRITE_D, pid,
			       (int *) (uintptr_t) rounded_offset,
			       buffer.word, NULL);
	    if (errno)
	      return TARGET_XFER_EOF;
	  }

	if (readbuf)
	  {
	    errno = 0;
	    if (arch64)
	      buffer.word = rs6000_ptrace64 (PT_READ_I, pid,
					     rounded_offset, 0, NULL);
	    else
	      buffer.word = rs6000_ptrace32 (PT_READ_I, pid,
					     (int *)(uintptr_t)rounded_offset,
					     0, NULL);
	    if (errno)
	      return TARGET_XFER_EOF;

	    /* Copy appropriate bytes out of the buffer.  */
	    memcpy (readbuf, buffer.byte + (offset - rounded_offset),
		    partial_len);
	  }

	*xfered_len = (ULONGEST) partial_len;
	return TARGET_XFER_OK;
      }

    default:
      return TARGET_XFER_E_IO;
    }
}

/* Wait for the child specified by PTID to do something.  Return the
   process ID of the child, or MINUS_ONE_PTID in case of error; store
   the status in *OURSTATUS.  */

ptid_t
rs6000_nat_target::wait (ptid_t ptid, struct target_waitstatus *ourstatus,
			 target_wait_flags options)
{
  pid_t pid;
  int status, save_errno;

  while (1)
    {
      set_sigint_trap ();

      do
	{
	  pid = waitpid (ptid.pid (), &status, 0);
	  save_errno = errno;
	}
      while (pid == -1 && errno == EINTR);

      clear_sigint_trap ();

      if (pid == -1)
	{
	  gdb_printf (gdb_stderr,
		      _("Child process unexpectedly missing: %s.\n"),
		      safe_strerror (save_errno));

	  ourstatus->set_ignore ();
	  return minus_one_ptid;
	}

      /* Ignore terminated detached child processes.  */
      if (!WIFSTOPPED (status) && find_inferior_pid (this, pid) == nullptr)
	continue;

      /* Check for a fork () event.  */
      if ((status & 0xff) == W_SFWTED)
	{
	  /* Checking whether it is a parent or a child event.  */

	  /* If the event is a child we check if there was a parent
	     event recorded before.  If yes we got the parent child
	     relationship.  If not we push this child and wait for
	     the next fork () event.  */
	  if (find_inferior_pid (this, pid) == nullptr)
	    {
	      pid_t parent_pid = has_my_aix_parent_reported (pid);
	      if (parent_pid > 0)
		{
		  ourstatus->set_forked (ptid_t (pid));
		  return ptid_t (parent_pid);
		}
	      aix_remember_child (pid);
	    }

	  /* If the event is a parent we check if there was a child
	     event recorded before.  If yes we got the parent child
	     relationship.  If not we push this parent and wait for
	     the next fork () event.  */
	  else
	    {
	      pid_t child_pid = has_my_aix_child_reported (pid);
	      if (child_pid > 0)
		{
		  ourstatus->set_forked (ptid_t (child_pid));
		  return ptid_t (pid);
		}
	      aix_remember_parent (pid);
	    }
	  continue;
	}

      break;
    }

  /* AIX has a couple of strange returns from wait().  */

  /* stop after load" status.  */
  if (status == 0x57c)
    ourstatus->set_loaded ();
  /* 0x7f is signal 0.  */
  else if (status == 0x7f)
    ourstatus->set_spurious ();
  /* A normal waitstatus.  Let the usual macros deal with it.  */
  else
    *ourstatus = host_status_to_waitstatus (status);

  return ptid_t (pid);
}


/* Set the current architecture from the host running GDB.  Called when
   starting a child process.  */

void
rs6000_nat_target::create_inferior (const char *exec_file,
				    const std::string &allargs,
				    char **env, int from_tty)
{
  enum bfd_architecture arch;
  unsigned long mach;
  bfd abfd;

  inf_ptrace_target::create_inferior (exec_file, allargs, env, from_tty);

  if (__power_rs ())
    {
      arch = bfd_arch_rs6000;
      mach = bfd_mach_rs6k;
    }
  else
    {
      arch = bfd_arch_powerpc;
      mach = bfd_mach_ppc;
    }

  /* FIXME: schauer/2002-02-25:
     We don't know if we are executing a 32 or 64 bit executable,
     and have no way to pass the proper word size to rs6000_gdbarch_init.
     So we have to avoid switching to a new architecture, if the architecture
     matches already.
     Blindly calling rs6000_gdbarch_init used to work in older versions of
     GDB, as rs6000_gdbarch_init incorrectly used the previous tdep to
     determine the wordsize.  */
  if (current_program_space->exec_bfd ())
    {
      const struct bfd_arch_info *exec_bfd_arch_info;

      exec_bfd_arch_info
	= bfd_get_arch_info (current_program_space->exec_bfd ());
      if (arch == exec_bfd_arch_info->arch)
	return;
    }

  bfd_default_set_arch_mach (&abfd, arch, mach);

  gdbarch_info info;
  info.bfd_arch_info = bfd_get_arch_info (&abfd);
  info.abfd = current_program_space->exec_bfd ();

  if (!gdbarch_update_p (info))
    internal_error (_("rs6000_create_inferior: failed "
		      "to select architecture"));
}


/* Shared Object support.  */

/* Return the LdInfo data for the given process.  Raises an error
   if the data could not be obtained.  */

static gdb::byte_vector
rs6000_ptrace_ldinfo (ptid_t ptid)
{
  const int pid = ptid.pid ();
  gdb::byte_vector ldi (1024);
  int rc = -1;

  while (1)
    {
      if (ARCH64 ())
	rc = rs6000_ptrace64 (PT_LDINFO, pid, (unsigned long) ldi.data (),
			      ldi.size (), NULL);
      else
	rc = rs6000_ptrace32 (PT_LDINFO, pid, (int *) ldi.data (),
			      ldi.size (), NULL);

      if (rc != -1)
	break; /* Success, we got the entire ld_info data.  */

      if (errno != ENOMEM)
	perror_with_name (_("ptrace ldinfo"));

      /* ldi is not big enough.  Double it and try again.  */
      ldi.resize (ldi.size () * 2);
    }

  return ldi;
}

/* Implement the to_xfer_partial target_ops method for
   TARGET_OBJECT_LIBRARIES_AIX objects.  */

enum target_xfer_status
rs6000_nat_target::xfer_shared_libraries
  (enum target_object object,
   const char *annex, gdb_byte *readbuf, const gdb_byte *writebuf,
   ULONGEST offset, ULONGEST len, ULONGEST *xfered_len)
{
  ULONGEST result;

  /* This function assumes that it is being run with a live process.
     Core files are handled via gdbarch.  */
  gdb_assert (target_has_execution ());

  if (writebuf)
    return TARGET_XFER_E_IO;

  gdb::byte_vector ldi_buf = rs6000_ptrace_ldinfo (inferior_ptid);
  result = rs6000_aix_ld_info_to_xml (current_inferior ()->arch (),
				      ldi_buf.data (),
				      readbuf, offset, len, 1);

  if (result == 0)
    return TARGET_XFER_EOF;
  else
    {
      *xfered_len = result;
      return TARGET_XFER_OK;
    }
}

void _initialize_rs6000_nat ();
void
_initialize_rs6000_nat ()
{
  add_inf_child_target (&the_rs6000_nat_target);
}
