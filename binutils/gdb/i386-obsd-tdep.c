/* Target-dependent code for OpenBSD/i386.

   Copyright (C) 1988-2024 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include "frame.h"
#include "frame-unwind.h"
#include "gdbcore.h"
#include "regcache.h"
#include "regset.h"
#include "symtab.h"
#include "objfiles.h"
#include "osabi.h"
#include "target.h"
#include "trad-frame.h"

#include "obsd-tdep.h"
#include "i386-tdep.h"
#include "i387-tdep.h"
#include "solib-svr4.h"
#include "bsd-uthread.h"

/* Support for signal handlers.  */

/* Since OpenBSD 3.2, the sigtramp routine is mapped at a random page
   in virtual memory.  The randomness makes it somewhat tricky to
   detect it, but fortunately we can rely on the fact that the start
   of the sigtramp routine is page-aligned.  We recognize the
   trampoline by looking for the code that invokes the sigreturn
   system call.  The offset where we can find that code varies from
   release to release.

   By the way, the mapping mentioned above is read-only, so you cannot
   place a breakpoint in the signal trampoline.  */

/* Default page size.  */
static const int i386obsd_page_size = 4096;

/* Offset for sigreturn(2).  */
static const int i386obsd_sigreturn_offset[] = {
  0x0a,				/* OpenBSD 3.2 */
  0x14,				/* OpenBSD 3.6 */
  0x3a,				/* OpenBSD 3.8 */
  -1
};

/* Return whether THIS_FRAME corresponds to an OpenBSD sigtramp
   routine.  */

static int
i386obsd_sigtramp_p (frame_info_ptr this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  CORE_ADDR start_pc = (pc & ~(i386obsd_page_size - 1));
  /* The call sequence invoking sigreturn(2).  */
  const gdb_byte sigreturn[] =
  {
    0xb8,
    0x67, 0x00, 0x00, 0x00,	/* movl $SYS_sigreturn, %eax */
    0xcd, 0x80			/* int $0x80 */
  };
  size_t buflen = sizeof sigreturn;
  const int *offset;
  gdb_byte *buf;
  const char *name;

  /* If the function has a valid symbol name, it isn't a
     trampoline.  */
  find_pc_partial_function (pc, &name, NULL, NULL);
  if (name != NULL)
    return 0;

  /* If the function lives in a valid section (even without a starting
     point) it isn't a trampoline.  */
  if (find_pc_section (pc) != NULL)
    return 0;

  /* Allocate buffer.  */
  buf = (gdb_byte *) alloca (buflen);

  /* Loop over all offsets.  */
  for (offset = i386obsd_sigreturn_offset; *offset != -1; offset++)
    {
      /* If we can't read the instructions, return zero.  */
      if (!safe_frame_unwind_memory (this_frame, start_pc + *offset,
				     {buf, buflen}))
	return 0;

      /* Check for sigreturn(2).  */
      if (memcmp (buf, sigreturn, buflen) == 0)
	return 1;
    }

  return 0;
}

/* Mapping between the general-purpose registers in `struct reg'
   format and GDB's register cache layout.  */

/* From <machine/reg.h>.  */
static int i386obsd_r_reg_offset[] =
{
  0 * 4,			/* %eax */
  1 * 4,			/* %ecx */
  2 * 4,			/* %edx */
  3 * 4,			/* %ebx */
  4 * 4,			/* %esp */
  5 * 4,			/* %ebp */
  6 * 4,			/* %esi */
  7 * 4,			/* %edi */
  8 * 4,			/* %eip */
  9 * 4,			/* %eflags */
  10 * 4,			/* %cs */
  11 * 4,			/* %ss */
  12 * 4,			/* %ds */
  13 * 4,			/* %es */
  14 * 4,			/* %fs */
  15 * 4			/* %gs */
};



/* Sigtramp routine location for OpenBSD 3.1 and earlier releases.  */
CORE_ADDR i386obsd_sigtramp_start_addr = 0xbfbfdf20;
CORE_ADDR i386obsd_sigtramp_end_addr = 0xbfbfdff0;

/* From <machine/signal.h>.  */
int i386obsd_sc_reg_offset[I386_NUM_GREGS] =
{
  10 * 4,			/* %eax */
  9 * 4,			/* %ecx */
  8 * 4,			/* %edx */
  7 * 4,			/* %ebx */
  14 * 4,			/* %esp */
  6 * 4,			/* %ebp */
  5 * 4,			/* %esi */
  4 * 4,			/* %edi */
  11 * 4,			/* %eip */
  13 * 4,			/* %eflags */
  12 * 4,			/* %cs */
  15 * 4,			/* %ss */
  3 * 4,			/* %ds */
  2 * 4,			/* %es */
  1 * 4,			/* %fs */
  0 * 4				/* %gs */
};

/* From /usr/src/lib/libpthread/arch/i386/uthread_machdep.c.  */
static int i386obsd_uthread_reg_offset[] =
{
  11 * 4,			/* %eax */
  10 * 4,			/* %ecx */
  9 * 4,			/* %edx */
  8 * 4,			/* %ebx */
  -1,				/* %esp */
  6 * 4,			/* %ebp */
  5 * 4,			/* %esi */
  4 * 4,			/* %edi */
  12 * 4,			/* %eip */
  -1,				/* %eflags */
  13 * 4,			/* %cs */
  -1,				/* %ss */
  3 * 4,			/* %ds */
  2 * 4,			/* %es */
  1 * 4,			/* %fs */
  0 * 4				/* %gs */
};

/* Offset within the thread structure where we can find the saved
   stack pointer (%esp).  */
#define I386OBSD_UTHREAD_ESP_OFFSET	176

static void
i386obsd_supply_uthread (struct regcache *regcache,
			 int regnum, CORE_ADDR addr)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp_addr = addr + I386OBSD_UTHREAD_ESP_OFFSET;
  CORE_ADDR sp = 0;
  gdb_byte buf[4];
  int i;

  gdb_assert (regnum >= -1);

  if (regnum == -1 || regnum == I386_ESP_REGNUM)
    {
      int offset;

      /* Fetch stack pointer from thread structure.  */
      sp = read_memory_unsigned_integer (sp_addr, 4, byte_order);

      /* Adjust the stack pointer such that it looks as if we just
	 returned from _thread_machdep_switch.  */
      offset = i386obsd_uthread_reg_offset[I386_EIP_REGNUM] + 4;
      store_unsigned_integer (buf, 4, byte_order, sp + offset);
      regcache->raw_supply (I386_ESP_REGNUM, buf);
    }

  for (i = 0; i < ARRAY_SIZE (i386obsd_uthread_reg_offset); i++)
    {
      if (i386obsd_uthread_reg_offset[i] != -1
	  && (regnum == -1 || regnum == i))
	{
	  /* Fetch stack pointer from thread structure (if we didn't
	     do so already).  */
	  if (sp == 0)
	    sp = read_memory_unsigned_integer (sp_addr, 4, byte_order);

	  /* Read the saved register from the stack frame.  */
	  read_memory (sp + i386obsd_uthread_reg_offset[i], buf, 4);
	  regcache->raw_supply (i, buf);
	}
    }
}

static void
i386obsd_collect_uthread (const struct regcache *regcache,
			  int regnum, CORE_ADDR addr)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp_addr = addr + I386OBSD_UTHREAD_ESP_OFFSET;
  CORE_ADDR sp = 0;
  gdb_byte buf[4];
  int i;

  gdb_assert (regnum >= -1);

  if (regnum == -1 || regnum == I386_ESP_REGNUM)
    {
      int offset;

      /* Calculate the stack pointer (frame pointer) that will be
	 stored into the thread structure.  */
      offset = i386obsd_uthread_reg_offset[I386_EIP_REGNUM] + 4;
      regcache->raw_collect (I386_ESP_REGNUM, buf);
      sp = extract_unsigned_integer (buf, 4, byte_order) - offset;

      /* Store the stack pointer.  */
      write_memory_unsigned_integer (sp_addr, 4, byte_order, sp);

      /* The stack pointer was (potentially) modified.  Make sure we
	 build a proper stack frame.  */
      regnum = -1;
    }

  for (i = 0; i < ARRAY_SIZE (i386obsd_uthread_reg_offset); i++)
    {
      if (i386obsd_uthread_reg_offset[i] != -1
	  && (regnum == -1 || regnum == i))
	{
	  /* Fetch stack pointer from thread structure (if we didn't
	     calculate it already).  */
	  if (sp == 0)
	    sp = read_memory_unsigned_integer (sp_addr, 4, byte_order);

	  /* Write the register into the stack frame.  */
	  regcache->raw_collect (i, buf);
	  write_memory (sp + i386obsd_uthread_reg_offset[i], buf, 4);
	}
    }
}

/* Kernel debugging support.  */

/* From <machine/frame.h>.  Note that %esp and %ess are only saved in
   a trap frame when entering the kernel from user space.  */
static int i386obsd_tf_reg_offset[] =
{
  10 * 4,			/* %eax */
  9 * 4,			/* %ecx */
  8 * 4,			/* %edx */
  7 * 4,			/* %ebx */
  -1,				/* %esp */
  6 * 4,			/* %ebp */
  5 * 4,			/* %esi */
  4 * 4,			/* %edi */
  13 * 4,			/* %eip */
  15 * 4,			/* %eflags */
  14 * 4,			/* %cs */
  -1,				/* %ss */
  3 * 4,			/* %ds */
  2 * 4,			/* %es */
  0 * 4,			/* %fs */
  1 * 4				/* %gs */
};

static struct trad_frame_cache *
i386obsd_trapframe_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  struct trad_frame_cache *cache;
  CORE_ADDR func, sp, addr;
  ULONGEST cs;
  const char *name;
  int i;

  if (*this_cache)
    return (struct trad_frame_cache *) *this_cache;

  cache = trad_frame_cache_zalloc (this_frame);
  *this_cache = cache;

  func = get_frame_func (this_frame);
  sp = get_frame_register_unsigned (this_frame, I386_ESP_REGNUM);

  find_pc_partial_function (func, &name, NULL, NULL);
  if (name && startswith (name, "Xintr"))
    addr = sp + 8;		/* It's an interrupt frame.  */
  else
    addr = sp;

  for (i = 0; i < ARRAY_SIZE (i386obsd_tf_reg_offset); i++)
    if (i386obsd_tf_reg_offset[i] != -1)
      trad_frame_set_reg_addr (cache, i, addr + i386obsd_tf_reg_offset[i]);

  /* Read %cs from trap frame.  */
  addr += i386obsd_tf_reg_offset[I386_CS_REGNUM];
  cs = read_memory_unsigned_integer (addr, 4, byte_order);
  if ((cs & I386_SEL_RPL) == I386_SEL_UPL)
    {
      /* Trap from user space; terminate backtrace.  */
      trad_frame_set_id (cache, outer_frame_id);
    }
  else
    {
      /* Construct the frame ID using the function start.  */
      trad_frame_set_id (cache, frame_id_build (sp + 8, func));
    }

  return cache;
}

static void
i386obsd_trapframe_this_id (frame_info_ptr this_frame,
			    void **this_cache, struct frame_id *this_id)
{
  struct trad_frame_cache *cache =
    i386obsd_trapframe_cache (this_frame, this_cache);
  
  trad_frame_get_id (cache, this_id);
}

static struct value *
i386obsd_trapframe_prev_register (frame_info_ptr this_frame,
				  void **this_cache, int regnum)
{
  struct trad_frame_cache *cache =
    i386obsd_trapframe_cache (this_frame, this_cache);

  return trad_frame_get_register (cache, this_frame, regnum);
}

static int
i386obsd_trapframe_sniffer (const struct frame_unwind *self,
			    frame_info_ptr this_frame,
			    void **this_prologue_cache)
{
  ULONGEST cs;
  const char *name;

  /* Check Current Privilege Level and bail out if we're not executing
     in kernel space.  */
  cs = get_frame_register_unsigned (this_frame, I386_CS_REGNUM);
  if ((cs & I386_SEL_RPL) == I386_SEL_UPL)
    return 0;

  find_pc_partial_function (get_frame_pc (this_frame), &name, NULL, NULL);
  return (name && (strcmp (name, "calltrap") == 0
		   || strcmp (name, "syscall1") == 0
		   || startswith (name, "Xintr")
		   || startswith (name, "Xsoft")));
}

static const struct frame_unwind i386obsd_trapframe_unwind = {
  "i386 openbsd trap",
  /* FIXME: kettenis/20051219: This really is more like an interrupt
     frame, but SIGTRAMP_FRAME would print <signal handler called>,
     which really is not what we want here.  */
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  i386obsd_trapframe_this_id,
  i386obsd_trapframe_prev_register,
  NULL,
  i386obsd_trapframe_sniffer
};


static void 
i386obsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  /* Obviously OpenBSD is BSD-based.  */
  i386bsd_init_abi (info, gdbarch);
  obsd_init_abi (info, gdbarch);
  i386_elf_init_abi (info, gdbarch);

  /* OpenBSD has a different `struct reg'.  */
  tdep->gregset_reg_offset = i386obsd_r_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (i386obsd_r_reg_offset);
  tdep->sizeof_gregset = 16 * 4;

  /* OpenBSD uses -freg-struct-return by default.  */
  tdep->struct_return = reg_struct_return;

  /* OpenBSD uses a different memory layout.  */
  tdep->sigtramp_start = i386obsd_sigtramp_start_addr;
  tdep->sigtramp_end = i386obsd_sigtramp_end_addr;
  tdep->sigtramp_p = i386obsd_sigtramp_p;

  /* OpenBSD has a `struct sigcontext' that's different from the
     original 4.3 BSD.  */
  tdep->sc_reg_offset = i386obsd_sc_reg_offset;
  tdep->sc_num_regs = ARRAY_SIZE (i386obsd_sc_reg_offset);

  /* OpenBSD provides a user-level threads implementation.  */
  bsd_uthread_set_supply_uthread (gdbarch, i386obsd_supply_uthread);
  bsd_uthread_set_collect_uthread (gdbarch, i386obsd_collect_uthread);

  /* Unwind kernel trap frames correctly.  */
  frame_unwind_prepend_unwinder (gdbarch, &i386obsd_trapframe_unwind);

  /* OpenBSD ELF uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);
}

void _initialize_i386obsd_tdep ();
void
_initialize_i386obsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_OPENBSD,
			  i386obsd_init_abi);
}
