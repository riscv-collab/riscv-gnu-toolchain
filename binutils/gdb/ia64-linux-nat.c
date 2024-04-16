/* Functions specific to running gdb native on IA-64 running
   GNU/Linux.

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
#include "target.h"
#include "gdbarch.h"
#include "gdbcore.h"
#include "regcache.h"
#include "ia64-tdep.h"
#include "linux-nat.h"

#include <signal.h>
#include "nat/gdb_ptrace.h"
#include "gdbsupport/gdb_wait.h"
#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif
#include <sys/syscall.h>
#include <sys/user.h>

#include <asm/ptrace_offsets.h>
#include <sys/procfs.h>

/* Prototypes for supply_gregset etc.  */
#include "gregset.h"

#include "inf-ptrace.h"

class ia64_linux_nat_target final : public linux_nat_target
{
public:
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  /* Override watchpoint routines.  */

  /* The IA-64 architecture can step over a watch point (without
     triggering it again) if the "dd" (data debug fault disable) bit
     in the processor status word is set.

     This PSR bit is set in
     ia64_linux_nat_target::stopped_by_watchpoint when the code there
     has determined that a hardware watchpoint has indeed been hit.
     The CPU will then be able to execute one instruction without
     triggering a watchpoint.  */
  bool have_steppable_watchpoint () override { return true; }

  int can_use_hw_breakpoint (enum bptype, int, int) override;
  bool stopped_by_watchpoint () override;
  bool stopped_data_address (CORE_ADDR *) override;
  int insert_watchpoint (CORE_ADDR, int, enum target_hw_bp_type,
			 struct expression *) override;
  int remove_watchpoint (CORE_ADDR, int, enum target_hw_bp_type,
			 struct expression *) override;
  /* Override linux_nat_target low methods.  */
  void low_new_thread (struct lwp_info *lp) override;
  bool low_status_is_event (int status) override;

  void enable_watchpoints_in_psr (ptid_t ptid);
};

static ia64_linux_nat_target the_ia64_linux_nat_target;

/* These must match the order of the register names.

   Some sort of lookup table is needed because the offsets associated
   with the registers are all over the board.  */

static int u_offsets[] =
  {
    /* general registers */
    -1,		/* gr0 not available; i.e, it's always zero.  */
    PT_R1,
    PT_R2,
    PT_R3,
    PT_R4,
    PT_R5,
    PT_R6,
    PT_R7,
    PT_R8,
    PT_R9,
    PT_R10,
    PT_R11,
    PT_R12,
    PT_R13,
    PT_R14,
    PT_R15,
    PT_R16,
    PT_R17,
    PT_R18,
    PT_R19,
    PT_R20,
    PT_R21,
    PT_R22,
    PT_R23,
    PT_R24,
    PT_R25,
    PT_R26,
    PT_R27,
    PT_R28,
    PT_R29,
    PT_R30,
    PT_R31,
    /* gr32 through gr127 not directly available via the ptrace interface.  */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* Floating point registers */
    -1, -1,	/* f0 and f1 not available (f0 is +0.0 and f1 is +1.0).  */
    PT_F2,
    PT_F3,
    PT_F4,
    PT_F5,
    PT_F6,
    PT_F7,
    PT_F8,
    PT_F9,
    PT_F10,
    PT_F11,
    PT_F12,
    PT_F13,
    PT_F14,
    PT_F15,
    PT_F16,
    PT_F17,
    PT_F18,
    PT_F19,
    PT_F20,
    PT_F21,
    PT_F22,
    PT_F23,
    PT_F24,
    PT_F25,
    PT_F26,
    PT_F27,
    PT_F28,
    PT_F29,
    PT_F30,
    PT_F31,
    PT_F32,
    PT_F33,
    PT_F34,
    PT_F35,
    PT_F36,
    PT_F37,
    PT_F38,
    PT_F39,
    PT_F40,
    PT_F41,
    PT_F42,
    PT_F43,
    PT_F44,
    PT_F45,
    PT_F46,
    PT_F47,
    PT_F48,
    PT_F49,
    PT_F50,
    PT_F51,
    PT_F52,
    PT_F53,
    PT_F54,
    PT_F55,
    PT_F56,
    PT_F57,
    PT_F58,
    PT_F59,
    PT_F60,
    PT_F61,
    PT_F62,
    PT_F63,
    PT_F64,
    PT_F65,
    PT_F66,
    PT_F67,
    PT_F68,
    PT_F69,
    PT_F70,
    PT_F71,
    PT_F72,
    PT_F73,
    PT_F74,
    PT_F75,
    PT_F76,
    PT_F77,
    PT_F78,
    PT_F79,
    PT_F80,
    PT_F81,
    PT_F82,
    PT_F83,
    PT_F84,
    PT_F85,
    PT_F86,
    PT_F87,
    PT_F88,
    PT_F89,
    PT_F90,
    PT_F91,
    PT_F92,
    PT_F93,
    PT_F94,
    PT_F95,
    PT_F96,
    PT_F97,
    PT_F98,
    PT_F99,
    PT_F100,
    PT_F101,
    PT_F102,
    PT_F103,
    PT_F104,
    PT_F105,
    PT_F106,
    PT_F107,
    PT_F108,
    PT_F109,
    PT_F110,
    PT_F111,
    PT_F112,
    PT_F113,
    PT_F114,
    PT_F115,
    PT_F116,
    PT_F117,
    PT_F118,
    PT_F119,
    PT_F120,
    PT_F121,
    PT_F122,
    PT_F123,
    PT_F124,
    PT_F125,
    PT_F126,
    PT_F127,
    /* Predicate registers - we don't fetch these individually.  */
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    /* branch registers */
    PT_B0,
    PT_B1,
    PT_B2,
    PT_B3,
    PT_B4,
    PT_B5,
    PT_B6,
    PT_B7,
    /* Virtual frame pointer and virtual return address pointer.  */
    -1, -1,
    /* other registers */
    PT_PR,
    PT_CR_IIP,	/* ip */
    PT_CR_IPSR, /* psr */
    PT_CFM,	/* cfm */
    /* kernel registers not visible via ptrace interface (?)  */
    -1, -1, -1, -1, -1, -1, -1, -1,
    /* hole */
    -1, -1, -1, -1, -1, -1, -1, -1,
    PT_AR_RSC,
    PT_AR_BSP,
    PT_AR_BSPSTORE,
    PT_AR_RNAT,
    -1,
    -1,		/* Not available: FCR, IA32 floating control register.  */
    -1, -1,
    -1,		/* Not available: EFLAG */
    -1,		/* Not available: CSD */
    -1,		/* Not available: SSD */
    -1,		/* Not available: CFLG */
    -1,		/* Not available: FSR */
    -1,		/* Not available: FIR */
    -1,		/* Not available: FDR */
    -1,
    PT_AR_CCV,
    -1, -1, -1,
    PT_AR_UNAT,
    -1, -1, -1,
    PT_AR_FPSR,
    -1, -1, -1,
    -1,		/* Not available: ITC */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    PT_AR_PFS,
    PT_AR_LC,
    PT_AR_EC,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1,
    /* nat bits - not fetched directly; instead we obtain these bits from
       either rnat or unat or from memory.  */
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
  };

static CORE_ADDR
ia64_register_addr (struct gdbarch *gdbarch, int regno)
{
  CORE_ADDR addr;

  if (regno < 0 || regno >= gdbarch_num_regs (gdbarch))
    error (_("Invalid register number %d."), regno);

  if (u_offsets[regno] == -1)
    addr = 0;
  else
    addr = (CORE_ADDR) u_offsets[regno];

  return addr;
}

static int
ia64_cannot_fetch_register (struct gdbarch *gdbarch, int regno)
{
  return regno < 0
	 || regno >= gdbarch_num_regs (gdbarch)
	 || u_offsets[regno] == -1;
}

static int
ia64_cannot_store_register (struct gdbarch *gdbarch, int regno)
{
  /* Rationale behind not permitting stores to bspstore...
  
     The IA-64 architecture provides bspstore and bsp which refer
     memory locations in the RSE's backing store.  bspstore is the
     next location which will be written when the RSE needs to write
     to memory.  bsp is the address at which r32 in the current frame
     would be found if it were written to the backing store.

     The IA-64 architecture provides read-only access to bsp and
     read/write access to bspstore (but only when the RSE is in
     the enforced lazy mode).  It should be noted that stores
     to bspstore also affect the value of bsp.  Changing bspstore
     does not affect the number of dirty entries between bspstore
     and bsp, so changing bspstore by N words will also cause bsp
     to be changed by (roughly) N as well.  (It could be N-1 or N+1
     depending upon where the NaT collection bits fall.)

     OTOH, the Linux kernel provides read/write access to bsp (and
     currently read/write access to bspstore as well).  But it
     is definitely the case that if you change one, the other
     will change at the same time.  It is more useful to gdb to
     be able to change bsp.  So in order to prevent strange and
     undesirable things from happening when a dummy stack frame
     is popped (after calling an inferior function), we allow
     bspstore to be read, but not written.  (Note that popping
     a (generic) dummy stack frame causes all registers that
     were previously read from the inferior process to be written
     back.)  */

  return regno < 0
	 || regno >= gdbarch_num_regs (gdbarch)
	 || u_offsets[regno] == -1
	 || regno == IA64_BSPSTORE_REGNUM;
}

void
supply_gregset (struct regcache *regcache, const gregset_t *gregsetp)
{
  int regi;
  const greg_t *regp = (const greg_t *) gregsetp;

  for (regi = IA64_GR0_REGNUM; regi <= IA64_GR31_REGNUM; regi++)
    {
      regcache->raw_supply (regi, regp + (regi - IA64_GR0_REGNUM));
    }

  /* FIXME: NAT collection bits are at index 32; gotta deal with these
     somehow...  */

  regcache->raw_supply (IA64_PR_REGNUM, regp + 33);

  for (regi = IA64_BR0_REGNUM; regi <= IA64_BR7_REGNUM; regi++)
    {
      regcache->raw_supply (regi, regp + 34 + (regi - IA64_BR0_REGNUM));
    }

  regcache->raw_supply (IA64_IP_REGNUM, regp + 42);
  regcache->raw_supply (IA64_CFM_REGNUM, regp + 43);
  regcache->raw_supply (IA64_PSR_REGNUM, regp + 44);
  regcache->raw_supply (IA64_RSC_REGNUM, regp + 45);
  regcache->raw_supply (IA64_BSP_REGNUM, regp + 46);
  regcache->raw_supply (IA64_BSPSTORE_REGNUM, regp + 47);
  regcache->raw_supply (IA64_RNAT_REGNUM, regp + 48);
  regcache->raw_supply (IA64_CCV_REGNUM, regp + 49);
  regcache->raw_supply (IA64_UNAT_REGNUM, regp + 50);
  regcache->raw_supply (IA64_FPSR_REGNUM, regp + 51);
  regcache->raw_supply (IA64_PFS_REGNUM, regp + 52);
  regcache->raw_supply (IA64_LC_REGNUM, regp + 53);
  regcache->raw_supply (IA64_EC_REGNUM, regp + 54);
}

void
fill_gregset (const struct regcache *regcache, gregset_t *gregsetp, int regno)
{
  int regi;
  greg_t *regp = (greg_t *) gregsetp;

#define COPY_REG(_idx_,_regi_) \
  if ((regno == -1) || regno == _regi_) \
    regcache->raw_collect (_regi_, regp + _idx_)

  for (regi = IA64_GR0_REGNUM; regi <= IA64_GR31_REGNUM; regi++)
    {
      COPY_REG (regi - IA64_GR0_REGNUM, regi);
    }

  /* FIXME: NAT collection bits at index 32?  */

  COPY_REG (33, IA64_PR_REGNUM);

  for (regi = IA64_BR0_REGNUM; regi <= IA64_BR7_REGNUM; regi++)
    {
      COPY_REG (34 + (regi - IA64_BR0_REGNUM), regi);
    }

  COPY_REG (42, IA64_IP_REGNUM);
  COPY_REG (43, IA64_CFM_REGNUM);
  COPY_REG (44, IA64_PSR_REGNUM);
  COPY_REG (45, IA64_RSC_REGNUM);
  COPY_REG (46, IA64_BSP_REGNUM);
  COPY_REG (47, IA64_BSPSTORE_REGNUM);
  COPY_REG (48, IA64_RNAT_REGNUM);
  COPY_REG (49, IA64_CCV_REGNUM);
  COPY_REG (50, IA64_UNAT_REGNUM);
  COPY_REG (51, IA64_FPSR_REGNUM);
  COPY_REG (52, IA64_PFS_REGNUM);
  COPY_REG (53, IA64_LC_REGNUM);
  COPY_REG (54, IA64_EC_REGNUM);
}

/*  Given a pointer to a floating point register set in /proc format
   (fpregset_t *), unpack the register contents and supply them as gdb's
   idea of the current floating point register values.  */

void
supply_fpregset (struct regcache *regcache, const fpregset_t *fpregsetp)
{
  int regi;
  const char *from;
  const gdb_byte f_zero[16] = { 0 };
  const gdb_byte f_one[16] =
    { 0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0xff, 0, 0, 0, 0, 0, 0 };

  /* Kernel generated cores have fr1==0 instead of 1.0.  Older GDBs
     did the same.  So ignore whatever might be recorded in fpregset_t
     for fr0/fr1 and always supply their expected values.  */

  /* fr0 is always read as zero.  */
  regcache->raw_supply (IA64_FR0_REGNUM, f_zero);
  /* fr1 is always read as one (1.0).  */
  regcache->raw_supply (IA64_FR1_REGNUM, f_one);

  for (regi = IA64_FR2_REGNUM; regi <= IA64_FR127_REGNUM; regi++)
    {
      from = (const char *) &((*fpregsetp)[regi - IA64_FR0_REGNUM]);
      regcache->raw_supply (regi, from);
    }
}

/*  Given a pointer to a floating point register set in /proc format
   (fpregset_t *), update the register specified by REGNO from gdb's idea
   of the current floating point register set.  If REGNO is -1, update
   them all.  */

void
fill_fpregset (const struct regcache *regcache,
	       fpregset_t *fpregsetp, int regno)
{
  int regi;

  for (regi = IA64_FR0_REGNUM; regi <= IA64_FR127_REGNUM; regi++)
    {
      if ((regno == -1) || (regno == regi))
	regcache->raw_collect (regi, &((*fpregsetp)[regi - IA64_FR0_REGNUM]));
    }
}

#define IA64_PSR_DB (1UL << 24)
#define IA64_PSR_DD (1UL << 39)

void
ia64_linux_nat_target::enable_watchpoints_in_psr (ptid_t ptid)
{
  struct regcache *regcache = get_thread_regcache (this, ptid);
  ULONGEST psr;

  regcache_cooked_read_unsigned (regcache, IA64_PSR_REGNUM, &psr);
  if (!(psr & IA64_PSR_DB))
    {
      psr |= IA64_PSR_DB;	/* Set the db bit - this enables hardware
				   watchpoints and breakpoints.  */
      regcache_cooked_write_unsigned (regcache, IA64_PSR_REGNUM, psr);
    }
}

static long debug_registers[8];

static void
store_debug_register (ptid_t ptid, int idx, long val)
{
  int tid;

  tid = ptid.lwp ();
  if (tid == 0)
    tid = ptid.pid ();

  (void) ptrace (PT_WRITE_U, tid, (PTRACE_TYPE_ARG3) (PT_DBR + 8 * idx), val);
}

static void
store_debug_register_pair (ptid_t ptid, int idx, long *dbr_addr,
			   long *dbr_mask)
{
  if (dbr_addr)
    store_debug_register (ptid, 2 * idx, *dbr_addr);
  if (dbr_mask)
    store_debug_register (ptid, 2 * idx + 1, *dbr_mask);
}

static int
is_power_of_2 (int val)
{
  int i, onecount;

  onecount = 0;
  for (i = 0; i < 8 * sizeof (val); i++)
    if (val & (1 << i))
      onecount++;

  return onecount <= 1;
}

int
ia64_linux_nat_target::insert_watchpoint (CORE_ADDR addr, int len,
					  enum target_hw_bp_type type,
					  struct expression *cond)
{
  int idx;
  long dbr_addr, dbr_mask;
  int max_watchpoints = 4;

  if (len <= 0 || !is_power_of_2 (len))
    return -1;

  for (idx = 0; idx < max_watchpoints; idx++)
    {
      dbr_mask = debug_registers[idx * 2 + 1];
      if ((dbr_mask & (0x3UL << 62)) == 0)
	{
	  /* Exit loop if both r and w bits clear.  */
	  break;
	}
    }

  if (idx == max_watchpoints)
    return -1;

  dbr_addr = (long) addr;
  dbr_mask = (~(len - 1) & 0x00ffffffffffffffL);  /* construct mask to match */
  dbr_mask |= 0x0800000000000000L;           /* Only match privilege level 3 */
  switch (type)
    {
    case hw_write:
      dbr_mask |= (1L << 62);			/* Set w bit */
      break;
    case hw_read:
      dbr_mask |= (1L << 63);			/* Set r bit */
      break;
    case hw_access:
      dbr_mask |= (3L << 62);			/* Set both r and w bits */
      break;
    default:
      return -1;
    }

  debug_registers[2 * idx] = dbr_addr;
  debug_registers[2 * idx + 1] = dbr_mask;

  for (const lwp_info *lp : all_lwps ())
    {
      store_debug_register_pair (lp->ptid, idx, &dbr_addr, &dbr_mask);
      enable_watchpoints_in_psr (lp->ptid);
    }

  return 0;
}

int
ia64_linux_nat_target::remove_watchpoint (CORE_ADDR addr, int len,
					  enum target_hw_bp_type type,
					  struct expression *cond)
{
  int idx;
  long dbr_addr, dbr_mask;
  int max_watchpoints = 4;

  if (len <= 0 || !is_power_of_2 (len))
    return -1;

  for (idx = 0; idx < max_watchpoints; idx++)
    {
      dbr_addr = debug_registers[2 * idx];
      dbr_mask = debug_registers[2 * idx + 1];
      if ((dbr_mask & (0x3UL << 62)) && addr == (CORE_ADDR) dbr_addr)
	{
	  debug_registers[2 * idx] = 0;
	  debug_registers[2 * idx + 1] = 0;
	  dbr_addr = 0;
	  dbr_mask = 0;

	  for (const lwp_info *lp : all_lwps ())
	    store_debug_register_pair (lp->ptid, idx, &dbr_addr, &dbr_mask);

	  return 0;
	}
    }
  return -1;
}

void
ia64_linux_nat_target::low_new_thread (struct lwp_info *lp)
{
  int i, any;

  any = 0;
  for (i = 0; i < 8; i++)
    {
      if (debug_registers[i] != 0)
	any = 1;
      store_debug_register (lp->ptid, i, debug_registers[i]);
    }

  if (any)
    enable_watchpoints_in_psr (lp->ptid);
}

bool
ia64_linux_nat_target::stopped_data_address (CORE_ADDR *addr_p)
{
  CORE_ADDR psr;
  siginfo_t siginfo;
  regcache *regcache = get_thread_regcache (inferior_thread ());

  if (!linux_nat_get_siginfo (inferior_ptid, &siginfo))
    return false;

  if (siginfo.si_signo != SIGTRAP
      || (siginfo.si_code & 0xffff) != 0x0004 /* TRAP_HWBKPT */)
    return false;

  regcache_cooked_read_unsigned (regcache, IA64_PSR_REGNUM, &psr);
  psr |= IA64_PSR_DD;	/* Set the dd bit - this will disable the watchpoint
			   for the next instruction.  */
  regcache_cooked_write_unsigned (regcache, IA64_PSR_REGNUM, psr);

  *addr_p = (CORE_ADDR) siginfo.si_addr;
  return true;
}

bool
ia64_linux_nat_target::stopped_by_watchpoint ()
{
  CORE_ADDR addr;
  return stopped_data_address (&addr);
}

int
ia64_linux_nat_target::can_use_hw_breakpoint (enum bptype type,
					      int cnt, int othertype)
{
  return 1;
}


/* Fetch register REGNUM from the inferior.  */

static void
ia64_linux_fetch_register (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  CORE_ADDR addr;
  size_t size;
  PTRACE_TYPE_RET *buf;
  pid_t pid;
  int i;

  /* r0 cannot be fetched but is always zero.  */
  if (regnum == IA64_GR0_REGNUM)
    {
      const gdb_byte zero[8] = { 0 };

      gdb_assert (sizeof (zero) == register_size (gdbarch, regnum));
      regcache->raw_supply (regnum, zero);
      return;
    }

  /* fr0 cannot be fetched but is always zero.  */
  if (regnum == IA64_FR0_REGNUM)
    {
      const gdb_byte f_zero[16] = { 0 };

      gdb_assert (sizeof (f_zero) == register_size (gdbarch, regnum));
      regcache->raw_supply (regnum, f_zero);
      return;
    }

  /* fr1 cannot be fetched but is always one (1.0).  */
  if (regnum == IA64_FR1_REGNUM)
    {
      const gdb_byte f_one[16] =
	{ 0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0xff, 0, 0, 0, 0, 0, 0 };

      gdb_assert (sizeof (f_one) == register_size (gdbarch, regnum));
      regcache->raw_supply (regnum, f_one);
      return;
    }

  if (ia64_cannot_fetch_register (gdbarch, regnum))
    {
      regcache->raw_supply (regnum, NULL);
      return;
    }

  pid = get_ptrace_pid (regcache->ptid ());

  /* This isn't really an address, but ptrace thinks of it as one.  */
  addr = ia64_register_addr (gdbarch, regnum);
  size = register_size (gdbarch, regnum);

  gdb_assert ((size % sizeof (PTRACE_TYPE_RET)) == 0);
  buf = (PTRACE_TYPE_RET *) alloca (size);

  /* Read the register contents from the inferior a chunk at a time.  */
  for (i = 0; i < size / sizeof (PTRACE_TYPE_RET); i++)
    {
      errno = 0;
      buf[i] = ptrace (PT_READ_U, pid, (PTRACE_TYPE_ARG3)addr, 0);
      if (errno != 0)
	error (_("Couldn't read register %s (#%d): %s."),
	       gdbarch_register_name (gdbarch, regnum),
	       regnum, safe_strerror (errno));

      addr += sizeof (PTRACE_TYPE_RET);
    }
  regcache->raw_supply (regnum, buf);
}

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

void
ia64_linux_nat_target::fetch_registers (struct regcache *regcache, int regnum)
{
  if (regnum == -1)
    for (regnum = 0;
	 regnum < gdbarch_num_regs (regcache->arch ());
	 regnum++)
      ia64_linux_fetch_register (regcache, regnum);
  else
    ia64_linux_fetch_register (regcache, regnum);
}

/* Store register REGNUM into the inferior.  */

static void
ia64_linux_store_register (const struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  CORE_ADDR addr;
  size_t size;
  PTRACE_TYPE_RET *buf;
  pid_t pid;
  int i;

  if (ia64_cannot_store_register (gdbarch, regnum))
    return;

  pid = get_ptrace_pid (regcache->ptid ());

  /* This isn't really an address, but ptrace thinks of it as one.  */
  addr = ia64_register_addr (gdbarch, regnum);
  size = register_size (gdbarch, regnum);

  gdb_assert ((size % sizeof (PTRACE_TYPE_RET)) == 0);
  buf = (PTRACE_TYPE_RET *) alloca (size);

  /* Write the register contents into the inferior a chunk at a time.  */
  regcache->raw_collect (regnum, buf);
  for (i = 0; i < size / sizeof (PTRACE_TYPE_RET); i++)
    {
      errno = 0;
      ptrace (PT_WRITE_U, pid, (PTRACE_TYPE_ARG3)addr, buf[i]);
      if (errno != 0)
	error (_("Couldn't write register %s (#%d): %s."),
	       gdbarch_register_name (gdbarch, regnum),
	       regnum, safe_strerror (errno));

      addr += sizeof (PTRACE_TYPE_RET);
    }
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers.  */

void
ia64_linux_nat_target::store_registers (struct regcache *regcache, int regnum)
{
  if (regnum == -1)
    for (regnum = 0;
	 regnum < gdbarch_num_regs (regcache->arch ());
	 regnum++)
      ia64_linux_store_register (regcache, regnum);
  else
    ia64_linux_store_register (regcache, regnum);
}

/* Implement the xfer_partial target_ops method.  */

enum target_xfer_status
ia64_linux_nat_target::xfer_partial (enum target_object object,
				     const char *annex,
				     gdb_byte *readbuf, const gdb_byte *writebuf,
				     ULONGEST offset, ULONGEST len,
				     ULONGEST *xfered_len)
{
  if (object == TARGET_OBJECT_UNWIND_TABLE && readbuf != NULL)
    {
      static long gate_table_size;
      gdb_byte *tmp_buf;
      long res;

      /* Probe for the table size once.  */
      if (gate_table_size == 0)
	gate_table_size = syscall (__NR_getunwind, NULL, 0);
      if (gate_table_size < 0)
	return TARGET_XFER_E_IO;

      if (offset >= gate_table_size)
	return TARGET_XFER_EOF;

      tmp_buf = (gdb_byte *) alloca (gate_table_size);
      res = syscall (__NR_getunwind, tmp_buf, gate_table_size);
      if (res < 0)
	return TARGET_XFER_E_IO;
      gdb_assert (res == gate_table_size);

      if (offset + len > gate_table_size)
	len = gate_table_size - offset;

      memcpy (readbuf, tmp_buf + offset, len);
      *xfered_len = len;
      return TARGET_XFER_OK;
    }

  return linux_nat_target::xfer_partial (object, annex, readbuf, writebuf,
					 offset, len, xfered_len);
}

/* For break.b instruction ia64 CPU forgets the immediate value and generates
   SIGILL with ILL_ILLOPC instead of more common SIGTRAP with TRAP_BRKPT.
   ia64 does not use gdbarch_decr_pc_after_break so we do not have to make any
   difference for the signals here.  */

bool
ia64_linux_nat_target::low_status_is_event (int status)
{
  return WIFSTOPPED (status) && (WSTOPSIG (status) == SIGTRAP
				 || WSTOPSIG (status) == SIGILL);
}

void _initialize_ia64_linux_nat ();
void
_initialize_ia64_linux_nat ()
{
  /* Register the target.  */
  linux_target = &the_ia64_linux_nat_target;
  add_inf_child_target (&the_ia64_linux_nat_target);
}
