/* Common target dependent code for GNU/Linux on PPC systems.

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "arch/ppc-linux-common.h"
#include "arch/ppc-linux-tdesc.h"

/* Decimal Floating Point bit in AT_HWCAP.

   This file can be used by a host with another architecture, e.g.
   when debugging core files, which might not provide this constant.  */

#ifndef PPC_FEATURE_HAS_DFP
#define PPC_FEATURE_HAS_DFP	0x00000400
#endif

bool
ppc_linux_has_isa205 (CORE_ADDR hwcap)
{
  /* Power ISA 2.05 (implemented by Power 6 and newer processors)
     increases the FPSCR from 32 bits to 64 bits.  Even though Power 7
     supports this ISA version, it doesn't have PPC_FEATURE_ARCH_2_05
     set, only PPC_FEATURE_ARCH_2_06.  Since for now the only bits
     used in the higher half of the register are for Decimal Floating
     Point, we check if that feature is available to decide the size
     of the FPSCR.  */
  return ((hwcap & PPC_FEATURE_HAS_DFP) != 0);
}

const struct target_desc *
ppc_linux_match_description (struct ppc_linux_features features)
{
  const struct target_desc *tdesc = NULL;

  if (features.wordsize == 8)
    {
      if (features.vsx)
	tdesc = (features.htm? tdesc_powerpc_isa207_htm_vsx64l
		 : features.isa207? tdesc_powerpc_isa207_vsx64l
		 : features.ppr_dscr? tdesc_powerpc_isa205_ppr_dscr_vsx64l
		 : features.isa205? tdesc_powerpc_isa205_vsx64l
		 : tdesc_powerpc_vsx64l);
      else if (features.altivec)
	tdesc = (features.isa205? tdesc_powerpc_isa205_altivec64l
		 : tdesc_powerpc_altivec64l);
      else
	tdesc = (features.isa205? tdesc_powerpc_isa205_64l
		 : tdesc_powerpc_64l);
    }
  else
    {
      gdb_assert (features.wordsize == 4);

      if (features.vsx)
	tdesc = (features.htm? tdesc_powerpc_isa207_htm_vsx32l
		 : features.isa207? tdesc_powerpc_isa207_vsx32l
		 : features.ppr_dscr? tdesc_powerpc_isa205_ppr_dscr_vsx32l
		 : features.isa205? tdesc_powerpc_isa205_vsx32l
		 : tdesc_powerpc_vsx32l);
      else if (features.altivec)
	tdesc = (features.isa205? tdesc_powerpc_isa205_altivec32l
		 : tdesc_powerpc_altivec32l);
      else
	tdesc = (features.isa205? tdesc_powerpc_isa205_32l
		 : tdesc_powerpc_32l);
    }

  gdb_assert (tdesc != NULL);

  return tdesc;
}
