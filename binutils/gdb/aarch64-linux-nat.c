/* Native-dependent code for GNU/Linux AArch64.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.
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

#include "defs.h"

#include "inferior.h"
#include "gdbcore.h"
#include "regcache.h"
#include "linux-nat.h"
#include "target-descriptions.h"
#include "auxv.h"
#include "gdbcmd.h"
#include "aarch64-nat.h"
#include "aarch64-tdep.h"
#include "aarch64-linux-tdep.h"
#include "aarch32-linux-nat.h"
#include "aarch32-tdep.h"
#include "arch/arm.h"
#include "nat/aarch64-linux.h"
#include "nat/aarch64-linux-hw-point.h"
#include "nat/aarch64-scalable-linux-ptrace.h"

#include "elf/external.h"
#include "elf/common.h"

#include "nat/gdb_ptrace.h"
#include <sys/utsname.h>
#include <asm/ptrace.h>

#include "gregset.h"
#include "linux-tdep.h"
#include "arm-tdep.h"

/* Defines ps_err_e, struct ps_prochandle.  */
#include "gdb_proc_service.h"
#include "arch-utils.h"

#include "arch/aarch64-mte-linux.h"

#include "nat/aarch64-mte-linux-ptrace.h"
#include "arch/aarch64-scalable-linux.h"

#include <string.h>

#ifndef TRAP_HWBKPT
#define TRAP_HWBKPT 0x0004
#endif

class aarch64_linux_nat_target final
  : public aarch64_nat_target<linux_nat_target>
{
public:
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  const struct target_desc *read_description () override;

  /* Add our hardware breakpoint and watchpoint implementation.  */
  bool stopped_by_watchpoint () override;
  bool stopped_data_address (CORE_ADDR *) override;

  int can_do_single_step () override;

  /* Override the GNU/Linux inferior startup hook.  */
  void post_startup_inferior (ptid_t) override;

  /* Override the GNU/Linux post attach hook.  */
  void post_attach (int pid) override;

  /* These three defer to common nat/ code.  */
  void low_new_thread (struct lwp_info *lp) override
  { aarch64_linux_new_thread (lp); }
  void low_delete_thread (struct arch_lwp_info *lp) override
  { aarch64_linux_delete_thread (lp); }
  void low_prepare_to_resume (struct lwp_info *lp) override
  { aarch64_linux_prepare_to_resume (lp); }

  void low_new_fork (struct lwp_info *parent, pid_t child_pid) override;
  void low_forget_process (pid_t pid) override;

  /* Add our siginfo layout converter.  */
  bool low_siginfo_fixup (siginfo_t *ptrace, gdb_byte *inf, int direction)
    override;

  struct gdbarch *thread_architecture (ptid_t) override;

  bool supports_memory_tagging () override;

  /* Read memory allocation tags from memory via PTRACE.  */
  bool fetch_memtags (CORE_ADDR address, size_t len,
		      gdb::byte_vector &tags, int type) override;

  /* Write allocation tags to memory via PTRACE.  */
  bool store_memtags (CORE_ADDR address, size_t len,
		      const gdb::byte_vector &tags, int type) override;
};

static aarch64_linux_nat_target the_aarch64_linux_nat_target;

/* Called whenever GDB is no longer debugging process PID.  It deletes
   data structures that keep track of debug register state.  */

void
aarch64_linux_nat_target::low_forget_process (pid_t pid)
{
  aarch64_remove_debug_reg_state (pid);
}

/* Fill GDB's register array with the general-purpose register values
   from the current thread.  */

static void
fetch_gregs_from_thread (struct regcache *regcache)
{
  int ret, tid;
  struct gdbarch *gdbarch = regcache->arch ();
  elf_gregset_t regs;
  struct iovec iovec;

  /* Make sure REGS can hold all registers contents on both aarch64
     and arm.  */
  static_assert (sizeof (regs) >= 18 * 4);

  tid = regcache->ptid ().lwp ();

  iovec.iov_base = &regs;
  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    iovec.iov_len = 18 * 4;
  else
    iovec.iov_len = sizeof (regs);

  ret = ptrace (PTRACE_GETREGSET, tid, NT_PRSTATUS, &iovec);
  if (ret < 0)
    perror_with_name (_("Unable to fetch general registers"));

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    aarch32_gp_regcache_supply (regcache, (uint32_t *) regs, 1);
  else
    {
      int regno;

      for (regno = AARCH64_X0_REGNUM; regno <= AARCH64_CPSR_REGNUM; regno++)
	regcache->raw_supply (regno, &regs[regno - AARCH64_X0_REGNUM]);
    }
}

/* Store to the current thread the valid general-purpose register
   values in the GDB's register array.  */

static void
store_gregs_to_thread (const struct regcache *regcache)
{
  int ret, tid;
  elf_gregset_t regs;
  struct iovec iovec;
  struct gdbarch *gdbarch = regcache->arch ();

  /* Make sure REGS can hold all registers contents on both aarch64
     and arm.  */
  static_assert (sizeof (regs) >= 18 * 4);
  tid = regcache->ptid ().lwp ();

  iovec.iov_base = &regs;
  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    iovec.iov_len = 18 * 4;
  else
    iovec.iov_len = sizeof (regs);

  ret = ptrace (PTRACE_GETREGSET, tid, NT_PRSTATUS, &iovec);
  if (ret < 0)
    perror_with_name (_("Unable to fetch general registers"));

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    aarch32_gp_regcache_collect (regcache, (uint32_t *) regs, 1);
  else
    {
      int regno;

      for (regno = AARCH64_X0_REGNUM; regno <= AARCH64_CPSR_REGNUM; regno++)
	if (REG_VALID == regcache->get_register_status (regno))
	  regcache->raw_collect (regno, &regs[regno - AARCH64_X0_REGNUM]);
    }

  ret = ptrace (PTRACE_SETREGSET, tid, NT_PRSTATUS, &iovec);
  if (ret < 0)
    perror_with_name (_("Unable to store general registers"));
}

/* Fill GDB's register array with the fp/simd register values
   from the current thread.  */

static void
fetch_fpregs_from_thread (struct regcache *regcache)
{
  int ret, tid;
  elf_fpregset_t regs;
  struct iovec iovec;
  struct gdbarch *gdbarch = regcache->arch ();

  /* Make sure REGS can hold all VFP registers contents on both aarch64
     and arm.  */
  static_assert (sizeof regs >= ARM_VFP3_REGS_SIZE);

  tid = regcache->ptid ().lwp ();

  iovec.iov_base = &regs;

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    {
      iovec.iov_len = ARM_VFP3_REGS_SIZE;

      ret = ptrace (PTRACE_GETREGSET, tid, NT_ARM_VFP, &iovec);
      if (ret < 0)
	perror_with_name (_("Unable to fetch VFP registers"));

      aarch32_vfp_regcache_supply (regcache, (gdb_byte *) &regs, 32);
    }
  else
    {
      int regno;

      iovec.iov_len = sizeof (regs);

      ret = ptrace (PTRACE_GETREGSET, tid, NT_FPREGSET, &iovec);
      if (ret < 0)
	perror_with_name (_("Unable to fetch vFP/SIMD registers"));

      for (regno = AARCH64_V0_REGNUM; regno <= AARCH64_V31_REGNUM; regno++)
	regcache->raw_supply (regno, &regs.vregs[regno - AARCH64_V0_REGNUM]);

      regcache->raw_supply (AARCH64_FPSR_REGNUM, &regs.fpsr);
      regcache->raw_supply (AARCH64_FPCR_REGNUM, &regs.fpcr);
    }
}

/* Store to the current thread the valid fp/simd register
   values in the GDB's register array.  */

static void
store_fpregs_to_thread (const struct regcache *regcache)
{
  int ret, tid;
  elf_fpregset_t regs;
  struct iovec iovec;
  struct gdbarch *gdbarch = regcache->arch ();

  /* Make sure REGS can hold all VFP registers contents on both aarch64
     and arm.  */
  static_assert (sizeof regs >= ARM_VFP3_REGS_SIZE);
  tid = regcache->ptid ().lwp ();

  iovec.iov_base = &regs;

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    {
      iovec.iov_len = ARM_VFP3_REGS_SIZE;

      ret = ptrace (PTRACE_GETREGSET, tid, NT_ARM_VFP, &iovec);
      if (ret < 0)
	perror_with_name (_("Unable to fetch VFP registers"));

      aarch32_vfp_regcache_collect (regcache, (gdb_byte *) &regs, 32);
    }
  else
    {
      int regno;

      iovec.iov_len = sizeof (regs);

      ret = ptrace (PTRACE_GETREGSET, tid, NT_FPREGSET, &iovec);
      if (ret < 0)
	perror_with_name (_("Unable to fetch FP/SIMD registers"));

      for (regno = AARCH64_V0_REGNUM; regno <= AARCH64_V31_REGNUM; regno++)
	if (REG_VALID == regcache->get_register_status (regno))
	  regcache->raw_collect
	    (regno, (char *) &regs.vregs[regno - AARCH64_V0_REGNUM]);

      if (REG_VALID == regcache->get_register_status (AARCH64_FPSR_REGNUM))
	regcache->raw_collect (AARCH64_FPSR_REGNUM, (char *) &regs.fpsr);
      if (REG_VALID == regcache->get_register_status (AARCH64_FPCR_REGNUM))
	regcache->raw_collect (AARCH64_FPCR_REGNUM, (char *) &regs.fpcr);
    }

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    {
      ret = ptrace (PTRACE_SETREGSET, tid, NT_ARM_VFP, &iovec);
      if (ret < 0)
	perror_with_name (_("Unable to store VFP registers"));
    }
  else
    {
      ret = ptrace (PTRACE_SETREGSET, tid, NT_FPREGSET, &iovec);
      if (ret < 0)
	perror_with_name (_("Unable to store FP/SIMD registers"));
    }
}

/* Fill GDB's REGCACHE with the valid SVE register values from the thread
   associated with REGCACHE.

   This function handles reading data from SVE or SSVE states, depending
   on which state is active at the moment.  */

static void
fetch_sveregs_from_thread (struct regcache *regcache)
{
  /* Fetch SVE state from the thread and copy it into the register cache.  */
  aarch64_sve_regs_copy_to_reg_buf (regcache->ptid ().lwp (), regcache);
}

/* Store the valid SVE register values from GDB's REGCACHE to the thread
   associated with REGCACHE.

   This function handles writing data to SVE or SSVE states, depending
   on which state is active at the moment.  */

static void
store_sveregs_to_thread (struct regcache *regcache)
{
  /* Fetch SVE state from the register cache and update the thread TID with
     it.  */
  aarch64_sve_regs_copy_from_reg_buf (regcache->ptid ().lwp (), regcache);
}

/* Fill GDB's REGCACHE with the ZA register set contents from the
   thread associated with REGCACHE.  If there is no active ZA register state,
   make the ZA register contents zero.  */

static void
fetch_za_from_thread (struct regcache *regcache)
{
  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (regcache->arch ());

  /* Read ZA state from the thread to the register cache.  */
  aarch64_za_regs_copy_to_reg_buf (regcache->ptid ().lwp (),
				   regcache,
				   tdep->sme_za_regnum,
				   tdep->sme_svg_regnum,
				   tdep->sme_svcr_regnum);
}

/* Store the NT_ARM_ZA register set contents from GDB's REGCACHE to the thread
   associated with REGCACHE.  */

static void
store_za_to_thread (struct regcache *regcache)
{
  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (regcache->arch ());

  /* Write ZA state from the register cache to the thread.  */
  aarch64_za_regs_copy_from_reg_buf (regcache->ptid ().lwp (),
				     regcache,
				     tdep->sme_za_regnum,
				     tdep->sme_svg_regnum,
				     tdep->sme_svcr_regnum);
}

/* Fill GDB's REGCACHE with the ZT register set contents from the
   thread associated with REGCACHE.  If there is no active ZA register state,
   make the ZT register contents zero.  */

static void
fetch_zt_from_thread (struct regcache *regcache)
{
  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (regcache->arch ());

  /* Read ZT state from the thread to the register cache.  */
  aarch64_zt_regs_copy_to_reg_buf (regcache->ptid ().lwp (),
				   regcache,
				   tdep->sme2_zt0_regnum);
}

/* Store the NT_ARM_ZT register set contents from GDB's REGCACHE to the
   thread associated with REGCACHE.  */

static void
store_zt_to_thread (struct regcache *regcache)
{
  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (regcache->arch ());

  /* Write ZT state from the register cache to the thread.  */
  aarch64_zt_regs_copy_from_reg_buf (regcache->ptid ().lwp (),
				     regcache,
				     tdep->sme2_zt0_regnum);
}

/* Fill GDB's register array with the pointer authentication mask values from
   the current thread.  */

static void
fetch_pauth_masks_from_thread (struct regcache *regcache)
{
  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (regcache->arch ());
  int ret;
  struct iovec iovec;
  uint64_t pauth_regset[2] = {0, 0};
  int tid = regcache->ptid ().lwp ();

  iovec.iov_base = &pauth_regset;
  iovec.iov_len = sizeof (pauth_regset);

  ret = ptrace (PTRACE_GETREGSET, tid, NT_ARM_PAC_MASK, &iovec);
  if (ret != 0)
    perror_with_name (_("unable to fetch pauth registers"));

  regcache->raw_supply (AARCH64_PAUTH_DMASK_REGNUM (tdep->pauth_reg_base),
			&pauth_regset[0]);
  regcache->raw_supply (AARCH64_PAUTH_CMASK_REGNUM (tdep->pauth_reg_base),
			&pauth_regset[1]);
}

/* Fill GDB's register array with the MTE register values from
   the current thread.  */

static void
fetch_mteregs_from_thread (struct regcache *regcache)
{
  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (regcache->arch ());
  int regno = tdep->mte_reg_base;

  gdb_assert (regno != -1);

  uint64_t tag_ctl = 0;
  struct iovec iovec;

  iovec.iov_base = &tag_ctl;
  iovec.iov_len = sizeof (tag_ctl);

  int tid = get_ptrace_pid (regcache->ptid ());
  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_TAGGED_ADDR_CTRL, &iovec) != 0)
      perror_with_name (_("unable to fetch MTE registers"));

  regcache->raw_supply (regno, &tag_ctl);
}

/* Store to the current thread the valid MTE register set in the GDB's
   register array.  */

static void
store_mteregs_to_thread (struct regcache *regcache)
{
  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (regcache->arch ());
  int regno = tdep->mte_reg_base;

  gdb_assert (regno != -1);

  uint64_t tag_ctl = 0;

  if (REG_VALID != regcache->get_register_status (regno))
    return;

  regcache->raw_collect (regno, (char *) &tag_ctl);

  struct iovec iovec;

  iovec.iov_base = &tag_ctl;
  iovec.iov_len = sizeof (tag_ctl);

  int tid = get_ptrace_pid (regcache->ptid ());
  if (ptrace (PTRACE_SETREGSET, tid, NT_ARM_TAGGED_ADDR_CTRL, &iovec) != 0)
    perror_with_name (_("unable to store MTE registers"));
}

/* Fill GDB's register array with the TLS register values from
   the current thread.  */

static void
fetch_tlsregs_from_thread (struct regcache *regcache)
{
  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (regcache->arch ());
  int regno = tdep->tls_regnum_base;

  gdb_assert (regno != -1);
  gdb_assert (tdep->tls_register_count > 0);

  uint64_t tpidrs[tdep->tls_register_count];
  memset(tpidrs, 0, sizeof(tpidrs));

  struct iovec iovec;
  iovec.iov_base = tpidrs;
  iovec.iov_len = sizeof (tpidrs);

  int tid = get_ptrace_pid (regcache->ptid ());
  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_TLS, &iovec) != 0)
      perror_with_name (_("unable to fetch TLS registers"));

  for (int i = 0; i < tdep->tls_register_count; i++)
    regcache->raw_supply (regno + i, &tpidrs[i]);
}

/* Store to the current thread the valid TLS register set in GDB's
   register array.  */

static void
store_tlsregs_to_thread (struct regcache *regcache)
{
  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (regcache->arch ());
  int regno = tdep->tls_regnum_base;

  gdb_assert (regno != -1);
  gdb_assert (tdep->tls_register_count > 0);

  uint64_t tpidrs[tdep->tls_register_count];
  memset(tpidrs, 0, sizeof(tpidrs));

  for (int i = 0; i < tdep->tls_register_count; i++)
    {
      if (REG_VALID != regcache->get_register_status (regno + i))
	continue;

      regcache->raw_collect (regno + i, (char *) &tpidrs[i]);
    }

  struct iovec iovec;
  iovec.iov_base = &tpidrs;
  iovec.iov_len = sizeof (tpidrs);

  int tid = get_ptrace_pid (regcache->ptid ());
  if (ptrace (PTRACE_SETREGSET, tid, NT_ARM_TLS, &iovec) != 0)
    perror_with_name (_("unable to store TLS register"));
}

/* The AArch64 version of the "fetch_registers" target_ops method.  Fetch
   REGNO from the target and place the result into REGCACHE.  */

static void
aarch64_fetch_registers (struct regcache *regcache, int regno)
{
  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (regcache->arch ());

  /* Do we need to fetch all registers?  */
  if (regno == -1)
    {
      fetch_gregs_from_thread (regcache);

      /* We attempt to fetch SVE registers if there is support for either
	 SVE or SME (due to the SSVE state of SME).  */
      if (tdep->has_sve () || tdep->has_sme ())
	fetch_sveregs_from_thread (regcache);
      else
	fetch_fpregs_from_thread (regcache);

      if (tdep->has_pauth ())
	fetch_pauth_masks_from_thread (regcache);

      if (tdep->has_mte ())
	fetch_mteregs_from_thread (regcache);

      if (tdep->has_tls ())
	fetch_tlsregs_from_thread (regcache);

      if (tdep->has_sme ())
	fetch_za_from_thread (regcache);

      if (tdep->has_sme2 ())
	fetch_zt_from_thread (regcache);
    }
  /* General purpose register?  */
  else if (regno < AARCH64_V0_REGNUM)
    fetch_gregs_from_thread (regcache);
  /* SVE register?  */
  else if ((tdep->has_sve () || tdep->has_sme ())
	   && regno <= AARCH64_SVE_VG_REGNUM)
    fetch_sveregs_from_thread (regcache);
  /* FPSIMD register?  */
  else if (regno <= AARCH64_FPCR_REGNUM)
    fetch_fpregs_from_thread (regcache);
  /* PAuth register?  */
  else if (tdep->has_pauth ()
	   && (regno == AARCH64_PAUTH_DMASK_REGNUM (tdep->pauth_reg_base)
	       || regno == AARCH64_PAUTH_CMASK_REGNUM (tdep->pauth_reg_base)))
    fetch_pauth_masks_from_thread (regcache);
  /* SME register?  */
  else if (tdep->has_sme () && regno >= tdep->sme_reg_base
	   && regno < tdep->sme_reg_base + 3)
    fetch_za_from_thread (regcache);
  /* SME2 register?  */
  else if (tdep->has_sme2 () && regno == tdep->sme2_zt0_regnum)
    fetch_zt_from_thread (regcache);
  /* MTE register?  */
  else if (tdep->has_mte ()
	   && (regno == tdep->mte_reg_base))
    fetch_mteregs_from_thread (regcache);
  /* TLS register?  */
  else if (tdep->has_tls ()
	   && regno >= tdep->tls_regnum_base
	   && regno < tdep->tls_regnum_base + tdep->tls_register_count)
    fetch_tlsregs_from_thread (regcache);
}

/* A version of the "fetch_registers" target_ops method used when running
   32-bit ARM code on an AArch64 target.  Fetch REGNO from the target and
   place the result into REGCACHE.  */

static void
aarch32_fetch_registers (struct regcache *regcache, int regno)
{
  arm_gdbarch_tdep *tdep
    = gdbarch_tdep<arm_gdbarch_tdep> (regcache->arch ());

  if (regno == -1)
    {
      fetch_gregs_from_thread (regcache);
      if (tdep->vfp_register_count > 0)
	fetch_fpregs_from_thread (regcache);
    }
  else if (regno < ARM_F0_REGNUM || regno == ARM_PS_REGNUM)
    fetch_gregs_from_thread (regcache);
  else if (tdep->vfp_register_count > 0
	   && regno >= ARM_D0_REGNUM
	   && (regno < ARM_D0_REGNUM + tdep->vfp_register_count
	       || regno == ARM_FPSCR_REGNUM))
    fetch_fpregs_from_thread (regcache);
}

/* Implement the "fetch_registers" target_ops method.  */

void
aarch64_linux_nat_target::fetch_registers (struct regcache *regcache,
					   int regno)
{
  if (gdbarch_bfd_arch_info (regcache->arch ())->bits_per_word == 32)
    aarch32_fetch_registers (regcache, regno);
  else
    aarch64_fetch_registers (regcache, regno);
}

/* The AArch64 version of the "store_registers" target_ops method.  Copy
   the value of register REGNO from REGCACHE into the the target.  */

static void
aarch64_store_registers (struct regcache *regcache, int regno)
{
  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (regcache->arch ());

  /* Do we need to store all registers?  */
  if (regno == -1)
    {
      store_gregs_to_thread (regcache);

      /* We attempt to store SVE registers if there is support for either
	 SVE or SME (due to the SSVE state of SME).  */
      if (tdep->has_sve () || tdep->has_sme ())
	store_sveregs_to_thread (regcache);
      else
	store_fpregs_to_thread (regcache);

      if (tdep->has_mte ())
	store_mteregs_to_thread (regcache);

      if (tdep->has_tls ())
	store_tlsregs_to_thread (regcache);

      if (tdep->has_sme ())
	store_za_to_thread (regcache);

      if (tdep->has_sme2 ())
	store_zt_to_thread (regcache);
    }
  /* General purpose register?  */
  else if (regno < AARCH64_V0_REGNUM)
    store_gregs_to_thread (regcache);
  /* SVE register?  */
  else if ((tdep->has_sve () || tdep->has_sme ())
	   && regno <= AARCH64_SVE_VG_REGNUM)
    store_sveregs_to_thread (regcache);
  /* FPSIMD register?  */
  else if (regno <= AARCH64_FPCR_REGNUM)
    store_fpregs_to_thread (regcache);
  /* SME register?  */
  else if (tdep->has_sme () && regno >= tdep->sme_reg_base
	   && regno < tdep->sme_reg_base + 3)
    store_za_to_thread (regcache);
  else if (tdep->has_sme2 () && regno == tdep->sme2_zt0_regnum)
    store_zt_to_thread (regcache);
  /* MTE register?  */
  else if (tdep->has_mte ()
	   && (regno == tdep->mte_reg_base))
    store_mteregs_to_thread (regcache);
  /* TLS register?  */
  else if (tdep->has_tls ()
	   && regno >= tdep->tls_regnum_base
	   && regno < tdep->tls_regnum_base + tdep->tls_register_count)
    store_tlsregs_to_thread (regcache);

  /* PAuth registers are read-only.  */
}

/* A version of the "store_registers" target_ops method used when running
   32-bit ARM code on an AArch64 target.  Copy the value of register REGNO
   from REGCACHE into the the target.  */

static void
aarch32_store_registers (struct regcache *regcache, int regno)
{
  arm_gdbarch_tdep *tdep
    = gdbarch_tdep<arm_gdbarch_tdep> (regcache->arch ());

  if (regno == -1)
    {
      store_gregs_to_thread (regcache);
      if (tdep->vfp_register_count > 0)
	store_fpregs_to_thread (regcache);
    }
  else if (regno < ARM_F0_REGNUM || regno == ARM_PS_REGNUM)
    store_gregs_to_thread (regcache);
  else if (tdep->vfp_register_count > 0
	   && regno >= ARM_D0_REGNUM
	   && (regno < ARM_D0_REGNUM + tdep->vfp_register_count
	       || regno == ARM_FPSCR_REGNUM))
    store_fpregs_to_thread (regcache);
}

/* Implement the "store_registers" target_ops method.  */

void
aarch64_linux_nat_target::store_registers (struct regcache *regcache,
					   int regno)
{
  if (gdbarch_bfd_arch_info (regcache->arch ())->bits_per_word == 32)
    aarch32_store_registers (regcache, regno);
  else
    aarch64_store_registers (regcache, regno);
}

/* Fill register REGNO (if it is a general-purpose register) in
   *GREGSETPS with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */

void
fill_gregset (const struct regcache *regcache,
	      gdb_gregset_t *gregsetp, int regno)
{
  regcache_collect_regset (&aarch64_linux_gregset, regcache,
			   regno, (gdb_byte *) gregsetp,
			   AARCH64_LINUX_SIZEOF_GREGSET);
}

/* Fill GDB's register array with the general-purpose register values
   in *GREGSETP.  */

void
supply_gregset (struct regcache *regcache, const gdb_gregset_t *gregsetp)
{
  regcache_supply_regset (&aarch64_linux_gregset, regcache, -1,
			  (const gdb_byte *) gregsetp,
			  AARCH64_LINUX_SIZEOF_GREGSET);
}

/* Fill register REGNO (if it is a floating-point register) in
   *FPREGSETP with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */

void
fill_fpregset (const struct regcache *regcache,
	       gdb_fpregset_t *fpregsetp, int regno)
{
  regcache_collect_regset (&aarch64_linux_fpregset, regcache,
			   regno, (gdb_byte *) fpregsetp,
			   AARCH64_LINUX_SIZEOF_FPREGSET);
}

/* Fill GDB's register array with the floating-point register values
   in *FPREGSETP.  */

void
supply_fpregset (struct regcache *regcache, const gdb_fpregset_t *fpregsetp)
{
  regcache_supply_regset (&aarch64_linux_fpregset, regcache, -1,
			  (const gdb_byte *) fpregsetp,
			  AARCH64_LINUX_SIZEOF_FPREGSET);
}

/* linux_nat_new_fork hook.   */

void
aarch64_linux_nat_target::low_new_fork (struct lwp_info *parent,
					pid_t child_pid)
{
  pid_t parent_pid;
  struct aarch64_debug_reg_state *parent_state;
  struct aarch64_debug_reg_state *child_state;

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
  parent_state = aarch64_get_debug_reg_state (parent_pid);
  child_state = aarch64_get_debug_reg_state (child_pid);
  *child_state = *parent_state;
}


/* Called by libthread_db.  Returns a pointer to the thread local
   storage (or its descriptor).  */

ps_err_e
ps_get_thread_area (struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
  gdbarch *arch = current_inferior ()->arch ();
  int is_64bit_p = (gdbarch_bfd_arch_info (arch)->bits_per_word == 64);

  return aarch64_ps_get_thread_area (ph, lwpid, idx, base, is_64bit_p);
}


/* Implement the virtual inf_ptrace_target::post_startup_inferior method.  */

void
aarch64_linux_nat_target::post_startup_inferior (ptid_t ptid)
{
  low_forget_process (ptid.pid ());
  aarch64_linux_get_debug_reg_capacity (ptid.pid ());
  linux_nat_target::post_startup_inferior (ptid);
}

/* Implement the "post_attach" target_ops method.  */

void
aarch64_linux_nat_target::post_attach (int pid)
{
  low_forget_process (pid);
  /* Set the hardware debug register capacity.  If
     aarch64_linux_get_debug_reg_capacity is not called
     (as it is in aarch64_linux_child_post_startup_inferior) then
     software watchpoints will be used instead of hardware
     watchpoints when attaching to a target.  */
  aarch64_linux_get_debug_reg_capacity (pid);
  linux_nat_target::post_attach (pid);
}

/* Implement the "read_description" target_ops method.  */

const struct target_desc *
aarch64_linux_nat_target::read_description ()
{
  int ret, tid;
  gdb_byte regbuf[ARM_VFP3_REGS_SIZE];
  struct iovec iovec;

  if (inferior_ptid == null_ptid)
    return this->beneath ()->read_description ();

  tid = inferior_ptid.pid ();

  iovec.iov_base = regbuf;
  iovec.iov_len = ARM_VFP3_REGS_SIZE;

  ret = ptrace (PTRACE_GETREGSET, tid, NT_ARM_VFP, &iovec);
  if (ret == 0)
    return aarch32_read_description ();

  CORE_ADDR hwcap = linux_get_hwcap ();
  CORE_ADDR hwcap2 = linux_get_hwcap2 ();

  aarch64_features features;
  /* SVE/SSVE check.  Reading VQ may return either the regular vector length
     or the streaming vector length, depending on whether streaming mode is
     active or not.  */
  features.vq = aarch64_sve_get_vq (tid);
  features.pauth = hwcap & AARCH64_HWCAP_PACA;
  features.mte = hwcap2 & HWCAP2_MTE;
  features.tls = aarch64_tls_register_count (tid);
  /* SME feature check.  */
  features.svq = aarch64_za_get_svq (tid);

  /* Check for SME2 support.  */
  if ((hwcap2 & HWCAP2_SME2) || (hwcap2 & HWCAP2_SME2P1))
    features.sme2 = supports_zt_registers (tid);

  return aarch64_read_description (features);
}

/* Convert a native/host siginfo object, into/from the siginfo in the
   layout of the inferiors' architecture.  Returns true if any
   conversion was done; false otherwise.  If DIRECTION is 1, then copy
   from INF to NATIVE.  If DIRECTION is 0, copy from NATIVE to
   INF.  */

bool
aarch64_linux_nat_target::low_siginfo_fixup (siginfo_t *native, gdb_byte *inf,
					     int direction)
{
  struct gdbarch *gdbarch = get_frame_arch (get_current_frame ());

  /* Is the inferior 32-bit?  If so, then do fixup the siginfo
     object.  */
  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
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

/* Implement the "stopped_data_address" target_ops method.  */

bool
aarch64_linux_nat_target::stopped_data_address (CORE_ADDR *addr_p)
{
  siginfo_t siginfo;
  struct aarch64_debug_reg_state *state;

  if (!linux_nat_get_siginfo (inferior_ptid, &siginfo))
    return false;

  /* This must be a hardware breakpoint.  */
  if (siginfo.si_signo != SIGTRAP
      || (siginfo.si_code & 0xffff) != TRAP_HWBKPT)
    return false;

  /* Make sure to ignore the top byte, otherwise we may not recognize a
     hardware watchpoint hit.  The stopped data addresses coming from the
     kernel can potentially be tagged addresses.  */
  struct gdbarch *gdbarch = thread_architecture (inferior_ptid);
  const CORE_ADDR addr_trap
    = gdbarch_remove_non_address_bits (gdbarch, (CORE_ADDR) siginfo.si_addr);

  /* Check if the address matches any watched address.  */
  state = aarch64_get_debug_reg_state (inferior_ptid.pid ());
  return aarch64_stopped_data_address (state, addr_trap, addr_p);
}

/* Implement the "stopped_by_watchpoint" target_ops method.  */

bool
aarch64_linux_nat_target::stopped_by_watchpoint ()
{
  CORE_ADDR addr;

  return stopped_data_address (&addr);
}

/* Implement the "can_do_single_step" target_ops method.  */

int
aarch64_linux_nat_target::can_do_single_step ()
{
  return 1;
}

/* Implement the "thread_architecture" target_ops method.

   Returns the gdbarch for the thread identified by PTID.  If the thread in
   question is a 32-bit ARM thread, then the architecture returned will be
   that of the process itself.

   If the thread is an AArch64 thread then we need to check the current
   vector length; if the vector length has changed then we need to lookup a
   new gdbarch that matches the new vector length.  */

struct gdbarch *
aarch64_linux_nat_target::thread_architecture (ptid_t ptid)
{
  /* Find the current gdbarch the same way as process_stratum_target.  */
  inferior *inf = find_inferior_ptid (this, ptid);
  gdb_assert (inf != NULL);

  /* If this is a 32-bit architecture, then this is ARM, not AArch64.
     There's no SVE vectors here, so just return the inferior
     architecture.  */
  if (gdbarch_bfd_arch_info (inf->arch ())->bits_per_word == 32)
    return inf->arch ();

  /* Only return the inferior's gdbarch if both vq and svq match the ones in
     the tdep.  */
  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (inf->arch ());
  uint64_t vq = aarch64_sve_get_vq (ptid.lwp ());
  uint64_t svq = aarch64_za_get_svq (ptid.lwp ());
  if (vq == tdep->vq && svq == tdep->sme_svq)
    return inf->arch ();

  /* We reach here if any vector length for the thread is different from its
     value at process start.  Lookup gdbarch via info (potentially creating a
     new one) by using a target description that corresponds to the new vq/svq
     value and the current architecture features.  */

  const struct target_desc *tdesc = gdbarch_target_desc (inf->arch ());
  aarch64_features features = aarch64_features_from_target_desc (tdesc);
  features.vq = vq;
  features.svq = svq;

  /* Check for the SME2 feature.  */
  features.sme2 = supports_zt_registers (ptid.lwp ());

  struct gdbarch_info info;
  info.bfd_arch_info = bfd_lookup_arch (bfd_arch_aarch64, bfd_mach_aarch64);
  info.target_desc = aarch64_read_description (features);
  return gdbarch_find_by_info (info);
}

/* Implement the "supports_memory_tagging" target_ops method.  */

bool
aarch64_linux_nat_target::supports_memory_tagging ()
{
  return (linux_get_hwcap2 () & HWCAP2_MTE) != 0;
}

/* Implement the "fetch_memtags" target_ops method.  */

bool
aarch64_linux_nat_target::fetch_memtags (CORE_ADDR address, size_t len,
					 gdb::byte_vector &tags, int type)
{
  int tid = get_ptrace_pid (inferior_ptid);

  /* Allocation tags?  */
  if (type == static_cast<int> (aarch64_memtag_type::mte_allocation))
    return aarch64_mte_fetch_memtags (tid, address, len, tags);

  return false;
}

/* Implement the "store_memtags" target_ops method.  */

bool
aarch64_linux_nat_target::store_memtags (CORE_ADDR address, size_t len,
					 const gdb::byte_vector &tags, int type)
{
  int tid = get_ptrace_pid (inferior_ptid);

  /* Allocation tags?  */
  if (type == static_cast<int> (aarch64_memtag_type::mte_allocation))
    return aarch64_mte_store_memtags (tid, address, len, tags);

  return false;
}

void _initialize_aarch64_linux_nat ();
void
_initialize_aarch64_linux_nat ()
{
  aarch64_initialize_hw_point ();

  /* Register the target.  */
  linux_target = &the_aarch64_linux_nat_target;
  add_inf_child_target (&the_aarch64_linux_nat_target);
}
