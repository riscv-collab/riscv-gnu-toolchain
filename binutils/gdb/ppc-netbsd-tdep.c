/* Target-dependent code for NetBSD/powerpc.

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
#include "gdbtypes.h"
#include "osabi.h"
#include "regcache.h"
#include "regset.h"
#include "trad-frame.h"
#include "tramp-frame.h"

#include "ppc-tdep.h"
#include "netbsd-tdep.h"
#include "ppc-tdep.h"
#include "solib-svr4.h"

/* Register offsets from <machine/reg.h>.  */
static ppc_reg_offsets ppcnbsd_reg_offsets;


/* Core file support.  */

/* NetBSD/powerpc register sets.  */

const struct regset ppcnbsd_gregset =
{
  &ppcnbsd_reg_offsets,
  ppc_supply_gregset
};

const struct regset ppcnbsd_fpregset =
{
  &ppcnbsd_reg_offsets,
  ppc_supply_fpregset
};

/* Iterate over core file register note sections.  */

static void
ppcnbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
				      iterate_over_regset_sections_cb *cb,
				      void *cb_data,
				      const struct regcache *regcache)
{
  cb (".reg", 148, 148, &ppcnbsd_gregset, NULL, cb_data);
  cb (".reg2", 264, 264, &ppcnbsd_fpregset, NULL, cb_data);
}


/* NetBSD is confused.  It appears that 1.5 was using the correct SVR4
   convention but, 1.6 switched to the below broken convention.  For
   the moment use the broken convention.  Ulgh!  */

static enum return_value_convention
ppcnbsd_return_value (struct gdbarch *gdbarch, struct value *function,
		      struct type *valtype, struct regcache *regcache,
		      gdb_byte *readbuf, const gdb_byte *writebuf)
{
#if 0
  if ((valtype->code () == TYPE_CODE_STRUCT
       || valtype->code () == TYPE_CODE_UNION)
      && !((valtype->length () == 16 || valtype->length () == 8)
	    && valtype->is_vector ())
      && !(valtype->length () == 1
	   || valtype->length () == 2
	   || valtype->length () == 4
	   || valtype->length () == 8))
    return RETURN_VALUE_STRUCT_CONVENTION;
  else
#endif
    return ppc_sysv_abi_broken_return_value (gdbarch, function, valtype,
					     regcache, readbuf, writebuf);
}


/* Signal trampolines.  */

extern const struct tramp_frame ppcnbsd2_sigtramp;

static void
ppcnbsd_sigtramp_cache_init (const struct tramp_frame *self,
			     frame_info_ptr this_frame,
			     struct trad_frame_cache *this_cache,
			     CORE_ADDR func)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  ppc_gdbarch_tdep *tdep = gdbarch_tdep<ppc_gdbarch_tdep> (gdbarch);
  CORE_ADDR addr, base;
  int i;

  base = get_frame_register_unsigned (this_frame,
				      gdbarch_sp_regnum (gdbarch));
  if (self == &ppcnbsd2_sigtramp)
    addr = base + 0x10 + 2 * tdep->wordsize;
  else
    addr = base + 0x18 + 2 * tdep->wordsize;
  for (i = 0; i < ppc_num_gprs; i++, addr += tdep->wordsize)
    {
      int regnum = i + tdep->ppc_gp0_regnum;
      trad_frame_set_reg_addr (this_cache, regnum, addr);
    }
  trad_frame_set_reg_addr (this_cache, tdep->ppc_lr_regnum, addr);
  addr += tdep->wordsize;
  trad_frame_set_reg_addr (this_cache, tdep->ppc_cr_regnum, addr);
  addr += tdep->wordsize;
  trad_frame_set_reg_addr (this_cache, tdep->ppc_xer_regnum, addr);
  addr += tdep->wordsize;
  trad_frame_set_reg_addr (this_cache, tdep->ppc_ctr_regnum, addr);
  addr += tdep->wordsize;
  trad_frame_set_reg_addr (this_cache, gdbarch_pc_regnum (gdbarch),
			   addr); /* SRR0?  */
  addr += tdep->wordsize;

  /* Construct the frame ID using the function start.  */
  trad_frame_set_id (this_cache, frame_id_build (base, func));
}

static const struct tramp_frame ppcnbsd_sigtramp =
{
  SIGTRAMP_FRAME,
  4,
  {
    { 0x3821fff0, ULONGEST_MAX },		/* add r1,r1,-16 */
    { 0x4e800021, ULONGEST_MAX },		/* blrl */
    { 0x38610018, ULONGEST_MAX },		/* addi r3,r1,24 */
    { 0x38000127, ULONGEST_MAX },		/* li r0,295 */
    { 0x44000002, ULONGEST_MAX },		/* sc */
    { 0x38000001, ULONGEST_MAX },		/* li r0,1 */
    { 0x44000002, ULONGEST_MAX },		/* sc */
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  ppcnbsd_sigtramp_cache_init
};

/* NetBSD 2.0 introduced a slightly different signal trampoline.  */

const struct tramp_frame ppcnbsd2_sigtramp =
{
  SIGTRAMP_FRAME,
  4,
  {
    { 0x3821fff0, ULONGEST_MAX },		/* add r1,r1,-16 */
    { 0x4e800021, ULONGEST_MAX },		/* blrl */
    { 0x38610010, ULONGEST_MAX },		/* addi r3,r1,16 */
    { 0x38000127, ULONGEST_MAX },		/* li r0,295 */
    { 0x44000002, ULONGEST_MAX },		/* sc */
    { 0x38000001, ULONGEST_MAX },		/* li r0,1 */
    { 0x44000002, ULONGEST_MAX },		/* sc */
    { TRAMP_SENTINEL_INSN, ULONGEST_MAX }
  },
  ppcnbsd_sigtramp_cache_init
};


static void
ppcnbsd_init_abi (struct gdbarch_info info,
		  struct gdbarch *gdbarch)
{
  nbsd_init_abi (info, gdbarch);

  /* For NetBSD, this is an on again, off again thing.  Some systems
     do use the broken struct convention, and some don't.  */
  set_gdbarch_return_value (gdbarch, ppcnbsd_return_value);

  /* NetBSD uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, ppcnbsd_iterate_over_regset_sections);

  tramp_frame_prepend_unwinder (gdbarch, &ppcnbsd_sigtramp);
  tramp_frame_prepend_unwinder (gdbarch, &ppcnbsd2_sigtramp);
}

void _initialize_ppcnbsd_tdep ();
void
_initialize_ppcnbsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_powerpc, 0, GDB_OSABI_NETBSD,
			  ppcnbsd_init_abi);

  /* Avoid initializing the register offsets again if they were
     already initialized by ppc-netbsd-nat.c.  */
  if (ppcnbsd_reg_offsets.pc_offset == 0)
    {
      /* General-purpose registers.  */
      ppcnbsd_reg_offsets.r0_offset = 0;
      ppcnbsd_reg_offsets.gpr_size = 4;
      ppcnbsd_reg_offsets.xr_size = 4;
      ppcnbsd_reg_offsets.lr_offset = 128;
      ppcnbsd_reg_offsets.cr_offset = 132;
      ppcnbsd_reg_offsets.xer_offset = 136;
      ppcnbsd_reg_offsets.ctr_offset = 140;
      ppcnbsd_reg_offsets.pc_offset = 144;
      ppcnbsd_reg_offsets.ps_offset = -1;
      ppcnbsd_reg_offsets.mq_offset = -1;

      /* Floating-point registers.  */
      ppcnbsd_reg_offsets.f0_offset = 0;
      ppcnbsd_reg_offsets.fpscr_offset = 256;
      ppcnbsd_reg_offsets.fpscr_size = 4;

    }
}
