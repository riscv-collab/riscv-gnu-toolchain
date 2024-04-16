/* Native-dependent code for GNU/Linux i386.

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
#include "elf/common.h"
#include "nat/gdb_ptrace.h"
#include <sys/uio.h>
#include "gregset.h"
#include "gdb_proc_service.h"

#include "i386-linux-nat.h"
#include "i387-tdep.h"
#include "i386-tdep.h"
#include "i386-linux-tdep.h"
#include "gdbsupport/x86-xstate.h"

#include "x86-linux-nat.h"
#include "nat/linux-ptrace.h"
#include "inf-ptrace.h"

struct i386_linux_nat_target final : public x86_linux_nat_target
{
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  /* Override the default ptrace resume method.  */
  void low_resume (ptid_t ptid, int step, enum gdb_signal sig) override;
};

static i386_linux_nat_target the_i386_linux_nat_target;

/* The register sets used in GNU/Linux ELF core-dumps are identical to
   the register sets in `struct user' that is used for a.out
   core-dumps, and is also used by `ptrace'.  The corresponding types
   are `elf_gregset_t' for the general-purpose registers (with
   `elf_greg_t' the type of a single GP register) and `elf_fpregset_t'
   for the floating-point registers.

   Those types used to be available under the names `gregset_t' and
   `fpregset_t' too, and this file used those names in the past.  But
   those names are now used for the register sets used in the
   `mcontext_t' type, and have a different size and layout.  */

/* Which ptrace request retrieves which registers?
   These apply to the corresponding SET requests as well.  */

#define GETREGS_SUPPLIES(regno) \
  ((0 <= (regno) && (regno) <= 15) || (regno) == I386_LINUX_ORIG_EAX_REGNUM)

#define GETFPXREGS_SUPPLIES(regno) \
  (I386_ST0_REGNUM <= (regno) && (regno) < I386_SSE_NUM_REGS)

#define GETXSTATEREGS_SUPPLIES(regno) \
  (I386_ST0_REGNUM <= (regno) && (regno) < I386_PKEYS_NUM_REGS)

/* Does the current host support the GETREGS request?  */
int have_ptrace_getregs =
#ifdef HAVE_PTRACE_GETREGS
  1
#else
  0
#endif
;

/* Does the current host support the GETFPXREGS request?  The header
   file may or may not define it, and even if it is defined, the
   kernel will return EIO if it's running on a pre-SSE processor.

   My instinct is to attach this to some architecture- or
   target-specific data structure, but really, a particular GDB
   process can only run on top of one kernel at a time.  So it's okay
   for this to be a simple variable.  */
int have_ptrace_getfpxregs =
#ifdef HAVE_PTRACE_GETFPXREGS
  -1
#else
  0
#endif
;


/* Accessing registers through the U area, one at a time.  */

/* Fetch one register.  */

static void
fetch_register (struct regcache *regcache, int regno)
{
  pid_t tid;
  int val;

  gdb_assert (!have_ptrace_getregs);
  if (i386_linux_gregset_reg_offset[regno] == -1)
    {
      regcache->raw_supply (regno, NULL);
      return;
    }

  tid = get_ptrace_pid (regcache->ptid ());

  errno = 0;
  val = ptrace (PTRACE_PEEKUSER, tid,
		i386_linux_gregset_reg_offset[regno], 0);
  if (errno != 0)
    error (_("Couldn't read register %s (#%d): %s."), 
	   gdbarch_register_name (regcache->arch (), regno),
	   regno, safe_strerror (errno));

  regcache->raw_supply (regno, &val);
}

/* Store one register.  */

static void
store_register (const struct regcache *regcache, int regno)
{
  pid_t tid;
  int val;

  gdb_assert (!have_ptrace_getregs);
  if (i386_linux_gregset_reg_offset[regno] == -1)
    return;

  tid = get_ptrace_pid (regcache->ptid ());

  errno = 0;
  regcache->raw_collect (regno, &val);
  ptrace (PTRACE_POKEUSER, tid,
	  i386_linux_gregset_reg_offset[regno], val);
  if (errno != 0)
    error (_("Couldn't write register %s (#%d): %s."),
	   gdbarch_register_name (regcache->arch (), regno),
	   regno, safe_strerror (errno));
}


/* Transfering the general-purpose registers between GDB, inferiors
   and core files.  */

/* Fill GDB's register array with the general-purpose register values
   in *GREGSETP.  */

void
supply_gregset (struct regcache *regcache, const elf_gregset_t *gregsetp)
{
  const gdb_byte *regp = (const gdb_byte *) gregsetp;
  int i;

  for (i = 0; i < I386_NUM_GREGS; i++)
    regcache->raw_supply (i, regp + i386_linux_gregset_reg_offset[i]);

  if (I386_LINUX_ORIG_EAX_REGNUM
	< gdbarch_num_regs (regcache->arch ()))
    regcache->raw_supply
      (I386_LINUX_ORIG_EAX_REGNUM,
       regp + i386_linux_gregset_reg_offset[I386_LINUX_ORIG_EAX_REGNUM]);
}

/* Fill register REGNO (if it is a general-purpose register) in
   *GREGSETPS with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */

void
fill_gregset (const struct regcache *regcache,
	      elf_gregset_t *gregsetp, int regno)
{
  gdb_byte *regp = (gdb_byte *) gregsetp;
  int i;

  for (i = 0; i < I386_NUM_GREGS; i++)
    if (regno == -1 || regno == i)
      regcache->raw_collect (i, regp + i386_linux_gregset_reg_offset[i]);

  if ((regno == -1 || regno == I386_LINUX_ORIG_EAX_REGNUM)
      && I386_LINUX_ORIG_EAX_REGNUM
	   < gdbarch_num_regs (regcache->arch ()))
    regcache->raw_collect
      (I386_LINUX_ORIG_EAX_REGNUM,
       regp + i386_linux_gregset_reg_offset[I386_LINUX_ORIG_EAX_REGNUM]);
}

#ifdef HAVE_PTRACE_GETREGS

/* Fetch all general-purpose registers from process/thread TID and
   store their values in GDB's register array.  */

static void
fetch_regs (struct regcache *regcache, int tid)
{
  elf_gregset_t regs;
  elf_gregset_t *regs_p = &regs;

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

  supply_gregset (regcache, (const elf_gregset_t *) regs_p);
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

static void fetch_regs (struct regcache *regcache, int tid) {}
static void store_regs (const struct regcache *regcache, int tid, int regno) {}

#endif


/* Transfering floating-point registers between GDB, inferiors and cores.  */

/* Fill GDB's register array with the floating-point register values in
   *FPREGSETP.  */

void 
supply_fpregset (struct regcache *regcache, const elf_fpregset_t *fpregsetp)
{
  i387_supply_fsave (regcache, -1, fpregsetp);
}

/* Fill register REGNO (if it is a floating-point register) in
   *FPREGSETP with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */

void
fill_fpregset (const struct regcache *regcache,
	       elf_fpregset_t *fpregsetp, int regno)
{
  i387_collect_fsave (regcache, regno, fpregsetp);
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

static void
fetch_fpregs (struct regcache *regcache, int tid)
{
}

static void
store_fpregs (const struct regcache *regcache, int tid, int regno)
{
}

#endif


/* Transfering floating-point and SSE registers to and from GDB.  */

/* Fetch all registers covered by the PTRACE_GETREGSET request from
   process/thread TID and store their values in GDB's register array.
   Return non-zero if successful, zero otherwise.  */

static int
fetch_xstateregs (struct regcache *regcache, int tid)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  char xstateregs[tdep->xsave_layout.sizeof_xsave];
  struct iovec iov;

  if (have_ptrace_getregset != TRIBOOL_TRUE)
    return 0;

  iov.iov_base = xstateregs;
  iov.iov_len = sizeof(xstateregs);
  if (ptrace (PTRACE_GETREGSET, tid, (unsigned int) NT_X86_XSTATE,
	      &iov) < 0)
    perror_with_name (_("Couldn't read extended state status"));

  i387_supply_xsave (regcache, -1, xstateregs);
  return 1;
}

/* Store all valid registers in GDB's register array covered by the
   PTRACE_SETREGSET request into the process/thread specified by TID.
   Return non-zero if successful, zero otherwise.  */

static int
store_xstateregs (const struct regcache *regcache, int tid, int regno)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  char xstateregs[tdep->xsave_layout.sizeof_xsave];
  struct iovec iov;

  if (have_ptrace_getregset != TRIBOOL_TRUE)
    return 0;
  
  iov.iov_base = xstateregs;
  iov.iov_len = sizeof(xstateregs);
  if (ptrace (PTRACE_GETREGSET, tid, (unsigned int) NT_X86_XSTATE,
	      &iov) < 0)
    perror_with_name (_("Couldn't read extended state status"));

  i387_collect_xsave (regcache, regno, xstateregs, 0);

  if (ptrace (PTRACE_SETREGSET, tid, (unsigned int) NT_X86_XSTATE,
	      (int) &iov) < 0)
    perror_with_name (_("Couldn't write extended state status"));

  return 1;
}

#ifdef HAVE_PTRACE_GETFPXREGS

/* Fetch all registers covered by the PTRACE_GETFPXREGS request from
   process/thread TID and store their values in GDB's register array.
   Return non-zero if successful, zero otherwise.  */

static int
fetch_fpxregs (struct regcache *regcache, int tid)
{
  elf_fpxregset_t fpxregs;

  if (! have_ptrace_getfpxregs)
    return 0;

  if (ptrace (PTRACE_GETFPXREGS, tid, 0, (int) &fpxregs) < 0)
    {
      if (errno == EIO)
	{
	  have_ptrace_getfpxregs = 0;
	  return 0;
	}

      perror_with_name (_("Couldn't read floating-point and SSE registers"));
    }

  i387_supply_fxsave (regcache, -1, (const elf_fpxregset_t *) &fpxregs);
  return 1;
}

/* Store all valid registers in GDB's register array covered by the
   PTRACE_SETFPXREGS request into the process/thread specified by TID.
   Return non-zero if successful, zero otherwise.  */

static int
store_fpxregs (const struct regcache *regcache, int tid, int regno)
{
  elf_fpxregset_t fpxregs;

  if (! have_ptrace_getfpxregs)
    return 0;
  
  if (ptrace (PTRACE_GETFPXREGS, tid, 0, &fpxregs) == -1)
    {
      if (errno == EIO)
	{
	  have_ptrace_getfpxregs = 0;
	  return 0;
	}

      perror_with_name (_("Couldn't read floating-point and SSE registers"));
    }

  i387_collect_fxsave (regcache, regno, &fpxregs);

  if (ptrace (PTRACE_SETFPXREGS, tid, 0, &fpxregs) == -1)
    perror_with_name (_("Couldn't write floating-point and SSE registers"));

  return 1;
}

#else

static int
fetch_fpxregs (struct regcache *regcache, int tid)
{
  return 0;
}

static int
store_fpxregs (const struct regcache *regcache, int tid, int regno)
{
  return 0;
}

#endif /* HAVE_PTRACE_GETFPXREGS */


/* Transferring arbitrary registers between GDB and inferior.  */

/* Fetch register REGNO from the child process.  If REGNO is -1, do
   this for all registers (including the floating point and SSE
   registers).  */

void
i386_linux_nat_target::fetch_registers (struct regcache *regcache, int regno)
{
  pid_t tid;

  /* Use the old method of peeking around in `struct user' if the
     GETREGS request isn't available.  */
  if (!have_ptrace_getregs)
    {
      int i;

      for (i = 0; i < gdbarch_num_regs (regcache->arch ()); i++)
	if (regno == -1 || regno == i)
	  fetch_register (regcache, i);

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
      if (!have_ptrace_getregs)
	{
	  fetch_registers (regcache, regno);
	  return;
	}

      if (fetch_xstateregs (regcache, tid))
	return;
      if (fetch_fpxregs (regcache, tid))
	return;
      fetch_fpregs (regcache, tid);
      return;
    }

  if (GETREGS_SUPPLIES (regno))
    {
      fetch_regs (regcache, tid);
      return;
    }

  if (GETXSTATEREGS_SUPPLIES (regno))
    {
      if (fetch_xstateregs (regcache, tid))
	return;
    }

  if (GETFPXREGS_SUPPLIES (regno))
    {
      if (fetch_fpxregs (regcache, tid))
	return;

      /* Either our processor or our kernel doesn't support the SSE
	 registers, so read the FP registers in the traditional way,
	 and fill the SSE registers with dummy values.  It would be
	 more graceful to handle differences in the register set using
	 gdbarch.  Until then, this will at least make things work
	 plausibly.  */
      fetch_fpregs (regcache, tid);
      return;
    }

  internal_error (_("Got request for bad register number %d."), regno);
}

/* Store register REGNO back into the child process.  If REGNO is -1,
   do this for all registers (including the floating point and SSE
   registers).  */
void
i386_linux_nat_target::store_registers (struct regcache *regcache, int regno)
{
  pid_t tid;

  /* Use the old method of poking around in `struct user' if the
     SETREGS request isn't available.  */
  if (!have_ptrace_getregs)
    {
      int i;

      for (i = 0; i < gdbarch_num_regs (regcache->arch ()); i++)
	if (regno == -1 || regno == i)
	  store_register (regcache, i);

      return;
    }

  tid = get_ptrace_pid (regcache->ptid ());

  /* Use the PTRACE_SETFPXREGS requests whenever possible, since it
     transfers more registers in one system call.  But remember that
     store_fpxregs can fail, and return zero.  */
  if (regno == -1)
    {
      store_regs (regcache, tid, regno);
      if (store_xstateregs (regcache, tid, regno))
	return;
      if (store_fpxregs (regcache, tid, regno))
	return;
      store_fpregs (regcache, tid, regno);
      return;
    }

  if (GETREGS_SUPPLIES (regno))
    {
      store_regs (regcache, tid, regno);
      return;
    }

  if (GETXSTATEREGS_SUPPLIES (regno))
    {
      if (store_xstateregs (regcache, tid, regno))
	return;
    }

  if (GETFPXREGS_SUPPLIES (regno))
    {
      if (store_fpxregs (regcache, tid, regno))
	return;

      /* Either our processor or our kernel doesn't support the SSE
	 registers, so just write the FP registers in the traditional
	 way.  */
      store_fpregs (regcache, tid, regno);
      return;
    }

  internal_error (_("Got request to store bad register number %d."), regno);
}


/* Called by libthread_db.  Returns a pointer to the thread local
   storage (or its descriptor).  */

ps_err_e
ps_get_thread_area (struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
  unsigned int base_addr;
  ps_err_e result;

  result = x86_linux_get_thread_area (lwpid, (void *) idx, &base_addr);

  if (result == PS_OK)
    *(int *) base = base_addr;

  return result;
}


/* The instruction for a GNU/Linux system call is:
       int $0x80
   or 0xcd 0x80.  */

static const unsigned char linux_syscall[] = { 0xcd, 0x80 };

#define LINUX_SYSCALL_LEN (sizeof linux_syscall)

/* The system call number is stored in the %eax register.  */
#define LINUX_SYSCALL_REGNUM I386_EAX_REGNUM

/* We are specifically interested in the sigreturn and rt_sigreturn
   system calls.  */

#ifndef SYS_sigreturn
#define SYS_sigreturn		0x77
#endif
#ifndef SYS_rt_sigreturn
#define SYS_rt_sigreturn	0xad
#endif

/* Offset to saved processor flags, from <asm/sigcontext.h>.  */
#define LINUX_SIGCONTEXT_EFLAGS_OFFSET (64)

/* Resume execution of the inferior process.
   If STEP is nonzero, single-step it.
   If SIGNAL is nonzero, give it that signal.  */

void
i386_linux_nat_target::low_resume (ptid_t ptid, int step, enum gdb_signal signal)
{
  int pid = ptid.lwp ();
  int request;

  if (catch_syscall_enabled ())
   request = PTRACE_SYSCALL;
  else
    request = PTRACE_CONT;

  if (step)
    {
      struct regcache *regcache = get_thread_regcache (this, ptid);
      struct gdbarch *gdbarch = regcache->arch ();
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      ULONGEST pc;
      gdb_byte buf[LINUX_SYSCALL_LEN];

      request = PTRACE_SINGLESTEP;

      regcache_cooked_read_unsigned (regcache,
				     gdbarch_pc_regnum (gdbarch), &pc);

      /* Returning from a signal trampoline is done by calling a
	 special system call (sigreturn or rt_sigreturn, see
	 i386-linux-tdep.c for more information).  This system call
	 restores the registers that were saved when the signal was
	 raised, including %eflags.  That means that single-stepping
	 won't work.  Instead, we'll have to modify the signal context
	 that's about to be restored, and set the trace flag there.  */

      /* First check if PC is at a system call.  */
      if (target_read_memory (pc, buf, LINUX_SYSCALL_LEN) == 0
	  && memcmp (buf, linux_syscall, LINUX_SYSCALL_LEN) == 0)
	{
	  ULONGEST syscall;
	  regcache_cooked_read_unsigned (regcache,
					 LINUX_SYSCALL_REGNUM, &syscall);

	  /* Then check the system call number.  */
	  if (syscall == SYS_sigreturn || syscall == SYS_rt_sigreturn)
	    {
	      ULONGEST sp, addr;
	      unsigned long int eflags;

	      regcache_cooked_read_unsigned (regcache, I386_ESP_REGNUM, &sp);
	      if (syscall == SYS_rt_sigreturn)
		addr = read_memory_unsigned_integer (sp + 8, 4, byte_order)
		  + 20;
	      else
		addr = sp;

	      /* Set the trace flag in the context that's about to be
		 restored.  */
	      addr += LINUX_SIGCONTEXT_EFLAGS_OFFSET;
	      read_memory (addr, (gdb_byte *) &eflags, 4);
	      eflags |= 0x0100;
	      write_memory (addr, (gdb_byte *) &eflags, 4);
	    }
	}
    }

  if (ptrace (request, pid, 0, gdb_signal_to_host (signal)) == -1)
    perror_with_name (("ptrace"));
}

void _initialize_i386_linux_nat ();
void
_initialize_i386_linux_nat ()
{
  linux_target = &the_i386_linux_nat_target;

  /* Add the target.  */
  add_inf_child_target (linux_target);
}
