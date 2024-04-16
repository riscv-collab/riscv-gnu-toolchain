/* Target-dependent code for SDE on MIPS processors.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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
#include "elf-bfd.h"
#include "symtab.h"

#include "frame.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "trad-frame.h"

#include "mips-tdep.h"

/* Fill in the register cache *THIS_CACHE for THIS_FRAME for use
   in the SDE frame unwinder.  */

static struct trad_frame_cache *
mips_sde_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  const struct mips_regnum *regs = mips_regnum (gdbarch);
  const int sizeof_reg_t = mips_abi_regsize (gdbarch);
  enum mips_abi abi = mips_abi (gdbarch);
  struct trad_frame_cache *cache;
  CORE_ADDR xcpt_frame;
  CORE_ADDR start_addr;
  CORE_ADDR stack_addr;
  CORE_ADDR pc;
  int i;

  if (*this_cache != NULL)
    return (struct trad_frame_cache *) *this_cache;
  cache = trad_frame_cache_zalloc (this_frame);
  *this_cache = cache;

  /* The previous registers are held in struct xcptcontext
     which is at $sp+offs

     struct xcptcontext {
       reg_t	sr;		CP0 Status
       reg_t	cr;		CP0 Cause
       reg_t	epc;		CP0 EPC
       reg_t	vaddr;		CP0 BadVAddr
       reg_t	regs[32];	General registers
       reg_t	mdlo;		LO
       reg_t	mdhi;		HI
       reg_t	mdex;		ACX
       ...
     };
  */

  stack_addr = get_frame_register_signed (this_frame,
					  gdbarch_sp_regnum (gdbarch));
  switch (abi)
    {
    case MIPS_ABI_O32:
      /* 40: XCPTCONTEXT
	 24: xcpt_gen() argspace		(16 bytes)
	 16: _xcptcall() saved ra, rounded up	( 8 bytes)
	 00: _xcptcall() argspace 		(16 bytes)  */
      xcpt_frame = stack_addr + 40;
      break;
    case MIPS_ABI_N32:
    case MIPS_ABI_N64:
    default:			/* Wild guess.  */
      /* 16: XCPTCONTEXT
	 16: xcpt_gen() argspace 		( 0 bytes)
	 00: _xcptcall() saved ra, rounded up	(16 bytes)  */
      xcpt_frame = stack_addr + 16;
      break;
    }

  trad_frame_set_reg_addr (cache,
			   MIPS_PS_REGNUM + gdbarch_num_regs (gdbarch),
			   xcpt_frame + 0 * sizeof_reg_t);
  trad_frame_set_reg_addr (cache,
			   regs->cause + gdbarch_num_regs (gdbarch),
			   xcpt_frame + 1 * sizeof_reg_t);
  trad_frame_set_reg_addr (cache,
			   regs->pc + gdbarch_num_regs (gdbarch),
			   xcpt_frame + 2 * sizeof_reg_t);
  trad_frame_set_reg_addr (cache,
			   regs->badvaddr + gdbarch_num_regs (gdbarch),
			   xcpt_frame + 3 * sizeof_reg_t);
  for (i = 0; i < MIPS_NUMREGS; i++)
    trad_frame_set_reg_addr (cache,
			     i + MIPS_ZERO_REGNUM + gdbarch_num_regs (gdbarch),
			     xcpt_frame + (4 + i) * sizeof_reg_t);
  trad_frame_set_reg_addr (cache,
			   regs->lo + gdbarch_num_regs (gdbarch),
			   xcpt_frame + 36 * sizeof_reg_t);
  trad_frame_set_reg_addr (cache,
			   regs->hi + gdbarch_num_regs (gdbarch),
			   xcpt_frame + 37 * sizeof_reg_t);

  pc = get_frame_pc (this_frame);
  find_pc_partial_function (pc, NULL, &start_addr, NULL);
  trad_frame_set_id (cache, frame_id_build (start_addr, stack_addr));

  return cache;
}

/* Implement the this_id function for the SDE frame unwinder.  */

static void
mips_sde_frame_this_id (frame_info_ptr this_frame, void **this_cache,
			struct frame_id *this_id)
{
  struct trad_frame_cache *this_trad_cache
    = mips_sde_frame_cache (this_frame, this_cache);

  trad_frame_get_id (this_trad_cache, this_id);
}

/* Implement the prev_register function for the SDE frame unwinder.  */

static struct value *
mips_sde_frame_prev_register (frame_info_ptr this_frame,
			      void **this_cache,
			      int prev_regnum)
{
  struct trad_frame_cache *trad_cache
    = mips_sde_frame_cache (this_frame, this_cache);

  return trad_frame_get_register (trad_cache, this_frame, prev_regnum);
}

/* Implement the sniffer function for the SDE frame unwinder.  */

static int
mips_sde_frame_sniffer (const struct frame_unwind *self,
			frame_info_ptr this_frame,
			void **this_cache)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);
  return (name
	  && (strcmp (name, "_xcptcall") == 0
	      || strcmp (name, "_sigtramp") == 0));
}

/* Data structure for the SDE frame unwinder.  */

static const struct frame_unwind mips_sde_frame_unwind =
{
  "mips sde sigtramp",
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  mips_sde_frame_this_id,
  mips_sde_frame_prev_register,
  NULL,
  mips_sde_frame_sniffer
};

/* Implement the this_base, this_locals, and this_args hooks
   for the normal unwinder.  */

static CORE_ADDR
mips_sde_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct trad_frame_cache *this_trad_cache
    = mips_sde_frame_cache (this_frame, this_cache);

  return trad_frame_get_this_base (this_trad_cache);
}

static const struct frame_base mips_sde_frame_base =
{
  &mips_sde_frame_unwind,
  mips_sde_frame_base_address,
  mips_sde_frame_base_address,
  mips_sde_frame_base_address
};

static const struct frame_base *
mips_sde_frame_base_sniffer (frame_info_ptr this_frame)
{
  if (mips_sde_frame_sniffer (&mips_sde_frame_unwind, this_frame, NULL))
    return &mips_sde_frame_base;
  else
    return NULL;
}

static void
mips_sde_elf_osabi_sniff_abi_tag_sections (bfd *abfd, asection *sect,
					   void *obj)
{
  enum gdb_osabi *os_ident_ptr = (enum gdb_osabi *) obj;
  const char *name;

  name = bfd_section_name (sect);

  /* The presence of a section with a ".sde" prefix is indicative
     of an SDE binary.  */
  if (startswith (name, ".sde"))
    *os_ident_ptr = GDB_OSABI_SDE;
}

/* OSABI sniffer for MIPS SDE.  */

static enum gdb_osabi
mips_sde_elf_osabi_sniffer (bfd *abfd)
{
  enum gdb_osabi osabi = GDB_OSABI_UNKNOWN;
  unsigned int elfosabi;

  /* If the generic sniffer gets a hit, return and let other sniffers
     get a crack at it.  */
  for (asection *sect : gdb_bfd_sections (abfd))
    generic_elf_osabi_sniff_abi_tag_sections (abfd, sect, &osabi);
  if (osabi != GDB_OSABI_UNKNOWN)
    return GDB_OSABI_UNKNOWN;

  elfosabi = elf_elfheader (abfd)->e_ident[EI_OSABI];

  if (elfosabi == ELFOSABI_NONE)
    {
      /* When elfosabi is ELFOSABI_NONE (0), then the ELF structures in the
	 file are conforming to the base specification for that machine
	 (there are no OS-specific extensions).  In order to determine the
	 real OS in use we must look for OS notes that have been added.

	 For SDE, we simply look for sections named with .sde as prefixes.  */
      bfd_map_over_sections (abfd,
			     mips_sde_elf_osabi_sniff_abi_tag_sections,
			     &osabi);
    }
  return osabi;
}

static void
mips_sde_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  frame_unwind_append_unwinder (gdbarch, &mips_sde_frame_unwind);
  frame_base_append_sniffer (gdbarch, mips_sde_frame_base_sniffer);
}

void _initialize_mips_sde_tdep ();
void
_initialize_mips_sde_tdep ()
{
  gdbarch_register_osabi_sniffer (bfd_arch_mips,
				  bfd_target_elf_flavour,
				  mips_sde_elf_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_mips, 0, GDB_OSABI_SDE, mips_sde_init_abi);
}
