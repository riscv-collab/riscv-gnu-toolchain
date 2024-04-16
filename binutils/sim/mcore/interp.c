/* Simulator for Motorola's MCore processor
   Copyright (C) 1999-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions.

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

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include "bfd.h"
#include "sim/callback.h"
#include "libiberty.h"
#include "sim/sim.h"

#include "sim-main.h"
#include "sim-base.h"
#include "sim-signal.h"
#include "sim-syscall.h"
#include "sim-options.h"

#include "target-newlib-syscall.h"

#include "mcore-sim.h"

#define target_big_endian (CURRENT_TARGET_BYTE_ORDER == BIG_ENDIAN)


static unsigned long
mcore_extract_unsigned_integer (const unsigned char *addr, int len)
{
  unsigned long retval;
  unsigned char * p;
  unsigned char * startaddr = (unsigned char *)addr;
  unsigned char * endaddr = startaddr + len;

  if (len > (int) sizeof (unsigned long))
    printf ("That operation is not available on integers of more than %zu bytes.",
	    sizeof (unsigned long));

  /* Start at the most significant end of the integer, and work towards
     the least significant.  */
  retval = 0;

  if (! target_big_endian)
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
mcore_store_unsigned_integer (unsigned char *addr, int len, unsigned long val)
{
  unsigned char * p;
  unsigned char * startaddr = (unsigned char *)addr;
  unsigned char * endaddr = startaddr + len;

  if (! target_big_endian)
    {
      for (p = startaddr; p < endaddr;)
	{
	  * p ++ = val & 0xff;
	  val >>= 8;
	}
    }
  else
    {
      for (p = endaddr; p > startaddr;)
	{
	  * -- p = val & 0xff;
	  val >>= 8;
	}
    }
}

static int memcycles = 1;

#define gr	MCORE_SIM_CPU (cpu)->active_gregs
#define cr	MCORE_SIM_CPU (cpu)->regs.cregs
#define sr	cr[0]
#define vbr	cr[1]
#define esr	cr[2]
#define fsr	cr[3]
#define epc	cr[4]
#define fpc	cr[5]
#define ss0	cr[6]
#define ss1	cr[7]
#define ss2	cr[8]
#define ss3	cr[9]
#define ss4	cr[10]
#define gcr	cr[11]
#define gsr	cr[12]

/* maniuplate the carry bit */
#define C_ON()		(sr & 1)
#define C_VALUE()	(sr & 1)
#define C_OFF()		((sr & 1) == 0)
#define SET_C()		{sr |= 1;}
#define CLR_C()		{sr &= 0xfffffffe;}
#define NEW_C(v)	{CLR_C(); sr |= ((v) & 1);}

#define SR_AF()		((sr >> 1) & 1)
static void set_active_regs (SIM_CPU *cpu)
{
  struct mcore_sim_cpu *mcore_cpu = MCORE_SIM_CPU (cpu);

  if (SR_AF())
    mcore_cpu->active_gregs = mcore_cpu->regs.alt_gregs;
  else
    mcore_cpu->active_gregs = mcore_cpu->regs.gregs;
}

#define	TRAPCODE	1	/* r1 holds which function we want */
#define	PARM1	2		/* first parameter  */
#define	PARM2	3
#define	PARM3	4
#define	PARM4	5
#define	RET1	2		/* register for return values. */

/* Default to a 8 Mbyte (== 2^23) memory space.  */
#define DEFAULT_MEMORY_SIZE 0x800000

static void
set_initial_gprs (SIM_CPU *cpu)
{
  struct mcore_sim_cpu *mcore_cpu = MCORE_SIM_CPU (cpu);

  /* Set up machine just out of reset.  */
  CPU_PC_SET (cpu, 0);
  sr = 0;

  /* Clean out the GPRs and alternate GPRs.  */
  memset (&mcore_cpu->regs.gregs, 0, sizeof(mcore_cpu->regs.gregs));
  memset (&mcore_cpu->regs.alt_gregs, 0, sizeof(mcore_cpu->regs.alt_gregs));

  /* Make our register set point to the right place.  */
  set_active_regs (cpu);

  /* ABI specifies initial values for these registers.  */
  gr[0] = DEFAULT_MEMORY_SIZE - 4;

  /* dac fix, the stack address must be 8-byte aligned! */
  gr[0] = gr[0] - gr[0] % 8;
  gr[PARM1] = 0;
  gr[PARM2] = 0;
  gr[PARM3] = 0;
  gr[PARM4] = gr[0];
}

/* Simulate a monitor trap.  */

static void
handle_trap1 (SIM_DESC sd, SIM_CPU *cpu)
{
  /* XXX: We don't pass back the actual errno value.  */
  gr[RET1] = sim_syscall (cpu, gr[TRAPCODE], gr[PARM1], gr[PARM2], gr[PARM3],
			  gr[PARM4]);
}

static void
process_stub (SIM_DESC sd, SIM_CPU *cpu, int what)
{
  /* These values should match those in libgloss/mcore/syscalls.s.  */
  switch (what)
    {
    case 3:  /* _read */
    case 4:  /* _write */
    case 5:  /* _open */
    case 6:  /* _close */
    case 10: /* _unlink */
    case 19: /* _lseek */
    case 43: /* _times */
      gr[TRAPCODE] = what;
      handle_trap1 (sd, cpu);
      break;

    default:
      if (STATE_VERBOSE_P (sd))
	fprintf (stderr, "Unhandled stub opcode: %d\n", what);
      break;
    }
}

static void
util (SIM_DESC sd, SIM_CPU *cpu, unsigned what)
{
  struct mcore_sim_cpu *mcore_cpu = MCORE_SIM_CPU (cpu);

  switch (what)
    {
    case 0:	/* exit */
      sim_engine_halt (sd, cpu, NULL, mcore_cpu->regs.pc, sim_exited, gr[PARM1]);
      break;

    case 1:	/* printf */
      if (STATE_VERBOSE_P (sd))
	fprintf (stderr, "WARNING: printf unimplemented\n");
      break;

    case 2:	/* scanf */
      if (STATE_VERBOSE_P (sd))
	fprintf (stderr, "WARNING: scanf unimplemented\n");
      break;

    case 3:	/* utime */
      gr[RET1] = mcore_cpu->insts;
      break;

    case 0xFF:
      process_stub (sd, cpu, gr[1]);
      break;

    default:
      if (STATE_VERBOSE_P (sd))
	fprintf (stderr, "Unhandled util code: %x\n", what);
      break;
    }
}

/* For figuring out whether we carried; addc/subc use this. */
static int
iu_carry (unsigned long a, unsigned long b, int cin)
{
  unsigned long	x;

  x = (a & 0xffff) + (b & 0xffff) + cin;
  x = (x >> 16) + (a >> 16) + (b >> 16);
  x >>= 16;

  return (x != 0);
}

/* TODO: Convert to common watchpoints.  */
#undef WATCHFUNCTIONS
#ifdef WATCHFUNCTIONS

#define MAXWL 80
int32_t WL[MAXWL];
char * WLstr[MAXWL];

int ENDWL=0;
int WLincyc;
int WLcyc[MAXWL];
int WLcnts[MAXWL];
int WLmax[MAXWL];
int WLmin[MAXWL];
int32_t WLendpc;
int WLbcyc;
int WLW;
#endif

#define RD	(inst        & 0xF)
#define RS	((inst >> 4) & 0xF)
#define RX	((inst >> 8) & 0xF)
#define IMM5	((inst >> 4) & 0x1F)
#define IMM4	((inst) & 0xF)

#define rbat(X)	sim_core_read_1 (cpu, 0, read_map, X)
#define rhat(X)	sim_core_read_2 (cpu, 0, read_map, X)
#define rlat(X)	sim_core_read_4 (cpu, 0, read_map, X)
#define wbat(X, D) sim_core_write_1 (cpu, 0, write_map, X, D)
#define what(X, D) sim_core_write_2 (cpu, 0, write_map, X, D)
#define wlat(X, D) sim_core_write_4 (cpu, 0, write_map, X, D)

static int tracing = 0;

#define ILLEGAL() \
  sim_engine_halt (sd, cpu, NULL, pc, sim_stopped, SIM_SIGILL)

static void
step_once (SIM_DESC sd, SIM_CPU *cpu)
{
  struct mcore_sim_cpu *mcore_cpu = MCORE_SIM_CPU (cpu);
  int needfetch;
  int32_t ibuf;
  int32_t pc;
  unsigned short inst;
  int memops;
  int bonus_cycles;
  int insts;
#ifdef WATCHFUNCTIONS
  int w;
  int32_t WLhash;
#endif

  pc = CPU_PC_GET (cpu);

  /* Fetch the initial instructions that we'll decode. */
  ibuf = rlat (pc & 0xFFFFFFFC);
  needfetch = 0;

  memops = 0;
  bonus_cycles = 0;
  insts = 0;

  /* make our register set point to the right place */
  set_active_regs (cpu);

#ifdef WATCHFUNCTIONS
  /* make a hash to speed exec loop, hope it's nonzero */
  WLhash = 0xFFFFFFFF;

  for (w = 1; w <= ENDWL; w++)
    WLhash = WLhash & WL[w];
#endif

  /* TODO: Unindent this block.  */
    {
      insts ++;

      if (pc & 02)
	{
	  if (! target_big_endian)
	    inst = ibuf >> 16;
	  else
	    inst = ibuf & 0xFFFF;
	  needfetch = 1;
	}
      else
	{
	  if (! target_big_endian)
	    inst = ibuf & 0xFFFF;
	  else
	    inst = ibuf >> 16;
	}

#ifdef WATCHFUNCTIONS
      /* now scan list of watch addresses, if match, count it and
	 note return address and count cycles until pc=return address */

      if ((WLincyc == 1) && (pc == WLendpc))
	{
	  int cycs = (mcore_cpu->cycles + (insts + bonus_cycles +
					   (memops * memcycles)) - WLbcyc);

	  if (WLcnts[WLW] == 1)
	    {
	      WLmax[WLW] = cycs;
	      WLmin[WLW] = cycs;
	      WLcyc[WLW] = 0;
	    }

	  if (cycs > WLmax[WLW])
	    {
	      WLmax[WLW] = cycs;
	    }

	  if (cycs < WLmin[WLW])
	    {
	      WLmin[WLW] = cycs;
	    }

	  WLcyc[WLW] += cycs;
	  WLincyc = 0;
	  WLendpc = 0;
	}

      /* Optimize with a hash to speed loop.  */
      if (WLincyc == 0)
	{
          if ((WLhash == 0) || ((WLhash & pc) != 0))
	    {
	      for (w=1; w <= ENDWL; w++)
		{
		  if (pc == WL[w])
		    {
		      WLcnts[w]++;
		      WLbcyc = mcore_cpu->cycles + insts
			+ bonus_cycles + (memops * memcycles);
		      WLendpc = gr[15];
		      WLincyc = 1;
		      WLW = w;
		      break;
		    }
		}
	    }
	}
#endif

      if (tracing)
	fprintf (stderr, "%.4x: inst = %.4x ", pc, inst);

      pc += 2;

      switch (inst >> 8)
	{
	case 0x00:
	  switch RS
	    {
	    case 0x0:
	      switch RD
		{
		case 0x0:				/* bkpt */
		  pc -= 2;
		  sim_engine_halt (sd, cpu, NULL, pc - 2,
				   sim_stopped, SIM_SIGTRAP);
		  break;

		case 0x1:				/* sync */
		  break;

		case 0x2:				/* rte */
		  pc = epc;
		  sr = esr;
		  needfetch = 1;

		  set_active_regs (cpu);
		  break;

		case 0x3:				/* rfi */
		  pc = fpc;
		  sr = fsr;
		  needfetch = 1;

		  set_active_regs (cpu);
		  break;

		case 0x4:				/* stop */
		  if (STATE_VERBOSE_P (sd))
		    fprintf (stderr, "WARNING: stop unimplemented\n");
		  break;

		case 0x5:				/* wait */
		  if (STATE_VERBOSE_P (sd))
		    fprintf (stderr, "WARNING: wait unimplemented\n");
		  break;

		case 0x6:				/* doze */
		  if (STATE_VERBOSE_P (sd))
		    fprintf (stderr, "WARNING: doze unimplemented\n");
		  break;

		case 0x7:
		  ILLEGAL ();				/* illegal */
		  break;

		case 0x8:				/* trap 0 */
		case 0xA:				/* trap 2 */
		case 0xB:				/* trap 3 */
		  sim_engine_halt (sd, cpu, NULL, pc,
				   sim_stopped, SIM_SIGTRAP);
		  break;

		case 0xC:				/* trap 4 */
		case 0xD:				/* trap 5 */
		case 0xE:				/* trap 6 */
		  ILLEGAL ();				/* illegal */
		  break;

		case 0xF: 				/* trap 7 */
		  sim_engine_halt (sd, cpu, NULL, pc,	/* integer div-by-0 */
				   sim_stopped, SIM_SIGTRAP);
		  break;

		case 0x9:				/* trap 1 */
		  handle_trap1 (sd, cpu);
		  break;
		}
	      break;

	    case 0x1:
	      ILLEGAL ();				/* illegal */
	      break;

	    case 0x2:					/* mvc */
	      gr[RD] = C_VALUE();
	      break;
	    case 0x3:					/* mvcv */
	      gr[RD] = C_OFF();
	      break;
	    case 0x4:					/* ldq */
	      {
		int32_t addr = gr[RD];
		int regno = 4;			/* always r4-r7 */

		bonus_cycles++;
		memops += 4;
		do
		  {
		    gr[regno] = rlat (addr);
		    addr += 4;
		    regno++;
		  }
		while ((regno&0x3) != 0);
	      }
	      break;
	    case 0x5:					/* stq */
	      {
		int32_t addr = gr[RD];
		int regno = 4;			/* always r4-r7 */

		memops += 4;
		bonus_cycles++;
		do
		  {
		    wlat (addr, gr[regno]);
		    addr += 4;
		    regno++;
		  }
		while ((regno & 0x3) != 0);
	      }
	      break;
	    case 0x6:					/* ldm */
	      {
		int32_t addr = gr[0];
		int regno = RD;

		/* bonus cycle is really only needed if
		   the next insn shifts the last reg loaded.

		   bonus_cycles++;
		*/
		memops += 16-regno;
		while (regno <= 0xF)
		  {
		    gr[regno] = rlat (addr);
		    addr += 4;
		    regno++;
		  }
	      }
	      break;
	    case 0x7:					/* stm */
	      {
		int32_t addr = gr[0];
		int regno = RD;

		/* this should be removed! */
		/*  bonus_cycles ++; */

		memops += 16 - regno;
		while (regno <= 0xF)
		  {
		    wlat (addr, gr[regno]);
		    addr += 4;
		    regno++;
		  }
	      }
	      break;

	    case 0x8:					/* dect */
	      gr[RD] -= C_VALUE();
	      break;
	    case 0x9:					/* decf */
	      gr[RD] -= C_OFF();
	      break;
	    case 0xA:					/* inct */
	      gr[RD] += C_VALUE();
	      break;
	    case 0xB:					/* incf */
	      gr[RD] += C_OFF();
	      break;
	    case 0xC:					/* jmp */
	      pc = gr[RD];
	      if (tracing && RD == 15)
		fprintf (stderr, "Func return, r2 = %xx, r3 = %x\n",
			 gr[2], gr[3]);
	      bonus_cycles++;
	      needfetch = 1;
	      break;
	    case 0xD:					/* jsr */
	      gr[15] = pc;
	      pc = gr[RD];
	      bonus_cycles++;
	      needfetch = 1;
	      break;
	    case 0xE:					/* ff1 */
	      {
		int32_t tmp, i;
		tmp = gr[RD];
		for (i = 0; !(tmp & 0x80000000) && i < 32; i++)
		  tmp <<= 1;
		gr[RD] = i;
	      }
	      break;
	    case 0xF:					/* brev */
	      {
		int32_t tmp;
		tmp = gr[RD];
		tmp = ((tmp & 0xaaaaaaaa) >>  1) | ((tmp & 0x55555555) <<  1);
		tmp = ((tmp & 0xcccccccc) >>  2) | ((tmp & 0x33333333) <<  2);
		tmp = ((tmp & 0xf0f0f0f0) >>  4) | ((tmp & 0x0f0f0f0f) <<  4);
		tmp = ((tmp & 0xff00ff00) >>  8) | ((tmp & 0x00ff00ff) <<  8);
		gr[RD] = ((tmp & 0xffff0000) >> 16) | ((tmp & 0x0000ffff) << 16);
	      }
	      break;
	    }
	  break;
	case 0x01:
	  switch RS
	    {
	    case 0x0:					/* xtrb3 */
	      gr[1] = (gr[RD]) & 0xFF;
	      NEW_C (gr[RD] != 0);
	      break;
	    case 0x1:					/* xtrb2 */
	      gr[1] = (gr[RD]>>8) & 0xFF;
	      NEW_C (gr[RD] != 0);
	      break;
	    case 0x2:					/* xtrb1 */
	      gr[1] = (gr[RD]>>16) & 0xFF;
	      NEW_C (gr[RD] != 0);
	      break;
	    case 0x3:					/* xtrb0 */
	      gr[1] = (gr[RD]>>24) & 0xFF;
	      NEW_C (gr[RD] != 0);
	      break;
	    case 0x4:					/* zextb */
	      gr[RD] &= 0x000000FF;
	      break;
	    case 0x5:					/* sextb */
	      {
		long tmp;
		tmp = gr[RD];
		tmp <<= (sizeof (tmp) * 8) - 8;
		tmp >>= (sizeof (tmp) * 8) - 8;
		gr[RD] = tmp;
	      }
	      break;
	    case 0x6:					/* zexth */
	      gr[RD] &= 0x0000FFFF;
	      break;
	    case 0x7:					/* sexth */
	      {
		long tmp;
		tmp = gr[RD];
		tmp <<= (sizeof (tmp) * 8) - 16;
		tmp >>= (sizeof (tmp) * 8) - 16;
		gr[RD] = tmp;
	      }
	      break;
	    case 0x8:					/* declt */
	      --gr[RD];
	      NEW_C ((long)gr[RD] < 0);
	      break;
	    case 0x9:					/* tstnbz */
	      {
		int32_t tmp = gr[RD];
		NEW_C ((tmp & 0xFF000000) != 0 &&
		       (tmp & 0x00FF0000) != 0 && (tmp & 0x0000FF00) != 0 &&
		       (tmp & 0x000000FF) != 0);
	      }
	      break;
	    case 0xA:					/* decgt */
	      --gr[RD];
	      NEW_C ((long)gr[RD] > 0);
	      break;
	    case 0xB:					/* decne */
	      --gr[RD];
	      NEW_C ((long)gr[RD] != 0);
	      break;
	    case 0xC:					/* clrt */
	      if (C_ON())
		gr[RD] = 0;
	      break;
	    case 0xD:					/* clrf */
	      if (C_OFF())
		gr[RD] = 0;
	      break;
	    case 0xE:					/* abs */
	      if (gr[RD] & 0x80000000)
		gr[RD] = ~gr[RD] + 1;
	      break;
	    case 0xF:					/* not */
	      gr[RD] = ~gr[RD];
	      break;
	    }
	  break;
	case 0x02:					/* movt */
	  if (C_ON())
	    gr[RD] = gr[RS];
	  break;
	case 0x03:					/* mult */
	  /* consume 2 bits per cycle from rs, until rs is 0 */
	  {
	    unsigned int t = gr[RS];
	    int ticks;
	    for (ticks = 0; t != 0 ; t >>= 2)
	      ticks++;
	    bonus_cycles += ticks;
	  }
	  bonus_cycles += 2;  /* min. is 3, so add 2, plus ticks above */
	  if (tracing)
	    fprintf (stderr, "  mult %x by %x to give %x",
		     gr[RD], gr[RS], gr[RD] * gr[RS]);
	  gr[RD] = gr[RD] * gr[RS];
	  break;
	case 0x04:					/* loopt */
	  if (C_ON())
	    {
	      pc += (IMM4 << 1) - 32;
	      bonus_cycles ++;
	      needfetch = 1;
	    }
	  --gr[RS];				/* not RD! */
	  NEW_C (((long)gr[RS]) > 0);
	  break;
	case 0x05:					/* subu */
	  gr[RD] -= gr[RS];
	  break;
	case 0x06:					/* addc */
	  {
	    unsigned long tmp, a, b;
	    a = gr[RD];
	    b = gr[RS];
	    gr[RD] = a + b + C_VALUE ();
	    tmp = iu_carry (a, b, C_VALUE ());
	    NEW_C (tmp);
	  }
	  break;
	case 0x07:					/* subc */
	  {
	    unsigned long tmp, a, b;
	    a = gr[RD];
	    b = gr[RS];
	    gr[RD] = a - b + C_VALUE () - 1;
	    tmp = iu_carry (a,~b, C_VALUE ());
	    NEW_C (tmp);
	  }
	  break;
	case 0x08:					/* illegal */
	case 0x09:					/* illegal*/
	  ILLEGAL ();
	  break;
	case 0x0A:					/* movf */
	  if (C_OFF())
	    gr[RD] = gr[RS];
	  break;
	case 0x0B:					/* lsr */
	  {
	    uint32_t dst, src;
	    dst = gr[RD];
	    src = gr[RS];
	    /* We must not rely solely upon the native shift operations, since they
	       may not match the M*Core's behaviour on boundary conditions.  */
	    dst = src > 31 ? 0 : dst >> src;
	    gr[RD] = dst;
	  }
	  break;
	case 0x0C:					/* cmphs */
	  NEW_C ((unsigned long )gr[RD] >=
		 (unsigned long)gr[RS]);
	  break;
	case 0x0D:					/* cmplt */
	  NEW_C ((long)gr[RD] < (long)gr[RS]);
	  break;
	case 0x0E:					/* tst */
	  NEW_C ((gr[RD] & gr[RS]) != 0);
	  break;
	case 0x0F:					/* cmpne */
	  NEW_C (gr[RD] != gr[RS]);
	  break;
	case 0x10: case 0x11:				/* mfcr */
	  {
	    unsigned r;
	    r = IMM5;
	    if (r <= LAST_VALID_CREG)
	      gr[RD] = cr[r];
	    else
	      ILLEGAL ();
	  }
	  break;

	case 0x12:					/* mov */
	  gr[RD] = gr[RS];
	  if (tracing)
	    fprintf (stderr, "MOV %x into reg %d", gr[RD], RD);
	  break;

	case 0x13:					/* bgenr */
	  if (gr[RS] & 0x20)
	    gr[RD] = 0;
	  else
	    gr[RD] = 1 << (gr[RS] & 0x1F);
	  break;

	case 0x14:					/* rsub */
	  gr[RD] = gr[RS] - gr[RD];
	  break;

	case 0x15:					/* ixw */
	  gr[RD] += gr[RS]<<2;
	  break;

	case 0x16:					/* and */
	  gr[RD] &= gr[RS];
	  break;

	case 0x17:					/* xor */
	  gr[RD] ^= gr[RS];
	  break;

	case 0x18: case 0x19:				/* mtcr */
	  {
	    unsigned r;
	    r = IMM5;
	    if (r <= LAST_VALID_CREG)
	      cr[r] = gr[RD];
	    else
	      ILLEGAL ();

	    /* we might have changed register sets... */
	    set_active_regs (cpu);
	  }
	  break;

	case 0x1A:					/* asr */
	  /* We must not rely solely upon the native shift operations, since they
	     may not match the M*Core's behaviour on boundary conditions.  */
	  if (gr[RS] > 30)
	    gr[RD] = ((long) gr[RD]) < 0 ? -1 : 0;
	  else
	    gr[RD] = (long) gr[RD] >> gr[RS];
	  break;

	case 0x1B:					/* lsl */
	  /* We must not rely solely upon the native shift operations, since they
	     may not match the M*Core's behaviour on boundary conditions.  */
	  gr[RD] = gr[RS] > 31 ? 0 : gr[RD] << gr[RS];
	  break;

	case 0x1C:					/* addu */
	  gr[RD] += gr[RS];
	  break;

	case 0x1D:					/* ixh */
	  gr[RD] += gr[RS] << 1;
	  break;

	case 0x1E:					/* or */
	  gr[RD] |= gr[RS];
	  break;

	case 0x1F:					/* andn */
	  gr[RD] &= ~gr[RS];
	  break;
	case 0x20: case 0x21:				/* addi */
	  gr[RD] =
	    gr[RD] + (IMM5 + 1);
	  break;
	case 0x22: case 0x23:				/* cmplti */
	  {
	    int tmp = (IMM5 + 1);
	    if (gr[RD] < tmp)
	      {
	        SET_C();
	      }
	    else
	      {
	        CLR_C();
	      }
	  }
	  break;
	case 0x24: case 0x25:				/* subi */
	  gr[RD] =
	    gr[RD] - (IMM5 + 1);
	  break;
	case 0x26: case 0x27:				/* illegal */
	  ILLEGAL ();
	  break;
	case 0x28: case 0x29:				/* rsubi */
	  gr[RD] =
	    IMM5 - gr[RD];
	  break;
	case 0x2A: case 0x2B:				/* cmpnei */
	  if (gr[RD] != IMM5)
	    {
	      SET_C();
	    }
	  else
	    {
	      CLR_C();
	    }
	  break;

	case 0x2C: case 0x2D:				/* bmaski, divu */
	  {
	    unsigned imm = IMM5;

	    if (imm == 1)
	      {
		int exe;
		int rxnlz, r1nlz;
		unsigned int rx, r1;

		rx = gr[RD];
		r1 = gr[1];
		exe = 0;

		/* unsigned divide */
		gr[RD] = (int32_t) ((unsigned int) gr[RD] / (unsigned int)gr[1] );

		/* compute bonus_cycles for divu */
		for (r1nlz = 0; ((r1 & 0x80000000) == 0) && (r1nlz < 32); r1nlz ++)
		  r1 = r1 << 1;

		for (rxnlz = 0; ((rx & 0x80000000) == 0) && (rxnlz < 32); rxnlz ++)
		  rx = rx << 1;

		if (r1nlz < rxnlz)
		  exe += 4;
		else
		  exe += 5 + r1nlz - rxnlz;

		if (exe >= (2 * memcycles - 1))
		  {
		    bonus_cycles += exe - (2 * memcycles) + 1;
		  }
	      }
	    else if (imm == 0 || imm >= 8)
	      {
		/* bmaski */
		if (imm == 0)
		  gr[RD] = -1;
		else
		  gr[RD] = (1 << imm) - 1;
	      }
	    else
	      {
		/* illegal */
		ILLEGAL ();
	      }
	  }
	  break;
	case 0x2E: case 0x2F:				/* andi */
	  gr[RD] = gr[RD] & IMM5;
	  break;
	case 0x30: case 0x31:				/* bclri */
	  gr[RD] = gr[RD] & ~(1<<IMM5);
	  break;
	case 0x32: case 0x33:				/* bgeni, divs */
	  {
	    unsigned imm = IMM5;
	    if (imm == 1)
	      {
		int exe,sc;
		int rxnlz, r1nlz;
		signed int rx, r1;

		/* compute bonus_cycles for divu */
		rx = gr[RD];
		r1 = gr[1];
		exe = 0;

		if (((rx < 0) && (r1 > 0)) || ((rx >= 0) && (r1 < 0)))
		  sc = 1;
		else
		  sc = 0;

		rx = abs (rx);
		r1 = abs (r1);

		/* signed divide, general registers are of type int, so / op is OK */
		gr[RD] = gr[RD] / gr[1];

		for (r1nlz = 0; ((r1 & 0x80000000) == 0) && (r1nlz < 32) ; r1nlz ++ )
		  r1 = r1 << 1;

		for (rxnlz = 0; ((rx & 0x80000000) == 0) && (rxnlz < 32) ; rxnlz ++ )
		  rx = rx << 1;

		if (r1nlz < rxnlz)
		  exe += 5;
		else
		  exe += 6 + r1nlz - rxnlz + sc;

		if (exe >= (2 * memcycles - 1))
		  {
		    bonus_cycles += exe - (2 * memcycles) + 1;
		  }
	      }
	    else if (imm >= 7)
	      {
		/* bgeni */
		gr[RD] = (1 << IMM5);
	      }
	    else
	      {
		/* illegal */
		ILLEGAL ();
	      }
	    break;
	  }
	case 0x34: case 0x35:				/* bseti */
	  gr[RD] = gr[RD] | (1 << IMM5);
	  break;
	case 0x36: case 0x37:				/* btsti */
	  NEW_C (gr[RD] >> IMM5);
	  break;
	case 0x38: case 0x39:				/* xsr, rotli */
	  {
	    unsigned imm = IMM5;
	    uint32_t tmp = gr[RD];
	    if (imm == 0)
	      {
		int32_t cbit;
		cbit = C_VALUE();
		NEW_C (tmp);
		gr[RD] = (cbit << 31) | (tmp >> 1);
	      }
	    else
	      gr[RD] = (tmp << imm) | (tmp >> (32 - imm));
	  }
	  break;
	case 0x3A: case 0x3B:				/* asrc, asri */
	  {
	    unsigned imm = IMM5;
	    long tmp = gr[RD];
	    if (imm == 0)
	      {
		NEW_C (tmp);
		gr[RD] = tmp >> 1;
	      }
	    else
	      gr[RD] = tmp >> imm;
	  }
	  break;
	case 0x3C: case 0x3D:				/* lslc, lsli */
	  {
	    unsigned imm = IMM5;
	    unsigned long tmp = gr[RD];
	    if (imm == 0)
	      {
		NEW_C (tmp >> 31);
		gr[RD] = tmp << 1;
	      }
	    else
	      gr[RD] = tmp << imm;
	  }
	  break;
	case 0x3E: case 0x3F:				/* lsrc, lsri */
	  {
	    unsigned imm = IMM5;
	    uint32_t tmp = gr[RD];
	    if (imm == 0)
	      {
		NEW_C (tmp);
		gr[RD] = tmp >> 1;
	      }
	    else
	      gr[RD] = tmp >> imm;
	  }
	  break;
	case 0x40: case 0x41: case 0x42: case 0x43:
	case 0x44: case 0x45: case 0x46: case 0x47:
	case 0x48: case 0x49: case 0x4A: case 0x4B:
	case 0x4C: case 0x4D: case 0x4E: case 0x4F:
	  ILLEGAL ();
	  break;
	case 0x50:
	  util (sd, cpu, inst & 0xFF);
	  break;
	case 0x51: case 0x52: case 0x53:
	case 0x54: case 0x55: case 0x56: case 0x57:
	case 0x58: case 0x59: case 0x5A: case 0x5B:
	case 0x5C: case 0x5D: case 0x5E: case 0x5F:
	  ILLEGAL ();
	  break;
	case 0x60: case 0x61: case 0x62: case 0x63:	/* movi  */
	case 0x64: case 0x65: case 0x66: case 0x67:
	  gr[RD] = (inst >> 4) & 0x7F;
	  break;
	case 0x68: case 0x69: case 0x6A: case 0x6B:
	case 0x6C: case 0x6D: case 0x6E: case 0x6F:	/* illegal */
	  ILLEGAL ();
	  break;
	case 0x71: case 0x72: case 0x73:
	case 0x74: case 0x75: case 0x76: case 0x77:
	case 0x78: case 0x79: case 0x7A: case 0x7B:
	case 0x7C: case 0x7D: case 0x7E:		/* lrw */
	  gr[RX] =  rlat ((pc + ((inst & 0xFF) << 2)) & 0xFFFFFFFC);
	  if (tracing)
	    fprintf (stderr, "LRW of 0x%x from 0x%x to reg %d",
		     rlat ((pc + ((inst & 0xFF) << 2)) & 0xFFFFFFFC),
		     (pc + ((inst & 0xFF) << 2)) & 0xFFFFFFFC, RX);
	  memops++;
	  break;
	case 0x7F:					/* jsri */
	  gr[15] = pc;
	  if (tracing)
	    fprintf (stderr,
		     "func call: r2 = %x r3 = %x r4 = %x r5 = %x r6 = %x r7 = %x\n",
		     gr[2], gr[3], gr[4], gr[5], gr[6], gr[7]);
	  ATTRIBUTE_FALLTHROUGH;
	case 0x70:					/* jmpi */
	  pc = rlat ((pc + ((inst & 0xFF) << 2)) & 0xFFFFFFFC);
	  memops++;
	  bonus_cycles++;
	  needfetch = 1;
	  break;

	case 0x80: case 0x81: case 0x82: case 0x83:
	case 0x84: case 0x85: case 0x86: case 0x87:
	case 0x88: case 0x89: case 0x8A: case 0x8B:
	case 0x8C: case 0x8D: case 0x8E: case 0x8F:	/* ld */
	  gr[RX] = rlat (gr[RD] + ((inst >> 2) & 0x003C));
	  if (tracing)
	    fprintf (stderr, "load reg %d from 0x%x with 0x%x",
		     RX,
		     gr[RD] + ((inst >> 2) & 0x003C), gr[RX]);
	  memops++;
	  break;
	case 0x90: case 0x91: case 0x92: case 0x93:
	case 0x94: case 0x95: case 0x96: case 0x97:
	case 0x98: case 0x99: case 0x9A: case 0x9B:
	case 0x9C: case 0x9D: case 0x9E: case 0x9F:	/* st */
	  wlat (gr[RD] + ((inst >> 2) & 0x003C), gr[RX]);
	  if (tracing)
	    fprintf (stderr, "store reg %d (containing 0x%x) to 0x%x",
		     RX, gr[RX],
		     gr[RD] + ((inst >> 2) & 0x003C));
	  memops++;
	  break;
	case 0xA0: case 0xA1: case 0xA2: case 0xA3:
	case 0xA4: case 0xA5: case 0xA6: case 0xA7:
	case 0xA8: case 0xA9: case 0xAA: case 0xAB:
	case 0xAC: case 0xAD: case 0xAE: case 0xAF:	/* ld.b */
	  gr[RX] = rbat (gr[RD] + RS);
	  memops++;
	  break;
	case 0xB0: case 0xB1: case 0xB2: case 0xB3:
	case 0xB4: case 0xB5: case 0xB6: case 0xB7:
	case 0xB8: case 0xB9: case 0xBA: case 0xBB:
	case 0xBC: case 0xBD: case 0xBE: case 0xBF:	/* st.b */
	  wbat (gr[RD] + RS, gr[RX]);
	  memops++;
	  break;
	case 0xC0: case 0xC1: case 0xC2: case 0xC3:
	case 0xC4: case 0xC5: case 0xC6: case 0xC7:
	case 0xC8: case 0xC9: case 0xCA: case 0xCB:
	case 0xCC: case 0xCD: case 0xCE: case 0xCF:	/* ld.h */
	  gr[RX] = rhat (gr[RD] + ((inst >> 3) & 0x001E));
	  memops++;
	  break;
	case 0xD0: case 0xD1: case 0xD2: case 0xD3:
	case 0xD4: case 0xD5: case 0xD6: case 0xD7:
	case 0xD8: case 0xD9: case 0xDA: case 0xDB:
	case 0xDC: case 0xDD: case 0xDE: case 0xDF:	/* st.h */
	  what (gr[RD] + ((inst >> 3) & 0x001E), gr[RX]);
	  memops++;
	  break;
	case 0xE8: case 0xE9: case 0xEA: case 0xEB:
	case 0xEC: case 0xED: case 0xEE: case 0xEF:	/* bf */
	  if (C_OFF())
	    {
	      int disp;
	      disp = inst & 0x03FF;
	      if (inst & 0x0400)
		disp |= 0xFFFFFC00;
	      pc += disp<<1;
	      bonus_cycles++;
	      needfetch = 1;
	    }
	  break;
	case 0xE0: case 0xE1: case 0xE2: case 0xE3:
	case 0xE4: case 0xE5: case 0xE6: case 0xE7:	/* bt */
	  if (C_ON())
	    {
	      int disp;
	      disp = inst & 0x03FF;
	      if (inst & 0x0400)
		disp |= 0xFFFFFC00;
	      pc += disp<<1;
	      bonus_cycles++;
	      needfetch = 1;
	    }
	  break;

	case 0xF8: case 0xF9: case 0xFA: case 0xFB:
	case 0xFC: case 0xFD: case 0xFE: case 0xFF:	/* bsr */
	  gr[15] = pc;
	  ATTRIBUTE_FALLTHROUGH;
	case 0xF0: case 0xF1: case 0xF2: case 0xF3:
	case 0xF4: case 0xF5: case 0xF6: case 0xF7:	/* br */
	  {
	    int disp;
	    disp = inst & 0x03FF;
	    if (inst & 0x0400)
	      disp |= 0xFFFFFC00;
	    pc += disp<<1;
	    bonus_cycles++;
	    needfetch = 1;
	  }
	  break;

	}

      if (tracing)
	fprintf (stderr, "\n");

      if (needfetch)
	{
	  ibuf = rlat (pc & 0xFFFFFFFC);
	  needfetch = 0;
	}
    }

  /* Hide away the things we've cached while executing.  */
  CPU_PC_SET (cpu, pc);
  mcore_cpu->insts += insts;		/* instructions done ... */
  mcore_cpu->cycles += insts;		/* and each takes a cycle */
  mcore_cpu->cycles += bonus_cycles;	/* and extra cycles for branches */
  mcore_cpu->cycles += memops * memcycles;	/* and memop cycle delays */
}

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr,  /* ignore  */
		int nr_cpus,      /* ignore  */
		int siggnal)      /* ignore  */
{
  sim_cpu *cpu;

  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  cpu = STATE_CPU (sd, 0);

  while (1)
    {
      step_once (sd, cpu);
      if (sim_events_tick (sd))
	sim_events_process (sd);
    }
}

static int
mcore_reg_store (SIM_CPU *cpu, int rn, const void *memory, int length)
{
  struct mcore_sim_cpu *mcore_cpu = MCORE_SIM_CPU (cpu);

  if (rn < NUM_MCORE_REGS && rn >= 0)
    {
      if (length == 4)
	{
	  long ival;

	  /* misalignment safe */
	  ival = mcore_extract_unsigned_integer (memory, 4);
	  mcore_cpu->asints[rn] = ival;
	}

      return 4;
    }
  else
    return 0;
}

static int
mcore_reg_fetch (SIM_CPU *cpu, int rn, void *memory, int length)
{
  struct mcore_sim_cpu *mcore_cpu = MCORE_SIM_CPU (cpu);

  if (rn < NUM_MCORE_REGS && rn >= 0)
    {
      if (length == 4)
	{
	  long ival = mcore_cpu->asints[rn];

	  /* misalignment-safe */
	  mcore_store_unsigned_integer (memory, 4, ival);
	}

      return 4;
    }
  else
    return 0;
}

void
sim_info (SIM_DESC sd, bool verbose)
{
  SIM_CPU *cpu = STATE_CPU (sd, 0);
  struct mcore_sim_cpu *mcore_cpu = MCORE_SIM_CPU (cpu);
#ifdef WATCHFUNCTIONS
  int w, wcyc;
#endif
  double virttime = mcore_cpu->cycles / 36.0e6;
  host_callback *callback = STATE_CALLBACK (sd);

  callback->printf_filtered (callback, "\n\n# instructions executed  %10d\n",
			     mcore_cpu->insts);
  callback->printf_filtered (callback, "# cycles                 %10d\n",
			     mcore_cpu->cycles);
  callback->printf_filtered (callback, "# pipeline stalls        %10d\n",
			     mcore_cpu->stalls);
  callback->printf_filtered (callback, "# virtual time taken     %10.4f\n",
			     virttime);

#ifdef WATCHFUNCTIONS
  callback->printf_filtered (callback, "\nNumber of watched functions: %d\n",
			     ENDWL);

  wcyc = 0;

  for (w = 1; w <= ENDWL; w++)
    {
      callback->printf_filtered (callback, "WL = %s %8x\n",WLstr[w],WL[w]);
      callback->printf_filtered (callback, "  calls = %d, cycles = %d\n",
				 WLcnts[w],WLcyc[w]);

      if (WLcnts[w] != 0)
	callback->printf_filtered (callback,
				   "  maxcpc = %d, mincpc = %d, avecpc = %d\n",
				   WLmax[w],WLmin[w],WLcyc[w]/WLcnts[w]);
      wcyc += WLcyc[w];
    }

  callback->printf_filtered (callback,
			     "Total cycles for watched functions: %d\n",wcyc);
#endif
}

static sim_cia
mcore_pc_get (sim_cpu *cpu)
{
  return MCORE_SIM_CPU (cpu)->regs.pc;
}

static void
mcore_pc_set (sim_cpu *cpu, sim_cia pc)
{
  MCORE_SIM_CPU (cpu)->regs.pc = pc;
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

  /* Set default options before parsing user options.  */
  cb->syscall_map = cb_mcore_syscall_map;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct mcore_sim_cpu))
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

      CPU_REG_FETCH (cpu) = mcore_reg_fetch;
      CPU_REG_STORE (cpu) = mcore_reg_store;
      CPU_PC_FETCH (cpu) = mcore_pc_get;
      CPU_PC_STORE (cpu) = mcore_pc_set;

      set_initial_gprs (cpu);	/* Reset the GPR registers.  */
    }

  /* Default to a 8 Mbyte (== 2^23) memory space.  */
  sim_do_commandf (sd, "memory-size %#x", DEFAULT_MEMORY_SIZE);

  return sd;
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *prog_bfd,
		     char * const *argv, char * const *env)
{
  SIM_CPU *cpu = STATE_CPU (sd, 0);
  char * const *avp;
  int nargs = 0;
  int nenv = 0;
  int s_length;
  int l;
  unsigned long strings;
  unsigned long pointers;
  unsigned long hi_stack;


  /* Set the initial register set.  */
  set_initial_gprs (cpu);

  hi_stack = DEFAULT_MEMORY_SIZE - 4;
  CPU_PC_SET (cpu, bfd_get_start_address (prog_bfd));

  /* Calculate the argument and environment strings.  */
  s_length = 0;
  nargs = 0;
  avp = argv;
  while (avp && *avp)
    {
      l = strlen (*avp) + 1;	/* include the null */
      s_length += (l + 3) & ~3;	/* make it a 4 byte boundary */
      nargs++; avp++;
    }

  nenv = 0;
  avp = env;
  while (avp && *avp)
    {
      l = strlen (*avp) + 1;	/* include the null */
      s_length += (l + 3) & ~ 3;/* make it a 4 byte boundary */
      nenv++; avp++;
    }

  /* Claim some memory for the pointers and strings. */
  pointers = hi_stack - sizeof(int32_t) * (nenv+1+nargs+1);
  pointers &= ~3;		/* must be 4-byte aligned */
  gr[0] = pointers;

  strings = gr[0] - s_length;
  strings &= ~3;		/* want to make it 4-byte aligned */
  gr[0] = strings;
  /* dac fix, the stack address must be 8-byte aligned! */
  gr[0] = gr[0] - gr[0] % 8;

  /* Loop through the arguments and fill them in.  */
  gr[PARM1] = nargs;
  if (nargs == 0)
    {
      /* No strings to fill in.  */
      gr[PARM2] = 0;
    }
  else
    {
      gr[PARM2] = pointers;
      avp = argv;
      while (avp && *avp)
	{
	  /* Save where we're putting it.  */
	  wlat (pointers, strings);

	  /* Copy the string.  */
	  l = strlen (* avp) + 1;
	  sim_core_write_buffer (sd, cpu, write_map, *avp, strings, l);

	  /* Bump the pointers.  */
	  avp++;
	  pointers += 4;
	  strings += l+1;
	}

      /* A null to finish the list.  */
      wlat (pointers, 0);
      pointers += 4;
    }

  /* Now do the environment pointers.  */
  if (nenv == 0)
    {
      /* No strings to fill in.  */
      gr[PARM3] = 0;
    }
  else
    {
      gr[PARM3] = pointers;
      avp = env;

      while (avp && *avp)
	{
	  /* Save where we're putting it.  */
	  wlat (pointers, strings);

	  /* Copy the string.  */
	  l = strlen (* avp) + 1;
	  sim_core_write_buffer (sd, cpu, write_map, *avp, strings, l);

	  /* Bump the pointers.  */
	  avp++;
	  pointers += 4;
	  strings += l+1;
	}

      /* A null to finish the list.  */
      wlat (pointers, 0);
      pointers += 4;
    }

  return SIM_RC_OK;
}
