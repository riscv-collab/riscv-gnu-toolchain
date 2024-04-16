/* Example synacor simulator.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.
   Contributed by Mike Frysinger.

   This file is part of simulators.

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

/* This file contains the main simulator decoding logic.  i.e. everything that
   is architecture specific.  */

/* This must come before any other includes.  */
#include "defs.h"

#include "sim-main.h"
#include "sim-signal.h"

#include "example-synacor-sim.h"

/* Get the register number from the number.  */
static uint16_t
register_num (SIM_CPU *cpu, uint16_t num)
{
  SIM_DESC sd = CPU_STATE (cpu);

  if (num < 0x8000 || num >= 0x8008)
    sim_engine_halt (sd, cpu, NULL, sim_pc_get (cpu), sim_signalled, SIM_SIGILL);

  return num & 0xf;
}

/* Helper to process immediates according to the ISA.  */
static uint16_t
interp_num (SIM_CPU *cpu, uint16_t num)
{
  SIM_DESC sd = CPU_STATE (cpu);
  struct example_sim_cpu *example_cpu = EXAMPLE_SIM_CPU (cpu);

  if (num < 0x8000)
    {
      /* Numbers 0..32767 mean a literal value.  */
      TRACE_DECODE (cpu, "%#x is a literal", num);
      return num;
    }
  else if (num < 0x8008)
    {
      /* Numbers 32768..32775 instead mean registers 0..7.  */
      TRACE_DECODE (cpu, "%#x is register R%i", num, num & 0xf);
      return example_cpu->regs[num & 0xf];
    }
  else
    {
      /* Numbers 32776..65535 are invalid.  */
      TRACE_DECODE (cpu, "%#x is an invalid number", num);
      sim_engine_halt (sd, cpu, NULL, example_cpu->pc, sim_signalled, SIM_SIGILL);
    }
}

/* Decode & execute a single instruction.  */
void step_once (SIM_CPU *cpu)
{
  SIM_DESC sd = CPU_STATE (cpu);
  struct example_sim_cpu *example_cpu = EXAMPLE_SIM_CPU (cpu);
  uint16_t iw1, num1;
  sim_cia pc = sim_pc_get (cpu);

  iw1 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc);
  TRACE_EXTRACT (cpu, "%04x: iw1: %#x", pc, iw1);
  /* This never happens, but technically is possible in the ISA.  */
  num1 = interp_num (cpu, iw1);

  if (num1 == 0)
    {
      /* halt: 0: Stop execution and terminate the program.  */
      TRACE_INSN (cpu, "HALT");
      sim_engine_halt (sd, cpu, NULL, pc, sim_exited, 0);
    }
  else if (num1 == 1)
    {
      /* set: 1 a b: Set register <a> to the value of <b>.  */
      uint16_t iw2, iw3, num2, num3;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = register_num (cpu, iw2);
      iw3 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 4);
      num3 = interp_num (cpu, iw3);
      TRACE_EXTRACT (cpu, "SET %#x %#x", iw2, iw3);
      TRACE_INSN (cpu, "SET R%i %#x", num2, num3);

      TRACE_REGISTER (cpu, "R%i = %#x", num2, num3);
      example_cpu->regs[num2] = num3;

      pc += 6;
    }
  else if (num1 == 2)
    {
      /* push: 2 a: Push <a> onto the stack.  */
      uint16_t iw2, num2;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = interp_num (cpu, iw2);
      TRACE_EXTRACT (cpu, "PUSH %#x", iw2);
      TRACE_INSN (cpu, "PUSH %#x", num2);

      sim_core_write_aligned_2 (cpu, pc, write_map, example_cpu->sp, num2);
      example_cpu->sp -= 2;
      TRACE_REGISTER (cpu, "SP = %#x", example_cpu->sp);

      pc += 4;
    }
  else if (num1 == 3)
    {
      /* pop: 3 a: Remove the top element from the stack and write it into <a>.
	 Empty stack = error.  */
      uint16_t iw2, num2, result;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = register_num (cpu, iw2);
      TRACE_EXTRACT (cpu, "POP %#x", iw2);
      TRACE_INSN (cpu, "POP R%i", num2);
      example_cpu->sp += 2;
      TRACE_REGISTER (cpu, "SP = %#x", example_cpu->sp);
      result = sim_core_read_aligned_2 (cpu, pc, read_map, example_cpu->sp);

      TRACE_REGISTER (cpu, "R%i = %#x", num2, result);
      example_cpu->regs[num2] = result;

      pc += 4;
    }
  else if (num1 == 4)
    {
      /* eq: 4 a b c: Set <a> to 1 if <b> is equal to <c>; set it to 0
	 otherwise.  */
      uint16_t iw2, iw3, iw4, num2, num3, num4, result;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = register_num (cpu, iw2);
      iw3 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 4);
      num3 = interp_num (cpu, iw3);
      iw4 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 6);
      num4 = interp_num (cpu, iw4);
      result = (num3 == num4);
      TRACE_EXTRACT (cpu, "EQ %#x %#x %#x", iw2, iw3, iw4);
      TRACE_INSN (cpu, "EQ R%i %#x %#x", num2, num3, num4);
      TRACE_DECODE (cpu, "R%i = (%#x == %#x) = %i", num2, num3, num4, result);

      TRACE_REGISTER (cpu, "R%i = %#x", num2, result);
      example_cpu->regs[num2] = result;

      pc += 8;
    }
  else if (num1 == 5)
    {
      /* gt: 5 a b c: Set <a> to 1 if <b> is greater than <c>; set it to 0
	 otherwise.  */
      uint16_t iw2, iw3, iw4, num2, num3, num4, result;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = register_num (cpu, iw2);
      iw3 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 4);
      num3 = interp_num (cpu, iw3);
      iw4 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 6);
      num4 = interp_num (cpu, iw4);
      result = (num3 > num4);
      TRACE_EXTRACT (cpu, "GT %#x %#x %#x", iw2, iw3, iw4);
      TRACE_INSN (cpu, "GT R%i %#x %#x", num2, num3, num4);
      TRACE_DECODE (cpu, "R%i = (%#x > %#x) = %i", num2, num3, num4, result);

      TRACE_REGISTER (cpu, "R%i = %#x", num2, result);
      example_cpu->regs[num2] = result;

      pc += 8;
    }
  else if (num1 == 6)
    {
      /* jmp: 6 a: Jump to <a>.  */
      uint16_t iw2, num2;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = interp_num (cpu, iw2);
      /* Addresses are 16-bit aligned.  */
      num2 <<= 1;
      TRACE_EXTRACT (cpu, "JMP %#x", iw2);
      TRACE_INSN (cpu, "JMP %#x", num2);

      pc = num2;
      TRACE_BRANCH (cpu, "JMP %#x", pc);
    }
  else if (num1 == 7)
    {
      /* jt: 7 a b: If <a> is nonzero, jump to <b>.  */
      uint16_t iw2, iw3, num2, num3;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = interp_num (cpu, iw2);
      iw3 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 4);
      num3 = interp_num (cpu, iw3);
      /* Addresses are 16-bit aligned.  */
      num3 <<= 1;
      TRACE_EXTRACT (cpu, "JT %#x %#x", iw2, iw3);
      TRACE_INSN (cpu, "JT %#x %#x", num2, num3);
      TRACE_DECODE (cpu, "JT %#x != 0 -> %s", num2, num2 ? "taken" : "nop");

      if (num2)
	{
	  pc = num3;
	  TRACE_BRANCH (cpu, "JT %#x", pc);
	}
      else
	pc += 6;
    }
  else if (num1 == 8)
    {
      /* jf: 8 a b: If <a> is zero, jump to <b>.  */
      uint16_t iw2, iw3, num2, num3;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = interp_num (cpu, iw2);
      iw3 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 4);
      num3 = interp_num (cpu, iw3);
      /* Addresses are 16-bit aligned.  */
      num3 <<= 1;
      TRACE_EXTRACT (cpu, "JF %#x %#x", iw2, iw3);
      TRACE_INSN (cpu, "JF %#x %#x", num2, num3);
      TRACE_DECODE (cpu, "JF %#x == 0 -> %s", num2, num2 ? "nop" : "taken");

      if (!num2)
	{
	  pc = num3;
	  TRACE_BRANCH (cpu, "JF %#x", pc);
	}
      else
	pc += 6;
    }
  else if (num1 == 9)
    {
      /* add: 9 a b c: Assign <a> the sum of <b> and <c> (modulo 32768).  */
      uint16_t iw2, iw3, iw4, num2, num3, num4, result;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = register_num (cpu, iw2);
      iw3 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 4);
      num3 = interp_num (cpu, iw3);
      iw4 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 6);
      num4 = interp_num (cpu, iw4);
      result = (num3 + num4) % 32768;
      TRACE_EXTRACT (cpu, "ADD %#x %#x %#x", iw2, iw3, iw4);
      TRACE_INSN (cpu, "ADD R%i %#x %#x", num2, num3, num4);
      TRACE_DECODE (cpu, "R%i = (%#x + %#x) %% %i = %#x", num2, num3, num4,
		    32768, result);

      TRACE_REGISTER (cpu, "R%i = %#x", num2, result);
      example_cpu->regs[num2] = result;

      pc += 8;
    }
  else if (num1 == 10)
    {
      /* mult: 10 a b c: Store into <a> the product of <b> and <c> (modulo
	 32768).  */
      uint16_t iw2, iw3, iw4, num2, num3, num4, result;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = register_num (cpu, iw2);
      iw3 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 4);
      num3 = interp_num (cpu, iw3);
      iw4 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 6);
      num4 = interp_num (cpu, iw4);
      result = (num3 * num4) % 32768;
      TRACE_EXTRACT (cpu, "MULT %#x %#x %#x", iw2, iw3, iw4);
      TRACE_INSN (cpu, "MULT R%i %#x %#x", num2, num3, num4);
      TRACE_DECODE (cpu, "R%i = (%#x * %#x) %% %i = %#x", num2, num3, num4,
		    32768, result);

      TRACE_REGISTER (cpu, "R%i = %#x", num2, result);
      example_cpu->regs[num2] = result;

      pc += 8;
    }
  else if (num1 == 11)
    {
      /* mod: 11 a b c: Store into <a> the remainder of <b> divided by <c>.  */
      uint16_t iw2, iw3, iw4, num2, num3, num4, result;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = register_num (cpu, iw2);
      iw3 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 4);
      num3 = interp_num (cpu, iw3);
      iw4 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 6);
      num4 = interp_num (cpu, iw4);
      result = num3 % num4;
      TRACE_EXTRACT (cpu, "MOD %#x %#x %#x", iw2, iw3, iw4);
      TRACE_INSN (cpu, "MOD R%i %#x %#x", num2, num3, num4);
      TRACE_DECODE (cpu, "R%i = %#x %% %#x = %#x", num2, num3, num4, result);

      TRACE_REGISTER (cpu, "R%i = %#x", num2, result);
      example_cpu->regs[num2] = result;

      pc += 8;
    }
  else if (num1 == 12)
    {
      /* and: 12 a b c: Stores into <a> the bitwise and of <b> and <c>.  */
      uint16_t iw2, iw3, iw4, num2, num3, num4, result;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = register_num (cpu, iw2);
      iw3 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 4);
      num3 = interp_num (cpu, iw3);
      iw4 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 6);
      num4 = interp_num (cpu, iw4);
      result = (num3 & num4);
      TRACE_EXTRACT (cpu, "AND %#x %#x %#x", iw2, iw3, iw4);
      TRACE_INSN (cpu, "AND R%i %#x %#x", num2, num3, num4);
      TRACE_DECODE (cpu, "R%i = %#x & %#x = %#x", num2, num3, num4, result);

      TRACE_REGISTER (cpu, "R%i = %#x", num2, result);
      example_cpu->regs[num2] = result;

      pc += 8;
    }
  else if (num1 == 13)
    {
      /* or: 13 a b c: Stores into <a> the bitwise or of <b> and <c>.  */
      uint16_t iw2, iw3, iw4, num2, num3, num4, result;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = register_num (cpu, iw2);
      iw3 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 4);
      num3 = interp_num (cpu, iw3);
      iw4 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 6);
      num4 = interp_num (cpu, iw4);
      result = (num3 | num4);
      TRACE_EXTRACT (cpu, "OR %#x %#x %#x", iw2, iw3, iw4);
      TRACE_INSN (cpu, "OR R%i %#x %#x", num2, num3, num4);
      TRACE_DECODE (cpu, "R%i = %#x | %#x = %#x", num2, num3, num4, result);

      TRACE_REGISTER (cpu, "R%i = %#x", num2, result);
      example_cpu->regs[num2] = result;

      pc += 8;
    }
  else if (num1 == 14)
    {
      /* not: 14 a b: Stores 15-bit bitwise inverse of <b> in <a>.  */
      uint16_t iw2, iw3, num2, num3, result;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = register_num (cpu, iw2);
      iw3 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 4);
      num3 = interp_num (cpu, iw3);
      result = (~num3) & 0x7fff;
      TRACE_EXTRACT (cpu, "NOT %#x %#x", iw2, iw3);
      TRACE_INSN (cpu, "NOT R%i %#x", num2, num3);
      TRACE_DECODE (cpu, "R%i = (~%#x) & 0x7fff = %#x", num2, num3, result);

      TRACE_REGISTER (cpu, "R%i = %#x", num2, result);
      example_cpu->regs[num2] = result;

      pc += 6;
    }
  else if (num1 == 15)
    {
      /* rmem: 15 a b: Read memory at address <b> and write it to <a>.  */
      uint16_t iw2, iw3, num2, num3, result;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = register_num (cpu, iw2);
      iw3 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 4);
      num3 = interp_num (cpu, iw3);
      /* Addresses are 16-bit aligned.  */
      num3 <<= 1;
      TRACE_EXTRACT (cpu, "RMEM %#x %#x", iw2, iw3);
      TRACE_INSN (cpu, "RMEM R%i %#x", num2, num3);

      TRACE_MEMORY (cpu, "reading %#x", num3);
      result = sim_core_read_aligned_2 (cpu, pc, read_map, num3);

      TRACE_REGISTER (cpu, "R%i = %#x", num2, result);
      example_cpu->regs[num2] = result;

      pc += 6;
    }
  else if (num1 == 16)
    {
      /* wmem: 16 a b: Write the value from <b> into memory at address <a>.  */
      uint16_t iw2, iw3, num2, num3;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = interp_num (cpu, iw2);
      iw3 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 4);
      num3 = interp_num (cpu, iw3);
      /* Addresses are 16-bit aligned.  */
      num2 <<= 1;
      TRACE_EXTRACT (cpu, "WMEM %#x %#x", iw2, iw3);
      TRACE_INSN (cpu, "WMEM %#x %#x", num2, num3);

      TRACE_MEMORY (cpu, "writing %#x to %#x", num3, num2);
      sim_core_write_aligned_2 (cpu, pc, write_map, num2, num3);

      pc += 6;
    }
  else if (num1 == 17)
    {
      /* call: 17 a: Write the address of the next instruction to the stack and
	 jump to <a>.  */
      uint16_t iw2, num2;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = interp_num (cpu, iw2);
      /* Addresses are 16-bit aligned.  */
      num2 <<= 1;
      TRACE_EXTRACT (cpu, "CALL %#x", iw2);
      TRACE_INSN (cpu, "CALL %#x", num2);

      TRACE_MEMORY (cpu, "pushing %#x onto stack", (pc + 4) >> 1);
      sim_core_write_aligned_2 (cpu, pc, write_map, example_cpu->sp, (pc + 4) >> 1);
      example_cpu->sp -= 2;
      TRACE_REGISTER (cpu, "SP = %#x", example_cpu->sp);

      pc = num2;
      TRACE_BRANCH (cpu, "CALL %#x", pc);
    }
  else if (num1 == 18)
    {
      /* ret: 18: Remove the top element from the stack and jump to it; empty
	 stack = halt.  */
      uint16_t result;

      TRACE_INSN (cpu, "RET");
      example_cpu->sp += 2;
      TRACE_REGISTER (cpu, "SP = %#x", example_cpu->sp);
      result = sim_core_read_aligned_2 (cpu, pc, read_map, example_cpu->sp);
      TRACE_MEMORY (cpu, "popping %#x off of stack", result << 1);

      pc = result << 1;
      TRACE_BRANCH (cpu, "RET -> %#x", pc);
    }
  else if (num1 == 19)
    {
      /* out: 19 a: Write the character <a> to the terminal.  */
      uint16_t iw2, num2;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = interp_num (cpu, iw2);
      TRACE_EXTRACT (cpu, "OUT %#x", iw2);
      TRACE_INSN (cpu, "OUT %#x", num2);
      TRACE_EVENTS (cpu, "write to stdout: %#x (%c)", num2, num2);

      sim_io_printf (sd, "%c", num2);

      pc += 4;
    }
  else if (num1 == 20)
    {
      /* in: 20 a: read a character from the terminal and write its ascii code
	 to <a>.  It can be assumed that once input starts, it will continue
	 until a newline is encountered.  This means that you can safely read
	 lines from the keyboard and trust that they will be fully read.  */
      uint16_t iw2, num2;
      char c;

      iw2 = sim_core_read_aligned_2 (cpu, pc, exec_map, pc + 2);
      num2 = register_num (cpu, iw2);
      TRACE_EXTRACT (cpu, "IN %#x", iw2);
      TRACE_INSN (cpu, "IN %#x", num2);
      sim_io_read_stdin (sd, &c, 1);
      TRACE_EVENTS (cpu, "read from stdin: %#x (%c)", c, c);

      /* The challenge uses lowercase for all inputs, so insert some low level
	 helpers of our own to make it a bit nicer.  */
      switch (c)
	{
	case 'Q':
	  sim_engine_halt (sd, cpu, NULL, pc, sim_exited, 0);
	  break;
	}

      TRACE_REGISTER (cpu, "R%i = %#x", iw2 & 0xf, c);
      example_cpu->regs[iw2 & 0xf] = c;

      pc += 4;
    }
  else if (num1 == 21)
    {
      /* noop: 21: no operation */
      TRACE_INSN (cpu, "NOOP");

      pc += 2;
    }
  else
    sim_engine_halt (sd, cpu, NULL, pc, sim_signalled, SIM_SIGILL);

  TRACE_REGISTER (cpu, "PC = %#x", pc);
  sim_pc_set (cpu, pc);
}

/* Return the program counter for this cpu. */
static sim_cia
pc_get (sim_cpu *cpu)
{
  struct example_sim_cpu *example_cpu = EXAMPLE_SIM_CPU (cpu);

  return example_cpu->pc;
}

/* Set the program counter for this cpu to the new pc value. */
static void
pc_set (sim_cpu *cpu, sim_cia pc)
{
  struct example_sim_cpu *example_cpu = EXAMPLE_SIM_CPU (cpu);

  example_cpu->pc = pc;
}

/* Initialize the state for a single cpu.  Usuaully this involves clearing all
   registers back to their reset state.  Should also hook up the fetch/store
   helper functions too.  */
void initialize_cpu (SIM_DESC sd, SIM_CPU *cpu)
{
  struct example_sim_cpu *example_cpu = EXAMPLE_SIM_CPU (cpu);

  memset (example_cpu->regs, 0, sizeof (example_cpu->regs));
  example_cpu->pc = 0;
  /* Make sure it's initialized outside of the 16-bit address space.  */
  example_cpu->sp = 0x80000;

  CPU_PC_FETCH (cpu) = pc_get;
  CPU_PC_STORE (cpu) = pc_set;
}
