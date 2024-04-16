/* reg.c --- register set model for RX simulator.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.
   Contributed by Red Hat, Inc.

   This file is part of the GNU simulators.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "bfd.h"
#include "trace.h"

int verbose = 0;
int trace = 0;
int enable_counting = 0;

int rx_in_gdb = 1;

int rx_flagmask;
int rx_flagand;
int rx_flagor;

int rx_big_endian;
regs_type regs;
int step_result;
unsigned int heapbottom = 0;
unsigned int heaptop = 0;

char *reg_names[] = {
  /* general registers */
  "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
  /* control register */
  "psw", "pc", "usp", "fpsw", "RES", "RES", "RES", "RES",
  "bpsw", "bpc", "isp", "fintv", "intb", "RES", "RES", "RES",
  "RES", "RES", "RES", "RES", "RES", "RES", "RES", "RES",
  "RES", "RES", "RES", "RES", "RES", "RES", "RES", "RES",
  "temp", "acc", "acchi", "accmi", "acclo"
};

unsigned int b2mask[] = { 0, 0xff, 0xffff, 0xffffff, 0xffffffff };
unsigned int b2signbit[] = { 0, (1 << 7), (1 << 15), (1 << 24), (1 << 31) };
int b2maxsigned[] = { 0, 0x7f, 0x7fff, 0x7fffff, 0x7fffffff };
int b2minsigned[] = { 0, -128, -32768, -8388608, -2147483647 - 1 };

static regs_type oldregs;

void
init_regs (void)
{
  memset (&regs, 0, sizeof (regs));
  memset (&oldregs, 0, sizeof (oldregs));

#ifdef CYCLE_ACCURATE
  regs.rt = -1;
  oldregs.rt = -1;
#endif
}

static unsigned int
get_reg_i (int id)
{
  if (id == 0)
    return regs.r_psw & FLAGBIT_U ? regs.r_usp : regs.r_isp;

  if (id >= 1 && id <= 15)
    return regs.r[id];

  switch (id)
    {
    case psw:
      return regs.r_psw;
    case fpsw:
      return regs.r_fpsw;
    case isp:
      return regs.r_isp;
    case usp:
      return regs.r_usp;
    case bpc:
      return regs.r_bpc;
    case bpsw:
      return regs.r_bpsw;
    case fintv:
      return regs.r_fintv;
    case intb:
      return regs.r_intb;
    case pc:
      return regs.r_pc;
    case r_temp_idx:
      return regs.r_temp;
    case acchi:
      return (SI)(regs.r_acc >> 32);
    case accmi:
      return (SI)(regs.r_acc >> 16);
    case acclo:
      return (SI)regs.r_acc;
    }
  abort();
}

unsigned int
get_reg (int id)
{
  unsigned int rv = get_reg_i (id);
  if (trace > ((id != pc && id != sp) ? 0 : 1))
    printf ("get_reg (%s) = %08x\n", reg_names[id], rv);
  return rv;
}

static unsigned long long
get_reg64_i (int id)
{
  switch (id)
    {
    case acc64:
      return regs.r_acc;
    default:
      abort ();
    }
}

unsigned long long
get_reg64 (int id)
{
  unsigned long long rv = get_reg64_i (id);
  if (trace > ((id != pc && id != sp) ? 0 : 1))
    printf ("get_reg (%s) = %016llx\n", reg_names[id], rv);
  return rv;
}

static int highest_sp = 0, lowest_sp = 0xffffff;

void
stack_heap_stats (void)
{
  if (heapbottom < heaptop)
    printf ("heap:  %08x - %08x (%d bytes)\n", heapbottom, heaptop,
	    heaptop - heapbottom);
  if (lowest_sp < highest_sp)
    printf ("stack: %08x - %08x (%d bytes)\n", lowest_sp, highest_sp,
	    highest_sp - lowest_sp);
}

void
put_reg (int id, unsigned int v)
{
  if (trace > ((id != pc) ? 0 : 1))
    printf ("put_reg (%s) = %08x\n", reg_names[id], v);


  switch (id)
    {
    case psw:
      regs.r_psw = v;
      break;
    case fpsw:
      {
	SI anded;
	/* This is an odd one - The Cx flags are AND'd, and the FS flag
	   is synthetic.  */
	anded = regs.r_fpsw & v;
	anded |= ~ FPSWBITS_CMASK;
	regs.r_fpsw = v & anded;
	if (regs.r_fpsw & FPSWBITS_FMASK)
	  regs.r_fpsw |= FPSWBITS_FSUM;
	else
	  regs.r_fpsw &= ~FPSWBITS_FSUM;
      }
      break;
    case isp:
      regs.r_isp = v;
      break;
    case usp:
      regs.r_usp = v;
      break;
    case bpc:
      regs.r_bpc = v;
      break;
    case bpsw:
      regs.r_bpsw = v;
      break;
    case fintv:
      regs.r_fintv = v;
      break;
    case intb:
      regs.r_intb = v;
      break;
    case pc:
      regs.r_pc = v;
      break;

    case acchi:
      regs.r_acc = (regs.r_acc & 0xffffffffULL) | ((DI)v << 32);
      break;
    case accmi:
      regs.r_acc = (regs.r_acc & ~0xffffffff0000ULL) | ((DI)v << 16);
      break;
    case acclo:
      regs.r_acc = (regs.r_acc & ~0xffffffffULL) | ((DI)v);
      break;

    case 0: /* Stack pointer is "in" R0.  */
      {
	if (v < heaptop)
	  {
	    unsigned int line;
	    const char * dummy;
	    const char * fname = NULL;

	    sim_get_current_source_location (& dummy, & fname, &line);

	    /* The setjmp and longjmp functions play tricks with the stack pointer.  */
	    if (fname == NULL
		|| (strcmp (fname, "_setjmp") != 0
		    && strcmp (fname, "_longjmp") != 0))
	      {
		printf ("collision in %s: pc %08x heap %08x stack %08x\n",
			fname, (unsigned int) regs.r_pc, heaptop, v);
		exit (1);
	      }
	  }
	else
	  {
	    if (v < lowest_sp)
	      lowest_sp = v;
	    if (v > highest_sp)
	      highest_sp = v;
	  }

	if (regs.r_psw & FLAGBIT_U)
	  regs.r_usp = v;
	else
	  regs.r_isp = v;
	break;
      }

    default:
      if (id >= 1 && id <= 15)
	regs.r[id] = v;
      else
	abort ();
    }
}

void
put_reg64 (int id, unsigned long long v)
{
  if (trace > ((id != pc) ? 0 : 1))
    printf ("put_reg (%s) = %016llx\n", reg_names[id], v);

  switch (id)
    {
    case acc64:
      regs.r_acc = v;
      break;
    default:
      abort ();
    }
}

int
condition_true (int cond_id)
{
  int f;

  static const char *cond_name[] = {
    "Z",
    "!Z",
    "C",
    "!C",
    "C&!Z",
    "!(C&!Z)",
    "!S",
    "S",
    "!(S^O)",
    "S^O",
    "!((S^O)|Z)",
    "(S^O)|Z",
    "O",
    "!O",
    "always",
    "never"
  };
  switch (cond_id & 15)
    {
    case 0:
      f = FLAG_Z;
      break;		/* EQ/Z */
    case 1:
      f = !FLAG_Z;
      break;		/* NE/NZ */
    case 2:
      f = FLAG_C;
      break;		/* GEU/C */
    case 3:
      f = !FLAG_C;
      break;		/* LTU/NC */
    case 4:
      f = FLAG_C & !FLAG_Z;
      break;		/* GTU */
    case 5:
      f = !(FLAG_C & !FLAG_Z);
      break;		/* LEU */
    case 6:
      f = !FLAG_S;
      break;		/* PZ */
    case 7:
      f = FLAG_S;
      break;		/* N */

    case 8:
      f = !(FLAG_S ^ FLAG_O);
      break;		/* GE */
    case 9:
      f = FLAG_S ^ FLAG_O;
      break;		/* LT */
    case 10:
      f = !((FLAG_S ^ FLAG_O) | FLAG_Z);
      break;		/* GT */
    case 11:
      f = (FLAG_S ^ FLAG_O) | FLAG_Z;
      break;		/* LE */
    case 12:
      f = FLAG_O;
      break;		/* O */
    case 13:
      f = !FLAG_O;
      break;		/* NO */
    case 14:
      f = 1;		/* always */
      break;
    default:
      f = 0;		/* never */
      break;
    }
  if (trace && ((cond_id & 15) != 14))
    printf ("cond[%d] %s = %s\n", cond_id, cond_name[cond_id & 15],
	    f ? "true" : "false");
  return f;
}

void
set_flags (int mask, int newbits)
{
  regs.r_psw &= rx_flagand;
  regs.r_psw |= rx_flagor;
  regs.r_psw |= (newbits & mask & rx_flagmask);

  if (trace)
    {
      int i;
      printf ("flags now \033[32m %d", (int)((regs.r_psw >> 24) & 7));
      for (i = 17; i >= 0; i--)
	if (0x3000f & (1 << i))
	  {
	    if (regs.r_psw & (1 << i))
	      putchar ("CZSO------------IU"[i]);
	    else
	      putchar ('-');
	  }
      printf ("\033[0m\n");
    }
}

void
set_oszc (long long value, int b, int c)
{
  unsigned int mask = b2mask[b];
  int f = 0;

  if (c)
    f |= FLAGBIT_C;
  if ((value & mask) == 0)
    f |= FLAGBIT_Z;
  if (value & b2signbit[b])
    f |= FLAGBIT_S;
  if ((value > b2maxsigned[b]) || (value < b2minsigned[b]))
    f |= FLAGBIT_O;
  set_flags (FLAGBIT_Z | FLAGBIT_S | FLAGBIT_O | FLAGBIT_C, f);
}

void
set_szc (long long value, int b, int c)
{
  unsigned int mask = b2mask[b];
  int f = 0;

  if (c)
    f |= FLAGBIT_C;
  if ((value & mask) == 0)
    f |= FLAGBIT_Z;
  if (value & b2signbit[b])
    f |= FLAGBIT_S;
  set_flags (FLAGBIT_Z | FLAGBIT_S | FLAGBIT_C, f);
}

void
set_osz (long long value, int b)
{
  unsigned int mask = b2mask[b];
  int f = 0;

  if ((value & mask) == 0)
    f |= FLAGBIT_Z;
  if (value & b2signbit[b])
    f |= FLAGBIT_S;
  if ((value > b2maxsigned[b]) || (value < b2minsigned[b]))
    f |= FLAGBIT_O;
  set_flags (FLAGBIT_Z | FLAGBIT_S | FLAGBIT_O, f);
}

void
set_sz (long long value, int b)
{
  unsigned int mask = b2mask[b];
  int f = 0;

  if ((value & mask) == 0)
    f |= FLAGBIT_Z;
  if (value & b2signbit[b])
    f |= FLAGBIT_S;
  set_flags (FLAGBIT_Z | FLAGBIT_S, f);
}

void
set_zc (int z, int c)
{
  set_flags (FLAGBIT_C | FLAGBIT_Z,
	     (c ? FLAGBIT_C : 0) | (z ? FLAGBIT_Z : 0));
}

void
set_c (int c)
{
  set_flags (FLAGBIT_C, c ? FLAGBIT_C : 0);
}

static char *
psw2str(int rpsw)
{
  static char buf[10];
  char *bp = buf;
  int i, ipl;

  ipl = (rpsw & FLAGBITS_IPL) >> FLAGSHIFT_IPL;
  if (ipl > 9)
    {
      *bp++ = (ipl / 10) + '0';
      ipl %= 10;
    }
  *bp++ = ipl + '0';
  for (i = 20; i >= 0; i--)
    if (0x13000f & (1 << i))
      {
	if (rpsw & (1 << i))
	  *bp++ = "CZSO------------IU--P"[i];
	else
	  *bp++ = '-';
      }
  *bp = 0;
  return buf;
}

static char *
fpsw2str(int rpsw)
{
  static char buf[100];
  char *bp = buf;
  int i;	/*   ---+---+---+---+---+---+---+---+ */
  const char s1[] = "FFFFFF-----------EEEEE-DCCCCCCRR";
  const char s2[] = "SXUZOV-----------XUZOV-NEXUZOV01";
  const char rm[4][3] = { "RC", "RZ", "RP", "RN" };

  for (i = 31; i >= 0; i--)
    if (0xfc007dfc & (1 << i))
      {
	if (rpsw & (1 << i))
	  {
	    if (bp > buf)
	      *bp++ = '.';
	    *bp++ = s1[31-i];
	    *bp++ = s2[31-i];
	  }
      }
  if (bp > buf)
    *bp++ = '.';
  strcpy (bp, rm[rpsw&3]);
  return buf;
}

#define TRC(f,n) \
  if (oldregs.f != regs.f) \
    { \
      if (tag) { printf ("%s", tag); tag = 0; }  \
      printf("  %s %08x:%08x", n, \
	     (unsigned int)oldregs.f, \
	     (unsigned int)regs.f); \
      oldregs.f = regs.f; \
    }

void
trace_register_changes (void)
{
  char *tag = "\033[36mREGS:";
  int i;

  if (!trace)
    return;
  for (i=1; i<16; i++)
    TRC (r[i], reg_names[i]);
  TRC (r_intb, "intb");
  TRC (r_usp, "usp");
  TRC (r_isp, "isp");
  if (oldregs.r_psw != regs.r_psw)
    {
      if (tag) { printf ("%s", tag); tag = 0; }
      printf("  psw %s:", psw2str(oldregs.r_psw));
      printf("%s", psw2str(regs.r_psw));
      oldregs.r_psw = regs.r_psw;
    }

  if (oldregs.r_fpsw != regs.r_fpsw)
    {
      if (tag) { printf ("%s", tag); tag = 0; }
      printf("  fpsw %s:", fpsw2str(oldregs.r_fpsw));
      printf("%s", fpsw2str(regs.r_fpsw));
      oldregs.r_fpsw = regs.r_fpsw;
    }

  if (oldregs.r_acc != regs.r_acc)
    {
      if (tag) { printf ("%s", tag); tag = 0; }
      printf("  acc %016" PRIx64 ":", oldregs.r_acc);
      printf("%016" PRIx64, regs.r_acc);
      oldregs.r_acc = regs.r_acc;
    }

  if (tag == 0)
    printf ("\033[0m\n");
}
