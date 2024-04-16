/* Target-dependent code for Solaris SPARC.

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
#include "symtab.h"
#include "objfiles.h"
#include "osabi.h"
#include "regcache.h"
#include "regset.h"
#include "target.h"
#include "trad-frame.h"

#include "sol2-tdep.h"
#include "sparc-tdep.h"
#include "solib-svr4.h"

/* From <sys/regset.h>.  */
const struct sparc_gregmap sparc32_sol2_gregmap =
{
  32 * 4,			/* %psr */
  33 * 4,			/* %pc */
  34 * 4,			/* %npc */
  35 * 4,			/* %y */
  36 * 4,			/* %wim */
  37 * 4,			/* %tbr */
  1 * 4,			/* %g1 */
  16 * 4,			/* %l0 */
};

const struct sparc_fpregmap sparc32_sol2_fpregmap =
{
  0 * 4,			/* %f0 */
  33 * 4,			/* %fsr */
};

static void
sparc32_sol2_supply_core_gregset (const struct regset *regset,
				  struct regcache *regcache,
				  int regnum, const void *gregs, size_t len)
{
  sparc32_supply_gregset (&sparc32_sol2_gregmap, regcache, regnum, gregs);
}

static void
sparc32_sol2_collect_core_gregset (const struct regset *regset,
				   const struct regcache *regcache,
				   int regnum, void *gregs, size_t len)
{
  sparc32_collect_gregset (&sparc32_sol2_gregmap, regcache, regnum, gregs);
}

static void
sparc32_sol2_supply_core_fpregset (const struct regset *regset,
				   struct regcache *regcache,
				   int regnum, const void *fpregs, size_t len)
{
  sparc32_supply_fpregset (&sparc32_sol2_fpregmap, regcache, regnum, fpregs);
}

static void
sparc32_sol2_collect_core_fpregset (const struct regset *regset,
				    const struct regcache *regcache,
				    int regnum, void *fpregs, size_t len)
{
  sparc32_collect_fpregset (&sparc32_sol2_fpregmap, regcache, regnum, fpregs);
}

static const struct regset sparc32_sol2_gregset =
  {
    NULL,
    sparc32_sol2_supply_core_gregset,
    sparc32_sol2_collect_core_gregset
  };

static const struct regset sparc32_sol2_fpregset =
  {
    NULL,
    sparc32_sol2_supply_core_fpregset,
    sparc32_sol2_collect_core_fpregset
  };


static struct sparc_frame_cache *
sparc32_sol2_sigtramp_frame_cache (frame_info_ptr this_frame,
				   void **this_cache)
{
  struct sparc_frame_cache *cache;
  CORE_ADDR mcontext_addr, addr;
  int regnum;

  if (*this_cache)
    return (struct sparc_frame_cache *) *this_cache;

  cache = sparc_frame_cache (this_frame, this_cache);
  gdb_assert (cache == *this_cache);

  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* The third argument is a pointer to an instance of `ucontext_t',
     which has a member `uc_mcontext' that contains the saved
     registers.  */
  regnum =
    (cache->copied_regs_mask & 0x04) ? SPARC_I2_REGNUM : SPARC_O2_REGNUM;
  mcontext_addr = get_frame_register_unsigned (this_frame, regnum) + 40;

  cache->saved_regs[SPARC32_PSR_REGNUM].set_addr (mcontext_addr + 0 * 4);
  cache->saved_regs[SPARC32_PC_REGNUM].set_addr (mcontext_addr + 1 * 4);
  cache->saved_regs[SPARC32_NPC_REGNUM].set_addr (mcontext_addr + 2 * 4);
  cache->saved_regs[SPARC32_Y_REGNUM].set_addr (mcontext_addr + 3 * 4);

  /* Since %g0 is always zero, keep the identity encoding.  */
  for (regnum = SPARC_G1_REGNUM, addr = mcontext_addr + 4 * 4;
       regnum <= SPARC_O7_REGNUM; regnum++, addr += 4)
    cache->saved_regs[regnum].set_addr (addr);

  if (get_frame_memory_unsigned (this_frame, mcontext_addr + 19 * 4, 4))
    {
      /* The register windows haven't been flushed.  */
      for (regnum = SPARC_L0_REGNUM; regnum <= SPARC_I7_REGNUM; regnum++)
	cache->saved_regs[regnum].set_unknown ();
    }
  else
    {
      addr = cache->saved_regs[SPARC_SP_REGNUM].addr ();
      addr = get_frame_memory_unsigned (this_frame, addr, 4);
      for (regnum = SPARC_L0_REGNUM;
	   regnum <= SPARC_I7_REGNUM; regnum++, addr += 4)
	cache->saved_regs[regnum].set_addr (addr);
    }

  return cache;
}

static void
sparc32_sol2_sigtramp_frame_this_id (frame_info_ptr this_frame,
				     void **this_cache,
				     struct frame_id *this_id)
{
  struct sparc_frame_cache *cache =
    sparc32_sol2_sigtramp_frame_cache (this_frame, this_cache);

  (*this_id) = frame_id_build (cache->base, cache->pc);
}

static struct value *
sparc32_sol2_sigtramp_frame_prev_register (frame_info_ptr this_frame,
					   void **this_cache,
					   int regnum)
{
  struct sparc_frame_cache *cache =
    sparc32_sol2_sigtramp_frame_cache (this_frame, this_cache);

  return trad_frame_get_prev_register (this_frame, cache->saved_regs, regnum);
}

static int
sparc32_sol2_sigtramp_frame_sniffer (const struct frame_unwind *self,
				     frame_info_ptr this_frame,
				     void **this_cache)
{
  return sol2_sigtramp_p (this_frame);
}

static const struct frame_unwind sparc32_sol2_sigtramp_frame_unwind =
{
  "sparc32 solaris sigtramp",
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  sparc32_sol2_sigtramp_frame_this_id,
  sparc32_sol2_sigtramp_frame_prev_register,
  NULL,
  sparc32_sol2_sigtramp_frame_sniffer
};



static void
sparc32_sol2_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (gdbarch);

  tdep->gregset = &sparc32_sol2_gregset;
  tdep->sizeof_gregset = 152;

  tdep->fpregset = &sparc32_sol2_fpregset;
  tdep->sizeof_fpregset = 400;

  sol2_init_abi (info, gdbarch);

  /* Solaris has SVR4-style shared libraries...  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  /* ...which means that we need some special handling when doing
     prologue analysis.  */
  tdep->plt_entry_size = 12;

  /* Solaris has kernel-assisted single-stepping support.  */
  set_gdbarch_software_single_step (gdbarch, NULL);

  frame_unwind_append_unwinder (gdbarch, &sparc32_sol2_sigtramp_frame_unwind);
}

void _initialize_sparc_sol2_tdep ();
void
_initialize_sparc_sol2_tdep ()
{
  gdbarch_register_osabi (bfd_arch_sparc, 0,
			  GDB_OSABI_SOLARIS, sparc32_sol2_init_abi);
}
