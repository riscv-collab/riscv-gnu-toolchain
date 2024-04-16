/* cpu.c --- CPU for RL78 simulator.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.
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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* This must come before any other includes.  */
#include "defs.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "opcode/rl78.h"
#include "mem.h"
#include "cpu.h"

int verbose = 0;
int trace = 0;
int rl78_in_gdb = 1;
int timer_enabled = 2;
int rl78_g10_mode = 0;
int g13_multiply = 0;
int g14_multiply = 0;

SI pc;

#define REGISTER_ADDRESS 0xffee0

typedef struct {
  unsigned char x;
  unsigned char a;
  unsigned char c;
  unsigned char b;
  unsigned char e;
  unsigned char d;
  unsigned char l;
  unsigned char h;
} RegBank;

static void trace_register_init (void);

/* This maps PSW to a pointer into memory[] */
static RegBank *regbase_table[256];

#define regbase regbase_table[memory[RL78_SFR_PSW]]

#define REG(r) ((regbase)->r)

void
init_cpu (void)
{
  int i;

  init_mem ();

  memset (memory+REGISTER_ADDRESS, 0x11, 8 * 4);
  memory[RL78_SFR_PSW] = 0x06;
  memory[RL78_SFR_ES] = 0x0f;
  memory[RL78_SFR_CS] = 0x00;
  memory[RL78_SFR_PMC] = 0x00;

  for (i = 0; i < 256; i ++)
    {
      int rb0 = (i & RL78_PSW_RBS0) ? 1 : 0;
      int rb1 = (i & RL78_PSW_RBS1) ? 2 : 0;
      int rb = rb1 | rb0;
      regbase_table[i] = (RegBank *)(memory + (3 - rb) * 8 + REGISTER_ADDRESS);
    }

  trace_register_init ();

  /* This means "by default" */
  timer_enabled = 2;
}

SI
get_reg (RL78_Register regno)
{
  switch (regno)
    {
    case RL78_Reg_None:
      /* Conditionals do this.  */
      return 0;

    default:
      abort ();
    case RL78_Reg_X:	return REG (x);
    case RL78_Reg_A:	return REG (a);
    case RL78_Reg_C:	return REG (c);
    case RL78_Reg_B:	return REG (b);
    case RL78_Reg_E:	return REG (e);
    case RL78_Reg_D:	return REG (d);
    case RL78_Reg_L:	return REG (l);
    case RL78_Reg_H:	return REG (h);
    case RL78_Reg_AX:	return REG (a) * 256 + REG (x);
    case RL78_Reg_BC:	return REG (b) * 256 + REG (c);
    case RL78_Reg_DE:	return REG (d) * 256 + REG (e);
    case RL78_Reg_HL:	return REG (h) * 256 + REG (l);
    case RL78_Reg_SP:	return memory[RL78_SFR_SP] + 256 * memory[RL78_SFR_SP+1];
    case RL78_Reg_PSW:	return memory[RL78_SFR_PSW];
    case RL78_Reg_CS:	return memory[RL78_SFR_CS];
    case RL78_Reg_ES:	return memory[RL78_SFR_ES];
    case RL78_Reg_PMC:	return memory[RL78_SFR_PMC];
    case RL78_Reg_MEM:	return memory[RL78_SFR_MEM];
    }
}

extern unsigned char initted[];

SI
set_reg (RL78_Register regno, SI val)
{
  switch (regno)
    {
    case RL78_Reg_None:
      abort ();
    case RL78_Reg_X:	REG (x) = val; break;
    case RL78_Reg_A:	REG (a) = val; break;
    case RL78_Reg_C:	REG (c) = val; break;
    case RL78_Reg_B:	REG (b) = val; break;
    case RL78_Reg_E:	REG (e) = val; break;
    case RL78_Reg_D:	REG (d) = val; break;
    case RL78_Reg_L:	REG (l) = val; break;
    case RL78_Reg_H:	REG (h) = val; break;
    case RL78_Reg_AX:
      REG (a) = val >> 8;
      REG (x) = val & 0xff;
      break;
    case RL78_Reg_BC:
      REG (b) = val >> 8;
      REG (c) = val & 0xff;
      break;
    case RL78_Reg_DE:
      REG (d) = val >> 8;
      REG (e) = val & 0xff;
      break;
    case RL78_Reg_HL:
      REG (h) = val >> 8;
      REG (l) = val & 0xff;
      break;
    case RL78_Reg_SP:
      if (val & 1)
	{
	  printf ("Warning: SP value 0x%04x truncated at pc=0x%05x\n", val, pc);
	  val &= ~1;
	}
      {
	int old_sp = get_reg (RL78_Reg_SP);
	if (val < old_sp)
	  {
	    int i;
	    for (i = val; i < old_sp; i ++)
	      initted[i + 0xf0000] = 0;
	  }
      }
      memory[RL78_SFR_SP] = val & 0xff;
      memory[RL78_SFR_SP + 1] = val >> 8;
      break;
    case RL78_Reg_PSW:	memory[RL78_SFR_PSW] = val; break;
    case RL78_Reg_CS:	memory[RL78_SFR_CS] = val; break;
    case RL78_Reg_ES:	memory[RL78_SFR_ES] = val; break;
    case RL78_Reg_PMC:	memory[RL78_SFR_PMC] = val; break;
    case RL78_Reg_MEM:	memory[RL78_SFR_MEM] = val; break;
    }
  return val;
}

int
condition_true (RL78_Condition cond_id, int val)
{
  int psw = get_reg (RL78_Reg_PSW);
  int z = (psw & RL78_PSW_Z) ? 1 : 0;
  int cy = (psw & RL78_PSW_CY) ? 1 : 0;

  switch (cond_id)
    {
    case RL78_Condition_T:
      return val != 0;
    case RL78_Condition_F:
      return val == 0;
    case RL78_Condition_C:
      return cy;
    case RL78_Condition_NC:
      return !cy;
    case RL78_Condition_H:
      return !(z | cy);
    case RL78_Condition_NH:
      return z | cy;
    case RL78_Condition_Z:
      return z;
    case RL78_Condition_NZ:
      return !z;
    default:
      abort ();
    }
}

const char * const
reg_names[] = {
  "none",
  "x",
  "a",
  "c",
  "b",
  "e",
  "d",
  "l",
  "h",
  "ax",
  "bc",
  "de",
  "hl",
  "sp",
  "psw",
  "cs",
  "es",
  "pmc",
  "mem"
};

static char *
psw_string (int psw)
{
  static char buf[30];
  const char *comma = "";

  buf[0] = 0;
  if (psw == 0)
    strcpy (buf, "-");
  else
    {
#define PSW1(bit, name) if (psw & bit) { strcat (buf, comma); strcat (buf, name); comma = ","; }
      PSW1 (RL78_PSW_IE, "ie");
      PSW1 (RL78_PSW_Z, "z");
      PSW1 (RL78_PSW_RBS1, "r1");
      PSW1 (RL78_PSW_AC, "ac");
      PSW1 (RL78_PSW_RBS0, "r0");
      PSW1 (RL78_PSW_ISP1, "i1");
      PSW1 (RL78_PSW_ISP0, "i0");
      PSW1 (RL78_PSW_CY, "cy");
    }
  printf ("%s", buf);
  return buf;
}

static unsigned char old_regs[32];
static int old_psw;
static int old_sp;

int trace_register_words;

void
trace_register_changes (void)
{
  int i;
  int any = 0;

  if (!trace)
    return;

#define TB(name,nv,ov) if (nv != ov) { printf ("%s: \033[31m%02x \033[32m%02x\033[0m ", name, ov, nv); ov = nv; any = 1; }
#define TW(name,nv,ov) if (nv != ov) { printf ("%s: \033[31m%04x \033[32m%04x\033[0m ", name, ov, nv); ov = nv; any = 1; }

  if (trace_register_words)
    {
#define TRW(name, idx) TW (name, memory[REGISTER_ADDRESS + (idx)], old_regs[idx])
      for (i = 0; i < 32; i += 2)
	{
	  char buf[10];
	  int o, n, a;
	  switch (i)
	    {
	    case 0: strcpy (buf, "AX"); break;
	    case 2: strcpy (buf, "BC"); break;
	    case 4: strcpy (buf, "DE"); break;
	    case 6: strcpy (buf, "HL"); break;
	    default: sprintf (buf, "r%d", i); break;
	    }
	  a = REGISTER_ADDRESS + (i ^ 0x18);
	  o = old_regs[i ^ 0x18] + old_regs[(i ^ 0x18) + 1] * 256;
	  n = memory[a] + memory[a + 1] * 256;
	  TW (buf, n, o);
	  old_regs[i ^ 0x18] = n;
	  old_regs[(i ^ 0x18) + 1] = n >> 8;
	}
    }
  else
    {
      for (i = 0; i < 32; i ++)
	{
	  char buf[10];
	  if (i < 8)
	    {
	      buf[0] = "XACBEDLH"[i];
	      buf[1] = 0;
	    }
	  else
	    sprintf (buf, "r%d", i);
#define TRB(name, idx) TB (name, memory[REGISTER_ADDRESS + (idx)], old_regs[idx])
	  TRB (buf, i ^ 0x18);
	}
    }
  if (memory[RL78_SFR_PSW] != old_psw)
    {
      printf ("PSW: \033[31m");
      psw_string (old_psw);
      printf (" \033[32m");
      psw_string (memory[RL78_SFR_PSW]);
      printf ("\033[0m ");
      old_psw = memory[RL78_SFR_PSW];
      any = 1;
    }
  TW ("SP", mem_get_hi (RL78_SFR_SP), old_sp);
  if (any)
    printf ("\n");
}

static void
trace_register_init (void)
{
  memcpy (old_regs, memory + REGISTER_ADDRESS, 8 * 4);
  old_psw = memory[RL78_SFR_PSW];
  old_sp = mem_get_hi (RL78_SFR_SP);
}
