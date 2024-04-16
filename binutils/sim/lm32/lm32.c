/* Lattice Mico32 simulator support code.
   Contributed by Jon Beniston <jon@beniston.com>

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

/* This must come before any other includes.  */
#include "defs.h"

#define WANT_CPU lm32bf
#define WANT_CPU_LM32BF

#include "sim-main.h"
#include "cgen-mem.h"
#include "cgen-ops.h"

/* The contents of BUF are in target byte order.  */

int
lm32bf_fetch_register (SIM_CPU * current_cpu, int rn, void *buf, int len)
{
  if (rn < 32)
    SETTSI (buf, lm32bf_h_gr_get (current_cpu, rn));
  else
    switch (rn)
      {
      case SIM_LM32_PC_REGNUM:
	SETTSI (buf, lm32bf_h_pc_get (current_cpu));
	break;
      default:
	return 0;
      }

  return -1;
}

/* The contents of BUF are in target byte order.  */

int
lm32bf_store_register (SIM_CPU * current_cpu, int rn, const void *buf, int len)
{
  if (rn < 32)
    lm32bf_h_gr_set (current_cpu, rn, GETTSI (buf));
  else
    switch (rn)
      {
      case SIM_LM32_PC_REGNUM:
	lm32bf_h_pc_set (current_cpu, GETTSI (buf));
	break;
      default:
	return 0;
      }

  return -1;
}



#if WITH_PROFILE_MODEL_P

/* Initialize cycle counting for an insn.
   FIRST_P is non-zero if this is the first insn in a set of parallel
   insns.  */

void
lm32bf_model_insn_before (SIM_CPU * cpu, int first_p)
{
}

/* Record the cycles computed for an insn.
   LAST_P is non-zero if this is the last insn in a set of parallel insns,
   and we update the total cycle count.
   CYCLES is the cycle count of the insn.  */

void
lm32bf_model_insn_after (SIM_CPU * cpu, int last_p, int cycles)
{
}

int
lm32bf_model_lm32_u_exec (SIM_CPU * cpu, const IDESC * idesc,
			  int unit_num, int referenced)
{
  return idesc->timing->units[unit_num].done;
}

#endif /* WITH_PROFILE_MODEL_P */
