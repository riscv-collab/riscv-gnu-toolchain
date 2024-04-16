/* Target-dependent code for Windows CE running on ARM processors,
   for GDB.

   Copyright (C) 2007-2024 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "target.h"
#include "frame.h"

#include "arch/arm.h"
#include "arm-tdep.h"
#include "windows-tdep.h"

static const gdb_byte arm_wince_le_breakpoint[] = { 0x10, 0x00, 0x00, 0xe6 };
static const gdb_byte arm_wince_thumb_le_breakpoint[] = { 0xfe, 0xdf };

/* Description of the longjmp buffer.  */
#define ARM_WINCE_JB_ELEMENT_SIZE	ARM_INT_REGISTER_SIZE
#define ARM_WINCE_JB_PC			10

static CORE_ADDR
arm_pe_skip_trampoline_code (frame_info_ptr frame, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST indirect;
  struct bound_minimal_symbol indsym;
  const char *symname;
  CORE_ADDR next_pc;

  /* The format of an ARM DLL trampoline is:

       ldr  ip, [pc]
       ldr  pc, [ip]
       .dw __imp_<func>

  */

  if (pc == 0
      || read_memory_unsigned_integer (pc + 0, 4, byte_order) != 0xe59fc000
      || read_memory_unsigned_integer (pc + 4, 4, byte_order) != 0xe59cf000)
    return 0;

  indirect = read_memory_unsigned_integer (pc + 8, 4, byte_order);
  if (indirect == 0)
    return 0;

  indsym = lookup_minimal_symbol_by_pc (indirect);
  if (indsym.minsym == NULL)
    return 0;

  symname = indsym.minsym->linkage_name ();
  if (symname == NULL || !startswith (symname, "__imp_"))
    return 0;

  next_pc = read_memory_unsigned_integer (indirect, 4, byte_order);
  if (next_pc != 0)
    return next_pc;

  /* Check with the default arm gdbarch_skip_trampoline.  */
  return arm_skip_stub (frame, pc);
}

/* GCC emits a call to __gccmain in the prologue of main.

   The function below examines the code pointed at by PC and checks to
   see if it corresponds to a call to __gccmain.  If so, it returns
   the address of the instruction following that call.  Otherwise, it
   simply returns PC.  */

static CORE_ADDR
arm_wince_skip_main_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST this_instr;

  this_instr = read_memory_unsigned_integer (pc, 4, byte_order);

  /* bl offset <__gccmain> */
  if ((this_instr & 0xfff00000) == 0xeb000000)
    {
#define sign_extend(V, N) \
  (((long) (V) ^ (1L << ((N) - 1))) - (1L << ((N) - 1)))

      long offset = sign_extend (this_instr & 0x000fffff, 23) << 2;
      CORE_ADDR call_dest = (pc + 8 + offset) & 0xffffffffU;
      struct bound_minimal_symbol s = lookup_minimal_symbol_by_pc (call_dest);

      if (s.minsym != NULL
	  && s.minsym->linkage_name () != NULL
	  && strcmp (s.minsym->linkage_name (), "__gccmain") == 0)
	pc += 4;
    }

  return pc;
}

static void
arm_wince_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  arm_gdbarch_tdep *tdep = gdbarch_tdep<arm_gdbarch_tdep> (gdbarch);

  windows_init_abi (info, gdbarch);

  tdep->arm_breakpoint = arm_wince_le_breakpoint;
  tdep->arm_breakpoint_size = sizeof (arm_wince_le_breakpoint);
  tdep->thumb_breakpoint = arm_wince_thumb_le_breakpoint;
  tdep->thumb_breakpoint_size = sizeof (arm_wince_thumb_le_breakpoint);
  tdep->struct_return = pcc_struct_return;

  tdep->fp_model = ARM_FLOAT_SOFT_VFP;

  tdep->jb_pc = ARM_WINCE_JB_PC;
  tdep->jb_elt_size = ARM_WINCE_JB_ELEMENT_SIZE;

  /* On ARM WinCE char defaults to signed.  */
  set_gdbarch_char_signed (gdbarch, 1);

  /* Shared library handling.  */
  set_gdbarch_skip_trampoline_code (gdbarch, arm_pe_skip_trampoline_code);

  /* Single stepping.  */
  set_gdbarch_software_single_step (gdbarch, arm_software_single_step);

  /* Skip call to __gccmain that gcc places in main.  */
  set_gdbarch_skip_main_prologue (gdbarch, arm_wince_skip_main_prologue);
}

static enum gdb_osabi
arm_wince_osabi_sniffer (bfd *abfd)
{
  const char *target_name = bfd_get_target (abfd);

  if (strcmp (target_name, "pei-arm-wince-little") == 0)
    return GDB_OSABI_WINCE;

  return GDB_OSABI_UNKNOWN;
}

void _initialize_arm_wince_tdep ();
void
_initialize_arm_wince_tdep ()
{
  gdbarch_register_osabi_sniffer (bfd_arch_arm, bfd_target_coff_flavour,
				  arm_wince_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_arm, 0, GDB_OSABI_WINCE,
			  arm_wince_init_abi);
}
