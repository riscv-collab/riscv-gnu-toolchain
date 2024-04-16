/* Simulator for Atmel's AVR core.
   Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Written by Tristan Gingold, AdaCore.

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

#include <string.h>

#include "bfd.h"
#include "libiberty.h"
#include "sim/sim.h"

#include "sim-main.h"
#include "sim-base.h"
#include "sim-options.h"
#include "sim-signal.h"
#include "avr-sim.h"

/* As AVR is a 8/16 bits processor, define handy types.  */
typedef unsigned short int word;
typedef signed short int sword;
typedef unsigned char byte;
typedef signed char sbyte;

/* Max size of I space (which is always flash on avr).  */
#define MAX_AVR_FLASH (128 * 1024)
#define PC_MASK (MAX_AVR_FLASH - 1)

/* Mac size of D space.  */
#define MAX_AVR_SRAM (64 * 1024)
#define SRAM_MASK (MAX_AVR_SRAM - 1)

/* D space offset in ELF file.  */
#define SRAM_VADDR 0x800000

/* Simulator specific ports.  */
#define STDIO_PORT	0x52
#define EXIT_PORT	0x4F
#define ABORT_PORT	0x49

/* GDB defined register numbers.  */
#define AVR_SREG_REGNUM  32
#define AVR_SP_REGNUM    33
#define AVR_PC_REGNUM    34

/* Memory mapped registers.  */
#define SREG	0x5F
#define REG_SP	0x5D
#define EIND	0x5C
#define RAMPZ	0x5B

#define REGX 0x1a
#define REGY 0x1c
#define REGZ 0x1e
#define REGZ_LO 0x1e
#define REGZ_HI 0x1f

/* Sreg (status) bits.  */
#define SREG_I 0x80
#define SREG_T 0x40
#define SREG_H 0x20
#define SREG_S 0x10
#define SREG_V 0x08
#define SREG_N 0x04
#define SREG_Z 0x02
#define SREG_C 0x01

/* In order to speed up emulation we use a simple approach:
   a code is associated with each instruction.  The pre-decoding occurs
   usually once when the instruction is first seen.
   This works well because I&D spaces are separated.

   Missing opcodes: sleep, spm, wdr (as they are mmcu dependent).
*/
enum avr_opcode
  {
    /* Opcode not yet decoded.  */
    OP_unknown,
    OP_bad,

    OP_nop,

    OP_rjmp,
    OP_rcall,
    OP_ret,
    OP_reti,

    OP_break,

    OP_brbs,
    OP_brbc,

    OP_bset,
    OP_bclr,

    OP_bld,
    OP_bst,

    OP_sbrc,
    OP_sbrs,

    OP_eor,
    OP_and,
    OP_andi,
    OP_or,
    OP_ori,
    OP_com,
    OP_swap,
    OP_neg,

    OP_out,
    OP_in,
    OP_cbi,
    OP_sbi,

    OP_sbic,
    OP_sbis,

    OP_ldi,
    OP_cpse,
    OP_cp,
    OP_cpi,
    OP_cpc,
    OP_sub,
    OP_sbc,
    OP_sbiw,
    OP_adiw,
    OP_add,
    OP_adc,
    OP_subi,
    OP_sbci,
    OP_inc,
    OP_dec,
    OP_lsr,
    OP_ror,
    OP_asr,

    OP_mul,
    OP_muls,
    OP_mulsu,
    OP_fmul,
    OP_fmuls,
    OP_fmulsu,

    OP_mov,
    OP_movw,

    OP_push,
    OP_pop,

    OP_st_X,
    OP_st_dec_X,
    OP_st_X_inc,
    OP_st_Y_inc,
    OP_st_dec_Y,
    OP_st_Z_inc,
    OP_st_dec_Z,
    OP_std_Y,
    OP_std_Z,
    OP_ldd_Y,
    OP_ldd_Z,
    OP_ld_Z_inc,
    OP_ld_dec_Z,
    OP_ld_Y_inc,
    OP_ld_dec_Y,
    OP_ld_X,
    OP_ld_X_inc,
    OP_ld_dec_X,
    
    OP_lpm,
    OP_lpm_Z,
    OP_lpm_inc_Z,
    OP_elpm,
    OP_elpm_Z,
    OP_elpm_inc_Z,

    OP_ijmp,
    OP_icall,

    OP_eijmp,
    OP_eicall,

    /* 2 words opcodes.  */
#define OP_2words OP_jmp
    OP_jmp,
    OP_call,
    OP_sts,
    OP_lds
  };

struct avr_insn_cell
{
  /* The insn (16 bits).  */
  word op;

  /* Pre-decoding code.  */
  enum avr_opcode code : 8;
  /* One byte of additional information.  */
  byte r;
};

/* I&D memories.  */
/* TODO: Should be moved to SIM_CPU.  */
static struct avr_insn_cell flash[MAX_AVR_FLASH];
static byte sram[MAX_AVR_SRAM];

/* Sign extend a value.  */
static int sign_ext (word val, int nb_bits)
{
  if (val & (1 << (nb_bits - 1)))
    return val | -(1 << nb_bits);
  return val;
}

/* Insn field extractors.  */

/* Extract xxxx_xxxRx_xxxx_RRRR.  */
static inline byte get_r (word op)
{
  return (op & 0xf) | ((op >> 5) & 0x10);
}

/* Extract xxxx_xxxxx_xxxx_RRRR.  */
static inline byte get_r16 (word op)
{
  return 16 + (op & 0xf);
}

/* Extract xxxx_xxxxx_xxxx_xRRR.  */
static inline byte get_r16_23 (word op)
{
  return 16 + (op & 0x7);
}

/* Extract xxxx_xxxD_DDDD_xxxx.  */
static inline byte get_d (word op)
{
  return (op >> 4) & 0x1f;
}

/* Extract xxxx_xxxx_DDDD_xxxx.  */
static inline byte get_d16 (word op)
{
  return 16 + ((op >> 4) & 0x0f);
}

/* Extract xxxx_xxxx_xDDD_xxxx.  */
static inline byte get_d16_23 (word op)
{
  return 16 + ((op >> 4) & 0x07);
}

/* Extract xxxx_xAAx_xxxx_AAAA.  */
static inline byte get_A (word op)
{
  return (op & 0x0f) | ((op & 0x600) >> 5);
}

/* Extract xxxx_xxxx_AAAA_Axxx.  */
static inline byte get_biA (word op)
{
  return (op >> 3) & 0x1f;
}

/* Extract xxxx_KKKK_xxxx_KKKK.  */
static inline byte get_K (word op)
{
  return (op & 0xf) | ((op & 0xf00) >> 4);
}

/* Extract xxxx_xxKK_KKKK_Kxxx.  */
static inline int get_k (word op)
{
  return sign_ext ((op & 0x3f8) >> 3, 7);
}

/* Extract xxxx_xxxx_xxDD_xxxx.  */
static inline byte get_d24 (word op)
{
  return 24 + ((op >> 3) & 6);
}

/* Extract xxxx_xxxx_KKxx_KKKK.  */
static inline byte get_k6 (word op)
{
  return (op & 0xf) | ((op >> 2) & 0x30);
}
 
/* Extract xxQx_QQxx_xxxx_xQQQ.  */
static inline byte get_q (word op)
{
  return (op & 7) | ((op >> 7) & 0x18)| ((op >> 8) & 0x20);
}

/* Extract xxxx_xxxx_xxxx_xBBB.  */
static inline byte get_b (word op)
{
  return (op & 7);
}

/* AVR is little endian.  */
static inline word
read_word (unsigned int addr)
{
  return sram[addr] | (sram[addr + 1] << 8);
}

static inline void
write_word (unsigned int addr, word w)
{
  sram[addr] = w;
  sram[addr + 1] = w >> 8;
}

static inline word
read_word_post_inc (unsigned int addr)
{
  word v = read_word (addr);
  write_word (addr, v + 1);
  return v;
}

static inline word
read_word_pre_dec (unsigned int addr)
{
  word v = read_word (addr) - 1;
  write_word (addr, v);
  return v;
}

static void
update_flags_logic (byte res)
{
  sram[SREG] &= ~(SREG_S | SREG_V | SREG_N | SREG_Z);
  if (res == 0)
    sram[SREG] |= SREG_Z;
  if (res & 0x80)
    sram[SREG] |= SREG_N | SREG_S;
}

static void
update_flags_add (byte r, byte a, byte b)
{
  byte carry;

  sram[SREG] &= ~(SREG_H | SREG_S | SREG_V | SREG_N | SREG_Z | SREG_C);
  if (r & 0x80)
    sram[SREG] |= SREG_N;
  carry = (a & b) | (a & ~r) | (b & ~r);
  if (carry & 0x08)
    sram[SREG] |= SREG_H;
  if (carry & 0x80)
    sram[SREG] |= SREG_C;
  if (((a & b & ~r) | (~a & ~b & r)) & 0x80)
    sram[SREG] |= SREG_V;
  if (!(sram[SREG] & SREG_N) ^ !(sram[SREG] & SREG_V))
    sram[SREG] |= SREG_S;
  if (r == 0)
    sram[SREG] |= SREG_Z;
}

static void update_flags_sub (byte r, byte a, byte b)
{
  byte carry;

  sram[SREG] &= ~(SREG_H | SREG_S | SREG_V | SREG_N | SREG_Z | SREG_C);
  if (r & 0x80)
    sram[SREG] |= SREG_N;
  carry = (~a & b) | (b & r) | (r & ~a);
  if (carry & 0x08)
    sram[SREG] |= SREG_H;
  if (carry & 0x80)
    sram[SREG] |= SREG_C;
  if (((a & ~b & ~r) | (~a & b & r)) & 0x80)
    sram[SREG] |= SREG_V;
  if (!(sram[SREG] & SREG_N) ^ !(sram[SREG] & SREG_V))
    sram[SREG] |= SREG_S;
  /* Note: Z is not set.  */
}

static enum avr_opcode
decode (unsigned int pc)
{
  word op1 = flash[pc].op;

  switch ((op1 >> 12) & 0x0f)
    {
    case 0x0:
      switch ((op1 >> 10) & 0x3)
        {
        case 0x0:
          switch ((op1 >> 8) & 0x3)
            {
            case 0x0:
              if (op1 == 0)
                return OP_nop;
              break;
            case 0x1:
              return OP_movw;
            case 0x2:
              return OP_muls;
            case 0x3:
              if (op1 & 0x80)
                {
                  if (op1 & 0x08)
                    return OP_fmulsu;
                  else
                    return OP_fmuls;
                }
              else
                {
                  if (op1 & 0x08)
                    return OP_fmul;
                  else
                    return OP_mulsu;
                }
            }
          break;
        case 0x1:
          return OP_cpc;
        case 0x2:
          flash[pc].r = SREG_C;
          return OP_sbc;
        case 0x3:
          flash[pc].r = 0;
          return OP_add;
        }
      break;
    case 0x1:
      switch ((op1 >> 10) & 0x3)
        {
        case 0x0:
          return OP_cpse;
        case 0x1:
          return OP_cp;
        case 0x2:
          flash[pc].r = 0;
          return OP_sub;
        case 0x3:
          flash[pc].r = SREG_C;
          return OP_adc;
        }
      break;
    case 0x2:
      switch ((op1 >> 10) & 0x3)
        {
        case 0x0:
          return OP_and;
        case 0x1:
          return OP_eor;
        case 0x2:
          return OP_or;
        case 0x3:
          return OP_mov;
        }
      break;
    case 0x3:
      return OP_cpi;
    case 0x4:
      return OP_sbci;
    case 0x5:
      return OP_subi;
    case 0x6:
      return OP_ori;
    case 0x7:
      return OP_andi;
    case 0x8:
    case 0xa:
      if (op1 & 0x0200)
        {
          if (op1 & 0x0008)
            {
              flash[pc].r = get_q (op1);
              return OP_std_Y;
            }
          else
            {
              flash[pc].r = get_q (op1);
              return OP_std_Z;
            }
        }
      else
        {
          if (op1 & 0x0008)
            {
              flash[pc].r = get_q (op1);
              return OP_ldd_Y;
            }
          else
            {
              flash[pc].r = get_q (op1);
              return OP_ldd_Z;
            }
        }
      break;
    case 0x9: /* 9xxx */
      switch ((op1 >> 8) & 0xf)
        {
        case 0x0:
        case 0x1:
          switch ((op1 >> 0) & 0xf)
            {
            case 0x0:
              return OP_lds;
            case 0x1:
              return OP_ld_Z_inc;
            case 0x2:
              return OP_ld_dec_Z;
            case 0x4:
              return OP_lpm_Z;
            case 0x5:
              return OP_lpm_inc_Z;
            case 0x6:
              return OP_elpm_Z;
            case 0x7:
              return OP_elpm_inc_Z;
            case 0x9:
              return OP_ld_Y_inc;
            case 0xa:
              return OP_ld_dec_Y;
            case 0xc:
              return OP_ld_X;
            case 0xd:
              return OP_ld_X_inc;
            case 0xe:
              return OP_ld_dec_X;
            case 0xf:
              return OP_pop;
            }
          break;
        case 0x2:
        case 0x3:
          switch ((op1 >> 0) & 0xf)
            {
            case 0x0:
              return OP_sts;
            case 0x1:
              return OP_st_Z_inc;
            case 0x2:
              return OP_st_dec_Z;
            case 0x9:
              return OP_st_Y_inc;
            case 0xa:
              return OP_st_dec_Y;
            case 0xc:
              return OP_st_X;
            case 0xd:
              return OP_st_X_inc;
            case 0xe:
              return OP_st_dec_X;
            case 0xf:
              return OP_push;
            }
          break;
        case 0x4:
        case 0x5:
          switch (op1 & 0xf)
            {
            case 0x0:
              return OP_com;
            case 0x1:
              return OP_neg;
            case 0x2:
              return OP_swap;
            case 0x3:
              return OP_inc;
            case 0x5:
              flash[pc].r = 0x80;
              return OP_asr;
            case 0x6:
              flash[pc].r = 0;
              return OP_lsr;
            case 0x7:
              return OP_ror;
            case 0x8: /* 9[45]x8 */
              switch ((op1 >> 4) & 0x1f)
                {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                  return OP_bset;
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x0e:
                case 0x0f:
                  return OP_bclr;
                case 0x10:
                  return OP_ret;
                case 0x11:
                  return OP_reti;
                case 0x19:
                  return OP_break;
                case 0x1c:
                  return OP_lpm;
                case 0x1d:
                  return OP_elpm;
                default:
                  break;
                }
              break;
            case 0x9: /* 9[45]x9 */
              switch ((op1 >> 4) & 0x1f)
                {
                case 0x00:
                  return OP_ijmp;
                case 0x01:
                  return OP_eijmp;
                case 0x10:
                  return OP_icall;
                case 0x11:
                  return OP_eicall;
                default:
                  break;
                }
              break;
            case 0xa:
              return OP_dec;
            case 0xc:
            case 0xd:
              flash[pc].r = ((op1 & 0x1f0) >> 3) | (op1 & 1);
              return OP_jmp;
            case 0xe:
            case 0xf:
              flash[pc].r = ((op1 & 0x1f0) >> 3) | (op1 & 1);
              return OP_call;
            }
          break;
        case 0x6:
          return OP_adiw;
        case 0x7:
          return OP_sbiw;
        case 0x8:
          return OP_cbi;
        case 0x9:
          return OP_sbic;
        case 0xa:
          return OP_sbi;
        case 0xb:
          return OP_sbis;
        case 0xc:
        case 0xd:
        case 0xe:
        case 0xf:
          return OP_mul;
        }
      break;
    case 0xb:
      flash[pc].r = get_A (op1);
      if (((op1 >> 11) & 1) == 0)
        return OP_in;
      else
        return OP_out;
    case 0xc:
      return OP_rjmp;
    case 0xd:
      return OP_rcall;
    case 0xe:
      return OP_ldi;
    case 0xf:
      switch ((op1 >> 9) & 7)
        {
        case 0:
        case 1:
          flash[pc].r = 1 << (op1 & 7);
          return OP_brbs;
        case 2:
        case 3:
          flash[pc].r = 1 << (op1 & 7);
          return OP_brbc;
        case 4:
          if ((op1 & 8) == 0)
            {
              flash[pc].r = 1 << (op1 & 7);
              return OP_bld;
            }
          break;
        case 5:
          if ((op1 & 8) == 0)
            {
              flash[pc].r = 1 << (op1 & 7);
              return OP_bst;
            }
          break;
        case 6:
          if ((op1 & 8) == 0)
            {
              flash[pc].r = 1 << (op1 & 7);
              return OP_sbrc;
            }
          break;
        case 7:
          if ((op1 & 8) == 0)
            {
              flash[pc].r = 1 << (op1 & 7);
              return OP_sbrs;
            }
          break;
        }
    }

  return OP_bad;
}

static void
do_call (SIM_CPU *cpu, unsigned int npc)
{
  const struct avr_sim_state *state = AVR_SIM_STATE (CPU_STATE (cpu));
  struct avr_sim_cpu *avr_cpu = AVR_SIM_CPU (cpu);
  unsigned int sp = read_word (REG_SP);

  /* Big endian!  */
  sram[sp--] = avr_cpu->pc;
  sram[sp--] = avr_cpu->pc >> 8;
  if (state->avr_pc22)
    {
      sram[sp--] = avr_cpu->pc >> 16;
      avr_cpu->cycles++;
    }
  write_word (REG_SP, sp);
  avr_cpu->pc = npc & PC_MASK;
  avr_cpu->cycles += 3;
}

static int
get_insn_length (unsigned int p)
{
  if (flash[p].code == OP_unknown)
    flash[p].code = decode(p);
  if (flash[p].code >= OP_2words)
    return 2;
  else
    return 1;
}

static unsigned int
get_z (void)
{
  return (sram[RAMPZ] << 16) | (sram[REGZ_HI] << 8) | sram[REGZ_LO];
}

static unsigned char
get_lpm (unsigned int addr)
{
  word w;

  w = flash[(addr >> 1) & PC_MASK].op;
  if (addr & 1)
    w >>= 8;
  return w;
}

static void
gen_mul (SIM_CPU *cpu, unsigned int res)
{
  struct avr_sim_cpu *avr_cpu = AVR_SIM_CPU (cpu);

  write_word (0, res);
  sram[SREG] &= ~(SREG_Z | SREG_C);
  if (res == 0)
    sram[SREG] |= SREG_Z;
  if (res & 0x8000)
    sram[SREG] |= SREG_C;
  avr_cpu->cycles++;
}

static void
step_once (SIM_CPU *cpu)
{
  struct avr_sim_cpu *avr_cpu = AVR_SIM_CPU (cpu);
  unsigned int ipc;

  int code;
  word op;
  byte res;
  byte r, d, vd;

 again:
  code = flash[avr_cpu->pc].code;
  op = flash[avr_cpu->pc].op;

#if 0
      if (tracing && code != OP_unknown)
	{
	  if (verbose > 0) {
	    int flags;
	    int i;

	    sim_cb_eprintf (callback, "R00-07:");
	    for (i = 0; i < 8; i++)
	      sim_cb_eprintf (callback, " %02x", sram[i]);
	    sim_cb_eprintf (callback, " -");
	    for (i = 8; i < 16; i++)
	      sim_cb_eprintf (callback, " %02x", sram[i]);
	    sim_cb_eprintf (callback, "  SP: %02x %02x",
                            sram[REG_SP + 1], sram[REG_SP]);
	    sim_cb_eprintf (callback, "\n");
	    sim_cb_eprintf (callback, "R16-31:");
	    for (i = 16; i < 24; i++)
	      sim_cb_eprintf (callback, " %02x", sram[i]);
	    sim_cb_eprintf (callback, " -");
	    for (i = 24; i < 32; i++)
	      sim_cb_eprintf (callback, " %02x", sram[i]);
	    sim_cb_eprintf (callback, "  ");
	    flags = sram[SREG];
	    for (i = 0; i < 8; i++)
	      sim_cb_eprintf (callback, "%c",
                              flags & (0x80 >> i) ? "ITHSVNZC"[i] : '-');
	    sim_cb_eprintf (callback, "\n");
	  }

	  if (!tracing)
	    sim_cb_eprintf (callback, "%06x: %04x\n", 2 * avr_cpu->pc, flash[avr_cpu->pc].op);
	  else
	    {
	      sim_cb_eprintf (callback, "pc=0x%06x insn=0x%04x code=%d r=%d\n",
                              2 * avr_cpu->pc, flash[avr_cpu->pc].op, code, flash[avr_cpu->pc].r);
	      disassemble_insn (CPU_STATE (cpu), avr_cpu->pc);
	      sim_cb_eprintf (callback, "\n");
	    }
	}
#endif

  ipc = avr_cpu->pc;
  avr_cpu->pc = (avr_cpu->pc + 1) & PC_MASK;
  avr_cpu->cycles++;

  switch (code)
    {
      case OP_unknown:
	flash[ipc].code = decode(ipc);
	avr_cpu->pc = ipc;
	avr_cpu->cycles--;
	goto again;

      case OP_nop:
	break;

      case OP_jmp:
	/* 2 words instruction, but we don't care about the pc.  */
	avr_cpu->pc = ((flash[ipc].r << 16) | flash[ipc + 1].op) & PC_MASK;
	avr_cpu->cycles += 2;
	break;

      case OP_eijmp:
	avr_cpu->pc = ((sram[EIND] << 16) | read_word (REGZ)) & PC_MASK;
	avr_cpu->cycles += 2;
	break;

      case OP_ijmp:
	avr_cpu->pc = read_word (REGZ) & PC_MASK;
	avr_cpu->cycles += 1;
	break;

      case OP_call:
	/* 2 words instruction.  */
	avr_cpu->pc++;
	do_call (cpu, (flash[ipc].r << 16) | flash[ipc + 1].op);
	break;

      case OP_eicall:
	do_call (cpu, (sram[EIND] << 16) | read_word (REGZ));
	break;

      case OP_icall:
	do_call (cpu, read_word (REGZ));
	break;

      case OP_rcall:
	do_call (cpu, avr_cpu->pc + sign_ext (op & 0xfff, 12));
	break;

      case OP_reti:
	sram[SREG] |= SREG_I;
	ATTRIBUTE_FALLTHROUGH;
      case OP_ret:
	{
	  const struct avr_sim_state *state = AVR_SIM_STATE (CPU_STATE (cpu));
	  unsigned int sp = read_word (REG_SP);
	  if (state->avr_pc22)
	    {
	      avr_cpu->pc = sram[++sp] << 16;
	      avr_cpu->cycles++;
	    }
	  else
	    avr_cpu->pc = 0;
	  avr_cpu->pc |= sram[++sp] << 8;
	  avr_cpu->pc |= sram[++sp];
	  write_word (REG_SP, sp);
	}
	avr_cpu->cycles += 3;
	break;

      case OP_break:
	/* Stop on this address.  */
	sim_engine_halt (CPU_STATE (cpu), cpu, NULL, ipc, sim_stopped, SIM_SIGTRAP);
	break;

      case OP_bld:
	d = get_d (op);
	r = flash[ipc].r;
	if (sram[SREG] & SREG_T)
	  sram[d] |= r;
	else
	  sram[d] &= ~r;
	break;

      case OP_bst:
	if (sram[get_d (op)] & flash[ipc].r)
	  sram[SREG] |= SREG_T;
	else
	  sram[SREG] &= ~SREG_T;
	break;

      case OP_sbrc:
      case OP_sbrs:
	if (((sram[get_d (op)] & flash[ipc].r) == 0) ^ ((op & 0x0200) != 0))
	  {
	    int l = get_insn_length (avr_cpu->pc);
	    avr_cpu->pc += l;
	    avr_cpu->cycles += l;
	  }
	break;

      case OP_push:
	{
	  unsigned int sp = read_word (REG_SP);
	  sram[sp--] = sram[get_d (op)];
	  write_word (REG_SP, sp);
	}
	avr_cpu->cycles++;
	break;

      case OP_pop:
	{
	  unsigned int sp = read_word (REG_SP);
	  sram[get_d (op)] = sram[++sp];
	  write_word (REG_SP, sp);
	}
	avr_cpu->cycles++;
	break;

      case OP_bclr:
	sram[SREG] &= ~(1 << ((op >> 4) & 0x7));
	break;

      case OP_bset:
	sram[SREG] |= 1 << ((op >> 4) & 0x7);
	break;

      case OP_rjmp:
	avr_cpu->pc = (avr_cpu->pc + sign_ext (op & 0xfff, 12)) & PC_MASK;
	avr_cpu->cycles++;
	break;

      case OP_eor:
	d = get_d (op);
	res = sram[d] ^ sram[get_r (op)];
	sram[d] = res;
	update_flags_logic (res);
	break;

      case OP_and:
	d = get_d (op);
	res = sram[d] & sram[get_r (op)];
	sram[d] = res;
	update_flags_logic (res);
	break;

      case OP_andi:
	d = get_d16 (op);
	res = sram[d] & get_K (op);
	sram[d] = res;
	update_flags_logic (res);
	break;

      case OP_or:
	d = get_d (op);
	res = sram[d] | sram[get_r (op)];
	sram[d] = res;
	update_flags_logic (res);
	break;

      case OP_ori:
	d = get_d16 (op);
	res = sram[d] | get_K (op);
	sram[d] = res;
	update_flags_logic (res);
	break;

      case OP_com:
	d = get_d (op);
	res = ~sram[d];
	sram[d] = res;
	update_flags_logic (res);
	sram[SREG] |= SREG_C;
	break;

      case OP_swap:
	d = get_d (op);
	vd = sram[d];
	sram[d] = (vd >> 4) | (vd << 4);
	break;

      case OP_neg:
	d = get_d (op);
	vd = sram[d];
	res = -vd;
	sram[d] = res;
	sram[SREG] &= ~(SREG_H | SREG_S | SREG_V | SREG_N | SREG_Z | SREG_C);
	if (res == 0)
	  sram[SREG] |= SREG_Z;
	else
	  sram[SREG] |= SREG_C;
	if (res == 0x80)
	  sram[SREG] |= SREG_V | SREG_N;
	else if (res & 0x80)
	  sram[SREG] |= SREG_N | SREG_S;
	if ((res | vd) & 0x08)
	  sram[SREG] |= SREG_H;
	break;

      case OP_inc:
	d = get_d (op);
	res = sram[d] + 1;
	sram[d] = res;
	sram[SREG] &= ~(SREG_S | SREG_V | SREG_N | SREG_Z);
	if (res == 0x80)
	  sram[SREG] |= SREG_V | SREG_N;
	else if (res & 0x80)
	  sram[SREG] |= SREG_N | SREG_S;
	else if (res == 0)
	  sram[SREG] |= SREG_Z;
	break;

      case OP_dec:
	d = get_d (op);
	res = sram[d] - 1;
	sram[d] = res;
	sram[SREG] &= ~(SREG_S | SREG_V | SREG_N | SREG_Z);
	if (res == 0x7f)
	  sram[SREG] |= SREG_V | SREG_S;
	else if (res & 0x80)
	  sram[SREG] |= SREG_N | SREG_S;
	else if (res == 0)
	  sram[SREG] |= SREG_Z;
	break;

      case OP_lsr:
      case OP_asr:
	d = get_d (op);
	vd = sram[d];
	res = (vd >> 1) | (vd & flash[ipc].r);
	sram[d] = res;
	sram[SREG] &= ~(SREG_S | SREG_V | SREG_N | SREG_Z | SREG_C);
	if (vd & 1)
	  sram[SREG] |= SREG_C | SREG_S;
	if (res & 0x80)
	  sram[SREG] |= SREG_N;
	if (!(sram[SREG] & SREG_N) ^ !(sram[SREG] & SREG_C))
	  sram[SREG] |= SREG_V;
	if (res == 0)
	  sram[SREG] |= SREG_Z;
	break;

      case OP_ror:
	d = get_d (op);
	vd = sram[d];
	res = vd >> 1 | (sram[SREG] << 7);
	sram[d] = res;
	sram[SREG] &= ~(SREG_S | SREG_V | SREG_N | SREG_Z | SREG_C);
	if (vd & 1)
	  sram[SREG] |= SREG_C | SREG_S;
	if (res & 0x80)
	  sram[SREG] |= SREG_N;
	if (!(sram[SREG] & SREG_N) ^ !(sram[SREG] & SREG_C))
	  sram[SREG] |= SREG_V;
	if (res == 0)
	  sram[SREG] |= SREG_Z;
	break;

      case OP_mul:
	gen_mul (cpu, (word)sram[get_r (op)] * (word)sram[get_d (op)]);
	break;

      case OP_muls:
	gen_mul (cpu, (sword)(sbyte)sram[get_r16 (op)]
		      * (sword)(sbyte)sram[get_d16 (op)]);
	break;

      case OP_mulsu:
	gen_mul (cpu, (sword)(word)sram[get_r16_23 (op)]
		      * (sword)(sbyte)sram[get_d16_23 (op)]);
	break;

      case OP_fmul:
	gen_mul (cpu, ((word)sram[get_r16_23 (op)]
		       * (word)sram[get_d16_23 (op)]) << 1);
	break;

      case OP_fmuls:
	gen_mul (cpu, ((sword)(sbyte)sram[get_r16_23 (op)]
		       * (sword)(sbyte)sram[get_d16_23 (op)]) << 1);
	break;

      case OP_fmulsu:
	gen_mul (cpu, ((sword)(word)sram[get_r16_23 (op)]
		       * (sword)(sbyte)sram[get_d16_23 (op)]) << 1);
	break;

      case OP_adc:
      case OP_add:
	r = sram[get_r (op)];
	d = get_d (op);
	vd = sram[d];
	res = r + vd + (sram[SREG] & flash[ipc].r);
	sram[d] = res;
	update_flags_add (res, vd, r);
	break;

      case OP_sub:
	d = get_d (op);
	vd = sram[d];
	r = sram[get_r (op)];
	res = vd - r;
	sram[d] = res;
	update_flags_sub (res, vd, r);
	if (res == 0)
	  sram[SREG] |= SREG_Z;
	break;

      case OP_sbc:
	{
	  byte old = sram[SREG];
	  d = get_d (op);
	  vd = sram[d];
	  r = sram[get_r (op)];
	  res = vd - r - (old & SREG_C);
	  sram[d] = res;
	  update_flags_sub (res, vd, r);
	  if (res == 0 && (old & SREG_Z))
	    sram[SREG] |= SREG_Z;
	}
	break;

      case OP_subi:
	d = get_d16 (op);
	vd = sram[d];
	r = get_K (op);
	res = vd - r;
	sram[d] = res;
	update_flags_sub (res, vd, r);
	if (res == 0)
	  sram[SREG] |= SREG_Z;
	break;

      case OP_sbci:
	{
	  byte old = sram[SREG];

	  d = get_d16 (op);
	  vd = sram[d];
	  r = get_K (op);
	  res = vd - r - (old & SREG_C);
	  sram[d] = res;
	  update_flags_sub (res, vd, r);
	  if (res == 0 && (old & SREG_Z))
	    sram[SREG] |= SREG_Z;
	}
	break;

      case OP_mov:
	sram[get_d (op)] = sram[get_r (op)];
	break;

      case OP_movw:
	d = (op & 0xf0) >> 3;
	r = (op & 0x0f) << 1;
	sram[d] = sram[r];
	sram[d + 1] = sram[r + 1];
	break;

      case OP_out:
	d = get_A (op) + 0x20;
	res = sram[get_d (op)];
	sram[d] = res;
	if (d == STDIO_PORT)
	  putchar (res);
	else if (d == EXIT_PORT)
	  sim_engine_halt (CPU_STATE (cpu), cpu, NULL, avr_cpu->pc, sim_exited, 0);
	else if (d == ABORT_PORT)
	  sim_engine_halt (CPU_STATE (cpu), cpu, NULL, avr_cpu->pc, sim_exited, 1);
	break;

      case OP_in:
	d = get_A (op) + 0x20;
	sram[get_d (op)] = sram[d];
	break;

      case OP_cbi:
	d = get_biA (op) + 0x20;
	sram[d] &= ~(1 << get_b(op));
	break;

      case OP_sbi:
	d = get_biA (op) + 0x20;
	sram[d] |= 1 << get_b(op);
	break;

      case OP_sbic:
	if (!(sram[get_biA (op) + 0x20] & 1 << get_b(op)))
	  {
	    int l = get_insn_length (avr_cpu->pc);
	    avr_cpu->pc += l;
	    avr_cpu->cycles += l;
	  }
	break;

      case OP_sbis:
	if (sram[get_biA (op) + 0x20] & 1 << get_b(op))
	  {
	    int l = get_insn_length (avr_cpu->pc);
	    avr_cpu->pc += l;
	    avr_cpu->cycles += l;
	  }
	break;

      case OP_ldi:
	res = get_K (op);
	d = get_d16 (op);
	sram[d] = res;
	break;

      case OP_lds:
	sram[get_d (op)] = sram[flash[avr_cpu->pc].op];
	avr_cpu->pc++;
	avr_cpu->cycles++;
	break;

      case OP_sts:
	sram[flash[avr_cpu->pc].op] = sram[get_d (op)];
	avr_cpu->pc++;
	avr_cpu->cycles++;
	break;

      case OP_cpse:
	if (sram[get_r (op)] == sram[get_d (op)])
	  {
	    int l = get_insn_length (avr_cpu->pc);
	    avr_cpu->pc += l;
	    avr_cpu->cycles += l;
	  }
	break;

      case OP_cp:
	r = sram[get_r (op)];
	d = sram[get_d (op)];
	res = d - r;
	update_flags_sub (res, d, r);
	if (res == 0)
	  sram[SREG] |= SREG_Z;
	break;

      case OP_cpi:
	r = get_K (op);
	d = sram[get_d16 (op)];
	res = d - r;
	update_flags_sub (res, d, r);
	if (res == 0)
	  sram[SREG] |= SREG_Z;
	break;

      case OP_cpc:
	{
	  byte old = sram[SREG];
	  d = sram[get_d (op)];
	  r = sram[get_r (op)];
	  res = d - r - (old & SREG_C);
	  update_flags_sub (res, d, r);
	  if (res == 0 && (old & SREG_Z))
	    sram[SREG] |= SREG_Z;
	}
	break;

      case OP_brbc:
	if (!(sram[SREG] & flash[ipc].r))
	  {
	    avr_cpu->pc = (avr_cpu->pc + get_k (op)) & PC_MASK;
	    avr_cpu->cycles++;
	  }
	break;

      case OP_brbs:
	if (sram[SREG] & flash[ipc].r)
	  {
	    avr_cpu->pc = (avr_cpu->pc + get_k (op)) & PC_MASK;
	    avr_cpu->cycles++;
	  }
	break;

      case OP_lpm:
	sram[0] = get_lpm (read_word (REGZ));
	avr_cpu->cycles += 2;
	break;

      case OP_lpm_Z:
	sram[get_d (op)] = get_lpm (read_word (REGZ));
	avr_cpu->cycles += 2;
	break;

      case OP_lpm_inc_Z:
	sram[get_d (op)] = get_lpm (read_word_post_inc (REGZ));
	avr_cpu->cycles += 2;
	break;

      case OP_elpm:
	sram[0] = get_lpm (get_z ());
	avr_cpu->cycles += 2;
	break;

      case OP_elpm_Z:
	sram[get_d (op)] = get_lpm (get_z ());
	avr_cpu->cycles += 2;
	break;

      case OP_elpm_inc_Z:
	{
	  unsigned int z = get_z ();

	  sram[get_d (op)] = get_lpm (z);
	  z++;
	  sram[REGZ_LO] = z;
	  sram[REGZ_HI] = z >> 8;
	  sram[RAMPZ] = z >> 16;
	}
	avr_cpu->cycles += 2;
	break;

      case OP_ld_Z_inc:
	sram[get_d (op)] = sram[read_word_post_inc (REGZ) & SRAM_MASK];
	avr_cpu->cycles++;
	break;

      case OP_ld_dec_Z:
	sram[get_d (op)] = sram[read_word_pre_dec (REGZ) & SRAM_MASK];
	avr_cpu->cycles++;
	break;

      case OP_ld_X_inc:
	sram[get_d (op)] = sram[read_word_post_inc (REGX) & SRAM_MASK];
	avr_cpu->cycles++;
	break;

      case OP_ld_dec_X:
	sram[get_d (op)] = sram[read_word_pre_dec (REGX) & SRAM_MASK];
	avr_cpu->cycles++;
	break;

      case OP_ld_Y_inc:
	sram[get_d (op)] = sram[read_word_post_inc (REGY) & SRAM_MASK];
	avr_cpu->cycles++;
	break;

      case OP_ld_dec_Y:
	sram[get_d (op)] = sram[read_word_pre_dec (REGY) & SRAM_MASK];
	avr_cpu->cycles++;
	break;

      case OP_st_X:
	sram[read_word (REGX) & SRAM_MASK] = sram[get_d (op)];
	avr_cpu->cycles++;
	break;

      case OP_st_X_inc:
	sram[read_word_post_inc (REGX) & SRAM_MASK] = sram[get_d (op)];
	avr_cpu->cycles++;
	break;

      case OP_st_dec_X:
	sram[read_word_pre_dec (REGX) & SRAM_MASK] = sram[get_d (op)];
	avr_cpu->cycles++;
	break;

      case OP_st_Z_inc:
	sram[read_word_post_inc (REGZ) & SRAM_MASK] = sram[get_d (op)];
	avr_cpu->cycles++;
	break;

      case OP_st_dec_Z:
	sram[read_word_pre_dec (REGZ) & SRAM_MASK] = sram[get_d (op)];
	avr_cpu->cycles++;
	break;

      case OP_st_Y_inc:
	sram[read_word_post_inc (REGY) & SRAM_MASK] = sram[get_d (op)];
	avr_cpu->cycles++;
	break;

      case OP_st_dec_Y:
	sram[read_word_pre_dec (REGY) & SRAM_MASK] = sram[get_d (op)];
	avr_cpu->cycles++;
	break;

      case OP_std_Y:
	sram[read_word (REGY) + flash[ipc].r] = sram[get_d (op)];
	avr_cpu->cycles++;
	break;

      case OP_std_Z:
	sram[read_word (REGZ) + flash[ipc].r] = sram[get_d (op)];
	avr_cpu->cycles++;
	break;

      case OP_ldd_Z:
	sram[get_d (op)] = sram[read_word (REGZ) + flash[ipc].r];
	avr_cpu->cycles++;
	break;

      case OP_ldd_Y:
	sram[get_d (op)] = sram[read_word (REGY) + flash[ipc].r];
	avr_cpu->cycles++;
	break;

      case OP_ld_X:
	sram[get_d (op)] = sram[read_word (REGX) & SRAM_MASK];
	avr_cpu->cycles++;
	break;

      case OP_sbiw:
	{
	  word wk = get_k6 (op);
	  word wres;
	  word wr;

	  d = get_d24 (op);
	  wr = read_word (d);
	  wres = wr - wk;

	  sram[SREG] &= ~(SREG_S | SREG_V | SREG_N | SREG_Z | SREG_C);
	  if (wres == 0)
	    sram[SREG] |= SREG_Z;
	  if (wres & 0x8000)
	    sram[SREG] |= SREG_N;
	  if (wres & ~wr & 0x8000)
	    sram[SREG] |= SREG_C;
	  if (~wres & wr & 0x8000)
	    sram[SREG] |= SREG_V;
	  if (((~wres & wr) ^ wres) & 0x8000)
	    sram[SREG] |= SREG_S;
	  write_word (d, wres);
	}
	avr_cpu->cycles++;
	break;

      case OP_adiw:
	{
	  word wk = get_k6 (op);
	  word wres;
	  word wr;

	  d = get_d24 (op);
	  wr = read_word (d);
	  wres = wr + wk;

	  sram[SREG] &= ~(SREG_S | SREG_V | SREG_N | SREG_Z | SREG_C);
	  if (wres == 0)
	    sram[SREG] |= SREG_Z;
	  if (wres & 0x8000)
	    sram[SREG] |= SREG_N;
	  if (~wres & wr & 0x8000)
	    sram[SREG] |= SREG_C;
	  if (wres & ~wr & 0x8000)
	    sram[SREG] |= SREG_V;
	  if (((wres & ~wr) ^ wres) & 0x8000)
	    sram[SREG] |= SREG_S;
	  write_word (d, wres);
	}
	avr_cpu->cycles++;
	break;

      case OP_bad:
	sim_engine_halt (CPU_STATE (cpu), cpu, NULL, avr_cpu->pc, sim_signalled, SIM_SIGILL);

      default:
	sim_engine_halt (CPU_STATE (cpu), cpu, NULL, avr_cpu->pc, sim_signalled, SIM_SIGILL);
      }
}

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr, /* ignore  */
		int nr_cpus, /* ignore  */
		int siggnal) /* ignore  */
{
  SIM_CPU *cpu;

  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  cpu = STATE_CPU (sd, 0);

  while (1)
    {
      step_once (cpu);
      if (sim_events_tick (sd))
	sim_events_process (sd);
    }
}

uint64_t
sim_write (SIM_DESC sd, uint64_t addr, const void *buffer, uint64_t size)
{
  int osize = size;

  if (addr >= 0 && addr < SRAM_VADDR)
    {
      const unsigned char *data = buffer;
      while (size > 0 && addr < (MAX_AVR_FLASH << 1))
	{
          word val = flash[addr >> 1].op;

          if (addr & 1)
            val = (val & 0xff) | (data[0] << 8);
          else
            val = (val & 0xff00) | data[0];

	  flash[addr >> 1].op = val;
	  flash[addr >> 1].code = OP_unknown;
	  addr++;
	  data++;
	  size--;
	}
      return osize - size;
    }
  else if (addr >= SRAM_VADDR && addr < SRAM_VADDR + MAX_AVR_SRAM)
    {
      addr -= SRAM_VADDR;
      if (addr + size > MAX_AVR_SRAM)
	size = MAX_AVR_SRAM - addr;
      memcpy (sram + addr, buffer, size);
      return size;
    }
  else
    return 0;
}

uint64_t
sim_read (SIM_DESC sd, uint64_t addr, void *buffer, uint64_t size)
{
  int osize = size;

  if (addr >= 0 && addr < SRAM_VADDR)
    {
      unsigned char *data = buffer;
      while (size > 0 && addr < (MAX_AVR_FLASH << 1))
	{
          word val = flash[addr >> 1].op;

          if (addr & 1)
            val >>= 8;

          *data++ = val;
	  addr++;
	  size--;
	}
      return osize - size;
    }
  else if (addr >= SRAM_VADDR && addr < SRAM_VADDR + MAX_AVR_SRAM)
    {
      addr -= SRAM_VADDR;
      if (addr + size > MAX_AVR_SRAM)
	size = MAX_AVR_SRAM - addr;
      memcpy (buffer, sram + addr, size);
      return size;
    }
  else
    {
      /* Avoid errors.  */
      memset (buffer, 0, size);
      return size;
    }
}

static int
avr_reg_store (SIM_CPU *cpu, int rn, const void *buf, int length)
{
  struct avr_sim_cpu *avr_cpu = AVR_SIM_CPU (cpu);
  const unsigned char *memory = buf;

  if (rn < 32 && length == 1)
    {
      sram[rn] = *memory;
      return 1;
    }
  if (rn == AVR_SREG_REGNUM && length == 1)
    {
      sram[SREG] = *memory;
      return 1;
    }
  if (rn == AVR_SP_REGNUM && length == 2)
    {
      sram[REG_SP] = memory[0];
      sram[REG_SP + 1] = memory[1];
      return 2;
    }
  if (rn == AVR_PC_REGNUM && length == 4)
    {
      avr_cpu->pc = (memory[0] >> 1) | (memory[1] << 7)
		| (memory[2] << 15) | (memory[3] << 23);
      avr_cpu->pc &= PC_MASK;
      return 4;
    }
  return 0;
}

static int
avr_reg_fetch (SIM_CPU *cpu, int rn, void *buf, int length)
{
  struct avr_sim_cpu *avr_cpu = AVR_SIM_CPU (cpu);
  unsigned char *memory = buf;

  if (rn < 32 && length == 1)
    {
      *memory = sram[rn];
      return 1;
    }
  if (rn == AVR_SREG_REGNUM && length == 1)
    {
      *memory = sram[SREG];
      return 1;
    }
  if (rn == AVR_SP_REGNUM && length == 2)
    {
      memory[0] = sram[REG_SP];
      memory[1] = sram[REG_SP + 1];
      return 2;
    }
  if (rn == AVR_PC_REGNUM && length == 4)
    {
      memory[0] = avr_cpu->pc << 1;
      memory[1] = avr_cpu->pc >> 7;
      memory[2] = avr_cpu->pc >> 15;
      memory[3] = avr_cpu->pc >> 23;
      return 4;
    }
  return 0;
}

static sim_cia
avr_pc_get (sim_cpu *cpu)
{
  return AVR_SIM_CPU (cpu)->pc;
}

static void
avr_pc_set (sim_cpu *cpu, sim_cia pc)
{
  AVR_SIM_CPU (cpu)->pc = pc;
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
  SIM_DESC sd = sim_state_alloc_extra (kind, cb, sizeof (struct avr_sim_state));
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  /* Set default options before parsing user options.  */
  current_alignment = STRICT_ALIGNMENT;
  current_target_byte_order = BFD_ENDIAN_LITTLE;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct avr_sim_cpu))
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

      CPU_REG_FETCH (cpu) = avr_reg_fetch;
      CPU_REG_STORE (cpu) = avr_reg_store;
      CPU_PC_FETCH (cpu) = avr_pc_get;
      CPU_PC_STORE (cpu) = avr_pc_set;
    }

  /* Clear all the memory.  */
  memset (sram, 0, sizeof (sram));
  memset (flash, 0, sizeof (flash));

  return sd;
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *abfd,
		     char * const *argv, char * const *env)
{
  struct avr_sim_state *state = AVR_SIM_STATE (sd);
  SIM_CPU *cpu = STATE_CPU (sd, 0);
  bfd_vma addr;

  /* Set the PC.  */
  if (abfd != NULL)
    addr = bfd_get_start_address (abfd);
  else
    addr = 0;
  sim_pc_set (cpu, addr);

  if (abfd != NULL)
    state->avr_pc22 = (bfd_get_mach (abfd) >= bfd_mach_avr6);

  return SIM_RC_OK;
}
