/*  thumbemu.c -- Thumb instruction emulation.
    Copyright (C) 1996, Cygnus Software Technologies Ltd.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, see <http://www.gnu.org/licenses/>. */

/* We can provide simple Thumb simulation by decoding the Thumb
instruction into its corresponding ARM instruction, and using the
existing ARM simulator.  */

/* This must come before any other includes.  */
#include "defs.h"

#ifndef MODET			/* required for the Thumb instruction support */
#if 1
#error "MODET needs to be defined for the Thumb world to work"
#else
#define MODET (1)
#endif
#endif

#include "armdefs.h"
#include "armemu.h"
#include "armos.h"

#define tBIT(n)    ( (ARMword)(tinstr >> (n)) & 1)
#define tBITS(m,n) ( (ARMword)(tinstr << (31 - (n))) >> ((31 - (n)) + (m)) )

#define ntBIT(n)    ( (ARMword)(next_instr >> (n)) & 1)
#define ntBITS(m,n) ( (ARMword)(next_instr << (31 - (n))) >> ((31 - (n)) + (m)) )

static int
test_cond (int cond, ARMul_State * state)
{
  switch (cond)
    {
    case EQ: return ZFLAG;
    case NE: return !ZFLAG;
    case VS: return VFLAG;
    case VC: return !VFLAG;
    case MI: return NFLAG;
    case PL: return !NFLAG;
    case CS: return CFLAG;
    case CC: return !CFLAG;
    case HI: return (CFLAG && !ZFLAG);
    case LS: return (!CFLAG || ZFLAG);
    case GE: return ((!NFLAG && !VFLAG) || (NFLAG && VFLAG));
    case LT: return ((NFLAG && !VFLAG) || (!NFLAG && VFLAG));
    case GT: return ((!NFLAG && !VFLAG && !ZFLAG)
		     || (NFLAG && VFLAG && !ZFLAG));
    case LE: return ((NFLAG && !VFLAG) || (!NFLAG && VFLAG)) || ZFLAG;
    case AL: return TRUE;
    case NV:
    default: return FALSE;
    }
}

static ARMword skipping_32bit_thumb = 0;

static int     IT_block_cond = AL;
static ARMword IT_block_mask = 0;
static int     IT_block_first = FALSE;

static void
handle_IT_block (ARMul_State * state,
		 ARMword       tinstr,
		 tdstate *     pvalid)
{
  * pvalid = t_branch;
  IT_block_mask = tBITS (0, 3);

  if (IT_block_mask == 0)
    // NOP or a HINT.
    return;

  IT_block_cond = tBITS (4, 7);
  IT_block_first = TRUE;
}

static int
in_IT_block (void)
{
  return IT_block_mask != 0;
}

static int
IT_block_allow (ARMul_State * state)
{
  int cond;

  if (IT_block_mask == 0)
    return TRUE;

  cond = IT_block_cond;

  if (IT_block_first)
    IT_block_first = FALSE;
  else
    {
      if ((IT_block_mask & 8) == 0)
	cond &= 0xe;
      else
	cond |= 1;
      IT_block_mask <<= 1;
      IT_block_mask &= 0xF;
    }

  if (IT_block_mask == 0x8)
    IT_block_mask = 0;

  return test_cond (cond, state);
}

static ARMword
ThumbExpandImm (ARMword tinstr)
{
  ARMword val;

  if (tBITS (10, 11) == 0)
    {
      switch (tBITS (8, 9))
	{
	case 0:	 val = tBITS (0, 7); break;
	case 1:	 val = tBITS (0, 7) << 8; break;
	case 2:  val = (tBITS (0, 7) << 8) | (tBITS (0, 7) << 24); break;
	case 3:  val = tBITS (0, 7) * 0x01010101; break;
	default: val = 0;
	}
    }
  else
    {
      int ror = tBITS (7, 11);

      val = (1 << 7) | tBITS (0, 6);
      val = (val >> ror) | (val << (32 - ror));
    }

  return val;
}

#define tASSERT(truth)							\
  do									\
    {									\
      if (! (truth))							\
	{								\
	  fprintf (stderr, "unhandled T2 insn %04x|%04x detected at thumbemu.c:%d\n", \
		   tinstr, next_instr, __LINE__);			\
	  return ;							\
	}								\
    }									\
  while (0)


/* Attempt to emulate a 32-bit ARMv7 Thumb instruction.
   Stores t_branch into PVALUE upon success or t_undefined otherwise.  */

static void
handle_T2_insn (ARMul_State * state,
		ARMword       tinstr,
		ARMword       next_instr,
		ARMword       pc,
		ARMword *     ainstr,
		tdstate *     pvalid)
{
  * pvalid = t_undefined;

  if (! state->is_v6)
    return;

  if (trace)
    fprintf (stderr, "|%04x ", next_instr);

  if (tBITS (11, 15) == 0x1E && ntBIT (15) == 1)
    {
      ARMsword simm32 = 0;
      int S = tBIT (10);

      * pvalid = t_branch;
      switch ((ntBIT (14) << 1) | ntBIT (12))
	{
	case 0: /* B<c>.W  */
	  {
	    ARMword cond = tBITS (6, 9);
	    ARMword imm6;
	    ARMword imm11;
	    ARMword J1;
	    ARMword J2;

	    tASSERT (cond != AL && cond != NV);
	    if (! test_cond (cond, state))
	      return;

	    imm6 = tBITS (0, 5);
	    imm11 = ntBITS (0, 10);
	    J1 = ntBIT (13);
	    J2 = ntBIT (11);

	    simm32 = (J1 << 19) | (J2 << 18) | (imm6 << 12) | (imm11 << 1);
	    if (S)
	      simm32 |= -(1 << 20);
	    break;
	  }

	case 1: /* B.W  */
	  {
	    ARMword imm10 = tBITS (0, 9);
	    ARMword imm11 = ntBITS (0, 10);
	    ARMword I1 = (ntBIT (13) ^ S) ? 0 : 1;
	    ARMword I2 = (ntBIT (11) ^ S) ? 0 : 1;

	    simm32 = (I1 << 23) | (I2 << 22) | (imm10 << 12) | (imm11 << 1);
	    if (S)
	      simm32 |= -(1 << 24);
	    break;
	  }

	case 2: /* BLX <label>  */
	  {
	    ARMword imm10h = tBITS (0, 9);
	    ARMword imm10l = ntBITS (1, 10);
	    ARMword I1 = (ntBIT (13) ^ S) ? 0 : 1;
	    ARMword I2 = (ntBIT (11) ^ S) ? 0 : 1;

	    simm32 = (I1 << 23) | (I2 << 22) | (imm10h << 12) | (imm10l << 2);
	    if (S)
	      simm32 |= -(1 << 24);

	    CLEART;
	    state->Reg[14] = (pc + 4) | 1;
	    break;
	  }

	case 3: /* BL <label>  */
	  {
	    ARMword imm10 = tBITS (0, 9);
	    ARMword imm11 = ntBITS (0, 10);
	    ARMword I1 = (ntBIT (13) ^ S) ? 0 : 1;
	    ARMword I2 = (ntBIT (11) ^ S) ? 0 : 1;

	    simm32 = (I1 << 23) | (I2 << 22) | (imm10 << 12) | (imm11 << 1);
	    if (S)
	      simm32 |= -(1 << 24);
	    state->Reg[14] = (pc + 4) | 1;
	    break;
	  }
	}

      state->Reg[15] = (pc + 4 + simm32);
      FLUSHPIPE;
      if (trace_funcs)
	fprintf (stderr, " pc changed to %x\n", state->Reg[15]);
      return;
    }

  switch (tBITS (5,12))
    {
    case 0x29:  // TST<c>.W <Rn>,<Rm>{,<shift>}
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rm = ntBITS (0, 3);
	ARMword type = ntBITS (4, 5);
	ARMword imm5 = (ntBITS (12, 14) << 2) | ntBITS (6, 7);

	tASSERT (ntBITS (8, 11) == 0xF);

	* ainstr = 0xE1100000;
	* ainstr |= (Rn << 16);
	* ainstr |= (Rm);
	* ainstr |= (type << 5);
	* ainstr |= (imm5 << 7);
	* pvalid = t_decoded;
	break;
      }

    case 0x46:
      if (tBIT (4) && ntBITS (5, 15) == 0x780)
	{
	  // Table Branch
	  ARMword Rn = tBITS (0, 3);
	  ARMword Rm = ntBITS (0, 3);
	  ARMword address, dest;

	  if (ntBIT (4))
	    {
	      // TBH
	      address = state->Reg[Rn] + state->Reg[Rm] * 2;
	      dest = ARMul_LoadHalfWord (state, address);
	    }
	  else
	    {
	      // TBB
	      address = state->Reg[Rn] + state->Reg[Rm];
	      dest = ARMul_LoadByte (state, address);
	    }

	  state->Reg[15] = (pc + 4 + dest * 2);
	  FLUSHPIPE;
	  * pvalid = t_branch;
	  break;
	}
      ATTRIBUTE_FALLTHROUGH;
    case 0x42:
    case 0x43:
    case 0x47:
    case 0x4A:
    case 0x4B:
    case 0x4E: // STRD
    case 0x4F: // LDRD
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rt = ntBITS (12, 15);
	ARMword Rt2 = ntBITS (8, 11);
	ARMword imm8 = ntBITS (0, 7);
	ARMword P = tBIT (8);
	ARMword U = tBIT (7);
	ARMword W = tBIT (5);

	tASSERT (Rt2 == Rt + 1);
	imm8 <<= 2;
	tASSERT (imm8 <= 255);
	tASSERT (P != 0 || W != 0);

	// Convert into an ARM A1 encoding.
	if (Rn == 15)
	  {
	    tASSERT (tBIT (4) == 1);
	    // LDRD (literal)
	    // Ignore W even if 1.
	    * ainstr = 0xE14F00D0;
	  }
	else
	  {
	    if (tBIT (4) == 1)
	      // LDRD (immediate)
	      * ainstr = 0xE04000D0;
	    else
	      {
		// STRD<c> <Rt>,<Rt2>,[<Rn>{,#+/-<imm8>}]
		// STRD<c> <Rt>,<Rt2>,[<Rn>],#+/-<imm8>
		// STRD<c> <Rt>,<Rt2>,[<Rn>,#+/-<imm8>]!
		* ainstr = 0xE04000F0;
	      }
	    * ainstr |= (Rn << 16);
	    * ainstr |= (P << 24);
	    * ainstr |= (W << 21);
	  }

	* ainstr |= (U << 23);
	* ainstr |= (Rt << 12);
	* ainstr |= ((imm8 << 4) & 0xF00);
	* ainstr |= (imm8 & 0xF);
	* pvalid = t_decoded;
	break;
      }

    case 0x44:
    case 0x45: // LDMIA
      {
	ARMword Rn   = tBITS (0, 3);
	int     W    = tBIT (5);
	ARMword list = (ntBIT (15) << 15) | (ntBIT (14) << 14) | ntBITS (0, 12);

	if (Rn == 13)
	  * ainstr = 0xE8BD0000;
	else
	  {
	    * ainstr = 0xE8900000;
	    * ainstr |= (W << 21);
	    * ainstr |= (Rn << 16);
	  }
	* ainstr |= list;
	* pvalid = t_decoded;
	break;
      }

    case 0x48:
    case 0x49: // STMDB
      {
	ARMword Rn   = tBITS (0, 3);
	int     W    = tBIT (5);
	ARMword list = (ntBIT (14) << 14) | ntBITS (0, 12);

	if (Rn == 13 && W)
	  * ainstr = 0xE92D0000;
	else
	  {
	    * ainstr = 0xE9000000;
	    * ainstr |= (W << 21);
	    * ainstr |= (Rn << 16);
	  }
	* ainstr |= list;
	* pvalid = t_decoded;
	break;
      }

    case 0x50:
      {
	ARMword Rd = ntBITS (8, 11);
	ARMword Rn = tBITS (0, 3);
	ARMword Rm = ntBITS (0, 3);
	ARMword imm5 = (ntBITS (12, 14) << 2) | ntBITS (6, 7);
	ARMword type = ntBITS (4, 5);

	tASSERT (ntBIT (15) == 0);

	if (Rd == 15)
	  {
	    tASSERT (tBIT (4) == 1);

	    // TST<c>.W <Rn>,<Rm>{,<shift>}
	    * ainstr = 0xE1100000;
	  }
	else
	  {
	    // AND{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
	    int S = tBIT (4);

	    * ainstr = 0xE0000000;

	    if (in_IT_block ())
	      S = 0;
	    * ainstr |= (S << 20);
	  }
	
	* ainstr |= (Rn << 16);
	* ainstr |= (imm5 << 7);
	* ainstr |= (type << 5);
	* ainstr |= (Rm << 0);
	* pvalid = t_decoded;
	break;
      }

    case 0x51: // BIC{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
      {
	ARMword Rn = tBITS (0, 3);
	ARMword S  = tBIT(4);
	ARMword Rm = ntBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);
	ARMword imm5 = (ntBITS (12, 14) << 2) | ntBITS (6, 7);
	ARMword type = ntBITS (4, 5);
	
	tASSERT (ntBIT (15) == 0);

	* ainstr = 0xE1C00000;
	* ainstr |= (S << 20);
	* ainstr |= (Rn << 16);
	* ainstr |= (Rd << 12);
	* ainstr |= (imm5 << 7);
	* ainstr |= (type << 5);
	* ainstr |= (Rm << 0);
	* pvalid = t_decoded;
	break;
      }

    case 0x52:
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);
	ARMword Rm = ntBITS (0, 3);
	int S = tBIT (4);
	ARMword imm5 = (ntBITS (12, 14) << 2) | ntBITS (6, 7);
	ARMword type = ntBITS (4, 5);

	tASSERT (Rd != 15);

	if (in_IT_block ())
	  S = 0;

	if (Rn == 15)
	  {
	    tASSERT (ntBIT (15) == 0);

	    switch (ntBITS (4, 5))
	      {
	      case 0:
		// LSL{S}<c>.W <Rd>,<Rm>,#<imm5>
		* ainstr = 0xE1A00000;
		break;
	      case 1:
		// LSR{S}<c>.W <Rd>,<Rm>,#<imm>
		* ainstr = 0xE1A00020;
		break;
	      case 2:
		// ASR{S}<c>.W <Rd>,<Rm>,#<imm>
		* ainstr = 0xE1A00040;
		break;
	      case 3:
		// ROR{S}<c> <Rd>,<Rm>,#<imm>
		* ainstr = 0xE1A00060;
		break;
	      default:
		tASSERT (0);
		* ainstr = 0;
	      }
	  }
	else
	  {
	    // ORR{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
	    * ainstr = 0xE1800000;
	    * ainstr |= (Rn << 16);
	    * ainstr |= (type << 5);
	  }
	
	* ainstr |= (Rd << 12);
	* ainstr |= (S << 20);
	* ainstr |= (imm5 << 7);
	* ainstr |= (Rm <<  0);
	* pvalid = t_decoded;
	break;
      }

    case 0x53: // MVN{S}<c>.W <Rd>,<Rm>{,<shift>}
      {
	ARMword Rd = ntBITS (8, 11);
	ARMword Rm = ntBITS (0, 3);
	int S = tBIT (4);
	ARMword imm5 = (ntBITS (12, 14) << 2) | ntBITS (6, 7);
	ARMword type = ntBITS (4, 5);

	tASSERT (ntBIT (15) == 0);

	if (in_IT_block ())
	  S = 0;

	* ainstr = 0xE1E00000;
	* ainstr |= (S << 20);
	* ainstr |= (Rd << 12);
	* ainstr |= (imm5 << 7);
	* ainstr |= (type << 5);
	* ainstr |= (Rm <<  0);
	* pvalid = t_decoded;
	break;
      }

    case 0x54:
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);
	ARMword Rm = ntBITS (0, 3);
	int S = tBIT (4);
	ARMword imm5 = (ntBITS (12, 14) << 2) | ntBITS (6, 7);
	ARMword type = ntBITS (4, 5);

	if (Rd == 15 && S)
	  {
	    // TEQ<c> <Rn>,<Rm>{,<shift>}
	    tASSERT (ntBIT (15) == 0);

	    * ainstr = 0xE1300000;
	  }
	else
	  {
	    // EOR{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
	    if (in_IT_block ())
	      S = 0;

	    * ainstr = 0xE0200000;
	    * ainstr |= (S << 20);
	    * ainstr |= (Rd << 8);
	  }

	* ainstr |= (Rn <<  16);
	* ainstr |= (imm5 << 7);
	* ainstr |= (type << 5);
	* ainstr |= (Rm <<  0);
	* pvalid = t_decoded;
	break;
      }

    case 0x58: // ADD{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);
	ARMword Rm = ntBITS (0, 3);
	int S = tBIT (4);
	ARMword imm5 = (ntBITS (12, 14) << 2) | ntBITS (6, 7);
	ARMword type = ntBITS (4, 5);
	
	tASSERT (! (Rd == 15 && S));

	if (in_IT_block ())
	  S = 0;

	* ainstr = 0xE0800000;
	* ainstr |= (S << 20);
	* ainstr |= (Rn << 16);
	* ainstr |= (Rd << 12);
	* ainstr |= (imm5 << 7);
	* ainstr |= (type << 5);
	* ainstr |= Rm;
	* pvalid = t_decoded;
	break;
      }

    case 0x5A: // ADC{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
      tASSERT (ntBIT (15) == 0);
      * ainstr = 0xE0A00000;
      if (! in_IT_block ())
	* ainstr |= (tBIT (4) << 20); // S
      * ainstr |= (tBITS (0, 3) << 16); // Rn
      * ainstr |= (ntBITS (8, 11) << 12); // Rd
      * ainstr |= ((ntBITS (12, 14) << 2) | ntBITS (6, 7)) << 7; // imm5
      * ainstr |= (ntBITS (4, 5) << 5); // type
      * ainstr |= ntBITS (0, 3); // Rm
      * pvalid = t_decoded;
      break;

    case 0x5B: // SBC{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);
	ARMword Rm = ntBITS (0, 3);
	int S = tBIT (4);
	ARMword imm5 = (ntBITS (12, 14) << 2) | ntBITS (6, 7);
	ARMword type = ntBITS (4, 5);
	
	tASSERT (ntBIT (15) == 0);

	if (in_IT_block ())
	  S = 0;

	* ainstr = 0xE0C00000;
	* ainstr |= (S << 20);
	* ainstr |= (Rn << 16);
	* ainstr |= (Rd << 12);
	* ainstr |= (imm5 << 7);
	* ainstr |= (type << 5);
	* ainstr |= Rm;
	* pvalid = t_decoded;
	break;
      }

    case 0x5E: // RSB{S}<c> <Rd>,<Rn>,<Rm>{,<shift>}
    case 0x5D: // SUB{S}<c>.W <Rd>,<Rn>,<Rm>{,<shift>}
      {
	ARMword Rn   = tBITS (0, 3);
	ARMword Rd   = ntBITS (8, 11);
	ARMword Rm   = ntBITS (0, 3);
	ARMword S    = tBIT (4);
	ARMword type = ntBITS (4, 5);
	ARMword imm5 = (ntBITS (12, 14) << 2) | ntBITS (6, 7);

	tASSERT (ntBIT(15) == 0);
	
	if (Rd == 15)
	  {
	    // CMP<c>.W <Rn>, <Rm> {,<shift>}
	    * ainstr = 0xE1500000;
	    Rd = 0;
	  }
	else if (tBIT (5))
	  * ainstr = 0xE0400000;
	else
	  * ainstr = 0xE0600000;

	* ainstr |= (S << 20);
	* ainstr |= (Rn << 16);
	* ainstr |= (Rd << 12);
	* ainstr |= (imm5 << 7);
	* ainstr |= (type << 5);
	* ainstr |= (Rm <<  0);
	* pvalid = t_decoded;
	break;
      }

    case 0x9D: // NOP.W
      tASSERT (tBITS (0, 15) == 0xF3AF);
      tASSERT (ntBITS (0, 15) == 0x8000);
      * pvalid = t_branch;
      break;

    case 0x80: // AND
    case 0xA0: // TST
      {
	ARMword Rn = tBITS (0, 3);
	ARMword imm12 = (tBIT(10) << 11) | (ntBITS (12, 14) << 8) | ntBITS (0, 7);
	ARMword Rd = ntBITS (8, 11);
	ARMword val;
	int S = tBIT (4);

	imm12 = ThumbExpandImm (imm12);	
	val = state->Reg[Rn] & imm12;

	if (Rd == 15)
	  {
	    // TST<c> <Rn>,#<const>
	    tASSERT (S == 1);
	  }
	else
	  {
	    // AND{S}<c> <Rd>,<Rn>,#<const>
	    if (in_IT_block ())
	      S = 0;

	    state->Reg[Rd] = val;
	  }

	if (S)
	  ARMul_NegZero (state, val);
	* pvalid = t_branch;
	break;
      }

    case 0xA1:
    case 0x81: // BIC.W
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);
	ARMword S = tBIT (4);
	ARMword imm8 = (ntBITS (12, 14) << 8) | ntBITS (0, 7);

	tASSERT (ntBIT (15) == 0);

	imm8 = ThumbExpandImm (imm8);
	state->Reg[Rd] = state->Reg[Rn] & ~ imm8;

	if (S && ! in_IT_block ())
	  ARMul_NegZero (state, state->Reg[Rd]);
	* pvalid = t_resolved;
	break;
      }

    case 0xA2:
    case 0x82: // MOV{S}<c>.W <Rd>,#<const>
      {
	ARMword val = (tBIT(10) << 11) | (ntBITS (12, 14) << 8) | ntBITS (0, 7);
	ARMword Rd = ntBITS (8, 11);

	val = ThumbExpandImm (val);
	state->Reg[Rd] = val;

	if (tBIT (4) && ! in_IT_block ())
	  ARMul_NegZero (state, val);
	/* Indicate that the instruction has been processed.  */
	* pvalid = t_branch;
	break;
      }

    case 0xA3:
    case 0x83: // MVN{S}<c> <Rd>,#<const>
      {
	ARMword val = (tBIT(10) << 11) | (ntBITS (12, 14) << 8) | ntBITS (0, 7);
	ARMword Rd = ntBITS (8, 11);

	val = ThumbExpandImm (val);
	val = ~ val;
	state->Reg[Rd] = val;

	if (tBIT (4) && ! in_IT_block ())
	  ARMul_NegZero (state, val);
	* pvalid = t_resolved;
	break;
      }

    case 0xA4: // EOR
    case 0x84: // TEQ
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);
	ARMword S = tBIT (4);
	ARMword imm12 = ((tBIT (10) << 11) | (ntBITS (12, 14) << 8) | ntBITS (0, 7));
	ARMword result;

	imm12 = ThumbExpandImm (imm12);

	result = state->Reg[Rn] ^ imm12;

	if (Rd == 15 && S)
	  // TEQ<c> <Rn>,#<const>
	  ;
	else
	  {
	    // EOR{S}<c> <Rd>,<Rn>,#<const>
	    state->Reg[Rd] = result;

	    if (in_IT_block ())
	      S = 0;
	  }

	if (S)
	  ARMul_NegZero (state, result);
	* pvalid = t_resolved;
	break;
      }

    case 0xA8: // CMN
    case 0x88: // ADD
      {
	ARMword Rd = ntBITS (8, 11);
	int S = tBIT (4);
	ARMword Rn = tBITS (0, 3);
	ARMword lhs = state->Reg[Rn];
	ARMword imm12 = (tBIT (10) << 11) | (ntBITS (12, 14) << 8) | ntBITS (0, 7);
	ARMword rhs = ThumbExpandImm (imm12);
	ARMword res = lhs + rhs;

	if (Rd == 15 && S)
	  {
	    // CMN<c> <Rn>,#<const>
	    res = lhs - rhs;
	  }
	else
	  {
	    // ADD{S}<c>.W <Rd>,<Rn>,#<const>
	    res = lhs + rhs;

	    if (in_IT_block ())
	      S = 0;

	    state->Reg[Rd] = res;
	  }

	if (S)
	  {
	    ARMul_NegZero (state, res);

	    if ((lhs | rhs) >> 30)
	      {
		/* Possible C,V,N to set.  */
		ARMul_AddCarry (state, lhs, rhs, res);
		ARMul_AddOverflow (state, lhs, rhs, res);
	      }
	    else
	      {
		CLEARC;
		CLEARV;
	      }
	  }
	
	* pvalid = t_branch;
	break;
      }

    case 0xAA:
    case 0x8A: // ADC{S}<c> <Rd>,<Rn>,#<const>
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);
	int S = tBIT (4);
	ARMword imm12 = (tBIT (10) << 11) | (ntBITS (12, 14) << 8) | ntBITS (0, 7);
	ARMword lhs = state->Reg[Rn];
	ARMword rhs = ThumbExpandImm (imm12);
	ARMword res;

	tASSERT (ntBIT (15) == 0);

	if (CFLAG)
	  rhs += 1;

	res = lhs + rhs;
	state->Reg[Rd] = res;

	if (in_IT_block ())
	  S = 0;

	if (S)
	  {
	    ARMul_NegZero (state, res);

	    if ((lhs >= rhs) || ((rhs | lhs) >> 31))
	      {
		ARMul_AddCarry (state, lhs, rhs, res);
		ARMul_AddOverflow (state, lhs, rhs, res);
	      }
	    else
	      {
		CLEARC;
		CLEARV;
	      }
	  }

	* pvalid = t_branch;
	break;
      }

    case 0xAB:
    case 0x8B: // SBC{S}<c> <Rd>,<Rn>,#<const>
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);
	int S = tBIT (4);
	ARMword imm12 = (tBIT (10) << 11) | (ntBITS (12, 14) << 8) | ntBITS (0, 7);
	ARMword lhs = state->Reg[Rn];
	ARMword rhs = ThumbExpandImm (imm12);
	ARMword res;

	tASSERT (ntBIT (15) == 0);

	if (! CFLAG)
	  rhs += 1;

	res = lhs - rhs;
	state->Reg[Rd] = res;

	if (in_IT_block ())
	  S = 0;

	if (S)
	  {
	    ARMul_NegZero (state, res);

	    if ((lhs >= rhs) || ((rhs | lhs) >> 31))
	      {
		ARMul_SubCarry (state, lhs, rhs, res);
		ARMul_SubOverflow (state, lhs, rhs, res);
	      }
	    else
	      {
		CLEARC;
		CLEARV;
	      }
	  }

	* pvalid = t_branch;
	break;
      }

    case 0xAD:
    case 0x8D: // SUB
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);
	int S = tBIT (4);
	ARMword imm12 = (tBIT (10) << 11) | (ntBITS (12, 14) << 8) | ntBITS (0, 7);
	ARMword lhs = state->Reg[Rn];
	ARMword rhs = ThumbExpandImm (imm12);
	ARMword res = lhs - rhs;

	if (Rd == 15 && S)
	  {
	    // CMP<c>.W <Rn>,#<const>
	    tASSERT (S);
	  }
	else
	  {
	    // SUB{S}<c>.W <Rd>,<Rn>,#<const>
	    if (in_IT_block ())
	      S = 0;

	    state->Reg[Rd] = res;
	  }

	if (S)
	  {
	    ARMul_NegZero (state, res);

	    if ((lhs >= rhs) || ((rhs | lhs) >> 31))
	      {
		ARMul_SubCarry (state, lhs, rhs, res);
		ARMul_SubOverflow (state, lhs, rhs, res);
	      }
	    else
	      {
		CLEARC;
		CLEARV;
	      }
	  }

	* pvalid = t_branch;
	break;
      }

    case 0xAE:
    case 0x8E: // RSB{S}<c>.W <Rd>,<Rn>,#<const>
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);
	ARMword imm12 = (tBIT (10) << 11) | (ntBITS (12, 14) << 8) | ntBITS (0, 7);
	int S = tBIT (4);
	ARMword lhs = imm12;
	ARMword rhs = state->Reg[Rn];
	ARMword res = lhs - rhs;

	tASSERT (ntBIT (15) == 0);
	
	state->Reg[Rd] = res;

	if (S)
	  {
	    ARMul_NegZero (state, res);

	    if ((lhs >= rhs) || ((rhs | lhs) >> 31))
	      {
		ARMul_SubCarry (state, lhs, rhs, res);
		ARMul_SubOverflow (state, lhs, rhs, res);
	      }
	    else
	      {
		CLEARC;
		CLEARV;
	      }
	  }

	* pvalid = t_branch;
	break;
      }

    case 0xB0:
    case 0x90: // ADDW<c> <Rd>,<Rn>,#<imm12>
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);
	ARMword imm12 = (tBIT (10) << 11) | (ntBITS (12, 14) << 8) | ntBITS (0, 7);

	tASSERT (tBIT (4) == 0);
	tASSERT (ntBIT (15) == 0);
	
	state->Reg[Rd] = state->Reg[Rn] + imm12;
	* pvalid = t_branch;
	break;
      }

    case 0xB2:
    case 0x92: // MOVW<c> <Rd>,#<imm16>
      {
	ARMword Rd = ntBITS (8, 11);
	ARMword imm = (tBITS (0, 3) << 12) | (tBIT (10) << 11) | (ntBITS (12, 14) << 8) | ntBITS (0, 7);

	state->Reg[Rd] = imm;
	/* Indicate that the instruction has been processed.  */
	* pvalid = t_branch;
	break;
      }

    case 0xb5:
    case 0x95:// SUBW<c> <Rd>,<Rn>,#<imm12>
      {
	ARMword Rd = ntBITS (8, 11);
	ARMword Rn = tBITS (0, 3);
	ARMword imm12 = (tBIT (10) << 11) | (ntBITS (12, 14) << 8) | ntBITS (0, 7);

	tASSERT (tBIT (4) == 0);
	tASSERT (ntBIT (15) == 0);

	/* Note the ARM ARM indicates special cases for Rn == 15 (ADR)
	   and Rn == 13 (SUB SP minus immediate), but these are implemented
	   in exactly the same way as the normal SUBW insn.  */
	state->Reg[Rd] = state->Reg[Rn] - imm12;

	* pvalid = t_resolved;
	break;
      }

    case 0xB6:
    case 0x96: // MOVT<c> <Rd>,#<imm16>
      {
	ARMword Rd = ntBITS (8, 11);
	ARMword imm = (tBITS (0, 3) << 12) | (tBIT (10) << 11) | (ntBITS (12, 14) << 8) | ntBITS (0, 7);

	state->Reg[Rd] &= 0xFFFF;
	state->Reg[Rd] |= (imm << 16);
	* pvalid = t_resolved;
	break;
      }

    case 0x9A: // SBFXc> <Rd>,<Rn>,#<lsb>,#<width>
      tASSERT (tBIT (4) == 0);
      tASSERT (ntBIT (15) == 0);
      tASSERT (ntBIT (5) == 0);
      * ainstr = 0xE7A00050;
      * ainstr |= (ntBITS (0, 4) << 16); // widthm1
      * ainstr |= (ntBITS (8, 11) << 12); // Rd
      * ainstr |= (((ntBITS (12, 14) << 2) | ntBITS (6, 7)) << 7); // lsb
      * ainstr |= tBITS (0, 3); // Rn
      * pvalid = t_decoded;
      break;

    case 0x9B:
      {
	ARMword Rd = ntBITS (8, 11);
	ARMword Rn = tBITS (0, 3);
	ARMword msbit = ntBITS (0, 5);
	ARMword lsbit = (ntBITS (12, 14) << 2) | ntBITS (6, 7);
	ARMword mask = -(1 << lsbit);

	tASSERT (tBIT (4) == 0);
	tASSERT (ntBIT (15) == 0);
	tASSERT (ntBIT (5) == 0);

	mask &= ((1 << (msbit + 1)) - 1);

	if (lsbit > msbit)
	  ; // UNPREDICTABLE
	else if (Rn == 15)
	  {
	    // BFC<c> <Rd>,#<lsb>,#<width>
	    state->Reg[Rd] &= ~ mask;
	  }
	else
	  {
	    // BFI<c> <Rd>,<Rn>,#<lsb>,#<width>
	    ARMword val = state->Reg[Rn] & (mask >> lsbit);

	    val <<= lsbit;
	    state->Reg[Rd] &= ~ mask;
	    state->Reg[Rd] |= val;
	  }
	
	* pvalid = t_resolved;
	break;
      }

    case 0x9E: // UBFXc> <Rd>,<Rn>,#<lsb>,#<width>
      tASSERT (tBIT (4) == 0);
      tASSERT (ntBIT (15) == 0);
      tASSERT (ntBIT (5) == 0);
      * ainstr = 0xE7E00050;
      * ainstr |= (ntBITS (0, 4) << 16); // widthm1
      * ainstr |= (ntBITS (8, 11) << 12); // Rd
      * ainstr |= (((ntBITS (12, 14) << 2) | ntBITS (6, 7)) << 7); // lsb
      * ainstr |= tBITS (0, 3); // Rn
      * pvalid = t_decoded;
      break;

    case 0xC0: // STRB
    case 0xC4: // LDRB
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rt = ntBITS (12, 15);
	
	if (tBIT (4))
	  {
	    if (Rn == 15)
	      {
		tASSERT (Rt != 15);

		/* LDRB<c> <Rt>,<label>                     => 1111 1000 U001 1111 */
		* ainstr = 0xE55F0000;
		* ainstr |= (tBIT (7) << 23);
		* ainstr |= ntBITS (0, 11);
	      }
	    else if (tBIT (7))
	      {
		/* LDRB<c>.W <Rt>,[<Rn>{,#<imm12>}]         => 1111 1000 1001 rrrr */
		* ainstr = 0xE5D00000;
		* ainstr |= ntBITS (0, 11);
	      }
	    else if (ntBIT (11) == 0)
	      {
		/* LDRB<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<imm2>}] => 1111 1000 0001 rrrr */
		* ainstr = 0xE7D00000;
		* ainstr |= (ntBITS (4, 5) << 7);
		* ainstr |= ntBITS (0, 3);
	      }
	    else
	      {
		int P = ntBIT (10);
		int U = ntBIT (9);
		int W = ntBIT (8);

		tASSERT (! (Rt == 15 && P && !U && !W));
		tASSERT (! (P && U && !W));

		/* LDRB<c> <Rt>,[<Rn>,#-<imm8>]             => 1111 1000 0001 rrrr
		   LDRB<c> <Rt>,[<Rn>],#+/-<imm8>           => 1111 1000 0001 rrrr
		   LDRB<c> <Rt>,[<Rn>,#+/-<imm8>]!          => 1111 1000 0001 rrrr */
		* ainstr = 0xE4500000;
		* ainstr |= (P << 24);
		* ainstr |= (U << 23);
		* ainstr |= (W << 21);
		* ainstr |= ntBITS (0, 7);
	      }
	  }
	else
	  {
	    if (tBIT (7) == 1)
	      {
		// STRB<c>.W <Rt>,[<Rn>,#<imm12>]
		ARMword imm12 = ntBITS (0, 11);

		ARMul_StoreByte (state, state->Reg[Rn] + imm12, state->Reg [Rt]);
		* pvalid = t_branch;
		break;
	      }
	    else if (ntBIT (11))
	      {
		// STRB<c> <Rt>,[<Rn>,#-<imm8>]
		// STRB<c> <Rt>,[<Rn>],#+/-<imm8>
		// STRB<c> <Rt>,[<Rn>,#+/-<imm8>]!
		int P = ntBIT (10);
		int U = ntBIT (9);
		int W = ntBIT (8);
		ARMword imm8 = ntBITS (0, 7);

		tASSERT (! (P && U && !W));
		tASSERT (! (Rn == 13 && P && !U && W && imm8 == 4));
		
		* ainstr = 0xE4000000;
		* ainstr |= (P << 24);
		* ainstr |= (U << 23);
		* ainstr |= (W << 21);
		* ainstr |= imm8;
	      }
	    else
	      {
		// STRB<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<imm2>}]
		tASSERT (ntBITS (6, 11) == 0);

		* ainstr = 0xE7C00000;
		* ainstr |= (ntBITS (4, 5) << 7);
		* ainstr |= ntBITS (0, 3);
	      }
	  }
	
	* ainstr |= (Rn << 16);
	* ainstr |= (Rt << 12);
	* pvalid = t_decoded;
	break;
      }

    case 0xC2: // LDR, STR
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rt = ntBITS (12, 15);
	ARMword imm8 = ntBITS (0, 7);
	ARMword P = ntBIT (10);
	ARMword U = ntBIT (9);
	ARMword W = ntBIT (8);

	tASSERT (Rn != 15);

	if (tBIT (4))
	  {
	    if (Rn == 15)
	      {
		// LDR<c>.W <Rt>,<label>
		* ainstr = 0xE51F0000;
		* ainstr |= ntBITS (0, 11);
	      }
	    else if (ntBIT (11))
	      {
		tASSERT (! (P && U && ! W));
		tASSERT (! (!P && U && W && Rn == 13 && imm8 == 4 && ntBIT (11) == 0));
		tASSERT (! (P && !U && W && Rn == 13 && imm8 == 4 && ntBIT (11)));

		// LDR<c> <Rt>,[<Rn>,#-<imm8>]
		// LDR<c> <Rt>,[<Rn>],#+/-<imm8>
		// LDR<c> <Rt>,[<Rn>,#+/-<imm8>]!
		if (!P && W)
		  W = 0;
		* ainstr = 0xE4100000;
		* ainstr |= (P << 24);
		* ainstr |= (U << 23);
		* ainstr |= (W << 21);
		* ainstr |= imm8;
	      }
	    else
	      {
		// LDR<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<imm2>}]

		tASSERT (ntBITS (6, 11) == 0);

		* ainstr = 0xE7900000;
		* ainstr |= ntBITS (4, 5) << 7;
		* ainstr |= ntBITS (0, 3);
	      }
	  }
	else
	  {
	    if (ntBIT (11))
	      {
		tASSERT (! (P && U && ! W));
		if (Rn == 13 && P && !U && W && imm8 == 4)
		  {
		    // PUSH<c>.W <register>
		    tASSERT (ntBITS (0, 11) == 0xD04);
		    tASSERT (tBITS (0, 4) == 0x0D);

		    * ainstr = 0xE92D0000;
		    * ainstr |= (1 << Rt);

		    Rt = Rn = 0;
		  }
		else
		  {
		    tASSERT (! (P && U && !W));
		    if (!P && W)
		      W = 0;
		    // STR<c> <Rt>,[<Rn>,#-<imm8>]
		    // STR<c> <Rt>,[<Rn>],#+/-<imm8>
		    // STR<c> <Rt>,[<Rn>,#+/-<imm8>]!
		    * ainstr = 0xE4000000;
		    * ainstr |= (P << 24);
		    * ainstr |= (U << 23);
		    * ainstr |= (W << 21);
		    * ainstr |= imm8;
		  }
	      }
	    else
	      {
		// STR<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<imm2>}]
		tASSERT (ntBITS (6, 11) == 0);

		* ainstr = 0xE7800000;
		* ainstr |= ntBITS (4, 5) << 7;
		* ainstr |= ntBITS (0, 3);
	      }
	  }
	
	* ainstr |= (Rn << 16);
	* ainstr |= (Rt << 12);
	* pvalid = t_decoded;
	break;
      }

    case 0xC1: // STRH
    case 0xC5: // LDRH
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rt = ntBITS (12, 15);
	ARMword address;

	tASSERT (Rn != 15);

	if (tBIT (4) == 1)
	  {
	    if (tBIT (7))
	      {
		// LDRH<c>.W <Rt>,[<Rn>{,#<imm12>}]
		ARMword imm12 = ntBITS (0, 11);
		address = state->Reg[Rn] + imm12;
	      }
	    else if (ntBIT (11))
	      {
		// LDRH<c> <Rt>,[<Rn>,#-<imm8>]
		// LDRH<c> <Rt>,[<Rn>],#+/-<imm8>
		// LDRH<c> <Rt>,[<Rn>,#+/-<imm8>]!
		ARMword P = ntBIT (10);
		ARMword U = ntBIT (9);
		ARMword W = ntBIT (8);
		ARMword imm8 = ntBITS (0, 7);

		tASSERT (Rn != 15);
		tASSERT (! (P && U && !W));

		* ainstr = 0xE05000B0;
		* ainstr |= (P << 24);
		* ainstr |= (U << 23);
		* ainstr |= (W << 21);
		* ainstr |= (Rn << 16);
		* ainstr |= (Rt << 12);
		* ainstr |= ((imm8 & 0xF0) << 4);
		* ainstr |= (imm8 & 0xF);
		* pvalid = t_decoded;
		break;
	      }
	    else
	      {
		// LDRH<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<imm2>}]
		ARMword Rm = ntBITS (0, 3);
		ARMword imm2 = ntBITS (4, 5);
		
		tASSERT (ntBITS (6, 10) == 0);

		address = state->Reg[Rn] + (state->Reg[Rm] << imm2);
	      }

	    state->Reg[Rt] = ARMul_LoadHalfWord (state, address);
	  }
	else
	  {
	    if (tBIT (7))
	      {
		// STRH<c>.W <Rt>,[<Rn>{,#<imm12>}]
		ARMword imm12 = ntBITS (0, 11);

		address = state->Reg[Rn] + imm12;
	      }
	    else if (ntBIT (11))
	      {
		// STRH<c> <Rt>,[<Rn>,#-<imm8>]
		// STRH<c> <Rt>,[<Rn>],#+/-<imm8>
		// STRH<c> <Rt>,[<Rn>,#+/-<imm8>]!
		ARMword P = ntBIT (10);
		ARMword U = ntBIT (9);
		ARMword W = ntBIT (8);
		ARMword imm8 = ntBITS (0, 7);

		tASSERT (! (P && U && !W));

		* ainstr = 0xE04000B0;
		* ainstr |= (P << 24);
		* ainstr |= (U << 23);
		* ainstr |= (W << 21);
		* ainstr |= (Rn << 16);
		* ainstr |= (Rt << 12);
		* ainstr |= ((imm8 & 0xF0) << 4);
		* ainstr |= (imm8 & 0xF);
		* pvalid = t_decoded;
		break;
	      }
	    else
	      {
		// STRH<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<imm2>}]
		ARMword Rm = ntBITS (0, 3);
		ARMword imm2 = ntBITS (4, 5);
		
		tASSERT (ntBITS (6, 10) == 0);

		address = state->Reg[Rn] + (state->Reg[Rm] << imm2);
	      }

	    ARMul_StoreHalfWord (state, address, state->Reg [Rt]);
	  }
	* pvalid = t_branch;
	break;
      }

    case 0xC6: // LDR.W/STR.W
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rt = ntBITS (12, 15);
	ARMword imm12 = ntBITS (0, 11);
	ARMword address = state->Reg[Rn];
	
	if (Rn == 15)
	  {
	    // LDR<c>.W <Rt>,<label>
	    tASSERT (tBIT (4) == 1);
	    // tASSERT (tBIT (7) == 1)
	  }

	address += imm12;
	if (tBIT (4) == 1)
	  state->Reg[Rt] = ARMul_LoadWordN (state, address);
	else
	  ARMul_StoreWordN (state, address, state->Reg [Rt]);

	* pvalid = t_resolved;
	break;
      }

    case 0xC8:
    case 0xCC: // LDRSB
      {
	ARMword Rt = ntBITS (12, 15);
	ARMword Rn = tBITS (0, 3);
	ARMword U = tBIT (7);
	ARMword address = state->Reg[Rn];

	tASSERT (tBIT (4) == 1);
	tASSERT (Rt != 15); // PLI

	if (Rn == 15)
	  {
	    // LDRSB<c> <Rt>,<label>
	    ARMword imm12 = ntBITS (0, 11);
	    address += (U ? imm12 : - imm12);
	  }
	else if (U)
	  {
	    // LDRSB<c> <Rt>,[<Rn>,#<imm12>]
	    ARMword imm12 = ntBITS (0, 11);
	    address += imm12;
	  }
	else if (ntBIT (11))
	  {
	    // LDRSB<c> <Rt>,[<Rn>,#-<imm8>]
	    // LDRSB<c> <Rt>,[<Rn>],#+/-<imm8>
	    // LDRSB<c> <Rt>,[<Rn>,#+/-<imm8>]!
	    * ainstr = 0xE05000D0;
	    * ainstr |= ntBIT (10) << 24; // P
	    * ainstr |= ntBIT (9) << 23; // U
	    * ainstr |= ntBIT (8) << 21; // W
	    * ainstr |= Rn << 16;
	    * ainstr |= Rt << 12;
	    * ainstr |= ntBITS (4, 7) << 8;
	    * ainstr |= ntBITS (0, 3);
	    * pvalid = t_decoded;
	    break;
	  }
	else
	  {
	    // LDRSB<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<imm2>}]
	    ARMword Rm = ntBITS (0, 3);
	    ARMword imm2 = ntBITS (4,5);

	    tASSERT (ntBITS (6, 11) == 0);

	    address += (state->Reg[Rm] << imm2);
	  }
	
	state->Reg[Rt] = ARMul_LoadByte (state, address);
	if (state->Reg[Rt] & 0x80)
	  state->Reg[Rt] |= -(1 << 8);

	* pvalid = t_resolved;
	break;
      }

    case 0xC9:
    case 0xCD:// LDRSH
      {
	ARMword Rt = ntBITS (12, 15);
	ARMword Rn = tBITS (0, 3);
	ARMword U = tBIT (7);
	ARMword address = state->Reg[Rn];

	tASSERT (tBIT (4) == 1);

	if (Rn == 15 || U == 1)
	  {
	    // Rn==15 => LDRSH<c> <Rt>,<label>
	    // Rn!=15 => LDRSH<c> <Rt>,[<Rn>,#<imm12>]
	    ARMword imm12 = ntBITS (0, 11);

	    address += (U ? imm12 : - imm12);
	  }
	else if (ntBIT (11))
	  {
	    // LDRSH<c> <Rt>,[<Rn>,#-<imm8>]
	    // LDRSH<c> <Rt>,[<Rn>],#+/-<imm8>
	    // LDRSH<c> <Rt>,[<Rn>,#+/-<imm8>]!
	    * ainstr = 0xE05000F0;
	    * ainstr |= ntBIT (10) << 24; // P
	    * ainstr |= ntBIT (9) << 23; // U
	    * ainstr |= ntBIT (8) << 21; // W
	    * ainstr |= Rn << 16;
	    * ainstr |= Rt << 12;
	    * ainstr |= ntBITS (4, 7) << 8;
	    * ainstr |= ntBITS (0, 3);
	    * pvalid = t_decoded;
	    break;
	  }
	else /* U == 0 */
	  {
	    // LDRSH<c>.W <Rt>,[<Rn>,<Rm>{,LSL #<imm2>}]
	    ARMword Rm = ntBITS (0, 3);
	    ARMword imm2 = ntBITS (4,5);

	    tASSERT (ntBITS (6, 11) == 0);

	    address += (state->Reg[Rm] << imm2);
	  }

	state->Reg[Rt] = ARMul_LoadHalfWord (state, address);
	if (state->Reg[Rt] & 0x8000)
	  state->Reg[Rt] |= -(1 << 16);

	* pvalid = t_branch;
	break;
      }

    case 0x0D0:
      {
	ARMword Rm = ntBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);

	tASSERT (ntBITS (12, 15) == 15);

	if (ntBIT (7) == 1)
	  {
	    // SXTH<c>.W <Rd>,<Rm>{,<rotation>}
	    ARMword ror = ntBITS (4, 5) << 3;
	    ARMword val;

	    val = state->Reg[Rm];
	    val = (val >> ror) | (val << (32 - ror));
	    if (val & 0x8000)
	      val |= -(1 << 16);
	    state->Reg[Rd] = val;
	  }
	else
	  {
	    // LSL{S}<c>.W <Rd>,<Rn>,<Rm>
	    ARMword Rn = tBITS (0, 3);

	    tASSERT (ntBITS (4, 6) == 0);

	    state->Reg[Rd] = state->Reg[Rn] << (state->Reg[Rm] & 0xFF);
	    if (tBIT (4))
	      ARMul_NegZero (state, state->Reg[Rd]);
	  }
	* pvalid = t_branch;
	break;
      }

    case 0x0D1: // LSR{S}<c>.W <Rd>,<Rn>,<Rm>
      {
	ARMword Rd = ntBITS (8, 11);
	ARMword Rn = tBITS (0, 3);
	ARMword Rm = ntBITS (0, 3);

	tASSERT (ntBITS (12, 15) == 15);
	tASSERT (ntBITS (4, 7) == 0);

	state->Reg[Rd] = state->Reg[Rn] >> (state->Reg[Rm] & 0xFF);
	if (tBIT (4))
	  ARMul_NegZero (state, state->Reg[Rd]);
	* pvalid = t_resolved;
	break;
      }

    case 0xD2:
      tASSERT (ntBITS (12, 15) == 15);
      if (ntBIT (7))
	{
	  tASSERT (ntBIT (6) == 0);
	  // UXTB<c>.W <Rd>,<Rm>{,<rotation>}
	  * ainstr = 0xE6EF0070;
	  * ainstr |= (ntBITS (4, 5) << 10); // rotate
	  * ainstr |= ntBITS (0, 3); // Rm
	}
      else
	{
	  // ASR{S}<c>.W <Rd>,<Rn>,<Rm>
	  tASSERT (ntBITS (4, 7) == 0);
	  * ainstr = 0xE1A00050;
	  if (! in_IT_block ())
	    * ainstr |= (tBIT (4) << 20);
	  * ainstr |= (ntBITS (0, 3) << 8); // Rm
	  * ainstr |= tBITS (0, 3); // Rn
	}

      * ainstr |= (ntBITS (8, 11) << 12); // Rd
      * pvalid = t_decoded;
      break;

    case 0xD3: // ROR{S}<c>.W <Rd>,<Rn>,<Rm>
      tASSERT (ntBITS (12, 15) == 15);
      tASSERT (ntBITS (4, 7) == 0);
      * ainstr = 0xE1A00070;
      if (! in_IT_block ())
	* ainstr |= (tBIT (4) << 20);
      * ainstr |= (ntBITS (8, 11) << 12); // Rd
      * ainstr |= (ntBITS (0, 3) << 8); // Rm
      * ainstr |= (tBITS (0, 3) << 0); // Rn
      * pvalid = t_decoded;
      break;

    case 0xD4:
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);
	ARMword Rm = ntBITS (0, 3);

	tASSERT (ntBITS (12, 15) == 15);

	if (ntBITS (4, 7) == 8)
	  {
	    // REV<c>.W <Rd>,<Rm>
	    ARMword val = state->Reg[Rm];

	    tASSERT (Rm == Rn);

	    state->Reg [Rd] =
	      (val >> 24)
	      | ((val >> 8) & 0xFF00)
	      | ((val << 8) & 0xFF0000)
	      | (val << 24);
	    * pvalid = t_resolved;
	  }
	else
	  {
	    tASSERT (ntBITS (4, 7) == 4);

	    if (tBIT (4) == 1)
	       // UADD8<c> <Rd>,<Rn>,<Rm>
	      * ainstr = 0xE6500F10;
	    else
	      // UADD16<c> <Rd>,<Rn>,<Rm>
	      * ainstr = 0xE6500F90;
	
	    * ainstr |= (Rn << 16);
	    * ainstr |= (Rd << 12);
	    * ainstr |= (Rm <<  0);
	    * pvalid = t_decoded;
	  }
	break;
      }

    case 0xD5:
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);
	ARMword Rm = ntBITS (0, 3);

	tASSERT (ntBITS (12, 15) == 15);
	tASSERT (ntBITS (4, 7) == 8);

	if (tBIT (4))
	  {
	    // CLZ<c> <Rd>,<Rm>
	    tASSERT (Rm == Rn);
	    * ainstr = 0xE16F0F10;
	  }
	else
	  {
	     // SEL<c> <Rd>,<Rn>,<Rm>
	    * ainstr = 0xE6800FB0;
	    * ainstr |= (Rn << 16);
	  }

	* ainstr |= (Rd << 12);
	* ainstr |= (Rm <<  0);
	* pvalid = t_decoded;
	break;
      }

    case 0xD8: // MUL
      {
	ARMword Rn = tBITS (0, 3);
	ARMword Rm = ntBITS (0, 3);
	ARMword Rd = ntBITS (8, 11);
	ARMword Ra = ntBITS (12, 15);

	if (tBIT (4))
	  {
	    // SMLA<x><y><c> <Rd>,<Rn>,<Rm>,<Ra>
	    ARMword nval = state->Reg[Rn];
	    ARMword mval = state->Reg[Rm];
	    ARMword res;

	    tASSERT (ntBITS (6, 7) == 0);
	    tASSERT (Ra != 15);

	    if (ntBIT (5))
	      nval >>= 16;
	    else
	      nval &= 0xFFFF;

	    if (ntBIT (4))
	      mval >>= 16;
	    else
	      mval &= 0xFFFF;

	    res = nval * mval;
	    res += state->Reg[Ra];
	    // FIXME: Test and clear/set the Q bit.
	    state->Reg[Rd] = res;
	  }
	else
	  {
	    if (ntBITS (4, 7) == 1)
	      {
		// MLS<c> <Rd>,<Rn>,<Rm>,<Ra>
		state->Reg[Rd] = state->Reg[Ra] - (state->Reg[Rn] * state->Reg[Rm]);
	      }
	    else
	      {
		tASSERT (ntBITS (4, 7) == 0);

		if (Ra == 15)
		  // MUL<c> <Rd>,<Rn>,<Rm>
		  state->Reg[Rd] = state->Reg[Rn] * state->Reg[Rm];
		else
		  // MLA<c> <Rd>,<Rn>,<Rm>,<Ra>
		  state->Reg[Rd] = state->Reg[Rn] * state->Reg[Rm] + state->Reg[Ra];
	      }
	  }
	* pvalid = t_resolved;
	break;
      }

    case 0xDC:
      if (tBIT (4) == 0 && ntBITS (4, 7) == 0)
	{
	  // SMULL
	  * ainstr = 0xE0C00090;
	  * ainstr |= (ntBITS (8, 11) << 16); // RdHi
	  * ainstr |= (ntBITS (12, 15) << 12); // RdLo
	  * ainstr |= (ntBITS (0, 3) << 8); // Rm
	  * ainstr |= tBITS (0, 3); // Rn
	  * pvalid = t_decoded;
	}
      else if (tBIT (4) == 1 && ntBITS (4, 7) == 0xF)
	{
	  // SDIV
	  * ainstr = 0xE710F010;
	  * ainstr |= (ntBITS (8, 11) << 16); // Rd
	  * ainstr |= (ntBITS (0, 3) << 8);   // Rm
	  * ainstr |= tBITS (0, 3); // Rn
	  * pvalid = t_decoded;
	}
      else
	{
	  fprintf (stderr, "(op = %x) ", tBITS (5,12));
	  tASSERT (0);
	  return;
	}
      break;

    case 0xDD:
      if (tBIT (4) == 0 && ntBITS (4, 7) == 0)
	{
	  // UMULL
	  * ainstr = 0xE0800090;
	  * ainstr |= (ntBITS (8, 11) << 16); // RdHi
	  * ainstr |= (ntBITS (12, 15) << 12); // RdLo
	  * ainstr |= (ntBITS (0, 3) << 8); // Rm
	  * ainstr |= tBITS (0, 3); // Rn
	  * pvalid = t_decoded;
	}
      else if (tBIT (4) == 1 && ntBITS (4, 7) == 0xF)
	{
	  // UDIV
	  * ainstr = 0xE730F010;
	  * ainstr |= (ntBITS (8, 11) << 16); // Rd
	  * ainstr |= (ntBITS (0, 3) << 8);   // Rm
	  * ainstr |= tBITS (0, 3); // Rn
	  * pvalid = t_decoded;
	}
      else
	{
	  fprintf (stderr, "(op = %x) ", tBITS (5,12));
	  tASSERT (0);
	  return;
	}
      break;

    case 0xDF: // UMLAL
      tASSERT (tBIT (4) == 0);
      tASSERT (ntBITS (4, 7) == 0);
      * ainstr = 0xE0A00090;
      * ainstr |= (ntBITS (8, 11) << 16); // RdHi
      * ainstr |= (ntBITS (12, 15) << 12); // RdLo
      * ainstr |= (ntBITS (0, 3) << 8); // Rm
      * ainstr |= tBITS (0, 3); // Rn
      * pvalid = t_decoded;
      break;

    default:
      fprintf (stderr, "(op = %x) ", tBITS (5,12));
      tASSERT (0);
      return;
    }

  /* Tell the Thumb decoder to skip the next 16-bit insn - it was
     part of this insn - unless this insn has changed the PC.  */
  skipping_32bit_thumb = pc + 2;
}

/* Attempt to emulate an ARMv6 instruction.
   Stores t_branch into PVALUE upon success or t_undefined otherwise.  */

static void
handle_v6_thumb_insn (ARMul_State * state,
		      ARMword       tinstr,
		      ARMword       next_instr,
		      ARMword       pc,
		      ARMword *     ainstr,
		      tdstate *     pvalid)
{
  if (! state->is_v6)
    {
      * pvalid = t_undefined;
      return;
    }

  if (tBITS (12, 15) == 0xB
      && tBIT (10) == 0
      && tBIT (8) == 1)
    {
      // Conditional branch forwards.
      ARMword Rn = tBITS (0, 2);
      ARMword imm5 = tBIT (9) << 5 | tBITS (3, 7);

      if (tBIT (11))
	{
	  if (state->Reg[Rn] != 0)
	    {
	      state->Reg[15] = (pc + 4 + imm5 * 2);
	      FLUSHPIPE;
	    }
	}
      else
	{
	  if (state->Reg[Rn] == 0)
	    {
	      state->Reg[15] = (pc + 4 + imm5 * 2);
	      FLUSHPIPE;
	    }
	}
      * pvalid = t_branch;
      return;
    }

  switch (tinstr & 0xFFC0)
    {
    case 0x4400:
    case 0x4480:
    case 0x4440:
    case 0x44C0: // ADD
      {
	ARMword Rd = (tBIT (7) << 3) | tBITS (0, 2);
	ARMword Rm = tBITS (3, 6);
	state->Reg[Rd] += state->Reg[Rm];
	break;
      }

    case 0x4600: // MOV<c> <Rd>,<Rm>
      {
	// instr [15, 8] = 0100 0110
	// instr [7]     = Rd<high>
	// instr [6,3]   = Rm
	// instr [2,0]   = Rd<low>
	ARMword Rd = (tBIT(7) << 3) | tBITS (0, 2);
	// FIXME: Check for Rd == 15 and ITblock.
	state->Reg[Rd] = state->Reg[tBITS (3, 6)];
	break;
      }

    case 0xBF00:
    case 0xBF40:
    case 0xBF80:
    case 0xBFC0:
      handle_IT_block (state, tinstr, pvalid);
      return;

    case 0xE840:
    case 0xE880: // LDMIA
    case 0xE8C0:
    case 0xE900: // STM
    case 0xE940:
    case 0xE980:
    case 0xE9C0: // LDRD
    case 0xEA00: // BIC
    case 0xEA40: // ORR
    case 0xEA80: // EOR
    case 0xEAC0:
    case 0xEB00: // ADD
    case 0xEB40: // SBC
    case 0xEB80: // SUB
    case 0xEBC0: // RSB
    case 0xFA80: // UADD, SEL
    case 0xFBC0: // UMULL, SMULL, SDIV, UDIV
      handle_T2_insn (state, tinstr, next_instr, pc, ainstr, pvalid);
      return;

    case 0xba00: /* rev */
      {
	ARMword val = state->Reg[tBITS (3, 5)];
	state->Reg [tBITS (0, 2)] =
	  (val >> 24)
	  | ((val >> 8) & 0xFF00)
	  | ((val << 8) & 0xFF0000)
	  | (val << 24);
	break;
      }

    case 0xba40: /* rev16 */
      {
	ARMword val = state->Reg[tBITS (3, 5)];
	state->Reg [tBITS (0, 2)] = (val >> 16) | (val << 16);
	break;
      }

    case 0xb660: /* cpsie */
    case 0xb670: /* cpsid */
    case 0xbac0: /* revsh */
    case 0xb650: /* setend */
    default:
      printf ("Unhandled v6 thumb insn: %04x\n", tinstr);
      * pvalid = t_undefined;
      return;

    case 0xb200: /* sxth */
      {
	ARMword Rm = state->Reg [(tinstr & 0x38) >> 3];

	if (Rm & 0x8000)
	  state->Reg [(tinstr & 0x7)] = (Rm & 0xffff) | 0xffff0000;
	else
	  state->Reg [(tinstr & 0x7)] = Rm & 0xffff;
	break;
      }

    case 0xb240: /* sxtb */
      {
	ARMword Rm = state->Reg [(tinstr & 0x38) >> 3];

	if (Rm & 0x80)
	  state->Reg [(tinstr & 0x7)] = (Rm & 0xff) | 0xffffff00;
	else
	  state->Reg [(tinstr & 0x7)] = Rm & 0xff;
	break;
      }

    case 0xb280: /* uxth */
      {
	ARMword Rm = state->Reg [(tinstr & 0x38) >> 3];

	state->Reg [(tinstr & 0x7)] = Rm & 0xffff;
	break;
      }

    case 0xb2c0: /* uxtb */
      {
	ARMword Rm = state->Reg [(tinstr & 0x38) >> 3];

	state->Reg [(tinstr & 0x7)] = Rm & 0xff;
	break;
      }
    }
  /* Indicate that the instruction has been processed.  */
  * pvalid = t_branch;
}

/* Decode a 16bit Thumb instruction.  The instruction is in the low
   16-bits of the tinstr field, with the following Thumb instruction
   held in the high 16-bits.  Passing in two Thumb instructions allows
   easier simulation of the special dual BL instruction.  */

tdstate
ARMul_ThumbDecode (ARMul_State * state,
		   ARMword       pc,
		   ARMword       tinstr,
		   ARMword *     ainstr)
{
  tdstate valid = t_decoded;	/* default assumes a valid instruction */
  ARMword next_instr;
  ARMword  old_tinstr = tinstr;

  if (skipping_32bit_thumb == pc)
    {
      skipping_32bit_thumb = 0;
      return t_branch;
    }
  skipping_32bit_thumb = 0;

  if (state->bigendSig)
    {
      next_instr = tinstr & 0xFFFF;
      tinstr >>= 16;
    }
  else
    {
      next_instr = tinstr >> 16;
      tinstr &= 0xFFFF;
    }

  if (! IT_block_allow (state))
    {
      if (   tBITS (11, 15) == 0x1F
	  || tBITS (11, 15) == 0x1E
	  || tBITS (11, 15) == 0x1D)
	{
	  if (trace)
	    fprintf (stderr, "pc: %x, SKIP  instr: %04x|%04x\n",
		     pc & ~1, tinstr, next_instr);
	  skipping_32bit_thumb = pc + 2;
	}
      else if (trace)
	fprintf (stderr, "pc: %x, SKIP  instr: %04x\n", pc & ~1, tinstr);

      return t_branch;
    }

  old_tinstr = tinstr;
  if (trace)
    fprintf (stderr, "pc: %x, Thumb instr: %x", pc & ~1, tinstr);

#if 1				/* debugging to catch non updates */
  *ainstr = 0xDEADC0DE;
#endif

  switch ((tinstr & 0xF800) >> 11)
    {
    case 0:			/* LSL */
    case 1:			/* LSR */
    case 2:			/* ASR */
      /* Format 1 */
      *ainstr = 0xE1B00000	/* base opcode */
	| ((tinstr & 0x1800) >> (11 - 5))	/* shift type */
	| ((tinstr & 0x07C0) << (7 - 6))	/* imm5 */
	| ((tinstr & 0x0038) >> 3)	/* Rs */
	| ((tinstr & 0x0007) << 12);	/* Rd */
      break;
    case 3:			/* ADD/SUB */
      /* Format 2 */
      {
	ARMword subset[4] =
	  {
	    0xE0900000,		/* ADDS Rd,Rs,Rn    */
	    0xE0500000,		/* SUBS Rd,Rs,Rn    */
	    0xE2900000,		/* ADDS Rd,Rs,#imm3 */
	    0xE2500000		/* SUBS Rd,Rs,#imm3 */
	  };
	/* It is quicker indexing into a table, than performing switch
	   or conditionals: */
	*ainstr = subset[(tinstr & 0x0600) >> 9]	/* base opcode */
	  | ((tinstr & 0x01C0) >> 6)	/* Rn or imm3 */
	  | ((tinstr & 0x0038) << (16 - 3))	/* Rs */
	  | ((tinstr & 0x0007) << (12 - 0));	/* Rd */

	if (in_IT_block ())
	  *ainstr &= ~ (1 << 20);
      }
      break;
    case 4:
      * ainstr = 0xE3A00000; /* MOV  Rd,#imm8    */
      if (! in_IT_block ())
	* ainstr |= (1 << 20);
      * ainstr |= tBITS (8, 10) << 12;
      * ainstr |= tBITS (0, 7);
      break;

    case 5:
      * ainstr = 0xE3500000;	/* CMP  Rd,#imm8    */
      * ainstr |= tBITS (8, 10) << 16;
      * ainstr |= tBITS (0, 7);
      break;

    case 6:
    case 7:
      * ainstr = tBIT (11)
	? 0xE2400000		/* SUB  Rd,Rd,#imm8 */
	: 0xE2800000;		/* ADD  Rd,Rd,#imm8 */
      if (! in_IT_block ())
	* ainstr |= (1 << 20);
      * ainstr |= tBITS (8, 10) << 12;
      * ainstr |= tBITS (8, 10) << 16;
      * ainstr |= tBITS (0, 7);
      break;

    case 8:			/* Arithmetic and high register transfers */
      /* TODO: Since the subsets for both Format 4 and Format 5
         instructions are made up of different ARM encodings, we could
         save the following conditional, and just have one large
         subset. */
      if ((tinstr & (1 << 10)) == 0)
	{
	  /* Format 4 */
	  struct insn_format {
	    ARMword opcode;
	    enum { t_norm, t_shift, t_neg, t_mul } otype;
	  };
	  static const struct insn_format subset[16] =
	  {
	    { 0xE0100000, t_norm},			/* ANDS Rd,Rd,Rs     */
	    { 0xE0300000, t_norm},			/* EORS Rd,Rd,Rs     */
	    { 0xE1B00010, t_shift},			/* MOVS Rd,Rd,LSL Rs */
	    { 0xE1B00030, t_shift},			/* MOVS Rd,Rd,LSR Rs */
	    { 0xE1B00050, t_shift},			/* MOVS Rd,Rd,ASR Rs */
	    { 0xE0B00000, t_norm},			/* ADCS Rd,Rd,Rs     */
	    { 0xE0D00000, t_norm},			/* SBCS Rd,Rd,Rs     */
	    { 0xE1B00070, t_shift},			/* MOVS Rd,Rd,ROR Rs */
	    { 0xE1100000, t_norm},			/* TST  Rd,Rs        */
	    { 0xE2700000, t_neg},			/* RSBS Rd,Rs,#0     */
	    { 0xE1500000, t_norm},			/* CMP  Rd,Rs        */
	    { 0xE1700000, t_norm},			/* CMN  Rd,Rs        */
	    { 0xE1900000, t_norm},			/* ORRS Rd,Rd,Rs     */
	    { 0xE0100090, t_mul} ,			/* MULS Rd,Rd,Rs     */
	    { 0xE1D00000, t_norm},			/* BICS Rd,Rd,Rs     */
	    { 0xE1F00000, t_norm}	/* MVNS Rd,Rs        */
	  };
	  *ainstr = subset[(tinstr & 0x03C0) >> 6].opcode;	/* base */

	  if (in_IT_block ())
	    {
	      static const struct insn_format it_subset[16] =
		{
		  { 0xE0000000, t_norm},	/* AND  Rd,Rd,Rs     */
		  { 0xE0200000, t_norm},	/* EOR  Rd,Rd,Rs     */
		  { 0xE1A00010, t_shift},	/* MOV  Rd,Rd,LSL Rs */
		  { 0xE1A00030, t_shift},	/* MOV  Rd,Rd,LSR Rs */
		  { 0xE1A00050, t_shift},	/* MOV  Rd,Rd,ASR Rs */
		  { 0xE0A00000, t_norm},	/* ADC  Rd,Rd,Rs     */
		  { 0xE0C00000, t_norm},	/* SBC  Rd,Rd,Rs     */
		  { 0xE1A00070, t_shift},	/* MOV  Rd,Rd,ROR Rs */
		  { 0xE1100000, t_norm},	/* TST  Rd,Rs        */
		  { 0xE2600000, t_neg},		/* RSB  Rd,Rs,#0     */
		  { 0xE1500000, t_norm},	/* CMP  Rd,Rs        */
		  { 0xE1700000, t_norm},	/* CMN  Rd,Rs        */
		  { 0xE1800000, t_norm},	/* ORR  Rd,Rd,Rs     */
		  { 0xE0000090, t_mul} ,	/* MUL  Rd,Rd,Rs     */
		  { 0xE1C00000, t_norm},	/* BIC  Rd,Rd,Rs     */
		  { 0xE1E00000, t_norm}		/* MVN  Rd,Rs        */
		};
	      *ainstr = it_subset[(tinstr & 0x03C0) >> 6].opcode;	/* base */
	    }

	  switch (subset[(tinstr & 0x03C0) >> 6].otype)
	    {
	    case t_norm:
	      *ainstr |= ((tinstr & 0x0007) << 16)	/* Rn */
		| ((tinstr & 0x0007) << 12)	/* Rd */
		| ((tinstr & 0x0038) >> 3);	/* Rs */
	      break;
	    case t_shift:
	      *ainstr |= ((tinstr & 0x0007) << 12)	/* Rd */
		| ((tinstr & 0x0007) >> 0)	/* Rm */
		| ((tinstr & 0x0038) << (8 - 3));	/* Rs */
	      break;
	    case t_neg:
	      *ainstr |= ((tinstr & 0x0007) << 12)	/* Rd */
		| ((tinstr & 0x0038) << (16 - 3));	/* Rn */
	      break;
	    case t_mul:
	      *ainstr |= ((tinstr & 0x0007) << 16)	/* Rd */
		| ((tinstr & 0x0007) << 8)	/* Rs */
		| ((tinstr & 0x0038) >> 3);	/* Rm */
	      break;
	    }
	}
      else
	{
	  /* Format 5 */
	  ARMword Rd = ((tinstr & 0x0007) >> 0);
	  ARMword Rs = ((tinstr & 0x0038) >> 3);
	  if (tinstr & (1 << 7))
	    Rd += 8;
	  if (tinstr & (1 << 6))
	    Rs += 8;
	  switch ((tinstr & 0x03C0) >> 6)
	    {
	    case 0x1:		/* ADD Rd,Rd,Hs */
	    case 0x2:		/* ADD Hd,Hd,Rs */
	    case 0x3:		/* ADD Hd,Hd,Hs */
	      *ainstr = 0xE0800000	/* base */
		| (Rd << 16)	/* Rn */
		| (Rd << 12)	/* Rd */
		| (Rs << 0);	/* Rm */
	      break;
	    case 0x5:		/* CMP Rd,Hs */
	    case 0x6:		/* CMP Hd,Rs */
	    case 0x7:		/* CMP Hd,Hs */
	      *ainstr = 0xE1500000	/* base */
		| (Rd << 16)	/* Rn */
		| (Rd << 12)	/* Rd */
		| (Rs << 0);	/* Rm */
	      break;
	    case 0x9:		/* MOV Rd,Hs */
	    case 0xA:		/* MOV Hd,Rs */
	    case 0xB:		/* MOV Hd,Hs */
	      *ainstr = 0xE1A00000	/* base */
		| (Rd << 12)	/* Rd */
		| (Rs << 0);	/* Rm */
	      break;
	    case 0xC:		/* BX Rs */
	    case 0xD:		/* BX Hs */
	      *ainstr = 0xE12FFF10	/* base */
		| ((tinstr & 0x0078) >> 3);	/* Rd */
	      break;
	    case 0xE:		/* UNDEFINED */
	    case 0xF:		/* UNDEFINED */
	      if (state->is_v5)
		{
		  /* BLX Rs; BLX Hs */
		  *ainstr = 0xE12FFF30	/* base */
		    | ((tinstr & 0x0078) >> 3);	/* Rd */
		  break;
		}
	      ATTRIBUTE_FALLTHROUGH;
	    default:
	    case 0x0:		/* UNDEFINED */
	    case 0x4:		/* UNDEFINED */
	    case 0x8:		/* UNDEFINED */
	      handle_v6_thumb_insn (state, tinstr, next_instr, pc, ainstr, & valid);
	      break;
	    }
	}
      break;
    case 9:			/* LDR Rd,[PC,#imm8] */
      /* Format 6 */
      *ainstr = 0xE59F0000	/* base */
	| ((tinstr & 0x0700) << (12 - 8))	/* Rd */
	| ((tinstr & 0x00FF) << (2 - 0));	/* off8 */
      break;
    case 10:
    case 11:
      /* TODO: Format 7 and Format 8 perform the same ARM encoding, so
         the following could be merged into a single subset, saving on
         the following boolean: */
      if ((tinstr & (1 << 9)) == 0)
	{
	  /* Format 7 */
	  ARMword subset[4] = {
	    0xE7800000,		/* STR  Rd,[Rb,Ro] */
	    0xE7C00000,		/* STRB Rd,[Rb,Ro] */
	    0xE7900000,		/* LDR  Rd,[Rb,Ro] */
	    0xE7D00000		/* LDRB Rd,[Rb,Ro] */
	  };
	  *ainstr = subset[(tinstr & 0x0C00) >> 10]	/* base */
	    | ((tinstr & 0x0007) << (12 - 0))	/* Rd */
	    | ((tinstr & 0x0038) << (16 - 3))	/* Rb */
	    | ((tinstr & 0x01C0) >> 6);	/* Ro */
	}
      else
	{
	  /* Format 8 */
	  ARMword subset[4] = {
	    0xE18000B0,		/* STRH  Rd,[Rb,Ro] */
	    0xE19000D0,		/* LDRSB Rd,[Rb,Ro] */
	    0xE19000B0,		/* LDRH  Rd,[Rb,Ro] */
	    0xE19000F0		/* LDRSH Rd,[Rb,Ro] */
	  };
	  *ainstr = subset[(tinstr & 0x0C00) >> 10]	/* base */
	    | ((tinstr & 0x0007) << (12 - 0))	/* Rd */
	    | ((tinstr & 0x0038) << (16 - 3))	/* Rb */
	    | ((tinstr & 0x01C0) >> 6);	/* Ro */
	}
      break;
    case 12:			/* STR Rd,[Rb,#imm5] */
    case 13:			/* LDR Rd,[Rb,#imm5] */
    case 14:			/* STRB Rd,[Rb,#imm5] */
    case 15:			/* LDRB Rd,[Rb,#imm5] */
      /* Format 9 */
      {
	ARMword subset[4] = {
	  0xE5800000,		/* STR  Rd,[Rb,#imm5] */
	  0xE5900000,		/* LDR  Rd,[Rb,#imm5] */
	  0xE5C00000,		/* STRB Rd,[Rb,#imm5] */
	  0xE5D00000		/* LDRB Rd,[Rb,#imm5] */
	};
	/* The offset range defends on whether we are transferring a
	   byte or word value: */
	*ainstr = subset[(tinstr & 0x1800) >> 11]	/* base */
	  | ((tinstr & 0x0007) << (12 - 0))	/* Rd */
	  | ((tinstr & 0x0038) << (16 - 3))	/* Rb */
	  | ((tinstr & 0x07C0) >> (6 - ((tinstr & (1 << 12)) ? 0 : 2)));	/* off5 */
      }
      break;
    case 16:			/* STRH Rd,[Rb,#imm5] */
    case 17:			/* LDRH Rd,[Rb,#imm5] */
      /* Format 10 */
      *ainstr = ((tinstr & (1 << 11))	/* base */
		 ? 0xE1D000B0	/* LDRH */
		 : 0xE1C000B0)	/* STRH */
	| ((tinstr & 0x0007) << (12 - 0))	/* Rd */
	| ((tinstr & 0x0038) << (16 - 3))	/* Rb */
	| ((tinstr & 0x01C0) >> (6 - 1))	/* off5, low nibble */
	| ((tinstr & 0x0600) >> (9 - 8));	/* off5, high nibble */
      break;
    case 18:			/* STR Rd,[SP,#imm8] */
    case 19:			/* LDR Rd,[SP,#imm8] */
      /* Format 11 */
      *ainstr = ((tinstr & (1 << 11))	/* base */
		 ? 0xE59D0000	/* LDR */
		 : 0xE58D0000)	/* STR */
	| ((tinstr & 0x0700) << (12 - 8))	/* Rd */
	| ((tinstr & 0x00FF) << 2);	/* off8 */
      break;
    case 20:			/* ADD Rd,PC,#imm8 */
    case 21:			/* ADD Rd,SP,#imm8 */
      /* Format 12 */
      if ((tinstr & (1 << 11)) == 0)
	{
	  /* NOTE: The PC value used here should by word aligned */
	  /* We encode shift-left-by-2 in the rotate immediate field,
	     so no shift of off8 is needed.  */
	  *ainstr = 0xE28F0F00	/* base */
	    | ((tinstr & 0x0700) << (12 - 8))	/* Rd */
	    | (tinstr & 0x00FF);	/* off8 */
	}
      else
	{
	  /* We encode shift-left-by-2 in the rotate immediate field,
	     so no shift of off8 is needed.  */
	  *ainstr = 0xE28D0F00	/* base */
	    | ((tinstr & 0x0700) << (12 - 8))	/* Rd */
	    | (tinstr & 0x00FF);	/* off8 */
	}
      break;
    case 22:
    case 23:
      switch (tinstr & 0x0F00)
	{
	case 0x0000:
	  /* Format 13 */
	  /* NOTE: The instruction contains a shift left of 2
	     equivalent (implemented as ROR #30):  */
	  *ainstr = ((tinstr & (1 << 7))	/* base */
		     ? 0xE24DDF00	/* SUB */
		     : 0xE28DDF00)	/* ADD */
	    | (tinstr & 0x007F);	/* off7 */
	  break;
	case 0x0400:
	  /* Format 14 - Push */
	  * ainstr = 0xE92D0000 | (tinstr & 0x00FF);
	  break;
	case 0x0500:
	  /* Format 14 - Push + LR */
	  * ainstr = 0xE92D4000 | (tinstr & 0x00FF);
	  break;
	case 0x0c00:
	  /* Format 14 - Pop */
	  * ainstr = 0xE8BD0000 | (tinstr & 0x00FF);
	  break;
	case 0x0d00:
	  /* Format 14 - Pop + PC */
	  * ainstr = 0xE8BD8000 | (tinstr & 0x00FF);
	  break;
	case 0x0e00:
	  if (state->is_v5)
	    {
	      /* This is normally an undefined instruction.  The v5t architecture
		 defines this particular pattern as a BKPT instruction, for
		 hardware assisted debugging.  We map onto the arm BKPT
		 instruction.  */
	      if (state->is_v6)
		// Map to the SVC instruction instead of the BKPT instruction.
		* ainstr = 0xEF000000 | tBITS (0, 7);
	      else
		* ainstr = 0xE1200070 | ((tinstr & 0xf0) << 4) | (tinstr & 0xf);
	      break;
	    }
	  ATTRIBUTE_FALLTHROUGH;
	default:
	  /* Everything else is an undefined instruction.  */
	  handle_v6_thumb_insn (state, tinstr, next_instr, pc, ainstr, & valid);
	  break;
	}
      break;
    case 24:			/* STMIA */
    case 25:			/* LDMIA */
      /* Format 15 */
      *ainstr = ((tinstr & (1 << 11))	/* base */
		 ? 0xE8B00000	/* LDMIA */
		 : 0xE8A00000)	/* STMIA */
	| ((tinstr & 0x0700) << (16 - 8))	/* Rb */
	| (tinstr & 0x00FF);	/* mask8 */
      break;
    case 26:			/* Bcc */
    case 27:			/* Bcc/SWI */
      if ((tinstr & 0x0F00) == 0x0F00)
	{
	  /* Format 17 : SWI */
	  *ainstr = 0xEF000000;
	  /* Breakpoint must be handled specially.  */
	  if ((tinstr & 0x00FF) == 0x18)
	    *ainstr |= ((tinstr & 0x00FF) << 16);
	  /* New breakpoint value.  See gdb/arm-tdep.c  */
	  else if ((tinstr & 0x00FF) == 0xFE)
	    *ainstr |= SWI_Breakpoint;
	  else
	    *ainstr |= (tinstr & 0x00FF);
	}
      else if ((tinstr & 0x0F00) != 0x0E00)
	{
	  /* Format 16 */
	  int doit = FALSE;
	  /* TODO: Since we are doing a switch here, we could just add
	     the SWI and undefined instruction checks into this
	     switch to same on a couple of conditionals: */
	  switch ((tinstr & 0x0F00) >> 8)
	    {
	    case EQ:
	      doit = ZFLAG;
	      break;
	    case NE:
	      doit = !ZFLAG;
	      break;
	    case VS:
	      doit = VFLAG;
	      break;
	    case VC:
	      doit = !VFLAG;
	      break;
	    case MI:
	      doit = NFLAG;
	      break;
	    case PL:
	      doit = !NFLAG;
	      break;
	    case CS:
	      doit = CFLAG;
	      break;
	    case CC:
	      doit = !CFLAG;
	      break;
	    case HI:
	      doit = (CFLAG && !ZFLAG);
	      break;
	    case LS:
	      doit = (!CFLAG || ZFLAG);
	      break;
	    case GE:
	      doit = ((!NFLAG && !VFLAG) || (NFLAG && VFLAG));
	      break;
	    case LT:
	      doit = ((NFLAG && !VFLAG) || (!NFLAG && VFLAG));
	      break;
	    case GT:
	      doit = ((!NFLAG && !VFLAG && !ZFLAG)
		      || (NFLAG && VFLAG && !ZFLAG));
	      break;
	    case LE:
	      doit = ((NFLAG && !VFLAG) || (!NFLAG && VFLAG)) || ZFLAG;
	      break;
	    }
	  if (doit)
	    {
	      state->Reg[15] = (pc + 4
				+ (((tinstr & 0x7F) << 1)
				   | ((tinstr & (1 << 7)) ? 0xFFFFFF00 : 0)));
	      FLUSHPIPE;
	    }
	  valid = t_branch;
	}
      else
	/* UNDEFINED : cc=1110(AL) uses different format.  */
	handle_v6_thumb_insn (state, tinstr, next_instr, pc, ainstr, & valid);
      break;
    case 28:			/* B */
      /* Format 18 */
      state->Reg[15] = (pc + 4
			+ (((tinstr & 0x3FF) << 1)
			   | ((tinstr & (1 << 10)) ? 0xFFFFF800 : 0)));
      FLUSHPIPE;
      valid = t_branch;
      break;
    case 29:			/* UNDEFINED */
      if (state->is_v6)
	{
	  handle_v6_thumb_insn (state, tinstr, next_instr, pc, ainstr, & valid);
	  break;
	}

      if (state->is_v5)
	{
	  if (tinstr & 1)
	    {
	      handle_v6_thumb_insn (state, tinstr, next_instr, pc, ainstr, & valid);
	      break;
	    }
	  /* Drop through.  */

	  /* Format 19 */
	  /* There is no single ARM instruction equivalent for this
	     instruction. Also, it should only ever be matched with the
	     fmt19 "BL/BLX instruction 1" instruction.  However, we do
	     allow the simulation of it on its own, with undefined results
	     if r14 is not suitably initialised.  */
	  {
	    ARMword tmp = (pc + 2);

	    state->Reg[15] = ((state->Reg[14] + ((tinstr & 0x07FF) << 1))
			      & 0xFFFFFFFC);
	    CLEART;
	    state->Reg[14] = (tmp | 1);
	    valid = t_branch;
	    FLUSHPIPE;
	    if (trace_funcs)
	      fprintf (stderr, " pc changed to %x\n", state->Reg[15]);
	    break;
	  }
	}

      handle_v6_thumb_insn (state, tinstr, next_instr, pc, ainstr, & valid);
      break;

    case 30:			/* BL instruction 1 */
      if (state->is_v6)
	{
	  handle_T2_insn (state, tinstr, next_instr, pc, ainstr, & valid);
	  break;
	}

      /* Format 19 */
      /* There is no single ARM instruction equivalent for this Thumb
         instruction. To keep the simulation simple (from the user
         perspective) we check if the following instruction is the
         second half of this BL, and if it is we simulate it
         immediately.  */
      state->Reg[14] = state->Reg[15] \
	+ (((tinstr & 0x07FF) << 12) \
	   | ((tinstr & (1 << 10)) ? 0xFF800000 : 0));

      valid = t_branch;		/* in-case we don't have the 2nd half */
      tinstr = next_instr;	/* move the instruction down */
      pc += 2;			/* point the pc at the 2nd half */
      if (((tinstr & 0xF800) >> 11) != 31)
	{
	  if (((tinstr & 0xF800) >> 11) == 29)
	    {
	      ARMword tmp = (pc + 2);

	      state->Reg[15] = ((state->Reg[14]
				 + ((tinstr & 0x07FE) << 1))
				& 0xFFFFFFFC);
	      CLEART;
	      state->Reg[14] = (tmp | 1);
	      valid = t_branch;
	      FLUSHPIPE;
	    }
	  else
	    /* Exit, since not correct instruction. */
	    pc -= 2;
	  break;
	}
      /* else we fall through to process the second half of the BL */
      pc += 2;			/* point the pc at the 2nd half */
      ATTRIBUTE_FALLTHROUGH;
    case 31:			/* BL instruction 2 */
      if (state->is_v6)
	{
	  handle_T2_insn (state, old_tinstr, next_instr, pc, ainstr, & valid);
	  break;
	}

      /* Format 19 */
      /* There is no single ARM instruction equivalent for this
         instruction. Also, it should only ever be matched with the
         fmt19 "BL instruction 1" instruction. However, we do allow
         the simulation of it on its own, with undefined results if
         r14 is not suitably initialised.  */
      {
	ARMword tmp = pc;

	state->Reg[15] = (state->Reg[14] + ((tinstr & 0x07FF) << 1));
	state->Reg[14] = (tmp | 1);
	valid = t_branch;
	FLUSHPIPE;
      }
      break;
    }

  if (trace && valid != t_decoded)
    fprintf (stderr, "\n");

  return valid;
}
