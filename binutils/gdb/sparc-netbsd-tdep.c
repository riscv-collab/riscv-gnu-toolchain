/* Target-dependent code for NetBSD/sparc.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.
   Contributed by Wasabi Systems, Inc.

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
#include "gdbtypes.h"
#include "osabi.h"
#include "regcache.h"
#include "regset.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "trad-frame.h"
#include "gdbarch.h"

#include "sparc-tdep.h"
#include "netbsd-tdep.h"

/* Macros to extract fields from SPARC instructions.  */
#define X_RS1(i) (((i) >> 14) & 0x1f)
#define X_RS2(i) ((i) & 0x1f)
#define X_I(i) (((i) >> 13) & 1)

const struct sparc_gregmap sparc32nbsd_gregmap =
{
  0 * 4,			/* %psr */
  1 * 4,			/* %pc */
  2 * 4,			/* %npc */
  3 * 4,			/* %y */
  -1,				/* %wim */
  -1,				/* %tbr */
  5 * 4,			/* %g1 */
  -1				/* %l0 */
};

static void
sparc32nbsd_supply_gregset (const struct regset *regset,
			    struct regcache *regcache,
			    int regnum, const void *gregs, size_t len)
{
  sparc32_supply_gregset (&sparc32nbsd_gregmap, regcache, regnum, gregs);

  /* Traditional NetBSD core files don't use multiple register sets.
     Instead, the general-purpose and floating-point registers are
     lumped together in a single section.  */
  if (len >= 212)
    sparc32_supply_fpregset (&sparc32_bsd_fpregmap, regcache, regnum,
			     (const char *) gregs + 80);
}

static void
sparc32nbsd_supply_fpregset (const struct regset *regset,
			     struct regcache *regcache,
			     int regnum, const void *fpregs, size_t len)
{
  sparc32_supply_fpregset (&sparc32_bsd_fpregmap, regcache, regnum, fpregs);
}


/* Signal trampolines.  */

/* The following variables describe the location of an on-stack signal
   trampoline.  The current values correspond to the memory layout for
   NetBSD 1.3 and up.  These shouldn't be necessary for NetBSD 2.0 and
   up, since NetBSD uses signal trampolines provided by libc now.  */

static const CORE_ADDR sparc32nbsd_sigtramp_start = 0xeffffef0;
static const CORE_ADDR sparc32nbsd_sigtramp_end = 0xeffffff0;

static int
sparc32nbsd_pc_in_sigtramp (CORE_ADDR pc, const char *name)
{
  if (pc >= sparc32nbsd_sigtramp_start && pc < sparc32nbsd_sigtramp_end)
    return 1;

  return nbsd_pc_in_sigtramp (pc, name);
}

trad_frame_saved_reg *
sparc32nbsd_sigcontext_saved_regs (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  trad_frame_saved_reg *saved_regs;
  CORE_ADDR addr, sigcontext_addr;
  int regnum, delta;
  ULONGEST psr;

  saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* We find the appropriate instance of `struct sigcontext' at a
     fixed offset in the signal frame.  */
  addr = get_frame_register_unsigned (this_frame, SPARC_FP_REGNUM);
  sigcontext_addr = addr + 64 + 16;

  /* The registers are saved in bits and pieces scattered all over the
     place.  The code below records their location on the assumption
     that the part of the signal trampoline that saves the state has
     been executed.  */

  saved_regs[SPARC_SP_REGNUM].set_addr (sigcontext_addr + 8);
  saved_regs[SPARC32_PC_REGNUM].set_addr (sigcontext_addr + 12);
  saved_regs[SPARC32_NPC_REGNUM].set_addr (sigcontext_addr + 16);
  saved_regs[SPARC32_PSR_REGNUM].set_addr (sigcontext_addr + 20);
  saved_regs[SPARC_G1_REGNUM].set_addr (sigcontext_addr + 24);
  saved_regs[SPARC_O0_REGNUM].set_addr (sigcontext_addr + 28);

  /* The remaining `global' registers and %y are saved in the `local'
     registers.  */
  delta = SPARC_L0_REGNUM - SPARC_G0_REGNUM;
  for (regnum = SPARC_G2_REGNUM; regnum <= SPARC_G7_REGNUM; regnum++)
    saved_regs[regnum].set_realreg (regnum + delta);
  saved_regs[SPARC32_Y_REGNUM].set_realreg (SPARC_L1_REGNUM);

  /* The remaining `out' registers can be found in the current frame's
     `in' registers.  */
  delta = SPARC_I0_REGNUM - SPARC_O0_REGNUM;
  for (regnum = SPARC_O1_REGNUM; regnum <= SPARC_O5_REGNUM; regnum++)
    saved_regs[regnum].set_realreg (regnum + delta);
  saved_regs[SPARC_O7_REGNUM].set_realreg (SPARC_I7_REGNUM);

  /* The `local' and `in' registers have been saved in the register
     save area.  */
  addr = saved_regs[SPARC_SP_REGNUM].addr ();
  addr = get_frame_memory_unsigned (this_frame, addr, 4);
  for (regnum = SPARC_L0_REGNUM;
       regnum <= SPARC_I7_REGNUM; regnum++, addr += 4)
    saved_regs[regnum].set_addr (addr);

  /* Handle StackGhost.  */
  {
    ULONGEST wcookie = sparc_fetch_wcookie (gdbarch);

    if (wcookie != 0)
      {
	ULONGEST i7;

	addr = saved_regs[SPARC_I7_REGNUM].addr ();
	i7 = get_frame_memory_unsigned (this_frame, addr, 4);
	saved_regs[SPARC_I7_REGNUM].set_value (i7 ^ wcookie);
      }
  }

  /* The floating-point registers are only saved if the EF bit in %prs
     has been set.  */

#define PSR_EF	0x00001000

  addr = saved_regs[SPARC32_PSR_REGNUM].addr ();
  psr = get_frame_memory_unsigned (this_frame, addr, 4);
  if (psr & PSR_EF)
    {
      CORE_ADDR sp;

      sp = get_frame_register_unsigned (this_frame, SPARC_SP_REGNUM);
      saved_regs[SPARC32_FSR_REGNUM].set_addr (sp + 96);
      for (regnum = SPARC_F0_REGNUM, addr = sp + 96 + 8;
	   regnum <= SPARC_F31_REGNUM; regnum++, addr += 4)
	saved_regs[regnum].set_addr (addr);
    }

  return saved_regs;
}

static struct sparc_frame_cache *
sparc32nbsd_sigcontext_frame_cache (frame_info_ptr this_frame,
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
      cache->pc = sparc32nbsd_sigtramp_start;

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
sparc32nbsd_sigcontext_frame_this_id (frame_info_ptr this_frame,
				      void **this_cache,
				      struct frame_id *this_id)
{
  struct sparc_frame_cache *cache =
    sparc32nbsd_sigcontext_frame_cache (this_frame, this_cache);

  (*this_id) = frame_id_build (cache->base, cache->pc);
}

static struct value *
sparc32nbsd_sigcontext_frame_prev_register (frame_info_ptr this_frame,
					    void **this_cache, int regnum)
{
  struct sparc_frame_cache *cache =
    sparc32nbsd_sigcontext_frame_cache (this_frame, this_cache);

  return trad_frame_get_prev_register (this_frame, cache->saved_regs, regnum);
}

static int
sparc32nbsd_sigcontext_frame_sniffer (const struct frame_unwind *self,
				      frame_info_ptr this_frame,
				      void **this_cache)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);
  if (sparc32nbsd_pc_in_sigtramp (pc, name))
    {
      if (name == NULL || !startswith (name, "__sigtramp_sigcontext"))
	return 1;
    }

  return 0;
}

static const struct frame_unwind sparc32nbsd_sigcontext_frame_unwind =
{
  "sparc32 netbsd sigcontext",
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  sparc32nbsd_sigcontext_frame_this_id,
  sparc32nbsd_sigcontext_frame_prev_register,
  NULL,
  sparc32nbsd_sigcontext_frame_sniffer
};

/* Return the address of a system call's alternative return
   address.  */

CORE_ADDR
sparcnbsd_step_trap (frame_info_ptr frame, unsigned long insn)
{
  if ((X_I (insn) == 0 && X_RS1 (insn) == 0 && X_RS2 (insn) == 0)
      || (X_I (insn) == 1 && X_RS1 (insn) == 0 && (insn & 0x7f) == 0))
    {
      /* "New" system call.  */
      ULONGEST number = get_frame_register_unsigned (frame, SPARC_G1_REGNUM);

      if (number & 0x400)
	return get_frame_register_unsigned (frame, SPARC_G2_REGNUM);
      if (number & 0x800)
	return get_frame_register_unsigned (frame, SPARC_G7_REGNUM);
    }

  return 0;
}


static const struct regset sparc32nbsd_gregset =
  {
    NULL, sparc32nbsd_supply_gregset, NULL
  };

static const struct regset sparc32nbsd_fpregset =
  {
    NULL, sparc32nbsd_supply_fpregset, NULL
  };

void
sparc32nbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  sparc_gdbarch_tdep *tdep = gdbarch_tdep<sparc_gdbarch_tdep> (gdbarch);

  nbsd_init_abi (info, gdbarch);

  /* NetBSD doesn't support the 128-bit `long double' from the psABI.  */
  set_gdbarch_long_double_bit (gdbarch, 64);
  set_gdbarch_long_double_format (gdbarch, floatformats_ieee_double);

  tdep->gregset = &sparc32nbsd_gregset;
  tdep->sizeof_gregset = 20 * 4;

  tdep->fpregset = &sparc32nbsd_fpregset;
  tdep->sizeof_fpregset = 33 * 4;

  /* Make sure we can single-step "new" syscalls.  */
  tdep->step_trap = sparcnbsd_step_trap;

  frame_unwind_append_unwinder (gdbarch, &sparc32nbsd_sigcontext_frame_unwind);

  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);
}

void _initialize_sparcnbsd_tdep ();
void
_initialize_sparcnbsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_sparc, 0, GDB_OSABI_NETBSD,
			  sparc32nbsd_init_abi);
}
