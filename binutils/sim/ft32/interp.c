/* Simulator for the FT32 processor

   Copyright (C) 2008-2024 Free Software Foundation, Inc.
   Contributed by FTDI <support@ftdichip.com>

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

/* This must come before any other includes.  */
#include "defs.h"

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>

#include "bfd.h"
#include "sim/callback.h"
#include "libiberty.h"
#include "sim/sim.h"

#include "sim-main.h"
#include "sim-options.h"
#include "sim-signal.h"

#include "opcode/ft32.h"

#include "ft32-sim.h"

/*
 * FT32 is a Harvard architecture: RAM and code occupy
 * different address spaces.
 *
 * sim and gdb model FT32 memory by adding 0x800000 to RAM
 * addresses. This means that sim/gdb can treat all addresses
 * similarly.
 *
 * The address space looks like:
 *
 *    00000   start of code memory
 *    3ffff   end of code memory
 *   800000   start of RAM
 *   80ffff   end of RAM
 */

#define RAM_BIAS  0x800000  /* Bias added to RAM addresses.  */

static unsigned long
ft32_extract_unsigned_integer (const unsigned char *addr, int len)
{
  unsigned long retval;
  unsigned char *p;
  unsigned char *startaddr = (unsigned char *) addr;
  unsigned char *endaddr = startaddr + len;

  /* Start at the most significant end of the integer, and work towards
     the least significant.  */
  retval = 0;

  for (p = endaddr; p > startaddr;)
    retval = (retval << 8) | * -- p;

  return retval;
}

static void
ft32_store_unsigned_integer (unsigned char *addr, int len, unsigned long val)
{
  unsigned char *p;
  unsigned char *startaddr = (unsigned char *)addr;
  unsigned char *endaddr = startaddr + len;

  for (p = startaddr; p < endaddr; p++)
    {
      *p = val & 0xff;
      val >>= 8;
    }
}

/*
 * Align EA according to its size DW.
 * The FT32 ignores the low bit of a 16-bit addresss,
 * and the low two bits of a 32-bit address.
 */
static uint32_t ft32_align (uint32_t dw, uint32_t ea)
{
  switch (dw)
    {
    case 1:
      ea &= ~1;
      break;
    case 2:
      ea &= ~3;
      break;
    default:
      break;
    }
  return ea;
}

/* Read an item from memory address EA, sized DW.  */
static uint32_t
ft32_read_item (SIM_DESC sd, int dw, uint32_t ea)
{
  sim_cpu *cpu = STATE_CPU (sd, 0);
  address_word cia = CPU_PC_GET (cpu);

  ea = ft32_align (dw, ea);

  switch (dw) {
    case 0:
      return sim_core_read_aligned_1 (cpu, cia, read_map, ea);
    case 1:
      return sim_core_read_aligned_2 (cpu, cia, read_map, ea);
    case 2:
      return sim_core_read_aligned_4 (cpu, cia, read_map, ea);
    default:
      abort ();
  }
}

/* Write item V to memory address EA, sized DW.  */
static void
ft32_write_item (SIM_DESC sd, int dw, uint32_t ea, uint32_t v)
{
  sim_cpu *cpu = STATE_CPU (sd, 0);
  address_word cia = CPU_PC_GET (cpu);

  ea = ft32_align (dw, ea);

  switch (dw) {
    case 0:
      sim_core_write_aligned_1 (cpu, cia, write_map, ea, v);
      break;
    case 1:
      sim_core_write_aligned_2 (cpu, cia, write_map, ea, v);
      break;
    case 2:
      sim_core_write_aligned_4 (cpu, cia, write_map, ea, v);
      break;
    default:
      abort ();
  }
}

#define ILLEGAL() \
  sim_engine_halt (sd, cpu, NULL, insnpc, sim_signalled, SIM_SIGILL)

static uint32_t cpu_mem_read (SIM_DESC sd, uint32_t dw, uint32_t ea)
{
  sim_cpu *cpu = STATE_CPU (sd, 0);
  struct ft32_cpu_state *ft32_cpu = FT32_SIM_CPU (cpu);
  uint32_t insnpc = ft32_cpu->pc;

  ea &= 0x1ffff;
  if (ea & ~0xffff)
    {
      /* Simulate some IO devices */
      switch (ea)
	{
	case 0x10000:
	  return getchar ();
	case 0x1fff4:
	  /* Read the simulator cycle timer.  */
	  return ft32_cpu->cycles / 100;
	default:
	  sim_io_eprintf (sd, "Illegal IO read address %08x, pc %#x\n",
			  ea, insnpc);
	  ILLEGAL ();
	}
    }
  return ft32_read_item (sd, dw, RAM_BIAS + ea);
}

static void cpu_mem_write (SIM_DESC sd, uint32_t dw, uint32_t ea, uint32_t d)
{
  sim_cpu *cpu = STATE_CPU (sd, 0);
  struct ft32_cpu_state *ft32_cpu = FT32_SIM_CPU (cpu);
  ea &= 0x1ffff;
  if (ea & 0x10000)
    {
      /* Simulate some IO devices */
      switch (ea)
	{
	case 0x10000:
	  /* Console output */
	  putchar (d & 0xff);
	  break;
	case 0x1fc80:
	  /* Unlock the PM write port */
	  ft32_cpu->pm_unlock = (d == 0x1337f7d1);
	  break;
	case 0x1fc84:
	  /* Set the PM write address register */
	  ft32_cpu->pm_addr = d;
	  break;
	case 0x1fc88:
	  if (ft32_cpu->pm_unlock)
	    {
	      /* Write to PM.  */
	      ft32_write_item (sd, dw, ft32_cpu->pm_addr, d);
	      ft32_cpu->pm_addr += 4;
	    }
	  break;
	case 0x1fffc:
	  /* Normal exit.  */
	  sim_engine_halt (sd, cpu, NULL, ft32_cpu->pc, sim_exited, ft32_cpu->regs[0]);
	  break;
	case 0x1fff8:
	  sim_io_printf (sd, "Debug write %08x\n", d);
	  break;
	default:
	  sim_io_eprintf (sd, "Unknown IO write %08x to to %08x\n", d, ea);
	}
    }
  else
    ft32_write_item (sd, dw, RAM_BIAS + ea, d);
}

#define GET_BYTE(ea)    cpu_mem_read (sd, 0, (ea))
#define PUT_BYTE(ea, d) cpu_mem_write (sd, 0, (ea), (d))

/* LSBS (n) is a mask of the least significant N bits.  */
#define LSBS(n) ((1U << (n)) - 1)

static void ft32_push (SIM_DESC sd, uint32_t v)
{
  sim_cpu *cpu = STATE_CPU (sd, 0);
  struct ft32_cpu_state *ft32_cpu = FT32_SIM_CPU (cpu);
  ft32_cpu->regs[FT32_HARD_SP] -= 4;
  ft32_cpu->regs[FT32_HARD_SP] &= 0xffff;
  cpu_mem_write (sd, 2, ft32_cpu->regs[FT32_HARD_SP], v);
}

static uint32_t ft32_pop (SIM_DESC sd)
{
  sim_cpu *cpu = STATE_CPU (sd, 0);
  struct ft32_cpu_state *ft32_cpu = FT32_SIM_CPU (cpu);
  uint32_t r = cpu_mem_read (sd, 2, ft32_cpu->regs[FT32_HARD_SP]);
  ft32_cpu->regs[FT32_HARD_SP] += 4;
  ft32_cpu->regs[FT32_HARD_SP] &= 0xffff;
  return r;
}

/* Extract the low SIZ bits of N as an unsigned number.  */
static int nunsigned (int siz, int n)
{
  return n & LSBS (siz);
}

/* Extract the low SIZ bits of N as a signed number.  */
static int nsigned (int siz, int n)
{
  int shift = (sizeof (int) * 8) - siz;
  return (n << shift) >> shift;
}

/* Signed division N / D, matching hw behavior for (MIN_INT, -1).  */
static uint32_t ft32sdiv (uint32_t n, uint32_t d)
{
  if (n == 0x80000000UL && d == 0xffffffffUL)
    return 0x80000000UL;
  else
    return (uint32_t)((int)n / (int)d);
}

/* Signed modulus N % D, matching hw behavior for (MIN_INT, -1).  */
static uint32_t ft32smod (uint32_t n, uint32_t d)
{
  if (n == 0x80000000UL && d == 0xffffffffUL)
    return 0;
  else
    return (uint32_t)((int)n % (int)d);
}

/* Circular rotate right N by B bits.  */
static uint32_t ror (uint32_t n, uint32_t b)
{
  b &= 31;
  return (n >> b) | (n << (32 - b));
}

/* Implement the BINS machine instruction.
   See FT32 Programmer's Reference for details.  */
static uint32_t bins (uint32_t d, uint32_t f, uint32_t len, uint32_t pos)
{
  uint32_t bitmask = LSBS (len) << pos;
  return (d & ~bitmask) | ((f << pos) & bitmask);
}

/* Implement the FLIP machine instruction.
   See FT32 Programmer's Reference for details.  */
static uint32_t flip (uint32_t x, uint32_t b)
{
  if (b & 1)
    x = (x & 0x55555555) <<  1 | (x & 0xAAAAAAAA) >>  1;
  if (b & 2)
    x = (x & 0x33333333) <<  2 | (x & 0xCCCCCCCC) >>  2;
  if (b & 4)
    x = (x & 0x0F0F0F0F) <<  4 | (x & 0xF0F0F0F0) >>  4;
  if (b & 8)
    x = (x & 0x00FF00FF) <<  8 | (x & 0xFF00FF00) >>  8;
  if (b & 16)
    x = (x & 0x0000FFFF) << 16 | (x & 0xFFFF0000) >> 16;
  return x;
}

static void
step_once (SIM_DESC sd)
{
  sim_cpu *cpu = STATE_CPU (sd, 0);
  struct ft32_cpu_state *ft32_cpu = FT32_SIM_CPU (cpu);
  uint32_t inst;
  uint32_t dw;
  uint32_t cb;
  uint32_t r_d;
  uint32_t cr;
  uint32_t cv;
  uint32_t bt;
  uint32_t r_1;
  uint32_t rimm;
  uint32_t r_2;
  uint32_t k20;
  uint32_t pa;
  uint32_t aa;
  uint32_t k16;
  uint32_t k15;
  uint32_t al;
  uint32_t r_1v;
  uint32_t rimmv;
  uint32_t bit_pos;
  uint32_t bit_len;
  uint32_t upper;
  uint32_t insnpc;
  unsigned int sc[2];
  int isize;

  inst = ft32_read_item (sd, 2, ft32_cpu->pc);
  ft32_cpu->cycles += 1;

  if ((STATE_ARCHITECTURE (sd)->mach == bfd_mach_ft32b)
      && ft32_decode_shortcode (ft32_cpu->pc, inst, sc))
    {
      if ((ft32_cpu->pc & 3) == 0)
        inst = sc[0];
      else
        inst = sc[1];
      isize = 2;
    }
  else
    isize = 4;

  /* Handle "call 8" (which is FT32's "break" equivalent) here.  */
  if (inst == 0x00340002)
    {
      sim_engine_halt (sd, cpu, NULL,
		       ft32_cpu->pc,
		       sim_stopped, SIM_SIGTRAP);
      goto escape;
    }

  dw   =              (inst >> FT32_FLD_DW_BIT) & LSBS (FT32_FLD_DW_SIZ);
  cb   =              (inst >> FT32_FLD_CB_BIT) & LSBS (FT32_FLD_CB_SIZ);
  r_d  =              (inst >> FT32_FLD_R_D_BIT) & LSBS (FT32_FLD_R_D_SIZ);
  cr   =              (inst >> FT32_FLD_CR_BIT) & LSBS (FT32_FLD_CR_SIZ);
  cv   =              (inst >> FT32_FLD_CV_BIT) & LSBS (FT32_FLD_CV_SIZ);
  bt   =              (inst >> FT32_FLD_BT_BIT) & LSBS (FT32_FLD_BT_SIZ);
  r_1  =              (inst >> FT32_FLD_R_1_BIT) & LSBS (FT32_FLD_R_1_SIZ);
  rimm =              (inst >> FT32_FLD_RIMM_BIT) & LSBS (FT32_FLD_RIMM_SIZ);
  r_2  =              (inst >> FT32_FLD_R_2_BIT) & LSBS (FT32_FLD_R_2_SIZ);
  k20  = nsigned (20, (inst >> FT32_FLD_K20_BIT) & LSBS (FT32_FLD_K20_SIZ));
  pa   =              (inst >> FT32_FLD_PA_BIT) & LSBS (FT32_FLD_PA_SIZ);
  aa   =              (inst >> FT32_FLD_AA_BIT) & LSBS (FT32_FLD_AA_SIZ);
  k16  =              (inst >> FT32_FLD_K16_BIT) & LSBS (FT32_FLD_K16_SIZ);
  k15  =              (inst >> FT32_FLD_K15_BIT) & LSBS (FT32_FLD_K15_SIZ);
  if (k15 & 0x80)
    k15 ^= 0x7f00;
  if (k15 & 0x4000)
    k15 -= 0x8000;
  al   =              (inst >> FT32_FLD_AL_BIT) & LSBS (FT32_FLD_AL_SIZ);

  r_1v = ft32_cpu->regs[r_1];
  rimmv = (rimm & 0x400) ? nsigned (10, rimm) : ft32_cpu->regs[rimm & 0x1f];

  bit_pos = rimmv & 31;
  bit_len = 0xf & (rimmv >> 5);
  if (bit_len == 0)
    bit_len = 16;

  upper = (inst >> 27);

  insnpc = ft32_cpu->pc;
  ft32_cpu->pc += isize;
  switch (upper)
    {
    case FT32_PAT_TOC:
    case FT32_PAT_TOCI:
      {
	int take = (cr == 3) || ((1 & (ft32_cpu->regs[28 + cr] >> cb)) == cv);
	if (take)
	  {
	    ft32_cpu->cycles += 1;
	    if (bt)
	      ft32_push (sd, ft32_cpu->pc); /* this is a call.  */
	    if (upper == FT32_PAT_TOC)
	      ft32_cpu->pc = pa << 2;
	    else
	      ft32_cpu->pc = ft32_cpu->regs[r_2];
	    if (ft32_cpu->pc == 0x8)
		goto escape;
	  }
      }
      break;

    case FT32_PAT_ALUOP:
    case FT32_PAT_CMPOP:
      {
	uint32_t result;
	switch (al)
	  {
	  case 0x0: result = r_1v + rimmv; break;
	  case 0x1: result = ror (r_1v, rimmv); break;
	  case 0x2: result = r_1v - rimmv; break;
	  case 0x3: result = (r_1v << 10) | (1023 & rimmv); break;
	  case 0x4: result = r_1v & rimmv; break;
	  case 0x5: result = r_1v | rimmv; break;
	  case 0x6: result = r_1v ^ rimmv; break;
	  case 0x7: result = ~(r_1v ^ rimmv); break;
	  case 0x8: result = r_1v << rimmv; break;
	  case 0x9: result = r_1v >> rimmv; break;
	  case 0xa: result = (int32_t)r_1v >> rimmv; break;
	  case 0xb: result = bins (r_1v, rimmv >> 10, bit_len, bit_pos); break;
	  case 0xc: result = nsigned (bit_len, r_1v >> bit_pos); break;
	  case 0xd: result = nunsigned (bit_len, r_1v >> bit_pos); break;
	  case 0xe: result = flip (r_1v, rimmv); break;
	  default:
	    sim_io_eprintf (sd, "Unhandled alu %#x\n", al);
	    ILLEGAL ();
	  }
	if (upper == FT32_PAT_ALUOP)
	  ft32_cpu->regs[r_d] = result;
	else
	  {
	    uint32_t dwmask = 0;
	    int dwsiz = 0;
	    int zero;
	    int sign;
	    int ahi;
	    int bhi;
	    int overflow;
	    int carry;
	    int bit;
	    uint64_t ra;
	    uint64_t rb;
	    int above;
	    int greater;
	    int greatereq;

	    switch (dw)
	      {
	      case 0: dwsiz = 7;  dwmask = 0xffU; break;
	      case 1: dwsiz = 15; dwmask = 0xffffU; break;
	      case 2: dwsiz = 31; dwmask = 0xffffffffU; break;
	      }

	    zero = (0 == (result & dwmask));
	    sign = 1 & (result >> dwsiz);
	    ahi = 1 & (r_1v >> dwsiz);
	    bhi = 1 & (rimmv >> dwsiz);
	    overflow = (sign != ahi) & (ahi == !bhi);
	    bit = (dwsiz + 1);
	    ra = r_1v & dwmask;
	    rb = rimmv & dwmask;
	    switch (al)
	      {
	      case 0x0: carry = 1 & ((ra + rb) >> bit); break;
	      case 0x2: carry = 1 & ((ra - rb) >> bit); break;
	      default:  carry = 0; break;
	      }
	    above = (!carry & !zero);
	    greater = (sign == overflow) & !zero;
	    greatereq = (sign == overflow);

	    ft32_cpu->regs[r_d] = (
	      (above << 6) |
	      (greater << 5) |
	      (greatereq << 4) |
	      (sign << 3) |
	      (overflow << 2) |
	      (carry << 1) |
	      (zero << 0));
	  }
      }
      break;

    case FT32_PAT_LDK:
      ft32_cpu->regs[r_d] = k20;
      break;

    case FT32_PAT_LPM:
      ft32_cpu->regs[r_d] = ft32_read_item (sd, dw, pa << 2);
      ft32_cpu->cycles += 1;
      break;

    case FT32_PAT_LPMI:
      ft32_cpu->regs[r_d] = ft32_read_item (sd, dw, ft32_cpu->regs[r_1] + k15);
      ft32_cpu->cycles += 1;
      break;

    case FT32_PAT_STA:
      cpu_mem_write (sd, dw, aa, ft32_cpu->regs[r_d]);
      break;

    case FT32_PAT_STI:
      cpu_mem_write (sd, dw, ft32_cpu->regs[r_d] + k15, ft32_cpu->regs[r_1]);
      break;

    case FT32_PAT_LDA:
      ft32_cpu->regs[r_d] = cpu_mem_read (sd, dw, aa);
      ft32_cpu->cycles += 1;
      break;

    case FT32_PAT_LDI:
      ft32_cpu->regs[r_d] = cpu_mem_read (sd, dw, ft32_cpu->regs[r_1] + k15);
      ft32_cpu->cycles += 1;
      break;

    case FT32_PAT_EXA:
      {
	uint32_t tmp;
	tmp = cpu_mem_read (sd, dw, aa);
	cpu_mem_write (sd, dw, aa, ft32_cpu->regs[r_d]);
	ft32_cpu->regs[r_d] = tmp;
	ft32_cpu->cycles += 1;
      }
      break;

    case FT32_PAT_EXI:
      {
	uint32_t tmp;
	tmp = cpu_mem_read (sd, dw, ft32_cpu->regs[r_1] + k15);
	cpu_mem_write (sd, dw, ft32_cpu->regs[r_1] + k15, ft32_cpu->regs[r_d]);
	ft32_cpu->regs[r_d] = tmp;
	ft32_cpu->cycles += 1;
      }
      break;

    case FT32_PAT_PUSH:
      ft32_push (sd, r_1v);
      break;

    case FT32_PAT_LINK:
      ft32_push (sd, ft32_cpu->regs[r_d]);
      ft32_cpu->regs[r_d] = ft32_cpu->regs[FT32_HARD_SP];
      ft32_cpu->regs[FT32_HARD_SP] -= k16;
      ft32_cpu->regs[FT32_HARD_SP] &= 0xffff;
      break;

    case FT32_PAT_UNLINK:
      ft32_cpu->regs[FT32_HARD_SP] = ft32_cpu->regs[r_d];
      ft32_cpu->regs[FT32_HARD_SP] &= 0xffff;
      ft32_cpu->regs[r_d] = ft32_pop (sd);
      break;

    case FT32_PAT_POP:
      ft32_cpu->cycles += 1;
      ft32_cpu->regs[r_d] = ft32_pop (sd);
      break;

    case FT32_PAT_RETURN:
      ft32_cpu->pc = ft32_pop (sd);
      break;

    case FT32_PAT_FFUOP:
      switch (al)
	{
	case 0x0:
	  ft32_cpu->regs[r_d] = r_1v / rimmv;
	  break;
	case 0x1:
	  ft32_cpu->regs[r_d] = r_1v % rimmv;
	  break;
	case 0x2:
	  ft32_cpu->regs[r_d] = ft32sdiv (r_1v, rimmv);
	  break;
	case 0x3:
	  ft32_cpu->regs[r_d] = ft32smod (r_1v, rimmv);
	  break;

	case 0x4:
	  {
	    /* strcmp instruction.  */
	    uint32_t a = r_1v;
	    uint32_t b = rimmv;
	    uint32_t i = 0;
	    while ((GET_BYTE (a + i) != 0) &&
		   (GET_BYTE (a + i) == GET_BYTE (b + i)))
	      i++;
	    ft32_cpu->regs[r_d] = GET_BYTE (a + i) - GET_BYTE (b + i);
	  }
	  break;

	case 0x5:
	  {
	    /* memcpy instruction.  */
	    uint32_t src = r_1v;
	    uint32_t dst = ft32_cpu->regs[r_d];
	    uint32_t i;
	    for (i = 0; i < (rimmv & 0x7fff); i++)
	      PUT_BYTE (dst + i, GET_BYTE (src + i));
	  }
	  break;
	case 0x6:
	  {
	    /* strlen instruction.  */
	    uint32_t src = r_1v;
	    uint32_t i;
	    for (i = 0; GET_BYTE (src + i) != 0; i++)
	      ;
	    ft32_cpu->regs[r_d] = i;
	  }
	  break;
	case 0x7:
	  {
	    /* memset instruction.  */
	    uint32_t dst = ft32_cpu->regs[r_d];
	    uint32_t i;
	    for (i = 0; i < (rimmv & 0x7fff); i++)
	      PUT_BYTE (dst + i, r_1v);
	  }
	  break;
	case 0x8:
	  ft32_cpu->regs[r_d] = r_1v * rimmv;
	  break;
	case 0x9:
	  ft32_cpu->regs[r_d] = ((uint64_t)r_1v * (uint64_t)rimmv) >> 32;
	  break;
	case 0xa:
	  {
	    /* stpcpy instruction.  */
	    uint32_t src = r_1v;
	    uint32_t dst = ft32_cpu->regs[r_d];
	    uint32_t i;
	    for (i = 0; GET_BYTE (src + i) != 0; i++)
	      PUT_BYTE (dst + i, GET_BYTE (src + i));
	    PUT_BYTE (dst + i, 0);
	    ft32_cpu->regs[r_d] = dst + i;
	  }
	  break;
	case 0xe:
	  {
	    /* streamout instruction.  */
	    uint32_t i;
	    uint32_t src = ft32_cpu->regs[r_1];
	    for (i = 0; i < rimmv; i += (1 << dw))
	      {
		cpu_mem_write (sd,
			       dw,
			       ft32_cpu->regs[r_d],
			       cpu_mem_read (sd, dw, src));
		src += (1 << dw);
	      }
	  }
	  break;
	default:
	  sim_io_eprintf (sd, "Unhandled ffu %#x at %08x\n", al, insnpc);
	  ILLEGAL ();
	}
      break;

    default:
      sim_io_eprintf (sd, "Unhandled pattern %d at %08x\n", upper, insnpc);
      ILLEGAL ();
    }
  ft32_cpu->num_i++;

escape:
  ;
}

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr,  /* ignore  */
		int nr_cpus,      /* ignore  */
		int siggnal)      /* ignore  */
{
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  while (1)
    {
      step_once (sd);
      if (sim_events_tick (sd))
	sim_events_process (sd);
    }
}

static uint32_t *
ft32_lookup_register (SIM_CPU *cpu, int nr)
{
  /* Handle the register number translation here.
   * Sim registers are 0-31.
   * Other tools (gcc, gdb) use:
   * 0 - fp
   * 1 - sp
   * 2 - r0
   * 31 - cc
   */

  struct ft32_cpu_state *ft32_cpu = FT32_SIM_CPU (cpu);

  if ((nr < 0) || (nr > 32))
    {
      sim_io_eprintf (CPU_STATE (cpu), "unknown register %i\n", nr);
      abort ();
    }

  switch (nr)
    {
    case FT32_FP_REGNUM:
      return &ft32_cpu->regs[FT32_HARD_FP];
    case FT32_SP_REGNUM:
      return &ft32_cpu->regs[FT32_HARD_SP];
    case FT32_CC_REGNUM:
      return &ft32_cpu->regs[FT32_HARD_CC];
    case FT32_PC_REGNUM:
      return &ft32_cpu->pc;
    default:
      return &ft32_cpu->regs[nr - 2];
    }
}

static int
ft32_reg_store (SIM_CPU *cpu,
		int rn,
		const void *memory,
		int length)
{
  if (0 <= rn && rn <= 32)
    {
      if (length == 4)
	*ft32_lookup_register (cpu, rn) = ft32_extract_unsigned_integer (memory, 4);

      return 4;
    }
  else
    return 0;
}

static int
ft32_reg_fetch (SIM_CPU *cpu,
		int rn,
		void *memory,
		int length)
{
  if (0 <= rn && rn <= 32)
    {
      if (length == 4)
	ft32_store_unsigned_integer (memory, 4, *ft32_lookup_register (cpu, rn));

      return 4;
    }
  else
    return 0;
}

static sim_cia
ft32_pc_get (SIM_CPU *cpu)
{
  return FT32_SIM_CPU (cpu)->pc;
}

static void
ft32_pc_set (SIM_CPU *cpu, sim_cia newpc)
{
  FT32_SIM_CPU (cpu)->pc = newpc;
}

/* Cover function of sim_state_free to free the cpu buffers as well.  */

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

SIM_DESC
sim_open (SIM_OPEN_KIND kind,
	  host_callback *cb,
	  struct bfd *abfd,
	  char * const *argv)
{
  char c;
  size_t i;
  SIM_DESC sd = sim_state_alloc (kind, cb);

  /* Set default options before parsing user options.  */
  current_alignment = STRICT_ALIGNMENT;
  current_target_byte_order = BFD_ENDIAN_LITTLE;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct ft32_cpu_state))
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

  /* Allocate external memory if none specified by user.
     Use address 4 here in case the user wanted address 0 unmapped.  */
  if (sim_core_read_buffer (sd, NULL, read_map, &c, 4, 1) == 0)
    {
      sim_do_command (sd, "memory region 0x00000000,0x40000");
      sim_do_command (sd, "memory region 0x800000,0x10000");
    }

  /* Check for/establish the reference program image.  */
  if (sim_analyze_program (sd, STATE_PROG_FILE (sd), abfd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Configure/verify the target byte order and other runtime
     configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* CPU specific initialization.  */
  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    {
      SIM_CPU *cpu = STATE_CPU (sd, i);

      CPU_REG_FETCH (cpu) = ft32_reg_fetch;
      CPU_REG_STORE (cpu) = ft32_reg_store;
      CPU_PC_FETCH (cpu) = ft32_pc_get;
      CPU_PC_STORE (cpu) = ft32_pc_set;
    }

  return sd;
}

SIM_RC
sim_create_inferior (SIM_DESC sd,
		     struct bfd *abfd,
		     char * const *argv,
		     char * const *env)
{
  uint32_t addr;
  sim_cpu *cpu = STATE_CPU (sd, 0);
  struct ft32_cpu_state *ft32_cpu = FT32_SIM_CPU (cpu);
  host_callback *cb = STATE_CALLBACK (sd);

  /* Set the PC.  */
  if (abfd != NULL)
    addr = bfd_get_start_address (abfd);
  else
    addr = 0;

  /* Standalone mode (i.e. `run`) will take care of the argv for us in
     sim_open() -> sim_parse_args().  But in debug mode (i.e. 'target sim'
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

  ft32_cpu->regs[FT32_HARD_SP] = addr;
  ft32_cpu->num_i = 0;
  ft32_cpu->cycles = 0;
  ft32_cpu->next_tick_cycle = 100000;

  return SIM_RC_OK;
}
