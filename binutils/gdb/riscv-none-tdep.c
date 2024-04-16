/* Copyright (C) 2020-2024 Free Software Foundation, Inc.

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

/* This file contain code that is specific for bare-metal RISC-V targets.  */

#include "defs.h"
#include "arch-utils.h"
#include "regcache.h"
#include "riscv-tdep.h"
#include "elf-bfd.h"
#include "regset.h"
#include "user-regs.h"
#include "target-descriptions.h"

#ifdef HAVE_ELF
#include "elf-none-tdep.h"
#endif

/* Define the general register mapping.  This follows the same format as
   the RISC-V linux corefile.  The linux kernel puts the PC at offset 0,
   gdb puts it at offset 32.  Register x0 is always 0 and can be ignored.
   Registers x1 to x31 are in the same place.  */

static const struct regcache_map_entry riscv_gregmap[] =
{
  { 1,  RISCV_PC_REGNUM, 0 },
  { 31, RISCV_RA_REGNUM, 0 }, /* x1 to x31 */
  { 0 }
};

/* Define the FP register mapping.  This follows the same format as the
   RISC-V linux corefile.  The kernel puts the 32 FP regs first, and then
   FCSR.  */

static const struct regcache_map_entry riscv_fregmap[] =
{
  { 32, RISCV_FIRST_FP_REGNUM, 0 },
  { 1, RISCV_CSR_FCSR_REGNUM, 4 },	/* Always stored as 4-bytes.  */
  { 0 }
};

/* Define the general register regset.  */

static const struct regset riscv_gregset =
{
  riscv_gregmap, riscv_supply_regset, regcache_collect_regset
};

/* Define the FP register regset.  */

static const struct regset riscv_fregset =
{
  riscv_fregmap, riscv_supply_regset, regcache_collect_regset
};

/* Define the CSR regset, this is not constant as the regmap field is
   updated dynamically based on the current target description.  */

static struct regset riscv_csrset =
{
  nullptr, regcache_supply_regset, regcache_collect_regset
};

/* Update the regmap field of RISCV_CSRSET based on the CSRs available in
   the current target description.  */

static void
riscv_update_csrmap (struct gdbarch *gdbarch,
		     const struct tdesc_feature *feature_csr)
{
  int i = 0;

  /* Release any previously defined map.  */
  delete[] ((struct regcache_map_entry *) riscv_csrset.regmap);

  /* Now create a register map for every csr found in the target
     description.  */
  struct regcache_map_entry *riscv_csrmap
    = new struct regcache_map_entry[feature_csr->registers.size() + 1];
  for (auto &csr : feature_csr->registers)
    {
      int regnum = user_reg_map_name_to_regnum (gdbarch, csr->name.c_str(),
						csr->name.length());
      riscv_csrmap[i++] = {1, regnum, 0};
    }

  /* Mark the end of the array.  */
  riscv_csrmap[i] = {0};
  riscv_csrset.regmap = riscv_csrmap;
}

/* Implement the "iterate_over_regset_sections" gdbarch method.  */

static void
riscv_iterate_over_regset_sections (struct gdbarch *gdbarch,
				    iterate_over_regset_sections_cb *cb,
				    void *cb_data,
				    const struct regcache *regcache)
{
  /* Write out the GPRs.  */
  int sz = 32 * riscv_isa_xlen (gdbarch);
  cb (".reg", sz, sz, &riscv_gregset, NULL, cb_data);

  /* Write out the FPRs, but only if present.  */
  if (riscv_isa_flen (gdbarch) > 0)
    {
      sz = (32 * riscv_isa_flen (gdbarch)
	    + register_size (gdbarch, RISCV_CSR_FCSR_REGNUM));
      cb (".reg2", sz, sz, &riscv_fregset, NULL, cb_data);
    }

  /* Read or write the CSRs.  The set of CSRs is defined by the current
     target description.  The user is responsible for ensuring that the
     same target description is in use when reading the core file as was
     in use when writing the core file.  */
  const struct target_desc *tdesc = gdbarch_target_desc (gdbarch);

  /* Do not dump/load any CSRs if there is no target description or the target
     description does not contain any CSRs.  */
  if (tdesc != nullptr)
    {
      const struct tdesc_feature *feature_csr
	= tdesc_find_feature (tdesc, riscv_feature_name_csr);
      if (feature_csr != nullptr && feature_csr->registers.size () > 0)
	{
	  riscv_update_csrmap (gdbarch, feature_csr);
	  cb (".reg-riscv-csr",
	      (feature_csr->registers.size() * riscv_isa_xlen (gdbarch)),
	      (feature_csr->registers.size() * riscv_isa_xlen (gdbarch)),
	      &riscv_csrset, NULL, cb_data);
	}
    }
}

/* Initialize RISC-V bare-metal ABI info.  */

static void
riscv_none_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
#ifdef HAVE_ELF
  elf_none_init_abi (gdbarch);
#endif

  /* Iterate over registers for reading and writing bare metal RISC-V core
     files.  */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, riscv_iterate_over_regset_sections);

}

/* Initialize RISC-V bare-metal target support.  */

void _initialize_riscv_none_tdep ();
void
_initialize_riscv_none_tdep ()
{
  gdbarch_register_osabi (bfd_arch_riscv, 0, GDB_OSABI_NONE,
			  riscv_none_init_abi);
}
