/* Darwin support for GDB, the GNU debugger.
   Copyright (C) 1997-2024 Free Software Foundation, Inc.

   Contributed by Apple Computer, Inc.

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
#include "target.h"
#include "symfile.h"
#include "symtab.h"
#include "objfiles.h"
#include "gdbcmd.h"
#include "regcache.h"
#include "i386-tdep.h"
#include "i387-tdep.h"
#include "gdbarch.h"
#include "arch-utils.h"
#include "gdbcore.h"

#include "x86-nat.h"
#include "darwin-nat.h"
#include "i386-darwin-tdep.h"

#ifdef BFD64
#include "amd64-nat.h"
#include "amd64-tdep.h"
#include "amd64-darwin-tdep.h"
#endif

struct i386_darwin_nat_target final : public x86_nat_target<darwin_nat_target>
{
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;
};

static struct i386_darwin_nat_target darwin_target;

/* Read register values from the inferior process.
   If REGNO is -1, do this for all registers.
   Otherwise, REGNO specifies which register (so we can save time).  */

void
i386_darwin_nat_target::fetch_registers (struct regcache *regcache, int regno)
{
  thread_t current_thread = regcache->ptid ().tid ();
  int fetched = 0;
  struct gdbarch *gdbarch = regcache->arch ();

#ifdef BFD64
  if (gdbarch_ptr_bit (gdbarch) == 64)
    {
      if (regno == -1 || amd64_native_gregset_supplies_p (gdbarch, regno))
	{
	  x86_thread_state_t gp_regs;
	  unsigned int gp_count = x86_THREAD_STATE_COUNT;
	  kern_return_t ret;

	  ret = thread_get_state
	    (current_thread, x86_THREAD_STATE, (thread_state_t) & gp_regs,
	     &gp_count);
	  if (ret != KERN_SUCCESS)
	    {
	      warning (_("Error calling thread_get_state for "
			 "GP registers for thread 0x%lx\n"),
		       (unsigned long) current_thread);
	      MACH_CHECK_ERROR (ret);
	    }

	  /* Some kernels don't sanitize the values.  */
	  gp_regs.uts.ts64.__fs &= 0xffff;
	  gp_regs.uts.ts64.__gs &= 0xffff;

	  amd64_supply_native_gregset (regcache, &gp_regs.uts, -1);
	  fetched++;
	}

      if (regno == -1 || !amd64_native_gregset_supplies_p (gdbarch, regno))
	{
	  x86_float_state_t fp_regs;
	  unsigned int fp_count = x86_FLOAT_STATE_COUNT;
	  kern_return_t ret;

	  ret = thread_get_state
	    (current_thread, x86_FLOAT_STATE, (thread_state_t) & fp_regs,
	     &fp_count);
	  if (ret != KERN_SUCCESS)
	    {
	      warning (_("Error calling thread_get_state for "
			 "float registers for thread 0x%lx\n"),
		       (unsigned long) current_thread);
	      MACH_CHECK_ERROR (ret);
	    }
	  amd64_supply_fxsave (regcache, -1, &fp_regs.ufs.fs64.__fpu_fcw);
	  fetched++;
	}
    }
  else
#endif
    {
      if (regno == -1 || regno < I386_NUM_GREGS)
	{
	  x86_thread_state32_t gp_regs;
	  unsigned int gp_count = x86_THREAD_STATE32_COUNT;
	  kern_return_t ret;
	  int i;

	  ret = thread_get_state
	    (current_thread, x86_THREAD_STATE32, (thread_state_t) &gp_regs,
	     &gp_count);
	  if (ret != KERN_SUCCESS)
	    {
	      warning (_("Error calling thread_get_state for "
			 "GP registers for thread 0x%lx\n"),
		       (unsigned long) current_thread);
	      MACH_CHECK_ERROR (ret);
	    }
	  for (i = 0; i < I386_NUM_GREGS; i++)
	    regcache->raw_supply
	      (i, (char *) &gp_regs + i386_darwin_thread_state_reg_offset[i]);

	  fetched++;
	}

      if (regno == -1
	  || (regno >= I386_ST0_REGNUM && regno < I386_SSE_NUM_REGS))
	{
	  x86_float_state32_t fp_regs;
	  unsigned int fp_count = x86_FLOAT_STATE32_COUNT;
	  kern_return_t ret;

	  ret = thread_get_state
	    (current_thread, x86_FLOAT_STATE32, (thread_state_t) &fp_regs,
	     &fp_count);
	  if (ret != KERN_SUCCESS)
	    {
	      warning (_("Error calling thread_get_state for "
			 "float registers for thread 0x%lx\n"),
		       (unsigned long) current_thread);
	      MACH_CHECK_ERROR (ret);
	    }
	  i387_supply_fxsave (regcache, -1, &fp_regs.__fpu_fcw);
	  fetched++;
	}
    }

  if (! fetched)
    {
      warning (_("unknown register %d"), regno);
      regcache->raw_supply (regno, NULL);
    }
}

/* Store our register values back into the inferior.
   If REGNO is -1, do this for all registers.
   Otherwise, REGNO specifies which register (so we can save time).  */

void
i386_darwin_nat_target::store_registers (struct regcache *regcache,
					 int regno)
{
  thread_t current_thread = regcache->ptid ().tid ();
  struct gdbarch *gdbarch = regcache->arch ();

#ifdef BFD64
  if (gdbarch_ptr_bit (gdbarch) == 64)
    {
      if (regno == -1 || amd64_native_gregset_supplies_p (gdbarch, regno))
	{
	  x86_thread_state_t gp_regs;
	  kern_return_t ret;
	  unsigned int gp_count = x86_THREAD_STATE_COUNT;

	  ret = thread_get_state
	    (current_thread, x86_THREAD_STATE, (thread_state_t) &gp_regs,
	     &gp_count);
	  MACH_CHECK_ERROR (ret);
	  gdb_assert (gp_regs.tsh.flavor == x86_THREAD_STATE64);
	  gdb_assert (gp_regs.tsh.count == x86_THREAD_STATE64_COUNT);

	  amd64_collect_native_gregset (regcache, &gp_regs.uts, regno);

	  /* Some kernels don't sanitize the values.  */
	  gp_regs.uts.ts64.__fs &= 0xffff;
	  gp_regs.uts.ts64.__gs &= 0xffff;

	  ret = thread_set_state (current_thread, x86_THREAD_STATE,
				  (thread_state_t) &gp_regs,
				  x86_THREAD_STATE_COUNT);
	  MACH_CHECK_ERROR (ret);
	}

      if (regno == -1 || !amd64_native_gregset_supplies_p (gdbarch, regno))
	{
	  x86_float_state_t fp_regs;
	  kern_return_t ret;
	  unsigned int fp_count = x86_FLOAT_STATE_COUNT;

	  ret = thread_get_state
	    (current_thread, x86_FLOAT_STATE, (thread_state_t) & fp_regs,
	     &fp_count);
	  MACH_CHECK_ERROR (ret);
	  gdb_assert (fp_regs.fsh.flavor == x86_FLOAT_STATE64);
	  gdb_assert (fp_regs.fsh.count == x86_FLOAT_STATE64_COUNT);

	  amd64_collect_fxsave (regcache, regno, &fp_regs.ufs.fs64.__fpu_fcw);

	  ret = thread_set_state (current_thread, x86_FLOAT_STATE,
				  (thread_state_t) & fp_regs,
				  x86_FLOAT_STATE_COUNT);
	  MACH_CHECK_ERROR (ret);
	}
    }
  else
#endif
    {
      if (regno == -1 || regno < I386_NUM_GREGS)
	{
	  x86_thread_state32_t gp_regs;
	  kern_return_t ret;
	  unsigned int gp_count = x86_THREAD_STATE32_COUNT;
	  int i;

	  ret = thread_get_state
	    (current_thread, x86_THREAD_STATE32, (thread_state_t) &gp_regs,
	     &gp_count);
	  MACH_CHECK_ERROR (ret);

	  for (i = 0; i < I386_NUM_GREGS; i++)
	    if (regno == -1 || regno == i)
	      regcache->raw_collect
		(i, (char *) &gp_regs + i386_darwin_thread_state_reg_offset[i]);

	  ret = thread_set_state (current_thread, x86_THREAD_STATE32,
				  (thread_state_t) &gp_regs,
				  x86_THREAD_STATE32_COUNT);
	  MACH_CHECK_ERROR (ret);
	}

      if (regno == -1
	  || (regno >= I386_ST0_REGNUM && regno < I386_SSE_NUM_REGS))
	{
	  x86_float_state32_t fp_regs;
	  unsigned int fp_count = x86_FLOAT_STATE32_COUNT;
	  kern_return_t ret;

	  ret = thread_get_state
	    (current_thread, x86_FLOAT_STATE32, (thread_state_t) & fp_regs,
	     &fp_count);
	  MACH_CHECK_ERROR (ret);

	  i387_collect_fxsave (regcache, regno, &fp_regs.__fpu_fcw);

	  ret = thread_set_state (current_thread, x86_FLOAT_STATE32,
				  (thread_state_t) &fp_regs,
				  x86_FLOAT_STATE32_COUNT);
	  MACH_CHECK_ERROR (ret);
	}
    }
}

/* Support for debug registers, boosted mostly from i386-linux-nat.c.  */

static void
i386_darwin_dr_set (int regnum, CORE_ADDR value)
{
  thread_t current_thread;
  x86_debug_state_t dr_regs;
  kern_return_t ret;
  unsigned int dr_count;

  gdb_assert (regnum >= 0 && regnum <= DR_CONTROL);

  current_thread = inferior_ptid.tid ();

  dr_regs.dsh.flavor = x86_DEBUG_STATE;
  dr_regs.dsh.count = x86_DEBUG_STATE_COUNT;
  dr_count = x86_DEBUG_STATE_COUNT;
  ret = thread_get_state (current_thread, x86_DEBUG_STATE,
			  (thread_state_t) &dr_regs, &dr_count);
  MACH_CHECK_ERROR (ret);

  switch (dr_regs.dsh.flavor)
    {
    case x86_DEBUG_STATE32:
      switch (regnum)
	{
	case 0:
	  dr_regs.uds.ds32.__dr0 = value;
	  break;
	case 1:
	  dr_regs.uds.ds32.__dr1 = value;
	  break;
	case 2:
	  dr_regs.uds.ds32.__dr2 = value;
	  break;
	case 3:
	  dr_regs.uds.ds32.__dr3 = value;
	  break;
	case 4:
	  dr_regs.uds.ds32.__dr4 = value;
	  break;
	case 5:
	  dr_regs.uds.ds32.__dr5 = value;
	  break;
	case 6:
	  dr_regs.uds.ds32.__dr6 = value;
	  break;
	case 7:
	  dr_regs.uds.ds32.__dr7 = value;
	  break;
	}
      break;
#ifdef BFD64
    case x86_DEBUG_STATE64:
      switch (regnum)
	{
	case 0:
	  dr_regs.uds.ds64.__dr0 = value;
	  break;
	case 1:
	  dr_regs.uds.ds64.__dr1 = value;
	  break;
	case 2:
	  dr_regs.uds.ds64.__dr2 = value;
	  break;
	case 3:
	  dr_regs.uds.ds64.__dr3 = value;
	  break;
	case 4:
	  dr_regs.uds.ds64.__dr4 = value;
	  break;
	case 5:
	  dr_regs.uds.ds64.__dr5 = value;
	  break;
	case 6:
	  dr_regs.uds.ds64.__dr6 = value;
	  break;
	case 7:
	  dr_regs.uds.ds64.__dr7 = value;
	  break;
	}
      break;
#endif
    }

  ret = thread_set_state (current_thread, dr_regs.dsh.flavor,
			  (thread_state_t) &dr_regs.uds, dr_count);

  MACH_CHECK_ERROR (ret);
}

static CORE_ADDR
i386_darwin_dr_get (int regnum)
{
  thread_t current_thread;
  x86_debug_state_t dr_regs;
  kern_return_t ret;
  unsigned int dr_count;

  gdb_assert (regnum >= 0 && regnum <= DR_CONTROL);

  current_thread = inferior_ptid.tid ();

  dr_regs.dsh.flavor = x86_DEBUG_STATE;
  dr_regs.dsh.count = x86_DEBUG_STATE_COUNT;
  dr_count = x86_DEBUG_STATE_COUNT;
  ret = thread_get_state (current_thread, x86_DEBUG_STATE,
			  (thread_state_t) &dr_regs, &dr_count);
  MACH_CHECK_ERROR (ret);

  switch (dr_regs.dsh.flavor)
    {
    case x86_DEBUG_STATE32:
      switch (regnum)
	{
	case 0:
	  return dr_regs.uds.ds32.__dr0;
	case 1:
	  return dr_regs.uds.ds32.__dr1;
	case 2:
	  return dr_regs.uds.ds32.__dr2;
	case 3:
	  return dr_regs.uds.ds32.__dr3;
	case 4:
	  return dr_regs.uds.ds32.__dr4;
	case 5:
	  return dr_regs.uds.ds32.__dr5;
	case 6:
	  return dr_regs.uds.ds32.__dr6;
	case 7:
	  return dr_regs.uds.ds32.__dr7;
	default:
	  return -1;
	}
      break;
#ifdef BFD64
    case x86_DEBUG_STATE64:
      switch (regnum)
	{
	case 0:
	  return dr_regs.uds.ds64.__dr0;
	case 1:
	  return dr_regs.uds.ds64.__dr1;
	case 2:
	  return dr_regs.uds.ds64.__dr2;
	case 3:
	  return dr_regs.uds.ds64.__dr3;
	case 4:
	  return dr_regs.uds.ds64.__dr4;
	case 5:
	  return dr_regs.uds.ds64.__dr5;
	case 6:
	  return dr_regs.uds.ds64.__dr6;
	case 7:
	  return dr_regs.uds.ds64.__dr7;
	default:
	  return -1;
	}
      break;
#endif
    default:
      return -1;
    }
}

static void
i386_darwin_dr_set_control (unsigned long control)
{
  i386_darwin_dr_set (DR_CONTROL, control);
}

static void
i386_darwin_dr_set_addr (int regnum, CORE_ADDR addr)
{
  gdb_assert (regnum >= 0 && regnum <= DR_LASTADDR - DR_FIRSTADDR);

  i386_darwin_dr_set (DR_FIRSTADDR + regnum, addr);
}

static CORE_ADDR
i386_darwin_dr_get_addr (int regnum)
{
  return i386_darwin_dr_get (regnum);
}

static unsigned long
i386_darwin_dr_get_status (void)
{
  return i386_darwin_dr_get (DR_STATUS);
}

static unsigned long
i386_darwin_dr_get_control (void)
{
  return i386_darwin_dr_get (DR_CONTROL);
}

void
darwin_check_osabi (darwin_inferior *inf, thread_t thread)
{
  if (gdbarch_osabi (current_inferior ()->arch ()) == GDB_OSABI_UNKNOWN)
    {
      /* Attaching to a process.  Let's figure out what kind it is.  */
      x86_thread_state_t gp_regs;
      unsigned int gp_count = x86_THREAD_STATE_COUNT;
      kern_return_t ret;

      ret = thread_get_state (thread, x86_THREAD_STATE,
			      (thread_state_t) &gp_regs, &gp_count);
      if (ret != KERN_SUCCESS)
	{
	  MACH_CHECK_ERROR (ret);
	  return;
	}

      gdbarch_info info;
      gdbarch_info_fill (&info);
      info.byte_order = gdbarch_byte_order (current_inferior ()->arch ());
      info.osabi = GDB_OSABI_DARWIN;
      if (gp_regs.tsh.flavor == x86_THREAD_STATE64)
	info.bfd_arch_info = bfd_lookup_arch (bfd_arch_i386,
					      bfd_mach_x86_64);
      else
	info.bfd_arch_info = bfd_lookup_arch (bfd_arch_i386,
					      bfd_mach_i386_i386);
      gdbarch_update_p (info);
    }
}

#define X86_EFLAGS_T 0x100UL

/* Returning from a signal trampoline is done by calling a
   special system call (sigreturn).  This system call
   restores the registers that were saved when the signal was
   raised, including %eflags/%rflags.  That means that single-stepping
   won't work.  Instead, we'll have to modify the signal context
   that's about to be restored, and set the trace flag there.  */

static int
i386_darwin_sstep_at_sigreturn (x86_thread_state_t *regs)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  static const gdb_byte darwin_syscall[] = { 0xcd, 0x80 }; /* int 0x80 */
  gdb_byte buf[sizeof (darwin_syscall)];

  /* Check if PC is at a sigreturn system call.  */
  if (target_read_memory (regs->uts.ts32.__eip, buf, sizeof (buf)) == 0
      && memcmp (buf, darwin_syscall, sizeof (darwin_syscall)) == 0
      && regs->uts.ts32.__eax == 0xb8 /* SYS_sigreturn */)
    {
      ULONGEST uctx_addr;
      ULONGEST mctx_addr;
      ULONGEST flags_addr;
      unsigned int eflags;

      uctx_addr = read_memory_unsigned_integer
		    (regs->uts.ts32.__esp + 4, 4, byte_order);
      mctx_addr = read_memory_unsigned_integer
		    (uctx_addr + 28, 4, byte_order);

      flags_addr = mctx_addr + 12 + 9 * 4;
      read_memory (flags_addr, (gdb_byte *) &eflags, 4);
      eflags |= X86_EFLAGS_T;
      write_memory (flags_addr, (gdb_byte *) &eflags, 4);

      return 1;
    }
  return 0;
}

#ifdef BFD64
static int
amd64_darwin_sstep_at_sigreturn (x86_thread_state_t *regs)
{
  bfd_endian byte_order = gdbarch_byte_order (current_inferior ()->arch ());
  static const gdb_byte darwin_syscall[] = { 0x0f, 0x05 }; /* syscall */
  gdb_byte buf[sizeof (darwin_syscall)];

  /* Check if PC is at a sigreturn system call.  */
  if (target_read_memory (regs->uts.ts64.__rip, buf, sizeof (buf)) == 0
      && memcmp (buf, darwin_syscall, sizeof (darwin_syscall)) == 0
      && (regs->uts.ts64.__rax & 0xffffffff) == 0x20000b8 /* SYS_sigreturn */)
    {
      ULONGEST mctx_addr;
      ULONGEST flags_addr;
      unsigned int rflags;

      mctx_addr = read_memory_unsigned_integer
		    (regs->uts.ts64.__rdi + 48, 8, byte_order);
      flags_addr = mctx_addr + 16 + 17 * 8;

      /* AMD64 is little endian.  */
      read_memory (flags_addr, (gdb_byte *) &rflags, 4);
      rflags |= X86_EFLAGS_T;
      write_memory (flags_addr, (gdb_byte *) &rflags, 4);

      return 1;
    }
  return 0;
}
#endif

void
darwin_set_sstep (thread_t thread, int enable)
{
  x86_thread_state_t regs;
  unsigned int count = x86_THREAD_STATE_COUNT;
  kern_return_t kret;

  kret = thread_get_state (thread, x86_THREAD_STATE,
			   (thread_state_t) &regs, &count);
  if (kret != KERN_SUCCESS)
    {
      warning (_("darwin_set_sstep: error %x, thread=%x\n"),
	       kret, thread);
      return;
    }

  switch (regs.tsh.flavor)
    {
    case x86_THREAD_STATE32:
      {
	__uint32_t bit = enable ? X86_EFLAGS_T : 0;

	if (enable && i386_darwin_sstep_at_sigreturn (&regs))
	  return;
	if ((regs.uts.ts32.__eflags & X86_EFLAGS_T) == bit)
	  return;
	regs.uts.ts32.__eflags
	  = (regs.uts.ts32.__eflags & ~X86_EFLAGS_T) | bit;
	kret = thread_set_state (thread, x86_THREAD_STATE,
				 (thread_state_t) &regs, count);
	MACH_CHECK_ERROR (kret);
      }
      break;
#ifdef BFD64
    case x86_THREAD_STATE64:
      {
	__uint64_t bit = enable ? X86_EFLAGS_T : 0;

	if (enable && amd64_darwin_sstep_at_sigreturn (&regs))
	  return;
	if ((regs.uts.ts64.__rflags & X86_EFLAGS_T) == bit)
	  return;
	regs.uts.ts64.__rflags
	  = (regs.uts.ts64.__rflags & ~X86_EFLAGS_T) | bit;
	kret = thread_set_state (thread, x86_THREAD_STATE,
				 (thread_state_t) &regs, count);
	MACH_CHECK_ERROR (kret);
      }
      break;
#endif
    default:
      error (_("darwin_set_sstep: unknown flavour: %d"), regs.tsh.flavor);
    }
}

void _initialize_i386_darwin_nat ();
void
_initialize_i386_darwin_nat ()
{
#ifdef BFD64
  amd64_native_gregset64_reg_offset = amd64_darwin_thread_state_reg_offset;
  amd64_native_gregset64_num_regs = amd64_darwin_thread_state_num_regs;
  amd64_native_gregset32_reg_offset = i386_darwin_thread_state_reg_offset;
  amd64_native_gregset32_num_regs = i386_darwin_thread_state_num_regs;
#endif

  x86_dr_low.set_control = i386_darwin_dr_set_control;
  x86_dr_low.set_addr = i386_darwin_dr_set_addr;
  x86_dr_low.get_addr = i386_darwin_dr_get_addr;
  x86_dr_low.get_status = i386_darwin_dr_get_status;
  x86_dr_low.get_control = i386_darwin_dr_get_control;

  /* Let's assume that the kernel is 64 bits iff the executable is.  */
#ifdef __x86_64__
  x86_set_debug_register_length (8);
#else
  x86_set_debug_register_length (4);
#endif

  add_inf_child_target (&darwin_target);
}
