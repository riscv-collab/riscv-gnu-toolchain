/* Target-dependent code for HP PA-RISC BSD's.

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
#include "objfiles.h"
#include "target.h"
#include "value.h"

#include "elf/common.h"

#include "hppa-tdep.h"
#include "hppa-bsd-tdep.h"
#include "dwarf2/frame.h"
#include "solib-svr4.h"

static CORE_ADDR
hppabsd_find_global_pointer (struct gdbarch *gdbarch, struct value *function)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR faddr = value_as_address (function);
  struct obj_section *faddr_sec;

  /* Is this a plabel? If so, dereference it to get the Global Pointer
     value.  */
  if (faddr & 2)
    {
      gdb_byte buf[4];
      if (target_read_memory ((faddr & ~3) + 4, buf, sizeof buf) == 0)
	return extract_unsigned_integer (buf, sizeof buf, byte_order);
    }

  /* If the address is in the .plt section, then the real function
     hasn't yet been fixed up by the linker so we cannot determine the
     Global Pointer for that function.  */
  if (in_plt_section (faddr))
    return 0;

  faddr_sec = find_pc_section (faddr);
  if (faddr_sec != NULL)
    {
      for (struct obj_section *sec : faddr_sec->objfile->sections ())
	{
	  if (strcmp (sec->the_bfd_section->name, ".dynamic") == 0)
	    {
	      CORE_ADDR addr = sec->addr ();
	      CORE_ADDR endaddr = sec->endaddr ();

	      while (addr < endaddr)
		{
		  gdb_byte buf[4];
		  LONGEST tag;

		  if (target_read_memory (addr, buf, sizeof buf) != 0)
		    break;

		  tag = extract_signed_integer (buf, byte_order);
		  if (tag == DT_PLTGOT)
		    {
		      CORE_ADDR pltgot;

		      if (target_read_memory (addr + 4, buf, sizeof buf) != 0)
			break;

		      /* The NetBSD/OpenBSD ld.so doesn't relocate
			 DT_PLTGOT, so we have to do it ourselves.  */
		      pltgot = extract_unsigned_integer (buf, sizeof buf,
							 byte_order);
		      pltgot += sec->objfile->text_section_offset ();

		      return pltgot;
		    }

		  if (tag == DT_NULL)
		    break;

		  addr += 8;
		}

	      break;
	    }
	}
    }

  return 0;
}


static void
hppabsd_dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
			       struct dwarf2_frame_state_reg *reg,
			       frame_info_ptr this_frame)
{
  if (regnum == HPPA_PCOQ_HEAD_REGNUM)
    reg->how = DWARF2_FRAME_REG_RA;
  else if (regnum == HPPA_SP_REGNUM)
    reg->how = DWARF2_FRAME_REG_CFA;
}

void
hppabsd_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  hppa_gdbarch_tdep *tdep = gdbarch_tdep<hppa_gdbarch_tdep> (gdbarch);

  /* OpenBSD and NetBSD have a 64-bit 'long double'.  */
  set_gdbarch_long_double_bit (gdbarch, 64);
  set_gdbarch_long_double_format (gdbarch, floatformats_ieee_double);

  /* OpenBSD and NetBSD use ELF.  */
  tdep->is_elf = 1;
  tdep->find_global_pointer = hppabsd_find_global_pointer;
  tdep->in_solib_call_trampoline = hppa_in_solib_call_trampoline;
  set_gdbarch_skip_trampoline_code (gdbarch, hppa_skip_trampoline_code);

  /* OpenBSD and NetBSD use SVR4-style shared libraries.  */
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  /* Hook in the DWARF CFI frame unwinder.  */
  dwarf2_frame_set_init_reg (gdbarch, hppabsd_dwarf2_frame_init_reg);
  dwarf2_append_unwinders (gdbarch);
}
