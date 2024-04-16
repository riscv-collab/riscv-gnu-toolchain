/* Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "nat/gdb_ptrace.h"
#include "mips-linux-watch.h"

/* Assuming usable watch registers REGS, return the irw_mask of
   register N.  */

uint32_t
mips_linux_watch_get_irw_mask (struct pt_watch_regs *regs, int n)
{
  switch (regs->style)
    {
    case pt_watch_style_mips32:
      return regs->mips32.watch_masks[n] & IRW_MASK;
    case pt_watch_style_mips64:
      return regs->mips64.watch_masks[n] & IRW_MASK;
    default:
      internal_error (_("Unrecognized watch register style"));
    }
}

/* Assuming usable watch registers REGS, return the reg_mask of
   register N.  */

static uint32_t
get_reg_mask (struct pt_watch_regs *regs, int n)
{
  switch (regs->style)
    {
    case pt_watch_style_mips32:
      return regs->mips32.watch_masks[n] & ~IRW_MASK;
    case pt_watch_style_mips64:
      return regs->mips64.watch_masks[n] & ~IRW_MASK;
    default:
      internal_error (_("Unrecognized watch register style"));
    }
}

/* Assuming usable watch registers REGS, return the num_valid.  */

uint32_t
mips_linux_watch_get_num_valid (struct pt_watch_regs *regs)
{
  switch (regs->style)
    {
    case pt_watch_style_mips32:
      return regs->mips32.num_valid;
    case pt_watch_style_mips64:
      return regs->mips64.num_valid;
    default:
      internal_error (_("Unrecognized watch register style"));
    }
}

/* Assuming usable watch registers REGS, return the watchlo of
   register N.  */

CORE_ADDR
mips_linux_watch_get_watchlo (struct pt_watch_regs *regs, int n)
{
  switch (regs->style)
    {
    case pt_watch_style_mips32:
      return regs->mips32.watchlo[n];
    case pt_watch_style_mips64:
      return regs->mips64.watchlo[n];
    default:
      internal_error (_("Unrecognized watch register style"));
    }
}

/* Assuming usable watch registers REGS, set watchlo of register N to
   VALUE.  */

void
mips_linux_watch_set_watchlo (struct pt_watch_regs *regs, int n,
			      CORE_ADDR value)
{
  switch (regs->style)
    {
    case pt_watch_style_mips32:
      /*  The cast will never throw away bits as 64 bit addresses can
	  never be used on a 32 bit kernel.  */
      regs->mips32.watchlo[n] = (uint32_t) value;
      break;
    case pt_watch_style_mips64:
      regs->mips64.watchlo[n] = value;
      break;
    default:
      internal_error (_("Unrecognized watch register style"));
    }
}

/* Assuming usable watch registers REGS, return the watchhi of
   register N.  */

uint32_t
mips_linux_watch_get_watchhi (struct pt_watch_regs *regs, int n)
{
  switch (regs->style)
    {
    case pt_watch_style_mips32:
      return regs->mips32.watchhi[n];
    case pt_watch_style_mips64:
      return regs->mips64.watchhi[n];
    default:
      internal_error (_("Unrecognized watch register style"));
    }
}

/* Assuming usable watch registers REGS, set watchhi of register N to
   VALUE.  */

void
mips_linux_watch_set_watchhi (struct pt_watch_regs *regs, int n,
			      uint16_t value)
{
  switch (regs->style)
    {
    case pt_watch_style_mips32:
      regs->mips32.watchhi[n] = value;
      break;
    case pt_watch_style_mips64:
      regs->mips64.watchhi[n] = value;
      break;
    default:
      internal_error (_("Unrecognized watch register style"));
    }
}

/* Read the watch registers of process LWPID and store it in
   WATCH_READBACK.  Save true to *WATCH_READBACK_VALID if watch
   registers are valid.  Return 1 if watch registers are usable.
   Cached information is used unless FORCE is true.  */

int
mips_linux_read_watch_registers (long lwpid,
				 struct pt_watch_regs *watch_readback,
				 int *watch_readback_valid, int force)
{
  if (force || *watch_readback_valid == 0)
    {
      if (ptrace (PTRACE_GET_WATCH_REGS, lwpid, watch_readback, NULL) == -1)
	{
	  *watch_readback_valid = -1;
	  return 0;
	}
      switch (watch_readback->style)
	{
	case pt_watch_style_mips32:
	  if (watch_readback->mips32.num_valid == 0)
	    {
	      *watch_readback_valid = -1;
	      return 0;
	    }
	  break;
	case pt_watch_style_mips64:
	  if (watch_readback->mips64.num_valid == 0)
	    {
	      *watch_readback_valid = -1;
	      return 0;
	    }
	  break;
	default:
	  *watch_readback_valid = -1;
	  return 0;
	}
      /* Watch registers appear to be usable.  */
      *watch_readback_valid = 1;
    }
  return (*watch_readback_valid == 1) ? 1 : 0;
}

/* Convert GDB's TYPE to an IRW mask.  */

uint32_t
mips_linux_watch_type_to_irw (enum target_hw_bp_type type)
{
  switch (type)
    {
    case hw_write:
      return W_MASK;
    case hw_read:
      return R_MASK;
    case hw_access:
      return (W_MASK | R_MASK);
    default:
      return 0;
    }
}

/* Set any low order bits in MASK that are not set.  */

static CORE_ADDR
fill_mask (CORE_ADDR mask)
{
  CORE_ADDR f = 1;

  while (f && f < mask)
    {
      mask |= f;
      f <<= 1;
    }
  return mask;
}

/* Try to add a single watch to the specified registers REGS.  The
   address of added watch is ADDR, the length is LEN, and the mask
   is IRW.  Return 1 on success, 0 on failure.  */

int
mips_linux_watch_try_one_watch (struct pt_watch_regs *regs,
				CORE_ADDR addr, int len, uint32_t irw)
{
  CORE_ADDR base_addr, last_byte, break_addr, segment_len;
  CORE_ADDR mask_bits, t_low;
  uint16_t t_hi;
  int i, free_watches;
  struct pt_watch_regs regs_copy;

  if (len <= 0)
    return 0;

  last_byte = addr + len - 1;
  mask_bits = fill_mask (addr ^ last_byte) | IRW_MASK;
  base_addr = addr & ~mask_bits;

  /* Check to see if it is covered by current registers.  */
  for (i = 0; i < mips_linux_watch_get_num_valid (regs); i++)
    {
      t_low = mips_linux_watch_get_watchlo (regs, i);
      if (t_low != 0 && irw == ((uint32_t) t_low & irw))
	{
	  t_hi = mips_linux_watch_get_watchhi (regs, i) | IRW_MASK;
	  t_low &= ~(CORE_ADDR) t_hi;
	  if (addr >= t_low && last_byte <= (t_low + t_hi))
	    return 1;
	}
    }
  /* Try to find an empty register.  */
  free_watches = 0;
  for (i = 0; i < mips_linux_watch_get_num_valid (regs); i++)
    {
      t_low = mips_linux_watch_get_watchlo (regs, i);
      if (t_low == 0
	  && irw == (mips_linux_watch_get_irw_mask (regs, i) & irw))
	{
	  if (mask_bits <= (get_reg_mask (regs, i) | IRW_MASK))
	    {
	      /* It fits, we'll take it.  */
	      mips_linux_watch_set_watchlo (regs, i, base_addr | irw);
	      mips_linux_watch_set_watchhi (regs, i, mask_bits & ~IRW_MASK);
	      return 1;
	    }
	  else
	    {
	      /* It doesn't fit, but has the proper IRW capabilities.  */
	      free_watches++;
	    }
	}
    }
  if (free_watches > 1)
    {
      /* Try to split it across several registers.  */
      regs_copy = *regs;
      for (i = 0; i < mips_linux_watch_get_num_valid (&regs_copy); i++)
	{
	  t_low = mips_linux_watch_get_watchlo (&regs_copy, i);
	  t_hi = get_reg_mask (&regs_copy, i) | IRW_MASK;
	  if (t_low == 0 && irw == (t_hi & irw))
	    {
	      t_low = addr & ~(CORE_ADDR) t_hi;
	      break_addr = t_low + t_hi + 1;
	      if (break_addr >= addr + len)
		segment_len = len;
	      else
		segment_len = break_addr - addr;
	      mask_bits = fill_mask (addr ^ (addr + segment_len - 1));
	      mips_linux_watch_set_watchlo (&regs_copy, i,
					    (addr & ~mask_bits) | irw);
	      mips_linux_watch_set_watchhi (&regs_copy, i,
					    mask_bits & ~IRW_MASK);
	      if (break_addr >= addr + len)
		{
		  *regs = regs_copy;
		  return 1;
		}
	      len = addr + len - break_addr;
	      addr = break_addr;
	    }
	}
    }
  /* It didn't fit anywhere, we failed.  */
  return 0;
}

/* Fill in the watch registers REGS with the currently cached
   watches CURRENT_WATCHES.  */

void
mips_linux_watch_populate_regs (struct mips_watchpoint *current_watches,
				struct pt_watch_regs *regs)
{
  struct mips_watchpoint *w;
  int i;

  /* Clear them out.  */
  for (i = 0; i < mips_linux_watch_get_num_valid (regs); i++)
    {
      mips_linux_watch_set_watchlo (regs, i, 0);
      mips_linux_watch_set_watchhi (regs, i, 0);
    }

  w = current_watches;
  while (w)
    {
      uint32_t irw = mips_linux_watch_type_to_irw (w->type);

      i = mips_linux_watch_try_one_watch (regs, w->addr, w->len, irw);
      /* They must all fit, because we previously calculated that they
	 would.  */
      gdb_assert (i);
      w = w->next;
    }
}
