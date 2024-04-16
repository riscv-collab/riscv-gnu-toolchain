/* GNU/Linux on ARM native support.
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

#include "defs.h"
#include "inferior.h"
#include "gdbcore.h"
#include "regcache.h"
#include "target.h"
#include "linux-nat.h"
#include "target-descriptions.h"
#include "auxv.h"
#include "observable.h"
#include "gdbthread.h"

#include "aarch32-tdep.h"
#include "arm-tdep.h"
#include "arm-linux-tdep.h"
#include "aarch32-linux-nat.h"

#include <elf/common.h>
#include <sys/user.h>
#include "nat/gdb_ptrace.h"
#include <sys/utsname.h>
#include <sys/procfs.h>

#include "nat/linux-ptrace.h"
#include "linux-tdep.h"

/* Prototypes for supply_gregset etc.  */
#include "gregset.h"

/* Defines ps_err_e, struct ps_prochandle.  */
#include "gdb_proc_service.h"

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 22
#endif

#ifndef PTRACE_GETWMMXREGS
#define PTRACE_GETWMMXREGS 18
#define PTRACE_SETWMMXREGS 19
#endif

#ifndef PTRACE_GETVFPREGS
#define PTRACE_GETVFPREGS 27
#define PTRACE_SETVFPREGS 28
#endif

#ifndef PTRACE_GETHBPREGS
#define PTRACE_GETHBPREGS 29
#define PTRACE_SETHBPREGS 30
#endif

class arm_linux_nat_target final : public linux_nat_target
{
public:
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  /* Add our hardware breakpoint and watchpoint implementation.  */
  int can_use_hw_breakpoint (enum bptype, int, int) override;

  int insert_hw_breakpoint (struct gdbarch *, struct bp_target_info *) override;

  int remove_hw_breakpoint (struct gdbarch *, struct bp_target_info *) override;

  int region_ok_for_hw_watchpoint (CORE_ADDR, int) override;

  int insert_watchpoint (CORE_ADDR, int, enum target_hw_bp_type,
			 struct expression *) override;

  int remove_watchpoint (CORE_ADDR, int, enum target_hw_bp_type,
			 struct expression *) override;
  bool stopped_by_watchpoint () override;

  bool stopped_data_address (CORE_ADDR *) override;

  bool watchpoint_addr_within_range (CORE_ADDR, CORE_ADDR, int) override;

  const struct target_desc *read_description () override;

  /* Override linux_nat_target low methods.  */

  /* Handle thread creation and exit.  */
  void low_new_thread (struct lwp_info *lp) override;
  void low_delete_thread (struct arch_lwp_info *lp) override;
  void low_prepare_to_resume (struct lwp_info *lp) override;

  /* Handle process creation and exit.  */
  void low_new_fork (struct lwp_info *parent, pid_t child_pid) override;
  void low_forget_process (pid_t pid) override;
};

static arm_linux_nat_target the_arm_linux_nat_target;

/* Get the whole floating point state of the process and store it
   into regcache.  */

static void
fetch_fpregs (struct regcache *regcache)
{
  int ret, regno, tid;
  gdb_byte fp[ARM_LINUX_SIZEOF_NWFPE];

  /* Get the thread id for the ptrace call.  */
  tid = regcache->ptid ().lwp ();

  /* Read the floating point state.  */
  if (have_ptrace_getregset == TRIBOOL_TRUE)
    {
      struct iovec iov;

      iov.iov_base = &fp;
      iov.iov_len = ARM_LINUX_SIZEOF_NWFPE;

      ret = ptrace (PTRACE_GETREGSET, tid, NT_FPREGSET, &iov);
    }
  else
    ret = ptrace (PT_GETFPREGS, tid, 0, fp);

  if (ret < 0)
    perror_with_name (_("Unable to fetch the floating point registers"));

  /* Fetch fpsr.  */
  regcache->raw_supply (ARM_FPS_REGNUM, fp + NWFPE_FPSR_OFFSET);

  /* Fetch the floating point registers.  */
  for (regno = ARM_F0_REGNUM; regno <= ARM_F7_REGNUM; regno++)
    supply_nwfpe_register (regcache, regno, fp);
}

/* Save the whole floating point state of the process using
   the contents from regcache.  */

static void
store_fpregs (const struct regcache *regcache)
{
  int ret, regno, tid;
  gdb_byte fp[ARM_LINUX_SIZEOF_NWFPE];

  /* Get the thread id for the ptrace call.  */
  tid = regcache->ptid ().lwp ();

  /* Read the floating point state.  */
  if (have_ptrace_getregset == TRIBOOL_TRUE)
    {
      elf_fpregset_t fpregs;
      struct iovec iov;

      iov.iov_base = &fpregs;
      iov.iov_len = sizeof (fpregs);

      ret = ptrace (PTRACE_GETREGSET, tid, NT_FPREGSET, &iov);
    }
  else
    ret = ptrace (PT_GETFPREGS, tid, 0, fp);

  if (ret < 0)
    perror_with_name (_("Unable to fetch the floating point registers"));

  /* Store fpsr.  */
  if (REG_VALID == regcache->get_register_status (ARM_FPS_REGNUM))
    regcache->raw_collect (ARM_FPS_REGNUM, fp + NWFPE_FPSR_OFFSET);

  /* Store the floating point registers.  */
  for (regno = ARM_F0_REGNUM; regno <= ARM_F7_REGNUM; regno++)
    if (REG_VALID == regcache->get_register_status (regno))
      collect_nwfpe_register (regcache, regno, fp);

  if (have_ptrace_getregset == TRIBOOL_TRUE)
    {
      struct iovec iov;

      iov.iov_base = &fp;
      iov.iov_len = ARM_LINUX_SIZEOF_NWFPE;

      ret = ptrace (PTRACE_SETREGSET, tid, NT_FPREGSET, &iov);
    }
  else
    ret = ptrace (PTRACE_SETFPREGS, tid, 0, fp);

  if (ret < 0)
    perror_with_name (_("Unable to store floating point registers"));
}

/* Fetch all general registers of the process and store into
   regcache.  */

static void
fetch_regs (struct regcache *regcache)
{
  int ret, tid;
  elf_gregset_t regs;

  /* Get the thread id for the ptrace call.  */
  tid = regcache->ptid ().lwp ();

  if (have_ptrace_getregset == TRIBOOL_TRUE)
    {
      struct iovec iov;

      iov.iov_base = &regs;
      iov.iov_len = sizeof (regs);

      ret = ptrace (PTRACE_GETREGSET, tid, NT_PRSTATUS, &iov);
    }
  else
    ret = ptrace (PTRACE_GETREGS, tid, 0, &regs);

  if (ret < 0)
    perror_with_name (_("Unable to fetch general registers"));

  aarch32_gp_regcache_supply (regcache, (uint32_t *) regs, arm_apcs_32);
}

static void
store_regs (const struct regcache *regcache)
{
  int ret, tid;
  elf_gregset_t regs;

  /* Get the thread id for the ptrace call.  */
  tid = regcache->ptid ().lwp ();

  /* Fetch the general registers.  */
  if (have_ptrace_getregset == TRIBOOL_TRUE)
    {
      struct iovec iov;

      iov.iov_base = &regs;
      iov.iov_len = sizeof (regs);

      ret = ptrace (PTRACE_GETREGSET, tid, NT_PRSTATUS, &iov);
    }
  else
    ret = ptrace (PTRACE_GETREGS, tid, 0, &regs);

  if (ret < 0)
    perror_with_name (_("Unable to fetch general registers"));

  aarch32_gp_regcache_collect (regcache, (uint32_t *) regs, arm_apcs_32);

  if (have_ptrace_getregset == TRIBOOL_TRUE)
    {
      struct iovec iov;

      iov.iov_base = &regs;
      iov.iov_len = sizeof (regs);

      ret = ptrace (PTRACE_SETREGSET, tid, NT_PRSTATUS, &iov);
    }
  else
    ret = ptrace (PTRACE_SETREGS, tid, 0, &regs);

  if (ret < 0)
    perror_with_name (_("Unable to store general registers"));
}

/* Fetch all WMMX registers of the process and store into
   regcache.  */

static void
fetch_wmmx_regs (struct regcache *regcache)
{
  char regbuf[IWMMXT_REGS_SIZE];
  int ret, regno, tid;

  /* Get the thread id for the ptrace call.  */
  tid = regcache->ptid ().lwp ();

  ret = ptrace (PTRACE_GETWMMXREGS, tid, 0, regbuf);
  if (ret < 0)
    perror_with_name (_("Unable to fetch WMMX registers"));

  for (regno = 0; regno < 16; regno++)
    regcache->raw_supply (regno + ARM_WR0_REGNUM, &regbuf[regno * 8]);

  for (regno = 0; regno < 2; regno++)
    regcache->raw_supply (regno + ARM_WCSSF_REGNUM,
			  &regbuf[16 * 8 + regno * 4]);

  for (regno = 0; regno < 4; regno++)
    regcache->raw_supply (regno + ARM_WCGR0_REGNUM,
			  &regbuf[16 * 8 + 2 * 4 + regno * 4]);
}

static void
store_wmmx_regs (const struct regcache *regcache)
{
  char regbuf[IWMMXT_REGS_SIZE];
  int ret, regno, tid;

  /* Get the thread id for the ptrace call.  */
  tid = regcache->ptid ().lwp ();

  ret = ptrace (PTRACE_GETWMMXREGS, tid, 0, regbuf);
  if (ret < 0)
    perror_with_name (_("Unable to fetch WMMX registers"));

  for (regno = 0; regno < 16; regno++)
    if (REG_VALID == regcache->get_register_status (regno + ARM_WR0_REGNUM))
      regcache->raw_collect (regno + ARM_WR0_REGNUM, &regbuf[regno * 8]);

  for (regno = 0; regno < 2; regno++)
    if (REG_VALID == regcache->get_register_status (regno + ARM_WCSSF_REGNUM))
      regcache->raw_collect (regno + ARM_WCSSF_REGNUM,
			     &regbuf[16 * 8 + regno * 4]);

  for (regno = 0; regno < 4; regno++)
    if (REG_VALID == regcache->get_register_status (regno + ARM_WCGR0_REGNUM))
      regcache->raw_collect (regno + ARM_WCGR0_REGNUM,
			     &regbuf[16 * 8 + 2 * 4 + regno * 4]);

  ret = ptrace (PTRACE_SETWMMXREGS, tid, 0, regbuf);

  if (ret < 0)
    perror_with_name (_("Unable to store WMMX registers"));
}

static void
fetch_vfp_regs (struct regcache *regcache)
{
  gdb_byte regbuf[ARM_VFP3_REGS_SIZE];
  int ret, tid;
  struct gdbarch *gdbarch = regcache->arch ();
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  /* Get the thread id for the ptrace call.  */
  tid = regcache->ptid ().lwp ();

  if (have_ptrace_getregset == TRIBOOL_TRUE)
    {
      struct iovec iov;

      iov.iov_base = regbuf;
      iov.iov_len = ARM_VFP3_REGS_SIZE;
      ret = ptrace (PTRACE_GETREGSET, tid, NT_ARM_VFP, &iov);
    }
  else
    ret = ptrace (PTRACE_GETVFPREGS, tid, 0, regbuf);

  if (ret < 0)
    perror_with_name (_("Unable to fetch VFP registers"));

  aarch32_vfp_regcache_supply (regcache, regbuf,
			       tdep->vfp_register_count);
}

static void
store_vfp_regs (const struct regcache *regcache)
{
  gdb_byte regbuf[ARM_VFP3_REGS_SIZE];
  int ret, tid;
  struct gdbarch *gdbarch = regcache->arch ();
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  /* Get the thread id for the ptrace call.  */
  tid = regcache->ptid ().lwp ();

  if (have_ptrace_getregset == TRIBOOL_TRUE)
    {
      struct iovec iov;

      iov.iov_base = regbuf;
      iov.iov_len = ARM_VFP3_REGS_SIZE;
      ret = ptrace (PTRACE_GETREGSET, tid, NT_ARM_VFP, &iov);
    }
  else
    ret = ptrace (PTRACE_GETVFPREGS, tid, 0, regbuf);

  if (ret < 0)
    perror_with_name (_("Unable to fetch VFP registers (for update)"));

  aarch32_vfp_regcache_collect (regcache, regbuf,
				tdep->vfp_register_count);

  if (have_ptrace_getregset == TRIBOOL_TRUE)
    {
      struct iovec iov;

      iov.iov_base = regbuf;
      iov.iov_len = ARM_VFP3_REGS_SIZE;
      ret = ptrace (PTRACE_SETREGSET, tid, NT_ARM_VFP, &iov);
    }
  else
    ret = ptrace (PTRACE_SETVFPREGS, tid, 0, regbuf);

  if (ret < 0)
    perror_with_name (_("Unable to store VFP registers"));
}

/* Fetch registers from the child process.  Fetch all registers if
   regno == -1, otherwise fetch all general registers or all floating
   point registers depending upon the value of regno.  */

void
arm_linux_nat_target::fetch_registers (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (-1 == regno)
    {
      fetch_regs (regcache);
      if (tdep->have_wmmx_registers)
	fetch_wmmx_regs (regcache);
      if (tdep->vfp_register_count > 0)
	fetch_vfp_regs (regcache);
      if (tdep->have_fpa_registers)
	fetch_fpregs (regcache);
    }
  else
    {
      if (regno < ARM_F0_REGNUM || regno == ARM_PS_REGNUM)
	fetch_regs (regcache);
      else if (regno >= ARM_F0_REGNUM && regno <= ARM_FPS_REGNUM)
	fetch_fpregs (regcache);
      else if (tdep->have_wmmx_registers
	       && regno >= ARM_WR0_REGNUM && regno <= ARM_WCGR7_REGNUM)
	fetch_wmmx_regs (regcache);
      else if (tdep->vfp_register_count > 0
	       && regno >= ARM_D0_REGNUM
	       && (regno < ARM_D0_REGNUM + tdep->vfp_register_count
		   || regno == ARM_FPSCR_REGNUM))
	fetch_vfp_regs (regcache);
    }
}

/* Store registers back into the inferior.  Store all registers if
   regno == -1, otherwise store all general registers or all floating
   point registers depending upon the value of regno.  */

void
arm_linux_nat_target::store_registers (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  if (-1 == regno)
    {
      store_regs (regcache);
      if (tdep->have_wmmx_registers)
	store_wmmx_regs (regcache);
      if (tdep->vfp_register_count > 0)
	store_vfp_regs (regcache);
      if (tdep->have_fpa_registers)
	store_fpregs (regcache);
    }
  else
    {
      if (regno < ARM_F0_REGNUM || regno == ARM_PS_REGNUM)
	store_regs (regcache);
      else if ((regno >= ARM_F0_REGNUM) && (regno <= ARM_FPS_REGNUM))
	store_fpregs (regcache);
      else if (tdep->have_wmmx_registers
	       && regno >= ARM_WR0_REGNUM && regno <= ARM_WCGR7_REGNUM)
	store_wmmx_regs (regcache);
      else if (tdep->vfp_register_count > 0
	       && regno >= ARM_D0_REGNUM
	       && (regno < ARM_D0_REGNUM + tdep->vfp_register_count
		   || regno == ARM_FPSCR_REGNUM))
	store_vfp_regs (regcache);
    }
}

/* Wrapper functions for the standard regset handling, used by
   thread debugging.  */

void
fill_gregset (const struct regcache *regcache,	
	      gdb_gregset_t *gregsetp, int regno)
{
  arm_linux_collect_gregset (NULL, regcache, regno, gregsetp, 0);
}

void
supply_gregset (struct regcache *regcache, const gdb_gregset_t *gregsetp)
{
  arm_linux_supply_gregset (NULL, regcache, -1, gregsetp, 0);
}

void
fill_fpregset (const struct regcache *regcache,
	       gdb_fpregset_t *fpregsetp, int regno)
{
  arm_linux_collect_nwfpe (NULL, regcache, regno, fpregsetp, 0);
}

/* Fill GDB's register array with the floating-point register values
   in *fpregsetp.  */

void
supply_fpregset (struct regcache *regcache, const gdb_fpregset_t *fpregsetp)
{
  arm_linux_supply_nwfpe (NULL, regcache, -1, fpregsetp, 0);
}

/* Fetch the thread-local storage pointer for libthread_db.  */

ps_err_e
ps_get_thread_area (struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
  if (ptrace (PTRACE_GET_THREAD_AREA, lwpid, NULL, base) != 0)
    return PS_ERR;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  *base = (void *) ((char *)*base - idx);

  return PS_OK;
}

const struct target_desc *
arm_linux_nat_target::read_description ()
{
  if (inferior_ptid == null_ptid)
    return this->beneath ()->read_description ();

  CORE_ADDR arm_hwcap = linux_get_hwcap ();

  if (have_ptrace_getregset == TRIBOOL_UNKNOWN)
    {
      elf_gregset_t gpregs;
      struct iovec iov;
      int tid = inferior_ptid.pid ();

      iov.iov_base = &gpregs;
      iov.iov_len = sizeof (gpregs);

      /* Check if PTRACE_GETREGSET works.  */
      if (ptrace (PTRACE_GETREGSET, tid, NT_PRSTATUS, &iov) < 0)
	have_ptrace_getregset = TRIBOOL_FALSE;
      else
	have_ptrace_getregset = TRIBOOL_TRUE;
    }

  if (arm_hwcap & HWCAP_IWMMXT)
    return arm_read_description (ARM_FP_TYPE_IWMMXT, false);

  if (arm_hwcap & HWCAP_VFP)
    {
      /* Make sure that the kernel supports reading VFP registers.  Support was
	 added in 2.6.30.  */
      int pid = inferior_ptid.pid ();
      errno = 0;
      char *buf = (char *) alloca (ARM_VFP3_REGS_SIZE);
      if (ptrace (PTRACE_GETVFPREGS, pid, 0, buf) < 0 && errno == EIO)
	return nullptr;

      /* NEON implies VFPv3-D32 or no-VFP unit.  Say that we only support
	 Neon with VFPv3-D32.  */
      if (arm_hwcap & HWCAP_NEON)
	return aarch32_read_description ();
      else if ((arm_hwcap & (HWCAP_VFPv3 | HWCAP_VFPv3D16)) == HWCAP_VFPv3)
	return arm_read_description (ARM_FP_TYPE_VFPV3, false);

      return arm_read_description (ARM_FP_TYPE_VFPV2, false);
    }

  return this->beneath ()->read_description ();
}

/* Information describing the hardware breakpoint capabilities.  */
struct arm_linux_hwbp_cap
{
  gdb_byte arch;
  gdb_byte max_wp_length;
  gdb_byte wp_count;
  gdb_byte bp_count;
};

/* Since we cannot dynamically allocate subfields of arm_linux_process_info,
   assume a maximum number of supported break-/watchpoints.  */
#define MAX_BPTS 16
#define MAX_WPTS 16

/* Get hold of the Hardware Breakpoint information for the target we are
   attached to.  Returns NULL if the kernel doesn't support Hardware 
   breakpoints at all, or a pointer to the information structure.  */
static const struct arm_linux_hwbp_cap *
arm_linux_get_hwbp_cap (void)
{
  /* The info structure we return.  */
  static struct arm_linux_hwbp_cap info;

  /* Is INFO in a good state?  -1 means that no attempt has been made to
     initialize INFO; 0 means an attempt has been made, but it failed; 1
     means INFO is in an initialized state.  */
  static int available = -1;

  if (available == -1)
    {
      int tid;
      unsigned int val;

      tid = inferior_ptid.lwp ();
      if (ptrace (PTRACE_GETHBPREGS, tid, 0, &val) < 0)
	available = 0;
      else
	{
	  info.arch = (gdb_byte)((val >> 24) & 0xff);
	  info.max_wp_length = (gdb_byte)((val >> 16) & 0xff);
	  info.wp_count = (gdb_byte)((val >> 8) & 0xff);
	  info.bp_count = (gdb_byte)(val & 0xff);

      if (info.wp_count > MAX_WPTS)
	{
	  warning (_("arm-linux-gdb supports %d hardware watchpoints but target \
		      supports %d"), MAX_WPTS, info.wp_count);
	  info.wp_count = MAX_WPTS;
	}

      if (info.bp_count > MAX_BPTS)
	{
	  warning (_("arm-linux-gdb supports %d hardware breakpoints but target \
		      supports %d"), MAX_BPTS, info.bp_count);
	  info.bp_count = MAX_BPTS;
	}
	  available = (info.arch != 0);
	}
    }

  return available == 1 ? &info : NULL;
}

/* How many hardware breakpoints are available?  */
static int
arm_linux_get_hw_breakpoint_count (void)
{
  const struct arm_linux_hwbp_cap *cap = arm_linux_get_hwbp_cap ();
  return cap != NULL ? cap->bp_count : 0;
}

/* How many hardware watchpoints are available?  */
static int
arm_linux_get_hw_watchpoint_count (void)
{
  const struct arm_linux_hwbp_cap *cap = arm_linux_get_hwbp_cap ();
  return cap != NULL ? cap->wp_count : 0;
}

/* Have we got a free break-/watch-point available for use?  Returns -1 if
   there is not an appropriate resource available, otherwise returns 1.  */
int
arm_linux_nat_target::can_use_hw_breakpoint (enum bptype type,
					     int cnt, int ot)
{
  if (type == bp_hardware_watchpoint || type == bp_read_watchpoint
      || type == bp_access_watchpoint || type == bp_watchpoint)
    {
      int count = arm_linux_get_hw_watchpoint_count ();

      if (count == 0)
	return 0;
      else if (cnt + ot > count)
	return -1;
    }
  else if (type == bp_hardware_breakpoint)
    {
      int count = arm_linux_get_hw_breakpoint_count ();

      if (count == 0)
	return 0;
      else if (cnt > count)
	return -1;
    }
  else
    gdb_assert_not_reached ("unknown breakpoint type");

  return 1;
}

/* Enum describing the different types of ARM hardware break-/watch-points.  */
typedef enum
{
  arm_hwbp_break = 0,
  arm_hwbp_load = 1,
  arm_hwbp_store = 2,
  arm_hwbp_access = 3
} arm_hwbp_type;

/* Type describing an ARM Hardware Breakpoint Control register value.  */
typedef unsigned int arm_hwbp_control_t;

/* Structure used to keep track of hardware break-/watch-points.  */
struct arm_linux_hw_breakpoint
{
  /* Address to break on, or being watched.  */
  unsigned int address;
  /* Control register for break-/watch- point.  */
  arm_hwbp_control_t control;
};

/* Structure containing arrays of per process hardware break-/watchpoints
   for caching address and control information.

   The Linux ptrace interface to hardware break-/watch-points presents the 
   values in a vector centred around 0 (which is used fo generic information).
   Positive indicies refer to breakpoint addresses/control registers, negative
   indices to watchpoint addresses/control registers.

   The Linux vector is indexed as follows:
      -((i << 1) + 2): Control register for watchpoint i.
      -((i << 1) + 1): Address register for watchpoint i.
		    0: Information register.
       ((i << 1) + 1): Address register for breakpoint i.
       ((i << 1) + 2): Control register for breakpoint i.

   This structure is used as a per-thread cache of the state stored by the 
   kernel, so that we don't need to keep calling into the kernel to find a 
   free breakpoint.

   We treat break-/watch-points with their enable bit clear as being deleted.
   */
struct arm_linux_debug_reg_state
{
  /* Hardware breakpoints for this process.  */
  struct arm_linux_hw_breakpoint bpts[MAX_BPTS];
  /* Hardware watchpoints for this process.  */
  struct arm_linux_hw_breakpoint wpts[MAX_WPTS];
};

/* Per-process arch-specific data we want to keep.  */
struct arm_linux_process_info
{
  /* Linked list.  */
  struct arm_linux_process_info *next;
  /* The process identifier.  */
  pid_t pid;
  /* Hardware break-/watchpoints state information.  */
  struct arm_linux_debug_reg_state state;

};

/* Per-thread arch-specific data we want to keep.  */
struct arch_lwp_info
{
  /* Non-zero if our copy differs from what's recorded in the thread.  */
  char bpts_changed[MAX_BPTS];
  char wpts_changed[MAX_WPTS];
};

static struct arm_linux_process_info *arm_linux_process_list = NULL;

/* Find process data for process PID.  */

static struct arm_linux_process_info *
arm_linux_find_process_pid (pid_t pid)
{
  struct arm_linux_process_info *proc;

  for (proc = arm_linux_process_list; proc; proc = proc->next)
    if (proc->pid == pid)
      return proc;

  return NULL;
}

/* Add process data for process PID.  Returns newly allocated info
   object.  */

static struct arm_linux_process_info *
arm_linux_add_process (pid_t pid)
{
  struct arm_linux_process_info *proc;

  proc = XCNEW (struct arm_linux_process_info);
  proc->pid = pid;

  proc->next = arm_linux_process_list;
  arm_linux_process_list = proc;

  return proc;
}

/* Get data specific info for process PID, creating it if necessary.
   Never returns NULL.  */

static struct arm_linux_process_info *
arm_linux_process_info_get (pid_t pid)
{
  struct arm_linux_process_info *proc;

  proc = arm_linux_find_process_pid (pid);
  if (proc == NULL)
    proc = arm_linux_add_process (pid);

  return proc;
}

/* Called whenever GDB is no longer debugging process PID.  It deletes
   data structures that keep track of debug register state.  */

void
arm_linux_nat_target::low_forget_process (pid_t pid)
{
  struct arm_linux_process_info *proc, **proc_link;

  proc = arm_linux_process_list;
  proc_link = &arm_linux_process_list;

  while (proc != NULL)
    {
      if (proc->pid == pid)
    {
      *proc_link = proc->next;

      xfree (proc);
      return;
    }

      proc_link = &proc->next;
      proc = *proc_link;
    }
}

/* Get hardware break-/watchpoint state for process PID.  */

static struct arm_linux_debug_reg_state *
arm_linux_get_debug_reg_state (pid_t pid)
{
  return &arm_linux_process_info_get (pid)->state;
}

/* Initialize an ARM hardware break-/watch-point control register value.
   BYTE_ADDRESS_SELECT is the mask of bytes to trigger on; HWBP_TYPE is the 
   type of break-/watch-point; ENABLE indicates whether the point is enabled.
   */
static arm_hwbp_control_t 
arm_hwbp_control_initialize (unsigned byte_address_select,
			     arm_hwbp_type hwbp_type,
			     int enable)
{
  gdb_assert ((byte_address_select & ~0xffU) == 0);
  gdb_assert (hwbp_type != arm_hwbp_break 
	      || ((byte_address_select & 0xfU) != 0));

  return (byte_address_select << 5) | (hwbp_type << 3) | (3 << 1) | enable;
}

/* Does the breakpoint control value CONTROL have the enable bit set?  */
static int
arm_hwbp_control_is_enabled (arm_hwbp_control_t control)
{
  return control & 0x1;
}

/* Change a breakpoint control word so that it is in the disabled state.  */
static arm_hwbp_control_t
arm_hwbp_control_disable (arm_hwbp_control_t control)
{
  return control & ~0x1;
}

/* Initialise the hardware breakpoint structure P.  The breakpoint will be
   enabled, and will point to the placed address of BP_TGT.  */
static void
arm_linux_hw_breakpoint_initialize (struct gdbarch *gdbarch,
				    struct bp_target_info *bp_tgt,
				    struct arm_linux_hw_breakpoint *p)
{
  unsigned mask;
  CORE_ADDR address = bp_tgt->placed_address = bp_tgt->reqstd_address;

  /* We have to create a mask for the control register which says which bits
     of the word pointed to by address to break on.  */
  if (arm_pc_is_thumb (gdbarch, address))
    {
      mask = 0x3;
      address &= ~1;
    }
  else
    {
      mask = 0xf;
      address &= ~3;
    }

  p->address = (unsigned int) address;
  p->control = arm_hwbp_control_initialize (mask, arm_hwbp_break, 1);
}

/* Get the ARM hardware breakpoint type from the TYPE value we're
   given when asked to set a watchpoint.  */
static arm_hwbp_type 
arm_linux_get_hwbp_type (enum target_hw_bp_type type)
{
  if (type == hw_read)
    return arm_hwbp_load;
  else if (type == hw_write)
    return arm_hwbp_store;
  else
    return arm_hwbp_access;
}

/* Initialize the hardware breakpoint structure P for a watchpoint at ADDR
   to LEN.  The type of watchpoint is given in RW.  */
static void
arm_linux_hw_watchpoint_initialize (CORE_ADDR addr, int len,
				    enum target_hw_bp_type type,
				    struct arm_linux_hw_breakpoint *p)
{
  const struct arm_linux_hwbp_cap *cap = arm_linux_get_hwbp_cap ();
  unsigned mask;

  gdb_assert (cap != NULL);
  gdb_assert (cap->max_wp_length != 0);

  mask = (1 << len) - 1;

  p->address = (unsigned int) addr;
  p->control = arm_hwbp_control_initialize (mask, 
					    arm_linux_get_hwbp_type (type), 1);
}

/* Are two break-/watch-points equal?  */
static int
arm_linux_hw_breakpoint_equal (const struct arm_linux_hw_breakpoint *p1,
			       const struct arm_linux_hw_breakpoint *p2)
{
  return p1->address == p2->address && p1->control == p2->control;
}

/* Callback to mark a watch-/breakpoint to be updated in all threads of
   the current process.  */

static int
update_registers_callback (struct lwp_info *lwp, int watch, int index)
{
  if (lwp->arch_private == NULL)
    lwp->arch_private = XCNEW (struct arch_lwp_info);

  /* The actual update is done later just before resuming the lwp,
     we just mark that the registers need updating.  */
  if (watch)
    lwp->arch_private->wpts_changed[index] = 1;
  else
    lwp->arch_private->bpts_changed[index] = 1;

  /* If the lwp isn't stopped, force it to momentarily pause, so
     we can update its breakpoint registers.  */
  if (!lwp->stopped)
    linux_stop_lwp (lwp);

  return 0;
}

/* Insert the hardware breakpoint (WATCHPOINT = 0) or watchpoint (WATCHPOINT
   =1) BPT for thread TID.  */
static void
arm_linux_insert_hw_breakpoint1 (const struct arm_linux_hw_breakpoint* bpt, 
				 int watchpoint)
{
  int pid;
  ptid_t pid_ptid;
  gdb_byte count, i;
  struct arm_linux_hw_breakpoint* bpts;

  pid = inferior_ptid.pid ();
  pid_ptid = ptid_t (pid);

  if (watchpoint)
    {
      count = arm_linux_get_hw_watchpoint_count ();
      bpts = arm_linux_get_debug_reg_state (pid)->wpts;
    }
  else
    {
      count = arm_linux_get_hw_breakpoint_count ();
      bpts = arm_linux_get_debug_reg_state (pid)->bpts;
    }

  for (i = 0; i < count; ++i)
    if (!arm_hwbp_control_is_enabled (bpts[i].control))
      {
	bpts[i] = *bpt;
	iterate_over_lwps (pid_ptid,
			   [=] (struct lwp_info *info)
			   {
			     return update_registers_callback (info, watchpoint,
							       i);
			   });
	break;
      }

  gdb_assert (i != count);
}

/* Remove the hardware breakpoint (WATCHPOINT = 0) or watchpoint
   (WATCHPOINT = 1) BPT for thread TID.  */
static void
arm_linux_remove_hw_breakpoint1 (const struct arm_linux_hw_breakpoint *bpt, 
				 int watchpoint)
{
  int pid;
  gdb_byte count, i;
  ptid_t pid_ptid;
  struct arm_linux_hw_breakpoint* bpts;

  pid = inferior_ptid.pid ();
  pid_ptid = ptid_t (pid);

  if (watchpoint)
    {
      count = arm_linux_get_hw_watchpoint_count ();
      bpts = arm_linux_get_debug_reg_state (pid)->wpts;
    }
  else
    {
      count = arm_linux_get_hw_breakpoint_count ();
      bpts = arm_linux_get_debug_reg_state (pid)->bpts;
    }

  for (i = 0; i < count; ++i)
    if (arm_linux_hw_breakpoint_equal (bpt, bpts + i))
      {
	bpts[i].control = arm_hwbp_control_disable (bpts[i].control);
	iterate_over_lwps (pid_ptid,
			   [=] (struct lwp_info *info)
			   {
			     return update_registers_callback (info, watchpoint,
							       i);
			   });
	break;
      }

  gdb_assert (i != count);
}

/* Insert a Hardware breakpoint.  */
int
arm_linux_nat_target::insert_hw_breakpoint (struct gdbarch *gdbarch,
					    struct bp_target_info *bp_tgt)
{
  struct arm_linux_hw_breakpoint p;

  if (arm_linux_get_hw_breakpoint_count () == 0)
    return -1;

  arm_linux_hw_breakpoint_initialize (gdbarch, bp_tgt, &p);

  arm_linux_insert_hw_breakpoint1 (&p, 0);

  return 0;
}

/* Remove a hardware breakpoint.  */
int
arm_linux_nat_target::remove_hw_breakpoint (struct gdbarch *gdbarch,
					    struct bp_target_info *bp_tgt)
{
  struct arm_linux_hw_breakpoint p;

  if (arm_linux_get_hw_breakpoint_count () == 0)
    return -1;

  arm_linux_hw_breakpoint_initialize (gdbarch, bp_tgt, &p);

  arm_linux_remove_hw_breakpoint1 (&p, 0);

  return 0;
}

/* Are we able to use a hardware watchpoint for the LEN bytes starting at 
   ADDR?  */
int
arm_linux_nat_target::region_ok_for_hw_watchpoint (CORE_ADDR addr, int len)
{
  const struct arm_linux_hwbp_cap *cap = arm_linux_get_hwbp_cap ();
  CORE_ADDR max_wp_length, aligned_addr;

  /* Can not set watchpoints for zero or negative lengths.  */
  if (len <= 0)
    return 0;

  /* Need to be able to use the ptrace interface.  */
  if (cap == NULL || cap->wp_count == 0)
    return 0;

  /* Test that the range [ADDR, ADDR + LEN) fits into the largest address
     range covered by a watchpoint.  */
  max_wp_length = (CORE_ADDR)cap->max_wp_length;
  aligned_addr = addr & ~(max_wp_length - 1);

  if (aligned_addr + max_wp_length < addr + len)
    return 0;

  /* The current ptrace interface can only handle watchpoints that are a
     power of 2.  */
  if ((len & (len - 1)) != 0)
    return 0;

  /* All tests passed so we must be able to set a watchpoint.  */
  return 1;
}

/* Insert a Hardware breakpoint.  */
int
arm_linux_nat_target::insert_watchpoint (CORE_ADDR addr, int len,
					 enum target_hw_bp_type rw,
					 struct expression *cond)
{
  struct arm_linux_hw_breakpoint p;

  if (arm_linux_get_hw_watchpoint_count () == 0)
    return -1;

  arm_linux_hw_watchpoint_initialize (addr, len, rw, &p);

  arm_linux_insert_hw_breakpoint1 (&p, 1);

  return 0;
}

/* Remove a hardware breakpoint.  */
int
arm_linux_nat_target::remove_watchpoint (CORE_ADDR addr,
					 int len, enum target_hw_bp_type rw,
					 struct expression *cond)
{
  struct arm_linux_hw_breakpoint p;

  if (arm_linux_get_hw_watchpoint_count () == 0)
    return -1;

  arm_linux_hw_watchpoint_initialize (addr, len, rw, &p);

  arm_linux_remove_hw_breakpoint1 (&p, 1);

  return 0;
}

/* What was the data address the target was stopped on accessing.  */
bool
arm_linux_nat_target::stopped_data_address (CORE_ADDR *addr_p)
{
  siginfo_t siginfo;
  int slot;

  if (!linux_nat_get_siginfo (inferior_ptid, &siginfo))
    return false;

  /* This must be a hardware breakpoint.  */
  if (siginfo.si_signo != SIGTRAP
      || (siginfo.si_code & 0xffff) != 0x0004 /* TRAP_HWBKPT */)
    return false;

  /* We must be able to set hardware watchpoints.  */
  if (arm_linux_get_hw_watchpoint_count () == 0)
    return 0;

  slot = siginfo.si_errno;

  /* If we are in a positive slot then we're looking at a breakpoint and not
     a watchpoint.  */
  if (slot >= 0)
    return false;

  *addr_p = (CORE_ADDR) (uintptr_t) siginfo.si_addr;
  return true;
}

/* Has the target been stopped by hitting a watchpoint?  */
bool
arm_linux_nat_target::stopped_by_watchpoint ()
{
  CORE_ADDR addr;
  return stopped_data_address (&addr);
}

bool
arm_linux_nat_target::watchpoint_addr_within_range (CORE_ADDR addr,
						    CORE_ADDR start,
						    int length)
{
  return start <= addr && start + length - 1 >= addr;
}

/* Handle thread creation.  We need to copy the breakpoints and watchpoints
   in the parent thread to the child thread.  */
void
arm_linux_nat_target::low_new_thread (struct lwp_info *lp)
{
  int i;
  struct arch_lwp_info *info = XCNEW (struct arch_lwp_info);

  /* Mark that all the hardware breakpoint/watchpoint register pairs
     for this thread need to be initialized.  */

  for (i = 0; i < MAX_BPTS; i++)
    {
      info->bpts_changed[i] = 1;
      info->wpts_changed[i] = 1;
    }

  lp->arch_private = info;
}

/* Function to call when a thread is being deleted.  */

void
arm_linux_nat_target::low_delete_thread (struct arch_lwp_info *arch_lwp)
{
  xfree (arch_lwp);
}

/* Called when resuming a thread.
   The hardware debug registers are updated when there is any change.  */

void
arm_linux_nat_target::low_prepare_to_resume (struct lwp_info *lwp)
{
  int pid, i;
  struct arm_linux_hw_breakpoint *bpts, *wpts;
  struct arch_lwp_info *arm_lwp_info = lwp->arch_private;

  pid = lwp->ptid.lwp ();
  bpts = arm_linux_get_debug_reg_state (lwp->ptid.pid ())->bpts;
  wpts = arm_linux_get_debug_reg_state (lwp->ptid.pid ())->wpts;

  /* NULL means this is the main thread still going through the shell,
     or, no watchpoint has been set yet.  In that case, there's
     nothing to do.  */
  if (arm_lwp_info == NULL)
    return;

  for (i = 0; i < arm_linux_get_hw_breakpoint_count (); i++)
    if (arm_lwp_info->bpts_changed[i])
      {
	errno = 0;
	if (arm_hwbp_control_is_enabled (bpts[i].control))
	  if (ptrace (PTRACE_SETHBPREGS, pid,
	      (PTRACE_TYPE_ARG3) ((i << 1) + 1), &bpts[i].address) < 0)
	    perror_with_name (_("Unexpected error setting breakpoint"));

	if (bpts[i].control != 0)
	  if (ptrace (PTRACE_SETHBPREGS, pid,
	      (PTRACE_TYPE_ARG3) ((i << 1) + 2), &bpts[i].control) < 0)
	    perror_with_name (_("Unexpected error setting breakpoint"));

	arm_lwp_info->bpts_changed[i] = 0;
      }

  for (i = 0; i < arm_linux_get_hw_watchpoint_count (); i++)
    if (arm_lwp_info->wpts_changed[i])
      {
	errno = 0;
	if (arm_hwbp_control_is_enabled (wpts[i].control))
	  if (ptrace (PTRACE_SETHBPREGS, pid,
	      (PTRACE_TYPE_ARG3) -((i << 1) + 1), &wpts[i].address) < 0)
	    perror_with_name (_("Unexpected error setting watchpoint"));

	if (wpts[i].control != 0)
	  if (ptrace (PTRACE_SETHBPREGS, pid,
	      (PTRACE_TYPE_ARG3) -((i << 1) + 2), &wpts[i].control) < 0)
	    perror_with_name (_("Unexpected error setting watchpoint"));

	arm_lwp_info->wpts_changed[i] = 0;
      }
}

/* linux_nat_new_fork hook.  */

void
arm_linux_nat_target::low_new_fork (struct lwp_info *parent, pid_t child_pid)
{
  pid_t parent_pid;
  struct arm_linux_debug_reg_state *parent_state;
  struct arm_linux_debug_reg_state *child_state;

  /* NULL means no watchpoint has ever been set in the parent.  In
     that case, there's nothing to do.  */
  if (parent->arch_private == NULL)
    return;

  /* GDB core assumes the child inherits the watchpoints/hw
     breakpoints of the parent, and will remove them all from the
     forked off process.  Copy the debug registers mirrors into the
     new process so that all breakpoints and watchpoints can be
     removed together.  */

  parent_pid = parent->ptid.pid ();
  parent_state = arm_linux_get_debug_reg_state (parent_pid);
  child_state = arm_linux_get_debug_reg_state (child_pid);
  *child_state = *parent_state;
}

void _initialize_arm_linux_nat ();
void
_initialize_arm_linux_nat ()
{
  /* Register the target.  */
  linux_target = &the_arm_linux_nat_target;
  add_inf_child_target (&the_arm_linux_nat_target);
}
