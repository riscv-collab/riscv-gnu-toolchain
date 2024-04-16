/* Target-dependent code for OpenBSD/hppa

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
#include "osabi.h"
#include "regcache.h"
#include "regset.h"

#include "hppa-tdep.h"
#include "hppa-bsd-tdep.h"
#include "gdbarch.h"

/* Core file support.  */

/* Sizeof `struct reg' in <machine/reg.h>.  */
#define HPPAOBSD_SIZEOF_GREGS	(34 * 4) /* OpenBSD 5.1 and earlier.  */
#define HPPANBSD_SIZEOF_GREGS	(46 * 4) /* NetBSD and OpenBSD 5.2 and later.  */

/* Sizeof `struct fpreg' in <machine/reg.h>.  */
#define HPPAOBSD_SIZEOF_FPREGS	(32 * 8)

/* Supply register REGNUM from the buffer specified by GREGS and LEN
   in the general-purpose register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
hppaobsd_supply_gregset (const struct regset *regset,
			 struct regcache *regcache,
			 int regnum, const void *gregs, size_t len)
{
  gdb_byte zero[4] = { 0 };
  const gdb_byte *regs = (const gdb_byte *) gregs;
  size_t offset;
  int i;

  gdb_assert (len >= HPPAOBSD_SIZEOF_GREGS);

  if (regnum == -1 || regnum == HPPA_R0_REGNUM)
    regcache->raw_supply (HPPA_R0_REGNUM, &zero);
  for (i = HPPA_R1_REGNUM, offset = 4; i <= HPPA_R31_REGNUM; i++, offset += 4)
    {
      if (regnum == -1 || regnum == i)
	regcache->raw_supply (i, regs + offset);
    }

  if (len >= HPPANBSD_SIZEOF_GREGS)
    {
      if (regnum == -1 || regnum == HPPA_IPSW_REGNUM)
	regcache->raw_supply (HPPA_IPSW_REGNUM, regs);
      if (regnum == -1 || regnum == HPPA_SAR_REGNUM)
	regcache->raw_supply (HPPA_SAR_REGNUM, regs + 32 * 4);
      if (regnum == -1 || regnum == HPPA_PCSQ_HEAD_REGNUM)
	regcache->raw_supply (HPPA_PCSQ_HEAD_REGNUM, regs + 33 * 4);
      if (regnum == -1 || regnum == HPPA_PCSQ_TAIL_REGNUM)
	regcache->raw_supply (HPPA_PCSQ_TAIL_REGNUM, regs + 34 * 4);
      if (regnum == -1 || regnum == HPPA_PCOQ_HEAD_REGNUM)
	regcache->raw_supply (HPPA_PCOQ_HEAD_REGNUM, regs + 35 * 4);
      if (regnum == -1 || regnum == HPPA_PCOQ_TAIL_REGNUM)
	regcache->raw_supply (HPPA_PCOQ_TAIL_REGNUM, regs + 36 * 4);
      if (regnum == -1 || regnum == HPPA_SR0_REGNUM)
	regcache->raw_supply (HPPA_SR0_REGNUM, regs + 37 * 4);
      if (regnum == -1 || regnum == HPPA_SR1_REGNUM)
	regcache->raw_supply (HPPA_SR1_REGNUM, regs + 38 * 4);
      if (regnum == -1 || regnum == HPPA_SR2_REGNUM)
	regcache->raw_supply (HPPA_SR2_REGNUM, regs + 39 * 4);
      if (regnum == -1 || regnum == HPPA_SR3_REGNUM)
	regcache->raw_supply (HPPA_SR3_REGNUM, regs + 40 * 4);
      if (regnum == -1 || regnum == HPPA_SR4_REGNUM)
	regcache->raw_supply (HPPA_SR4_REGNUM, regs + 41 * 4);
      if (regnum == -1 || regnum == HPPA_SR5_REGNUM)
	regcache->raw_supply (HPPA_SR5_REGNUM, regs + 42 * 4);
      if (regnum == -1 || regnum == HPPA_SR6_REGNUM)
	regcache->raw_supply (HPPA_SR6_REGNUM, regs + 43 * 4);
      if (regnum == -1 || regnum == HPPA_SR7_REGNUM)
	regcache->raw_supply (HPPA_SR7_REGNUM, regs + 44 * 4);
      if (regnum == -1 || regnum == HPPA_CR26_REGNUM)
	regcache->raw_supply (HPPA_CR26_REGNUM, regs + 45 * 4);
      if (regnum == -1 || regnum == HPPA_CR27_REGNUM)
	regcache->raw_supply (HPPA_CR27_REGNUM, regs + 46 * 4);
    }
  else
    {
      if (regnum == -1 || regnum == HPPA_SAR_REGNUM)
	regcache->raw_supply (HPPA_SAR_REGNUM, regs);
      if (regnum == -1 || regnum == HPPA_PCOQ_HEAD_REGNUM)
	regcache->raw_supply (HPPA_PCOQ_HEAD_REGNUM, regs + 32 * 4);
      if (regnum == -1 || regnum == HPPA_PCOQ_TAIL_REGNUM)
	regcache->raw_supply (HPPA_PCOQ_TAIL_REGNUM, regs + 33 * 4);
    }
}

/* Supply register REGNUM from the buffer specified by FPREGS and LEN
   in the floating-point register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
hppaobsd_supply_fpregset (const struct regset *regset,
			  struct regcache *regcache,
			  int regnum, const void *fpregs, size_t len)
{
  const gdb_byte *regs = (const gdb_byte *) fpregs;
  int i;

  gdb_assert (len >= HPPAOBSD_SIZEOF_FPREGS);

  for (i = HPPA_FP0_REGNUM; i <= HPPA_FP31R_REGNUM; i++)
    {
      if (regnum == i || regnum == -1)
	regcache->raw_supply (i, regs + (i - HPPA_FP0_REGNUM) * 4);
    }
}

/* OpenBSD/hppa register sets.  */

static const struct regset hppaobsd_gregset =
{
  NULL,
  hppaobsd_supply_gregset,
  NULL,
  REGSET_VARIABLE_SIZE
};

static const struct regset hppaobsd_fpregset =
{
  NULL,
  hppaobsd_supply_fpregset
};

/* Iterate over supported core file register note sections. */

static void
hppaobsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
				       iterate_over_regset_sections_cb *cb,
				       void *cb_data,
				       const struct regcache *regcache)
{
  cb (".reg", HPPAOBSD_SIZEOF_GREGS, HPPAOBSD_SIZEOF_GREGS, &hppaobsd_gregset,
      NULL, cb_data);
  cb (".reg2", HPPAOBSD_SIZEOF_FPREGS, HPPAOBSD_SIZEOF_FPREGS,
      &hppaobsd_fpregset, NULL, cb_data);
}


static void
hppaobsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  /* Obviously OpenBSD is BSD-based.  */
  hppabsd_init_abi (info, gdbarch);

  /* Core file support.  */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, hppaobsd_iterate_over_regset_sections);
}

void _initialize_hppabsd_tdep ();
void
_initialize_hppabsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_hppa, 0, GDB_OSABI_OPENBSD,
			  hppaobsd_init_abi);
}
