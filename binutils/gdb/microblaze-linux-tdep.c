/* Target-dependent code for Xilinx MicroBlaze.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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
#include "frame.h"
#include "inferior.h"
#include "symtab.h"
#include "target.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "symfile.h"
#include "objfiles.h"
#include "regcache.h"
#include "value.h"
#include "osabi.h"
#include "regset.h"
#include "solib-svr4.h"
#include "microblaze-tdep.h"
#include "trad-frame.h"
#include "frame-unwind.h"
#include "tramp-frame.h"
#include "linux-tdep.h"

static int
microblaze_linux_memory_remove_breakpoint (struct gdbarch *gdbarch, 
					   struct bp_target_info *bp_tgt)
{
  CORE_ADDR addr = bp_tgt->reqstd_address;
  const gdb_byte *bp;
  int val;
  int bplen;
  gdb_byte old_contents[BREAKPOINT_MAX];

  /* Determine appropriate breakpoint contents and size for this address.  */
  bp = gdbarch_breakpoint_from_pc (gdbarch, &addr, &bplen);

  val = target_read_memory (addr, old_contents, bplen);

  /* If our breakpoint is no longer at the address, this means that the
     program modified the code on us, so it is wrong to put back the
     old value.  */
  if (val == 0 && memcmp (bp, old_contents, bplen) == 0)
    val = target_write_raw_memory (addr, bp_tgt->shadow_contents, bplen);

  return val;
}

static void
microblaze_linux_sigtramp_cache (frame_info_ptr next_frame,
				 struct trad_frame_cache *this_cache,
				 CORE_ADDR func, LONGEST offset,
				 int bias)
{
  CORE_ADDR base;
  CORE_ADDR gpregs;
  int regnum;

  base = frame_unwind_register_unsigned (next_frame, MICROBLAZE_SP_REGNUM);
  if (bias > 0 && get_frame_address_in_block (next_frame) != func)
    /* See below, some signal trampolines increment the stack as their
       first instruction, need to compensate for that.  */
    base -= bias;

  /* Find the address of the register buffer.  */
  gpregs = base + offset;

  /* Registers saved on stack.  */
  for (regnum = 0; regnum < MICROBLAZE_BTR_REGNUM; regnum++)
    trad_frame_set_reg_addr (this_cache, regnum, 
			     gpregs + regnum * MICROBLAZE_REGISTER_SIZE);
  trad_frame_set_id (this_cache, frame_id_build (base, func));
}


static void
microblaze_linux_sighandler_cache_init (const struct tramp_frame *self,
					frame_info_ptr next_frame,
					struct trad_frame_cache *this_cache,
					CORE_ADDR func)
{
  microblaze_linux_sigtramp_cache (next_frame, this_cache, func,
				   0 /* Offset to ucontext_t.  */
				   + 24 /* Offset to .reg.  */,
				   0);
}

static struct tramp_frame microblaze_linux_sighandler_tramp_frame = 
{
  SIGTRAMP_FRAME,
  4,
  {
    { 0x31800077, ULONGEST_MAX }, /* addik R12,R0,119.  */
    { 0xb9cc0008, ULONGEST_MAX }, /* brki R14,8.  */
    { TRAMP_SENTINEL_INSN },
  },
  microblaze_linux_sighandler_cache_init
};


static void
microblaze_linux_init_abi (struct gdbarch_info info,
			   struct gdbarch *gdbarch)
{
  linux_init_abi (info, gdbarch, 0);

  set_gdbarch_memory_remove_breakpoint (gdbarch,
					microblaze_linux_memory_remove_breakpoint);

  /* Shared library handling.  */
  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 linux_ilp32_fetch_link_map_offsets);

  /* Trampolines.  */
  tramp_frame_prepend_unwinder (gdbarch,
				&microblaze_linux_sighandler_tramp_frame);
}

void _initialize_microblaze_linux_tdep ();
void
_initialize_microblaze_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_microblaze, 0, GDB_OSABI_LINUX, 
			  microblaze_linux_init_abi);
}
