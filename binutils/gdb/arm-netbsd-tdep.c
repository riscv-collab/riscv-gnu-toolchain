/* Target-dependent code for NetBSD/arm.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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

#include "arch/arm.h"
#include "arm-netbsd-tdep.h"
#include "netbsd-tdep.h"
#include "arm-tdep.h"
#include "regset.h"
#include "solib-svr4.h"

/* Description of the longjmp buffer.  */
#define ARM_NBSD_JB_PC 24
#define ARM_NBSD_JB_ELEMENT_SIZE ARM_INT_REGISTER_SIZE

/* For compatibility with previous implementations of GDB on arm/NetBSD,
   override the default little-endian breakpoint.  */
static const gdb_byte arm_nbsd_arm_le_breakpoint[] = {0x11, 0x00, 0x00, 0xe6};
static const gdb_byte arm_nbsd_arm_be_breakpoint[] = {0xe6, 0x00, 0x00, 0x11};
static const gdb_byte arm_nbsd_thumb_le_breakpoint[] = {0xfe, 0xde};
static const gdb_byte arm_nbsd_thumb_be_breakpoint[] = {0xde, 0xfe};

/* This matches struct reg from NetBSD's sys/arch/arm/include/reg.h:
   https://github.com/NetBSD/src/blob/7c13e6e6773bb171f4ed3ed53013e9d24b3c1eac/sys/arch/arm/include/reg.h#L39
 */
struct arm_nbsd_reg
{
  uint32_t reg[13];
  uint32_t sp;
  uint32_t lr;
  uint32_t pc;
  uint32_t cpsr;
};

void
arm_nbsd_supply_gregset (const struct regset *regset, struct regcache *regcache,
			 int regnum, const void *gregs, size_t len)
{
  const arm_nbsd_reg *gregset = static_cast<const arm_nbsd_reg *>(gregs);
  gdb_assert (len >= sizeof (arm_nbsd_reg));

  /* Integer registers.  */
  for (int i = ARM_A1_REGNUM; i < ARM_SP_REGNUM; i++)
    if (regnum == -1 || regnum == i)
      regcache->raw_supply (i, (char *) &gregset->reg[i]);

  if (regnum == -1 || regnum == ARM_SP_REGNUM)
    regcache->raw_supply (ARM_SP_REGNUM, (char *) &gregset->sp);

  if (regnum == -1 || regnum == ARM_LR_REGNUM)
    regcache->raw_supply (ARM_LR_REGNUM, (char *) &gregset->lr);

  if (regnum == -1 || regnum == ARM_PC_REGNUM)
    {
      CORE_ADDR r_pc = gdbarch_addr_bits_remove (regcache->arch (), gregset->pc);
      regcache->raw_supply (ARM_PC_REGNUM, (char *) &r_pc);
    }

  if (regnum == -1 || regnum == ARM_PS_REGNUM)
    {
      if (arm_apcs_32)
	regcache->raw_supply (ARM_PS_REGNUM, (char *) &gregset->cpsr);
      else
	regcache->raw_supply (ARM_PS_REGNUM, (char *) &gregset->pc);
    }
}

static const struct regset arm_nbsd_regset = {
  nullptr,
  arm_nbsd_supply_gregset,
  /* We don't need a collect function because we only use this reading registers
     (via iterate_over_regset_sections and fetch_regs/fetch_register).  */
  nullptr,
  0
};

static void
arm_nbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
				       iterate_over_regset_sections_cb *cb,
				       void *cb_data,
				       const struct regcache *regcache)
{
  cb (".reg", sizeof (arm_nbsd_reg), sizeof (arm_nbsd_reg), &arm_nbsd_regset,
      NULL, cb_data);
  /* cbiesinger/2020-02-12 -- as far as I can tell, ARM/NetBSD does
     not write any floating point registers into the core file (tested
     with NetBSD 9.1_RC1).  When it does, this function will need to read them,
     and the arm-netbsd gdbarch will need a core_read_description function
     to return the right description for them.  */
}

static void
arm_netbsd_init_abi_common (struct gdbarch_info info,
			    struct gdbarch *gdbarch)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  tdep->lowest_pc = 0x8000;
  switch (info.byte_order)
    {
    case BFD_ENDIAN_LITTLE:
      tdep->arm_breakpoint = arm_nbsd_arm_le_breakpoint;
      tdep->thumb_breakpoint = arm_nbsd_thumb_le_breakpoint;
      tdep->arm_breakpoint_size = sizeof (arm_nbsd_arm_le_breakpoint);
      tdep->thumb_breakpoint_size = sizeof (arm_nbsd_thumb_le_breakpoint);
      break;

    case BFD_ENDIAN_BIG:
      tdep->arm_breakpoint = arm_nbsd_arm_be_breakpoint;
      tdep->thumb_breakpoint = arm_nbsd_thumb_be_breakpoint;
      tdep->arm_breakpoint_size = sizeof (arm_nbsd_arm_be_breakpoint);
      tdep->thumb_breakpoint_size = sizeof (arm_nbsd_thumb_be_breakpoint);
      break;

    default:
      internal_error (_("arm_gdbarch_init: bad byte order for float format"));
    }

  tdep->jb_pc = ARM_NBSD_JB_PC;
  tdep->jb_elt_size = ARM_NBSD_JB_ELEMENT_SIZE;

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, arm_nbsd_iterate_over_regset_sections);
  /* Single stepping.  */
  set_gdbarch_software_single_step (gdbarch, arm_software_single_step);
}

static void
arm_netbsd_elf_init_abi (struct gdbarch_info info,
			 struct gdbarch *gdbarch)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  arm_netbsd_init_abi_common (info, gdbarch);

  nbsd_init_abi (info, gdbarch);

  if (tdep->fp_model == ARM_FLOAT_AUTO)
    tdep->fp_model = ARM_FLOAT_SOFT_VFP;

  /* NetBSD ELF uses SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);
}

void _initialize_arm_netbsd_tdep ();
void
_initialize_arm_netbsd_tdep ()
{
  gdbarch_register_osabi (bfd_arch_arm, 0, GDB_OSABI_NETBSD,
			  arm_netbsd_elf_init_abi);
}
