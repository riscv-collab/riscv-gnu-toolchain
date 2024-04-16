/* Lattice Mico32 exception and system call support.
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
#include "sim-signal.h"
#include "sim-syscall.h"
#include "lm32-sim.h"
#include "target-newlib-syscall.h"

/* Handle invalid instructions.  */

SEM_PC
sim_engine_invalid_insn (SIM_CPU * current_cpu, IADDR cia, SEM_PC pc)
{
  SIM_DESC sd = CPU_STATE (current_cpu);

  sim_engine_halt (sd, current_cpu, NULL, cia, sim_stopped, SIM_SIGILL);

  return pc;
}

/* Handle divide instructions. */

USI
lm32bf_divu_insn (SIM_CPU * current_cpu, IADDR pc, USI r0, USI r1, USI r2)
{
  SIM_DESC sd = CPU_STATE (current_cpu);

  /* Check for divide by zero */
  if (GET_H_GR (r1) == 0)
    {
      if (STATE_ENVIRONMENT (sd) != OPERATING_ENVIRONMENT)
	sim_engine_halt (sd, current_cpu, NULL, pc, sim_stopped, SIM_SIGFPE);
      else
	{
	  /* Save PC in exception address register.  */
	  SET_H_GR (30, pc);
	  /* Save and clear interrupt enable.  */
	  SET_H_CSR (LM32_CSR_IE, (GET_H_CSR (LM32_CSR_IE) & 1) << 1);
	  /* Branch to divide by zero exception handler.  */
	  return GET_H_CSR (LM32_CSR_EBA) + LM32_EID_DIVIDE_BY_ZERO * 32;
	}
    }
  else
    {
      SET_H_GR (r2, (USI) GET_H_GR (r0) / (USI) GET_H_GR (r1));
      return pc + 4;
    }
}

USI
lm32bf_modu_insn (SIM_CPU * current_cpu, IADDR pc, USI r0, USI r1, USI r2)
{
  SIM_DESC sd = CPU_STATE (current_cpu);

  /* Check for divide by zero.  */
  if (GET_H_GR (r1) == 0)
    {
      if (STATE_ENVIRONMENT (sd) != OPERATING_ENVIRONMENT)
	sim_engine_halt (sd, current_cpu, NULL, pc, sim_stopped, SIM_SIGFPE);
      else
	{
	  /* Save PC in exception address register.  */
	  SET_H_GR (30, pc);
	  /* Save and clear interrupt enable.  */
	  SET_H_CSR (LM32_CSR_IE, (GET_H_CSR (LM32_CSR_IE) & 1) << 1);
	  /* Branch to divide by zero exception handler.  */
	  return GET_H_CSR (LM32_CSR_EBA) + LM32_EID_DIVIDE_BY_ZERO * 32;
	}
    }
  else
    {
      SET_H_GR (r2, (USI) GET_H_GR (r0) % (USI) GET_H_GR (r1));
      return pc + 4;
    }
}

/* Handle break instructions.  */

USI
lm32bf_break_insn (SIM_CPU * current_cpu, IADDR pc)
{
  SIM_DESC sd = CPU_STATE (current_cpu);

  /* Breakpoint.  */
  if (STATE_ENVIRONMENT (sd) != OPERATING_ENVIRONMENT)
    {
      sim_engine_halt (sd, current_cpu, NULL, pc, sim_stopped, SIM_SIGTRAP);
      return pc;
    }
  else
    {
      /* Save PC in breakpoint address register.  */
      SET_H_GR (31, pc);
      /* Save and clear interrupt enable.  */
      SET_H_CSR (LM32_CSR_IE, (GET_H_CSR (LM32_CSR_IE) & 1) << 2);
      /* Branch to breakpoint exception handler.  */
      return GET_H_CSR (LM32_CSR_DEBA) + LM32_EID_BREAKPOINT * 32;
    }
}

/* Handle scall instructions.  */

USI
lm32bf_scall_insn (SIM_CPU * current_cpu, IADDR pc)
{
  SIM_DESC sd = CPU_STATE (current_cpu);

  if ((STATE_ENVIRONMENT (sd) != OPERATING_ENVIRONMENT)
      || (GET_H_GR (8) == TARGET_NEWLIB_SYS_exit))
    {
      /* Delegate system call to host O/S.  */
      long result, result2;
      int errcode;

      /* Perform the system call.  */
      sim_syscall_multi (current_cpu, GET_H_GR (8), GET_H_GR (1), GET_H_GR (2),
			 GET_H_GR (3), GET_H_GR (4), &result, &result2,
			 &errcode);
      /* Store the return value in the CPU's registers.  */
      SET_H_GR (1, result);
      SET_H_GR (2, result2);
      SET_H_GR (3, errcode);

      /* Skip over scall instruction.  */
      return pc + 4;
    }
  else
    {
      /* Save PC in exception address register.  */
      SET_H_GR (30, pc);
      /* Save and clear interrupt enable */
      SET_H_CSR (LM32_CSR_IE, (GET_H_CSR (LM32_CSR_IE) & 1) << 1);
      /* Branch to system call exception handler.  */
      return GET_H_CSR (LM32_CSR_EBA) + LM32_EID_SYSTEM_CALL * 32;
    }
}

/* Handle b instructions.  */

USI
lm32bf_b_insn (SIM_CPU * current_cpu, USI r0, USI f_r0)
{
  /* Restore interrupt enable.  */
  if (f_r0 == 30)
    SET_H_CSR (LM32_CSR_IE, (GET_H_CSR (LM32_CSR_IE) & 2) >> 1);
  else if (f_r0 == 31)
    SET_H_CSR (LM32_CSR_IE, (GET_H_CSR (LM32_CSR_IE) & 4) >> 2);
  return r0;
}

/* Handle wcsr instructions.  */

void
lm32bf_wcsr_insn (SIM_CPU * current_cpu, USI f_csr, USI r1)
{
  /* Writing a 1 to IP CSR clears a bit, writing 0 has no effect.  */
  if (f_csr == LM32_CSR_IP)
    SET_H_CSR (f_csr, GET_H_CSR (f_csr) & ~r1);
  else
    SET_H_CSR (f_csr, r1);
}

/* Handle signals.  */

void
lm32_core_signal (SIM_DESC sd,
		  sim_cpu * cpu,
		  sim_cia cia,
		  unsigned map,
		  int nr_bytes,
		  address_word addr,
		  transfer_type transfer, sim_core_signals sig)
{
  const char *copy = (transfer == read_transfer ? "read" : "write");
  address_word ip = CIA_ADDR (cia);
  SIM_CPU *current_cpu = cpu;

  switch (sig)
    {
    case sim_core_unmapped_signal:
      sim_io_eprintf (sd,
		      "core: %d byte %s to unmapped address 0x%lx at 0x%lx\n",
		      nr_bytes, copy, (unsigned long) addr,
		      (unsigned long) ip);
      SET_H_GR (30, ip);
      /* Save and clear interrupt enable.  */
      SET_H_CSR (LM32_CSR_IE, (GET_H_CSR (LM32_CSR_IE) & 1) << 1);
      CPU_PC_SET (cpu, GET_H_CSR (LM32_CSR_EBA) + LM32_EID_DATA_BUS_ERROR * 32);
      sim_engine_halt (sd, cpu, NULL, LM32_EID_DATA_BUS_ERROR * 32,
		       sim_stopped, SIM_SIGSEGV);
      break;
    case sim_core_unaligned_signal:
      sim_io_eprintf (sd,
		      "core: %d byte misaligned %s to address 0x%lx at 0x%lx\n",
		      nr_bytes, copy, (unsigned long) addr,
		      (unsigned long) ip);
      SET_H_GR (30, ip);
      /* Save and clear interrupt enable.  */
      SET_H_CSR (LM32_CSR_IE, (GET_H_CSR (LM32_CSR_IE) & 1) << 1);
      CPU_PC_SET (cpu, GET_H_CSR (LM32_CSR_EBA) + LM32_EID_DATA_BUS_ERROR * 32);
      sim_engine_halt (sd, cpu, NULL, LM32_EID_DATA_BUS_ERROR * 32,
		       sim_stopped, SIM_SIGBUS);
      break;
    default:
      sim_engine_abort (sd, cpu, cia,
			"sim_core_signal - internal error - bad switch");
    }
}
