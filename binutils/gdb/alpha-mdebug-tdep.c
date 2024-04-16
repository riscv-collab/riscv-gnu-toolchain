/* Target-dependent mdebug code for the ALPHA architecture.
   Copyright (C) 1993-2024 Free Software Foundation, Inc.

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
#include "frame-unwind.h"
#include "frame-base.h"
#include "symtab.h"
#include "gdbcore.h"
#include "block.h"
#include "trad-frame.h"

#include "alpha-tdep.h"
#include "mdebugread.h"
#include "gdbarch.h"

/* FIXME: Some of this code should perhaps be merged with mips.  */

/* Layout of a stack frame on the alpha:

		|				|
 pdr members:	|  7th ... nth arg,		|
		|  `pushed' by caller.		|
		|				|
----------------|-------------------------------|<--  old_sp == vfp
   ^  ^  ^  ^	|				|
   |  |  |  |	|				|
   |  |localoff	|  Copies of 1st .. 6th		|
   |  |  |  |	|  argument if necessary.	|
   |  |  |  v	|				|
   |  |  |  ---	|-------------------------------|<-- LOCALS_ADDRESS
   |  |  |      |				|
   |  |  |      |  Locals and temporaries.	|
   |  |  |      |				|
   |  |  |      |-------------------------------|
   |  |  |      |				|
   |-fregoffset	|  Saved float registers.	|
   |  |  |      |  F9				|
   |  |  |      |   .				|
   |  |  |      |   .				|
   |  |  |      |  F2				|
   |  |  v      |				|
   |  |  -------|-------------------------------|
   |  |         |				|
   |  |         |  Saved registers.		|
   |  |         |  S6				|
   |-regoffset	|   .				|
   |  |         |   .				|
   |  |         |  S0				|
   |  |         |  pdr.pcreg			|
   |  v         |				|
   |  ----------|-------------------------------|
   |            |				|
 frameoffset    |  Argument build area, gets	|
   |            |  7th ... nth arg for any	|
   |            |  called procedure.		|
   v            |  				|
   -------------|-------------------------------|<-- sp
		|				|
*/

#define PROC_LOW_ADDR(proc) ((proc)->pdr.adr)
#define PROC_FRAME_OFFSET(proc) ((proc)->pdr.frameoffset)
#define PROC_FRAME_REG(proc) ((proc)->pdr.framereg)
#define PROC_REG_MASK(proc) ((proc)->pdr.regmask)
#define PROC_FREG_MASK(proc) ((proc)->pdr.fregmask)
#define PROC_REG_OFFSET(proc) ((proc)->pdr.regoffset)
#define PROC_FREG_OFFSET(proc) ((proc)->pdr.fregoffset)
#define PROC_PC_REG(proc) ((proc)->pdr.pcreg)
#define PROC_LOCALOFF(proc) ((proc)->pdr.localoff)

/* Locate the mdebug PDR for the given PC.  Return null if one can't
   be found; you'll have to fall back to other methods in that case.  */

static struct mdebug_extra_func_info *
find_proc_desc (CORE_ADDR pc)
{
  const struct block *b = block_for_pc (pc);
  struct mdebug_extra_func_info *proc_desc = NULL;
  struct symbol *sym = NULL;
  const char *sh_name = NULL;

  if (b)
    {
      CORE_ADDR startaddr;
      find_pc_partial_function (pc, &sh_name, &startaddr, NULL);

      if (startaddr > b->start ())
	/* This is the "pathological" case referred to in a comment in
	   print_frame_info.  It might be better to move this check into
	   symbol reading.  */
	sym = NULL;
      else
	sym = lookup_symbol (MDEBUG_EFI_SYMBOL_NAME, b, LABEL_DOMAIN,
			     0).symbol;
    }

  if (sym)
    {
      proc_desc = (struct mdebug_extra_func_info *) sym->value_bytes ();

      /* Correct incorrect setjmp procedure descriptor from the library
	 to make backtrace through setjmp work.  */
      if (proc_desc->pdr.pcreg == 0
	  && strcmp (sh_name, "setjmp") == 0)
	{
	  proc_desc->pdr.pcreg = ALPHA_RA_REGNUM;
	  proc_desc->pdr.regmask = 0x80000000;
	  proc_desc->pdr.regoffset = -4;
	}

      /* If we never found a PDR for this function in symbol reading,
	 then examine prologues to find the information.  */
      if (proc_desc->pdr.framereg == -1)
	proc_desc = NULL;
    }

  return proc_desc;
}

/* Return a non-zero result if the function is frameless; zero otherwise.  */

static int
alpha_mdebug_frameless (struct mdebug_extra_func_info *proc_desc)
{
  return (PROC_FRAME_REG (proc_desc) == ALPHA_SP_REGNUM
	  && PROC_FRAME_OFFSET (proc_desc) == 0);
}

/* This returns the PC of the first inst after the prologue.  If we can't
   find the prologue, then return 0.  */

static CORE_ADDR
alpha_mdebug_after_prologue (CORE_ADDR pc,
			     struct mdebug_extra_func_info *proc_desc)
{
  if (proc_desc)
    {
      /* If function is frameless, then we need to do it the hard way.  I
	 strongly suspect that frameless always means prologueless...  */
      if (alpha_mdebug_frameless (proc_desc))
	return 0;
    }

  return alpha_after_prologue (pc);
}

/* Return non-zero if we *might* be in a function prologue.  Return zero
   if we are definitively *not* in a function prologue.  */

static int
alpha_mdebug_in_prologue (CORE_ADDR pc,
			  struct mdebug_extra_func_info *proc_desc)
{
  CORE_ADDR after_prologue_pc = alpha_mdebug_after_prologue (pc, proc_desc);
  return (after_prologue_pc == 0 || pc < after_prologue_pc);
}


/* Frame unwinder that reads mdebug PDRs.  */

struct alpha_mdebug_unwind_cache
{
  struct mdebug_extra_func_info *proc_desc;
  CORE_ADDR vfp;
  trad_frame_saved_reg *saved_regs;
};

/* Extract all of the information about the frame from PROC_DESC
   and store the resulting register save locations in the structure.  */

static struct alpha_mdebug_unwind_cache *
alpha_mdebug_frame_unwind_cache (frame_info_ptr this_frame, 
				 void **this_prologue_cache)
{
  struct alpha_mdebug_unwind_cache *info;
  struct mdebug_extra_func_info *proc_desc;
  ULONGEST vfp;
  CORE_ADDR pc, reg_position;
  unsigned long mask;
  int ireg, returnreg;

  if (*this_prologue_cache)
    return (struct alpha_mdebug_unwind_cache *) *this_prologue_cache;

  info = FRAME_OBSTACK_ZALLOC (struct alpha_mdebug_unwind_cache);
  *this_prologue_cache = info;
  pc = get_frame_address_in_block (this_frame);

  /* ??? We don't seem to be able to cache the lookup of the PDR
     from alpha_mdebug_frame_p.  It'd be nice if we could change
     the arguments to that function.  Oh well.  */
  proc_desc = find_proc_desc (pc);
  info->proc_desc = proc_desc;
  gdb_assert (proc_desc != NULL);

  info->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* The VFP of the frame is at FRAME_REG+FRAME_OFFSET.  */
  vfp = get_frame_register_unsigned (this_frame, PROC_FRAME_REG (proc_desc));
  vfp += PROC_FRAME_OFFSET (info->proc_desc);
  info->vfp = vfp;

  /* Fill in the offsets for the registers which gen_mask says were saved.  */

  reg_position = vfp + PROC_REG_OFFSET (proc_desc);
  mask = PROC_REG_MASK (proc_desc);
  returnreg = PROC_PC_REG (proc_desc);

  /* Note that RA is always saved first, regardless of its actual
     register number.  */
  if (mask & (1 << returnreg))
    {
      /* Clear bit for RA so we don't save it again later.  */
      mask &= ~(1 << returnreg);

      info->saved_regs[returnreg].set_addr (reg_position);
      reg_position += 8;
    }

  for (ireg = 0; ireg <= 31; ++ireg)
    if (mask & (1 << ireg))
      {
	info->saved_regs[ireg].set_addr (reg_position);
	reg_position += 8;
      }

  reg_position = vfp + PROC_FREG_OFFSET (proc_desc);
  mask = PROC_FREG_MASK (proc_desc);

  for (ireg = 0; ireg <= 31; ++ireg)
    if (mask & (1 << ireg))
      {
	info->saved_regs[ALPHA_FP0_REGNUM + ireg].set_addr (reg_position);
	reg_position += 8;
      }

  /* The stack pointer of the previous frame is computed by popping
     the current stack frame.  */
  if (!info->saved_regs[ALPHA_SP_REGNUM].is_addr ())
    info->saved_regs[ALPHA_SP_REGNUM].set_value (vfp);

  return info;
}

/* Given a GDB frame, determine the address of the calling function's
   frame.  This will be used to create a new GDB frame struct.  */

static void
alpha_mdebug_frame_this_id (frame_info_ptr this_frame,
			    void **this_prologue_cache,
			    struct frame_id *this_id)
{
  struct alpha_mdebug_unwind_cache *info
    = alpha_mdebug_frame_unwind_cache (this_frame, this_prologue_cache);

  *this_id = frame_id_build (info->vfp, get_frame_func (this_frame));
}

/* Retrieve the value of REGNUM in FRAME.  Don't give up!  */

static struct value *
alpha_mdebug_frame_prev_register (frame_info_ptr this_frame,
				  void **this_prologue_cache, int regnum)
{
  struct alpha_mdebug_unwind_cache *info
    = alpha_mdebug_frame_unwind_cache (this_frame, this_prologue_cache);

  /* The PC of the previous frame is stored in the link register of
     the current frame.  Frob regnum so that we pull the value from
     the correct place.  */
  if (regnum == ALPHA_PC_REGNUM)
    regnum = PROC_PC_REG (info->proc_desc);
  
  return trad_frame_get_prev_register (this_frame, info->saved_regs, regnum);
}

/* Return a non-zero result if the size of the stack frame exceeds the
   maximum debuggable frame size (512 Kbytes); zero otherwise.  */

static int
alpha_mdebug_max_frame_size_exceeded (struct mdebug_extra_func_info *proc_desc)
{
  /* If frame offset is null, we can be in two cases: either the
     function is frameless (the stack frame is null) or its
     frame exceeds the maximum debuggable frame size (512 Kbytes).  */

  return (PROC_FRAME_OFFSET (proc_desc) == 0
	  && !alpha_mdebug_frameless (proc_desc));
}

static int
alpha_mdebug_frame_sniffer (const struct frame_unwind *self,
			    frame_info_ptr this_frame,
			    void **this_cache)
{
  CORE_ADDR pc = get_frame_address_in_block (this_frame);
  struct mdebug_extra_func_info *proc_desc;

  /* If this PC does not map to a PDR, then clearly this isn't an
     mdebug frame.  */
  proc_desc = find_proc_desc (pc);
  if (proc_desc == NULL)
    return 0;

  /* If we're in the prologue, the PDR for this frame is not yet valid.
     Say no here and we'll fall back on the heuristic unwinder.  */
  if (alpha_mdebug_in_prologue (pc, proc_desc))
    return 0;

  /* If the maximum debuggable frame size has been exceeded, the
     proc desc is bogus.  Fall back on the heuristic unwinder.  */
  if (alpha_mdebug_max_frame_size_exceeded (proc_desc))
    return 0;

  return 1;
}

static const struct frame_unwind alpha_mdebug_frame_unwind =
{
  "alpha mdebug",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  alpha_mdebug_frame_this_id,
  alpha_mdebug_frame_prev_register,
  NULL,
  alpha_mdebug_frame_sniffer
};

static CORE_ADDR
alpha_mdebug_frame_base_address (frame_info_ptr this_frame,
				 void **this_prologue_cache)
{
  struct alpha_mdebug_unwind_cache *info
    = alpha_mdebug_frame_unwind_cache (this_frame, this_prologue_cache);

  return info->vfp;
}

static CORE_ADDR
alpha_mdebug_frame_locals_address (frame_info_ptr this_frame,
				   void **this_prologue_cache)
{
  struct alpha_mdebug_unwind_cache *info
    = alpha_mdebug_frame_unwind_cache (this_frame, this_prologue_cache);

  return info->vfp - PROC_LOCALOFF (info->proc_desc);
}

static CORE_ADDR
alpha_mdebug_frame_args_address (frame_info_ptr this_frame,
				 void **this_prologue_cache)
{
  struct alpha_mdebug_unwind_cache *info
    = alpha_mdebug_frame_unwind_cache (this_frame, this_prologue_cache);

  return info->vfp - ALPHA_NUM_ARG_REGS * 8;
}

static const struct frame_base alpha_mdebug_frame_base = {
  &alpha_mdebug_frame_unwind,
  alpha_mdebug_frame_base_address,
  alpha_mdebug_frame_locals_address,
  alpha_mdebug_frame_args_address
};

static const struct frame_base *
alpha_mdebug_frame_base_sniffer (frame_info_ptr this_frame)
{
  CORE_ADDR pc = get_frame_address_in_block (this_frame);
  struct mdebug_extra_func_info *proc_desc;

  /* If this PC does not map to a PDR, then clearly this isn't an
     mdebug frame.  */
  proc_desc = find_proc_desc (pc);
  if (proc_desc == NULL)
    return NULL;

  /* If the maximum debuggable frame size has been exceeded, the
     proc desc is bogus.  Fall back on the heuristic unwinder.  */
  if (alpha_mdebug_max_frame_size_exceeded (proc_desc))
    return 0;

  return &alpha_mdebug_frame_base;
}


void
alpha_mdebug_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  frame_unwind_append_unwinder (gdbarch, &alpha_mdebug_frame_unwind);
  frame_base_append_sniffer (gdbarch, alpha_mdebug_frame_base_sniffer);
}
