/* Simulator for Xilinx MicroBlaze processor
   Copyright 2009-2024 Free Software Foundation, Inc.

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
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

/* This must come before any other includes.  */
#include "defs.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bfd.h"
#include "sim/callback.h"
#include "libiberty.h"
#include "sim/sim.h"

#include "sim-main.h"
#include "sim-options.h"
#include "sim-signal.h"
#include "sim-syscall.h"

#include "microblaze-sim.h"
#include "opcodes/microblaze-dis.h"

#define target_big_endian (CURRENT_TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)

static unsigned long
microblaze_extract_unsigned_integer (const unsigned char *addr, int len)
{
  unsigned long retval;
  unsigned char *p;
  unsigned char *startaddr = (unsigned char *)addr;
  unsigned char *endaddr = startaddr + len;

  if (len > (int) sizeof (unsigned long))
    printf ("That operation is not available on integers of more than "
	    "%zu bytes.", sizeof (unsigned long));

  /* Start at the most significant end of the integer, and work towards
     the least significant.  */
  retval = 0;

  if (!target_big_endian)
    {
      for (p = endaddr; p > startaddr;)
	retval = (retval << 8) | * -- p;
    }
  else
    {
      for (p = startaddr; p < endaddr;)
	retval = (retval << 8) | * p ++;
    }

  return retval;
}

static void
microblaze_store_unsigned_integer (unsigned char *addr, int len,
				   unsigned long val)
{
  unsigned char *p;
  unsigned char *startaddr = (unsigned char *)addr;
  unsigned char *endaddr = startaddr + len;

  if (!target_big_endian)
    {
      for (p = startaddr; p < endaddr;)
	{
	  *p++ = val & 0xff;
	  val >>= 8;
	}
    }
  else
    {
      for (p = endaddr; p > startaddr;)
	{
	  *--p = val & 0xff;
	  val >>= 8;
	}
    }
}

static void
set_initial_gprs (SIM_CPU *cpu)
{
  int i;

  /* Set up machine just out of reset.  */
  PC = 0;
  MSR = 0;

  /* Clean out the GPRs */
  for (i = 0; i < 32; i++)
    CPU.regs[i] = 0;
  CPU.insts = 0;
  CPU.cycles = 0;
  CPU.imm_enable = 0;
}

static int tracing = 0;

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr, /* ignore  */
		int nr_cpus, /* ignore  */
		int siggnal) /* ignore  */
{
  SIM_CPU *cpu = STATE_CPU (sd, 0);
  signed_4 inst;
  enum microblaze_instr op;
  int memops;
  int bonus_cycles;
  int insts;
  unsigned_1 carry;
  bool imm_unsigned;
  short ra, rb, rd;
  unsigned_4 oldpc, newpc;
  short delay_slot_enable;
  short branch_taken;
  short num_delay_slot; /* UNUSED except as reqd parameter */
  enum microblaze_instr_type insn_type;

  memops = 0;
  bonus_cycles = 0;
  insts = 0;

  while (1)
    {
      /* Fetch the initial instructions that we'll decode. */
      inst = MEM_RD_WORD (PC & 0xFFFFFFFC);

      op = get_insn_microblaze (inst, &imm_unsigned, &insn_type,
				&num_delay_slot);

      if (op == invalid_inst)
	fprintf (stderr, "Unknown instruction 0x%04x", inst);

      if (tracing)
	fprintf (stderr, "%.4x: inst = %.4x ", PC, inst);

      rd = GET_RD;
      rb = GET_RB;
      ra = GET_RA;
      /*      immword = IMM_W; */

      oldpc = PC;
      delay_slot_enable = 0;
      branch_taken = 0;
      if (op == microblaze_brk)
	sim_engine_halt (sd, NULL, NULL, NULL_CIA, sim_stopped, SIM_SIGTRAP);
      else if (inst == MICROBLAZE_HALT_INST)
	{
	  insts += 1;
	  bonus_cycles++;
	  TRACE_INSN (cpu, "HALT (%i)", RETREG);
	  sim_engine_halt (sd, NULL, NULL, NULL_CIA, sim_exited, RETREG);
	}
      else
	{
	  switch(op)
	    {
#define INSTRUCTION(NAME, OPCODE, TYPE, ACTION)		\
	    case NAME:					\
	      TRACE_INSN (cpu, #NAME);			\
	      ACTION;					\
	      break;
#include "microblaze.isa"
#undef INSTRUCTION

	    default:
	      sim_engine_halt (sd, NULL, NULL, NULL_CIA, sim_signalled,
			       SIM_SIGILL);
	      fprintf (stderr, "ERROR: Unknown opcode\n");
	    }
	  /* Make R0 consistent */
	  CPU.regs[0] = 0;

	  /* Check for imm instr */
	  if (op == imm)
	    IMM_ENABLE = 1;
	  else
	    IMM_ENABLE = 0;

	  /* Update cycle counts */
	  insts ++;
	  if (insn_type == memory_store_inst || insn_type == memory_load_inst)
	    memops++;
	  if (insn_type == mult_inst)
	    bonus_cycles++;
	  if (insn_type == barrel_shift_inst)
	    bonus_cycles++;
	  if (insn_type == anyware_inst)
	    bonus_cycles++;
	  if (insn_type == div_inst)
	    bonus_cycles += 33;

	  if ((insn_type == branch_inst || insn_type == return_inst)
	      && branch_taken)
	    {
	      /* Add an extra cycle for taken branches */
	      bonus_cycles++;
	      /* For branch instructions handle the instruction in the delay slot */
	      if (delay_slot_enable)
	        {
	          newpc = PC;
	          PC = oldpc + INST_SIZE;
	          inst = MEM_RD_WORD (PC & 0xFFFFFFFC);
	          op = get_insn_microblaze (inst, &imm_unsigned, &insn_type,
					    &num_delay_slot);
	          if (op == invalid_inst)
		    fprintf (stderr, "Unknown instruction 0x%04x", inst);
	          if (tracing)
		    fprintf (stderr, "%.4x: inst = %.4x ", PC, inst);
	          rd = GET_RD;
	          rb = GET_RB;
	          ra = GET_RA;
	          /*	      immword = IMM_W; */
	          if (op == microblaze_brk)
		    {
		      if (STATE_VERBOSE_P (sd))
		        fprintf (stderr, "Breakpoint set in delay slot "
			         "(at address 0x%x) will not be honored\n", PC);
		      /* ignore the breakpoint */
		    }
	          else if (insn_type == branch_inst || insn_type == return_inst)
		    {
		      if (STATE_VERBOSE_P (sd))
		        fprintf (stderr, "Cannot have branch or return instructions "
			         "in delay slot (at address 0x%x)\n", PC);
		      sim_engine_halt (sd, NULL, NULL, NULL_CIA, sim_signalled,
				       SIM_SIGILL);
		    }
	          else
		    {
		      switch(op)
		        {
#define INSTRUCTION(NAME, OPCODE, TYPE, ACTION)		\
		        case NAME:			\
			  ACTION;			\
			  break;
#include "microblaze.isa"
#undef INSTRUCTION

		        default:
		          sim_engine_halt (sd, NULL, NULL, NULL_CIA,
					   sim_signalled, SIM_SIGILL);
		          fprintf (stderr, "ERROR: Unknown opcode at 0x%x\n", PC);
		        }
		      /* Update cycle counts */
		      insts++;
		      if (insn_type == memory_store_inst
		          || insn_type == memory_load_inst)
		        memops++;
		      if (insn_type == mult_inst)
		        bonus_cycles++;
		      if (insn_type == barrel_shift_inst)
		        bonus_cycles++;
		      if (insn_type == anyware_inst)
		        bonus_cycles++;
		      if (insn_type == div_inst)
		        bonus_cycles += 33;
		    }
	          /* Restore the PC */
	          PC = newpc;
	          /* Make R0 consistent */
	          CPU.regs[0] = 0;
	          /* Check for imm instr */
	          if (op == imm)
		    IMM_ENABLE = 1;
	          else
		    IMM_ENABLE = 0;
	        }
	      else
		{
		  if (op == brki && IMM == 8)
		    {
		      RETREG = sim_syscall (cpu, CPU.regs[12], CPU.regs[5],
					    CPU.regs[6], CPU.regs[7],
					    CPU.regs[8]);
		      PC = RD + INST_SIZE;
		    }

		  /* no delay slot: increment cycle count */
		  bonus_cycles++;
		}
	    }
	}

      if (tracing)
	fprintf (stderr, "\n");

      if (sim_events_tick (sd))
	sim_events_process (sd);
    }

  /* Hide away the things we've cached while executing.  */
  /*  CPU.pc = pc; */
  CPU.insts += insts;		/* instructions done ... */
  CPU.cycles += insts;		/* and each takes a cycle */
  CPU.cycles += bonus_cycles;	/* and extra cycles for branches */
  CPU.cycles += memops; 	/* and memop cycle delays */
}

static int
microblaze_reg_store (SIM_CPU *cpu, int rn, const void *memory, int length)
{
  if (rn < NUM_REGS + NUM_SPECIAL && rn >= 0)
    {
      if (length == 4)
	{
	  /* misalignment safe */
	  long ival = microblaze_extract_unsigned_integer (memory, 4);
	  if (rn < NUM_REGS)
	    CPU.regs[rn] = ival;
	  else
	    CPU.spregs[rn-NUM_REGS] = ival;
	  return 4;
	}
      else
	return 0;
    }
  else
    return 0;
}

static int
microblaze_reg_fetch (SIM_CPU *cpu, int rn, void *memory, int length)
{
  long ival;

  if (rn < NUM_REGS + NUM_SPECIAL && rn >= 0)
    {
      if (length == 4)
	{
	  if (rn < NUM_REGS)
	    ival = CPU.regs[rn];
	  else
	    ival = CPU.spregs[rn-NUM_REGS];

	  /* misalignment-safe */
	  microblaze_store_unsigned_integer (memory, 4, ival);
	  return 4;
	}
      else
	return 0;
    }
  else
    return 0;
}

void
sim_info (SIM_DESC sd, bool verbose)
{
  SIM_CPU *cpu = STATE_CPU (sd, 0);
  host_callback *callback = STATE_CALLBACK (sd);

  callback->printf_filtered (callback, "\n\n# instructions executed  %10d\n",
			     CPU.insts);
  callback->printf_filtered (callback, "# cycles                 %10d\n",
			     (CPU.cycles) ? CPU.cycles+2 : 0);
}

static sim_cia
microblaze_pc_get (sim_cpu *cpu)
{
  return MICROBLAZE_SIM_CPU (cpu)->spregs[0];
}

static void
microblaze_pc_set (sim_cpu *cpu, sim_cia pc)
{
  MICROBLAZE_SIM_CPU (cpu)->spregs[0] = pc;
}

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *cb,
	  struct bfd *abfd, char * const *argv)
{
  int i;
  SIM_DESC sd = sim_state_alloc (kind, cb);
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct microblaze_regset))
      != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* The parser will print an error message for us, so we silently return.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Check for/establish the a reference program image.  */
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

      CPU_REG_FETCH (cpu) = microblaze_reg_fetch;
      CPU_REG_STORE (cpu) = microblaze_reg_store;
      CPU_PC_FETCH (cpu) = microblaze_pc_get;
      CPU_PC_STORE (cpu) = microblaze_pc_set;

      set_initial_gprs (cpu);
    }

  /* Default to a 8 Mbyte (== 2^23) memory space.  */
  sim_do_commandf (sd, "memory-size 0x800000");

  return sd;
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *prog_bfd,
		     char * const *argv, char * const *env)
{
  SIM_CPU *cpu = STATE_CPU (sd, 0);

  PC = bfd_get_start_address (prog_bfd);

  return SIM_RC_OK;
}
