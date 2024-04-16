/* none on ARM target support.

   Copyright (C) 2020-2024 Free Software Foundation, Inc.

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
#include "arm-tdep.h"
#include "arch-utils.h"
#include "regcache.h"
#include "elf-bfd.h"
#include "regset.h"
#include "user-regs.h"

#ifdef HAVE_ELF
#include "elf-none-tdep.h"
#endif

/* Core file and register set support.  */
#define ARM_NONE_SIZEOF_GREGSET (18 * ARM_INT_REGISTER_SIZE)

/* Support VFP register format.  */
#define ARM_NONE_SIZEOF_VFP (32 * 8 + 4)

/* The index to access CPSR in user_regs as defined in GLIBC.  */
#define ARM_NONE_CPSR_GREGNUM 16

/* Supply register REGNUM from buffer GREGS_BUF (length LEN bytes) into
   REGCACHE.  If REGNUM is -1 then supply all registers.  The set of
   registers that this function will supply is limited to the general
   purpose registers.

   The layout of the registers here is based on the ARM GNU/Linux
   layout.  */

static void
arm_none_supply_gregset (const struct regset *regset,
			 struct regcache *regcache,
			 int regnum, const void *gregs_buf, size_t len)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  const gdb_byte *gregs = (const gdb_byte *) gregs_buf;

  for (int regno = ARM_A1_REGNUM; regno < ARM_PC_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      regcache->raw_supply (regno, gregs + ARM_INT_REGISTER_SIZE * regno);

  if (regnum == ARM_PS_REGNUM || regnum == -1)
    {
      if (arm_apcs_32)
	regcache->raw_supply (ARM_PS_REGNUM,
			      gregs + ARM_INT_REGISTER_SIZE
			      * ARM_NONE_CPSR_GREGNUM);
      else
	regcache->raw_supply (ARM_PS_REGNUM,
			     gregs + ARM_INT_REGISTER_SIZE * ARM_PC_REGNUM);
    }

  if (regnum == ARM_PC_REGNUM || regnum == -1)
    {
      gdb_byte pc_buf[ARM_INT_REGISTER_SIZE];

      CORE_ADDR reg_pc
	= extract_unsigned_integer (gregs + ARM_INT_REGISTER_SIZE
				    * ARM_PC_REGNUM,
				    ARM_INT_REGISTER_SIZE, byte_order);
      reg_pc = gdbarch_addr_bits_remove (gdbarch, reg_pc);
      store_unsigned_integer (pc_buf, ARM_INT_REGISTER_SIZE, byte_order,
			      reg_pc);
      regcache->raw_supply (ARM_PC_REGNUM, pc_buf);
    }
}

/* Collect register REGNUM from REGCACHE and place it into buffer GREGS_BUF
   (length LEN bytes).  If REGNUM is -1 then collect all registers.  The
   set of registers that this function will collect is limited to the
   general purpose registers.

   The layout of the registers here is based on the ARM GNU/Linux
   layout.  */

static void
arm_none_collect_gregset (const struct regset *regset,
			  const struct regcache *regcache,
			  int regnum, void *gregs_buf, size_t len)
{
  gdb_byte *gregs = (gdb_byte *) gregs_buf;

  for (int regno = ARM_A1_REGNUM; regno < ARM_PC_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      regcache->raw_collect (regno,
			     gregs + ARM_INT_REGISTER_SIZE * regno);

  if (regnum == ARM_PS_REGNUM || regnum == -1)
    {
      if (arm_apcs_32)
	regcache->raw_collect (ARM_PS_REGNUM,
			       gregs + ARM_INT_REGISTER_SIZE
			       * ARM_NONE_CPSR_GREGNUM);
      else
	regcache->raw_collect (ARM_PS_REGNUM,
			       gregs + ARM_INT_REGISTER_SIZE * ARM_PC_REGNUM);
    }

  if (regnum == ARM_PC_REGNUM || regnum == -1)
    regcache->raw_collect (ARM_PC_REGNUM,
			   gregs + ARM_INT_REGISTER_SIZE * ARM_PC_REGNUM);
}

/* Supply VFP registers from REGS_BUF into REGCACHE.  */

static void
arm_none_supply_vfp (const struct regset *regset,
		     struct regcache *regcache,
		     int regnum, const void *regs_buf, size_t len)
{
  const gdb_byte *regs = (const gdb_byte *) regs_buf;

  if (regnum == ARM_FPSCR_REGNUM || regnum == -1)
    regcache->raw_supply (ARM_FPSCR_REGNUM, regs + 32 * 8);

  for (int regno = ARM_D0_REGNUM; regno <= ARM_D31_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      regcache->raw_supply (regno, regs + (regno - ARM_D0_REGNUM) * 8);
}

/* Collect VFP registers from REGCACHE into REGS_BUF.  */

static void
arm_none_collect_vfp (const struct regset *regset,
		      const struct regcache *regcache,
		      int regnum, void *regs_buf, size_t len)
{
  gdb_byte *regs = (gdb_byte *) regs_buf;

  if (regnum == ARM_FPSCR_REGNUM || regnum == -1)
    regcache->raw_collect (ARM_FPSCR_REGNUM, regs + 32 * 8);

  for (int regno = ARM_D0_REGNUM; regno <= ARM_D31_REGNUM; regno++)
    if (regnum == -1 || regnum == regno)
      regcache->raw_collect (regno, regs + (regno - ARM_D0_REGNUM) * 8);
}

/* The general purpose register set.  */

static const struct regset arm_none_gregset =
  {
    nullptr, arm_none_supply_gregset, arm_none_collect_gregset
  };

/* The VFP register set.  */

static const struct regset arm_none_vfpregset =
  {
    nullptr, arm_none_supply_vfp, arm_none_collect_vfp
  };

/* Iterate over core file register note sections.  */

static void
arm_none_iterate_over_regset_sections (struct gdbarch *gdbarch,
				       iterate_over_regset_sections_cb *cb,
				       void *cb_data,
				       const struct regcache *regcache)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  cb (".reg", ARM_NONE_SIZEOF_GREGSET, ARM_NONE_SIZEOF_GREGSET,
      &arm_none_gregset, nullptr, cb_data);

  if (tdep->vfp_register_count > 0)
    cb (".reg-arm-vfp", ARM_NONE_SIZEOF_VFP, ARM_NONE_SIZEOF_VFP,
	&arm_none_vfpregset, "VFP floating-point", cb_data);
}

/* Initialize ARM bare-metal ABI info.  */

static void
arm_none_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
#ifdef HAVE_ELF
  elf_none_init_abi (gdbarch);
#endif

  /* Iterate over registers for reading and writing bare metal ARM core
     files.  */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, arm_none_iterate_over_regset_sections);
}

/* Initialize ARM bare-metal target support.  */

void _initialize_arm_none_tdep ();
void
_initialize_arm_none_tdep ()
{
  gdbarch_register_osabi (bfd_arch_arm, 0, GDB_OSABI_NONE,
			  arm_none_init_abi);
}
