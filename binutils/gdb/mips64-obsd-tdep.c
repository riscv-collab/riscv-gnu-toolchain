/* Target-dependent code for OpenBSD/mips64.

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
#include "gdbtypes.h"
#include "osabi.h"
#include "regcache.h"
#include "regset.h"
#include "trad-frame.h"
#include "tramp-frame.h"

#include "obsd-tdep.h"
#include "mips-tdep.h"
#include "solib-svr4.h"

#define MIPS64OBSD_NUM_REGS 73

/* Core file support.  */

/* Supply register REGNUM from the buffer specified by GREGS and LEN
   in the general-purpose register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
mips64obsd_supply_gregset (const struct regset *regset,
			   struct regcache *regcache, int regnum,
			   const void *gregs, size_t len)
{
  const char *regs = (const char *) gregs;
  int i;

  for (i = 0; i < MIPS64OBSD_NUM_REGS; i++)
    {
      if (regnum == i || regnum == -1)
	regcache->raw_supply (i, regs + i * 8);
    }
}

/* OpenBSD/mips64 register set.  */

static const struct regset mips64obsd_gregset =
{
  NULL,
  mips64obsd_supply_gregset
};

/* Iterate over core file register note sections.  */

static void
mips64obsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
					 iterate_over_regset_sections_cb *cb,
					 void *cb_data,
					 const struct regcache *regcache)
{
  cb (".reg", MIPS64OBSD_NUM_REGS * 8, MIPS64OBSD_NUM_REGS * 8,
      &mips64obsd_gregset, NULL, cb_data);
}


/* Signal trampolines.  */

static void
mips64obsd_sigframe_init (const struct tramp_frame *self,
			  frame_info_ptr this_frame,
			  struct trad_frame_cache *cache,
			  CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  CORE_ADDR sp, sigcontext_addr, addr;
  int regnum;

  /* We find the appropriate instance of `struct sigcontext' at a
     fixed offset in the signal frame.  */
  sp = get_frame_register_signed (this_frame,
				  MIPS_SP_REGNUM + gdbarch_num_regs (gdbarch));
  sigcontext_addr = sp + 32;

  /* PC.  */
  regnum = mips_regnum (gdbarch)->pc;
  trad_frame_set_reg_addr (cache,
			   regnum + gdbarch_num_regs (gdbarch),
			    sigcontext_addr + 16);

  /* GPRs.  */
  for (regnum = MIPS_AT_REGNUM, addr = sigcontext_addr + 32;
       regnum <= MIPS_RA_REGNUM; regnum++, addr += 8)
    trad_frame_set_reg_addr (cache,
			     regnum + gdbarch_num_regs (gdbarch),
			     addr);

  /* HI and LO.  */
  regnum = mips_regnum (gdbarch)->lo;
  trad_frame_set_reg_addr (cache,
			   regnum + gdbarch_num_regs (gdbarch),
			   sigcontext_addr + 280);
  regnum = mips_regnum (gdbarch)->hi;
  trad_frame_set_reg_addr (cache,
			   regnum + gdbarch_num_regs (gdbarch),
			   sigcontext_addr + 288);

  /* TODO: Handle the floating-point registers.  */

  trad_frame_set_id (cache, frame_id_build (sp, func));
}

static const struct tramp_frame mips64obsd_sigframe =
{
  SIGTRAMP_FRAME,
  MIPS_INSN32_SIZE,
  {
    { 0x67a40020, ULONGEST_MAX },		/* daddiu  a0,sp,32 */
    { 0x24020067, ULONGEST_MAX },		/* li      v0,103 */
    { 0x0000000c, ULONGEST_MAX },		/* syscall */
    { 0x0000000d, ULONGEST_MAX },		/* break */
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  mips64obsd_sigframe_init
};


static void
mips64obsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  /* OpenBSD/mips64 only supports the n64 ABI, but the braindamaged
     way GDB works, forces us to pretend we can handle them all.  */

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, mips64obsd_iterate_over_regset_sections);

  tramp_frame_prepend_unwinder (gdbarch, &mips64obsd_sigframe);

  set_gdbarch_long_double_bit (gdbarch, 128);
  set_gdbarch_long_double_format (gdbarch, floatformats_ieee_quad);

  obsd_init_abi(info, gdbarch);

  /* OpenBSD/mips64 has SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_lp64_fetch_link_map_offsets);
}

void _initialize_mips64obsd_tdep ();
void
_initialize_mips64obsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_mips, 0, GDB_OSABI_OPENBSD,
			  mips64obsd_init_abi);
}
