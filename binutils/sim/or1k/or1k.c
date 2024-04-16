/* OpenRISC simulator support code
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
#include "symcat.h"
#include "cgen-ops.h"
#include "cgen-mem.h"
#include "cpuall.h"

#include <string.h>

int
or1k32bf_fetch_register (sim_cpu *current_cpu, int rn, void *buf, int len)
{
  if (rn < 32)
    SETTWI (buf, GET_H_GPR (rn));
  else
    switch (rn)
      {
      case PPC_REGNUM:
	SETTWI (buf, GET_H_SYS_PPC ());
	break;
      case PC_REGNUM:
	SETTWI (buf, GET_H_PC ());
	break;
      case SR_REGNUM:
	SETTWI (buf, GET_H_SYS_SR ());
	break;
      default:
	return 0;
      }
  return sizeof (WI);		/* WI from arch.h */
}

int
or1k32bf_store_register (sim_cpu *current_cpu, int rn, const void *buf, int len)
{
  if (rn < 32)
    SET_H_GPR (rn, GETTWI (buf));
  else
    switch (rn)
      {
      case PPC_REGNUM:
	SET_H_SYS_PPC (GETTWI (buf));
	break;
      case PC_REGNUM:
	SET_H_PC (GETTWI (buf));
	break;
      case SR_REGNUM:
	SET_H_SYS_SR (GETTWI (buf));
	break;
      default:
	return 0;
      }
  return sizeof (WI);		/* WI from arch.h */
}

int
or1k32bf_model_or1200_u_exec (sim_cpu *current_cpu, const IDESC *idesc,
			      int unit_num, int referenced)
{
  return -1;
}

int
or1k32bf_model_or1200nd_u_exec (sim_cpu *current_cpu, const IDESC *idesc,
				int unit_num, int referenced)
{
  return -1;
}

void
or1k32bf_model_insn_before (sim_cpu *current_cpu, int first_p)
{
}

void
or1k32bf_model_insn_after (sim_cpu *current_cpu, int last_p, int cycles)
{
}

USI
or1k32bf_h_spr_get_raw (sim_cpu *current_cpu, USI addr)
{
  SIM_DESC sd = CPU_STATE (current_cpu);
  struct or1k_sim_cpu *or1k_cpu = OR1K_SIM_CPU (current_cpu);

  SIM_ASSERT (addr < NUM_SPR);
  return or1k_cpu->spr[addr];
}

void
or1k32bf_h_spr_set_raw (sim_cpu *current_cpu, USI addr, USI val)
{
  SIM_DESC sd = CPU_STATE (current_cpu);
  struct or1k_sim_cpu *or1k_cpu = OR1K_SIM_CPU (current_cpu);

  SIM_ASSERT (addr < NUM_SPR);
  or1k_cpu->spr[addr] = val;
}

USI
or1k32bf_h_spr_field_get_raw (sim_cpu *current_cpu, USI addr, int msb, int lsb)
{
  SIM_DESC sd = CPU_STATE (current_cpu);
  struct or1k_sim_cpu *or1k_cpu = OR1K_SIM_CPU (current_cpu);

  SIM_ASSERT (addr < NUM_SPR);
  return LSEXTRACTED (or1k_cpu->spr[addr], msb, lsb);
}

void
or1k32bf_h_spr_field_set_raw (sim_cpu *current_cpu, USI addr, int msb, int lsb,
			      USI val)
{
  struct or1k_sim_cpu *or1k_cpu = OR1K_SIM_CPU (current_cpu);

  or1k_cpu->spr[addr] &= ~LSMASK32 (msb, lsb);
  or1k_cpu->spr[addr] |= LSINSERTED (val, msb, lsb);
}

/* Initialize a sim cpu object.  */
void
or1k_cpu_init (SIM_DESC sd, sim_cpu *current_cpu, const USI or1k_vr,
	       const USI or1k_upr, const USI or1k_cpucfgr)
{
  struct or1k_sim_cpu *or1k_cpu = OR1K_SIM_CPU (current_cpu);

  /* Set the configuration registers passed from the user.  */
  SET_H_SYS_VR (or1k_vr);
  SET_H_SYS_UPR (or1k_upr);
  SET_H_SYS_CPUCFGR (or1k_cpucfgr);

#define CHECK_SPR_FIELD(GROUP, INDEX, FIELD, test) \
  do \
    { \
      USI field = GET_H_##SYS##_##INDEX##_##FIELD (); \
      if (!(test)) \
	sim_io_eprintf \
	  (sd, "WARNING: unsupported %s field in %s register: 0x%x\n", \
	   #FIELD, #INDEX, field); \
    } while (0)

  /* Set flags indicating if we are in a delay slot or not.  */
  or1k_cpu->next_delay_slot = 0;
  or1k_cpu->delay_slot = 0;

  /* Verify any user passed fields and warn on configurations we don't
     support.  */
  CHECK_SPR_FIELD (SYS, UPR, UP, field == 1);
  CHECK_SPR_FIELD (SYS, UPR, DCP, field == 0);
  CHECK_SPR_FIELD (SYS, UPR, ICP, field == 0);
  CHECK_SPR_FIELD (SYS, UPR, DMP, field == 0);
  CHECK_SPR_FIELD (SYS, UPR, MP, field == 0);
  CHECK_SPR_FIELD (SYS, UPR, IMP, field == 0);
  CHECK_SPR_FIELD (SYS, UPR, DUP, field == 0);
  CHECK_SPR_FIELD (SYS, UPR, PCUP, field == 0);
  CHECK_SPR_FIELD (SYS, UPR, PICP, field == 0);
  CHECK_SPR_FIELD (SYS, UPR, PMP, field == 0);
  CHECK_SPR_FIELD (SYS, UPR, TTP, field == 0);
  CHECK_SPR_FIELD (SYS, UPR, CUP, field == 0);

  CHECK_SPR_FIELD (SYS, CPUCFGR, NSGR, field == 0);
  CHECK_SPR_FIELD (SYS, CPUCFGR, CGF, field == 0);
  CHECK_SPR_FIELD (SYS, CPUCFGR, OB32S, field == 1);
  CHECK_SPR_FIELD (SYS, CPUCFGR, OF32S, field == 1);
  CHECK_SPR_FIELD (SYS, CPUCFGR, OB64S, field == 0);
  CHECK_SPR_FIELD (SYS, CPUCFGR, OF64S, field == 0);
  CHECK_SPR_FIELD (SYS, CPUCFGR, OV64S, field == 0);

#undef CHECK_SPR_FIELD

  /* Configure the fpu operations and mark fpu available.  */
  cgen_init_accurate_fpu (current_cpu, CGEN_CPU_FPU (current_cpu),
			  or1k32bf_fpu_error);
  SET_H_SYS_CPUCFGR_OF32S (1);

  /* Set the UPR[UP] flag, even if the user tried to unset it, as we always
     support the Unit Present Register.  */
  SET_H_SYS_UPR_UP (1);

  /* Set the supervisor register to indicate we are in supervisor mode and
     set the Fixed-One bit which must always be set.  */
  SET_H_SYS_SR (SPR_FIELD_MASK_SYS_SR_SM | SPR_FIELD_MASK_SYS_SR_FO);

  /* Clear the floating point control status register.  */
  SET_H_SYS_FPCSR (0);
}

void
or1k32bf_insn_before (sim_cpu *current_cpu, SEM_PC vpc, const IDESC *idesc)
{
  SIM_DESC sd = CPU_STATE (current_cpu);
  struct or1k_sim_cpu *or1k_cpu = OR1K_SIM_CPU (current_cpu);

  or1k_cpu->delay_slot = or1k_cpu->next_delay_slot;
  or1k_cpu->next_delay_slot = 0;

  if (or1k_cpu->delay_slot &&
      CGEN_ATTR_BOOLS (CGEN_INSN_ATTRS ((idesc)->idata)) &
      CGEN_ATTR_MASK (CGEN_INSN_NOT_IN_DELAY_SLOT))
    {
      USI pc;
#ifdef WITH_SCACHE
      pc = vpc->argbuf.addr;
#else
      pc = vpc;
#endif
      sim_io_error (sd, "invalid instruction in a delay slot at PC 0x%08x",
		    pc);
    }

}

void
or1k32bf_insn_after (sim_cpu *current_cpu, SEM_PC vpc, const IDESC *idesc)
{
  SIM_DESC sd = CPU_STATE (current_cpu);
  struct or1k_sim_cpu *or1k_cpu = OR1K_SIM_CPU (current_cpu);
  USI ppc;

#ifdef WITH_SCACHE
  ppc = vpc->argbuf.addr;
#else
  ppc = vpc;
#endif

  SET_H_SYS_PPC (ppc);

  if (!GET_H_SYS_CPUCFGR_ND () &&
      CGEN_ATTR_BOOLS (CGEN_INSN_ATTRS ((idesc)->idata)) &
      CGEN_ATTR_MASK (CGEN_INSN_DELAYED_CTI))
    {
      SIM_ASSERT (!or1k_cpu->delay_slot);
      or1k_cpu->next_delay_slot = 1;
    }
}

void
or1k32bf_nop (sim_cpu *current_cpu, USI uimm16)
{
  SIM_DESC sd = CPU_STATE (current_cpu);

  switch (uimm16)
    {

    case NOP_NOP:
      break;

    case NOP_EXIT:
      sim_io_printf (CPU_STATE (current_cpu), "exit(%d)\n", GET_H_GPR (3));
      ATTRIBUTE_FALLTHROUGH;
    case NOP_EXIT_SILENT:
      sim_engine_halt (sd, current_cpu, NULL, CPU_PC_GET (current_cpu),
		       sim_exited, GET_H_GPR (3));
      break;

    case NOP_REPORT:
      sim_io_printf (CPU_STATE (current_cpu), "report(0x%08x);\n",
		     GET_H_GPR (3));
      break;

    case NOP_PUTC:
      sim_io_printf (CPU_STATE (current_cpu), "%c",
		     (char) (GET_H_GPR (3) & 0xff));
      break;

    default:
      sim_io_eprintf (sd, "WARNING: l.nop with unsupported code 0x%08x\n",
		      uimm16);
      break;
    }

}

/* Build an address value used for load and store instructions.  For example,
   the instruction 'l.lws rD, I(rA)' will require to load data from the 4 byte
   address represented by rA + I.  Here the argument base is rA, offset is I
   and the size is the read size in bytes.  Note, OpenRISC requires that word
   and half-word access be word and half-word aligned respectively, the check
   for alignment is not needed here.  */

USI
or1k32bf_make_load_store_addr (sim_cpu *current_cpu, USI base, SI offset,
			       int size)
{
  SIM_DESC sd = CPU_STATE (current_cpu);

  USI addr = base + offset;

  /* If little endian load/store is enabled we adjust the byte and half-word
     addresses to the little endian equivalent.  */
  if (GET_H_SYS_SR_LEE ())
    {
      switch (size)
	{

	case 4: /* We are retrieving the entire word no adjustment.  */
	  break;

	case 2: /* Perform half-word adjustment 0 -> 2, 2 -> 0.  */
	  addr ^= 0x2;
	  break;

	case 1: /* Perform byte adjustment, 0 -> 3, 2 -> 3, etc.  */
	  addr ^= 0x3;
	  break;

	default:
	  SIM_ASSERT (0);
	  return 0;
	}
    }

  return addr;
}

/* The find first 1 instruction returns the location of the first set bit
   in the argument register.  */

USI
or1k32bf_ff1 (sim_cpu *current_cpu, USI val)
{
  USI bit;
  USI ret;
  for (bit = 1, ret = 1; bit; bit <<= 1, ret++)
    {
      if (val & bit)
	return ret;
    }
  return 0;
}

/* The find last 1 instruction returns the location of the last set bit in
   the argument register.  */

USI
or1k32bf_fl1 (sim_cpu *current_cpu, USI val)
{
  USI bit;
  USI ret;
  for (bit = 1 << 31, ret = 32; bit; bit >>= 1, ret--)
    {
      if (val & bit)
	return ret;
    }
  return 0;
}
