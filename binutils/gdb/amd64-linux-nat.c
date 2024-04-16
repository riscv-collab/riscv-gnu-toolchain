/* Native-dependent code for GNU/Linux x86-64.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.
   Contributed by Jiri Smid, SuSE Labs.

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
#include "elf/common.h"
#include <sys/uio.h>
#include "nat/gdb_ptrace.h"
#include <asm/prctl.h>
#include <sys/reg.h>
#include "gregset.h"
#include "gdb_proc_service.h"

#include "amd64-nat.h"
#include "amd64-tdep.h"
#include "amd64-linux-tdep.h"
#include "i386-linux-tdep.h"
#include "gdbsupport/x86-xstate.h"

#include "x86-linux-nat.h"
#include "nat/linux-ptrace.h"
#include "nat/amd64-linux-siginfo.h"

/* This definition comes from prctl.h.  Kernels older than 2.5.64
   do not have it.  */
#ifndef PTRACE_ARCH_PRCTL
#define PTRACE_ARCH_PRCTL      30
#endif

struct amd64_linux_nat_target final : public x86_linux_nat_target
{
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  bool low_siginfo_fixup (siginfo_t *ptrace, gdb_byte *inf, int direction)
    override;
};

static amd64_linux_nat_target the_amd64_linux_nat_target;

/* Mapping between the general-purpose registers in GNU/Linux x86-64
   `struct user' format and GDB's register cache layout for GNU/Linux
   i386.

   Note that most GNU/Linux x86-64 registers are 64-bit, while the
   GNU/Linux i386 registers are all 32-bit, but since we're
   little-endian we get away with that.  */

/* From <sys/reg.h> on GNU/Linux i386.  */
static int amd64_linux_gregset32_reg_offset[] =
{
  RAX * 8, RCX * 8,		/* %eax, %ecx */
  RDX * 8, RBX * 8,		/* %edx, %ebx */
  RSP * 8, RBP * 8,		/* %esp, %ebp */
  RSI * 8, RDI * 8,		/* %esi, %edi */
  RIP * 8, EFLAGS * 8,		/* %eip, %eflags */
  CS * 8, SS * 8,		/* %cs, %ss */
  DS * 8, ES * 8,		/* %ds, %es */
  FS * 8, GS * 8,		/* %fs, %gs */
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1,		  /* MPX registers BND0 ... BND3.  */
  -1, -1,			  /* MPX registers BNDCFGU, BNDSTATUS.  */
  -1, -1, -1, -1, -1, -1, -1, -1, /* k0 ... k7 (AVX512)  */
  -1, -1, -1, -1, -1, -1, -1, -1, /* zmm0 ... zmm7 (AVX512)  */
  -1,				  /* PKEYS register PKRU  */
  ORIG_RAX * 8			  /* "orig_eax"  */
};


/* Transfering the general-purpose registers between GDB, inferiors
   and core files.  */

/* See amd64_collect_native_gregset.  This linux specific version handles
   issues with negative EAX values not being restored correctly upon syscall
   return when debugging 32-bit targets.  It has no effect on 64-bit
   targets.  */

static void
amd64_linux_collect_native_gregset (const struct regcache *regcache,
				    void *gregs, int regnum)
{
  amd64_collect_native_gregset (regcache, gregs, regnum);

  struct gdbarch *gdbarch = regcache->arch ();
  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    {
      /* Sign extend EAX value to avoid potential syscall restart
	 problems.  

	 On Linux, when a syscall is interrupted by a signal, the
	 (kernel function implementing the) syscall may return
	 -ERESTARTSYS when a signal occurs.  Doing so indicates that
	 the syscall is restartable.  Then, depending on settings
	 associated with the signal handler, and after the signal
	 handler is called, the kernel can then either return -EINTR
	 or it can cause the syscall to be restarted.  We are
	 concerned with the latter case here.
	 
	 On (32-bit) i386, the status (-ERESTARTSYS) is placed in the
	 EAX register.  When debugging a 32-bit process from a 64-bit
	 (amd64) GDB, the debugger fetches 64-bit registers even
	 though the process being debugged is only 32-bit.  The
	 register cache is only 32 bits wide though; GDB discards the
	 high 32 bits when placing 64-bit values in the 32-bit
	 regcache.  Normally, this is not a problem since the 32-bit
	 process should only care about the lower 32-bit portions of
	 these registers.  That said, it can happen that the 64-bit
	 value being restored will be different from the 64-bit value
	 that was originally retrieved from the kernel.  The one place
	 (that we know of) where it does matter is in the kernel's
	 syscall restart code.  The kernel's code for restarting a
	 syscall after a signal expects to see a negative value
	 (specifically -ERESTARTSYS) in the 64-bit RAX register in
	 order to correctly cause a syscall to be restarted.
	 
	 The call to amd64_collect_native_gregset, above, is setting
	 the high 32 bits of RAX (and other registers too) to 0.  For
	 syscall restart, we need to sign extend EAX so that RAX will
	 appear as a negative value when EAX is set to -ERESTARTSYS. 
	 This in turn will cause the signal handling code in the
	 kernel to recognize -ERESTARTSYS which will in turn cause the
	 syscall to be restarted.

	 The test case gdb.base/interrupt.exp tests for this problem.
	 Without this sign extension code in place, it'll show
	 a number of failures when testing against unix/-m32.  */

      if (regnum == -1 || regnum == I386_EAX_REGNUM)
	{
	  void *ptr = ((gdb_byte *) gregs 
		       + amd64_linux_gregset32_reg_offset[I386_EAX_REGNUM]);

	  *(int64_t *) ptr = *(int32_t *) ptr;
	}
    }
}

/* Fill GDB's register cache with the general-purpose register values
   in *GREGSETP.  */

void
supply_gregset (struct regcache *regcache, const elf_gregset_t *gregsetp)
{
  amd64_supply_native_gregset (regcache, gregsetp, -1);
}

/* Fill register REGNUM (if it is a general-purpose register) in
   *GREGSETP with the value in GDB's register cache.  If REGNUM is -1,
   do this for all registers.  */

void
fill_gregset (const struct regcache *regcache,
	      elf_gregset_t *gregsetp, int regnum)
{
  amd64_linux_collect_native_gregset (regcache, gregsetp, regnum);
}

/* Transfering floating-point registers between GDB, inferiors and cores.  */

/* Fill GDB's register cache with the floating-point and SSE register
   values in *FPREGSETP.  */

void
supply_fpregset (struct regcache *regcache, const elf_fpregset_t *fpregsetp)
{
  amd64_supply_fxsave (regcache, -1, fpregsetp);
}

/* Fill register REGNUM (if it is a floating-point or SSE register) in
   *FPREGSETP with the value in GDB's register cache.  If REGNUM is
   -1, do this for all registers.  */

void
fill_fpregset (const struct regcache *regcache,
	       elf_fpregset_t *fpregsetp, int regnum)
{
  amd64_collect_fxsave (regcache, regnum, fpregsetp);
}


/* Transferring arbitrary registers between GDB and inferior.  */

/* Fetch register REGNUM from the child process.  If REGNUM is -1, do
   this for all registers (including the floating point and SSE
   registers).  */

void
amd64_linux_nat_target::fetch_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  int tid;

  /* GNU/Linux LWP ID's are process ID's.  */
  tid = regcache->ptid ().lwp ();
  if (tid == 0)
    tid = regcache->ptid ().pid (); /* Not a threaded program.  */

  if (regnum == -1 || amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      elf_gregset_t regs;

      if (ptrace (PTRACE_GETREGS, tid, 0, (long) &regs) < 0)
	perror_with_name (_("Couldn't get registers"));

      amd64_supply_native_gregset (regcache, &regs, -1);
      if (regnum != -1)
	return;
    }

  if (regnum == -1 || !amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      elf_fpregset_t fpregs;

      if (have_ptrace_getregset == TRIBOOL_TRUE)
	{
	  char xstateregs[tdep->xsave_layout.sizeof_xsave];
	  struct iovec iov;

	  /* Pre-4.14 kernels have a bug (fixed by commit 0852b374173b
	     "x86/fpu: Add FPU state copying quirk to handle XRSTOR failure on
	     Intel Skylake CPUs") that sometimes causes the mxcsr location in
	     xstateregs not to be copied by PTRACE_GETREGSET.  Make sure that
	     the location is at least initialized with a defined value.  */
	  memset (xstateregs, 0, sizeof (xstateregs));
	  iov.iov_base = xstateregs;
	  iov.iov_len = sizeof (xstateregs);
	  if (ptrace (PTRACE_GETREGSET, tid,
		      (unsigned int) NT_X86_XSTATE, (long) &iov) < 0)
	    perror_with_name (_("Couldn't get extended state status"));

	  amd64_supply_xsave (regcache, -1, xstateregs);
	}
      else
	{
	  if (ptrace (PTRACE_GETFPREGS, tid, 0, (long) &fpregs) < 0)
	    perror_with_name (_("Couldn't get floating point status"));

	  amd64_supply_fxsave (regcache, -1, &fpregs);
	}
    }
}

/* Store register REGNUM back into the child process.  If REGNUM is
   -1, do this for all registers (including the floating-point and SSE
   registers).  */

void
amd64_linux_nat_target::store_registers (struct regcache *regcache, int regnum)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  int tid;

  /* GNU/Linux LWP ID's are process ID's.  */
  tid = regcache->ptid ().lwp ();
  if (tid == 0)
    tid = regcache->ptid ().pid (); /* Not a threaded program.  */

  if (regnum == -1 || amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      elf_gregset_t regs;

      if (ptrace (PTRACE_GETREGS, tid, 0, (long) &regs) < 0)
	perror_with_name (_("Couldn't get registers"));

      amd64_linux_collect_native_gregset (regcache, &regs, regnum);

      if (ptrace (PTRACE_SETREGS, tid, 0, (long) &regs) < 0)
	perror_with_name (_("Couldn't write registers"));

      if (regnum != -1)
	return;
    }

  if (regnum == -1 || !amd64_native_gregset_supplies_p (gdbarch, regnum))
    {
      elf_fpregset_t fpregs;

      if (have_ptrace_getregset == TRIBOOL_TRUE)
	{
	  char xstateregs[tdep->xsave_layout.sizeof_xsave];
	  struct iovec iov;

	  iov.iov_base = xstateregs;
	  iov.iov_len = sizeof (xstateregs);
	  if (ptrace (PTRACE_GETREGSET, tid,
		      (unsigned int) NT_X86_XSTATE, (long) &iov) < 0)
	    perror_with_name (_("Couldn't get extended state status"));

	  amd64_collect_xsave (regcache, regnum, xstateregs, 0);

	  if (ptrace (PTRACE_SETREGSET, tid,
		      (unsigned int) NT_X86_XSTATE, (long) &iov) < 0)
	    perror_with_name (_("Couldn't write extended state status"));
	}
      else
	{
	  if (ptrace (PTRACE_GETFPREGS, tid, 0, (long) &fpregs) < 0)
	    perror_with_name (_("Couldn't get floating point status"));

	  amd64_collect_fxsave (regcache, regnum, &fpregs);

	  if (ptrace (PTRACE_SETFPREGS, tid, 0, (long) &fpregs) < 0)
	    perror_with_name (_("Couldn't write floating point status"));
	}
    }
}


/* This function is called by libthread_db as part of its handling of
   a request for a thread's local storage address.  */

ps_err_e
ps_get_thread_area (struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
  if (gdbarch_bfd_arch_info (ph->thread->inf->arch ())->bits_per_word == 32)
    {
      unsigned int base_addr;
      ps_err_e result;

      result = x86_linux_get_thread_area (lwpid, (void *) (long) idx,
					  &base_addr);
      if (result == PS_OK)
	{
	  /* Extend the value to 64 bits.  Here it's assumed that
	     a "long" and a "void *" are the same.  */
	  (*base) = (void *) (long) base_addr;
	}
      return result;
    }
  else
    {

      /* FIXME: ezannoni-2003-07-09 see comment above about include
	 file order.  We could be getting bogus values for these two.  */
      gdb_assert (FS < ELF_NGREG);
      gdb_assert (GS < ELF_NGREG);
      switch (idx)
	{
	case FS:
	    {
	      unsigned long fs;
	      errno = 0;
	      fs = ptrace (PTRACE_PEEKUSER, lwpid,
			   offsetof (struct user_regs_struct, fs_base), 0);
	      if (errno == 0)
		{
		  *base = (void *) fs;
		  return PS_OK;
		}
	    }

	  break;

	case GS:
	    {
	      unsigned long gs;
	      errno = 0;
	      gs = ptrace (PTRACE_PEEKUSER, lwpid,
			   offsetof (struct user_regs_struct, gs_base), 0);
	      if (errno == 0)
		{
		  *base = (void *) gs;
		  return PS_OK;
		}
	    }
	  break;

	default:                   /* Should not happen.  */
	  return PS_BADADDR;
	}
    }
  return PS_ERR;               /* ptrace failed.  */
}


/* Convert a ptrace/host siginfo object, into/from the siginfo in the
   layout of the inferiors' architecture.  Returns true if any
   conversion was done; false otherwise.  If DIRECTION is 1, then copy
   from INF to PTRACE.  If DIRECTION is 0, copy from PTRACE to
   INF.  */

bool
amd64_linux_nat_target::low_siginfo_fixup (siginfo_t *ptrace,
					   gdb_byte *inf,
					   int direction)
{
  struct gdbarch *gdbarch = get_frame_arch (get_current_frame ());

  /* Is the inferior 32-bit?  If so, then do fixup the siginfo
     object.  */
  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    return amd64_linux_siginfo_fixup_common (ptrace, inf, direction,
					     FIXUP_32);
  /* No fixup for native x32 GDB.  */
  else if (gdbarch_addr_bit (gdbarch) == 32 && sizeof (void *) == 8)
    return amd64_linux_siginfo_fixup_common (ptrace, inf, direction,
					     FIXUP_X32);
  else
    return false;
}

void _initialize_amd64_linux_nat ();
void
_initialize_amd64_linux_nat ()
{
  amd64_native_gregset32_reg_offset = amd64_linux_gregset32_reg_offset;
  amd64_native_gregset32_num_regs = I386_LINUX_NUM_REGS;
  amd64_native_gregset64_reg_offset = amd64_linux_gregset_reg_offset;
  amd64_native_gregset64_num_regs = AMD64_LINUX_NUM_REGS;

  gdb_assert (ARRAY_SIZE (amd64_linux_gregset32_reg_offset)
	      == amd64_native_gregset32_num_regs);

  linux_target = &the_amd64_linux_nat_target;

  /* Add the target.  */
  add_inf_child_target (linux_target);
}
