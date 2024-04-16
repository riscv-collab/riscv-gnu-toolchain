/* Target-dependent code for OpenBSD/sparc.

   Copyright (C) 2004-2024 Free Software Foundation, Inc.

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
#include "frame-unwind.h"
#include "gdbcore.h"
#include "osabi.h"
#include "regcache.h"
#include "symtab.h"
#include "trad-frame.h"
#include "inferior.h"

#include "obsd-tdep.h"
#include "sparc-tdep.h"
#include "solib-svr4.h"
#include "bsd-uthread.h"
#include "gdbarch.h"

/* Signal trampolines.  */

/* The OpenBSD kernel maps the signal trampoline at some random
   location in user space, which means that the traditional BSD way of
   detecting it won't work.

   The signal trampoline will be mapped at an address that is page
   aligned.  We recognize the signal trampoline by looking for the
   sigreturn system call.  */

static const int sparc32obsd_page_size = 4096;

static int
sparc32obsd_pc_in_sigtramp (CORE_ADDR pc, const char *name)
{
  CORE_ADDR start_pc = (pc & ~(sparc32obsd_page_size - 1));
  unsigned long insn;

  if (name)
    return 0;

  /* Check for "restore %g0, SYS_sigreturn, %g1".  */
  insn = sparc_fetch_instruction (start_pc + 0xec);
  if (insn != 0x83e82067)
    return 0;

  /* Check for "t ST_SYSCALL".  */
  insn = sparc_fetch_instruction (start_pc + 0xf4);
  if (insn != 0x91d02000)
    return 0;

  return 1;
}

static struct sparc_frame_cache *
sparc32obsd_sigtramp_frame_cache (frame_info_ptr this_frame,
				  void **this_cache)
{
  struct sparc_frame_cache *cache;
  CORE_ADDR addr;

  if (*this_cache)
    return (struct sparc_frame_cache *) *this_cache;

  cache = sparc_frame_cache (this_frame, this_cache);
  gdb_assert (cache == *this_cache);

  /* If we couldn't find the frame's function, we're probably dealing
     with an on-stack signal trampoline.  */
  if (cache->pc == 0)
    {
      cache->pc = get_frame_pc (this_frame);
      cache->pc &= ~(sparc32obsd_page_size - 1);

      /* Since we couldn't find the frame's function, the cache was
	 initialized under the assumption that we're frameless.  */
      sparc_record_save_insn (cache);
      addr = get_frame_register_unsigned (this_frame, SPARC_FP_REGNUM);
      cache->base = addr;
    }

  cache->saved_regs = sparc32nbsd_sigcontext_saved_regs (this_frame);

  return cache;
}

static void
sparc32obsd_sigtramp_frame_this_id (frame_info_ptr this_frame,
				    void **this_cache,
				    struct frame_id *this_id)
{
  struct sparc_frame_cache *cache =
    sparc32obsd_sigtramp_frame_cache (this_frame, this_cache);

  (*this_id) = frame_id_build (cache->base, cache->pc);
}

static struct value *
sparc32obsd_sigtramp_frame_prev_register (frame_info_ptr this_frame,
					  void **this_cache, int regnum)
{
  struct sparc_frame_cache *cache =
    sparc32obsd_sigtramp_frame_cache (this_frame, this_cache);

  return trad_frame_get_prev_register (this_frame, cache->saved_regs, regnum);
}

static int
sparc32obsd_sigtramp_frame_sniffer (const struct frame_unwind *self,
				    frame_info_ptr this_frame,
				    void **this_cache)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);
  if (sparc32obsd_pc_in_sigtramp (pc, name))
    return 1;

  return 0;
}
static const struct frame_unwind sparc32obsd_sigtramp_frame_unwind =
{
  "sparc32 openbsd sigtramp",
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  sparc32obsd_sigtramp_frame_this_id,
  sparc32obsd_sigtramp_frame_prev_register,
  NULL,
  sparc32obsd_sigtramp_frame_sniffer
};



/* Offset wthin the thread structure where we can find %fp and %i7.  */
#define SPARC32OBSD_UTHREAD_FP_OFFSET	128
#define SPARC32OBSD_UTHREAD_PC_OFFSET	132

static void
sparc32obsd_supply_uthread (struct regcache *regcache,
			    int regnum, CORE_ADDR addr)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR fp, fp_addr = addr + SPARC32OBSD_UTHREAD_FP_OFFSET;
  gdb_byte buf[4];

  /* This function calls functions that depend on the global current thread.  */
  gdb_assert (regcache->ptid () == inferior_ptid);

  gdb_assert (regnum >= -1);

  fp = read_memory_unsigned_integer (fp_addr, 4, byte_order);
  if (regnum == SPARC_SP_REGNUM || regnum == -1)
    {
      store_unsigned_integer (buf, 4, byte_order, fp);
      regcache->raw_supply (SPARC_SP_REGNUM, buf);

      if (regnum == SPARC_SP_REGNUM)
	return;
    }

  if (regnum == SPARC32_PC_REGNUM || regnum == SPARC32_NPC_REGNUM
      || regnum == -1)
    {
      CORE_ADDR i7, i7_addr = addr + SPARC32OBSD_UTHREAD_PC_OFFSET;

      i7 = read_memory_unsigned_integer (i7_addr, 4, byte_order);
      if (regnum == SPARC32_PC_REGNUM || regnum == -1)
	{
	  store_unsigned_integer (buf, 4, byte_order, i7 + 8);
	  regcache->raw_supply (SPARC32_PC_REGNUM, buf);
	}
      if (regnum == SPARC32_NPC_REGNUM || regnum == -1)
	{
	  store_unsigned_integer (buf, 4, byte_order, i7 + 12);
	  regcache->raw_supply (SPARC32_NPC_REGNUM, buf);
	}

      if (regnum == SPARC32_PC_REGNUM || regnum == SPARC32_NPC_REGNUM)
	return;
    }

  sparc_supply_rwindow (regcache, fp, regnum);
}

static void
sparc32obsd_collect_uthread(const struct regcache *regcache,
			    int regnum, CORE_ADDR addr)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR sp;
  gdb_byte buf[4];

  /* This function calls functions that depend on the global current thread.  */
  gdb_assert (regcache->ptid () == inferior_ptid);

  gdb_assert (regnum >= -1);

  if (regnum == SPARC_SP_REGNUM || regnum == -1)
    {
      CORE_ADDR fp_addr = addr + SPARC32OBSD_UTHREAD_FP_OFFSET;

      regcache->raw_collect (SPARC_SP_REGNUM, buf);
      write_memory (fp_addr,buf, 4);
    }

  if (regnum == SPARC32_PC_REGNUM || regnum == -1)
    {
      CORE_ADDR i7, i7_addr = addr + SPARC32OBSD_UTHREAD_PC_OFFSET;

      regcache->raw_collect (SPARC32_PC_REGNUM, buf);
      i7 = extract_unsigned_integer (buf, 4, byte_order) - 8;
      write_memory_unsigned_integer (i7_addr, 4, byte_order, i7);

      if (regnum == SPARC32_PC_REGNUM)
	return;
    }

  regcache->raw_collect (SPARC_SP_REGNUM, buf);
  sp = extract_unsigned_integer (buf, 4, byte_order);
  sparc_collect_rwindow (regcache, sp, regnum);
}


static void
sparc32obsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  /* OpenBSD/sparc is very similar to NetBSD/sparc ELF.  */
  sparc32nbsd_init_abi (info, gdbarch);

  set_gdbarch_skip_solib_resolver (gdbarch, obsd_skip_solib_resolver);

  frame_unwind_append_unwinder (gdbarch, &sparc32obsd_sigtramp_frame_unwind);

  /* OpenBSD provides a user-level threads implementation.  */
  bsd_uthread_set_supply_uthread (gdbarch, sparc32obsd_supply_uthread);
  bsd_uthread_set_collect_uthread (gdbarch, sparc32obsd_collect_uthread);
}

void _initialize_sparc32obsd_tdep ();
void
_initialize_sparc32obsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_sparc, 0, GDB_OSABI_OPENBSD,
			  sparc32obsd_init_abi);
}
