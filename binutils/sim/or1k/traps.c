/* OpenRISC exception, interrupts, syscall and trap support
   Copyright (C) 2017-2024 Free Software Foundation, Inc.

   This file is part of GDB, the GNU debugger.

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

#define WANT_CPU_OR1K32BF
#define WANT_CPU

#include "sim-main.h"
#include "sim-fpu.h"
#include "sim-signal.h"
#include "cgen-ops.h"

/* Implement the sim invalid instruction function.  This will set the error
   effective address to that of the invalid instruction then call the
   exception handler.  */

SEM_PC
sim_engine_invalid_insn (SIM_CPU *current_cpu, IADDR cia, SEM_PC vpc)
{
  SET_H_SYS_EEAR0 (cia);

#ifdef WANT_CPU_OR1K32BF
  or1k32bf_exception (current_cpu, cia, EXCEPT_ILLEGAL);
#endif

  return vpc;
}

/* Generate the appropriate OpenRISC fpu exception based on the status code from
   the sim fpu.  */
void
or1k32bf_fpu_error (CGEN_FPU* fpu, int status)
{
  SIM_CPU *current_cpu = (SIM_CPU *)fpu->owner;

  /* If floating point exceptions are enabled.  */
  if (GET_H_SYS_FPCSR_FPEE() != 0)
    {
      /* Set all of the status flag bits.  */
      if (status
	  & (sim_fpu_status_invalid_snan
	     | sim_fpu_status_invalid_qnan
	     | sim_fpu_status_invalid_isi
	     | sim_fpu_status_invalid_idi
	     | sim_fpu_status_invalid_zdz
	     | sim_fpu_status_invalid_imz
	     | sim_fpu_status_invalid_cvi
	     | sim_fpu_status_invalid_cmp
	     | sim_fpu_status_invalid_sqrt))
	SET_H_SYS_FPCSR_IVF (1);

      if (status & sim_fpu_status_invalid_snan)
	SET_H_SYS_FPCSR_SNF (1);

      if (status & sim_fpu_status_invalid_qnan)
	SET_H_SYS_FPCSR_QNF (1);

      if (status & sim_fpu_status_overflow)
	SET_H_SYS_FPCSR_OVF (1);

      if (status & sim_fpu_status_underflow)
	SET_H_SYS_FPCSR_UNF (1);

      if (status
	  & (sim_fpu_status_invalid_isi
	     | sim_fpu_status_invalid_idi))
	SET_H_SYS_FPCSR_INF (1);

      if (status & sim_fpu_status_invalid_div0)
	SET_H_SYS_FPCSR_DZF (1);

      if (status & sim_fpu_status_inexact)
	SET_H_SYS_FPCSR_IXF (1);

      /* If any of the exception bits were actually set.  */
      if (GET_H_SYS_FPCSR()
	  & (SPR_FIELD_MASK_SYS_FPCSR_IVF
	     | SPR_FIELD_MASK_SYS_FPCSR_SNF
	     | SPR_FIELD_MASK_SYS_FPCSR_QNF
	     | SPR_FIELD_MASK_SYS_FPCSR_OVF
	     | SPR_FIELD_MASK_SYS_FPCSR_UNF
	     | SPR_FIELD_MASK_SYS_FPCSR_INF
	     | SPR_FIELD_MASK_SYS_FPCSR_DZF
	     | SPR_FIELD_MASK_SYS_FPCSR_IXF))
	{
	  SIM_DESC sd = CPU_STATE (current_cpu);

	  /* If the sim is running in fast mode, i.e. not profiling,
	     per-instruction callbacks are not triggered which would allow
	     us to track the PC.  This means we cannot track which
	     instruction caused the FPU error.  */
	  if (!PROFILE_ANY_P (current_cpu) && !TRACE_ANY_P (current_cpu))
	    sim_io_eprintf
	      (sd, "WARNING: ignoring fpu error caught in fast mode.\n");
	  else
	    or1k32bf_exception (current_cpu, GET_H_SYS_PPC (), EXCEPT_FPE);
	}
    }
}


/* Implement the OpenRISC exception function.  This is mostly used by the
   CGEN generated files.  For example, this is used when handling a
   overflow exception during a multiplication instruction. */

void
or1k32bf_exception (sim_cpu *current_cpu, USI pc, USI exnum)
{
  SIM_DESC sd = CPU_STATE (current_cpu);
  struct or1k_sim_cpu *or1k_cpu = OR1K_SIM_CPU (current_cpu);

  if (exnum == EXCEPT_TRAP)
    {
      /* Trap, used for breakpoints, sends control back to gdb breakpoint
	 handling.  */
      sim_engine_halt (sd, current_cpu, NULL, pc, sim_stopped, SIM_SIGTRAP);
    }
  else
    {
      IADDR handler_pc;

      /* Calculate the exception program counter.  */
      switch (exnum)
	{
	case EXCEPT_RESET:
	  break;

	case EXCEPT_FPE:
	case EXCEPT_SYSCALL:
	  SET_H_SYS_EPCR0 (pc + 4 - (or1k_cpu->delay_slot ? 4 : 0));
	  break;

	case EXCEPT_BUSERR:
	case EXCEPT_ALIGN:
	case EXCEPT_ILLEGAL:
	case EXCEPT_RANGE:
	  SET_H_SYS_EPCR0 (pc - (or1k_cpu->delay_slot ? 4 : 0));
	  break;

	default:
	  sim_io_error (sd, "unexpected exception 0x%x raised at PC 0x%08x",
			exnum, pc);
	  break;
	}

      /* Store the current SR into ESR0.  */
      SET_H_SYS_ESR0 (GET_H_SYS_SR ());

      /* Indicate in SR if the failed instruction is in delay slot or not.  */
      SET_H_SYS_SR_DSX (or1k_cpu->delay_slot);

      or1k_cpu->next_delay_slot = 0;

      /* Jump program counter into handler.  */
      handler_pc =
	(GET_H_SYS_SR_EPH () ? 0xf0000000 : 0x00000000) + (exnum << 8);

      sim_engine_restart (sd, current_cpu, NULL, handler_pc);
    }
}

/* Implement the return from exception instruction.  This is used to return
   the CPU to its previous state from within an exception handler.  */

void
or1k32bf_rfe (sim_cpu *current_cpu)
{
  struct or1k_sim_cpu *or1k_cpu = OR1K_SIM_CPU (current_cpu);

  SET_H_SYS_SR (GET_H_SYS_ESR0 ());
  SET_H_SYS_SR_FO (1);

  or1k_cpu->next_delay_slot = 0;

  sim_engine_restart (CPU_STATE (current_cpu), current_cpu, NULL,
		      GET_H_SYS_EPCR0 ());
}

/* Implement the move from SPR instruction.  This is used to read from the
   CPU's special purpose registers.  */

USI
or1k32bf_mfspr (sim_cpu *current_cpu, USI addr)
{
  SIM_DESC sd = CPU_STATE (current_cpu);
  SI val;

  if (!GET_H_SYS_SR_SM () && !GET_H_SYS_SR_SUMRA ())
    {
      sim_io_eprintf (sd, "WARNING: l.mfspr in user mode (SR 0x%x)\n",
		      GET_H_SYS_SR ());
      return 0;
    }

  if (addr >= NUM_SPR)
    goto bad_address;

  val = GET_H_SPR (addr);

  switch (addr)
    {

    case SPR_ADDR (SYS, VR):
    case SPR_ADDR (SYS, UPR):
    case SPR_ADDR (SYS, CPUCFGR):
    case SPR_ADDR (SYS, SR):
    case SPR_ADDR (SYS, PPC):
    case SPR_ADDR (SYS, FPCSR):
    case SPR_ADDR (SYS, EPCR0):
    case SPR_ADDR (MAC, MACLO):
    case SPR_ADDR (MAC, MACHI):
      break;

    default:
      if (addr < SPR_ADDR (SYS, GPR0) || addr > SPR_ADDR (SYS, GPR511))
	goto bad_address;
      break;

    }

  return val;

bad_address:
  sim_io_eprintf (sd, "WARNING: l.mfspr with invalid SPR address 0x%x\n", addr);
  return 0;

}

/* Implement the move to SPR instruction.  This is used to write too the
   CPU's special purpose registers.  */

void
or1k32bf_mtspr (sim_cpu *current_cpu, USI addr, USI val)
{
  SIM_DESC sd = CPU_STATE (current_cpu);
  struct or1k_sim_cpu *or1k_cpu = OR1K_SIM_CPU (current_cpu);

  if (!GET_H_SYS_SR_SM () && !GET_H_SYS_SR_SUMRA ())
    {
      sim_io_eprintf
	(sd, "WARNING: l.mtspr with address 0x%x in user mode (SR 0x%x)\n",
	 addr, GET_H_SYS_SR ());
      return;
    }

  if (addr >= NUM_SPR)
    goto bad_address;

  switch (addr)
    {

    case SPR_ADDR (SYS, FPCSR):
    case SPR_ADDR (SYS, EPCR0):
    case SPR_ADDR (SYS, ESR0):
    case SPR_ADDR (MAC, MACHI):
    case SPR_ADDR (MAC, MACLO):
      SET_H_SPR (addr, val);
      break;

    case SPR_ADDR (SYS, SR):
      SET_H_SPR (addr, val);
      SET_H_SYS_SR_FO (1);
      break;

    case SPR_ADDR (SYS, NPC):
      or1k_cpu->next_delay_slot = 0;

      sim_engine_restart (sd, current_cpu, NULL, val);
      break;

    case SPR_ADDR (TICK, TTMR):
      /* Allow some registers to be silently cleared.  */
      if (val != 0)
	sim_io_eprintf
	  (sd, "WARNING: l.mtspr to SPR address 0x%x with invalid value 0x%x\n",
	   addr, val);
      break;

    default:
      if (addr >= SPR_ADDR (SYS, GPR0) && addr <= SPR_ADDR (SYS, GPR511))
	SET_H_SPR (addr, val);
      else
	goto bad_address;
      break;

    }

  return;

bad_address:
  sim_io_eprintf (sd, "WARNING: l.mtspr with invalid SPR address 0x%x\n", addr);

}
