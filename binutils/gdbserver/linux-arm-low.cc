/* GNU/Linux/ARM specific low level interface, for the remote server for GDB.
   Copyright (C) 1995-2024 Free Software Foundation, Inc.

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

#include "server.h"
#include "linux-low.h"
#include "arch/arm.h"
#include "arch/arm-linux.h"
#include "arch/arm-get-next-pcs.h"
#include "linux-aarch32-low.h"
#include "linux-aarch32-tdesc.h"
#include "linux-arm-tdesc.h"
#include "gdbsupport/gdb-checked-static-cast.h"

#include <sys/uio.h>
/* Don't include elf.h if linux/elf.h got included by gdb_proc_service.h.
   On Bionic elf.h and linux/elf.h have conflicting definitions.  */
#ifndef ELFMAG0
#include <elf.h>
#endif
#include "nat/gdb_ptrace.h"
#include <signal.h>
#include <sys/syscall.h>

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 22
#endif

#ifndef PTRACE_GETWMMXREGS
# define PTRACE_GETWMMXREGS 18
# define PTRACE_SETWMMXREGS 19
#endif

#ifndef PTRACE_GETVFPREGS
# define PTRACE_GETVFPREGS 27
# define PTRACE_SETVFPREGS 28
#endif

#ifndef PTRACE_GETHBPREGS
#define PTRACE_GETHBPREGS 29
#define PTRACE_SETHBPREGS 30
#endif

/* Linux target op definitions for the ARM architecture.  */

class arm_target : public linux_process_target
{
public:

  const regs_info *get_regs_info () override;

  int breakpoint_kind_from_pc (CORE_ADDR *pcptr) override;

  int breakpoint_kind_from_current_state (CORE_ADDR *pcptr) override;

  const gdb_byte *sw_breakpoint_from_kind (int kind, int *size) override;

  bool supports_software_single_step () override;

  bool supports_z_point_type (char z_type) override;

  bool supports_hardware_single_step () override;

protected:

  void low_arch_setup () override;

  bool low_cannot_fetch_register (int regno) override;

  bool low_cannot_store_register (int regno) override;

  bool low_supports_breakpoints () override;

  CORE_ADDR low_get_pc (regcache *regcache) override;

  void low_set_pc (regcache *regcache, CORE_ADDR newpc) override;

  std::vector<CORE_ADDR> low_get_next_pcs (regcache *regcache) override;

  bool low_breakpoint_at (CORE_ADDR pc) override;

  int low_insert_point (raw_bkpt_type type, CORE_ADDR addr,
			int size, raw_breakpoint *bp) override;

  int low_remove_point (raw_bkpt_type type, CORE_ADDR addr,
			int size, raw_breakpoint *bp) override;

  bool low_stopped_by_watchpoint () override;

  CORE_ADDR low_stopped_data_address () override;

  arch_process_info *low_new_process () override;

  void low_delete_process (arch_process_info *info) override;

  void low_new_thread (lwp_info *) override;

  void low_delete_thread (arch_lwp_info *) override;

  void low_new_fork (process_info *parent, process_info *child) override;

  void low_prepare_to_resume (lwp_info *lwp) override;

  bool low_supports_catch_syscall () override;

  void low_get_syscall_trapinfo (regcache *regcache, int *sysno) override;
};

/* The singleton target ops object.  */

static arm_target the_arm_target;

bool
arm_target::low_supports_breakpoints ()
{
  return true;
}

CORE_ADDR
arm_target::low_get_pc (regcache *regcache)
{
  return linux_get_pc_32bit (regcache);
}

void
arm_target::low_set_pc (regcache *regcache, CORE_ADDR pc)
{
  linux_set_pc_32bit (regcache, pc);
}

int
arm_target::breakpoint_kind_from_pc (CORE_ADDR *pcptr)
{
  return arm_breakpoint_kind_from_pc (pcptr);
}

int
arm_target::breakpoint_kind_from_current_state (CORE_ADDR *pcptr)
{
  return arm_breakpoint_kind_from_current_state (pcptr);
}

const gdb_byte *
arm_target::sw_breakpoint_from_kind (int kind, int *size)
{
  return arm_sw_breakpoint_from_kind (kind, size);
}

bool
arm_target::low_breakpoint_at (CORE_ADDR pc)
{
  return arm_breakpoint_at (pc);
}

/* Information describing the hardware breakpoint capabilities.  */
static struct
{
  unsigned char arch;
  unsigned char max_wp_length;
  unsigned char wp_count;
  unsigned char bp_count;
} arm_linux_hwbp_cap;

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

/* Since we cannot dynamically allocate subfields of arch_process_info,
   assume a maximum number of supported break-/watchpoints.  */
#define MAX_BPTS 32
#define MAX_WPTS 32

/* Per-process arch-specific data we want to keep.  */
struct arch_process_info
{
  /* Hardware breakpoints for this process.  */
  struct arm_linux_hw_breakpoint bpts[MAX_BPTS];
  /* Hardware watchpoints for this process.  */
  struct arm_linux_hw_breakpoint wpts[MAX_WPTS];
};

/* Per-thread arch-specific data we want to keep.  */
struct arch_lwp_info
{
  /* Non-zero if our copy differs from what's recorded in the thread.  */
  char bpts_changed[MAX_BPTS];
  char wpts_changed[MAX_WPTS];
  /* Cached stopped data address.  */
  CORE_ADDR stopped_data_address;
};

/* These are in <asm/elf.h> in current kernels.  */
#define HWCAP_VFP       64
#define HWCAP_IWMMXT    512
#define HWCAP_NEON      4096
#define HWCAP_VFPv3     8192
#define HWCAP_VFPv3D16  16384

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#define arm_num_regs 26

static int arm_regmap[] = {
  0, 4, 8, 12, 16, 20, 24, 28,
  32, 36, 40, 44, 48, 52, 56, 60,
  -1, -1, -1, -1, -1, -1, -1, -1, -1,
  64
};

/* Forward declarations needed for get_next_pcs ops.  */
static ULONGEST get_next_pcs_read_memory_unsigned_integer (CORE_ADDR memaddr,
							   int len,
							   int byte_order);

static CORE_ADDR get_next_pcs_addr_bits_remove (struct arm_get_next_pcs *self,
						CORE_ADDR val);

static CORE_ADDR get_next_pcs_syscall_next_pc (struct arm_get_next_pcs *self);

static int get_next_pcs_is_thumb (struct arm_get_next_pcs *self);

/* get_next_pcs operations.  */
static struct arm_get_next_pcs_ops get_next_pcs_ops = {
  get_next_pcs_read_memory_unsigned_integer,
  get_next_pcs_syscall_next_pc,
  get_next_pcs_addr_bits_remove,
  get_next_pcs_is_thumb,
  arm_linux_get_next_pcs_fixup,
};

bool
arm_target::low_cannot_store_register (int regno)
{
  return (regno >= arm_num_regs);
}

bool
arm_target::low_cannot_fetch_register (int regno)
{
  return (regno >= arm_num_regs);
}

static void
arm_fill_wmmxregset (struct regcache *regcache, void *buf)
{
  if (arm_linux_get_tdesc_fp_type (regcache->tdesc) != ARM_FP_TYPE_IWMMXT)
    return;

  for (int i = 0; i < 16; i++)
    collect_register (regcache, arm_num_regs + i, (char *) buf + i * 8);

  /* We only have access to wcssf, wcasf, and wcgr0-wcgr3.  */
  for (int i = 0; i < 6; i++)
    collect_register (regcache, arm_num_regs + i + 16,
		      (char *) buf + 16 * 8 + i * 4);
}

static void
arm_store_wmmxregset (struct regcache *regcache, const void *buf)
{
  if (arm_linux_get_tdesc_fp_type (regcache->tdesc) != ARM_FP_TYPE_IWMMXT)
    return;

  for (int i = 0; i < 16; i++)
    supply_register (regcache, arm_num_regs + i, (char *) buf + i * 8);

  /* We only have access to wcssf, wcasf, and wcgr0-wcgr3.  */
  for (int i = 0; i < 6; i++)
    supply_register (regcache, arm_num_regs + i + 16,
		     (char *) buf + 16 * 8 + i * 4);
}

static void
arm_fill_vfpregset (struct regcache *regcache, void *buf)
{
  int num;

  if (is_aarch32_linux_description (regcache->tdesc))
    num = 32;
  else
    {
      arm_fp_type fp_type = arm_linux_get_tdesc_fp_type (regcache->tdesc);

      if (fp_type == ARM_FP_TYPE_VFPV3)
	num = 32;
      else if (fp_type == ARM_FP_TYPE_VFPV2)
	num = 16;
      else
	return;
    }

  arm_fill_vfpregset_num (regcache, buf, num);
}

/* Wrapper of UNMAKE_THUMB_ADDR for get_next_pcs.  */
static CORE_ADDR
get_next_pcs_addr_bits_remove (struct arm_get_next_pcs *self, CORE_ADDR val)
{
  return UNMAKE_THUMB_ADDR (val);
}

static void
arm_store_vfpregset (struct regcache *regcache, const void *buf)
{
  int num;

  if (is_aarch32_linux_description (regcache->tdesc))
    num = 32;
  else
    {
      arm_fp_type fp_type = arm_linux_get_tdesc_fp_type (regcache->tdesc);

      if (fp_type == ARM_FP_TYPE_VFPV3)
	num = 32;
      else if (fp_type == ARM_FP_TYPE_VFPV2)
	num = 16;
      else
	return;
    }

  arm_store_vfpregset_num (regcache, buf, num);
}

/* Wrapper of arm_is_thumb_mode for get_next_pcs.  */
static int
get_next_pcs_is_thumb (struct arm_get_next_pcs *self)
{
  return arm_is_thumb_mode ();
}

/* Read memory from the inferior.
   BYTE_ORDER is ignored and there to keep compatiblity with GDB's
   read_memory_unsigned_integer. */
static ULONGEST
get_next_pcs_read_memory_unsigned_integer (CORE_ADDR memaddr,
					   int len,
					   int byte_order)
{
  ULONGEST res;

  res = 0;
  target_read_memory (memaddr, (unsigned char *) &res, len);

  return res;
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


/* Query Hardware Breakpoint information for the target we are attached to
   (using PID as ptrace argument) and set up arm_linux_hwbp_cap.  */
static void
arm_linux_init_hwbp_cap (int pid)
{
  unsigned int val;

  if (ptrace (PTRACE_GETHBPREGS, pid, 0, &val) < 0)
    return;

  arm_linux_hwbp_cap.arch = (unsigned char)((val >> 24) & 0xff);
  if (arm_linux_hwbp_cap.arch == 0)
    return;

  arm_linux_hwbp_cap.max_wp_length = (unsigned char)((val >> 16) & 0xff);
  arm_linux_hwbp_cap.wp_count = (unsigned char)((val >> 8) & 0xff);
  arm_linux_hwbp_cap.bp_count = (unsigned char)(val & 0xff);

  if (arm_linux_hwbp_cap.wp_count > MAX_WPTS)
    internal_error ("Unsupported number of watchpoints");
  if (arm_linux_hwbp_cap.bp_count > MAX_BPTS)
    internal_error ("Unsupported number of breakpoints");
}

/* How many hardware breakpoints are available?  */
static int
arm_linux_get_hw_breakpoint_count (void)
{
  return arm_linux_hwbp_cap.bp_count;
}

/* How many hardware watchpoints are available?  */
static int
arm_linux_get_hw_watchpoint_count (void)
{
  return arm_linux_hwbp_cap.wp_count;
}

/* Maximum length of area watched by hardware watchpoint.  */
static int
arm_linux_get_hw_watchpoint_max_length (void)
{
  return arm_linux_hwbp_cap.max_wp_length;
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

/* Is the breakpoint control value CONTROL initialized?  */
static int
arm_hwbp_control_is_initialized (arm_hwbp_control_t control)
{
  return control != 0;
}

/* Change a breakpoint control word so that it is in the disabled state.  */
static arm_hwbp_control_t
arm_hwbp_control_disable (arm_hwbp_control_t control)
{
  return control & ~0x1;
}

/* Are two break-/watch-points equal?  */
static int
arm_linux_hw_breakpoint_equal (const struct arm_linux_hw_breakpoint *p1,
			       const struct arm_linux_hw_breakpoint *p2)
{
  return p1->address == p2->address && p1->control == p2->control;
}

/* Convert a raw breakpoint type to an enum arm_hwbp_type.  */

static arm_hwbp_type
raw_bkpt_type_to_arm_hwbp_type (enum raw_bkpt_type raw_type)
{
  switch (raw_type)
    {
    case raw_bkpt_type_hw:
      return arm_hwbp_break;
    case raw_bkpt_type_write_wp:
      return arm_hwbp_store;
    case raw_bkpt_type_read_wp:
      return arm_hwbp_load;
    case raw_bkpt_type_access_wp:
      return arm_hwbp_access;
    default:
      gdb_assert_not_reached ("unhandled raw type");
    }
}

/* Initialize the hardware breakpoint structure P for a breakpoint or
   watchpoint at ADDR to LEN.  The type of watchpoint is given in TYPE.
   Returns -1 if TYPE is unsupported, or -2 if the particular combination
   of ADDR and LEN cannot be implemented.  Otherwise, returns 0 if TYPE
   represents a breakpoint and 1 if type represents a watchpoint.  */
static int
arm_linux_hw_point_initialize (enum raw_bkpt_type raw_type, CORE_ADDR addr,
			       int len, struct arm_linux_hw_breakpoint *p)
{
  arm_hwbp_type hwbp_type;
  unsigned mask;

  hwbp_type = raw_bkpt_type_to_arm_hwbp_type (raw_type);

  if (hwbp_type == arm_hwbp_break)
    {
      /* For breakpoints, the length field encodes the mode.  */
      switch (len)
	{
	case 2:	 /* 16-bit Thumb mode breakpoint */
	case 3:  /* 32-bit Thumb mode breakpoint */
	  mask = 0x3;
	  addr &= ~1;
	  break;
	case 4:  /* 32-bit ARM mode breakpoint */
	  mask = 0xf;
	  addr &= ~3;
	  break;
	default:
	  /* Unsupported. */
	  return -2;
	}
    }
  else
    {
      CORE_ADDR max_wp_length = arm_linux_get_hw_watchpoint_max_length ();
      CORE_ADDR aligned_addr;

      /* Can not set watchpoints for zero or negative lengths.  */
      if (len <= 0)
	return -2;
      /* The current ptrace interface can only handle watchpoints that are a
	 power of 2.  */
      if ((len & (len - 1)) != 0)
	return -2;

      /* Test that the range [ADDR, ADDR + LEN) fits into the largest address
	 range covered by a watchpoint.  */
      aligned_addr = addr & ~(max_wp_length - 1);
      if (aligned_addr + max_wp_length < addr + len)
	return -2;

      mask = (1 << len) - 1;
    }

  p->address = (unsigned int) addr;
  p->control = arm_hwbp_control_initialize (mask, hwbp_type, 1);

  return hwbp_type != arm_hwbp_break;
}

/* Callback to mark a watch-/breakpoint to be updated in all threads of
   the current process.  */

static void
update_registers_callback (thread_info *thread, int watch, int i)
{
  struct lwp_info *lwp = get_thread_lwp (thread);

  /* The actual update is done later just before resuming the lwp,
     we just mark that the registers need updating.  */
  if (watch)
    lwp->arch_private->wpts_changed[i] = 1;
  else
    lwp->arch_private->bpts_changed[i] = 1;

  /* If the lwp isn't stopped, force it to momentarily pause, so
     we can update its breakpoint registers.  */
  if (!lwp->stopped)
    linux_stop_lwp (lwp);
}

bool
arm_target::supports_z_point_type (char z_type)
{
  switch (z_type)
    {
    case Z_PACKET_SW_BP:
    case Z_PACKET_HW_BP:
    case Z_PACKET_WRITE_WP:
    case Z_PACKET_READ_WP:
    case Z_PACKET_ACCESS_WP:
      return true;
    default:
      /* Leave the handling of sw breakpoints with the gdb client.  */
      return false;
    }
}

/* Insert hardware break-/watchpoint.  */
int
arm_target::low_insert_point (raw_bkpt_type type, CORE_ADDR addr,
			      int len, raw_breakpoint *bp)
{
  struct process_info *proc = current_process ();
  struct arm_linux_hw_breakpoint p, *pts;
  int watch, i, count;

  watch = arm_linux_hw_point_initialize (type, addr, len, &p);
  if (watch < 0)
    {
      /* Unsupported.  */
      return watch == -1 ? 1 : -1;
    }

  if (watch)
    {
      count = arm_linux_get_hw_watchpoint_count ();
      pts = proc->priv->arch_private->wpts;
    }
  else
    {
      count = arm_linux_get_hw_breakpoint_count ();
      pts = proc->priv->arch_private->bpts;
    }

  for (i = 0; i < count; i++)
    if (!arm_hwbp_control_is_enabled (pts[i].control))
      {
	pts[i] = p;

	/* Only update the threads of the current process.  */
	for_each_thread (current_thread->id.pid (), [&] (thread_info *thread)
	  {
	    update_registers_callback (thread, watch, i);
	  });

	return 0;
      }

  /* We're out of watchpoints.  */
  return -1;
}

/* Remove hardware break-/watchpoint.  */
int
arm_target::low_remove_point (raw_bkpt_type type, CORE_ADDR addr,
			      int len, raw_breakpoint *bp)
{
  struct process_info *proc = current_process ();
  struct arm_linux_hw_breakpoint p, *pts;
  int watch, i, count;

  watch = arm_linux_hw_point_initialize (type, addr, len, &p);
  if (watch < 0)
    {
      /* Unsupported.  */
      return -1;
    }

  if (watch)
    {
      count = arm_linux_get_hw_watchpoint_count ();
      pts = proc->priv->arch_private->wpts;
    }
  else
    {
      count = arm_linux_get_hw_breakpoint_count ();
      pts = proc->priv->arch_private->bpts;
    }

  for (i = 0; i < count; i++)
    if (arm_linux_hw_breakpoint_equal (&p, pts + i))
      {
	pts[i].control = arm_hwbp_control_disable (pts[i].control);

	/* Only update the threads of the current process.  */
	for_each_thread (current_thread->id.pid (), [&] (thread_info *thread)
	  {
	    update_registers_callback (thread, watch, i);
	  });

	return 0;
      }

  /* No watchpoint matched.  */
  return -1;
}

/* Return whether current thread is stopped due to a watchpoint.  */
bool
arm_target::low_stopped_by_watchpoint ()
{
  struct lwp_info *lwp = get_thread_lwp (current_thread);
  siginfo_t siginfo;

  /* We must be able to set hardware watchpoints.  */
  if (arm_linux_get_hw_watchpoint_count () == 0)
    return false;

  /* Retrieve siginfo.  */
  errno = 0;
  ptrace (PTRACE_GETSIGINFO, lwpid_of (current_thread), 0, &siginfo);
  if (errno != 0)
    return false;

  /* This must be a hardware breakpoint.  */
  if (siginfo.si_signo != SIGTRAP
      || (siginfo.si_code & 0xffff) != 0x0004 /* TRAP_HWBKPT */)
    return false;

  /* If we are in a positive slot then we're looking at a breakpoint and not
     a watchpoint.  */
  if (siginfo.si_errno >= 0)
    return false;

  /* Cache stopped data address for use by arm_stopped_data_address.  */
  lwp->arch_private->stopped_data_address
    = (CORE_ADDR) (uintptr_t) siginfo.si_addr;

  return true;
}

/* Return data address that triggered watchpoint.  Called only if
   low_stopped_by_watchpoint returned true.  */
CORE_ADDR
arm_target::low_stopped_data_address ()
{
  struct lwp_info *lwp = get_thread_lwp (current_thread);
  return lwp->arch_private->stopped_data_address;
}

/* Called when a new process is created.  */
arch_process_info *
arm_target::low_new_process ()
{
  struct arch_process_info *info = XCNEW (struct arch_process_info);
  return info;
}

/* Called when a process is being deleted.  */

void
arm_target::low_delete_process (arch_process_info *info)
{
  xfree (info);
}

/* Called when a new thread is detected.  */
void
arm_target::low_new_thread (lwp_info *lwp)
{
  struct arch_lwp_info *info = XCNEW (struct arch_lwp_info);
  int i;

  for (i = 0; i < MAX_BPTS; i++)
    info->bpts_changed[i] = 1;
  for (i = 0; i < MAX_WPTS; i++)
    info->wpts_changed[i] = 1;

  lwp->arch_private = info;
}

/* Function to call when a thread is being deleted.  */

void
arm_target::low_delete_thread (arch_lwp_info *arch_lwp)
{
  xfree (arch_lwp);
}

void
arm_target::low_new_fork (process_info *parent, process_info *child)
{
  struct arch_process_info *parent_proc_info;
  struct arch_process_info *child_proc_info;
  struct lwp_info *child_lwp;
  struct arch_lwp_info *child_lwp_info;
  int i;

  /* These are allocated by linux_add_process.  */
  gdb_assert (parent->priv != NULL
	      && parent->priv->arch_private != NULL);
  gdb_assert (child->priv != NULL
	      && child->priv->arch_private != NULL);

  parent_proc_info = parent->priv->arch_private;
  child_proc_info = child->priv->arch_private;

  /* Linux kernel before 2.6.33 commit
     72f674d203cd230426437cdcf7dd6f681dad8b0d
     will inherit hardware debug registers from parent
     on fork/vfork/clone.  Newer Linux kernels create such tasks with
     zeroed debug registers.

     GDB core assumes the child inherits the watchpoints/hw
     breakpoints of the parent, and will remove them all from the
     forked off process.  Copy the debug registers mirrors into the
     new process so that all breakpoints and watchpoints can be
     removed together.  The debug registers mirror will become zeroed
     in the end before detaching the forked off process, thus making
     this compatible with older Linux kernels too.  */

  *child_proc_info = *parent_proc_info;

  /* Mark all the hardware breakpoints and watchpoints as changed to
     make sure that the registers will be updated.  */
  child_lwp = find_lwp_pid (ptid_t (child->pid));
  child_lwp_info = child_lwp->arch_private;
  for (i = 0; i < MAX_BPTS; i++)
    child_lwp_info->bpts_changed[i] = 1;
  for (i = 0; i < MAX_WPTS; i++)
    child_lwp_info->wpts_changed[i] = 1;
}

/* Called when resuming a thread.
   If the debug regs have changed, update the thread's copies.  */
void
arm_target::low_prepare_to_resume (lwp_info *lwp)
{
  struct thread_info *thread = get_lwp_thread (lwp);
  int pid = lwpid_of (thread);
  struct process_info *proc = find_process_pid (pid_of (thread));
  struct arch_process_info *proc_info = proc->priv->arch_private;
  struct arch_lwp_info *lwp_info = lwp->arch_private;
  int i;

  for (i = 0; i < arm_linux_get_hw_breakpoint_count (); i++)
    if (lwp_info->bpts_changed[i])
      {
	errno = 0;

	if (arm_hwbp_control_is_enabled (proc_info->bpts[i].control))
	  if (ptrace (PTRACE_SETHBPREGS, pid,
		      (PTRACE_TYPE_ARG3) ((i << 1) + 1),
		      &proc_info->bpts[i].address) < 0)
	    perror_with_name ("Unexpected error setting breakpoint address");

	if (arm_hwbp_control_is_initialized (proc_info->bpts[i].control))
	  if (ptrace (PTRACE_SETHBPREGS, pid,
		      (PTRACE_TYPE_ARG3) ((i << 1) + 2),
		      &proc_info->bpts[i].control) < 0)
	    perror_with_name ("Unexpected error setting breakpoint");

	lwp_info->bpts_changed[i] = 0;
      }

  for (i = 0; i < arm_linux_get_hw_watchpoint_count (); i++)
    if (lwp_info->wpts_changed[i])
      {
	errno = 0;

	if (arm_hwbp_control_is_enabled (proc_info->wpts[i].control))
	  if (ptrace (PTRACE_SETHBPREGS, pid,
		      (PTRACE_TYPE_ARG3) -((i << 1) + 1),
		      &proc_info->wpts[i].address) < 0)
	    perror_with_name ("Unexpected error setting watchpoint address");

	if (arm_hwbp_control_is_initialized (proc_info->wpts[i].control))
	  if (ptrace (PTRACE_SETHBPREGS, pid,
		      (PTRACE_TYPE_ARG3) -((i << 1) + 2),
		      &proc_info->wpts[i].control) < 0)
	    perror_with_name ("Unexpected error setting watchpoint");

	lwp_info->wpts_changed[i] = 0;
      }
}

/* Find the next pc for a sigreturn or rt_sigreturn syscall.  In
   addition, set IS_THUMB depending on whether we will return to ARM
   or Thumb code.
   See arm-linux.h for stack layout details.  */
static CORE_ADDR
arm_sigreturn_next_pc (struct regcache *regcache, int svc_number,
		       int *is_thumb)
{
  unsigned long sp;
  unsigned long sp_data;
  /* Offset of PC register.  */
  int pc_offset = 0;
  CORE_ADDR next_pc = 0;
  uint32_t cpsr;

  gdb_assert (svc_number == __NR_sigreturn || svc_number == __NR_rt_sigreturn);

  collect_register_by_name (regcache, "sp", &sp);
  the_target->read_memory (sp, (unsigned char *) &sp_data, 4);

  pc_offset = arm_linux_sigreturn_next_pc_offset
    (sp, sp_data, svc_number, __NR_sigreturn == svc_number ? 1 : 0);

  the_target->read_memory (sp + pc_offset, (unsigned char *) &next_pc, 4);

  /* Set IS_THUMB according the CPSR saved on the stack.  */
  the_target->read_memory (sp + pc_offset + 4, (unsigned char *) &cpsr, 4);
  *is_thumb = ((cpsr & CPSR_T) != 0);

  return next_pc;
}

/* When PC is at a syscall instruction, return the PC of the next
   instruction to be executed.  */
static CORE_ADDR
get_next_pcs_syscall_next_pc (struct arm_get_next_pcs *self)
{
  CORE_ADDR next_pc = 0;
  CORE_ADDR pc = regcache_read_pc (self->regcache);
  int is_thumb = arm_is_thumb_mode ();
  ULONGEST svc_number = 0;
  regcache *regcache
    = gdb::checked_static_cast<struct regcache *> (self->regcache);

  if (is_thumb)
    {
      collect_register (regcache, 7, &svc_number);
      next_pc = pc + 2;
    }
  else
    {
      unsigned long this_instr;
      unsigned long svc_operand;

      target_read_memory (pc, (unsigned char *) &this_instr, 4);
      svc_operand = (0x00ffffff & this_instr);

      if (svc_operand)  /* OABI.  */
	{
	  svc_number = svc_operand - 0x900000;
	}
      else /* EABI.  */
	{
	  collect_register (regcache, 7, &svc_number);
	}

      next_pc = pc + 4;
    }

  /* This is a sigreturn or sigreturn_rt syscall.  */
  if (svc_number == __NR_sigreturn || svc_number == __NR_rt_sigreturn)
    {
      /* SIGRETURN or RT_SIGRETURN may affect the arm thumb mode, so
	 update IS_THUMB.   */
      next_pc = arm_sigreturn_next_pc (regcache, svc_number, &is_thumb);
    }

  /* Addresses for calling Thumb functions have the bit 0 set.  */
  if (is_thumb)
    next_pc = MAKE_THUMB_ADDR (next_pc);

  return next_pc;
}

static const struct target_desc *
arm_read_description (void)
{
  unsigned long arm_hwcap = linux_get_hwcap (current_thread->id.pid (), 4);

  if (arm_hwcap & HWCAP_IWMMXT)
    return arm_linux_read_description (ARM_FP_TYPE_IWMMXT);

  if (arm_hwcap & HWCAP_VFP)
    {
      /* Make sure that the kernel supports reading VFP registers.  Support was
	 added in 2.6.30.  */
      int pid = lwpid_of (current_thread);
      errno = 0;
      char *buf = (char *) alloca (ARM_VFP3_REGS_SIZE);
      if (ptrace (PTRACE_GETVFPREGS, pid, 0, buf) < 0 && errno == EIO)
	return arm_linux_read_description (ARM_FP_TYPE_NONE);

      /* NEON implies either no VFP, or VFPv3-D32.  We only support
	 it with VFP.  */
      if (arm_hwcap & HWCAP_NEON)
	return aarch32_linux_read_description ();
      else if ((arm_hwcap & (HWCAP_VFPv3 | HWCAP_VFPv3D16)) == HWCAP_VFPv3)
	return arm_linux_read_description (ARM_FP_TYPE_VFPV3);
      else
	return arm_linux_read_description (ARM_FP_TYPE_VFPV2);
    }

  /* The default configuration uses legacy FPA registers, probably
     simulated.  */
  return arm_linux_read_description (ARM_FP_TYPE_NONE);
}

void
arm_target::low_arch_setup ()
{
  int tid = lwpid_of (current_thread);
  int gpregs[18];
  struct iovec iov;

  /* Query hardware watchpoint/breakpoint capabilities.  */
  arm_linux_init_hwbp_cap (tid);

  current_process ()->tdesc = arm_read_description ();

  iov.iov_base = gpregs;
  iov.iov_len = sizeof (gpregs);

  /* Check if PTRACE_GETREGSET works.  */
  if (ptrace (PTRACE_GETREGSET, tid, NT_PRSTATUS, &iov) == 0)
    have_ptrace_getregset = 1;
  else
    have_ptrace_getregset = 0;
}

bool
arm_target::supports_software_single_step ()
{
  return true;
}

/* Fetch the next possible PCs after the current instruction executes.  */

std::vector<CORE_ADDR>
arm_target::low_get_next_pcs (regcache *regcache)
{
  struct arm_get_next_pcs next_pcs_ctx;

  arm_get_next_pcs_ctor (&next_pcs_ctx,
			 &get_next_pcs_ops,
			 /* Byte order is ignored assumed as host.  */
			 0,
			 0,
			 1,
			 regcache);

  return arm_get_next_pcs (&next_pcs_ctx);
}

/* Support for hardware single step.  */

bool
arm_target::supports_hardware_single_step ()
{
  return false;
}

bool
arm_target::low_supports_catch_syscall ()
{
  return true;
}

/* Implementation of linux target ops method "low_get_syscall_trapinfo".  */

void
arm_target::low_get_syscall_trapinfo (regcache *regcache, int *sysno)
{
  if (arm_is_thumb_mode ())
    collect_register_by_name (regcache, "r7", sysno);
  else
    {
      unsigned long pc;
      unsigned long insn;

      collect_register_by_name (regcache, "pc", &pc);

      if (read_memory (pc - 4, (unsigned char *) &insn, 4))
	*sysno = UNKNOWN_SYSCALL;
      else
	{
	  unsigned long svc_operand = (0x00ffffff & insn);

	  if (svc_operand)
	    {
	      /* OABI */
	      *sysno = svc_operand - 0x900000;
	    }
	  else
	    {
	      /* EABI */
	      collect_register_by_name (regcache, "r7", sysno);
	    }
	}
    }
}

/* Register sets without using PTRACE_GETREGSET.  */

static struct regset_info arm_regsets[] = {
  { PTRACE_GETREGS, PTRACE_SETREGS, 0,
    ARM_CORE_REGS_SIZE + ARM_INT_REGISTER_SIZE, GENERAL_REGS,
    arm_fill_gregset, arm_store_gregset },
  { PTRACE_GETWMMXREGS, PTRACE_SETWMMXREGS, 0, IWMMXT_REGS_SIZE, EXTENDED_REGS,
    arm_fill_wmmxregset, arm_store_wmmxregset },
  { PTRACE_GETVFPREGS, PTRACE_SETVFPREGS, 0, ARM_VFP3_REGS_SIZE, EXTENDED_REGS,
    arm_fill_vfpregset, arm_store_vfpregset },
  NULL_REGSET
};

static struct regsets_info arm_regsets_info =
  {
    arm_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

static struct usrregs_info arm_usrregs_info =
  {
    arm_num_regs,
    arm_regmap,
  };

static struct regs_info regs_info_arm =
  {
    NULL, /* regset_bitmap */
    &arm_usrregs_info,
    &arm_regsets_info
  };

const regs_info *
arm_target::get_regs_info ()
{
  const struct target_desc *tdesc = current_process ()->tdesc;

  if (have_ptrace_getregset == 1
      && (is_aarch32_linux_description (tdesc)
	  || arm_linux_get_tdesc_fp_type (tdesc) == ARM_FP_TYPE_VFPV3))
    return &regs_info_aarch32;

  return &regs_info_arm;
}

/* The linux target ops object.  */

linux_process_target *the_linux_target = &the_arm_target;

void
initialize_low_arch (void)
{
  initialize_low_arch_aarch32 ();
  initialize_regsets_info (&arm_regsets_info);
}
