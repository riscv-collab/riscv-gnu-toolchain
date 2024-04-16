/* Copyright (C) 1999-2024 Free Software Foundation, Inc.

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

#include "regcache.h"
#include "arm-tdep.h"
#include "arm-linux-tdep.h"
#include "arch/arm-linux.h"

#include "aarch32-linux-nat.h"

/* Supply GP registers contents, stored in REGS, to REGCACHE.  ARM_APCS_32
   is true if the 32-bit mode is in use, otherwise, it is false.  */

void
aarch32_gp_regcache_supply (struct regcache *regcache, uint32_t *regs,
			    int arm_apcs_32)
{
  int regno;

  for (regno = ARM_A1_REGNUM; regno < ARM_PC_REGNUM; regno++)
    regcache->raw_supply (regno, &regs[regno]);

  if (arm_apcs_32)
    {
      /* Clear reserved bits bit 20 to bit 23.  */
      regs[ARM_CPSR_GREGNUM] &= 0xff0fffff;
      regcache->raw_supply (ARM_PS_REGNUM, &regs[ARM_CPSR_GREGNUM]);
    }
  else
    regcache->raw_supply (ARM_PS_REGNUM, &regs[ARM_PC_REGNUM]);

  regs[ARM_PC_REGNUM] = gdbarch_addr_bits_remove
			  (regcache->arch (), regs[ARM_PC_REGNUM]);
  regcache->raw_supply (ARM_PC_REGNUM, &regs[ARM_PC_REGNUM]);
}

/* Collect GP registers from REGCACHE to buffer REGS.  ARM_APCS_32 is
   true if the 32-bit mode is in use, otherwise, it is false.  */

void
aarch32_gp_regcache_collect (const struct regcache *regcache, uint32_t *regs,
			     int arm_apcs_32)
{
  int regno;

  for (regno = ARM_A1_REGNUM; regno <= ARM_PC_REGNUM; regno++)
    {
      if (REG_VALID == regcache->get_register_status (regno))
	regcache->raw_collect (regno, &regs[regno]);
    }

  if (arm_apcs_32
      && REG_VALID == regcache->get_register_status (ARM_PS_REGNUM))
    {
      uint32_t cpsr = regs[ARM_CPSR_GREGNUM];

      regcache->raw_collect (ARM_PS_REGNUM, &regs[ARM_CPSR_GREGNUM]);
      /* Keep reserved bits bit 20 to bit 23.  */
      regs[ARM_CPSR_GREGNUM] = ((regs[ARM_CPSR_GREGNUM] & 0xff0fffff)
				| (cpsr & 0x00f00000));
    }
}

/* Supply VFP registers contents, stored in REGS, to REGCACHE.
   VFP_REGISTER_COUNT is the number of VFP registers.  */

void
aarch32_vfp_regcache_supply (struct regcache *regcache, gdb_byte *regs,
			     const int vfp_register_count)
{
  int regno;

  for (regno = 0; regno < vfp_register_count; regno++)
    regcache->raw_supply (regno + ARM_D0_REGNUM, regs + regno * 8);

  regcache->raw_supply (ARM_FPSCR_REGNUM, regs + 32 * 8);
}

/* Collect VFP registers from REGCACHE to buffer REGS.
   VFP_REGISTER_COUNT is the number VFP registers.  */

void
aarch32_vfp_regcache_collect (const struct regcache *regcache, gdb_byte *regs,
			      const int vfp_register_count)
{
  int regno;

  for (regno = 0; regno < vfp_register_count; regno++)
    regcache->raw_collect (regno + ARM_D0_REGNUM, regs + regno * 8);

  regcache->raw_collect (ARM_FPSCR_REGNUM, regs + 32 * 8);
}
