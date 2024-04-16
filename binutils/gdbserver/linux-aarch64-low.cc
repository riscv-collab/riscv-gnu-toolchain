/* GNU/Linux/AArch64 specific low level interface, for the remote server for
   GDB.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.
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

#include "server.h"
#include "linux-low.h"
#include "nat/aarch64-linux.h"
#include "nat/aarch64-linux-hw-point.h"
#include "arch/aarch64-insn.h"
#include "linux-aarch32-low.h"
#include "elf/common.h"
#include "ax.h"
#include "tracepoint.h"
#include "debug.h"

#include <signal.h>
#include <sys/user.h>
#include "nat/gdb_ptrace.h"
#include <asm/ptrace.h>
#include <inttypes.h>
#include <endian.h>
#include <sys/uio.h>

#include "gdb_proc_service.h"
#include "arch/aarch64.h"
#include "arch/aarch64-mte-linux.h"
#include "arch/aarch64-scalable-linux.h"
#include "linux-aarch32-tdesc.h"
#include "linux-aarch64-tdesc.h"
#include "nat/aarch64-mte-linux-ptrace.h"
#include "nat/aarch64-scalable-linux-ptrace.h"
#include "tdesc.h"

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#ifdef HAVE_GETAUXVAL
#include <sys/auxv.h>
#endif

/* Linux target op definitions for the AArch64 architecture.  */

class aarch64_target : public linux_process_target
{
public:

  const regs_info *get_regs_info () override;

  int breakpoint_kind_from_pc (CORE_ADDR *pcptr) override;

  int breakpoint_kind_from_current_state (CORE_ADDR *pcptr) override;

  const gdb_byte *sw_breakpoint_from_kind (int kind, int *size) override;

  bool supports_z_point_type (char z_type) override;

  bool supports_tracepoints () override;

  bool supports_fast_tracepoints () override;

  int install_fast_tracepoint_jump_pad
    (CORE_ADDR tpoint, CORE_ADDR tpaddr, CORE_ADDR collector,
     CORE_ADDR lockaddr, ULONGEST orig_size, CORE_ADDR *jump_entry,
     CORE_ADDR *trampoline, ULONGEST *trampoline_size,
     unsigned char *jjump_pad_insn, ULONGEST *jjump_pad_insn_size,
     CORE_ADDR *adjusted_insn_addr, CORE_ADDR *adjusted_insn_addr_end,
     char *err) override;

  int get_min_fast_tracepoint_insn_len () override;

  struct emit_ops *emit_ops () override;

  bool supports_memory_tagging () override;

  bool fetch_memtags (CORE_ADDR address, size_t len,
		      gdb::byte_vector &tags, int type) override;

  bool store_memtags (CORE_ADDR address, size_t len,
		      const gdb::byte_vector &tags, int type) override;

protected:

  void low_arch_setup () override;

  bool low_cannot_fetch_register (int regno) override;

  bool low_cannot_store_register (int regno) override;

  bool low_supports_breakpoints () override;

  CORE_ADDR low_get_pc (regcache *regcache) override;

  void low_set_pc (regcache *regcache, CORE_ADDR newpc) override;

  bool low_breakpoint_at (CORE_ADDR pc) override;

  int low_insert_point (raw_bkpt_type type, CORE_ADDR addr,
			int size, raw_breakpoint *bp) override;

  int low_remove_point (raw_bkpt_type type, CORE_ADDR addr,
			int size, raw_breakpoint *bp) override;

  bool low_stopped_by_watchpoint () override;

  CORE_ADDR low_stopped_data_address () override;

  bool low_siginfo_fixup (siginfo_t *native, gdb_byte *inf,
			  int direction) override;

  arch_process_info *low_new_process () override;

  void low_delete_process (arch_process_info *info) override;

  void low_new_thread (lwp_info *) override;

  void low_delete_thread (arch_lwp_info *) override;

  void low_new_fork (process_info *parent, process_info *child) override;

  void low_prepare_to_resume (lwp_info *lwp) override;

  int low_get_thread_area (int lwpid, CORE_ADDR *addrp) override;

  bool low_supports_range_stepping () override;

  bool low_supports_catch_syscall () override;

  void low_get_syscall_trapinfo (regcache *regcache, int *sysno) override;
};

/* The singleton target ops object.  */

static aarch64_target the_aarch64_target;

bool
aarch64_target::low_cannot_fetch_register (int regno)
{
  gdb_assert_not_reached ("linux target op low_cannot_fetch_register "
			  "is not implemented by the target");
}

bool
aarch64_target::low_cannot_store_register (int regno)
{
  gdb_assert_not_reached ("linux target op low_cannot_store_register "
			  "is not implemented by the target");
}

void
aarch64_target::low_prepare_to_resume (lwp_info *lwp)
{
  aarch64_linux_prepare_to_resume (lwp);
}

/* Per-process arch-specific data we want to keep.  */

struct arch_process_info
{
  /* Hardware breakpoint/watchpoint data.
     The reason for them to be per-process rather than per-thread is
     due to the lack of information in the gdbserver environment;
     gdbserver is not told that whether a requested hardware
     breakpoint/watchpoint is thread specific or not, so it has to set
     each hw bp/wp for every thread in the current process.  The
     higher level bp/wp management in gdb will resume a thread if a hw
     bp/wp trap is not expected for it.  Since the hw bp/wp setting is
     same for each thread, it is reasonable for the data to live here.
     */
  struct aarch64_debug_reg_state debug_reg_state;
};

/* Return true if the size of register 0 is 8 byte.  */

static int
is_64bit_tdesc (void)
{
  /* We may not have a current thread at this point, so go straight to
     the process's target description.  */
  return register_size (current_process ()->tdesc, 0) == 8;
}

static void
aarch64_fill_gregset (struct regcache *regcache, void *buf)
{
  struct user_pt_regs *regset = (struct user_pt_regs *) buf;
  int i;

  for (i = 0; i < AARCH64_X_REGS_NUM; i++)
    collect_register (regcache, AARCH64_X0_REGNUM + i, &regset->regs[i]);
  collect_register (regcache, AARCH64_SP_REGNUM, &regset->sp);
  collect_register (regcache, AARCH64_PC_REGNUM, &regset->pc);
  collect_register (regcache, AARCH64_CPSR_REGNUM, &regset->pstate);
}

static void
aarch64_store_gregset (struct regcache *regcache, const void *buf)
{
  const struct user_pt_regs *regset = (const struct user_pt_regs *) buf;
  int i;

  for (i = 0; i < AARCH64_X_REGS_NUM; i++)
    supply_register (regcache, AARCH64_X0_REGNUM + i, &regset->regs[i]);
  supply_register (regcache, AARCH64_SP_REGNUM, &regset->sp);
  supply_register (regcache, AARCH64_PC_REGNUM, &regset->pc);
  supply_register (regcache, AARCH64_CPSR_REGNUM, &regset->pstate);
}

static void
aarch64_fill_fpregset (struct regcache *regcache, void *buf)
{
  struct user_fpsimd_state *regset = (struct user_fpsimd_state *) buf;
  int i;

  for (i = 0; i < AARCH64_V_REGS_NUM; i++)
    collect_register (regcache, AARCH64_V0_REGNUM + i, &regset->vregs[i]);
  collect_register (regcache, AARCH64_FPSR_REGNUM, &regset->fpsr);
  collect_register (regcache, AARCH64_FPCR_REGNUM, &regset->fpcr);
}

static void
aarch64_store_fpregset (struct regcache *regcache, const void *buf)
{
  const struct user_fpsimd_state *regset
    = (const struct user_fpsimd_state *) buf;
  int i;

  for (i = 0; i < AARCH64_V_REGS_NUM; i++)
    supply_register (regcache, AARCH64_V0_REGNUM + i, &regset->vregs[i]);
  supply_register (regcache, AARCH64_FPSR_REGNUM, &regset->fpsr);
  supply_register (regcache, AARCH64_FPCR_REGNUM, &regset->fpcr);
}

/* Store the pauth registers to regcache.  */

static void
aarch64_store_pauthregset (struct regcache *regcache, const void *buf)
{
  uint64_t *pauth_regset = (uint64_t *) buf;
  int pauth_base = find_regno (regcache->tdesc, "pauth_dmask");

  if (pauth_base == 0)
    return;

  supply_register (regcache, AARCH64_PAUTH_DMASK_REGNUM (pauth_base),
		   &pauth_regset[0]);
  supply_register (regcache, AARCH64_PAUTH_CMASK_REGNUM (pauth_base),
		   &pauth_regset[1]);
}

/* Fill BUF with the MTE registers from the regcache.  */

static void
aarch64_fill_mteregset (struct regcache *regcache, void *buf)
{
  uint64_t *mte_regset = (uint64_t *) buf;
  int mte_base = find_regno (regcache->tdesc, "tag_ctl");

  collect_register (regcache, mte_base, mte_regset);
}

/* Store the MTE registers to regcache.  */

static void
aarch64_store_mteregset (struct regcache *regcache, const void *buf)
{
  uint64_t *mte_regset = (uint64_t *) buf;
  int mte_base = find_regno (regcache->tdesc, "tag_ctl");

  /* Tag Control register */
  supply_register (regcache, mte_base, mte_regset);
}

/* Fill BUF with TLS register from the regcache.  */

static void
aarch64_fill_tlsregset (struct regcache *regcache, void *buf)
{
  gdb_byte *tls_buf = (gdb_byte *) buf;
  int tls_regnum  = find_regno (regcache->tdesc, "tpidr");

  collect_register (regcache, tls_regnum, tls_buf);

  /* Read TPIDR2, if it exists.  */
  std::optional<int> regnum = find_regno_no_throw (regcache->tdesc, "tpidr2");

  if (regnum.has_value ())
    collect_register (regcache, *regnum, tls_buf + sizeof (uint64_t));
}

/* Store TLS register to regcache.  */

static void
aarch64_store_tlsregset (struct regcache *regcache, const void *buf)
{
  gdb_byte *tls_buf = (gdb_byte *) buf;
  int tls_regnum  = find_regno (regcache->tdesc, "tpidr");

  supply_register (regcache, tls_regnum, tls_buf);

  /* Write TPIDR2, if it exists.  */
  std::optional<int> regnum = find_regno_no_throw (regcache->tdesc, "tpidr2");

  if (regnum.has_value ())
    supply_register (regcache, *regnum, tls_buf + sizeof (uint64_t));
}

bool
aarch64_target::low_supports_breakpoints ()
{
  return true;
}

/* Implementation of linux target ops method "low_get_pc".  */

CORE_ADDR
aarch64_target::low_get_pc (regcache *regcache)
{
  if (register_size (regcache->tdesc, 0) == 8)
    return linux_get_pc_64bit (regcache);
  else
    return linux_get_pc_32bit (regcache);
}

/* Implementation of linux target ops method "low_set_pc".  */

void
aarch64_target::low_set_pc (regcache *regcache, CORE_ADDR pc)
{
  if (register_size (regcache->tdesc, 0) == 8)
    linux_set_pc_64bit (regcache, pc);
  else
    linux_set_pc_32bit (regcache, pc);
}

#define aarch64_breakpoint_len 4

/* AArch64 BRK software debug mode instruction.
   This instruction needs to match gdb/aarch64-tdep.c
   (aarch64_default_breakpoint).  */
static const gdb_byte aarch64_breakpoint[] = {0x00, 0x00, 0x20, 0xd4};

/* Implementation of linux target ops method "low_breakpoint_at".  */

bool
aarch64_target::low_breakpoint_at (CORE_ADDR where)
{
  if (is_64bit_tdesc ())
    {
      gdb_byte insn[aarch64_breakpoint_len];

      read_memory (where, (unsigned char *) &insn, aarch64_breakpoint_len);
      if (memcmp (insn, aarch64_breakpoint, aarch64_breakpoint_len) == 0)
	return true;

      return false;
    }
  else
    return arm_breakpoint_at (where);
}

static void
aarch64_init_debug_reg_state (struct aarch64_debug_reg_state *state)
{
  int i;

  for (i = 0; i < AARCH64_HBP_MAX_NUM; ++i)
    {
      state->dr_addr_bp[i] = 0;
      state->dr_ctrl_bp[i] = 0;
      state->dr_ref_count_bp[i] = 0;
    }

  for (i = 0; i < AARCH64_HWP_MAX_NUM; ++i)
    {
      state->dr_addr_wp[i] = 0;
      state->dr_ctrl_wp[i] = 0;
      state->dr_ref_count_wp[i] = 0;
    }
}

/* Return the pointer to the debug register state structure in the
   current process' arch-specific data area.  */

struct aarch64_debug_reg_state *
aarch64_get_debug_reg_state (pid_t pid)
{
  struct process_info *proc = find_process_pid (pid);

  return &proc->priv->arch_private->debug_reg_state;
}

/* Implementation of target ops method "supports_z_point_type".  */

bool
aarch64_target::supports_z_point_type (char z_type)
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
      return false;
    }
}

/* Implementation of linux target ops method "low_insert_point".

   It actually only records the info of the to-be-inserted bp/wp;
   the actual insertion will happen when threads are resumed.  */

int
aarch64_target::low_insert_point (raw_bkpt_type type, CORE_ADDR addr,
				  int len, raw_breakpoint *bp)
{
  int ret;
  enum target_hw_bp_type targ_type;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (pid_of (current_thread));

  if (show_debug_regs)
    fprintf (stderr, "insert_point on entry (addr=0x%08lx, len=%d)\n",
	     (unsigned long) addr, len);

  /* Determine the type from the raw breakpoint type.  */
  targ_type = raw_bkpt_type_to_target_hw_bp_type (type);

  if (targ_type != hw_execute)
    {
      if (aarch64_region_ok_for_watchpoint (addr, len))
	ret = aarch64_handle_watchpoint (targ_type, addr, len,
					 1 /* is_insert */,
					 current_lwp_ptid (), state);
      else
	ret = -1;
    }
  else
    {
      if (len == 3)
	{
	  /* LEN is 3 means the breakpoint is set on a 32-bit thumb
	     instruction.   Set it to 2 to correctly encode length bit
	     mask in hardware/watchpoint control register.  */
	  len = 2;
	}
      ret = aarch64_handle_breakpoint (targ_type, addr, len,
				       1 /* is_insert */, current_lwp_ptid (),
				       state);
    }

  if (show_debug_regs)
    aarch64_show_debug_reg_state (state, "insert_point", addr, len,
				  targ_type);

  return ret;
}

/* Implementation of linux target ops method "low_remove_point".

   It actually only records the info of the to-be-removed bp/wp,
   the actual removal will be done when threads are resumed.  */

int
aarch64_target::low_remove_point (raw_bkpt_type type, CORE_ADDR addr,
				  int len, raw_breakpoint *bp)
{
  int ret;
  enum target_hw_bp_type targ_type;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (pid_of (current_thread));

  if (show_debug_regs)
    fprintf (stderr, "remove_point on entry (addr=0x%08lx, len=%d)\n",
	     (unsigned long) addr, len);

  /* Determine the type from the raw breakpoint type.  */
  targ_type = raw_bkpt_type_to_target_hw_bp_type (type);

  /* Set up state pointers.  */
  if (targ_type != hw_execute)
    ret =
      aarch64_handle_watchpoint (targ_type, addr, len, 0 /* is_insert */,
				 current_lwp_ptid (), state);
  else
    {
      if (len == 3)
	{
	  /* LEN is 3 means the breakpoint is set on a 32-bit thumb
	     instruction.   Set it to 2 to correctly encode length bit
	     mask in hardware/watchpoint control register.  */
	  len = 2;
	}
      ret = aarch64_handle_breakpoint (targ_type, addr, len,
				       0 /* is_insert */,  current_lwp_ptid (),
				       state);
    }

  if (show_debug_regs)
    aarch64_show_debug_reg_state (state, "remove_point", addr, len,
				  targ_type);

  return ret;
}

static CORE_ADDR
aarch64_remove_non_address_bits (CORE_ADDR pointer)
{
  /* By default, we assume TBI and discard the top 8 bits plus the
     VA range select bit (55).  */
  CORE_ADDR mask = AARCH64_TOP_BITS_MASK;

  /* Check if PAC is available for this target.  */
  if (tdesc_contains_feature (current_process ()->tdesc,
			      "org.gnu.gdb.aarch64.pauth"))
    {
      /* Fetch the PAC masks.  These masks are per-process, so we can just
	 fetch data from whatever thread we have at the moment.

	 Also, we have both a code mask and a data mask.  For now they are the
	 same, but this may change in the future.  */

      struct regcache *regs = get_thread_regcache (current_thread, 1);
      CORE_ADDR dmask = regcache_raw_get_unsigned_by_name (regs, "pauth_dmask");
      CORE_ADDR cmask = regcache_raw_get_unsigned_by_name (regs, "pauth_cmask");
      mask |= aarch64_mask_from_pac_registers (cmask, dmask);
    }

  return aarch64_remove_top_bits (pointer, mask);
}

/* Implementation of linux target ops method "low_stopped_data_address".  */

CORE_ADDR
aarch64_target::low_stopped_data_address ()
{
  siginfo_t siginfo;
  int pid, i;
  struct aarch64_debug_reg_state *state;

  pid = lwpid_of (current_thread);

  /* Get the siginfo.  */
  if (ptrace (PTRACE_GETSIGINFO, pid, NULL, &siginfo) != 0)
    return (CORE_ADDR) 0;

  /* Need to be a hardware breakpoint/watchpoint trap.  */
  if (siginfo.si_signo != SIGTRAP
      || (siginfo.si_code & 0xffff) != 0x0004 /* TRAP_HWBKPT */)
    return (CORE_ADDR) 0;

  /* Make sure to ignore the top byte, otherwise we may not recognize a
     hardware watchpoint hit.  The stopped data addresses coming from the
     kernel can potentially be tagged addresses.  */
  const CORE_ADDR addr_trap
    = aarch64_remove_non_address_bits ((CORE_ADDR) siginfo.si_addr);

  /* Check if the address matches any watched address.  */
  state = aarch64_get_debug_reg_state (pid_of (current_thread));
  for (i = aarch64_num_wp_regs - 1; i >= 0; --i)
    {
      const unsigned int offset
	= aarch64_watchpoint_offset (state->dr_ctrl_wp[i]);
      const unsigned int len = aarch64_watchpoint_length (state->dr_ctrl_wp[i]);
      const CORE_ADDR addr_watch = state->dr_addr_wp[i] + offset;
      const CORE_ADDR addr_watch_aligned = align_down (state->dr_addr_wp[i], 8);
      const CORE_ADDR addr_orig = state->dr_addr_orig_wp[i];

      if (state->dr_ref_count_wp[i]
	  && DR_CONTROL_ENABLED (state->dr_ctrl_wp[i])
	  && addr_trap >= addr_watch_aligned
	  && addr_trap < addr_watch + len)
	{
	  /* ADDR_TRAP reports the first address of the memory range
	     accessed by the CPU, regardless of what was the memory
	     range watched.  Thus, a large CPU access that straddles
	     the ADDR_WATCH..ADDR_WATCH+LEN range may result in an
	     ADDR_TRAP that is lower than the
	     ADDR_WATCH..ADDR_WATCH+LEN range.  E.g.:

	     addr: |   4   |   5   |   6   |   7   |   8   |
				   |---- range watched ----|
		   |----------- range accessed ------------|

	     In this case, ADDR_TRAP will be 4.

	     To match a watchpoint known to GDB core, we must never
	     report *ADDR_P outside of any ADDR_WATCH..ADDR_WATCH+LEN
	     range.  ADDR_WATCH <= ADDR_TRAP < ADDR_ORIG is a false
	     positive on kernels older than 4.10.  See PR
	     external/20207.  */
	  return addr_orig;
	}
    }

  return (CORE_ADDR) 0;
}

/* Implementation of linux target ops method "low_stopped_by_watchpoint".  */

bool
aarch64_target::low_stopped_by_watchpoint ()
{
  return (low_stopped_data_address () != 0);
}

/* Fetch the thread-local storage pointer for libthread_db.  */

ps_err_e
ps_get_thread_area (struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
  return aarch64_ps_get_thread_area (ph, lwpid, idx, base,
				     is_64bit_tdesc ());
}

/* Implementation of linux target ops method "low_siginfo_fixup".  */

bool
aarch64_target::low_siginfo_fixup (siginfo_t *native, gdb_byte *inf,
				   int direction)
{
  /* Is the inferior 32-bit?  If so, then fixup the siginfo object.  */
  if (!is_64bit_tdesc ())
    {
      if (direction == 0)
	aarch64_compat_siginfo_from_siginfo ((struct compat_siginfo *) inf,
					     native);
      else
	aarch64_siginfo_from_compat_siginfo (native,
					     (struct compat_siginfo *) inf);

      return true;
    }

  return false;
}

/* Implementation of linux target ops method "low_new_process".  */

arch_process_info *
aarch64_target::low_new_process ()
{
  struct arch_process_info *info = XCNEW (struct arch_process_info);

  aarch64_init_debug_reg_state (&info->debug_reg_state);

  return info;
}

/* Implementation of linux target ops method "low_delete_process".  */

void
aarch64_target::low_delete_process (arch_process_info *info)
{
  xfree (info);
}

void
aarch64_target::low_new_thread (lwp_info *lwp)
{
  aarch64_linux_new_thread (lwp);
}

void
aarch64_target::low_delete_thread (arch_lwp_info *arch_lwp)
{
  aarch64_linux_delete_thread (arch_lwp);
}

/* Implementation of linux target ops method "low_new_fork".  */

void
aarch64_target::low_new_fork (process_info *parent,
			      process_info *child)
{
  /* These are allocated by linux_add_process.  */
  gdb_assert (parent->priv != NULL
	      && parent->priv->arch_private != NULL);
  gdb_assert (child->priv != NULL
	      && child->priv->arch_private != NULL);

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

  *child->priv->arch_private = *parent->priv->arch_private;
}

/* Wrapper for aarch64_sve_regs_copy_to_reg_buf.  */

static void
aarch64_sve_regs_copy_to_regcache (struct regcache *regcache,
				   ATTRIBUTE_UNUSED const void *buf)
{
  /* BUF is unused here since we collect the data straight from a ptrace
     request in aarch64_sve_regs_copy_to_reg_buf, therefore bypassing
     gdbserver's own call to ptrace.  */

  int tid = lwpid_of (current_thread);

  /* Update the register cache.  aarch64_sve_regs_copy_to_reg_buf handles
     fetching the NT_ARM_SVE state from thread TID.  */
  aarch64_sve_regs_copy_to_reg_buf (tid, regcache);
}

/* Wrapper for aarch64_sve_regs_copy_from_reg_buf.  */

static void
aarch64_sve_regs_copy_from_regcache (struct regcache *regcache, void *buf)
{
  int tid = lwpid_of (current_thread);

  /* Update the thread SVE state.  aarch64_sve_regs_copy_from_reg_buf
     handles writing the SVE/FPSIMD state back to thread TID.  */
  aarch64_sve_regs_copy_from_reg_buf (tid, regcache);

  /* We need to return the expected data in BUF, so copy whatever the kernel
     already has to BUF.  */
  gdb::byte_vector sve_state = aarch64_fetch_sve_regset (tid);
  memcpy (buf, sve_state.data (), sve_state.size ());
}

/* Wrapper for aarch64_za_regs_copy_to_reg_buf, to help copying NT_ARM_ZA
   state from the thread (BUF) to the register cache.  */

static void
aarch64_za_regs_copy_to_regcache (struct regcache *regcache,
				  ATTRIBUTE_UNUSED const void *buf)
{
  /* BUF is unused here since we collect the data straight from a ptrace
     request, therefore bypassing gdbserver's own call to ptrace.  */
  int tid = lwpid_of (current_thread);

  int za_regnum = find_regno (regcache->tdesc, "za");
  int svg_regnum = find_regno (regcache->tdesc, "svg");
  int svcr_regnum = find_regno (regcache->tdesc, "svcr");

  /* Update the register cache.  aarch64_za_regs_copy_to_reg_buf handles
     fetching the NT_ARM_ZA state from thread TID.  */
  aarch64_za_regs_copy_to_reg_buf (tid, regcache, za_regnum, svg_regnum,
				   svcr_regnum);
}

/* Wrapper for aarch64_za_regs_copy_from_reg_buf, to help copying NT_ARM_ZA
   state from the register cache to the thread (BUF).  */

static void
aarch64_za_regs_copy_from_regcache (struct regcache *regcache, void *buf)
{
  int tid = lwpid_of (current_thread);

  int za_regnum = find_regno (regcache->tdesc, "za");
  int svg_regnum = find_regno (regcache->tdesc, "svg");
  int svcr_regnum = find_regno (regcache->tdesc, "svcr");

  /* Update the thread NT_ARM_ZA state.  aarch64_za_regs_copy_from_reg_buf
     handles writing the ZA state back to thread TID.  */
  aarch64_za_regs_copy_from_reg_buf (tid, regcache, za_regnum, svg_regnum,
				     svcr_regnum);

  /* We need to return the expected data in BUF, so copy whatever the kernel
     already has to BUF.  */

  /* Obtain a dump of ZA from ptrace.  */
  gdb::byte_vector za_state = aarch64_fetch_za_regset (tid);
  memcpy (buf, za_state.data (), za_state.size ());
}

/* Wrapper for aarch64_zt_regs_copy_to_reg_buf, to help copying NT_ARM_ZT
   state from the thread (BUF) to the register cache.  */

static void
aarch64_zt_regs_copy_to_regcache (struct regcache *regcache,
				  ATTRIBUTE_UNUSED const void *buf)
{
  /* BUF is unused here since we collect the data straight from a ptrace
     request, therefore bypassing gdbserver's own call to ptrace.  */
  int tid = lwpid_of (current_thread);

  int zt_regnum = find_regno (regcache->tdesc, "zt0");

  /* Update the register cache.  aarch64_zt_regs_copy_to_reg_buf handles
     fetching the NT_ARM_ZT state from thread TID.  */
  aarch64_zt_regs_copy_to_reg_buf (tid, regcache, zt_regnum);
}

/* Wrapper for aarch64_zt_regs_copy_from_reg_buf, to help copying NT_ARM_ZT
   state from the register cache to the thread (BUF).  */

static void
aarch64_zt_regs_copy_from_regcache (struct regcache *regcache, void *buf)
{
  int tid = lwpid_of (current_thread);

  int zt_regnum = find_regno (regcache->tdesc, "zt0");

  /* Update the thread NT_ARM_ZT state.  aarch64_zt_regs_copy_from_reg_buf
     handles writing the ZT state back to thread TID.  */
  aarch64_zt_regs_copy_from_reg_buf (tid, regcache, zt_regnum);

  /* We need to return the expected data in BUF, so copy whatever the kernel
     already has to BUF.  */

  /* Obtain a dump of NT_ARM_ZT from ptrace.  */
  gdb::byte_vector zt_state = aarch64_fetch_zt_regset (tid);
  memcpy (buf, zt_state.data (), zt_state.size ());
}

/* Array containing all the possible register sets for AArch64/Linux.  During
   architecture setup, these will be checked against the HWCAP/HWCAP2 bits for
   validity and enabled/disabled accordingly.

   Their sizes are set to 0 here, but they will be adjusted later depending
   on whether each register set is available or not.  */
static struct regset_info aarch64_regsets[] =
{
  /* GPR registers.  */
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PRSTATUS,
    0, GENERAL_REGS,
    aarch64_fill_gregset, aarch64_store_gregset },
  /* Floating Point (FPU) registers.  */
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_FPREGSET,
    0, FP_REGS,
    aarch64_fill_fpregset, aarch64_store_fpregset
  },
  /* Scalable Vector Extension (SVE) registers.  */
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_ARM_SVE,
    0, EXTENDED_REGS,
    aarch64_sve_regs_copy_from_regcache, aarch64_sve_regs_copy_to_regcache
  },
  /* Scalable Matrix Extension (SME) ZA register.  */
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_ARM_ZA,
    0, EXTENDED_REGS,
    aarch64_za_regs_copy_from_regcache, aarch64_za_regs_copy_to_regcache
  },
  /* Scalable Matrix Extension 2 (SME2) ZT registers.  */
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_ARM_ZT,
    0, EXTENDED_REGS,
    aarch64_zt_regs_copy_from_regcache, aarch64_zt_regs_copy_to_regcache
  },
  /* PAC registers.  */
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_ARM_PAC_MASK,
    0, OPTIONAL_REGS,
    nullptr, aarch64_store_pauthregset },
  /* Tagged address control / MTE registers.  */
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_ARM_TAGGED_ADDR_CTRL,
    0, OPTIONAL_REGS,
    aarch64_fill_mteregset, aarch64_store_mteregset },
  /* TLS register.  */
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_ARM_TLS,
    0, OPTIONAL_REGS,
    aarch64_fill_tlsregset, aarch64_store_tlsregset },
  NULL_REGSET
};

static struct regsets_info aarch64_regsets_info =
  {
    aarch64_regsets, /* regsets */
    0, /* num_regsets */
    nullptr, /* disabled_regsets */
  };

static struct regs_info regs_info_aarch64 =
  {
    nullptr, /* regset_bitmap */
    nullptr, /* usrregs */
    &aarch64_regsets_info,
  };

/* Given FEATURES, adjust the available register sets by setting their
   sizes.  A size of 0 means the register set is disabled and won't be
   used.  */

static void
aarch64_adjust_register_sets (const struct aarch64_features &features)
{
  struct regset_info *regset;

  for (regset = aarch64_regsets; regset->size >= 0; regset++)
    {
      switch (regset->nt_type)
	{
	case NT_PRSTATUS:
	  /* General purpose registers are always present.  */
	  regset->size = sizeof (struct user_pt_regs);
	  break;
	case NT_FPREGSET:
	  /* This is unavailable when SVE is present.  */
	  if (features.vq == 0)
	    regset->size = sizeof (struct user_fpsimd_state);
	  break;
	case NT_ARM_SVE:
	  if (features.vq > 0)
	    regset->size = SVE_PT_SIZE (AARCH64_MAX_SVE_VQ, SVE_PT_REGS_SVE);
	  break;
	case NT_ARM_PAC_MASK:
	  if (features.pauth)
	    regset->size = AARCH64_PAUTH_REGS_SIZE;
	  break;
	case NT_ARM_TAGGED_ADDR_CTRL:
	  if (features.mte)
	    regset->size = AARCH64_LINUX_SIZEOF_MTE;
	  break;
	case NT_ARM_TLS:
	  if (features.tls > 0)
	    regset->size = AARCH64_TLS_REGISTER_SIZE * features.tls;
	  break;
	case NT_ARM_ZA:
	  if (features.svq > 0)
	    regset->size = ZA_PT_SIZE (features.svq);
	  break;
	case NT_ARM_ZT:
	  if (features.sme2)
	    regset->size = AARCH64_SME2_ZT0_SIZE;
	  break;
	default:
	  gdb_assert_not_reached ("Unknown register set found.");
	}
    }
}

/* Matches HWCAP_PACA in kernel header arch/arm64/include/uapi/asm/hwcap.h.  */
#define AARCH64_HWCAP_PACA (1 << 30)

/* Implementation of linux target ops method "low_arch_setup".  */

void
aarch64_target::low_arch_setup ()
{
  unsigned int machine;
  int is_elf64;
  int tid;

  tid = lwpid_of (current_thread);

  is_elf64 = linux_pid_exe_is_elf_64_file (tid, &machine);

  if (is_elf64)
    {
      struct aarch64_features features;
      int pid = current_thread->id.pid ();

      features.vq = aarch64_sve_get_vq (tid);
      /* A-profile PAC is 64-bit only.  */
      features.pauth = linux_get_hwcap (pid, 8) & AARCH64_HWCAP_PACA;
      /* A-profile MTE is 64-bit only.  */
      features.mte = linux_get_hwcap2 (pid, 8) & HWCAP2_MTE;
      features.tls = aarch64_tls_register_count (tid);

      /* Scalable Matrix Extension feature and size check.  */
      if (linux_get_hwcap2 (pid, 8) & HWCAP2_SME)
	features.svq = aarch64_za_get_svq (tid);

      /* Scalable Matrix Extension 2 feature check.  */
      CORE_ADDR hwcap2 = linux_get_hwcap2 (pid, 8);
      if ((hwcap2 & HWCAP2_SME2) || (hwcap2 & HWCAP2_SME2P1))
	{
	  /* Make sure ptrace supports NT_ARM_ZT.  */
	  features.sme2 = supports_zt_registers (tid);
	}

      current_process ()->tdesc = aarch64_linux_read_description (features);

      /* Adjust the register sets we should use for this particular set of
	 features.  */
      aarch64_adjust_register_sets (features);
    }
  else
    current_process ()->tdesc = aarch32_linux_read_description ();

  aarch64_linux_get_debug_reg_capacity (lwpid_of (current_thread));
}

/* Implementation of linux target ops method "get_regs_info".  */

const regs_info *
aarch64_target::get_regs_info ()
{
  if (!is_64bit_tdesc ())
    return &regs_info_aarch32;

  /* AArch64 64-bit registers.  */
  return &regs_info_aarch64;
}

/* Implementation of target ops method "supports_tracepoints".  */

bool
aarch64_target::supports_tracepoints ()
{
  if (current_thread == NULL)
    return true;
  else
    {
      /* We don't support tracepoints on aarch32 now.  */
      return is_64bit_tdesc ();
    }
}

/* Implementation of linux target ops method "low_get_thread_area".  */

int
aarch64_target::low_get_thread_area (int lwpid, CORE_ADDR *addrp)
{
  struct iovec iovec;
  uint64_t reg;

  iovec.iov_base = &reg;
  iovec.iov_len = sizeof (reg);

  if (ptrace (PTRACE_GETREGSET, lwpid, NT_ARM_TLS, &iovec) != 0)
    return -1;

  *addrp = reg;

  return 0;
}

bool
aarch64_target::low_supports_catch_syscall ()
{
  return true;
}

/* Implementation of linux target ops method "low_get_syscall_trapinfo".  */

void
aarch64_target::low_get_syscall_trapinfo (regcache *regcache, int *sysno)
{
  int use_64bit = register_size (regcache->tdesc, 0) == 8;

  if (use_64bit)
    {
      long l_sysno;

      collect_register_by_name (regcache, "x8", &l_sysno);
      *sysno = (int) l_sysno;
    }
  else
    collect_register_by_name (regcache, "r7", sysno);
}

/* List of condition codes that we need.  */

enum aarch64_condition_codes
{
  EQ = 0x0,
  NE = 0x1,
  LO = 0x3,
  GE = 0xa,
  LT = 0xb,
  GT = 0xc,
  LE = 0xd,
};

enum aarch64_operand_type
{
  OPERAND_IMMEDIATE,
  OPERAND_REGISTER,
};

/* Representation of an operand.  At this time, it only supports register
   and immediate types.  */

struct aarch64_operand
{
  /* Type of the operand.  */
  enum aarch64_operand_type type;

  /* Value of the operand according to the type.  */
  union
    {
      uint32_t imm;
      struct aarch64_register reg;
    };
};

/* List of registers that we are currently using, we can add more here as
   we need to use them.  */

/* General purpose scratch registers (64 bit).  */
static const struct aarch64_register x0 = { 0, 1 };
static const struct aarch64_register x1 = { 1, 1 };
static const struct aarch64_register x2 = { 2, 1 };
static const struct aarch64_register x3 = { 3, 1 };
static const struct aarch64_register x4 = { 4, 1 };

/* General purpose scratch registers (32 bit).  */
static const struct aarch64_register w0 = { 0, 0 };
static const struct aarch64_register w2 = { 2, 0 };

/* Intra-procedure scratch registers.  */
static const struct aarch64_register ip0 = { 16, 1 };

/* Special purpose registers.  */
static const struct aarch64_register fp = { 29, 1 };
static const struct aarch64_register lr = { 30, 1 };
static const struct aarch64_register sp = { 31, 1 };
static const struct aarch64_register xzr = { 31, 1 };

/* Dynamically allocate a new register.  If we know the register
   statically, we should make it a global as above instead of using this
   helper function.  */

static struct aarch64_register
aarch64_register (unsigned num, int is64)
{
  return (struct aarch64_register) { num, is64 };
}

/* Helper function to create a register operand, for instructions with
   different types of operands.

   For example:
   p += emit_mov (p, x0, register_operand (x1));  */

static struct aarch64_operand
register_operand (struct aarch64_register reg)
{
  struct aarch64_operand operand;

  operand.type = OPERAND_REGISTER;
  operand.reg = reg;

  return operand;
}

/* Helper function to create an immediate operand, for instructions with
   different types of operands.

   For example:
   p += emit_mov (p, x0, immediate_operand (12));  */

static struct aarch64_operand
immediate_operand (uint32_t imm)
{
  struct aarch64_operand operand;

  operand.type = OPERAND_IMMEDIATE;
  operand.imm = imm;

  return operand;
}

/* Helper function to create an offset memory operand.

   For example:
   p += emit_ldr (p, x0, sp, offset_memory_operand (16));  */

static struct aarch64_memory_operand
offset_memory_operand (int32_t offset)
{
  return (struct aarch64_memory_operand) { MEMORY_OPERAND_OFFSET, offset };
}

/* Helper function to create a pre-index memory operand.

   For example:
   p += emit_ldr (p, x0, sp, preindex_memory_operand (16));  */

static struct aarch64_memory_operand
preindex_memory_operand (int32_t index)
{
  return (struct aarch64_memory_operand) { MEMORY_OPERAND_PREINDEX, index };
}

/* Helper function to create a post-index memory operand.

   For example:
   p += emit_ldr (p, x0, sp, postindex_memory_operand (16));  */

static struct aarch64_memory_operand
postindex_memory_operand (int32_t index)
{
  return (struct aarch64_memory_operand) { MEMORY_OPERAND_POSTINDEX, index };
}

/* System control registers.  These special registers can be written and
   read with the MRS and MSR instructions.

   - NZCV: Condition flags.  GDB refers to this register under the CPSR
	   name.
   - FPSR: Floating-point status register.
   - FPCR: Floating-point control registers.
   - TPIDR_EL0: Software thread ID register.  */

enum aarch64_system_control_registers
{
  /*          op0           op1           crn          crm          op2  */
  NZCV =      (0x1 << 14) | (0x3 << 11) | (0x4 << 7) | (0x2 << 3) | 0x0,
  FPSR =      (0x1 << 14) | (0x3 << 11) | (0x4 << 7) | (0x4 << 3) | 0x1,
  FPCR =      (0x1 << 14) | (0x3 << 11) | (0x4 << 7) | (0x4 << 3) | 0x0,
  TPIDR_EL0 = (0x1 << 14) | (0x3 << 11) | (0xd << 7) | (0x0 << 3) | 0x2
};

/* Write a BLR instruction into *BUF.

     BLR rn

   RN is the register to branch to.  */

static int
emit_blr (uint32_t *buf, struct aarch64_register rn)
{
  return aarch64_emit_insn (buf, BLR | ENCODE (rn.num, 5, 5));
}

/* Write a RET instruction into *BUF.

     RET xn

   RN is the register to branch to.  */

static int
emit_ret (uint32_t *buf, struct aarch64_register rn)
{
  return aarch64_emit_insn (buf, RET | ENCODE (rn.num, 5, 5));
}

static int
emit_load_store_pair (uint32_t *buf, enum aarch64_opcodes opcode,
		      struct aarch64_register rt,
		      struct aarch64_register rt2,
		      struct aarch64_register rn,
		      struct aarch64_memory_operand operand)
{
  uint32_t opc;
  uint32_t pre_index;
  uint32_t write_back;

  if (rt.is64)
    opc = ENCODE (2, 2, 30);
  else
    opc = ENCODE (0, 2, 30);

  switch (operand.type)
    {
    case MEMORY_OPERAND_OFFSET:
      {
	pre_index = ENCODE (1, 1, 24);
	write_back = ENCODE (0, 1, 23);
	break;
      }
    case MEMORY_OPERAND_POSTINDEX:
      {
	pre_index = ENCODE (0, 1, 24);
	write_back = ENCODE (1, 1, 23);
	break;
      }
    case MEMORY_OPERAND_PREINDEX:
      {
	pre_index = ENCODE (1, 1, 24);
	write_back = ENCODE (1, 1, 23);
	break;
      }
    default:
      return 0;
    }

  return aarch64_emit_insn (buf, opcode | opc | pre_index | write_back
			    | ENCODE (operand.index >> 3, 7, 15)
			    | ENCODE (rt2.num, 5, 10)
			    | ENCODE (rn.num, 5, 5) | ENCODE (rt.num, 5, 0));
}

/* Write a STP instruction into *BUF.

     STP rt, rt2, [rn, #offset]
     STP rt, rt2, [rn, #index]!
     STP rt, rt2, [rn], #index

   RT and RT2 are the registers to store.
   RN is the base address register.
   OFFSET is the immediate to add to the base address.  It is limited to a
   -512 .. 504 range (7 bits << 3).  */

static int
emit_stp (uint32_t *buf, struct aarch64_register rt,
	  struct aarch64_register rt2, struct aarch64_register rn,
	  struct aarch64_memory_operand operand)
{
  return emit_load_store_pair (buf, STP, rt, rt2, rn, operand);
}

/* Write a LDP instruction into *BUF.

     LDP rt, rt2, [rn, #offset]
     LDP rt, rt2, [rn, #index]!
     LDP rt, rt2, [rn], #index

   RT and RT2 are the registers to store.
   RN is the base address register.
   OFFSET is the immediate to add to the base address.  It is limited to a
   -512 .. 504 range (7 bits << 3).  */

static int
emit_ldp (uint32_t *buf, struct aarch64_register rt,
	  struct aarch64_register rt2, struct aarch64_register rn,
	  struct aarch64_memory_operand operand)
{
  return emit_load_store_pair (buf, LDP, rt, rt2, rn, operand);
}

/* Write a LDP (SIMD&VFP) instruction using Q registers into *BUF.

     LDP qt, qt2, [rn, #offset]

   RT and RT2 are the Q registers to store.
   RN is the base address register.
   OFFSET is the immediate to add to the base address.  It is limited to
   -1024 .. 1008 range (7 bits << 4).  */

static int
emit_ldp_q_offset (uint32_t *buf, unsigned rt, unsigned rt2,
		   struct aarch64_register rn, int32_t offset)
{
  uint32_t opc = ENCODE (2, 2, 30);
  uint32_t pre_index = ENCODE (1, 1, 24);

  return aarch64_emit_insn (buf, LDP_SIMD_VFP | opc | pre_index
			    | ENCODE (offset >> 4, 7, 15)
			    | ENCODE (rt2, 5, 10)
			    | ENCODE (rn.num, 5, 5) | ENCODE (rt, 5, 0));
}

/* Write a STP (SIMD&VFP) instruction using Q registers into *BUF.

     STP qt, qt2, [rn, #offset]

   RT and RT2 are the Q registers to store.
   RN is the base address register.
   OFFSET is the immediate to add to the base address.  It is limited to
   -1024 .. 1008 range (7 bits << 4).  */

static int
emit_stp_q_offset (uint32_t *buf, unsigned rt, unsigned rt2,
		   struct aarch64_register rn, int32_t offset)
{
  uint32_t opc = ENCODE (2, 2, 30);
  uint32_t pre_index = ENCODE (1, 1, 24);

  return aarch64_emit_insn (buf, STP_SIMD_VFP | opc | pre_index
			    | ENCODE (offset >> 4, 7, 15)
			    | ENCODE (rt2, 5, 10)
			    | ENCODE (rn.num, 5, 5) | ENCODE (rt, 5, 0));
}

/* Write a LDRH instruction into *BUF.

     LDRH wt, [xn, #offset]
     LDRH wt, [xn, #index]!
     LDRH wt, [xn], #index

   RT is the register to store.
   RN is the base address register.
   OFFSET is the immediate to add to the base address.  It is limited to
   0 .. 32760 range (12 bits << 3).  */

static int
emit_ldrh (uint32_t *buf, struct aarch64_register rt,
	   struct aarch64_register rn,
	   struct aarch64_memory_operand operand)
{
  return aarch64_emit_load_store (buf, 1, LDR, rt, rn, operand);
}

/* Write a LDRB instruction into *BUF.

     LDRB wt, [xn, #offset]
     LDRB wt, [xn, #index]!
     LDRB wt, [xn], #index

   RT is the register to store.
   RN is the base address register.
   OFFSET is the immediate to add to the base address.  It is limited to
   0 .. 32760 range (12 bits << 3).  */

static int
emit_ldrb (uint32_t *buf, struct aarch64_register rt,
	   struct aarch64_register rn,
	   struct aarch64_memory_operand operand)
{
  return aarch64_emit_load_store (buf, 0, LDR, rt, rn, operand);
}



/* Write a STR instruction into *BUF.

     STR rt, [rn, #offset]
     STR rt, [rn, #index]!
     STR rt, [rn], #index

   RT is the register to store.
   RN is the base address register.
   OFFSET is the immediate to add to the base address.  It is limited to
   0 .. 32760 range (12 bits << 3).  */

static int
emit_str (uint32_t *buf, struct aarch64_register rt,
	  struct aarch64_register rn,
	  struct aarch64_memory_operand operand)
{
  return aarch64_emit_load_store (buf, rt.is64 ? 3 : 2, STR, rt, rn, operand);
}

/* Helper function emitting an exclusive load or store instruction.  */

static int
emit_load_store_exclusive (uint32_t *buf, uint32_t size,
			   enum aarch64_opcodes opcode,
			   struct aarch64_register rs,
			   struct aarch64_register rt,
			   struct aarch64_register rt2,
			   struct aarch64_register rn)
{
  return aarch64_emit_insn (buf, opcode | ENCODE (size, 2, 30)
			    | ENCODE (rs.num, 5, 16) | ENCODE (rt2.num, 5, 10)
			    | ENCODE (rn.num, 5, 5) | ENCODE (rt.num, 5, 0));
}

/* Write a LAXR instruction into *BUF.

     LDAXR rt, [xn]

   RT is the destination register.
   RN is the base address register.  */

static int
emit_ldaxr (uint32_t *buf, struct aarch64_register rt,
	    struct aarch64_register rn)
{
  return emit_load_store_exclusive (buf, rt.is64 ? 3 : 2, LDAXR, xzr, rt,
				    xzr, rn);
}

/* Write a STXR instruction into *BUF.

     STXR ws, rt, [xn]

   RS is the result register, it indicates if the store succeeded or not.
   RT is the destination register.
   RN is the base address register.  */

static int
emit_stxr (uint32_t *buf, struct aarch64_register rs,
	   struct aarch64_register rt, struct aarch64_register rn)
{
  return emit_load_store_exclusive (buf, rt.is64 ? 3 : 2, STXR, rs, rt,
				    xzr, rn);
}

/* Write a STLR instruction into *BUF.

     STLR rt, [xn]

   RT is the register to store.
   RN is the base address register.  */

static int
emit_stlr (uint32_t *buf, struct aarch64_register rt,
	   struct aarch64_register rn)
{
  return emit_load_store_exclusive (buf, rt.is64 ? 3 : 2, STLR, xzr, rt,
				    xzr, rn);
}

/* Helper function for data processing instructions with register sources.  */

static int
emit_data_processing_reg (uint32_t *buf, uint32_t opcode,
			  struct aarch64_register rd,
			  struct aarch64_register rn,
			  struct aarch64_register rm)
{
  uint32_t size = ENCODE (rd.is64, 1, 31);

  return aarch64_emit_insn (buf, opcode | size | ENCODE (rm.num, 5, 16)
			    | ENCODE (rn.num, 5, 5) | ENCODE (rd.num, 5, 0));
}

/* Helper function for data processing instructions taking either a register
   or an immediate.  */

static int
emit_data_processing (uint32_t *buf, enum aarch64_opcodes opcode,
		      struct aarch64_register rd,
		      struct aarch64_register rn,
		      struct aarch64_operand operand)
{
  uint32_t size = ENCODE (rd.is64, 1, 31);
  /* The opcode is different for register and immediate source operands.  */
  uint32_t operand_opcode;

  if (operand.type == OPERAND_IMMEDIATE)
    {
      /* xxx1 000x xxxx xxxx xxxx xxxx xxxx xxxx */
      operand_opcode = ENCODE (8, 4, 25);

      return aarch64_emit_insn (buf, opcode | operand_opcode | size
				| ENCODE (operand.imm, 12, 10)
				| ENCODE (rn.num, 5, 5)
				| ENCODE (rd.num, 5, 0));
    }
  else
    {
      /* xxx0 101x xxxx xxxx xxxx xxxx xxxx xxxx */
      operand_opcode = ENCODE (5, 4, 25);

      return emit_data_processing_reg (buf, opcode | operand_opcode, rd,
				       rn, operand.reg);
    }
}

/* Write an ADD instruction into *BUF.

     ADD rd, rn, #imm
     ADD rd, rn, rm

   This function handles both an immediate and register add.

   RD is the destination register.
   RN is the input register.
   OPERAND is the source operand, either of type OPERAND_IMMEDIATE or
   OPERAND_REGISTER.  */

static int
emit_add (uint32_t *buf, struct aarch64_register rd,
	  struct aarch64_register rn, struct aarch64_operand operand)
{
  return emit_data_processing (buf, ADD, rd, rn, operand);
}

/* Write a SUB instruction into *BUF.

     SUB rd, rn, #imm
     SUB rd, rn, rm

   This function handles both an immediate and register sub.

   RD is the destination register.
   RN is the input register.
   IMM is the immediate to substract to RN.  */

static int
emit_sub (uint32_t *buf, struct aarch64_register rd,
	  struct aarch64_register rn, struct aarch64_operand operand)
{
  return emit_data_processing (buf, SUB, rd, rn, operand);
}

/* Write a MOV instruction into *BUF.

     MOV rd, #imm
     MOV rd, rm

   This function handles both a wide immediate move and a register move,
   with the condition that the source register is not xzr.  xzr and the
   stack pointer share the same encoding and this function only supports
   the stack pointer.

   RD is the destination register.
   OPERAND is the source operand, either of type OPERAND_IMMEDIATE or
   OPERAND_REGISTER.  */

static int
emit_mov (uint32_t *buf, struct aarch64_register rd,
	  struct aarch64_operand operand)
{
  if (operand.type == OPERAND_IMMEDIATE)
    {
      uint32_t size = ENCODE (rd.is64, 1, 31);
      /* Do not shift the immediate.  */
      uint32_t shift = ENCODE (0, 2, 21);

      return aarch64_emit_insn (buf, MOV | size | shift
				| ENCODE (operand.imm, 16, 5)
				| ENCODE (rd.num, 5, 0));
    }
  else
    return emit_add (buf, rd, operand.reg, immediate_operand (0));
}

/* Write a MOVK instruction into *BUF.

     MOVK rd, #imm, lsl #shift

   RD is the destination register.
   IMM is the immediate.
   SHIFT is the logical shift left to apply to IMM.   */

static int
emit_movk (uint32_t *buf, struct aarch64_register rd, uint32_t imm,
	   unsigned shift)
{
  uint32_t size = ENCODE (rd.is64, 1, 31);

  return aarch64_emit_insn (buf, MOVK | size | ENCODE (shift, 2, 21) |
			    ENCODE (imm, 16, 5) | ENCODE (rd.num, 5, 0));
}

/* Write instructions into *BUF in order to move ADDR into a register.
   ADDR can be a 64-bit value.

   This function will emit a series of MOV and MOVK instructions, such as:

     MOV  xd, #(addr)
     MOVK xd, #(addr >> 16), lsl #16
     MOVK xd, #(addr >> 32), lsl #32
     MOVK xd, #(addr >> 48), lsl #48  */

static int
emit_mov_addr (uint32_t *buf, struct aarch64_register rd, CORE_ADDR addr)
{
  uint32_t *p = buf;

  /* The MOV (wide immediate) instruction clears to top bits of the
     register.  */
  p += emit_mov (p, rd, immediate_operand (addr & 0xffff));

  if ((addr >> 16) != 0)
    p += emit_movk (p, rd, (addr >> 16) & 0xffff, 1);
  else
    return p - buf;

  if ((addr >> 32) != 0)
    p += emit_movk (p, rd, (addr >> 32) & 0xffff, 2);
  else
    return p - buf;

  if ((addr >> 48) != 0)
    p += emit_movk (p, rd, (addr >> 48) & 0xffff, 3);

  return p - buf;
}

/* Write a SUBS instruction into *BUF.

     SUBS rd, rn, rm

   This instruction update the condition flags.

   RD is the destination register.
   RN and RM are the source registers.  */

static int
emit_subs (uint32_t *buf, struct aarch64_register rd,
	   struct aarch64_register rn, struct aarch64_operand operand)
{
  return emit_data_processing (buf, SUBS, rd, rn, operand);
}

/* Write a CMP instruction into *BUF.

     CMP rn, rm

   This instruction is an alias of SUBS xzr, rn, rm.

   RN and RM are the registers to compare.  */

static int
emit_cmp (uint32_t *buf, struct aarch64_register rn,
	      struct aarch64_operand operand)
{
  return emit_subs (buf, xzr, rn, operand);
}

/* Write a AND instruction into *BUF.

     AND rd, rn, rm

   RD is the destination register.
   RN and RM are the source registers.  */

static int
emit_and (uint32_t *buf, struct aarch64_register rd,
	  struct aarch64_register rn, struct aarch64_register rm)
{
  return emit_data_processing_reg (buf, AND, rd, rn, rm);
}

/* Write a ORR instruction into *BUF.

     ORR rd, rn, rm

   RD is the destination register.
   RN and RM are the source registers.  */

static int
emit_orr (uint32_t *buf, struct aarch64_register rd,
	  struct aarch64_register rn, struct aarch64_register rm)
{
  return emit_data_processing_reg (buf, ORR, rd, rn, rm);
}

/* Write a ORN instruction into *BUF.

     ORN rd, rn, rm

   RD is the destination register.
   RN and RM are the source registers.  */

static int
emit_orn (uint32_t *buf, struct aarch64_register rd,
	  struct aarch64_register rn, struct aarch64_register rm)
{
  return emit_data_processing_reg (buf, ORN, rd, rn, rm);
}

/* Write a EOR instruction into *BUF.

     EOR rd, rn, rm

   RD is the destination register.
   RN and RM are the source registers.  */

static int
emit_eor (uint32_t *buf, struct aarch64_register rd,
	  struct aarch64_register rn, struct aarch64_register rm)
{
  return emit_data_processing_reg (buf, EOR, rd, rn, rm);
}

/* Write a MVN instruction into *BUF.

     MVN rd, rm

   This is an alias for ORN rd, xzr, rm.

   RD is the destination register.
   RM is the source register.  */

static int
emit_mvn (uint32_t *buf, struct aarch64_register rd,
	  struct aarch64_register rm)
{
  return emit_orn (buf, rd, xzr, rm);
}

/* Write a LSLV instruction into *BUF.

     LSLV rd, rn, rm

   RD is the destination register.
   RN and RM are the source registers.  */

static int
emit_lslv (uint32_t *buf, struct aarch64_register rd,
	   struct aarch64_register rn, struct aarch64_register rm)
{
  return emit_data_processing_reg (buf, LSLV, rd, rn, rm);
}

/* Write a LSRV instruction into *BUF.

     LSRV rd, rn, rm

   RD is the destination register.
   RN and RM are the source registers.  */

static int
emit_lsrv (uint32_t *buf, struct aarch64_register rd,
	   struct aarch64_register rn, struct aarch64_register rm)
{
  return emit_data_processing_reg (buf, LSRV, rd, rn, rm);
}

/* Write a ASRV instruction into *BUF.

     ASRV rd, rn, rm

   RD is the destination register.
   RN and RM are the source registers.  */

static int
emit_asrv (uint32_t *buf, struct aarch64_register rd,
	   struct aarch64_register rn, struct aarch64_register rm)
{
  return emit_data_processing_reg (buf, ASRV, rd, rn, rm);
}

/* Write a MUL instruction into *BUF.

     MUL rd, rn, rm

   RD is the destination register.
   RN and RM are the source registers.  */

static int
emit_mul (uint32_t *buf, struct aarch64_register rd,
	  struct aarch64_register rn, struct aarch64_register rm)
{
  return emit_data_processing_reg (buf, MUL, rd, rn, rm);
}

/* Write a MRS instruction into *BUF.  The register size is 64-bit.

     MRS xt, system_reg

   RT is the destination register.
   SYSTEM_REG is special purpose register to read.  */

static int
emit_mrs (uint32_t *buf, struct aarch64_register rt,
	  enum aarch64_system_control_registers system_reg)
{
  return aarch64_emit_insn (buf, MRS | ENCODE (system_reg, 15, 5)
			    | ENCODE (rt.num, 5, 0));
}

/* Write a MSR instruction into *BUF.  The register size is 64-bit.

     MSR system_reg, xt

   SYSTEM_REG is special purpose register to write.
   RT is the input register.  */

static int
emit_msr (uint32_t *buf, enum aarch64_system_control_registers system_reg,
	  struct aarch64_register rt)
{
  return aarch64_emit_insn (buf, MSR | ENCODE (system_reg, 15, 5)
			    | ENCODE (rt.num, 5, 0));
}

/* Write a SEVL instruction into *BUF.

   This is a hint instruction telling the hardware to trigger an event.  */

static int
emit_sevl (uint32_t *buf)
{
  return aarch64_emit_insn (buf, SEVL);
}

/* Write a WFE instruction into *BUF.

   This is a hint instruction telling the hardware to wait for an event.  */

static int
emit_wfe (uint32_t *buf)
{
  return aarch64_emit_insn (buf, WFE);
}

/* Write a SBFM instruction into *BUF.

     SBFM rd, rn, #immr, #imms

   This instruction moves the bits from #immr to #imms into the
   destination, sign extending the result.

   RD is the destination register.
   RN is the source register.
   IMMR is the bit number to start at (least significant bit).
   IMMS is the bit number to stop at (most significant bit).  */

static int
emit_sbfm (uint32_t *buf, struct aarch64_register rd,
	   struct aarch64_register rn, uint32_t immr, uint32_t imms)
{
  uint32_t size = ENCODE (rd.is64, 1, 31);
  uint32_t n = ENCODE (rd.is64, 1, 22);

  return aarch64_emit_insn (buf, SBFM | size | n | ENCODE (immr, 6, 16)
			    | ENCODE (imms, 6, 10) | ENCODE (rn.num, 5, 5)
			    | ENCODE (rd.num, 5, 0));
}

/* Write a SBFX instruction into *BUF.

     SBFX rd, rn, #lsb, #width

   This instruction moves #width bits from #lsb into the destination, sign
   extending the result.  This is an alias for:

     SBFM rd, rn, #lsb, #(lsb + width - 1)

   RD is the destination register.
   RN is the source register.
   LSB is the bit number to start at (least significant bit).
   WIDTH is the number of bits to move.  */

static int
emit_sbfx (uint32_t *buf, struct aarch64_register rd,
	   struct aarch64_register rn, uint32_t lsb, uint32_t width)
{
  return emit_sbfm (buf, rd, rn, lsb, lsb + width - 1);
}

/* Write a UBFM instruction into *BUF.

     UBFM rd, rn, #immr, #imms

   This instruction moves the bits from #immr to #imms into the
   destination, extending the result with zeros.

   RD is the destination register.
   RN is the source register.
   IMMR is the bit number to start at (least significant bit).
   IMMS is the bit number to stop at (most significant bit).  */

static int
emit_ubfm (uint32_t *buf, struct aarch64_register rd,
	   struct aarch64_register rn, uint32_t immr, uint32_t imms)
{
  uint32_t size = ENCODE (rd.is64, 1, 31);
  uint32_t n = ENCODE (rd.is64, 1, 22);

  return aarch64_emit_insn (buf, UBFM | size | n | ENCODE (immr, 6, 16)
			    | ENCODE (imms, 6, 10) | ENCODE (rn.num, 5, 5)
			    | ENCODE (rd.num, 5, 0));
}

/* Write a UBFX instruction into *BUF.

     UBFX rd, rn, #lsb, #width

   This instruction moves #width bits from #lsb into the destination,
   extending the result with zeros.  This is an alias for:

     UBFM rd, rn, #lsb, #(lsb + width - 1)

   RD is the destination register.
   RN is the source register.
   LSB is the bit number to start at (least significant bit).
   WIDTH is the number of bits to move.  */

static int
emit_ubfx (uint32_t *buf, struct aarch64_register rd,
	   struct aarch64_register rn, uint32_t lsb, uint32_t width)
{
  return emit_ubfm (buf, rd, rn, lsb, lsb + width - 1);
}

/* Write a CSINC instruction into *BUF.

     CSINC rd, rn, rm, cond

   This instruction conditionally increments rn or rm and places the result
   in rd.  rn is chosen is the condition is true.

   RD is the destination register.
   RN and RM are the source registers.
   COND is the encoded condition.  */

static int
emit_csinc (uint32_t *buf, struct aarch64_register rd,
	    struct aarch64_register rn, struct aarch64_register rm,
	    unsigned cond)
{
  uint32_t size = ENCODE (rd.is64, 1, 31);

  return aarch64_emit_insn (buf, CSINC | size | ENCODE (rm.num, 5, 16)
			    | ENCODE (cond, 4, 12) | ENCODE (rn.num, 5, 5)
			    | ENCODE (rd.num, 5, 0));
}

/* Write a CSET instruction into *BUF.

     CSET rd, cond

   This instruction conditionally write 1 or 0 in the destination register.
   1 is written if the condition is true.  This is an alias for:

     CSINC rd, xzr, xzr, !cond

   Note that the condition needs to be inverted.

   RD is the destination register.
   RN and RM are the source registers.
   COND is the encoded condition.  */

static int
emit_cset (uint32_t *buf, struct aarch64_register rd, unsigned cond)
{
  /* The least significant bit of the condition needs toggling in order to
     invert it.  */
  return emit_csinc (buf, rd, xzr, xzr, cond ^ 0x1);
}

/* Write LEN instructions from BUF into the inferior memory at *TO.

   Note instructions are always little endian on AArch64, unlike data.  */

static void
append_insns (CORE_ADDR *to, size_t len, const uint32_t *buf)
{
  size_t byte_len = len * sizeof (uint32_t);
#if (__BYTE_ORDER == __BIG_ENDIAN)
  uint32_t *le_buf = (uint32_t *) xmalloc (byte_len);
  size_t i;

  for (i = 0; i < len; i++)
    le_buf[i] = htole32 (buf[i]);

  target_write_memory (*to, (const unsigned char *) le_buf, byte_len);

  xfree (le_buf);
#else
  target_write_memory (*to, (const unsigned char *) buf, byte_len);
#endif

  *to += byte_len;
}

/* Sub-class of struct aarch64_insn_data, store information of
   instruction relocation for fast tracepoint.  Visitor can
   relocate an instruction from BASE.INSN_ADDR to NEW_ADDR and save
   the relocated instructions in buffer pointed by INSN_PTR.  */

struct aarch64_insn_relocation_data
{
  struct aarch64_insn_data base;

  /* The new address the instruction is relocated to.  */
  CORE_ADDR new_addr;
  /* Pointer to the buffer of relocated instruction(s).  */
  uint32_t *insn_ptr;
};

/* Implementation of aarch64_insn_visitor method "b".  */

static void
aarch64_ftrace_insn_reloc_b (const int is_bl, const int32_t offset,
			     struct aarch64_insn_data *data)
{
  struct aarch64_insn_relocation_data *insn_reloc
    = (struct aarch64_insn_relocation_data *) data;
  int64_t new_offset
    = insn_reloc->base.insn_addr - insn_reloc->new_addr + offset;

  if (can_encode_int32 (new_offset, 28))
    insn_reloc->insn_ptr += emit_b (insn_reloc->insn_ptr, is_bl, new_offset);
}

/* Implementation of aarch64_insn_visitor method "b_cond".  */

static void
aarch64_ftrace_insn_reloc_b_cond (const unsigned cond, const int32_t offset,
				  struct aarch64_insn_data *data)
{
  struct aarch64_insn_relocation_data *insn_reloc
    = (struct aarch64_insn_relocation_data *) data;
  int64_t new_offset
    = insn_reloc->base.insn_addr - insn_reloc->new_addr + offset;

  if (can_encode_int32 (new_offset, 21))
    {
      insn_reloc->insn_ptr += emit_bcond (insn_reloc->insn_ptr, cond,
					  new_offset);
    }
  else if (can_encode_int32 (new_offset, 28))
    {
      /* The offset is out of range for a conditional branch
	 instruction but not for a unconditional branch.  We can use
	 the following instructions instead:

	 B.COND TAKEN    ; If cond is true, then jump to TAKEN.
	 B NOT_TAKEN     ; Else jump over TAKEN and continue.
	 TAKEN:
	 B #(offset - 8)
	 NOT_TAKEN:

      */

      insn_reloc->insn_ptr += emit_bcond (insn_reloc->insn_ptr, cond, 8);
      insn_reloc->insn_ptr += emit_b (insn_reloc->insn_ptr, 0, 8);
      insn_reloc->insn_ptr += emit_b (insn_reloc->insn_ptr, 0, new_offset - 8);
    }
}

/* Implementation of aarch64_insn_visitor method "cb".  */

static void
aarch64_ftrace_insn_reloc_cb (const int32_t offset, const int is_cbnz,
			      const unsigned rn, int is64,
			      struct aarch64_insn_data *data)
{
  struct aarch64_insn_relocation_data *insn_reloc
    = (struct aarch64_insn_relocation_data *) data;
  int64_t new_offset
    = insn_reloc->base.insn_addr - insn_reloc->new_addr + offset;

  if (can_encode_int32 (new_offset, 21))
    {
      insn_reloc->insn_ptr += emit_cb (insn_reloc->insn_ptr, is_cbnz,
				       aarch64_register (rn, is64), new_offset);
    }
  else if (can_encode_int32 (new_offset, 28))
    {
      /* The offset is out of range for a compare and branch
	 instruction but not for a unconditional branch.  We can use
	 the following instructions instead:

	 CBZ xn, TAKEN   ; xn == 0, then jump to TAKEN.
	 B NOT_TAKEN     ; Else jump over TAKEN and continue.
	 TAKEN:
	 B #(offset - 8)
	 NOT_TAKEN:

      */
      insn_reloc->insn_ptr += emit_cb (insn_reloc->insn_ptr, is_cbnz,
				       aarch64_register (rn, is64), 8);
      insn_reloc->insn_ptr += emit_b (insn_reloc->insn_ptr, 0, 8);
      insn_reloc->insn_ptr += emit_b (insn_reloc->insn_ptr, 0, new_offset - 8);
    }
}

/* Implementation of aarch64_insn_visitor method "tb".  */

static void
aarch64_ftrace_insn_reloc_tb (const int32_t offset, int is_tbnz,
			      const unsigned rt, unsigned bit,
			      struct aarch64_insn_data *data)
{
  struct aarch64_insn_relocation_data *insn_reloc
    = (struct aarch64_insn_relocation_data *) data;
  int64_t new_offset
    = insn_reloc->base.insn_addr - insn_reloc->new_addr + offset;

  if (can_encode_int32 (new_offset, 16))
    {
      insn_reloc->insn_ptr += emit_tb (insn_reloc->insn_ptr, is_tbnz, bit,
				       aarch64_register (rt, 1), new_offset);
    }
  else if (can_encode_int32 (new_offset, 28))
    {
      /* The offset is out of range for a test bit and branch
	 instruction but not for a unconditional branch.  We can use
	 the following instructions instead:

	 TBZ xn, #bit, TAKEN ; xn[bit] == 0, then jump to TAKEN.
	 B NOT_TAKEN         ; Else jump over TAKEN and continue.
	 TAKEN:
	 B #(offset - 8)
	 NOT_TAKEN:

      */
      insn_reloc->insn_ptr += emit_tb (insn_reloc->insn_ptr, is_tbnz, bit,
				       aarch64_register (rt, 1), 8);
      insn_reloc->insn_ptr += emit_b (insn_reloc->insn_ptr, 0, 8);
      insn_reloc->insn_ptr += emit_b (insn_reloc->insn_ptr, 0,
				      new_offset - 8);
    }
}

/* Implementation of aarch64_insn_visitor method "adr".  */

static void
aarch64_ftrace_insn_reloc_adr (const int32_t offset, const unsigned rd,
			       const int is_adrp,
			       struct aarch64_insn_data *data)
{
  struct aarch64_insn_relocation_data *insn_reloc
    = (struct aarch64_insn_relocation_data *) data;
  /* We know exactly the address the ADR{P,} instruction will compute.
     We can just write it to the destination register.  */
  CORE_ADDR address = data->insn_addr + offset;

  if (is_adrp)
    {
      /* Clear the lower 12 bits of the offset to get the 4K page.  */
      insn_reloc->insn_ptr += emit_mov_addr (insn_reloc->insn_ptr,
					     aarch64_register (rd, 1),
					     address & ~0xfff);
    }
  else
    insn_reloc->insn_ptr += emit_mov_addr (insn_reloc->insn_ptr,
					   aarch64_register (rd, 1), address);
}

/* Implementation of aarch64_insn_visitor method "ldr_literal".  */

static void
aarch64_ftrace_insn_reloc_ldr_literal (const int32_t offset, const int is_sw,
				       const unsigned rt, const int is64,
				       struct aarch64_insn_data *data)
{
  struct aarch64_insn_relocation_data *insn_reloc
    = (struct aarch64_insn_relocation_data *) data;
  CORE_ADDR address = data->insn_addr + offset;

  insn_reloc->insn_ptr += emit_mov_addr (insn_reloc->insn_ptr,
					 aarch64_register (rt, 1), address);

  /* We know exactly what address to load from, and what register we
     can use:

     MOV xd, #(oldloc + offset)
     MOVK xd, #((oldloc + offset) >> 16), lsl #16
     ...

     LDR xd, [xd] ; or LDRSW xd, [xd]

  */

  if (is_sw)
    insn_reloc->insn_ptr += emit_ldrsw (insn_reloc->insn_ptr,
					aarch64_register (rt, 1),
					aarch64_register (rt, 1),
					offset_memory_operand (0));
  else
    insn_reloc->insn_ptr += emit_ldr (insn_reloc->insn_ptr,
				      aarch64_register (rt, is64),
				      aarch64_register (rt, 1),
				      offset_memory_operand (0));
}

/* Implementation of aarch64_insn_visitor method "others".  */

static void
aarch64_ftrace_insn_reloc_others (const uint32_t insn,
				  struct aarch64_insn_data *data)
{
  struct aarch64_insn_relocation_data *insn_reloc
    = (struct aarch64_insn_relocation_data *) data;

  /* The instruction is not PC relative.  Just re-emit it at the new
     location.  */
  insn_reloc->insn_ptr += aarch64_emit_insn (insn_reloc->insn_ptr, insn);
}

static const struct aarch64_insn_visitor visitor =
{
  aarch64_ftrace_insn_reloc_b,
  aarch64_ftrace_insn_reloc_b_cond,
  aarch64_ftrace_insn_reloc_cb,
  aarch64_ftrace_insn_reloc_tb,
  aarch64_ftrace_insn_reloc_adr,
  aarch64_ftrace_insn_reloc_ldr_literal,
  aarch64_ftrace_insn_reloc_others,
};

bool
aarch64_target::supports_fast_tracepoints ()
{
  return true;
}

/* Implementation of target ops method
   "install_fast_tracepoint_jump_pad".  */

int
aarch64_target::install_fast_tracepoint_jump_pad
  (CORE_ADDR tpoint, CORE_ADDR tpaddr, CORE_ADDR collector,
   CORE_ADDR lockaddr, ULONGEST orig_size, CORE_ADDR *jump_entry,
   CORE_ADDR *trampoline, ULONGEST *trampoline_size,
   unsigned char *jjump_pad_insn, ULONGEST *jjump_pad_insn_size,
   CORE_ADDR *adjusted_insn_addr, CORE_ADDR *adjusted_insn_addr_end,
   char *err)
{
  uint32_t buf[256];
  uint32_t *p = buf;
  int64_t offset;
  int i;
  uint32_t insn;
  CORE_ADDR buildaddr = *jump_entry;
  struct aarch64_insn_relocation_data insn_data;

  /* We need to save the current state on the stack both to restore it
     later and to collect register values when the tracepoint is hit.

     The saved registers are pushed in a layout that needs to be in sync
     with aarch64_ft_collect_regmap (see linux-aarch64-ipa.c).  Later on
     the supply_fast_tracepoint_registers function will fill in the
     register cache from a pointer to saved registers on the stack we build
     here.

     For simplicity, we set the size of each cell on the stack to 16 bytes.
     This way one cell can hold any register type, from system registers
     to the 128 bit SIMD&FP registers.  Furthermore, the stack pointer
     has to be 16 bytes aligned anyway.

     Note that the CPSR register does not exist on AArch64.  Instead we
     can access system bits describing the process state with the
     MRS/MSR instructions, namely the condition flags.  We save them as
     if they are part of a CPSR register because that's how GDB
     interprets these system bits.  At the moment, only the condition
     flags are saved in CPSR (NZCV).

     Stack layout, each cell is 16 bytes (descending):

     High *-------- SIMD&FP registers from 31 down to 0. --------*
	  | q31                                                  |
	  .                                                      .
	  .                                                      . 32 cells
	  .                                                      .
	  | q0                                                   |
	  *---- General purpose registers from 30 down to 0. ----*
	  | x30                                                  |
	  .                                                      .
	  .                                                      . 31 cells
	  .                                                      .
	  | x0                                                   |
	  *------------- Special purpose registers. -------------*
	  | SP                                                   |
	  | PC                                                   |
	  | CPSR (NZCV)                                          | 5 cells
	  | FPSR                                                 |
	  | FPCR                                                 | <- SP + 16
	  *------------- collecting_t object --------------------*
	  | TPIDR_EL0               | struct tracepoint *        |
     Low  *------------------------------------------------------*

     After this stack is set up, we issue a call to the collector, passing
     it the saved registers at (SP + 16).  */

  /* Push SIMD&FP registers on the stack:

       SUB sp, sp, #(32 * 16)

       STP q30, q31, [sp, #(30 * 16)]
       ...
       STP q0, q1, [sp]

     */
  p += emit_sub (p, sp, sp, immediate_operand (32 * 16));
  for (i = 30; i >= 0; i -= 2)
    p += emit_stp_q_offset (p, i, i + 1, sp, i * 16);

  /* Push general purpose registers on the stack.  Note that we do not need
     to push x31 as it represents the xzr register and not the stack
     pointer in a STR instruction.

       SUB sp, sp, #(31 * 16)

       STR x30, [sp, #(30 * 16)]
       ...
       STR x0, [sp]

     */
  p += emit_sub (p, sp, sp, immediate_operand (31 * 16));
  for (i = 30; i >= 0; i -= 1)
    p += emit_str (p, aarch64_register (i, 1), sp,
		   offset_memory_operand (i * 16));

  /* Make space for 5 more cells.

       SUB sp, sp, #(5 * 16)

     */
  p += emit_sub (p, sp, sp, immediate_operand (5 * 16));


  /* Save SP:

       ADD x4, sp, #((32 + 31 + 5) * 16)
       STR x4, [sp, #(4 * 16)]

     */
  p += emit_add (p, x4, sp, immediate_operand ((32 + 31 + 5) * 16));
  p += emit_str (p, x4, sp, offset_memory_operand (4 * 16));

  /* Save PC (tracepoint address):

       MOV  x3, #(tpaddr)
       ...

       STR x3, [sp, #(3 * 16)]

     */

  p += emit_mov_addr (p, x3, tpaddr);
  p += emit_str (p, x3, sp, offset_memory_operand (3 * 16));

  /* Save CPSR (NZCV), FPSR and FPCR:

       MRS x2, nzcv
       MRS x1, fpsr
       MRS x0, fpcr

       STR x2, [sp, #(2 * 16)]
       STR x1, [sp, #(1 * 16)]
       STR x0, [sp, #(0 * 16)]

     */
  p += emit_mrs (p, x2, NZCV);
  p += emit_mrs (p, x1, FPSR);
  p += emit_mrs (p, x0, FPCR);
  p += emit_str (p, x2, sp, offset_memory_operand (2 * 16));
  p += emit_str (p, x1, sp, offset_memory_operand (1 * 16));
  p += emit_str (p, x0, sp, offset_memory_operand (0 * 16));

  /* Push the collecting_t object.  It consist of the address of the
     tracepoint and an ID for the current thread.  We get the latter by
     reading the tpidr_el0 system register.  It corresponds to the
     NT_ARM_TLS register accessible with ptrace.

       MOV x0, #(tpoint)
       ...

       MRS x1, tpidr_el0

       STP x0, x1, [sp, #-16]!

     */

  p += emit_mov_addr (p, x0, tpoint);
  p += emit_mrs (p, x1, TPIDR_EL0);
  p += emit_stp (p, x0, x1, sp, preindex_memory_operand (-16));

  /* Spin-lock:

     The shared memory for the lock is at lockaddr.  It will hold zero
     if no-one is holding the lock, otherwise it contains the address of
     the collecting_t object on the stack of the thread which acquired it.

     At this stage, the stack pointer points to this thread's collecting_t
     object.

     We use the following registers:
     - x0: Address of the lock.
     - x1: Pointer to collecting_t object.
     - x2: Scratch register.

       MOV x0, #(lockaddr)
       ...
       MOV x1, sp

       ; Trigger an event local to this core.  So the following WFE
       ; instruction is ignored.
       SEVL
     again:
       ; Wait for an event.  The event is triggered by either the SEVL
       ; or STLR instructions (store release).
       WFE

       ; Atomically read at lockaddr.  This marks the memory location as
       ; exclusive.  This instruction also has memory constraints which
       ; make sure all previous data reads and writes are done before
       ; executing it.
       LDAXR x2, [x0]

       ; Try again if another thread holds the lock.
       CBNZ x2, again

       ; We can lock it!  Write the address of the collecting_t object.
       ; This instruction will fail if the memory location is not marked
       ; as exclusive anymore.  If it succeeds, it will remove the
       ; exclusive mark on the memory location.  This way, if another
       ; thread executes this instruction before us, we will fail and try
       ; all over again.
       STXR w2, x1, [x0]
       CBNZ w2, again

     */

  p += emit_mov_addr (p, x0, lockaddr);
  p += emit_mov (p, x1, register_operand (sp));

  p += emit_sevl (p);
  p += emit_wfe (p);
  p += emit_ldaxr (p, x2, x0);
  p += emit_cb (p, 1, w2, -2 * 4);
  p += emit_stxr (p, w2, x1, x0);
  p += emit_cb (p, 1, x2, -4 * 4);

  /* Call collector (struct tracepoint *, unsigned char *):

       MOV x0, #(tpoint)
       ...

       ; Saved registers start after the collecting_t object.
       ADD x1, sp, #16

       ; We use an intra-procedure-call scratch register.
       MOV ip0, #(collector)
       ...

       ; And call back to C!
       BLR ip0

     */

  p += emit_mov_addr (p, x0, tpoint);
  p += emit_add (p, x1, sp, immediate_operand (16));

  p += emit_mov_addr (p, ip0, collector);
  p += emit_blr (p, ip0);

  /* Release the lock.

       MOV x0, #(lockaddr)
       ...

       ; This instruction is a normal store with memory ordering
       ; constraints.  Thanks to this we do not have to put a data
       ; barrier instruction to make sure all data read and writes are done
       ; before this instruction is executed.  Furthermore, this instruction
       ; will trigger an event, letting other threads know they can grab
       ; the lock.
       STLR xzr, [x0]

     */
  p += emit_mov_addr (p, x0, lockaddr);
  p += emit_stlr (p, xzr, x0);

  /* Free collecting_t object:

       ADD sp, sp, #16

     */
  p += emit_add (p, sp, sp, immediate_operand (16));

  /* Restore CPSR (NZCV), FPSR and FPCR.  And free all special purpose
     registers from the stack.

       LDR x2, [sp, #(2 * 16)]
       LDR x1, [sp, #(1 * 16)]
       LDR x0, [sp, #(0 * 16)]

       MSR NZCV, x2
       MSR FPSR, x1
       MSR FPCR, x0

       ADD sp, sp #(5 * 16)

     */
  p += emit_ldr (p, x2, sp, offset_memory_operand (2 * 16));
  p += emit_ldr (p, x1, sp, offset_memory_operand (1 * 16));
  p += emit_ldr (p, x0, sp, offset_memory_operand (0 * 16));
  p += emit_msr (p, NZCV, x2);
  p += emit_msr (p, FPSR, x1);
  p += emit_msr (p, FPCR, x0);

  p += emit_add (p, sp, sp, immediate_operand (5 * 16));

  /* Pop general purpose registers:

       LDR x0, [sp]
       ...
       LDR x30, [sp, #(30 * 16)]

       ADD sp, sp, #(31 * 16)

     */
  for (i = 0; i <= 30; i += 1)
    p += emit_ldr (p, aarch64_register (i, 1), sp,
		   offset_memory_operand (i * 16));
  p += emit_add (p, sp, sp, immediate_operand (31 * 16));

  /* Pop SIMD&FP registers:

       LDP q0, q1, [sp]
       ...
       LDP q30, q31, [sp, #(30 * 16)]

       ADD sp, sp, #(32 * 16)

     */
  for (i = 0; i <= 30; i += 2)
    p += emit_ldp_q_offset (p, i, i + 1, sp, i * 16);
  p += emit_add (p, sp, sp, immediate_operand (32 * 16));

  /* Write the code into the inferior memory.  */
  append_insns (&buildaddr, p - buf, buf);

  /* Now emit the relocated instruction.  */
  *adjusted_insn_addr = buildaddr;
  target_read_uint32 (tpaddr, &insn);

  insn_data.base.insn_addr = tpaddr;
  insn_data.new_addr = buildaddr;
  insn_data.insn_ptr = buf;

  aarch64_relocate_instruction (insn, &visitor,
				(struct aarch64_insn_data *) &insn_data);

  /* We may not have been able to relocate the instruction.  */
  if (insn_data.insn_ptr == buf)
    {
      sprintf (err,
	       "E.Could not relocate instruction from %s to %s.",
	       core_addr_to_string_nz (tpaddr),
	       core_addr_to_string_nz (buildaddr));
      return 1;
    }
  else
    append_insns (&buildaddr, insn_data.insn_ptr - buf, buf);
  *adjusted_insn_addr_end = buildaddr;

  /* Go back to the start of the buffer.  */
  p = buf;

  /* Emit a branch back from the jump pad.  */
  offset = (tpaddr + orig_size - buildaddr);
  if (!can_encode_int32 (offset, 28))
    {
      sprintf (err,
	       "E.Jump back from jump pad too far from tracepoint "
	       "(offset 0x%" PRIx64 " cannot be encoded in 28 bits).",
	       offset);
      return 1;
    }

  p += emit_b (p, 0, offset);
  append_insns (&buildaddr, p - buf, buf);

  /* Give the caller a branch instruction into the jump pad.  */
  offset = (*jump_entry - tpaddr);
  if (!can_encode_int32 (offset, 28))
    {
      sprintf (err,
	       "E.Jump pad too far from tracepoint "
	       "(offset 0x%" PRIx64 " cannot be encoded in 28 bits).",
	       offset);
      return 1;
    }

  emit_b ((uint32_t *) jjump_pad_insn, 0, offset);
  *jjump_pad_insn_size = 4;

  /* Return the end address of our pad.  */
  *jump_entry = buildaddr;

  return 0;
}

/* Helper function writing LEN instructions from START into
   current_insn_ptr.  */

static void
emit_ops_insns (const uint32_t *start, int len)
{
  CORE_ADDR buildaddr = current_insn_ptr;

  threads_debug_printf ("Adding %d instructions at %s",
			len, paddress (buildaddr));

  append_insns (&buildaddr, len, start);
  current_insn_ptr = buildaddr;
}

/* Pop a register from the stack.  */

static int
emit_pop (uint32_t *buf, struct aarch64_register rt)
{
  return emit_ldr (buf, rt, sp, postindex_memory_operand (1 * 16));
}

/* Push a register on the stack.  */

static int
emit_push (uint32_t *buf, struct aarch64_register rt)
{
  return emit_str (buf, rt, sp, preindex_memory_operand (-1 * 16));
}

/* Implementation of emit_ops method "emit_prologue".  */

static void
aarch64_emit_prologue (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  /* This function emit a prologue for the following function prototype:

     enum eval_result_type f (unsigned char *regs,
			      ULONGEST *value);

     The first argument is a buffer of raw registers.  The second
     argument is the result of
     evaluating the expression, which will be set to whatever is on top of
     the stack at the end.

     The stack set up by the prologue is as such:

     High *------------------------------------------------------*
	  | LR                                                   |
	  | FP                                                   | <- FP
	  | x1  (ULONGEST *value)                                |
	  | x0  (unsigned char *regs)                            |
     Low  *------------------------------------------------------*

     As we are implementing a stack machine, each opcode can expand the
     stack so we never know how far we are from the data saved by this
     prologue.  In order to be able refer to value and regs later, we save
     the current stack pointer in the frame pointer.  This way, it is not
     clobbered when calling C functions.

     Finally, throughout every operation, we are using register x0 as the
     top of the stack, and x1 as a scratch register.  */

  p += emit_stp (p, x0, x1, sp, preindex_memory_operand (-2 * 16));
  p += emit_str (p, lr, sp, offset_memory_operand (3 * 8));
  p += emit_str (p, fp, sp, offset_memory_operand (2 * 8));

  p += emit_add (p, fp, sp, immediate_operand (2 * 8));


  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_epilogue".  */

static void
aarch64_emit_epilogue (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  /* Store the result of the expression (x0) in *value.  */
  p += emit_sub (p, x1, fp, immediate_operand (1 * 8));
  p += emit_ldr (p, x1, x1, offset_memory_operand (0));
  p += emit_str (p, x0, x1, offset_memory_operand (0));

  /* Restore the previous state.  */
  p += emit_add (p, sp, fp, immediate_operand (2 * 8));
  p += emit_ldp (p, fp, lr, fp, offset_memory_operand (0));

  /* Return expr_eval_no_error.  */
  p += emit_mov (p, x0, immediate_operand (expr_eval_no_error));
  p += emit_ret (p, lr);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_add".  */

static void
aarch64_emit_add (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_add (p, x0, x1, register_operand (x0));

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_sub".  */

static void
aarch64_emit_sub (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_sub (p, x0, x1, register_operand (x0));

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_mul".  */

static void
aarch64_emit_mul (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_mul (p, x0, x1, x0);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_lsh".  */

static void
aarch64_emit_lsh (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_lslv (p, x0, x1, x0);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_rsh_signed".  */

static void
aarch64_emit_rsh_signed (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_asrv (p, x0, x1, x0);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_rsh_unsigned".  */

static void
aarch64_emit_rsh_unsigned (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_lsrv (p, x0, x1, x0);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_ext".  */

static void
aarch64_emit_ext (int arg)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_sbfx (p, x0, x0, 0, arg);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_log_not".  */

static void
aarch64_emit_log_not (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  /* If the top of the stack is 0, replace it with 1.  Else replace it with
     0.  */

  p += emit_cmp (p, x0, immediate_operand (0));
  p += emit_cset (p, x0, EQ);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_bit_and".  */

static void
aarch64_emit_bit_and (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_and (p, x0, x0, x1);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_bit_or".  */

static void
aarch64_emit_bit_or (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_orr (p, x0, x0, x1);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_bit_xor".  */

static void
aarch64_emit_bit_xor (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_eor (p, x0, x0, x1);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_bit_not".  */

static void
aarch64_emit_bit_not (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_mvn (p, x0, x0);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_equal".  */

static void
aarch64_emit_equal (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_cmp (p, x0, register_operand (x1));
  p += emit_cset (p, x0, EQ);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_less_signed".  */

static void
aarch64_emit_less_signed (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_cmp (p, x1, register_operand (x0));
  p += emit_cset (p, x0, LT);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_less_unsigned".  */

static void
aarch64_emit_less_unsigned (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_cmp (p, x1, register_operand (x0));
  p += emit_cset (p, x0, LO);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_ref".  */

static void
aarch64_emit_ref (int size)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  switch (size)
    {
    case 1:
      p += emit_ldrb (p, w0, x0, offset_memory_operand (0));
      break;
    case 2:
      p += emit_ldrh (p, w0, x0, offset_memory_operand (0));
      break;
    case 4:
      p += emit_ldr (p, w0, x0, offset_memory_operand (0));
      break;
    case 8:
      p += emit_ldr (p, x0, x0, offset_memory_operand (0));
      break;
    default:
      /* Unknown size, bail on compilation.  */
      emit_error = 1;
      break;
    }

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_if_goto".  */

static void
aarch64_emit_if_goto (int *offset_p, int *size_p)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  /* The Z flag is set or cleared here.  */
  p += emit_cmp (p, x0, immediate_operand (0));
  /* This instruction must not change the Z flag.  */
  p += emit_pop (p, x0);
  /* Branch over the next instruction if x0 == 0.  */
  p += emit_bcond (p, EQ, 8);

  /* The NOP instruction will be patched with an unconditional branch.  */
  if (offset_p)
    *offset_p = (p - buf) * 4;
  if (size_p)
    *size_p = 4;
  p += emit_nop (p);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_goto".  */

static void
aarch64_emit_goto (int *offset_p, int *size_p)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  /* The NOP instruction will be patched with an unconditional branch.  */
  if (offset_p)
    *offset_p = 0;
  if (size_p)
    *size_p = 4;
  p += emit_nop (p);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "write_goto_address".  */

static void
aarch64_write_goto_address (CORE_ADDR from, CORE_ADDR to, int size)
{
  uint32_t insn;

  emit_b (&insn, 0, to - from);
  append_insns (&from, 1, &insn);
}

/* Implementation of emit_ops method "emit_const".  */

static void
aarch64_emit_const (LONGEST num)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_mov_addr (p, x0, num);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_call".  */

static void
aarch64_emit_call (CORE_ADDR fn)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_mov_addr (p, ip0, fn);
  p += emit_blr (p, ip0);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_reg".  */

static void
aarch64_emit_reg (int reg)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  /* Set x0 to unsigned char *regs.  */
  p += emit_sub (p, x0, fp, immediate_operand (2 * 8));
  p += emit_ldr (p, x0, x0, offset_memory_operand (0));
  p += emit_mov (p, x1, immediate_operand (reg));

  emit_ops_insns (buf, p - buf);

  aarch64_emit_call (get_raw_reg_func_addr ());
}

/* Implementation of emit_ops method "emit_pop".  */

static void
aarch64_emit_pop (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x0);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_stack_flush".  */

static void
aarch64_emit_stack_flush (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_push (p, x0);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_zero_ext".  */

static void
aarch64_emit_zero_ext (int arg)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_ubfx (p, x0, x0, 0, arg);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_swap".  */

static void
aarch64_emit_swap (void)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_ldr (p, x1, sp, offset_memory_operand (0 * 16));
  p += emit_str (p, x0, sp, offset_memory_operand (0 * 16));
  p += emit_mov (p, x0, register_operand (x1));

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_stack_adjust".  */

static void
aarch64_emit_stack_adjust (int n)
{
  /* This is not needed with our design.  */
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_add (p, sp, sp, immediate_operand (n * 16));

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_int_call_1".  */

static void
aarch64_emit_int_call_1 (CORE_ADDR fn, int arg1)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_mov (p, x0, immediate_operand (arg1));

  emit_ops_insns (buf, p - buf);

  aarch64_emit_call (fn);
}

/* Implementation of emit_ops method "emit_void_call_2".  */

static void
aarch64_emit_void_call_2 (CORE_ADDR fn, int arg1)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  /* Push x0 on the stack.  */
  aarch64_emit_stack_flush ();

  /* Setup arguments for the function call:

     x0: arg1
     x1: top of the stack

       MOV x1, x0
       MOV x0, #arg1  */

  p += emit_mov (p, x1, register_operand (x0));
  p += emit_mov (p, x0, immediate_operand (arg1));

  emit_ops_insns (buf, p - buf);

  aarch64_emit_call (fn);

  /* Restore x0.  */
  aarch64_emit_pop ();
}

/* Implementation of emit_ops method "emit_eq_goto".  */

static void
aarch64_emit_eq_goto (int *offset_p, int *size_p)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_cmp (p, x1, register_operand (x0));
  /* Branch over the next instruction if x0 != x1.  */
  p += emit_bcond (p, NE, 8);
  /* The NOP instruction will be patched with an unconditional branch.  */
  if (offset_p)
    *offset_p = (p - buf) * 4;
  if (size_p)
    *size_p = 4;
  p += emit_nop (p);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_ne_goto".  */

static void
aarch64_emit_ne_goto (int *offset_p, int *size_p)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_cmp (p, x1, register_operand (x0));
  /* Branch over the next instruction if x0 == x1.  */
  p += emit_bcond (p, EQ, 8);
  /* The NOP instruction will be patched with an unconditional branch.  */
  if (offset_p)
    *offset_p = (p - buf) * 4;
  if (size_p)
    *size_p = 4;
  p += emit_nop (p);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_lt_goto".  */

static void
aarch64_emit_lt_goto (int *offset_p, int *size_p)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_cmp (p, x1, register_operand (x0));
  /* Branch over the next instruction if x0 >= x1.  */
  p += emit_bcond (p, GE, 8);
  /* The NOP instruction will be patched with an unconditional branch.  */
  if (offset_p)
    *offset_p = (p - buf) * 4;
  if (size_p)
    *size_p = 4;
  p += emit_nop (p);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_le_goto".  */

static void
aarch64_emit_le_goto (int *offset_p, int *size_p)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_cmp (p, x1, register_operand (x0));
  /* Branch over the next instruction if x0 > x1.  */
  p += emit_bcond (p, GT, 8);
  /* The NOP instruction will be patched with an unconditional branch.  */
  if (offset_p)
    *offset_p = (p - buf) * 4;
  if (size_p)
    *size_p = 4;
  p += emit_nop (p);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_gt_goto".  */

static void
aarch64_emit_gt_goto (int *offset_p, int *size_p)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_cmp (p, x1, register_operand (x0));
  /* Branch over the next instruction if x0 <= x1.  */
  p += emit_bcond (p, LE, 8);
  /* The NOP instruction will be patched with an unconditional branch.  */
  if (offset_p)
    *offset_p = (p - buf) * 4;
  if (size_p)
    *size_p = 4;
  p += emit_nop (p);

  emit_ops_insns (buf, p - buf);
}

/* Implementation of emit_ops method "emit_ge_got".  */

static void
aarch64_emit_ge_got (int *offset_p, int *size_p)
{
  uint32_t buf[16];
  uint32_t *p = buf;

  p += emit_pop (p, x1);
  p += emit_cmp (p, x1, register_operand (x0));
  /* Branch over the next instruction if x0 <= x1.  */
  p += emit_bcond (p, LT, 8);
  /* The NOP instruction will be patched with an unconditional branch.  */
  if (offset_p)
    *offset_p = (p - buf) * 4;
  if (size_p)
    *size_p = 4;
  p += emit_nop (p);

  emit_ops_insns (buf, p - buf);
}

static struct emit_ops aarch64_emit_ops_impl =
{
  aarch64_emit_prologue,
  aarch64_emit_epilogue,
  aarch64_emit_add,
  aarch64_emit_sub,
  aarch64_emit_mul,
  aarch64_emit_lsh,
  aarch64_emit_rsh_signed,
  aarch64_emit_rsh_unsigned,
  aarch64_emit_ext,
  aarch64_emit_log_not,
  aarch64_emit_bit_and,
  aarch64_emit_bit_or,
  aarch64_emit_bit_xor,
  aarch64_emit_bit_not,
  aarch64_emit_equal,
  aarch64_emit_less_signed,
  aarch64_emit_less_unsigned,
  aarch64_emit_ref,
  aarch64_emit_if_goto,
  aarch64_emit_goto,
  aarch64_write_goto_address,
  aarch64_emit_const,
  aarch64_emit_call,
  aarch64_emit_reg,
  aarch64_emit_pop,
  aarch64_emit_stack_flush,
  aarch64_emit_zero_ext,
  aarch64_emit_swap,
  aarch64_emit_stack_adjust,
  aarch64_emit_int_call_1,
  aarch64_emit_void_call_2,
  aarch64_emit_eq_goto,
  aarch64_emit_ne_goto,
  aarch64_emit_lt_goto,
  aarch64_emit_le_goto,
  aarch64_emit_gt_goto,
  aarch64_emit_ge_got,
};

/* Implementation of target ops method "emit_ops".  */

emit_ops *
aarch64_target::emit_ops ()
{
  return &aarch64_emit_ops_impl;
}

/* Implementation of target ops method
   "get_min_fast_tracepoint_insn_len".  */

int
aarch64_target::get_min_fast_tracepoint_insn_len ()
{
  return 4;
}

/* Implementation of linux target ops method "low_supports_range_stepping".  */

bool
aarch64_target::low_supports_range_stepping ()
{
  return true;
}

/* Implementation of target ops method "sw_breakpoint_from_kind".  */

const gdb_byte *
aarch64_target::sw_breakpoint_from_kind (int kind, int *size)
{
  if (is_64bit_tdesc ())
    {
      *size = aarch64_breakpoint_len;
      return aarch64_breakpoint;
    }
  else
    return arm_sw_breakpoint_from_kind (kind, size);
}

/* Implementation of target ops method "breakpoint_kind_from_pc".  */

int
aarch64_target::breakpoint_kind_from_pc (CORE_ADDR *pcptr)
{
  if (is_64bit_tdesc ())
    return aarch64_breakpoint_len;
  else
    return arm_breakpoint_kind_from_pc (pcptr);
}

/* Implementation of the target ops method
   "breakpoint_kind_from_current_state".  */

int
aarch64_target::breakpoint_kind_from_current_state (CORE_ADDR *pcptr)
{
  if (is_64bit_tdesc ())
    return aarch64_breakpoint_len;
  else
    return arm_breakpoint_kind_from_current_state (pcptr);
}

/* Returns true if memory tagging is supported.  */
bool
aarch64_target::supports_memory_tagging ()
{
  if (current_thread == NULL)
    {
      /* We don't have any processes running, so don't attempt to
	 use linux_get_hwcap2 as it will try to fetch the current
	 thread id.  Instead, just fetch the auxv from the self
	 PID.  */
#ifdef HAVE_GETAUXVAL
      return (getauxval (AT_HWCAP2) & HWCAP2_MTE) != 0;
#else
      return true;
#endif
    }

  return (linux_get_hwcap2 (current_thread->id.pid (), 8) & HWCAP2_MTE) != 0;
}

bool
aarch64_target::fetch_memtags (CORE_ADDR address, size_t len,
			       gdb::byte_vector &tags, int type)
{
  /* Allocation tags are per-process, so any tid is fine.  */
  int tid = lwpid_of (current_thread);

  /* Allocation tag?  */
  if (type == static_cast <int> (aarch64_memtag_type::mte_allocation))
    return aarch64_mte_fetch_memtags (tid, address, len, tags);

  return false;
}

bool
aarch64_target::store_memtags (CORE_ADDR address, size_t len,
			       const gdb::byte_vector &tags, int type)
{
  /* Allocation tags are per-process, so any tid is fine.  */
  int tid = lwpid_of (current_thread);

  /* Allocation tag?  */
  if (type == static_cast <int> (aarch64_memtag_type::mte_allocation))
    return aarch64_mte_store_memtags (tid, address, len, tags);

  return false;
}

/* The linux target ops object.  */

linux_process_target *the_linux_target = &the_aarch64_target;

void
initialize_low_arch (void)
{
  initialize_low_arch_aarch32 ();

  initialize_regsets_info (&aarch64_regsets_info);
}
