/* Target-dependent code for Motorola 68000 BSD's.

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
#include "arch-utils.h"
#include "frame.h"
#include "osabi.h"
#include "regcache.h"
#include "regset.h"
#include "trad-frame.h"
#include "tramp-frame.h"
#include "gdbtypes.h"

#include "m68k-tdep.h"
#include "solib-svr4.h"

/* Core file support.  */

/* Sizeof `struct reg' in <machine/reg.h>.  */
#define M68KBSD_SIZEOF_GREGS	(18 * 4)

/* Sizeof `struct fpreg' in <machine/reg.h.  */
#define M68KBSD_SIZEOF_FPREGS	(((8 * 3) + 3) * 4)

int
m68kbsd_fpreg_offset (struct gdbarch *gdbarch, int regnum)
{
  int fp_len = gdbarch_register_type (gdbarch, regnum)->length ();
  
  if (regnum >= M68K_FPC_REGNUM)
    return 8 * fp_len + (regnum - M68K_FPC_REGNUM) * 4;

  return (regnum - M68K_FP0_REGNUM) * fp_len;
}

/* Supply register REGNUM from the buffer specified by FPREGS and LEN
   in the floating-point register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
m68kbsd_supply_fpregset (const struct regset *regset,
			 struct regcache *regcache,
			 int regnum, const void *fpregs, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const gdb_byte *regs = (const gdb_byte *) fpregs;
  int i;

  gdb_assert (len >= M68KBSD_SIZEOF_FPREGS);

  for (i = M68K_FP0_REGNUM; i <= M68K_PC_REGNUM; i++)
    {
      if (regnum == i || regnum == -1)
	regcache->raw_supply (i, regs + m68kbsd_fpreg_offset (gdbarch, i));
    }
}

/* Supply register REGNUM from the buffer specified by GREGS and LEN
   in the general-purpose register set REGSET to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

static void
m68kbsd_supply_gregset (const struct regset *regset,
			struct regcache *regcache,
			int regnum, const void *gregs, size_t len)
{
  const gdb_byte *regs = (const gdb_byte *) gregs;
  int i;

  gdb_assert (len >= M68KBSD_SIZEOF_GREGS);

  for (i = M68K_D0_REGNUM; i <= M68K_PC_REGNUM; i++)
    {
      if (regnum == i || regnum == -1)
	regcache->raw_supply (i, regs + i * 4);
    }

  if (len >= M68KBSD_SIZEOF_GREGS + M68KBSD_SIZEOF_FPREGS)
    {
      regs += M68KBSD_SIZEOF_GREGS;
      len -= M68KBSD_SIZEOF_GREGS;
      m68kbsd_supply_fpregset (regset, regcache, regnum, regs, len);
    }
}

/* Motorola 68000 register sets.  */

static const struct regset m68kbsd_gregset =
{
  NULL,
  m68kbsd_supply_gregset,
  NULL,
  REGSET_VARIABLE_SIZE
};

static const struct regset m68kbsd_fpregset =
{
  NULL,
  m68kbsd_supply_fpregset
};

/* Iterate over core file register note sections.  */

static void
m68kbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
				      iterate_over_regset_sections_cb *cb,
				      void *cb_data,
				      const struct regcache *regcache)
{
  cb (".reg", M68KBSD_SIZEOF_GREGS, M68KBSD_SIZEOF_GREGS, &m68kbsd_gregset,
      NULL, cb_data);
  cb (".reg2", M68KBSD_SIZEOF_FPREGS, M68KBSD_SIZEOF_FPREGS, &m68kbsd_fpregset,
      NULL, cb_data);
}


static void
m68kbsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  m68k_gdbarch_tdep *tdep = gdbarch_tdep<m68k_gdbarch_tdep> (gdbarch);

  tdep->jb_pc = 5;
  tdep->jb_elt_size = 4;

  set_gdbarch_decr_pc_after_break (gdbarch, 2);

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, m68kbsd_iterate_over_regset_sections);

  /* NetBSD ELF uses the SVR4 ABI.  */
  m68k_svr4_init_abi (info, gdbarch);
  tdep->struct_return = pcc_struct_return;

  /* NetBSD ELF uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);
}

void _initialize_m68kbsd_tdep ();
void
_initialize_m68kbsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_m68k, 0, GDB_OSABI_NETBSD,
			  m68kbsd_init_abi);
}
