/* Simulate breakpoints by patching locations in the target system, for GDB.

   Copyright (C) 1990-2024 Free Software Foundation, Inc.

   Contributed by Cygnus Support.  Written by John Gilmore.

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
#include "symtab.h"
#include "breakpoint.h"
#include "inferior.h"
#include "target.h"
#include "gdbarch.h"

/* Insert a breakpoint on targets that don't have any better
   breakpoint support.  We read the contents of the target location
   and stash it, then overwrite it with a breakpoint instruction.
   BP_TGT->placed_address is the target location in the target
   machine.  BP_TGT->shadow_contents is some memory allocated for
   saving the target contents.  It is guaranteed by the caller to be
   long enough to save BREAKPOINT_LEN bytes (this is accomplished via
   BREAKPOINT_MAX).  */

int
default_memory_insert_breakpoint (struct gdbarch *gdbarch,
				  struct bp_target_info *bp_tgt)
{
  CORE_ADDR addr = bp_tgt->placed_address;
  const unsigned char *bp;
  gdb_byte *readbuf;
  int bplen;
  int val;

  /* Determine appropriate breakpoint contents and size for this address.  */
  bp = gdbarch_sw_breakpoint_from_kind (gdbarch, bp_tgt->kind, &bplen);

  /* Save the memory contents in the shadow_contents buffer and then
     write the breakpoint instruction.  */
  readbuf = (gdb_byte *) alloca (bplen);
  val = target_read_memory (addr, readbuf, bplen);
  if (val == 0)
    {
      /* These must be set together, either before or after the shadow
	 read, so that if we're "reinserting" a breakpoint that
	 doesn't have a shadow yet, the breakpoint masking code inside
	 target_read_memory doesn't mask out this breakpoint using an
	 unfilled shadow buffer.  The core may be trying to reinsert a
	 permanent breakpoint, for targets that support breakpoint
	 conditions/commands on the target side for some types of
	 breakpoints, such as target remote.  */
      bp_tgt->shadow_len = bplen;
      memcpy (bp_tgt->shadow_contents, readbuf, bplen);

      val = target_write_raw_memory (addr, bp, bplen);
    }

  return val;
}


int
default_memory_remove_breakpoint (struct gdbarch *gdbarch,
				  struct bp_target_info *bp_tgt)
{
  int bplen;

  gdbarch_sw_breakpoint_from_kind (gdbarch, bp_tgt->kind, &bplen);

  return target_write_raw_memory (bp_tgt->placed_address, bp_tgt->shadow_contents,
				  bplen);
}


int
memory_insert_breakpoint (struct target_ops *ops, struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt)
{
  return gdbarch_memory_insert_breakpoint (gdbarch, bp_tgt);
}

int
memory_remove_breakpoint (struct target_ops *ops, struct gdbarch *gdbarch,
			  struct bp_target_info *bp_tgt,
			  enum remove_bp_reason reason)
{
  return gdbarch_memory_remove_breakpoint (gdbarch, bp_tgt);
}

int
memory_validate_breakpoint (struct gdbarch *gdbarch,
			    struct bp_target_info *bp_tgt)
{
  CORE_ADDR addr = bp_tgt->placed_address;
  const gdb_byte *bp;
  int val;
  int bplen;
  gdb_byte cur_contents[BREAKPOINT_MAX];

  /* Determine appropriate breakpoint contents and size for this
     address.  */
  bp = gdbarch_breakpoint_from_pc (gdbarch, &addr, &bplen);

  if (bp == NULL)
    return 0;

  /* Make sure we see the memory breakpoints.  */
  scoped_restore restore_memory
    = make_scoped_restore_show_memory_breakpoints (1);
  val = target_read_memory (addr, cur_contents, bplen);

  /* If our breakpoint is no longer at the address, this means that
     the program modified the code on us, so it is wrong to put back
     the old value.  */
  return (val == 0 && memcmp (bp, cur_contents, bplen) == 0);
}
