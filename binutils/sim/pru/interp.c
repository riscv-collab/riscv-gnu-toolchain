/* Simulator for the Texas Instruments PRU processor
   Copyright 2009-2024 Free Software Foundation, Inc.
   Inspired by the Microblaze simulator
   Contributed by Dimitar Dimitrov <dimitar@dinux.eu>

   This file is part of the simulators.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

/* This must come before any other includes.  */
#include "defs.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "bfd.h"
#include "sim/callback.h"
#include "libiberty.h"
#include "sim/sim.h"
#include "sim-main.h"
#include "sim-assert.h"
#include "sim-options.h"
#include "sim-signal.h"
#include "sim-syscall.h"
#include "pru.h"

/* DMEM zero address is perfectly valid.  But if CRT leaves the first word
   alone, we can use it as a trap to catch NULL pointer access.  */
static bfd_boolean abort_on_dmem_zero_access;

enum {
  OPTION_ERROR_NULL_DEREF = OPTION_START,
};

/* Extract (from PRU endianess) and return an integer in HOST's endianness.  */
static uint32_t
pru_extract_unsigned_integer (const uint8_t *addr, size_t len)
{
  uint32_t retval;
  const uint8_t *p;
  const uint8_t *startaddr = addr;
  const uint8_t *endaddr = startaddr + len;

  /* Start at the most significant end of the integer, and work towards
     the least significant.  */
  retval = 0;

  for (p = endaddr; p > startaddr;)
    retval = (retval << 8) | * -- p;
  return retval;
}

/* Store "val" (which is in HOST's endianess) into "addr"
   (using PRU's endianness).  */
static void
pru_store_unsigned_integer (uint8_t *addr, size_t len, uint32_t val)
{
  uint8_t *p;
  uint8_t *startaddr = (uint8_t *)addr;
  uint8_t *endaddr = startaddr + len;

  for (p = startaddr; p < endaddr;)
    {
      *p++ = val & 0xff;
      val >>= 8;
    }
}

/* Extract a field value from CPU register using the given REGSEL selector.

   Byte number maps directly to first values of RSEL, so we can
   safely use "regsel" as a register byte number (0..3).  */
static inline uint32_t
extract_regval (uint32_t val, uint32_t regsel)
{
  ASSERT (RSEL_7_0 == 0);
  ASSERT (RSEL_15_8 == 1);
  ASSERT (RSEL_23_16 == 2);
  ASSERT (RSEL_31_24 == 3);

  switch (regsel)
    {
    case RSEL_7_0:    return (val >> 0) & 0xff;
    case RSEL_15_8:   return (val >> 8) & 0xff;
    case RSEL_23_16:  return (val >> 16) & 0xff;
    case RSEL_31_24:  return (val >> 24) & 0xff;
    case RSEL_15_0:   return (val >> 0) & 0xffff;
    case RSEL_23_8:   return (val >> 8) & 0xffff;
    case RSEL_31_16:  return (val >> 16) & 0xffff;
    case RSEL_31_0:   return val;
    default:	      sim_io_error (NULL, "invalid regsel");
    }
}

/* Write a value into CPU subregister pointed by reg and regsel.  */
static inline void
write_regval (uint32_t val, uint32_t *reg, uint32_t regsel)
{
  uint32_t mask, sh;

  switch (regsel)
    {
    case RSEL_7_0:    mask = (0xffu << 0); sh = 0; break;
    case RSEL_15_8:   mask = (0xffu << 8); sh = 8; break;
    case RSEL_23_16:  mask = (0xffu << 16); sh = 16; break;
    case RSEL_31_24:  mask = (0xffu << 24); sh = 24; break;
    case RSEL_15_0:   mask = (0xffffu << 0); sh = 0; break;
    case RSEL_23_8:   mask = (0xffffu << 8); sh = 8; break;
    case RSEL_31_16:  mask = (0xffffu << 16); sh = 16; break;
    case RSEL_31_0:   mask = 0xffffffffu; sh = 0; break;
    default:	      sim_io_error (NULL, "invalid regsel");
    }

  *reg = (*reg & ~mask) | ((val << sh) & mask);
}

/* Convert the given IMEM word address to a regular byte address used by the
   GNU ELF container.  */
static uint32_t
imem_wordaddr_to_byteaddr (SIM_CPU *cpu, uint16_t wa)
{
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);

  return (((uint32_t) wa << 2) & IMEM_ADDR_MASK) | PC_ADDR_SPACE_MARKER;
}

/* Convert the given ELF text byte address to IMEM word address.  */
static uint16_t
imem_byteaddr_to_wordaddr (SIM_CPU *cpu, uint32_t ba)
{
  return (ba >> 2) & 0xffff;
}


/* Store "nbytes" into DMEM "addr" from CPU register file, starting with
   register "regn", and byte "regb" within it.  */
static inline void
pru_reg2dmem (SIM_CPU *cpu, uint32_t addr, unsigned int nbytes,
	      int regn, int regb)
{
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);
  /* GDB assumes unconditional access to all memories, so enable additional
     checks only in standalone mode.  */
  bool standalone = (STATE_OPEN_KIND (CPU_STATE (cpu)) == SIM_OPEN_STANDALONE);

  if (abort_on_dmem_zero_access && addr < 4)
    {
      sim_core_signal (CPU_STATE (cpu), cpu, PC_byteaddr, write_map,
		       nbytes, addr, write_transfer,
		       sim_core_unmapped_signal);
    }
  else if (standalone && ((addr >= PC_ADDR_SPACE_MARKER)
			  || (addr + nbytes > PC_ADDR_SPACE_MARKER)))
    {
      sim_core_signal (CPU_STATE (cpu), cpu, PC_byteaddr, write_map,
		       nbytes, addr, write_transfer,
		       sim_core_unmapped_signal);
    }
  else if ((regn * 4 + regb + nbytes) > (32 * 4))
    {
      sim_io_eprintf (CPU_STATE (cpu),
		      "SBBO/SBCO with invalid store data length\n");
      RAISE_SIGILL (CPU_STATE (cpu));
    }
  else
    {
      TRACE_MEMORY (cpu, "write of %d bytes to %08x", nbytes, addr);
      while (nbytes--)
	{
	  sim_core_write_1 (cpu,
			    PC_byteaddr,
			    write_map,
			    addr++,
			    extract_regval (CPU.regs[regn], regb));

	  if (++regb >= 4)
	    {
	      regb = 0;
	      regn++;
	    }
	}
    }
}

/* Load "nbytes" from DMEM "addr" into CPU register file, starting with
   register "regn", and byte "regb" within it.  */
static inline void
pru_dmem2reg (SIM_CPU *cpu, uint32_t addr, unsigned int nbytes,
	      int regn, int regb)
{
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);
  /* GDB assumes unconditional access to all memories, so enable additional
     checks only in standalone mode.  */
  bool standalone = (STATE_OPEN_KIND (CPU_STATE (cpu)) == SIM_OPEN_STANDALONE);

  if (abort_on_dmem_zero_access && addr < 4)
    {
      sim_core_signal (CPU_STATE (cpu), cpu, PC_byteaddr, read_map,
		       nbytes, addr, read_transfer,
		       sim_core_unmapped_signal);
    }
  else if (standalone && ((addr >= PC_ADDR_SPACE_MARKER)
			  || (addr + nbytes > PC_ADDR_SPACE_MARKER)))
    {
      /* This check is necessary because our IMEM "address space"
	 is not really accessible, yet we have mapped it as a generic
	 memory space.  */
      sim_core_signal (CPU_STATE (cpu), cpu, PC_byteaddr, read_map,
		       nbytes, addr, read_transfer,
		       sim_core_unmapped_signal);
    }
  else if ((regn * 4 + regb + nbytes) > (32 * 4))
    {
      sim_io_eprintf (CPU_STATE (cpu),
		      "LBBO/LBCO with invalid load data length\n");
      RAISE_SIGILL (CPU_STATE (cpu));
    }
  else
    {
      unsigned int b;
      TRACE_MEMORY (cpu, "read of %d bytes from %08x", nbytes, addr);
      while (nbytes--)
	{
	  b = sim_core_read_1 (cpu, PC_byteaddr, read_map, addr++);

	  /* Reuse the fact the Register Byte Number maps directly to RSEL.  */
	  ASSERT (RSEL_7_0 == 0);
	  write_regval (b, &CPU.regs[regn], regb);

	  if (++regb >= 4)
	    {
	      regb = 0;
	      regn++;
	    }
	}
    }
}

/* Set reset values of general-purpose registers.  */
static void
set_initial_gprs (SIM_CPU *cpu)
{
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);
  int i;

  /* Set up machine just out of reset.  */
  CPU_PC_SET (cpu, 0);
  PC_ADDR_SPACE_MARKER = IMEM_ADDR_DEFAULT; /* from default linker script? */

  /* Clean out the GPRs.  */
  for (i = 0; i < ARRAY_SIZE (CPU.regs); i++)
    CPU.regs[i] = 0;
  for (i = 0; i < ARRAY_SIZE (CPU.macregs); i++)
    CPU.macregs[i] = 0;

  CPU.loop.looptop = CPU.loop.loopend = 0;
  CPU.loop.loop_in_progress = 0;
  CPU.loop.loop_counter = 0;

  CPU.carry = 0;
  CPU.insts = 0;
  CPU.cycles = 0;

  /* AM335x should provide sane defaults.  */
  CPU.ctable[0] = 0x00020000;
  CPU.ctable[1] = 0x48040000;
  CPU.ctable[2] = 0x4802a000;
  CPU.ctable[3] = 0x00030000;
  CPU.ctable[4] = 0x00026000;
  CPU.ctable[5] = 0x48060000;
  CPU.ctable[6] = 0x48030000;
  CPU.ctable[7] = 0x00028000;
  CPU.ctable[8] = 0x46000000;
  CPU.ctable[9] = 0x4a100000;
  CPU.ctable[10] = 0x48318000;
  CPU.ctable[11] = 0x48022000;
  CPU.ctable[12] = 0x48024000;
  CPU.ctable[13] = 0x48310000;
  CPU.ctable[14] = 0x481cc000;
  CPU.ctable[15] = 0x481d0000;
  CPU.ctable[16] = 0x481a0000;
  CPU.ctable[17] = 0x4819c000;
  CPU.ctable[18] = 0x48300000;
  CPU.ctable[19] = 0x48302000;
  CPU.ctable[20] = 0x48304000;
  CPU.ctable[21] = 0x00032400;
  CPU.ctable[22] = 0x480c8000;
  CPU.ctable[23] = 0x480ca000;
  CPU.ctable[24] = 0x00000000;
  CPU.ctable[25] = 0x00002000;
  CPU.ctable[26] = 0x0002e000;
  CPU.ctable[27] = 0x00032000;
  CPU.ctable[28] = 0x00000000;
  CPU.ctable[29] = 0x49000000;
  CPU.ctable[30] = 0x40000000;
  CPU.ctable[31] = 0x80000000;
}

/* Map regsel selector to subregister field width.  */
static inline unsigned int
regsel_width (uint32_t regsel)
{
  switch (regsel)
    {
    case RSEL_7_0:    return 8;
    case RSEL_15_8:   return 8;
    case RSEL_23_16:  return 8;
    case RSEL_31_24:  return 8;
    case RSEL_15_0:   return 16;
    case RSEL_23_8:   return 16;
    case RSEL_31_16:  return 16;
    case RSEL_31_0:   return 32;
    default:	      sim_io_error (NULL, "invalid regsel");
    }
}

/* Handle XIN instruction addressing the MAC peripheral.  */
static void
pru_sim_xin_mac (SIM_DESC sd, SIM_CPU *cpu, unsigned int rd_regn,
		 unsigned int rdb, unsigned int length)
{
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);

  if (rd_regn < 25 || (rd_regn * 4 + rdb + length) > (27 + 1) * 4)
    sim_io_error (sd, "XIN MAC: invalid transfer regn=%u.%u, length=%u\n",
		  rd_regn, rdb, length);

  /* Copy from MAC to PRU regs.  Ranges have been validated above.  */
  while (length--)
    {
      write_regval (CPU.macregs[rd_regn - 25] >> (rdb * 8),
		    &CPU.regs[rd_regn],
		    rdb);
      if (++rdb == 4)
	{
	  rdb = 0;
	  rd_regn++;
	}
    }
}

/* Handle XIN instruction.  */
static void
pru_sim_xin (SIM_DESC sd, SIM_CPU *cpu, unsigned int wba,
	     unsigned int rd_regn, unsigned int rdb, unsigned int length)
{
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);

  if (wba == 0)
    {
      pru_sim_xin_mac (sd, cpu, rd_regn, rdb, length);
    }
  else if (wba == XFRID_SCRATCH_BANK_0 || wba == XFRID_SCRATCH_BANK_1
	   || wba == XFRID_SCRATCH_BANK_2 || wba == XFRID_SCRATCH_BANK_PEER)
    {
      while (length--)
	{
	  unsigned int val;

	  val = extract_regval (CPU.scratchpads[wba][rd_regn], rdb);
	  write_regval (val, &CPU.regs[rd_regn], rdb);
	  if (++rdb == 4)
	    {
	      rdb = 0;
	      rd_regn++;
	    }
	}
    }
  else if (wba == 254 || wba == 255)
    {
      /* FILL/ZERO pseudos implemented via XIN.  */
      unsigned int fillbyte = (wba == 254) ? 0xff : 0x00;
      while (length--)
	{
	  write_regval (fillbyte, &CPU.regs[rd_regn], rdb);
	  if (++rdb == 4)
	    {
	      rdb = 0;
	      rd_regn++;
	    }
	}
    }
  else
    {
      sim_io_error (sd, "XIN: XFR device %d not supported.\n", wba);
    }
}

/* Handle XOUT instruction addressing the MAC peripheral.  */
static void
pru_sim_xout_mac (SIM_DESC sd, SIM_CPU *cpu, unsigned int rd_regn,
		  unsigned int rdb, unsigned int length)
{
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);
  const int modereg_accessed = (rd_regn == 25);

  /* Multiple Accumulate.  */
  if (rd_regn < 25 || (rd_regn * 4 + rdb + length) > (27 + 1) * 4)
    sim_io_error (sd, "XOUT MAC: invalid transfer regn=%u.%u, length=%u\n",
		  rd_regn, rdb, length);

  /* Copy from PRU to MAC regs.  Ranges have been validated above.  */
  while (length--)
    {
      write_regval (CPU.regs[rd_regn] >> (rdb * 8),
		    &CPU.macregs[rd_regn - 25],
		    rdb);
      if (++rdb == 4)
	{
	  rdb = 0;
	  rd_regn++;
	}
    }

  if (modereg_accessed
      && (CPU.macregs[PRU_MACREG_MODE] & MAC_R25_MAC_MODE_MASK))
    {
      /* MUL/MAC operands are sampled every XOUT in multiply and
	 accumulate mode.  */
      uint64_t prod, oldsum, sum;
      CPU.macregs[PRU_MACREG_OP_0] = CPU.regs[28];
      CPU.macregs[PRU_MACREG_OP_1] = CPU.regs[29];

      prod = CPU.macregs[PRU_MACREG_OP_0];
      prod *= (uint64_t)CPU.macregs[PRU_MACREG_OP_1];

      oldsum = CPU.macregs[PRU_MACREG_ACC_L];
      oldsum += (uint64_t)CPU.macregs[PRU_MACREG_ACC_H] << 32;
      sum = oldsum + prod;

      CPU.macregs[PRU_MACREG_PROD_L] = sum & 0xfffffffful;
      CPU.macregs[PRU_MACREG_PROD_H] = sum >> 32;
      CPU.macregs[PRU_MACREG_ACC_L] = CPU.macregs[PRU_MACREG_PROD_L];
      CPU.macregs[PRU_MACREG_ACC_H] = CPU.macregs[PRU_MACREG_PROD_H];

      if (oldsum > sum)
	CPU.macregs[PRU_MACREG_MODE] |= MAC_R25_ACC_CARRY_MASK;
    }
  if (modereg_accessed
      && (CPU.macregs[PRU_MACREG_MODE] & MAC_R25_ACC_CARRY_MASK))
    {
      /* store 1 to clear.  */
      CPU.macregs[PRU_MACREG_MODE] &= ~MAC_R25_ACC_CARRY_MASK;
      CPU.macregs[PRU_MACREG_ACC_L] = 0;
      CPU.macregs[PRU_MACREG_ACC_H] = 0;
    }

}

/* Handle XOUT instruction.  */
static void
pru_sim_xout (SIM_DESC sd, SIM_CPU *cpu, unsigned int wba,
	      unsigned int rd_regn, unsigned int rdb, unsigned int length)
{
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);

  if (wba == 0)
    {
      pru_sim_xout_mac (sd, cpu, rd_regn, rdb, length);
    }
  else if (wba == XFRID_SCRATCH_BANK_0 || wba == XFRID_SCRATCH_BANK_1
	   || wba == XFRID_SCRATCH_BANK_2 || wba == XFRID_SCRATCH_BANK_PEER)
    {
      while (length--)
	{
	  unsigned int val;

	  val = extract_regval (CPU.regs[rd_regn], rdb);
	  write_regval (val, &CPU.scratchpads[wba][rd_regn], rdb);
	  if (++rdb == 4)
	    {
	      rdb = 0;
	      rd_regn++;
	    }
	}
    }
  else
    sim_io_error (sd, "XOUT: XFR device %d not supported.\n", wba);
}

/* Handle XCHG instruction.  */
static void
pru_sim_xchg (SIM_DESC sd, SIM_CPU *cpu, unsigned int wba,
	      unsigned int rd_regn, unsigned int rdb, unsigned int length)
{
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);

  if (wba == XFRID_SCRATCH_BANK_0 || wba == XFRID_SCRATCH_BANK_1
	   || wba == XFRID_SCRATCH_BANK_2 || wba == XFRID_SCRATCH_BANK_PEER)
    {
      while (length--)
	{
	  unsigned int valr, vals;

	  valr = extract_regval (CPU.regs[rd_regn], rdb);
	  vals = extract_regval (CPU.scratchpads[wba][rd_regn], rdb);
	  write_regval (valr, &CPU.scratchpads[wba][rd_regn], rdb);
	  write_regval (vals, &CPU.regs[rd_regn], rdb);
	  if (++rdb == 4)
	    {
	      rdb = 0;
	      rd_regn++;
	    }
	}
    }
  else
    sim_io_error (sd, "XOUT: XFR device %d not supported.\n", wba);
}

/* Handle syscall simulation.  Its ABI is specific to the GNU simulator.  */
static void
pru_sim_syscall (SIM_DESC sd, SIM_CPU *cpu)
{
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);
  /* If someday TI confirms that the "reserved" HALT opcode fields
     can be used for extra arguments, then maybe we can embed
     the syscall number there.  Until then, let's use R1.  */
  const uint32_t syscall_num = CPU.regs[1];
  long ret;

  ret = sim_syscall (cpu, syscall_num,
		     CPU.regs[14], CPU.regs[15],
		     CPU.regs[16], CPU.regs[17]);
  CPU.regs[14] = ret;
}

/* Simulate one instruction.  */
static void
sim_step_once (SIM_DESC sd)
{
  SIM_CPU *cpu = STATE_CPU (sd, 0);
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);
  const struct pru_opcode *op;
  uint32_t inst;
  uint32_t _RDVAL, OP2;	/* intermediate values.  */
  int rd_is_modified = 0;	/* RD modified and must be stored back.  */

  /* Fetch the initial instruction that we'll decode.  */
  inst = sim_core_read_4 (cpu, PC_byteaddr, exec_map, PC_byteaddr);
  TRACE_MEMORY (cpu, "read of insn 0x%08x from %08x", inst, PC_byteaddr);

  op = pru_find_opcode (inst);

  if (!op)
    {
      sim_io_eprintf (sd, "Unknown instruction 0x%04x\n", inst);
      RAISE_SIGILL (sd);
    }
  else
    {
      TRACE_DISASM (cpu, PC_byteaddr);

      /* In multiply-only mode, R28/R29 operands are sampled on every clock
	 cycle.  */
      if ((CPU.macregs[PRU_MACREG_MODE] & MAC_R25_MAC_MODE_MASK) == 0)
	{
	  CPU.macregs[PRU_MACREG_OP_0] = CPU.regs[28];
	  CPU.macregs[PRU_MACREG_OP_1] = CPU.regs[29];
	}

      switch (op->type)
	{
/* Helper macro to improve clarity of pru.isa.  The empty while is a
   guard against using RD as a left-hand side value.  */
#define RD  do { } while (0); rd_is_modified = 1; _RDVAL
#define INSTRUCTION(NAME, ACTION)		\
	case prui_ ## NAME:			\
		ACTION;				\
	  break;
#include "pru.isa"
#undef INSTRUCTION
#undef RD

	default:
	  RAISE_SIGILL (sd);
	}

      if (rd_is_modified)
	write_regval (_RDVAL, &CPU.regs[RD_REGN], RDSEL);

      /* Don't treat r30 and r31 as regular registers, they are I/O!  */
      CPU.regs[30] = 0;
      CPU.regs[31] = 0;

      /* Handle PC match of loop end.  */
      if (LOOP_IN_PROGRESS && (PC == LOOPEND))
	{
	  SIM_ASSERT (LOOPCNT > 0);
	  if (--LOOPCNT == 0)
	    LOOP_IN_PROGRESS = 0;
	  else
	    PC = LOOPTOP;
	}

      /* In multiply-only mode, MAC does multiplication every cycle.  */
      if ((CPU.macregs[PRU_MACREG_MODE] & MAC_R25_MAC_MODE_MASK) == 0)
	{
	  uint64_t prod;
	  prod = CPU.macregs[PRU_MACREG_OP_0];
	  prod *= (uint64_t)CPU.macregs[PRU_MACREG_OP_1];
	  CPU.macregs[PRU_MACREG_PROD_L] = prod & 0xfffffffful;
	  CPU.macregs[PRU_MACREG_PROD_H] = prod >> 32;

	  /* Clear the MAC accumulator when in normal mode.  */
	  CPU.macregs[PRU_MACREG_ACC_L] = 0;
	  CPU.macregs[PRU_MACREG_ACC_H] = 0;
	}

      /* Update cycle counts.  */
      CPU.insts += 1;		  /* One instruction completed ...  */
      CPU.cycles += 1;		  /* ... and it takes a single cycle.  */

      /* Account for memory access latency with a reasonable estimate.
	 No distinction is currently made between SRAM, DRAM and generic
	 L3 slaves.  */
      if (op->type == prui_lbbo || op->type == prui_sbbo
	  || op->type == prui_lbco || op->type == prui_sbco)
	CPU.cycles += 2;

    }
}

/* Implement standard sim_engine_run function.  */
void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr, /* ignore  */
		int nr_cpus, /* ignore  */
		int siggnal) /* ignore  */
{
  while (1)
    {
      sim_step_once (sd);
      if (sim_events_tick (sd))
	sim_events_process (sd);
    }
}


/* Implement callback for standard CPU_PC_FETCH routine.  */
static sim_cia
pru_pc_get (sim_cpu *cpu)
{
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);

  /* Present PC as byte address.  */
  return imem_wordaddr_to_byteaddr (cpu, pru_cpu->pc);
}

/* Implement callback for standard CPU_PC_STORE routine.  */
static void
pru_pc_set (sim_cpu *cpu, sim_cia pc)
{
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);

  /* PC given as byte address.  */
  pru_cpu->pc = imem_byteaddr_to_wordaddr (cpu, pc);
}


/* Implement callback for standard CPU_REG_STORE routine.  */
static int
pru_store_register (SIM_CPU *cpu, int rn, const void *memory, int length)
{
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);

  if (rn < NUM_REGS && rn >= 0)
    {
      if (length == 4)
	{
	  /* Misalignment safe.  */
	  long ival = pru_extract_unsigned_integer (memory, 4);
	  if (rn < 32)
	    CPU.regs[rn] = ival;
	  else
	    pru_pc_set (cpu, ival);
	  return 4;
	}
      else
	return 0;
    }
  else
    return 0;
}

/* Implement callback for standard CPU_REG_FETCH routine.  */
static int
pru_fetch_register (SIM_CPU *cpu, int rn, void *memory, int length)
{
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);
  long ival;

  if (rn < NUM_REGS && rn >= 0)
    {
      if (length == 4)
	{
	  if (rn < 32)
	    ival = CPU.regs[rn];
	  else
	    ival = pru_pc_get (cpu);

	  /* Misalignment-safe.  */
	  pru_store_unsigned_integer (memory, 4, ival);
	  return 4;
	}
      else
	return 0;
    }
  else
    return 0;
}

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

/* Declare the PRU option handler.  */
static DECLARE_OPTION_HANDLER (pru_option_handler);

/* Implement the PRU option handler.  */
static SIM_RC
pru_option_handler (SIM_DESC sd, sim_cpu *cpu, int opt, char *arg,
		    int is_command)
{
  switch (opt)
    {
    case OPTION_ERROR_NULL_DEREF:
      abort_on_dmem_zero_access = TRUE;
      return SIM_RC_OK;

    default:
      sim_io_eprintf (sd, "Unknown PRU option %d\n", opt);
      return SIM_RC_FAIL;
    }
}

/* List of PRU-specific options.  */
static const OPTION pru_options[] =
{
  { {"error-null-deref", no_argument, NULL, OPTION_ERROR_NULL_DEREF},
      '\0', NULL, "Trap any access to DMEM address zero",
      pru_option_handler, NULL },

  { {NULL, no_argument, NULL, 0}, '\0', NULL, NULL, NULL, NULL }
};

/* Implement standard sim_open function.  */
SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *cb,
	  struct bfd *abfd, char * const *argv)
{
  int i;
  char c;
  SIM_DESC sd = sim_state_alloc (kind, cb);
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  /* Set default options before parsing user options.  */
  current_alignment = STRICT_ALIGNMENT;
  current_target_byte_order = BFD_ENDIAN_LITTLE;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct pru_regset)) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }
  sim_add_option_table (sd, NULL, pru_options);

  /* The parser will print an error message for us, so we silently return.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Check for/establish a reference program image.  */
  if (sim_analyze_program (sd, STATE_PROG_FILE (sd), abfd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Configure/verify the target byte order and other runtime
     configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
      sim_module_uninstall (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      /* Uninstall the modules to avoid memory leaks,
	 file descriptor leaks, etc.  */
      sim_module_uninstall (sd);
      return 0;
    }

  /* CPU specific initialization.  */
  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    {
      SIM_CPU *cpu = STATE_CPU (sd, i);

      CPU_REG_STORE (cpu) = pru_store_register;
      CPU_REG_FETCH (cpu) = pru_fetch_register;
      CPU_PC_FETCH (cpu) = pru_pc_get;
      CPU_PC_STORE (cpu) = pru_pc_set;

      set_initial_gprs (cpu);
    }

  /* Allocate external memory if none specified by user.
     Use address 4 here in case the user wanted address 0 unmapped.  */
  if (sim_core_read_buffer (sd, NULL, read_map, &c, 4, 1) == 0)
    {
      sim_do_commandf (sd, "memory-region 0x%x,0x%x",
		       0,
		       DMEM_DEFAULT_SIZE);
    }
  if (sim_core_read_buffer (sd, NULL, read_map, &c, IMEM_ADDR_DEFAULT, 1) == 0)
    {
      sim_do_commandf (sd, "memory-region 0x%x,0x%x",
		       IMEM_ADDR_DEFAULT,
		       IMEM_DEFAULT_SIZE);
    }

  return sd;
}

/* Implement standard sim_create_inferior function.  */
SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *prog_bfd,
		     char * const *argv, char * const *env)
{
  SIM_CPU *cpu = STATE_CPU (sd, 0);
  struct pru_regset *pru_cpu = PRU_SIM_CPU (cpu);
  host_callback *cb = STATE_CALLBACK (sd);
  bfd_vma addr;

  addr = bfd_get_start_address (prog_bfd);

  sim_pc_set (cpu, addr);
  PC_ADDR_SPACE_MARKER = addr & ~IMEM_ADDR_MASK;

  /* Standalone mode (i.e. `run`) will take care of the argv for us in
     sim_open () -> sim_parse_args ().  But in debug mode (i.e. 'target sim'
     with `gdb`), we need to handle it because the user can change the
     argv on the fly via gdb's 'run'.  */
  if (STATE_PROG_ARGV (sd) != argv)
    {
      freeargv (STATE_PROG_ARGV (sd));
      STATE_PROG_ARGV (sd) = dupargv (argv);
    }

  if (STATE_PROG_ENVP (sd) != env)
    {
      freeargv (STATE_PROG_ENVP (sd));
      STATE_PROG_ENVP (sd) = dupargv (env);
    }

  cb->argv = STATE_PROG_ARGV (sd);
  cb->envp = STATE_PROG_ENVP (sd);

  return SIM_RC_OK;
}
