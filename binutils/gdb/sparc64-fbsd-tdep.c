/* Target-dependent code for FreeBSD/sparc64.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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
#include "regset.h"
#include "target.h"
#include "trad-frame.h"

#include "sparc64-tdep.h"
#include "fbsd-tdep.h"
#include "solib-svr4.h"
#include "gdbarch.h"

/* From <machine/reg.h>.  */
const struct sparc_gregmap sparc64fbsd_gregmap =
{
  26 * 8,			/* "tstate" */
  25 * 8,			/* %pc */
  24 * 8,			/* %npc */
  28 * 8,			/* %y */
  16 * 8,			/* %fprs */
  -1,
  1 * 8,			/* %g1 */
  -1,				/* %l0 */
  8				/* sizeof (%y) */
};


static void
sparc64fbsd_supply_gregset (const struct regset *regset,
			    struct regcache *regcache,
			    int regnum, const void *gregs, size_t len)
{
  sparc64_supply_gregset (&sparc64fbsd_gregmap, regcache, regnum, gregs);
}

static void
sparc64fbsd_collect_gregset (const struct regset *regset,
			     const struct regcache *regcache,
			     int regnum, void *gregs, size_t len)
{
  sparc64_collect_gregset (&sparc64fbsd_gregmap, regcache, regnum, gregs);
}

static void
sparc64fbsd_supply_fpregset (const struct regset *regset,
			     struct regcache *regcache,
			     int regnum, const void *fpregs, size_t len)
{
  sparc64_supply_fpregset (&sparc64_bsd_fpregmap, regcache, regnum, fpregs);
}

static void
sparc64fbsd_collect_fpregset (const struct regset *regset,
			      const struct regcache *regcache,
			      int regnum, void *fpregs, size_t len)
{
  sparc64_collect_fpregset (&sparc64_bsd_fpregmap, regcache, regnum, fpregs);
}


/* Signal trampolines.  */

static int
sparc64fbsd_pc_in_sigtramp (CORE_ADDR pc, const char *name)
{
  return (name && strcmp (name, "__sigtramp") == 0);
}

static struct sparc_frame_cache *
sparc64fbsd_sigtramp_frame_cache (frame_info_ptr this_frame,
				   void **this_cache)
{
  struct sparc_frame_cache *cache;
  CORE_ADDR addr, mcontext_addr, sp;
  LONGEST fprs;
  int regnum;

  if (*this_cache)
    return (struct sparc_frame_cache *) *this_cache;

  cache = sparc_frame_cache (this_frame, this_cache);
  gdb_assert (cache == *this_cache);

  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* The third argument is a pointer to an instance of `ucontext_t',
     which has a member `uc_mcontext' that contains the saved
     registers.  */
  addr = get_frame_register_unsigned (this_frame, SPARC_O2_REGNUM);
  mcontext_addr = addr + 64;

  /* The following registers travel in the `mc_local' slots of
     `mcontext_t'.  */
  addr = mcontext_addr + 16 * 8;
  cache->saved_regs[SPARC64_FPRS_REGNUM].set_addr (addr + 0 * 8);
  cache->saved_regs[SPARC64_FSR_REGNUM].set_addr (addr + 1 * 8);

  /* The following registers travel in the `mc_in' slots of
     `mcontext_t'.  */
  addr = mcontext_addr + 24 * 8;
  cache->saved_regs[SPARC64_NPC_REGNUM].set_addr (addr + 0 * 8);
  cache->saved_regs[SPARC64_PC_REGNUM].set_addr (addr + 1 * 8);
  cache->saved_regs[SPARC64_STATE_REGNUM].set_addr (addr + 2 * 8);
  cache->saved_regs[SPARC64_Y_REGNUM].set_addr (addr + 4 * 8);

  /* The `global' and `out' registers travel in the `mc_global' and
     `mc_out' slots of `mcontext_t', except for %g0.  Since %g0 is
     always zero, keep the identity encoding.  */
  for (regnum = SPARC_G1_REGNUM, addr = mcontext_addr + 8;
       regnum <= SPARC_O7_REGNUM; regnum++, addr += 8)
    cache->saved_regs[regnum].set_addr (addr);

  /* The `local' and `in' registers have been saved in the register
     save area.  */
  addr = cache->saved_regs[SPARC_SP_REGNUM].addr ();
  sp = get_frame_memory_unsigned (this_frame, addr, 8);
  for (regnum = SPARC_L0_REGNUM, addr = sp + BIAS;
       regnum <= SPARC_I7_REGNUM; regnum++, addr += 8)
    cache->saved_regs[regnum].set_addr (addr);

  /* The floating-point registers are only saved if the FEF bit in
     %fprs has been set.  */

#define FPRS_FEF	(1 << 2)

  addr = cache->saved_regs[SPARC64_FPRS_REGNUM].addr ();
  fprs = get_frame_memory_unsigned (this_frame, addr, 8);
  if (fprs & FPRS_FEF)
    {
      for (regnum = SPARC_F0_REGNUM, addr = mcontext_addr + 32 * 8;
	   regnum <= SPARC_F31_REGNUM; regnum++, addr += 4)
	cache->saved_regs[regnum].set_addr (addr);

      for (regnum = SPARC64_F32_REGNUM;
	   regnum <= SPARC64_F62_REGNUM; regnum++, addr += 8)
	cache->saved_regs[regnum].set_addr (addr);
    }

  return cache;
}

static void
sparc64fbsd_sigtramp_frame_this_id (frame_info_ptr this_frame,
				    void **this_cache,
				    struct frame_id *this_id)
{
  struct sparc_frame_cache *cache =
    sparc64fbsd_sigtramp_frame_cache (this_frame, this_cache);

  (*this_id) = frame_id_build (cache->base, cache->pc);
}

static struct value *
sparc64fbsd_sigtramp_frame_prev_register (frame_info_ptr this_frame,
					  void **this_cache, int regnum)
{
  struct sparc_frame_cache *cache =
    sparc64fbsd_sigtramp_frame_cache (this_frame, this_cache);

  return trad_frame_get_prev_register (this_frame, cache->saved_regs, regnum);
}

static int
sparc64fbsd_sigtramp_frame_sniffer (const struct frame_unwind *self,
				    frame_info_ptr this_frame,
				    void **this_cache)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);
  if (sparc64fbsd_pc_in_sigtramp (pc, name))
    return 1;

  return 0;
}

static const struct frame_unwind sparc64fbsd_sigtramp_frame_unwind =
{
  "sparc64 freebsd sigtramp",
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  sparc64fbsd_sigtramp_frame_this_id,
  sparc64fbsd_sigtramp_frame_prev_register,
  NULL,
  sparc64fbsd_sigtramp_frame_sniffer
};


static const struct regset sparc64fbsd_gregset =
  {
    NULL, sparc64fbsd_supply_gregset, sparc64fbsd_collect_gregset
  };

static const struct regset sparc64fbsd_fpregset =
  {
    NULL, sparc64fbsd_supply_fpregset, sparc64fbsd_collect_fpregset
  };

static void
sparc64fbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (gdbarch);

  /* Generic FreeBSD support. */
  fbsd_init_abi (info, gdbarch);

  tdep->gregset = &sparc64fbsd_gregset;
  tdep->sizeof_gregset = 256;

  tdep->fpregset = &sparc64fbsd_fpregset;
  tdep->sizeof_fpregset = 272;

  frame_unwind_append_unwinder (gdbarch, &sparc64fbsd_sigtramp_frame_unwind);

  sparc64_init_abi (info, gdbarch);

  /* FreeBSD/sparc64 has SVR4-style shared libraries.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_lp64_fetch_link_map_offsets);
}

void _initialize_sparc64fbsd_tdep ();
void
_initialize_sparc64fbsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_sparc, bfd_mach_sparc_v9,
			  GDB_OSABI_FREEBSD, sparc64fbsd_init_abi);
}
