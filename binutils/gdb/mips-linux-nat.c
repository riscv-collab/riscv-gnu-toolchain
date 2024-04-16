/* Native-dependent code for GNU/Linux on MIPS processors.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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
#include "command.h"
#include "gdbcmd.h"
#include "inferior.h"
#include "mips-tdep.h"
#include "target.h"
#include "regcache.h"
#include "linux-nat-trad.h"
#include "mips-linux-tdep.h"
#include "target-descriptions.h"

#include "gdb_proc_service.h"
#include "gregset.h"

#include <sgidefs.h>
#include "nat/gdb_ptrace.h"
#include <asm/ptrace.h>
#include "inf-ptrace.h"

#include "nat/mips-linux-watch.h"

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 25
#endif

class mips_linux_nat_target final : public linux_nat_trad_target
{
public:
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  void close () override;

  int can_use_hw_breakpoint (enum bptype, int, int) override;

  int remove_watchpoint (CORE_ADDR, int, enum target_hw_bp_type,
			 struct expression *) override;

  int insert_watchpoint (CORE_ADDR, int, enum target_hw_bp_type,
			 struct expression *) override;

  bool stopped_by_watchpoint () override;

  bool stopped_data_address (CORE_ADDR *) override;

  int region_ok_for_hw_watchpoint (CORE_ADDR, int) override;

  const struct target_desc *read_description () override;

protected:
  /* Override linux_nat_trad_target methods.  */
  CORE_ADDR register_u_offset (struct gdbarch *gdbarch,
			       int regno, int store_p) override;

  /* Override linux_nat_target low methods.  */
  void low_new_thread (struct lwp_info *lp) override;

private:
  /* Helpers.  See definitions.  */
  void mips64_regsets_store_registers (struct regcache *regcache,
				       int regno);
  void mips64_regsets_fetch_registers (struct regcache *regcache,
				       int regno);
};

static mips_linux_nat_target the_mips_linux_nat_target;

/* Assume that we have PTRACE_GETREGS et al. support.  If we do not,
   we'll clear this and use PTRACE_PEEKUSER instead.  */
static int have_ptrace_regsets = 1;

/* Map gdb internal register number to ptrace ``address''.
   These ``addresses'' are normally defined in <asm/ptrace.h>. 

   ptrace does not provide a way to read (or set) MIPS_PS_REGNUM,
   and there's no point in reading or setting MIPS_ZERO_REGNUM.
   We also can not set BADVADDR, CAUSE, or FCRIR via ptrace().  */

static CORE_ADDR
mips_linux_register_addr (struct gdbarch *gdbarch, int regno, int store)
{
  CORE_ADDR regaddr;

  if (regno < 0 || regno >= gdbarch_num_regs (gdbarch))
    error (_("Bogon register number %d."), regno);

  if (regno > MIPS_ZERO_REGNUM && regno < MIPS_ZERO_REGNUM + 32)
    regaddr = regno;
  else if ((regno >= mips_regnum (gdbarch)->fp0)
	   && (regno < mips_regnum (gdbarch)->fp0 + 32))
    regaddr = FPR_BASE + (regno - mips_regnum (gdbarch)->fp0);
  else if (regno == mips_regnum (gdbarch)->pc)
    regaddr = PC;
  else if (regno == mips_regnum (gdbarch)->cause)
    regaddr = store? (CORE_ADDR) -1 : CAUSE;
  else if (regno == mips_regnum (gdbarch)->badvaddr)
    regaddr = store? (CORE_ADDR) -1 : BADVADDR;
  else if (regno == mips_regnum (gdbarch)->lo)
    regaddr = MMLO;
  else if (regno == mips_regnum (gdbarch)->hi)
    regaddr = MMHI;
  else if (regno == mips_regnum (gdbarch)->fp_control_status)
    regaddr = FPC_CSR;
  else if (regno == mips_regnum (gdbarch)->fp_implementation_revision)
    regaddr = store? (CORE_ADDR) -1 : FPC_EIR;
  else if (mips_regnum (gdbarch)->dspacc != -1
	   && regno >= mips_regnum (gdbarch)->dspacc
	   && regno < mips_regnum (gdbarch)->dspacc + 6)
    regaddr = DSP_BASE + (regno - mips_regnum (gdbarch)->dspacc);
  else if (regno == mips_regnum (gdbarch)->dspctl)
    regaddr = DSP_CONTROL;
  else if (mips_linux_restart_reg_p (gdbarch) && regno == MIPS_RESTART_REGNUM)
    regaddr = 0;
  else
    regaddr = (CORE_ADDR) -1;

  return regaddr;
}

static CORE_ADDR
mips64_linux_register_addr (struct gdbarch *gdbarch, int regno, int store)
{
  CORE_ADDR regaddr;

  if (regno < 0 || regno >= gdbarch_num_regs (gdbarch))
    error (_("Bogon register number %d."), regno);

  /* On n32 we can't access 64-bit registers via PTRACE_PEEKUSR
     or PTRACE_POKEUSR.  */
  if (register_size (gdbarch, regno) > sizeof (PTRACE_TYPE_RET))
    return (CORE_ADDR) -1;

  if (regno > MIPS_ZERO_REGNUM && regno < MIPS_ZERO_REGNUM + 32)
    regaddr = regno;
  else if ((regno >= mips_regnum (gdbarch)->fp0)
	   && (regno < mips_regnum (gdbarch)->fp0 + 32))
    regaddr = MIPS64_FPR_BASE + (regno - gdbarch_fp0_regnum (gdbarch));
  else if (regno == mips_regnum (gdbarch)->pc)
    regaddr = MIPS64_PC;
  else if (regno == mips_regnum (gdbarch)->cause)
    regaddr = store? (CORE_ADDR) -1 : MIPS64_CAUSE;
  else if (regno == mips_regnum (gdbarch)->badvaddr)
    regaddr = store? (CORE_ADDR) -1 : MIPS64_BADVADDR;
  else if (regno == mips_regnum (gdbarch)->lo)
    regaddr = MIPS64_MMLO;
  else if (regno == mips_regnum (gdbarch)->hi)
    regaddr = MIPS64_MMHI;
  else if (regno == mips_regnum (gdbarch)->fp_control_status)
    regaddr = MIPS64_FPC_CSR;
  else if (regno == mips_regnum (gdbarch)->fp_implementation_revision)
    regaddr = store? (CORE_ADDR) -1 : MIPS64_FPC_EIR;
  else if (mips_regnum (gdbarch)->dspacc != -1
	   && regno >= mips_regnum (gdbarch)->dspacc
	   && regno < mips_regnum (gdbarch)->dspacc + 6)
    regaddr = DSP_BASE + (regno - mips_regnum (gdbarch)->dspacc);
  else if (regno == mips_regnum (gdbarch)->dspctl)
    regaddr = DSP_CONTROL;
  else if (mips_linux_restart_reg_p (gdbarch) && regno == MIPS_RESTART_REGNUM)
    regaddr = 0;
  else
    regaddr = (CORE_ADDR) -1;

  return regaddr;
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

/* Wrapper functions.  These are only used by libthread_db.  */

void
supply_gregset (struct regcache *regcache, const gdb_gregset_t *gregsetp)
{
  if (mips_isa_regsize (regcache->arch ()) == 4)
    mips_supply_gregset (regcache, (const mips_elf_gregset_t *) gregsetp);
  else
    mips64_supply_gregset (regcache, (const mips64_elf_gregset_t *) gregsetp);
}

void
fill_gregset (const struct regcache *regcache,
	      gdb_gregset_t *gregsetp, int regno)
{
  if (mips_isa_regsize (regcache->arch ()) == 4)
    mips_fill_gregset (regcache, (mips_elf_gregset_t *) gregsetp, regno);
  else
    mips64_fill_gregset (regcache, (mips64_elf_gregset_t *) gregsetp, regno);
}

void
supply_fpregset (struct regcache *regcache, const gdb_fpregset_t *fpregsetp)
{
  mips64_supply_fpregset (regcache, (const mips64_elf_fpregset_t *) fpregsetp);
}

void
fill_fpregset (const struct regcache *regcache,
	       gdb_fpregset_t *fpregsetp, int regno)
{
  mips64_fill_fpregset (regcache, (mips64_elf_fpregset_t *) fpregsetp, regno);
}


/* Fetch REGNO (or all registers if REGNO == -1) from the target
   using PTRACE_GETREGS et al.  */

void
mips_linux_nat_target::mips64_regsets_fetch_registers
  (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  int is_fp, is_dsp;
  int have_dsp;
  int regi;
  int tid;

  if (regno >= mips_regnum (gdbarch)->fp0
      && regno <= mips_regnum (gdbarch)->fp0 + 32)
    is_fp = 1;
  else if (regno == mips_regnum (gdbarch)->fp_control_status)
    is_fp = 1;
  else if (regno == mips_regnum (gdbarch)->fp_implementation_revision)
    is_fp = 1;
  else
    is_fp = 0;

  /* DSP registers are optional and not a part of any set.  */
  have_dsp = mips_regnum (gdbarch)->dspctl != -1;
  if (!have_dsp)
    is_dsp = 0;
  else if (regno >= mips_regnum (gdbarch)->dspacc
      && regno < mips_regnum (gdbarch)->dspacc + 6)
    is_dsp = 1;
  else if (regno == mips_regnum (gdbarch)->dspctl)
    is_dsp = 1;
  else
    is_dsp = 0;

  tid = get_ptrace_pid (regcache->ptid ());

  if (regno == -1 || (!is_fp && !is_dsp))
    {
      mips64_elf_gregset_t regs;

      if (ptrace (PTRACE_GETREGS, tid, 0L, (PTRACE_TYPE_ARG3) &regs) == -1)
	{
	  if (errno == EIO)
	    {
	      have_ptrace_regsets = 0;
	      return;
	    }
	  perror_with_name (_("Couldn't get registers"));
	}

      mips64_supply_gregset (regcache,
			     (const mips64_elf_gregset_t *) &regs);
    }

  if (regno == -1 || is_fp)
    {
      mips64_elf_fpregset_t fp_regs;

      if (ptrace (PTRACE_GETFPREGS, tid, 0L,
		  (PTRACE_TYPE_ARG3) &fp_regs) == -1)
	{
	  if (errno == EIO)
	    {
	      have_ptrace_regsets = 0;
	      return;
	    }
	  perror_with_name (_("Couldn't get FP registers"));
	}

      mips64_supply_fpregset (regcache,
			      (const mips64_elf_fpregset_t *) &fp_regs);
    }

  if (is_dsp)
    linux_nat_trad_target::fetch_registers (regcache, regno);
  else if (regno == -1 && have_dsp)
    {
      for (regi = mips_regnum (gdbarch)->dspacc;
	   regi < mips_regnum (gdbarch)->dspacc + 6;
	   regi++)
	linux_nat_trad_target::fetch_registers (regcache, regi);
      linux_nat_trad_target::fetch_registers (regcache,
					      mips_regnum (gdbarch)->dspctl);
    }
}

/* Store REGNO (or all registers if REGNO == -1) to the target
   using PTRACE_SETREGS et al.  */

void
mips_linux_nat_target::mips64_regsets_store_registers
  (struct regcache *regcache, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  int is_fp, is_dsp;
  int have_dsp;
  int regi;
  int tid;

  if (regno >= mips_regnum (gdbarch)->fp0
      && regno <= mips_regnum (gdbarch)->fp0 + 32)
    is_fp = 1;
  else if (regno == mips_regnum (gdbarch)->fp_control_status)
    is_fp = 1;
  else if (regno == mips_regnum (gdbarch)->fp_implementation_revision)
    is_fp = 1;
  else
    is_fp = 0;

  /* DSP registers are optional and not a part of any set.  */
  have_dsp = mips_regnum (gdbarch)->dspctl != -1;
  if (!have_dsp)
    is_dsp = 0;
  else if (regno >= mips_regnum (gdbarch)->dspacc
      && regno < mips_regnum (gdbarch)->dspacc + 6)
    is_dsp = 1;
  else if (regno == mips_regnum (gdbarch)->dspctl)
    is_dsp = 1;
  else
    is_dsp = 0;

  tid = get_ptrace_pid (regcache->ptid ());

  if (regno == -1 || (!is_fp && !is_dsp))
    {
      mips64_elf_gregset_t regs;

      if (ptrace (PTRACE_GETREGS, tid, 0L, (PTRACE_TYPE_ARG3) &regs) == -1)
	perror_with_name (_("Couldn't get registers"));

      mips64_fill_gregset (regcache, &regs, regno);

      if (ptrace (PTRACE_SETREGS, tid, 0L, (PTRACE_TYPE_ARG3) &regs) == -1)
	perror_with_name (_("Couldn't set registers"));
    }

  if (regno == -1 || is_fp)
    {
      mips64_elf_fpregset_t fp_regs;

      if (ptrace (PTRACE_GETFPREGS, tid, 0L,
		  (PTRACE_TYPE_ARG3) &fp_regs) == -1)
	perror_with_name (_("Couldn't get FP registers"));

      mips64_fill_fpregset (regcache, &fp_regs, regno);

      if (ptrace (PTRACE_SETFPREGS, tid, 0L,
		  (PTRACE_TYPE_ARG3) &fp_regs) == -1)
	perror_with_name (_("Couldn't set FP registers"));
    }

  if (is_dsp)
    linux_nat_trad_target::store_registers (regcache, regno);
  else if (regno == -1 && have_dsp)
    {
      for (regi = mips_regnum (gdbarch)->dspacc;
	   regi < mips_regnum (gdbarch)->dspacc + 6;
	   regi++)
	linux_nat_trad_target::store_registers (regcache, regi);
      linux_nat_trad_target::store_registers (regcache,
					      mips_regnum (gdbarch)->dspctl);
    }
}

/* Fetch REGNO (or all registers if REGNO == -1) from the target
   using any working method.  */

void
mips_linux_nat_target::fetch_registers (struct regcache *regcache, int regnum)
{
  /* Unless we already know that PTRACE_GETREGS does not work, try it.  */
  if (have_ptrace_regsets)
    mips64_regsets_fetch_registers (regcache, regnum);

  /* If we know, or just found out, that PTRACE_GETREGS does not work, fall
     back to PTRACE_PEEKUSER.  */
  if (!have_ptrace_regsets)
    {
      linux_nat_trad_target::fetch_registers (regcache, regnum);

      /* Fill the inaccessible zero register with zero.  */
      if (regnum == MIPS_ZERO_REGNUM || regnum == -1)
	regcache->raw_supply_zeroed (MIPS_ZERO_REGNUM);
    }
}

/* Store REGNO (or all registers if REGNO == -1) to the target
   using any working method.  */

void
mips_linux_nat_target::store_registers (struct regcache *regcache, int regnum)
{
  /* Unless we already know that PTRACE_GETREGS does not work, try it.  */
  if (have_ptrace_regsets)
    mips64_regsets_store_registers (regcache, regnum);

  /* If we know, or just found out, that PTRACE_GETREGS does not work, fall
     back to PTRACE_PEEKUSER.  */
  if (!have_ptrace_regsets)
    linux_nat_trad_target::store_registers (regcache, regnum);
}

/* Return the address in the core dump or inferior of register
   REGNO.  */

CORE_ADDR
mips_linux_nat_target::register_u_offset (struct gdbarch *gdbarch,
					  int regno, int store_p)
{
  if (mips_abi_regsize (gdbarch) == 8)
    return mips64_linux_register_addr (gdbarch, regno, store_p);
  else
    return mips_linux_register_addr (gdbarch, regno, store_p);
}

const struct target_desc *
mips_linux_nat_target::read_description ()
{
  static int have_dsp = -1;

  if (have_dsp < 0)
    {
      /* Assume no DSP if there is no inferior to inspect with ptrace.  */
      if (inferior_ptid == null_ptid)
	return _MIPS_SIM == _ABIO32 ? tdesc_mips_linux : tdesc_mips64_linux;

      int tid = get_ptrace_pid (inferior_ptid);

      errno = 0;
      ptrace (PTRACE_PEEKUSER, tid, DSP_CONTROL, 0);
      switch (errno)
	{
	case 0:
	  have_dsp = 1;
	  break;
	case EIO:
	  have_dsp = 0;
	  break;
	default:
	  perror_with_name (_("Couldn't check DSP support"));
	  break;
	}
    }

  /* Report that target registers are a size we know for sure
     that we can get from ptrace.  */
  if (_MIPS_SIM == _ABIO32)
    return have_dsp ? tdesc_mips_dsp_linux : tdesc_mips_linux;
  else
    return have_dsp ? tdesc_mips64_dsp_linux : tdesc_mips64_linux;
}

/* -1 if the kernel and/or CPU do not support watch registers.
    1 if watch_readback is valid and we can read style, num_valid
      and the masks.
    0 if we need to read the watch_readback.  */

static int watch_readback_valid;

/* Cached watch register read values.  */

static struct pt_watch_regs watch_readback;

static struct mips_watchpoint *current_watches;

/*  The current set of watch register values for writing the
    registers.  */

static struct pt_watch_regs watch_mirror;

static void
mips_show_dr (const char *func, CORE_ADDR addr,
	      int len, enum target_hw_bp_type type)
{
  int i;

  gdb_puts (func, gdb_stdlog);
  if (addr || len)
    gdb_printf (gdb_stdlog,
		" (addr=%s, len=%d, type=%s)",
		paddress (current_inferior ()->arch (), addr), len,
		type == hw_write ? "data-write"
		: (type == hw_read ? "data-read"
		   : (type == hw_access ? "data-read/write"
		      : (type == hw_execute ? "instruction-execute"
			 : "??unknown??"))));
  gdb_puts (":\n", gdb_stdlog);

  for (i = 0; i < MAX_DEBUG_REGISTER; i++)
    gdb_printf (gdb_stdlog, "\tDR%d: lo=%s, hi=%s\n", i,
		paddress (current_inferior ()->arch (),
			  mips_linux_watch_get_watchlo (&watch_mirror,
							i)),
		paddress (current_inferior ()->arch (),
			  mips_linux_watch_get_watchhi (&watch_mirror,
							i)));
}

/* Target to_can_use_hw_breakpoint implementation.  Return 1 if we can
   handle the specified watch type.  */

int
mips_linux_nat_target::can_use_hw_breakpoint (enum bptype type,
					      int cnt, int ot)
{
  int i;
  uint32_t wanted_mask, irw_mask;

  if (!mips_linux_read_watch_registers (inferior_ptid.lwp (),
					&watch_readback,
					&watch_readback_valid, 0))
    return 0;

   switch (type)
    {
    case bp_hardware_watchpoint:
      wanted_mask = W_MASK;
      break;
    case bp_read_watchpoint:
      wanted_mask = R_MASK;
      break;
    case bp_access_watchpoint:
      wanted_mask = R_MASK | W_MASK;
      break;
    default:
      return 0;
    }
 
  for (i = 0;
       i < mips_linux_watch_get_num_valid (&watch_readback) && cnt;
       i++)
    {
      irw_mask = mips_linux_watch_get_irw_mask (&watch_readback, i);
      if ((irw_mask & wanted_mask) == wanted_mask)
	cnt--;
    }
  return (cnt == 0) ? 1 : 0;
}

/* Target to_stopped_by_watchpoint implementation.  Return 1 if
   stopped by watchpoint.  The watchhi R and W bits indicate the watch
   register triggered.  */

bool
mips_linux_nat_target::stopped_by_watchpoint ()
{
  int n;
  int num_valid;

  if (!mips_linux_read_watch_registers (inferior_ptid.lwp (),
					&watch_readback,
					&watch_readback_valid, 1))
    return false;

  num_valid = mips_linux_watch_get_num_valid (&watch_readback);

  for (n = 0; n < MAX_DEBUG_REGISTER && n < num_valid; n++)
    if (mips_linux_watch_get_watchhi (&watch_readback, n) & (R_MASK | W_MASK))
      return true;

  return false;
}

/* Target to_stopped_data_address implementation.  Set the address
   where the watch triggered (if known).  Return 1 if the address was
   known.  */

bool
mips_linux_nat_target::stopped_data_address (CORE_ADDR *paddr)
{
  /* On mips we don't know the low order 3 bits of the data address,
     so we must return false.  */
  return false;
}

/* Target to_region_ok_for_hw_watchpoint implementation.  Return 1 if
   the specified region can be covered by the watch registers.  */

int
mips_linux_nat_target::region_ok_for_hw_watchpoint (CORE_ADDR addr, int len)
{
  struct pt_watch_regs dummy_regs;
  int i;

  if (!mips_linux_read_watch_registers (inferior_ptid.lwp (),
					&watch_readback,
					&watch_readback_valid, 0))
    return 0;

  dummy_regs = watch_readback;
  /* Clear them out.  */
  for (i = 0; i < mips_linux_watch_get_num_valid (&dummy_regs); i++)
    mips_linux_watch_set_watchlo (&dummy_regs, i, 0);
  return mips_linux_watch_try_one_watch (&dummy_regs, addr, len, 0);
}

/* Write the mirrored watch register values for each thread.  */

static int
write_watchpoint_regs (void)
{
  for (const lwp_info *lp : all_lwps ())
    {
      int tid = lp->ptid.lwp ();
      if (ptrace (PTRACE_SET_WATCH_REGS, tid, &watch_mirror, NULL) == -1)
	perror_with_name (_("Couldn't write debug register"));
    }
  return 0;
}

/* linux_nat_target::low_new_thread implementation.  Write the
   mirrored watch register values for the new thread.  */

void
mips_linux_nat_target::low_new_thread (struct lwp_info *lp)
{
  long tid = lp->ptid.lwp ();

  if (!mips_linux_read_watch_registers (tid,
					&watch_readback,
					&watch_readback_valid, 0))
    return;

  if (ptrace (PTRACE_SET_WATCH_REGS, tid, &watch_mirror, NULL) == -1)
    perror_with_name (_("Couldn't write debug register"));
}

/* Target to_insert_watchpoint implementation.  Try to insert a new
   watch.  Return zero on success.  */

int
mips_linux_nat_target::insert_watchpoint (CORE_ADDR addr, int len,
					  enum target_hw_bp_type type,
					  struct expression *cond)
{
  struct pt_watch_regs regs;
  struct mips_watchpoint *new_watch;
  struct mips_watchpoint **pw;

  int retval;

  if (!mips_linux_read_watch_registers (inferior_ptid.lwp (),
					&watch_readback,
					&watch_readback_valid, 0))
    return -1;

  if (len <= 0)
    return -1;

  regs = watch_readback;
  /* Add the current watches.  */
  mips_linux_watch_populate_regs (current_watches, &regs);

  /* Now try to add the new watch.  */
  if (!mips_linux_watch_try_one_watch (&regs, addr, len,
				       mips_linux_watch_type_to_irw (type)))
    return -1;

  /* It fit.  Stick it on the end of the list.  */
  new_watch = XNEW (struct mips_watchpoint);
  new_watch->addr = addr;
  new_watch->len = len;
  new_watch->type = type;
  new_watch->next = NULL;

  pw = &current_watches;
  while (*pw != NULL)
    pw = &(*pw)->next;
  *pw = new_watch;

  watch_mirror = regs;
  retval = write_watchpoint_regs ();

  if (show_debug_regs)
    mips_show_dr ("insert_watchpoint", addr, len, type);

  return retval;
}

/* Target to_remove_watchpoint implementation.  Try to remove a watch.
   Return zero on success.  */

int
mips_linux_nat_target::remove_watchpoint (CORE_ADDR addr, int len,
					  enum target_hw_bp_type type,
					  struct expression *cond)
{
  int retval;
  int deleted_one;

  struct mips_watchpoint **pw;
  struct mips_watchpoint *w;

  /* Search for a known watch that matches.  Then unlink and free
     it.  */
  deleted_one = 0;
  pw = &current_watches;
  while ((w = *pw))
    {
      if (w->addr == addr && w->len == len && w->type == type)
	{
	  *pw = w->next;
	  xfree (w);
	  deleted_one = 1;
	  break;
	}
      pw = &(w->next);
    }

  if (!deleted_one)
    return -1;  /* We don't know about it, fail doing nothing.  */

  /* At this point watch_readback is known to be valid because we
     could not have added the watch without reading it.  */
  gdb_assert (watch_readback_valid == 1);

  watch_mirror = watch_readback;
  mips_linux_watch_populate_regs (current_watches, &watch_mirror);

  retval = write_watchpoint_regs ();

  if (show_debug_regs)
    mips_show_dr ("remove_watchpoint", addr, len, type);

  return retval;
}

/* Target to_close implementation.  Free any watches and call the
   super implementation.  */

void
mips_linux_nat_target::close ()
{
  struct mips_watchpoint *w;
  struct mips_watchpoint *nw;

  /* Clean out the current_watches list.  */
  w = current_watches;
  while (w)
    {
      nw = w->next;
      xfree (w);
      w = nw;
    }
  current_watches = NULL;

  linux_nat_trad_target::close ();
}

void _initialize_mips_linux_nat ();
void
_initialize_mips_linux_nat ()
{
  add_setshow_boolean_cmd ("show-debug-regs", class_maintenance,
			   &show_debug_regs, _("\
Set whether to show variables that mirror the mips debug registers."), _("\
Show whether to show variables that mirror the mips debug registers."), _("\
Use \"on\" to enable, \"off\" to disable.\n\
If enabled, the debug registers values are shown when GDB inserts\n\
or removes a hardware breakpoint or watchpoint, and when the inferior\n\
triggers a breakpoint or watchpoint."),
			   NULL,
			   NULL,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);

  linux_target = &the_mips_linux_nat_target;
  add_inf_child_target (&the_mips_linux_nat_target);
}
