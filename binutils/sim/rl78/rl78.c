/* rl78.c --- opcode semantics for stand-alone RL78 simulator.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.
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
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <time.h>

#include "opcode/rl78.h"
#include "cpu.h"
#include "mem.h"

extern int skip_init;
static int opcode_pc = 0;

jmp_buf decode_jmp_buf;
#define DO_RETURN(x) longjmp (decode_jmp_buf, x)

#define tprintf if (trace) printf

#define WILD_JUMP_CHECK(new_pc)						\
  do {									\
    if (new_pc == 0 || new_pc > 0xfffff)				\
      {									\
	pc = opcode_pc;							\
	fprintf (stderr, "Wild jump to 0x%x from 0x%x!\n", new_pc, pc); \
	DO_RETURN (RL78_MAKE_HIT_BREAK ());				\
      }									\
  } while (0)

typedef struct {
  unsigned long dpc;
} RL78_Data;

static int
rl78_get_byte (void *vdata)
{
  RL78_Data *rl78_data = (RL78_Data *)vdata;
  int rv = mem_get_pc (rl78_data->dpc);
  rl78_data->dpc ++;
  return rv;
}

static int
op_addr (const RL78_Opcode_Operand *o, int for_data)
{
  int v = o->addend;
  if (o->reg != RL78_Reg_None)
    v += get_reg (o->reg);
  if (o->reg2 != RL78_Reg_None)
    v += get_reg (o->reg2);
  if (o->use_es)
    v |= (get_reg (RL78_Reg_ES) & 0xf) << 16;
  else if (for_data)
    v |= 0xf0000;
  v &= 0xfffff;
  return v;
}

static int
get_op (const RL78_Opcode_Decoded *rd, int i, int for_data)
{
  int v, r;
  const RL78_Opcode_Operand *o = rd->op + i;

  switch (o->type)
    {
    case RL78_Operand_None:
      /* condition code does this. */
      v = 0;
      break;

    case RL78_Operand_Immediate:
      tprintf (" #");
      v = o->addend;
      break;

    case RL78_Operand_Register:
      tprintf (" %s=", reg_names[o->reg]);
      v = get_reg (o->reg);
      break;
 
    case RL78_Operand_Bit:
      tprintf (" %s.%d=", reg_names[o->reg], o->bit_number);
      v = get_reg (o->reg);
      v = (v & (1 << o->bit_number)) ? 1 : 0;
      break;

    case RL78_Operand_Indirect:
      v = op_addr (o, for_data);
      tprintf (" [0x%x]=", v);
      if (rd->size == RL78_Word)
	v = mem_get_hi (v);
      else
	v = mem_get_qi (v);
      break;

    case RL78_Operand_BitIndirect:
      v = op_addr (o, for_data);
      tprintf (" [0x%x].%d=", v, o->bit_number);
      v = (mem_get_qi (v) & (1 << o->bit_number)) ? 1 : 0;
      break;

    case RL78_Operand_PreDec:
      r = get_reg (o->reg);
      tprintf (" [--%s]", reg_names[o->reg]);
      if (rd->size == RL78_Word)
	{
	  r -= 2;
	  v = mem_get_hi (r | 0xf0000);
	}
      else
	{
	  r -= 1;
	  v = mem_get_qi (r | 0xf0000);
	}
      set_reg (o->reg, r);
      break;
      
    case RL78_Operand_PostInc:
      tprintf (" [%s++]", reg_names[o->reg]);
      r = get_reg (o->reg);
      if (rd->size == RL78_Word)
	{
	  v = mem_get_hi (r | 0xf0000);
	  r += 2;
	}
      else
	{
	  v = mem_get_qi (r | 0xf0000);
	  r += 1;
	}
      set_reg (o->reg, r);
      break;
      
    default:
      abort ();
    }
  tprintf ("%d", v);
  return v;
}

static void
put_op (const RL78_Opcode_Decoded *rd, int i, int for_data, int v)
{
  int r, a;
  const RL78_Opcode_Operand *o = rd->op + i;

  tprintf (" -> ");

  switch (o->type)
    {
    case RL78_Operand_Register:
      tprintf ("%s", reg_names[o->reg]);
      set_reg (o->reg, v);
      break;
 
    case RL78_Operand_Bit:
      tprintf ("%s.%d", reg_names[o->reg], o->bit_number);
      r = get_reg (o->reg);
      if (v)
	r |= (1 << o->bit_number);
      else
	r &= ~(1 << o->bit_number);
      set_reg (o->reg, r);
      break;

    case RL78_Operand_Indirect:
      r = op_addr (o, for_data);
      tprintf ("[0x%x]", r);
      if (rd->size == RL78_Word)
	mem_put_hi (r, v);
      else
	mem_put_qi (r, v);
      break;

    case RL78_Operand_BitIndirect:
      a = op_addr (o, for_data);
      tprintf ("[0x%x].%d", a, o->bit_number);
      r = mem_get_qi (a);
      if (v)
	r |= (1 << o->bit_number);
      else
	r &= ~(1 << o->bit_number);
      mem_put_qi (a, r);
      break;

    case RL78_Operand_PreDec:
      r = get_reg (o->reg);
      tprintf ("[--%s]", reg_names[o->reg]);
      if (rd->size == RL78_Word)
	{
	  r -= 2;
	  set_reg (o->reg, r);
	  mem_put_hi (r | 0xf0000, v);
	}
      else
	{
	  r -= 1;
	  set_reg (o->reg, r);
	  mem_put_qi (r | 0xf0000, v);
	}
      break;
      
    case RL78_Operand_PostInc:
      tprintf ("[%s++]", reg_names[o->reg]);
      r = get_reg (o->reg);
      if (rd->size == RL78_Word)
	{
	  mem_put_hi (r | 0xf0000, v);
	  r += 2;
	}
      else
	{
	  mem_put_qi (r | 0xf0000, v);
	  r += 1;
	}
      set_reg (o->reg, r);
      break;

    default:
      abort ();
    }
  tprintf ("\n");
}

static void
op_flags (int before, int after, int mask, RL78_Size size)
{
  int vmask, cmask, amask, avmask;
  int psw;

  if (size == RL78_Word)
    {
      cmask = 0x10000;
      vmask = 0xffff;
      amask = 0x100;
      avmask = 0x0ff;
    }
  else
    {
      cmask = 0x100;
      vmask = 0xff;
      amask = 0x10;
      avmask = 0x0f;
    }

  psw = get_reg (RL78_Reg_PSW);
  psw &= ~mask;

  if (mask & RL78_PSW_CY)
    {
      if ((after & cmask) != (before & cmask))
	psw |= RL78_PSW_CY;
    }
  if (mask & RL78_PSW_AC)
    {
      if ((after & amask) != (before & amask)
	  && (after & avmask) < (before & avmask))
	psw |= RL78_PSW_AC;
    }
  if (mask & RL78_PSW_Z)
    {
      if (! (after & vmask))
	psw |= RL78_PSW_Z;
    }

  set_reg (RL78_Reg_PSW, psw);
}

#define FLAGS(before,after) if (opcode.flags) op_flags (before, after, opcode.flags, opcode.size)

#define PD(x) put_op (&opcode, 0, 1, x)
#define PS(x) put_op (&opcode, 1, 1, x)
#define GD() get_op (&opcode, 0, 1)
#define GS() get_op (&opcode, 1, 1)

#define GPC() gpc (&opcode, 0)
static int
gpc (RL78_Opcode_Decoded *opcode, int idx)
{
  int a = get_op (opcode, 0, 1);
  if (opcode->op[idx].type == RL78_Operand_Register)
    a =(a & 0x0ffff) | ((get_reg (RL78_Reg_CS) & 0x0f) << 16);
  else
    a &= 0xfffff;
  return a;
}

static int
get_carry (void)
{
  return (get_reg (RL78_Reg_PSW) & RL78_PSW_CY) ? 1 : 0;
}

static void
set_carry (int c)
{
  int p = get_reg (RL78_Reg_PSW);
  tprintf ("set_carry (%d)\n", c ? 1 : 0);
  if (c)
    p |= RL78_PSW_CY;
  else
    p &= ~RL78_PSW_CY;
  set_reg (RL78_Reg_PSW, p);
}

/* We simulate timer TM00 in interval mode, no clearing, with
   interrupts.  I.e. it's a cycle counter.  */

unsigned int counts_per_insn[0x100000];

int pending_clocks = 0;
long long total_clocks = 0;

#define TCR0	0xf0180
#define	MK1	0xfffe6
static void
process_clock_tick (void)
{
  unsigned short cnt;
  unsigned short ivect;
  unsigned short mask;
  unsigned char psw;
  int save_trace;

  save_trace = trace;
  trace = 0;

  pending_clocks ++;

  counts_per_insn[opcode_pc] += pending_clocks;
  total_clocks += pending_clocks;

  while (pending_clocks)
    {
      pending_clocks --;
      cnt = mem_get_hi (TCR0);
      cnt --;
      mem_put_hi (TCR0, cnt);
      if (cnt != 0xffff)
	continue;

      /* overflow.  */
      psw = get_reg (RL78_Reg_PSW);
      ivect = mem_get_hi (0x0002c);
      mask = mem_get_hi (MK1);

      if ((psw & RL78_PSW_IE)
	  && (ivect != 0)
	  && !(mask & 0x0010))
	{
	  unsigned short sp = get_reg (RL78_Reg_SP);
	  set_reg (RL78_Reg_SP, sp - 4);
	  sp --;
	  mem_put_qi (sp | 0xf0000, psw);
	  sp -= 3;
	  mem_put_psi (sp | 0xf0000, pc);
	  psw &= ~RL78_PSW_IE;
	  set_reg (RL78_Reg_PSW, psw);
	  pc = ivect;
	  /* Spec says 9-14 clocks */
	  pending_clocks += 9;
	}
    }

  trace = save_trace;
}

void
dump_counts_per_insn (const char * filename)
{
  int i;
  FILE *f;
  f = fopen (filename, "w");
  if (!f)
    {
      perror (filename);
      return;
    }
  for (i = 0; i < 0x100000; i ++)
    {
      if (counts_per_insn[i])
	fprintf (f, "%05x %d\n", i, counts_per_insn[i]);
    }
  fclose (f);
}

static void
CLOCKS (int n)
{
  pending_clocks += n - 1;
}

int
decode_opcode (void)
{
  RL78_Data rl78_data;
  RL78_Opcode_Decoded opcode;
  int opcode_size;
  int a, b, v, v2;
  unsigned int u, u2;
  int obits;
  RL78_Dis_Isa isa;

  isa = (rl78_g10_mode ? RL78_ISA_G10
	: g14_multiply ? RL78_ISA_G14
	: g13_multiply ? RL78_ISA_G13
	: RL78_ISA_DEFAULT);

  rl78_data.dpc = pc;
  opcode_size = rl78_decode_opcode (pc, &opcode,
				    rl78_get_byte, &rl78_data, isa);

  opcode_pc = pc;
  pc += opcode_size;

  trace_register_words = opcode.size == RL78_Word ? 1 : 0;

  /* Used by shfit/rotate instructions */
  obits = opcode.size == RL78_Word ? 16 : 8;

  switch (opcode.id)
    {
    case RLO_add:
      tprintf ("ADD: ");
      a = GS ();
      b = GD ();
      v = a + b;
      FLAGS (b, v);
      PD (v);
      if (opcode.op[0].type == RL78_Operand_Indirect)
	CLOCKS (2);
      break;

    case RLO_addc:
      tprintf ("ADDC: ");
      a = GS ();
      b = GD ();
      v = a + b + get_carry ();
      FLAGS (b, v);
      PD (v);
      if (opcode.op[0].type == RL78_Operand_Indirect)
	CLOCKS (2);
      break;

    case RLO_and:
      tprintf ("AND: ");
      a = GS ();
      b = GD ();
      v = a & b;
      FLAGS (b, v);
      PD (v);
      if (opcode.op[0].type == RL78_Operand_Indirect)
	CLOCKS (2);
      break;

    case RLO_branch_cond:
    case RLO_branch_cond_clear:
      tprintf ("BRANCH_COND: ");
      if (!condition_true (opcode.op[1].condition, GS ()))
	{
	  tprintf (" false\n");
	  if (opcode.op[1].condition == RL78_Condition_T
	      || opcode.op[1].condition == RL78_Condition_F)
	    CLOCKS (3);
	  else
	    CLOCKS (2);
	  break;
	}
      if (opcode.id == RLO_branch_cond_clear)
	PS (0);
      tprintf (" ");
      if (opcode.op[1].condition == RL78_Condition_T
	  || opcode.op[1].condition == RL78_Condition_F)
	CLOCKS (3); /* note: adds two clocks, total 5 clocks */
      else
	CLOCKS (2); /* note: adds one clock, total 4 clocks */
      ATTRIBUTE_FALLTHROUGH;
    case RLO_branch:
      tprintf ("BRANCH: ");
      v = GPC ();
      WILD_JUMP_CHECK (v);
      pc = v;
      tprintf (" => 0x%05x\n", pc);
      CLOCKS (3);
      break;

    case RLO_break:
      tprintf ("BRK: ");
      CLOCKS (5);
      if (rl78_in_gdb)
	DO_RETURN (RL78_MAKE_HIT_BREAK ());
      else
	DO_RETURN (RL78_MAKE_EXITED (1));
      break;

    case RLO_call:
      tprintf ("CALL: ");
      a = get_reg (RL78_Reg_SP);
      set_reg (RL78_Reg_SP, a - 4);
      mem_put_psi ((a - 4) | 0xf0000, pc);
      v = GPC ();
      WILD_JUMP_CHECK (v);
      pc = v;
#if 0
      /* Enable this code to dump the arguments for each call.  */
      if (trace)
	{
	  int i;
	  skip_init ++;
	  for (i = 0; i < 8; i ++)
	    printf (" %02x", mem_get_qi (0xf0000 | (a + i)) & 0xff);
	  skip_init --;
	}
#endif
      tprintf ("\n");
      CLOCKS (3);
      break;

    case RLO_cmp:
      tprintf ("CMP: ");
      a = GD ();
      b = GS ();
      v = a - b;
      FLAGS (b, v);
      tprintf (" (%d)\n", v);
      break;

    case RLO_divhu:
      a = get_reg (RL78_Reg_AX);
      b = get_reg (RL78_Reg_DE);
      tprintf (" %d / %d = ", a, b);
      if (b == 0)
	{
	  tprintf ("%d rem %d\n", 0xffff, a);
	  set_reg (RL78_Reg_AX, 0xffff);
	  set_reg (RL78_Reg_DE, a);
	}
      else
	{
	  v = a / b;
	  a = a % b;
	  tprintf ("%d rem %d\n", v, a);
	  set_reg (RL78_Reg_AX, v);
	  set_reg (RL78_Reg_DE, a);
	}
      CLOCKS (9);
      break;

    case RLO_divwu:
      {
	unsigned long bcax, hlde, quot, rem;
	bcax = get_reg (RL78_Reg_AX) + 65536 * get_reg (RL78_Reg_BC);
	hlde = get_reg (RL78_Reg_DE) + 65536 * get_reg (RL78_Reg_HL);

	tprintf (" %lu / %lu = ", bcax, hlde);
	if (hlde == 0)
	  {
	    tprintf ("%lu rem %lu\n", 0xffffLU, bcax);
	    set_reg (RL78_Reg_AX, 0xffffLU);
	    set_reg (RL78_Reg_BC, 0xffffLU);
	    set_reg (RL78_Reg_DE, bcax);
	    set_reg (RL78_Reg_HL, bcax >> 16);
	  }
	else
	  {
	    quot = bcax / hlde;
	    rem = bcax % hlde;
	    tprintf ("%lu rem %lu\n", quot, rem);
	    set_reg (RL78_Reg_AX, quot);
	    set_reg (RL78_Reg_BC, quot >> 16);
	    set_reg (RL78_Reg_DE, rem);
	    set_reg (RL78_Reg_HL, rem >> 16);
	  }
      }
      CLOCKS (17);
      break;

    case RLO_halt:
      tprintf ("HALT.\n");
      DO_RETURN (RL78_MAKE_EXITED (get_reg (RL78_Reg_A)));

    case RLO_mov:
      tprintf ("MOV: ");
      a = GS ();
      FLAGS (a, a);
      PD (a);
      break;

#define MACR 0xffff0
    case RLO_mach:
      tprintf ("MACH:");
      a = sign_ext (get_reg (RL78_Reg_AX), 16);
      b = sign_ext (get_reg (RL78_Reg_BC), 16);
      v = sign_ext (mem_get_si (MACR), 32);
      tprintf ("%08x %d + %d * %d = ", v, v, a, b);
      v2 = sign_ext (v + a * b, 32);
      tprintf ("%08x %d\n", v2, v2);
      mem_put_si (MACR, v2);
      a = get_reg (RL78_Reg_PSW);
      v ^= v2;
      if (v & (1<<31))
	a |= RL78_PSW_CY;
      else
	a &= ~RL78_PSW_CY;
      if (v2 & (1 << 31))
	a |= RL78_PSW_AC;
      else
	a &= ~RL78_PSW_AC;
      set_reg (RL78_Reg_PSW, a);
      CLOCKS (3);
      break;

    case RLO_machu:
      tprintf ("MACHU:");
      a = get_reg (RL78_Reg_AX);
      b = get_reg (RL78_Reg_BC);
      u = mem_get_si (MACR);
      tprintf ("%08x %u + %u * %u = ", u, u, a, b);
      u2 = (u + (unsigned)a * (unsigned)b) & 0xffffffffUL;
      tprintf ("%08x %u\n", u2, u2);
      mem_put_si (MACR, u2);
      a = get_reg (RL78_Reg_PSW);
      if (u2 < u)
	a |= RL78_PSW_CY;
      else
	a &= ~RL78_PSW_CY;
      a &= ~RL78_PSW_AC;
      set_reg (RL78_Reg_PSW, a);
      CLOCKS (3);
      break;

    case RLO_mulu:
      tprintf ("MULU:");
      a = get_reg (RL78_Reg_A);
      b = get_reg (RL78_Reg_X);
      v = a * b;
      tprintf (" %d * %d = %d\n", a, b, v);
      set_reg (RL78_Reg_AX, v);
      break;

    case RLO_mulh:
      tprintf ("MUL:");
      a = sign_ext (get_reg (RL78_Reg_AX), 16);
      b = sign_ext (get_reg (RL78_Reg_BC), 16);
      v = a * b;
      tprintf (" %d * %d = %d\n", a, b, v);
      set_reg (RL78_Reg_BC, v >> 16);
      set_reg (RL78_Reg_AX, v);
      CLOCKS (2);
      break;

    case RLO_mulhu:
      tprintf ("MULHU:");
      a = get_reg (RL78_Reg_AX);
      b = get_reg (RL78_Reg_BC);
      v = a * b;
      tprintf (" %d * %d = %d\n", a, b, v);
      set_reg (RL78_Reg_BC, v >> 16);
      set_reg (RL78_Reg_AX, v);
      CLOCKS (2);
      break;

    case RLO_nop:
      tprintf ("NOP.\n");
      break;

    case RLO_or:
      tprintf ("OR:");
      a = GS ();
      b = GD ();
      v = a | b;
      FLAGS (b, v);
      PD (v);
      if (opcode.op[0].type == RL78_Operand_Indirect)
	CLOCKS (2);
      break;

    case RLO_ret:
      tprintf ("RET: ");
      a = get_reg (RL78_Reg_SP);
      v = mem_get_psi (a | 0xf0000);
      WILD_JUMP_CHECK (v);
      pc = v;
      set_reg (RL78_Reg_SP, a + 4);
#if 0
      /* Enable this code to dump the return values for each return.  */
      if (trace)
	{
	  int i;
	  skip_init ++;
	  for (i = 0; i < 8; i ++)
	    printf (" %02x", mem_get_qi (0xffef0 + i) & 0xff);
	  skip_init --;
	}
#endif
      tprintf ("\n");
      CLOCKS (6);
      break;

    case RLO_reti:
      tprintf ("RETI: ");
      a = get_reg (RL78_Reg_SP);
      v = mem_get_psi (a | 0xf0000);
      WILD_JUMP_CHECK (v);
      pc = v;
      b = mem_get_qi ((a + 3) | 0xf0000);
      set_reg (RL78_Reg_PSW, b);
      set_reg (RL78_Reg_SP, a + 4);
      tprintf ("\n");
      break;

    case RLO_rol:
      tprintf ("ROL:"); /* d <<= s */
      a = GS ();
      b = GD ();
      v = b;
      while (a --)
	{
	  v = b << 1;
	  v |= (b >> (obits - 1)) & 1;
	  set_carry ((b >> (obits - 1)) & 1);
	  b = v;
	}
      PD (v);
      break;

    case RLO_rolc:
      tprintf ("ROLC:"); /* d <<= s */
      a = GS ();
      b = GD ();
      v = b;
      while (a --)
	{
	  v = b << 1;
	  v |= get_carry ();
	  set_carry ((b >> (obits - 1)) & 1);
	  b = v;
	}
      PD (v);
      break;

    case RLO_ror:
      tprintf ("ROR:"); /* d >>= s */
      a = GS ();
      b = GD ();
      v = b;
      while (a --)
	{
	  v = b >> 1;
	  v |= (b & 1) << (obits - 1);
	  set_carry (b & 1);
	  b = v;
	}
      PD (v);
      break;

    case RLO_rorc:
      tprintf ("RORC:"); /* d >>= s */
      a = GS ();
      b = GD ();
      v = b;
      while (a --)
	{
	  v = b >> 1;
	  v |= (get_carry () << (obits - 1));
	  set_carry (b & 1);
	  b = v;
	}
      PD (v);
      break;

    case RLO_sar:
      tprintf ("SAR:"); /* d >>= s */
      a = GS ();
      b = GD ();
      v = b;
      while (a --)
	{
	  v = b >> 1;
	  v |= b & (1 << (obits - 1));
	  set_carry (b & 1);
	  b = v;
	}
      PD (v);
      break;

    case RLO_sel:
      tprintf ("SEL:");
      a = GS ();
      b = get_reg (RL78_Reg_PSW);
      b &= ~(RL78_PSW_RBS1 | RL78_PSW_RBS0);
      if (a & 1)
	b |= RL78_PSW_RBS0;
      if (a & 2)
	b |= RL78_PSW_RBS1;
      set_reg (RL78_Reg_PSW, b);
      tprintf ("\n");
      break;

    case RLO_shl:
      tprintf ("SHL%d:", obits); /* d <<= s */
      a = GS ();
      b = GD ();
      v = b;
      while (a --)
	{
	  v = b << 1;
	  tprintf ("b = 0x%x & 0x%x\n", b, 1<<(obits - 1));
	  set_carry (b & (1<<(obits - 1)));
	  b = v;
	}
      PD (v);
      break;

    case RLO_shr:
      tprintf ("SHR:"); /* d >>= s */
      a = GS ();
      b = GD ();
      v = b;
      while (a --)
	{
	  v = b >> 1;
	  set_carry (b & 1);
	  b = v;
	}
      PD (v);
      break;

    case RLO_skip:
      tprintf ("SKIP: ");
      if (!condition_true (opcode.op[1].condition, GS ()))
	{
	  tprintf (" false\n");
	  break;
	}

      rl78_data.dpc = pc;
      opcode_size = rl78_decode_opcode (pc, &opcode,
					rl78_get_byte, &rl78_data, isa);
      pc += opcode_size;
      tprintf (" skipped: %s\n", opcode.syntax);
      break;

    case RLO_stop:
      tprintf ("STOP.\n");
      DO_RETURN (RL78_MAKE_EXITED (get_reg (RL78_Reg_A)));
      DO_RETURN (RL78_MAKE_HIT_BREAK ());

    case RLO_sub:
      tprintf ("SUB: ");
      a = GS ();
      b = GD ();
      v = b - a;
      FLAGS (b, v);
      PD (v);
      tprintf ("%d (0x%x) - %d (0x%x) = %d (0x%x)\n", b, b, a, a, v, v);
      if (opcode.op[0].type == RL78_Operand_Indirect)
	CLOCKS (2);
      break;

    case RLO_subc:
      tprintf ("SUBC: ");
      a = GS ();
      b = GD ();
      v = b - a - get_carry ();
      FLAGS (b, v);
      PD (v);
      if (opcode.op[0].type == RL78_Operand_Indirect)
	CLOCKS (2);
      break;

    case RLO_xch:
      tprintf ("XCH: ");
      a = GS ();
      b = GD ();
      PD (a);
      PS (b);
      break;

    case RLO_xor:
      tprintf ("XOR:");
      a = GS ();
      b = GD ();
      v = a ^ b;
      FLAGS (b, v);
      PD (v);
      if (opcode.op[0].type == RL78_Operand_Indirect)
	CLOCKS (2);
      break;

    default:
      tprintf ("Unknown opcode?\n");
      DO_RETURN (RL78_MAKE_HIT_BREAK ());
    }

  if (timer_enabled)
    process_clock_tick ();

  return RL78_MAKE_STEPPED ();
}
