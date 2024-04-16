/* Simulator for Analog Devices Blackfin processors.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "ansidecl.h"
#include "opcode/bfin.h"
#include "sim-main.h"
#include "arch.h"
#include "bfin-sim.h"
#include "dv-bfin_cec.h"
#include "dv-bfin_mmu.h"

#define HOST_LONG_WORD_SIZE (sizeof (long) * 8)

#define SIGNEXTEND(v, n) \
  (((bs32)(v) << (HOST_LONG_WORD_SIZE - (n))) >> (HOST_LONG_WORD_SIZE - (n)))

static ATTRIBUTE_NORETURN void
illegal_instruction (SIM_CPU *cpu)
{
  TRACE_INSN (cpu, "ILLEGAL INSTRUCTION");
  while (1)
    cec_exception (cpu, VEC_UNDEF_I);
}

static ATTRIBUTE_NORETURN void
illegal_instruction_combination (SIM_CPU *cpu)
{
  TRACE_INSN (cpu, "ILLEGAL INSTRUCTION COMBINATION");
  while (1)
    cec_exception (cpu, VEC_ILGAL_I);
}

static ATTRIBUTE_NORETURN void
illegal_instruction_or_combination (SIM_CPU *cpu)
{
  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    illegal_instruction_combination (cpu);
  else
    illegal_instruction (cpu);
}

static ATTRIBUTE_NORETURN void
unhandled_instruction (SIM_CPU *cpu, const char *insn)
{
  SIM_DESC sd = CPU_STATE (cpu);
  bu16 iw0, iw1;
  bu32 iw2;

  TRACE_EVENTS (cpu, "unhandled instruction");

  iw0 = IFETCH (PCREG);
  iw1 = IFETCH (PCREG + 2);
  iw2 = ((bu32)iw0 << 16) | iw1;

  sim_io_eprintf (sd, "Unhandled instruction at 0x%08x (%s opcode 0x", PCREG, insn);
  if ((iw0 & 0xc000) == 0xc000)
    sim_io_eprintf (sd, "%08x", iw2);
  else
    sim_io_eprintf (sd, "%04x", iw0);

  sim_io_eprintf (sd, ") ... aborting\n");

  illegal_instruction (cpu);
}

static const char * const astat_names[] =
{
  [ 0] = "AZ",
  [ 1] = "AN",
  [ 2] = "AC0_COPY",
  [ 3] = "V_COPY",
  [ 4] = "ASTAT_4",
  [ 5] = "CC",
  [ 6] = "AQ",
  [ 7] = "ASTAT_7",
  [ 8] = "RND_MOD",
  [ 9] = "ASTAT_9",
  [10] = "ASTAT_10",
  [11] = "ASTAT_11",
  [12] = "AC0",
  [13] = "AC1",
  [14] = "ASTAT_14",
  [15] = "ASTAT_15",
  [16] = "AV0",
  [17] = "AV0S",
  [18] = "AV1",
  [19] = "AV1S",
  [20] = "ASTAT_20",
  [21] = "ASTAT_21",
  [22] = "ASTAT_22",
  [23] = "ASTAT_23",
  [24] = "V",
  [25] = "VS",
  [26] = "ASTAT_26",
  [27] = "ASTAT_27",
  [28] = "ASTAT_28",
  [29] = "ASTAT_29",
  [30] = "ASTAT_30",
  [31] = "ASTAT_31",
};

typedef enum
{
  c_0, c_1, c_4, c_2, c_uimm2, c_uimm3, c_imm3, c_pcrel4,
  c_imm4, c_uimm4s4, c_uimm4s4d, c_uimm4, c_uimm4s2, c_negimm5s4, c_imm5,
  c_imm5d, c_uimm5, c_imm6, c_imm7, c_imm7d, c_imm8, c_uimm8, c_pcrel8,
  c_uimm8s4, c_pcrel8s4, c_lppcrel10, c_pcrel10, c_pcrel12, c_imm16s4,
  c_luimm16, c_imm16, c_imm16d, c_huimm16, c_rimm16, c_imm16s2, c_uimm16s4,
  c_uimm16s4d, c_uimm16, c_pcrel24, c_uimm32, c_imm32, c_huimm32, c_huimm32e,
} const_forms_t;

static const struct
{
  const char *name;
  const int nbits;
  const char reloc;
  const char issigned;
  const char pcrel;
  const char scale;
  const char offset;
  const char negative;
  const char positive;
  const char decimal;
  const char leading;
  const char exact;
} constant_formats[] =
{
  { "0",          0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  { "1",          0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  { "4",          0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  { "2",          0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  { "uimm2",      2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { "uimm3",      3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { "imm3",       3, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  { "pcrel4",     4, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0},
  { "imm4",       4, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  { "uimm4s4",    4, 0, 0, 0, 2, 0, 0, 1, 0, 0, 0},
  { "uimm4s4d",   4, 0, 0, 0, 2, 0, 0, 1, 1, 0, 0},
  { "uimm4",      4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { "uimm4s2",    4, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
  { "negimm5s4",  5, 0, 1, 0, 2, 0, 1, 0, 0, 0, 0},
  { "imm5",       5, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  { "imm5d",      5, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0},
  { "uimm5",      5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { "imm6",       6, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  { "imm7",       7, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  { "imm7d",      7, 0, 1, 0, 0, 0, 0, 0, 1, 3, 0},
  { "imm8",       8, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  { "uimm8",      8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { "pcrel8",     8, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0},
  { "uimm8s4",    8, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0},
  { "pcrel8s4",   8, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0},
  { "lppcrel10", 10, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0},
  { "pcrel10",   10, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
  { "pcrel12",   12, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
  { "imm16s4",   16, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0},
  { "luimm16",   16, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { "imm16",     16, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  { "imm16d",    16, 0, 1, 0, 0, 0, 0, 0, 1, 3, 0},
  { "huimm16",   16, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { "rimm16",    16, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  { "imm16s2",   16, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0},
  { "uimm16s4",  16, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0},
  { "uimm16s4d", 16, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0},
  { "uimm16",    16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { "pcrel24",   24, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
  { "uimm32",    32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { "imm32",     32, 0, 1, 0, 0, 0, 0, 0, 1, 3, 0},
  { "huimm32",   32, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { "huimm32e",  32, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
};

static const char *
fmtconst_str (const_forms_t cf, bs32 x, bu32 pc)
{
  static char buf[60];

  if (constant_formats[cf].reloc)
    {
#if 0
      bu32 ea = (((constant_formats[cf].pcrel ? SIGNEXTEND (x, constant_formats[cf].nbits)
		      : x) + constant_formats[cf].offset) << constant_formats[cf].scale);
      if (constant_formats[cf].pcrel)
	ea += pc;
      if (outf->symbol_at_address_func (ea, outf) || !constant_formats[cf].exact)
       {
	  outf->print_address_func (ea, outf);
	  return "";
       }
      else
#endif
       {
	  sprintf (buf, "%#x", x);
	  return buf;
       }
    }

  /* Negative constants have an implied sign bit.  */
  if (constant_formats[cf].negative)
    {
      int nb = constant_formats[cf].nbits + 1;

      x = x | (1 << constant_formats[cf].nbits);
      x = SIGNEXTEND (x, nb);
    }
  else
    x = constant_formats[cf].issigned ? SIGNEXTEND (x, constant_formats[cf].nbits) : x;

  if (constant_formats[cf].offset)
    x += constant_formats[cf].offset;

  if (constant_formats[cf].scale)
    x <<= constant_formats[cf].scale;

  if (constant_formats[cf].decimal)
    sprintf (buf, "%*i", constant_formats[cf].leading, x);
  else
    {
      if (constant_formats[cf].issigned && x < 0)
	sprintf (buf, "-0x%x", abs (x));
      else
	sprintf (buf, "0x%x", x);
    }

  return buf;
}

static bu32
fmtconst_val (const_forms_t cf, bu32 x, bu32 pc)
{
  if (0 && constant_formats[cf].reloc)
    {
      bu32 ea = (((constant_formats[cf].pcrel
		   ? (bu32)SIGNEXTEND (x, constant_formats[cf].nbits)
		   : x) + constant_formats[cf].offset)
		 << constant_formats[cf].scale);
      if (constant_formats[cf].pcrel)
	ea += pc;

      return ea;
    }

  /* Negative constants have an implied sign bit.  */
  if (constant_formats[cf].negative)
    {
      int nb = constant_formats[cf].nbits + 1;
      x = x | (1 << constant_formats[cf].nbits);
      x = SIGNEXTEND (x, nb);
    }
  else if (constant_formats[cf].issigned)
    x = SIGNEXTEND (x, constant_formats[cf].nbits);

  x += constant_formats[cf].offset;
  x <<= constant_formats[cf].scale;

  return x;
}

#define uimm16s4(x)	fmtconst_val (c_uimm16s4, x, 0)
#define uimm16s4_str(x)	fmtconst_str (c_uimm16s4, x, 0)
#define uimm16s4d(x)	fmtconst_val (c_uimm16s4d, x, 0)
#define pcrel4(x)	fmtconst_val (c_pcrel4, x, pc)
#define pcrel8(x)	fmtconst_val (c_pcrel8, x, pc)
#define pcrel8s4(x)	fmtconst_val (c_pcrel8s4, x, pc)
#define pcrel10(x)	fmtconst_val (c_pcrel10, x, pc)
#define pcrel12(x)	fmtconst_val (c_pcrel12, x, pc)
#define negimm5s4(x)	fmtconst_val (c_negimm5s4, x, 0)
#define negimm5s4_str(x)	fmtconst_str (c_negimm5s4, x, 0)
#define rimm16(x)	fmtconst_val (c_rimm16, x, 0)
#define huimm16(x)	fmtconst_val (c_huimm16, x, 0)
#define imm16(x)	fmtconst_val (c_imm16, x, 0)
#define imm16_str(x)	fmtconst_str (c_imm16, x, 0)
#define imm16d(x)	fmtconst_val (c_imm16d, x, 0)
#define uimm2(x)	fmtconst_val (c_uimm2, x, 0)
#define uimm3(x)	fmtconst_val (c_uimm3, x, 0)
#define uimm3_str(x)	fmtconst_str (c_uimm3, x, 0)
#define luimm16(x)	fmtconst_val (c_luimm16, x, 0)
#define luimm16_str(x)	fmtconst_str (c_luimm16, x, 0)
#define uimm4(x)	fmtconst_val (c_uimm4, x, 0)
#define uimm4_str(x)	fmtconst_str (c_uimm4, x, 0)
#define uimm5(x)	fmtconst_val (c_uimm5, x, 0)
#define uimm5_str(x)	fmtconst_str (c_uimm5, x, 0)
#define imm16s2(x)	fmtconst_val (c_imm16s2, x, 0)
#define imm16s2_str(x)	fmtconst_str (c_imm16s2, x, 0)
#define uimm8(x)	fmtconst_val (c_uimm8, x, 0)
#define imm16s4(x)	fmtconst_val (c_imm16s4, x, 0)
#define imm16s4_str(x)	fmtconst_str (c_imm16s4, x, 0)
#define uimm4s2(x)	fmtconst_val (c_uimm4s2, x, 0)
#define uimm4s2_str(x)	fmtconst_str (c_uimm4s2, x, 0)
#define uimm4s4(x)	fmtconst_val (c_uimm4s4, x, 0)
#define uimm4s4_str(x)	fmtconst_str (c_uimm4s4, x, 0)
#define uimm4s4d(x)	fmtconst_val (c_uimm4s4d, x, 0)
#define lppcrel10(x)	fmtconst_val (c_lppcrel10, x, pc)
#define imm3(x)		fmtconst_val (c_imm3, x, 0)
#define imm3_str(x)	fmtconst_str (c_imm3, x, 0)
#define imm4(x)		fmtconst_val (c_imm4, x, 0)
#define uimm8s4(x)	fmtconst_val (c_uimm8s4, x, 0)
#define imm5(x)		fmtconst_val (c_imm5, x, 0)
#define imm5d(x)	fmtconst_val (c_imm5d, x, 0)
#define imm6(x)		fmtconst_val (c_imm6, x, 0)
#define imm7(x)		fmtconst_val (c_imm7, x, 0)
#define imm7_str(x)	fmtconst_str (c_imm7, x, 0)
#define imm7d(x)	fmtconst_val (c_imm7d, x, 0)
#define imm8(x)		fmtconst_val (c_imm8, x, 0)
#define pcrel24(x)	fmtconst_val (c_pcrel24, x, pc)
#define pcrel24_str(x)	fmtconst_str (c_pcrel24, x, pc)
#define uimm16(x)	fmtconst_val (c_uimm16, x, 0)
#define uimm32(x)	fmtconst_val (c_uimm32, x, 0)
#define imm32(x)	fmtconst_val (c_imm32, x, 0)
#define huimm32(x)	fmtconst_val (c_huimm32, x, 0)
#define huimm32e(x)	fmtconst_val (c_huimm32e, x, 0)

/* Table C-4. Core Register Encoding Map.  */
const char * const greg_names[] =
{
  "R0",    "R1",      "R2",     "R3",    "R4",    "R5",    "R6",     "R7",
  "P0",    "P1",      "P2",     "P3",    "P4",    "P5",    "SP",     "FP",
  "I0",    "I1",      "I2",     "I3",    "M0",    "M1",    "M2",     "M3",
  "B0",    "B1",      "B2",     "B3",    "L0",    "L1",    "L2",     "L3",
  "A0.X",  "A0.W",    "A1.X",   "A1.W",  "<res>", "<res>", "ASTAT",  "RETS",
  "<res>", "<res>",   "<res>",  "<res>", "<res>", "<res>", "<res>",  "<res>",
  "LC0",   "LT0",     "LB0",    "LC1",   "LT1",   "LB1",   "CYCLES", "CYCLES2",
  "USP",   "SEQSTAT", "SYSCFG", "RETI",  "RETX",  "RETN",  "RETE",   "EMUDAT",
};
static const char *
get_allreg_name (int grp, int reg)
{
  return greg_names[(grp << 3) | reg];
}
static const char *
get_preg_name (int reg)
{
  return get_allreg_name (1, reg);
}

static bool
reg_is_reserved (int grp, int reg)
{
  return (grp == 4 && (reg == 4 || reg == 5)) || (grp == 5);
}

static bu32 *
get_allreg (SIM_CPU *cpu, int grp, int reg)
{
  int fullreg = (grp << 3) | reg;
  /* REG_R0, REG_R1, REG_R2, REG_R3, REG_R4, REG_R5, REG_R6, REG_R7,
     REG_P0, REG_P1, REG_P2, REG_P3, REG_P4, REG_P5, REG_SP, REG_FP,
     REG_I0, REG_I1, REG_I2, REG_I3, REG_M0, REG_M1, REG_M2, REG_M3,
     REG_B0, REG_B1, REG_B2, REG_B3, REG_L0, REG_L1, REG_L2, REG_L3,
     REG_A0x, REG_A0w, REG_A1x, REG_A1w, , , REG_ASTAT, REG_RETS,
     , , , , , , , ,
     REG_LC0, REG_LT0, REG_LB0, REG_LC1, REG_LT1, REG_LB1, REG_CYCLES,
     REG_CYCLES2,
     REG_USP, REG_SEQSTAT, REG_SYSCFG, REG_RETI, REG_RETX, REG_RETN, REG_RETE,
     REG_LASTREG  */
  switch (fullreg >> 2)
    {
    case 0: case 1: return &DREG (reg);
    case 2: case 3: return &PREG (reg);
    case 4: return &IREG (reg & 3);
    case 5: return &MREG (reg & 3);
    case 6: return &BREG (reg & 3);
    case 7: return &LREG (reg & 3);
    default:
      switch (fullreg)
	{
	case 32: return &AXREG (0);
	case 33: return &AWREG (0);
	case 34: return &AXREG (1);
	case 35: return &AWREG (1);
	case 39: return &RETSREG;
	case 48: return &LCREG (0);
	case 49: return &LTREG (0);
	case 50: return &LBREG (0);
	case 51: return &LCREG (1);
	case 52: return &LTREG (1);
	case 53: return &LBREG (1);
	case 54: return &CYCLESREG;
	case 55: return &CYCLES2REG;
	case 56: return &USPREG;
	case 57: return &SEQSTATREG;
	case 58: return &SYSCFGREG;
	case 59: return &RETIREG;
	case 60: return &RETXREG;
	case 61: return &RETNREG;
	case 62: return &RETEREG;
	case 63: return &EMUDAT_INREG;
	}
      illegal_instruction (cpu);
    }
}

static const char *
amod0 (int s0, int x0)
{
  static const char * const mod0[] = {
    "", " (S)", " (CO)", " (SCO)",
  };
  int i = s0 + (x0 << 1);

  if (i < ARRAY_SIZE (mod0))
    return mod0[i];
  else
    return "";
}

static const char *
amod0amod2 (int s0, int x0, int aop0)
{
  static const char * const mod02[] = {
    "", " (S)", " (CO)", " (SCO)",
    "", "", "", "",
    " (ASR)", " (S, ASR)", " (CO, ASR)", " (SCO, ASR)",
    " (ASL)", " (S, ASL)", " (CO, ASL)", " (SCO, ASL)",
  };
  int i = s0 + (x0 << 1) + (aop0 << 2);

  if (i < ARRAY_SIZE (mod02))
    return mod02[i];
  else
    return "";
}

static const char *
amod1 (int s0, int x0)
{
  static const char * const mod1[] = {
    " (NS)", " (S)",
  };
  int i = s0 + (x0 << 1);

  if (i < ARRAY_SIZE (mod1))
    return mod1[i];
  else
    return "";
}

static const char *
mac_optmode (int mmod, int MM)
{
  static const char * const omode[] = {
    [(M_S2RND << 1) + 0] = " (S2RND)",
    [(M_T     << 1) + 0] = " (T)",
    [(M_W32   << 1) + 0] = " (W32)",
    [(M_FU    << 1) + 0] = " (FU)",
    [(M_TFU   << 1) + 0] = " (TFU)",
    [(M_IS    << 1) + 0] = " (IS)",
    [(M_ISS2  << 1) + 0] = " (ISS2)",
    [(M_IH    << 1) + 0] = " (IH)",
    [(M_IU    << 1) + 0] = " (IU)",
    [(M_S2RND << 1) + 1] = " (M, S2RND)",
    [(M_T     << 1) + 1] = " (M, T)",
    [(M_W32   << 1) + 1] = " (M, W32)",
    [(M_FU    << 1) + 1] = " (M, FU)",
    [(M_TFU   << 1) + 1] = " (M, TFU)",
    [(M_IS    << 1) + 1] = " (M, IS)",
    [(M_ISS2  << 1) + 1] = " (M, ISS2)",
    [(M_IH    << 1) + 1] = " (M, IH)",
    [(M_IU    << 1) + 1] = " (M, IU)",
  };
  int i = MM + (mmod << 1);

  if (i < ARRAY_SIZE (omode) && omode[i])
    return omode[i];
  else
    return "";
}

static const char *
get_store_name (SIM_CPU *cpu, bu32 *p)
{
  if (p >= &DREG (0) && p <= &CYCLESREG)
    return greg_names[p - &DREG (0)];
  else if (p == &AXREG (0))
    return greg_names[4 * 8 + 0];
  else if (p == &AWREG (0))
    return greg_names[4 * 8 + 1];
  else if (p == &AXREG (1))
    return greg_names[4 * 8 + 2];
  else if (p == &AWREG (1))
    return greg_names[4 * 8 + 3];
  else if (p == &ASTATREG (ac0))
    return "ASTAT[ac0]";
  else if (p == &ASTATREG (ac0_copy))
    return "ASTAT[ac0_copy]";
  else if (p == &ASTATREG (ac1))
    return "ASTAT[ac1]";
  else if (p == &ASTATREG (an))
    return "ASTAT[an]";
  else if (p == &ASTATREG (aq))
    return "ASTAT[aq]";
  else if (p == &ASTATREG (av0))
    return "ASTAT[av0]";
  else if (p == &ASTATREG (av0s))
    return "ASTAT[av0s]";
  else if (p == &ASTATREG (av1))
    return "ASTAT[av1]";
  else if (p == &ASTATREG (av1s))
    return "ASTAT[av1s]";
  else if (p == &ASTATREG (az))
    return "ASTAT[az]";
  else if (p == &ASTATREG (v))
    return "ASTAT[v]";
  else if (p == &ASTATREG (v_copy))
    return "ASTAT[v_copy]";
  else if (p == &ASTATREG (vs))
    return "ASTAT[vs]";
  else
    {
      /* Worry about this when we start to STORE() it.  */
      sim_io_eprintf (CPU_STATE (cpu), "STORE(): unknown register\n");
      abort ();
    }
}

static void
queue_store (SIM_CPU *cpu, bu32 *addr, bu32 val)
{
  struct store *s = &BFIN_CPU_STATE.stores[BFIN_CPU_STATE.n_stores];
  s->addr = addr;
  s->val = val;
  TRACE_REGISTER (cpu, "queuing write %s = %#x",
		  get_store_name (cpu, addr), val);
  ++BFIN_CPU_STATE.n_stores;
}
#define STORE(X, Y) \
  do { \
    if (BFIN_CPU_STATE.n_stores == 20) abort (); \
    queue_store (cpu, &(X), (Y)); \
  } while (0)

static void
setflags_nz (SIM_CPU *cpu, bu32 val)
{
  SET_ASTATREG (az, val == 0);
  SET_ASTATREG (an, val >> 31);
}

static void
setflags_nz_2x16 (SIM_CPU *cpu, bu32 val)
{
  SET_ASTATREG (an, (bs16)val < 0 || (bs16)(val >> 16) < 0);
  SET_ASTATREG (az, (bs16)val == 0 || (bs16)(val >> 16) == 0);
}

static void
setflags_logical (SIM_CPU *cpu, bu32 val)
{
  setflags_nz (cpu, val);
  SET_ASTATREG (ac0, 0);
  SET_ASTATREG (v, 0);
}

static bu32
add_brev (bu32 addend1, bu32 addend2)
{
  bu32 mask, b, r;
  int i, cy;

  mask = 0x80000000;
  r = 0;
  cy = 0;

  for (i = 31; i >= 0; --i)
    {
      b = ((addend1 & mask) >> i) + ((addend2 & mask) >> i);
      b += cy;
      cy = b >> 1;
      b &= 1;
      r |= b << i;
      mask >>= 1;
    }

  return r;
}

/* This is a bit crazy, but we want to simulate the hardware behavior exactly
   rather than worry about the circular buffers being used correctly.  Which
   isn't to say there isn't room for improvement here, just that we want to
   be conservative.  See also dagsub().  */
static bu32
dagadd (SIM_CPU *cpu, int dagno, bs32 M)
{
  bu64 i = IREG (dagno);
  bu64 l = LREG (dagno);
  bu64 b = BREG (dagno);
  bu64 m = (bu32)M;

  bu64 LB, IM, IML;
  bu32 im32, iml32, lb32, res;
  bu64 msb, car;

  /* A naïve implementation that mostly works:
  res = i + m;
  if (l && res >= b + l)
    res -= l;
  STORE (IREG (dagno), res);
   */

  msb = (bu64)1 << 31;
  car = (bu64)1 << 32;

  IM = i + m;
  im32 = IM;
  LB = l + b;
  lb32 = LB;

  if (M < 0)
    {
      IML = i + m + l;
      iml32 = IML;
      if ((i & msb) || (IM & car))
	res = (im32 < b) ? iml32 : im32;
      else
	res = (im32 < b) ? im32 : iml32;
    }
  else
    {
      IML = i + m - l;
      iml32 = IML;
      if ((IM & car) == (LB & car))
	res = (im32 < lb32) ? im32 : iml32;
      else
	res = (im32 < lb32) ? iml32 : im32;
    }

  STORE (IREG (dagno), res);
  return res;
}

/* See dagadd() notes above.  */
static bu32
dagsub (SIM_CPU *cpu, int dagno, bs32 M)
{
  bu64 i = IREG (dagno);
  bu64 l = LREG (dagno);
  bu64 b = BREG (dagno);
  bu64 m = (bu32)M;

  bu64 mbar = (bu32)(~m + 1);
  bu64 LB, IM, IML;
  bu32 b32, im32, iml32, lb32, res;
  bu64 msb, car;

  /* A naïve implementation that mostly works:
  res = i - m;
  if (l && newi < b)
    newi += l;
  STORE (IREG (dagno), newi);
   */

  msb = (bu64)1 << 31;
  car = (bu64)1 << 32;

  IM = i + mbar;
  im32 = IM;
  LB = l + b;
  lb32 = LB;

  if (M < 0)
    {
      IML = i + mbar - l;
      iml32 = IML;
      if (!!((i & msb) && (IM & car)) == !!(LB & car))
	res = (im32 < lb32) ? im32 : iml32;
      else
	res = (im32 < lb32) ? iml32 : im32;
    }
  else
    {
      IML = i + mbar + l;
      iml32 = IML;
      b32 = b;
      if (M == 0 || IM & car)
	res = (im32 < b32) ? iml32 : im32;
      else
	res = (im32 < b32) ? im32 : iml32;
    }

  STORE (IREG (dagno), res);
  return res;
}

static bu40
ashiftrt (SIM_CPU *cpu, bu40 val, int cnt, int size)
{
  int real_cnt = cnt > size ? size : cnt;
  bu40 sgn = ~(((val & 0xFFFFFFFFFFull) >> (size - 1)) - 1);
  int sgncnt = size - real_cnt;
  if (sgncnt > 16)
    sgn <<= 16, sgncnt -= 16;
  sgn <<= sgncnt;
  if (real_cnt > 16)
    val >>= 16, real_cnt -= 16;
  val >>= real_cnt;
  val |= sgn;
  SET_ASTATREG (an, val >> (size - 1));
  SET_ASTATREG (az, val == 0);
  if (size != 40)
    SET_ASTATREG (v, 0);
  return val;
}

static bu64
lshiftrt (SIM_CPU *cpu, bu64 val, int cnt, int size)
{
  int real_cnt = cnt > size ? size : cnt;
  if (real_cnt > 16)
    val >>= 16, real_cnt -= 16;
  val >>= real_cnt;
  switch (size)
    {
    case 16:
      val &= 0xFFFF;
      break;
    case 32:
      val &= 0xFFFFFFFF;
      break;
    case 40:
      val &= 0xFFFFFFFFFFull;
      break;
    default:
      illegal_instruction (cpu);
      break;
    }
  SET_ASTATREG (an, val >> (size - 1));
  SET_ASTATREG (az, val == 0);
  if (size != 40)
    SET_ASTATREG (v, 0);
  return val;
}

static bu64
lshift (SIM_CPU *cpu, bu64 val, int cnt, int size, bool saturate, bool overflow)
{
  int v_i, real_cnt = cnt > size ? size : cnt;
  bu64 sgn = ~((val >> (size - 1)) - 1);
  int mask_cnt = size - 1;
  bu64 masked, new_val = val;
  bu64 mask = ~0;

  mask <<= mask_cnt;
  sgn <<= mask_cnt;
  masked = val & mask;

  if (real_cnt > 16)
    new_val <<= 16, real_cnt -= 16;

  new_val <<= real_cnt;

  masked = new_val & mask;

  /* If an operation would otherwise cause a positive value to overflow
     and become negative, instead, saturation limits the result to the
     maximum positive value for the size register being used.

     Conversely, if an operation would otherwise cause a negative value
     to overflow and become positive, saturation limits the result to the
     maximum negative value for the register size.

     However, it's a little more complex than looking at sign bits, we need
     to see if we are shifting the sign information away...  */
  if (((val << cnt) >> size) == 0
      || (((val << cnt) >> size) == ~((bu32)~0 << cnt)
	  && ((new_val >> (size - 1)) & 0x1)))
    v_i = 0;
  else
    v_i = 1;

  switch (size)
    {
    case 16:
      new_val &= 0xFFFF;
      if (saturate && (v_i || ((val >> (size - 1)) != (new_val >> (size - 1)))))
	{
	  new_val = (val >> (size - 1)) == 0 ? 0x7fff : 0x8000;
	  v_i = 1;
	}
      break;
    case 32:
      new_val &= 0xFFFFFFFF;
      masked &= 0xFFFFFFFF;
      sgn &= 0xFFFFFFFF;
      if (saturate
	  && (v_i
	      || (sgn != masked)
	      || (!sgn && new_val == 0 && val != 0)))
	{
	  new_val = sgn == 0 ? 0x7fffffff : 0x80000000;
	  v_i = 1;
	}
      break;
    case 40:
      new_val &= 0xFFFFFFFFFFull;
      masked &= 0xFFFFFFFFFFull;
      break;
    default:
      illegal_instruction (cpu);
      break;
    }

  SET_ASTATREG (an, new_val >> (size - 1));
  SET_ASTATREG (az, new_val == 0);
  if (size != 40)
    {
      SET_ASTATREG (v, overflow && v_i);
      if (overflow && v_i)
	SET_ASTATREG (vs, 1);
    }

  return new_val;
}

static bu32
algn (bu32 l, bu32 h, bu32 aln)
{
  if (aln == 0)
    return l;
  else
    return (l >> (8 * aln)) | (h << (32 - 8 * aln));
}

static bu32
saturate_s16 (bu64 val, bu32 *overflow)
{
  if ((bs64)val < -0x8000ll)
    {
      if (overflow)
	*overflow = 1;
      return 0x8000;
    }
  if ((bs64)val > 0x7fff)
    {
      if (overflow)
	*overflow = 1;
      return 0x7fff;
    }
  return val & 0xffff;
}

static bu40
rot40 (bu40 val, int shift, bu32 *cc)
{
  const int nbits = 40;
  bu40 ret;

  shift = CLAMP (shift, -nbits, nbits);
  if (shift == 0)
    return val;

  /* Reduce everything to rotate left.  */
  if (shift < 0)
    shift += nbits + 1;

  ret = shift == nbits ? 0 : val << shift;
  ret |= shift == 1 ? 0 : val >> ((nbits + 1) - shift);
  ret |= (bu40)*cc << (shift - 1);
  *cc = (val >> (nbits - shift)) & 1;

  return ret;
}

static bu32
rot32 (bu32 val, int shift, bu32 *cc)
{
  const int nbits = 32;
  bu32 ret;

  shift = CLAMP (shift, -nbits, nbits);
  if (shift == 0)
    return val;

  /* Reduce everything to rotate left.  */
  if (shift < 0)
    shift += nbits + 1;

  ret = shift == nbits ? 0 : val << shift;
  ret |= shift == 1 ? 0 : val >> ((nbits + 1) - shift);
  ret |= (bu32)*cc << (shift - 1);
  *cc = (val >> (nbits - shift)) & 1;

  return ret;
}

static bu32
add32 (SIM_CPU *cpu, bu32 a, bu32 b, int carry, int sat)
{
  int flgs = (a >> 31) & 1;
  int flgo = (b >> 31) & 1;
  bu32 v = a + b;
  int flgn = (v >> 31) & 1;
  int overflow = (flgs ^ flgn) & (flgo ^ flgn);

  if (sat && overflow)
    {
      v = (bu32)1 << 31;
      if (flgn)
	v -= 1;
      flgn = (v >> 31) & 1;
    }

  SET_ASTATREG (an, flgn);
  if (overflow)
    SET_ASTATREG (vs, 1);
  SET_ASTATREG (v, overflow);
  ASTATREG (v_internal) |= overflow;
  SET_ASTATREG (az, v == 0);
  if (carry)
    SET_ASTATREG (ac0, ~a < b);

  return v;
}

static bu32
sub32 (SIM_CPU *cpu, bu32 a, bu32 b, int carry, int sat, int parallel)
{
  int flgs = (a >> 31) & 1;
  int flgo = (b >> 31) & 1;
  bu32 v = a - b;
  int flgn = (v >> 31) & 1;
  int overflow = (flgs ^ flgo) & (flgn ^ flgs);

  if (sat && overflow)
    {
      v = (bu32)1 << 31;
      if (flgn)
	v -= 1;
      flgn = (v >> 31) & 1;
    }

  if (!parallel || flgn)
    SET_ASTATREG (an, flgn);
  if (overflow)
    SET_ASTATREG (vs, 1);
  if (!parallel || overflow)
    SET_ASTATREG (v, overflow);
  if (!parallel || overflow)
    ASTATREG (v_internal) |= overflow;
  if (!parallel || v == 0)
    SET_ASTATREG (az, v == 0);
  if (carry && (!parallel || b <= a))
    SET_ASTATREG (ac0, b <= a);

  return v;
}

static bu32
add16 (SIM_CPU *cpu, bu16 a, bu16 b, bu32 *carry, bu32 *overfl,
       bu32 *zero, bu32 *neg, int sat, int scale)
{
  int flgs = (a >> 15) & 1;
  int flgo = (b >> 15) & 1;
  bs64 v = (bs16)a + (bs16)b;
  int flgn = (v >> 15) & 1;
  int overflow = (flgs ^ flgn) & (flgo ^ flgn);

  switch (scale)
    {
    case 0:
      break;
    case 2:
      /* (ASR)  */
      v = (a >> 1) + (a & 0x8000) + (b >> 1) + (b & 0x8000)
	  + (((a & 1) + (b & 1)) >> 1);
      v |= -(v & 0x8000);
      break;
    case 3:
      /* (ASL)  */
      v = (v << 1);
      break;
    default:
      illegal_instruction (cpu);
    }

  flgn = (v >> 15) & 1;
  overflow = (flgs ^ flgn) & (flgo ^ flgn);

  if (v > (bs64)0xffff)
    overflow = 1;

  if (sat)
    v = saturate_s16 (v, 0);

  if (neg)
    *neg |= (v >> 15) & 1;
  if (overfl)
    *overfl |= overflow;
  if (zero)
    *zero |= (v & 0xFFFF) == 0;
  if (carry)
      *carry |= ((bu16)~a < (bu16)b);

  return v & 0xffff;
}

static bu32
sub16 (SIM_CPU *cpu, bu16 a, bu16 b, bu32 *carry, bu32 *overfl,
       bu32 *zero, bu32 *neg, int sat, int scale)
{
  int flgs = (a >> 15) & 1;
  int flgo = (b >> 15) & 1;
  bs64 v = (bs16)a - (bs16)b;
  int flgn = (v >> 15) & 1;
  int overflow = (flgs ^ flgo) & (flgn ^ flgs);

  switch (scale)
    {
    case 0:
      break;
    case 2:
      /* (ASR)  */
      if (sat)
	v = ((a >> 1) + (a & 0x8000)) - ( (b >> 1) + (b & 0x8000))
	    + (((a & 1)-(b & 1)));
      else
	{
	  v = ((v & 0xFFFF) >> 1);
	  if ((!flgs & !flgo & flgn)
	      || (flgs & !flgo & !flgn)
	      || (flgs & flgo & flgn)
	      || (flgs & !flgo & flgn))
	    v |= 0x8000;
	}
      v |= -(v & 0x8000);
      flgn = (v >> 15) & 1;
      overflow = (flgs ^ flgo) & (flgn ^ flgs);
      break;
    case 3:
      /* (ASL)  */
      v <<= 1;
      if (v > (bs64)0x7fff || v < (bs64)-0xffff)
	overflow = 1;
      break;
    default:
      illegal_instruction (cpu);
    }

  if (sat)
    {
      v = saturate_s16 (v, 0);
    }
  if (neg)
    *neg |= (v >> 15) & 1;
  if (zero)
    *zero |= (v & 0xFFFF) == 0;
  if (overfl)
    *overfl |= overflow;
  if (carry)
    *carry |= (bu16)b <= (bu16)a;
  return v;
}

static bu32
min32 (SIM_CPU *cpu, bu32 a, bu32 b)
{
  int val = a;
  if ((bs32)a > (bs32)b)
    val = b;
  setflags_nz (cpu, val);
  SET_ASTATREG (v, 0);
  return val;
}

static bu32
max32 (SIM_CPU *cpu, bu32 a, bu32 b)
{
  int val = a;
  if ((bs32)a < (bs32)b)
    val = b;
  setflags_nz (cpu, val);
  SET_ASTATREG (v, 0);
  return val;
}

static bu32
min2x16 (SIM_CPU *cpu, bu32 a, bu32 b)
{
  int val = a;
  if ((bs16)a > (bs16)b)
    val = (val & 0xFFFF0000) | (b & 0xFFFF);
  if ((bs16)(a >> 16) > (bs16)(b >> 16))
    val = (val & 0xFFFF) | (b & 0xFFFF0000);
  setflags_nz_2x16 (cpu, val);
  SET_ASTATREG (v, 0);
  return val;
}

static bu32
max2x16 (SIM_CPU *cpu, bu32 a, bu32 b)
{
  int val = a;
  if ((bs16)a < (bs16)b)
    val = (val & 0xFFFF0000) | (b & 0xFFFF);
  if ((bs16)(a >> 16) < (bs16)(b >> 16))
    val = (val & 0xFFFF) | (b & 0xFFFF0000);
  setflags_nz_2x16 (cpu, val);
  SET_ASTATREG (v, 0);
  return val;
}

static bu32
add_and_shift (SIM_CPU *cpu, bu32 a, bu32 b, int shift)
{
  int v;
  ASTATREG (v_internal) = 0;
  v = add32 (cpu, a, b, 0, 0);
  while (shift-- > 0)
    {
      int x = (v >> 30) & 0x3;
      if (x == 1 || x == 2)
	ASTATREG (v_internal) = 1;
      v <<= 1;
    }
  SET_ASTATREG (az, v == 0);
  SET_ASTATREG (an, v & 0x80000000);
  SET_ASTATREG (v, ASTATREG (v_internal));
  if (ASTATREG (v))
    SET_ASTATREG (vs, 1);
  return v;
}

static bu32
xor_reduce (bu64 acc0, bu64 acc1)
{
  int i;
  bu32 v = 0;
  for (i = 0; i < 40; ++i)
    {
      v ^= (acc0 & acc1 & 1);
      acc0 >>= 1;
      acc1 >>= 1;
    }
  return v;
}

/* DIVS ( Dreg, Dreg ) ;
   Initialize for DIVQ.  Set the AQ status bit based on the signs of
   the 32-bit dividend and the 16-bit divisor.  Left shift the dividend
   one bit.  Copy AQ into the dividend LSB.  */
static bu32
divs (SIM_CPU *cpu, bu32 pquo, bu16 divisor)
{
  bu16 r = pquo >> 16;
  int aq;

  aq = (r ^ divisor) >> 15;  /* Extract msb's and compute quotient bit.  */
  SET_ASTATREG (aq, aq);     /* Update global quotient state.  */

  pquo <<= 1;
  pquo |= aq;
  pquo = (pquo & 0x1FFFF) | (r << 17);
  return pquo;
}

/* DIVQ ( Dreg, Dreg ) ;
   Based on AQ status bit, either add or subtract the divisor from
   the dividend.  Then set the AQ status bit based on the MSBs of the
   32-bit dividend and the 16-bit divisor.  Left shift the dividend one
   bit.  Copy the logical inverse of AQ into the dividend LSB.  */
static bu32
divq (SIM_CPU *cpu, bu32 pquo, bu16 divisor)
{
  unsigned short af = pquo >> 16;
  unsigned short r;
  int aq;

  if (ASTATREG (aq))
    r = divisor + af;
  else
    r = af - divisor;

  aq = (r ^ divisor) >> 15;  /* Extract msb's and compute quotient bit.  */
  SET_ASTATREG (aq, aq);     /* Update global quotient state.  */

  pquo <<= 1;
  pquo |= !aq;
  pquo = (pquo & 0x1FFFF) | (r << 17);
  return pquo;
}

/* ONES ( Dreg ) ;
   Count the number of bits set to 1 in the 32bit value.  */
static bu32
ones (bu32 val)
{
  bu32 i;
  bu32 ret;

  ret = 0;
  for (i = 0; i < 32; ++i)
    ret += !!(val & (1 << i));

  return ret;
}

static void
reg_check_sup (SIM_CPU *cpu, int grp, int reg)
{
  if (grp == 7)
    cec_require_supervisor (cpu);
}

static void
reg_write (SIM_CPU *cpu, int grp, int reg, bu32 value)
{
  bu32 *whichreg;

  /* ASTAT is special!  */
  if (grp == 4 && reg == 6)
    {
      SET_ASTAT (value);
      return;
    }

  /* Check supervisor after get_allreg() so exception order is correct.  */
  whichreg = get_allreg (cpu, grp, reg);
  reg_check_sup (cpu, grp, reg);

  if (whichreg == &CYCLES2REG)
    /* Writes to CYCLES2 goes to the shadow.  */
    whichreg = &CYCLES2SHDREG;
  else if (whichreg == &SEQSTATREG)
    /* Register is read only -- discard writes.  */
    return;
  else if (whichreg == &EMUDAT_INREG)
    /* Writes to EMUDAT goes to the output.  */
    whichreg = &EMUDAT_OUTREG;
  else if (whichreg == &LTREG (0) || whichreg == &LTREG (1))
    /* Writes to LT clears LSB automatically.  */
    value &= ~0x1;
  else if (whichreg == &AXREG (0) || whichreg == &AXREG (1))
    value &= 0xFF;

  TRACE_REGISTER (cpu, "wrote %s = %#x", get_allreg_name (grp, reg), value);

  *whichreg = value;
}

static bu32
reg_read (SIM_CPU *cpu, int grp, int reg)
{
  bu32 *whichreg;
  bu32 value;

  /* ASTAT is special!  */
  if (grp == 4 && reg == 6)
    return ASTAT;

  /* Check supervisor after get_allreg() so exception order is correct.  */
  whichreg = get_allreg (cpu, grp, reg);
  reg_check_sup (cpu, grp, reg);

  value = *whichreg;

  if (whichreg == &CYCLESREG)
    /* Reads of CYCLES reloads CYCLES2 from the shadow.  */
    SET_CYCLES2REG (CYCLES2SHDREG);
  else if ((whichreg == &AXREG (1) || whichreg == &AXREG (0)) && (value & 0x80))
    /* Sign extend if necessary.  */
    value |= 0xFFFFFF00;

  return value;
}

static bu64
get_extended_cycles (SIM_CPU *cpu)
{
  return ((bu64)CYCLES2SHDREG << 32) | CYCLESREG;
}

/* We can't re-use sim_events_time() because the CYCLES registers may be
   written/cleared/reset/stopped/started at any time by software.  */
static void
cycles_inc (SIM_CPU *cpu, bu32 inc)
{
  bu64 cycles;
  bu32 cycles2;

  if (!(SYSCFGREG & SYSCFG_CCEN))
    return;

  cycles = get_extended_cycles (cpu) + inc;
  SET_CYCLESREG (cycles);
  cycles2 = cycles >> 32;
  if (CYCLES2SHDREG != cycles2)
    SET_CYCLES2SHDREG (cycles2);
}

static bu64
get_unextended_acc (SIM_CPU *cpu, int which)
{
  return ((bu64)(AXREG (which) & 0xff) << 32) | AWREG (which);
}

static bu64
get_extended_acc (SIM_CPU *cpu, int which)
{
  bu64 acc = AXREG (which);
  /* Sign extend accumulator values before adding.  */
  if (acc & 0x80)
    acc |= -0x80;
  else
    acc &= 0xFF;
  acc <<= 32;
  acc |= AWREG (which);
  return acc;
}

/* Perform a multiplication of D registers SRC0 and SRC1, sign- or
   zero-extending the result to 64 bit.  H0 and H1 determine whether the
   high part or the low part of the source registers is used.  Store 1 in
   *PSAT if saturation occurs, 0 otherwise.  */
static bu64
decode_multfunc (SIM_CPU *cpu, int h0, int h1, int src0, int src1, int mmod,
		 int MM, bu32 *psat)
{
  bu32 s0 = DREG (src0), s1 = DREG (src1);
  bu32 sgn0, sgn1;
  bu32 val;
  bu64 val1;

  if (h0)
    s0 >>= 16;

  if (h1)
    s1 >>= 16;

  s0 &= 0xffff;
  s1 &= 0xffff;

  sgn0 = -(s0 & 0x8000);
  sgn1 = -(s1 & 0x8000);

  if (MM)
    s0 |= sgn0;
  else
    switch (mmod)
      {
      case 0:
      case M_S2RND:
      case M_T:
      case M_IS:
      case M_ISS2:
      case M_IH:
      case M_W32:
	s0 |= sgn0;
	s1 |= sgn1;
	break;
      case M_FU:
      case M_IU:
      case M_TFU:
	break;
      default:
	illegal_instruction (cpu);
      }

  val = s0 * s1;
  /* Perform shift correction if appropriate for the mode.  */
  *psat = 0;
  if (!MM && (mmod == 0 || mmod == M_T || mmod == M_S2RND || mmod == M_W32))
    {
      if (val == 0x40000000)
	{
	  if (mmod == M_W32)
	    val = 0x7fffffff;
	  else
	    val = 0x80000000;
	  *psat = 1;
	}
      else
	val <<= 1;
    }
  val1 = val;

  /* In signed modes, sign extend.  */
  if (is_macmod_signed (mmod) || MM)
    val1 |= -(val1 & 0x80000000);

  if (*psat)
    val1 &= 0xFFFFFFFFull;

  return val1;
}

static bu40
saturate_s40_astat (bu64 val, bu32 *v)
{
  if ((bs64)val < -((bs64)1 << 39))
    {
      *v = 1;
      return -((bs64)1 << 39);
    }
  else if ((bs64)val > ((bs64)1 << 39) - 1)
    {
      *v = 1;
      return ((bu64)1 << 39) - 1;
    }
  *v = 0; /* No overflow.  */
  return val;
}

static bu40
saturate_s40 (bu64 val)
{
  bu32 v;
  return saturate_s40_astat (val, &v);
}

static bu32
saturate_s32 (bu64 val, bu32 *overflow)
{
  if ((bs64)val < -0x80000000ll)
    {
      if (overflow)
	*overflow = 1;
      return 0x80000000;
    }
  if ((bs64)val > 0x7fffffff)
    {
      if (overflow)
	*overflow = 1;
      return 0x7fffffff;
    }
  return val;
}

static bu32
saturate_u32 (bu64 val, bu32 *overflow)
{
  if (val > 0xffffffff)
    {
      if (overflow)
	*overflow = 1;
      return 0xffffffff;
    }
  return val;
}

static bu32
saturate_u16 (bu64 val, bu32 *overflow)
{
  if (val > 0xffff)
    {
      if (overflow)
	*overflow = 1;
      return 0xffff;
    }
  return val;
}

static bu64
rnd16 (bu64 val)
{
  bu64 sgnbits;

  /* FIXME: Should honour rounding mode.  */
  if ((val & 0xffff) > 0x8000
      || ((val & 0xffff) == 0x8000 && (val & 0x10000)))
    val += 0x8000;

  sgnbits = val & 0xffff000000000000ull;
  val >>= 16;
  return val | sgnbits;
}

static bu64
trunc16 (bu64 val)
{
  bu64 sgnbits = val & 0xffff000000000000ull;
  val >>= 16;
  return val | sgnbits;
}

static int
signbits (bu64 val, int size)
{
  bu64 mask = (bu64)1 << (size - 1);
  bu64 bit = val & mask;
  int count = 0;
  for (;;)
    {
      mask >>= 1;
      bit >>= 1;
      if (mask == 0)
	break;
      if ((val & mask) != bit)
	break;
      count++;
    }
  if (size == 40)
    count -= 8;

  return count;
}

/* Extract a 16 or 32 bit value from a 64 bit multiplication result.
   These 64 bits must be sign- or zero-extended properly from the source
   we want to extract, either a 32 bit multiply or a 40 bit accumulator.  */

static bu32
extract_mult (SIM_CPU *cpu, bu64 res, int mmod, int MM,
	      int fullword, bu32 *overflow)
{
  if (fullword)
    switch (mmod)
      {
      case 0:
      case M_IS:
	return saturate_s32 (res, overflow);
      case M_IU:
	if (MM)
	  return saturate_s32 (res, overflow);
	return saturate_u32 (res, overflow);
      case M_FU:
	if (MM)
	  return saturate_s32 (res, overflow);
	return saturate_u32 (res, overflow);
      case M_S2RND:
      case M_ISS2:
	return saturate_s32 (res << 1, overflow);
      default:
	illegal_instruction (cpu);
      }
  else
    switch (mmod)
      {
      case 0:
      case M_W32:
      case M_IH:
	return saturate_s16 (rnd16 (res), overflow);
      case M_IS:
	return saturate_s16 (res, overflow);
      case M_FU:
	if (MM)
	  return saturate_s16 (rnd16 (res), overflow);
	return saturate_u16 (rnd16 (res), overflow);
      case M_IU:
	if (MM)
	  return saturate_s16 (res, overflow);
	return saturate_u16 (res, overflow);

      case M_T:
	return saturate_s16 (trunc16 (res), overflow);
      case M_TFU:
	if (MM)
	  return saturate_s16 (trunc16 (res), overflow);
	return saturate_u16 (trunc16 (res), overflow);

      case M_S2RND:
	return saturate_s16 (rnd16 (res << 1), overflow);
      case M_ISS2:
	return saturate_s16 (res << 1, overflow);
      default:
	illegal_instruction (cpu);
      }
}

static bu32
decode_macfunc (SIM_CPU *cpu, int which, int op, int h0, int h1, int src0,
		int src1, int mmod, int MM, int fullword, bu32 *overflow,
		bu32 *neg)
{
  bu64 acc;
  bu32 sat = 0, tsat, ret;

  /* Sign extend accumulator if necessary, otherwise unsigned.  */
  if (is_macmod_signed (mmod) || MM)
    acc = get_extended_acc (cpu, which);
  else
    acc = get_unextended_acc (cpu, which);

  if (op != 3)
    {
      /* TODO: Figure out how the 32-bit sign is used.  */
      ATTRIBUTE_UNUSED bu8 sgn0 = (acc >> 31) & 1;
      bu8 sgn40 = (acc >> 39) & 1;
      bu40 nosat_acc;

      /* This can't saturate, so we don't keep track of the sat flag.  */
      bu64 res = decode_multfunc (cpu, h0, h1, src0, src1, mmod,
				  MM, &tsat);

      /* Perform accumulation.  */
      switch (op)
	{
	case 0:
	  acc = res;
	  sgn0 = (acc >> 31) & 1;
	  break;
	case 1:
	  acc = acc + res;
	  break;
	case 2:
	  acc = acc - res;
	  break;
	}

      nosat_acc = acc;
      /* Saturate.  */
      switch (mmod)
	{
	case 0:
	case M_T:
	case M_IS:
	case M_ISS2:
	case M_S2RND:
	  if ((bs64)acc < -((bs64)1 << 39))
	    acc = -((bu64)1 << 39), sat = 1;
	  else if ((bs64)acc > 0x7fffffffffll)
	    acc = 0x7fffffffffull, sat = 1;
	  break;
	case M_TFU:
	  if (MM)
	    {
	      if ((bs64)acc < -((bs64)1 << 39))
		acc = -((bu64)1 << 39), sat = 1;
	      if ((bs64)acc > 0x7FFFFFFFFFll)
		acc = 0x7FFFFFFFFFull, sat = 1;
	    }
	  else
	    {
	      if ((bs64)acc < 0)
		acc = 0, sat = 1;
	      if ((bs64)acc > 0xFFFFFFFFFFull)
		acc = 0xFFFFFFFFFFull, sat = 1;
	    }
	  break;
	case M_IU:
	  if (!MM && acc & 0x8000000000000000ull)
	    acc = 0x0, sat = 1;
	  if (!MM && acc > 0xFFFFFFFFFFull)
	    acc = 0xFFFFFFFFFFull, sat = 1;
	  if (MM && acc > 0xFFFFFFFFFFull)
	    acc &= 0xFFFFFFFFFFull;
	  if (acc & 0x8000000000ull)
	    acc |= 0xffffff0000000000ull;
	  break;
	case M_FU:
	  if (MM)
	    {
	      if ((bs64)acc < -((bs64)1 << 39))
		acc = -((bu64)1 << 39), sat = 1;
	      if ((bs64)acc > 0x7FFFFFFFFFll)
		acc = 0x7FFFFFFFFFull, sat = 1;
	      else if (acc & 0x8000000000ull)
		acc |= 0xffffff0000000000ull;
	    }
	  else
	    {
	      if ((bs64)acc < 0)
		acc = 0x0, sat = 1;
	      else if ((bs64)acc > (bs64)0xFFFFFFFFFFll)
		acc = 0xFFFFFFFFFFull, sat = 1;
	    }
	  break;
	case M_IH:
	  if ((bs64)acc < -0x80000000ll)
	    acc = -0x80000000ull, sat = 1;
	  else if ((bs64)acc > 0x7fffffffll)
	    acc = 0x7fffffffull, sat = 1;
	  break;
	case M_W32:
	  /* check max negative value */
	  if (sgn40 && ((acc >> 31) != 0x1ffffffff)
	      && ((acc >> 31) != 0x0))
	    acc = 0x80000000, sat = 1;
	  if (!sat && !sgn40 && ((acc >> 31) != 0x0)
	      && ((acc >> 31) != 0x1ffffffff))
	    acc = 0x7FFFFFFF, sat = 1;
	  acc &= 0xffffffff;
	  if (acc & 0x80000000)
	    acc |= 0xffffffff00000000ull;
	  if (tsat)
	    sat = 1;
	  break;
	default:
	  illegal_instruction (cpu);
	}

      if (acc & 0x8000000000ull)
	*neg = 1;

      STORE (AXREG (which), (acc >> 32) & 0xff);
      STORE (AWREG (which), acc & 0xffffffff);
      STORE (ASTATREG (av[which]), sat);
      if (sat)
	STORE (ASTATREG (avs[which]), sat);

      /* Figure out the overflow bit.  */
      if (sat)
	{
	  if (fullword)
	    *overflow = 1;
	  else
	    ret = extract_mult (cpu, nosat_acc, mmod, MM, fullword, overflow);
	}
    }

  ret = extract_mult (cpu, acc, mmod, MM, fullword, overflow);

  if (!fullword)
    {
      if (ret & 0x8000)
	*neg = 1;
    }
  else
    {
      if (ret & 0x80000000)
	*neg = 1;
    }

  return ret;
}

bu32
hwloop_get_next_pc (SIM_CPU *cpu, bu32 pc, bu32 insn_len)
{
  int i;

  if (insn_len == 0)
    return pc;

  /* If our PC has reached the bottom of a hardware loop,
     move back up to the top of the hardware loop.  */
  for (i = 1; i >= 0; --i)
    if (LCREG (i) > 1 && pc == LBREG (i))
      {
	BFIN_TRACE_BRANCH (cpu, pc, LTREG (i), i, "Hardware loop %i", i);
	return LTREG (i);
      }

  return pc + insn_len;
}

static void
decode_ProgCtrl_0 (SIM_CPU *cpu, bu16 iw0, bu32 pc)
{
  /* ProgCtrl
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |.prgfunc.......|.poprnd........|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int poprnd  = ((iw0 >> ProgCtrl_poprnd_bits) & ProgCtrl_poprnd_mask);
  int prgfunc = ((iw0 >> ProgCtrl_prgfunc_bits) & ProgCtrl_prgfunc_mask);

  TRACE_EXTRACT (cpu, "%s: poprnd:%i prgfunc:%i", __func__, poprnd, prgfunc);

  if (prgfunc == 0 && poprnd == 0)
    {
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_nop);
      TRACE_INSN (cpu, "NOP;");
    }
  else if (prgfunc == 1 && poprnd == 0)
    {
      bu32 newpc = RETSREG;
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_branch);
      TRACE_INSN (cpu, "RTS;");
      IFETCH_CHECK (newpc);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      BFIN_TRACE_BRANCH (cpu, pc, newpc, -1, "RTS");
      SET_PCREG (newpc);
      BFIN_CPU_STATE.did_jump = true;
      CYCLE_DELAY = 5;
    }
  else if (prgfunc == 1 && poprnd == 1)
    {
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_branch);
      TRACE_INSN (cpu, "RTI;");
      /* Do not do IFETCH_CHECK here -- LSB has special meaning.  */
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      cec_return (cpu, -1);
      CYCLE_DELAY = 5;
    }
  else if (prgfunc == 1 && poprnd == 2)
    {
      bu32 newpc = RETXREG;
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_branch);
      TRACE_INSN (cpu, "RTX;");
      /* XXX: Not sure if this is what the hardware does.  */
      IFETCH_CHECK (newpc);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      cec_return (cpu, IVG_EVX);
      CYCLE_DELAY = 5;
    }
  else if (prgfunc == 1 && poprnd == 3)
    {
      bu32 newpc = RETNREG;
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_branch);
      TRACE_INSN (cpu, "RTN;");
      /* XXX: Not sure if this is what the hardware does.  */
      IFETCH_CHECK (newpc);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      cec_return (cpu, IVG_NMI);
      CYCLE_DELAY = 5;
    }
  else if (prgfunc == 1 && poprnd == 4)
    {
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_branch);
      TRACE_INSN (cpu, "RTE;");
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      cec_return (cpu, IVG_EMU);
      CYCLE_DELAY = 5;
    }
  else if (prgfunc == 2 && poprnd == 0)
    {
      SIM_DESC sd = CPU_STATE (cpu);
      sim_events *events = STATE_EVENTS (sd);

      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_sync);
      /* XXX: in supervisor mode, utilizes wake up sources
         in user mode, it's a NOP ...  */
      TRACE_INSN (cpu, "IDLE;");

      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);

      /* Timewarp !  */
      if (events->queue)
	CYCLE_DELAY = events->time_from_event;
      else
	abort (); /* XXX: Should this ever happen ?  */
    }
  else if (prgfunc == 2 && poprnd == 3)
    {
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_sync);
      /* Just NOP it.  */
      TRACE_INSN (cpu, "CSYNC;");
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      CYCLE_DELAY = 10;
    }
  else if (prgfunc == 2 && poprnd == 4)
    {
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_sync);
      /* Just NOP it.  */
      TRACE_INSN (cpu, "SSYNC;");
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);

      /* Really 10+, but no model info for this.  */
      CYCLE_DELAY = 10;
    }
  else if (prgfunc == 2 && poprnd == 5)
    {
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_cec);
      TRACE_INSN (cpu, "EMUEXCPT;");
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      cec_exception (cpu, VEC_SIM_TRAP);
    }
  else if (prgfunc == 3 && poprnd < 8)
    {
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_cec);
      TRACE_INSN (cpu, "CLI R%i;", poprnd);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      SET_DREG (poprnd, cec_cli (cpu));
    }
  else if (prgfunc == 4 && poprnd < 8)
    {
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_cec);
      TRACE_INSN (cpu, "STI R%i;", poprnd);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      cec_sti (cpu, DREG (poprnd));
      CYCLE_DELAY = 3;
    }
  else if (prgfunc == 5 && poprnd < 8)
    {
      bu32 newpc = PREG (poprnd);
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_branch);
      TRACE_INSN (cpu, "JUMP (%s);", get_preg_name (poprnd));
      IFETCH_CHECK (newpc);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      BFIN_TRACE_BRANCH (cpu, pc, newpc, -1, "JUMP (Preg)");
      SET_PCREG (newpc);
      BFIN_CPU_STATE.did_jump = true;
      PROFILE_BRANCH_TAKEN (cpu);
      CYCLE_DELAY = 5;
    }
  else if (prgfunc == 6 && poprnd < 8)
    {
      bu32 newpc = PREG (poprnd);
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_branch);
      TRACE_INSN (cpu, "CALL (%s);", get_preg_name (poprnd));
      IFETCH_CHECK (newpc);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      BFIN_TRACE_BRANCH (cpu, pc, newpc, -1, "CALL (Preg)");
      /* If we're at the end of a hardware loop, RETS is going to be
         the top of the loop rather than the next instruction.  */
      SET_RETSREG (hwloop_get_next_pc (cpu, pc, 2));
      SET_PCREG (newpc);
      BFIN_CPU_STATE.did_jump = true;
      PROFILE_BRANCH_TAKEN (cpu);
      CYCLE_DELAY = 5;
    }
  else if (prgfunc == 7 && poprnd < 8)
    {
      bu32 newpc = pc + PREG (poprnd);
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_branch);
      TRACE_INSN (cpu, "CALL (PC + %s);", get_preg_name (poprnd));
      IFETCH_CHECK (newpc);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      BFIN_TRACE_BRANCH (cpu, pc, newpc, -1, "CALL (PC + Preg)");
      SET_RETSREG (hwloop_get_next_pc (cpu, pc, 2));
      SET_PCREG (newpc);
      BFIN_CPU_STATE.did_jump = true;
      PROFILE_BRANCH_TAKEN (cpu);
      CYCLE_DELAY = 5;
    }
  else if (prgfunc == 8 && poprnd < 8)
    {
      bu32 newpc = pc + PREG (poprnd);
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_branch);
      TRACE_INSN (cpu, "JUMP (PC + %s);", get_preg_name (poprnd));
      IFETCH_CHECK (newpc);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      BFIN_TRACE_BRANCH (cpu, pc, newpc, -1, "JUMP (PC + Preg)");
      SET_PCREG (newpc);
      BFIN_CPU_STATE.did_jump = true;
      PROFILE_BRANCH_TAKEN (cpu);
      CYCLE_DELAY = 5;
    }
  else if (prgfunc == 9)
    {
      int raise = uimm4 (poprnd);
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_cec);
      TRACE_INSN (cpu, "RAISE %s;", uimm4_str (raise));
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      cec_require_supervisor (cpu);
      if (raise == IVG_IVHW)
	cec_hwerr (cpu, HWERR_RAISE_5);
      else
	cec_latch (cpu, raise);
      CYCLE_DELAY = 3; /* XXX: Only if IVG is unmasked.  */
    }
  else if (prgfunc == 10)
    {
      int excpt = uimm4 (poprnd);
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_cec);
      TRACE_INSN (cpu, "EXCPT %s;", uimm4_str (excpt));
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      cec_exception (cpu, excpt);
      CYCLE_DELAY = 3;
    }
  else if (prgfunc == 11 && poprnd < 6)
    {
      bu32 addr = PREG (poprnd);
      bu8 byte;
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ProgCtrl_atomic);
      TRACE_INSN (cpu, "TESTSET (%s);", get_preg_name (poprnd));
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      byte = GET_WORD (addr);
      SET_CCREG (byte == 0);
      PUT_BYTE (addr, byte | 0x80);
      /* Also includes memory stalls, but we don't model that.  */
      CYCLE_DELAY = 2;
    }
  else
    illegal_instruction_or_combination (cpu);
}

static void
decode_CaCTRL_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* CaCTRL
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 1 |.a.|.op....|.reg.......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int a   = ((iw0 >> CaCTRL_a_bits) & CaCTRL_a_mask);
  int op  = ((iw0 >> CaCTRL_op_bits) & CaCTRL_op_mask);
  int reg = ((iw0 >> CaCTRL_reg_bits) & CaCTRL_reg_mask);
  bu32 preg = PREG (reg);
  const char * const sinsn[] = { "PREFETCH", "FLUSHINV", "FLUSH", "IFLUSH", };

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_CaCTRL);
  TRACE_EXTRACT (cpu, "%s: a:%i op:%i reg:%i", __func__, a, op, reg);
  TRACE_INSN (cpu, "%s [%s%s];", sinsn[op], get_preg_name (reg), a ? "++" : "");

  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    /* None of these can be part of a parallel instruction.  */
    illegal_instruction_combination (cpu);

  /* No cache simulation, so these are (mostly) all NOPs.
     XXX: The hardware takes care of masking to cache lines, but need
     to check behavior of the post increment.  Should we be aligning
     the value to the cache line before adding the cache line size, or
     do we just add the cache line size ?  */
  if (op == 0)
    {	/* PREFETCH  */
      mmu_check_cache_addr (cpu, preg, false, false);
    }
  else if (op == 1)
    {	/* FLUSHINV  */
      mmu_check_cache_addr (cpu, preg, true, false);
    }
  else if (op == 2)
    {	/* FLUSH  */
      mmu_check_cache_addr (cpu, preg, true, false);
    }
  else if (op == 3)
    {	/* IFLUSH  */
      mmu_check_cache_addr (cpu, preg, false, true);
    }

  if (a)
    SET_PREG (reg, preg + BFIN_L1_CACHE_BYTES);
}

static void
decode_PushPopReg_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* PushPopReg
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 |.W.|.grp.......|.reg.......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int W   = ((iw0 >> PushPopReg_W_bits) & PushPopReg_W_mask);
  int grp = ((iw0 >> PushPopReg_grp_bits) & PushPopReg_grp_mask);
  int reg = ((iw0 >> PushPopReg_reg_bits) & PushPopReg_reg_mask);
  const char *reg_name = get_allreg_name (grp, reg);
  bu32 value;
  bu32 sp = SPREG;

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_PushPopReg);
  TRACE_EXTRACT (cpu, "%s: W:%i grp:%i reg:%i", __func__, W, grp, reg);
  TRACE_DECODE (cpu, "%s: reg:%s", __func__, reg_name);

  /* Can't push/pop reserved registers  */
  if (reg_is_reserved (grp, reg))
    illegal_instruction_or_combination (cpu);

  if (W == 0)
    {
      /* Dreg and Preg are not supported by this instruction.  */
      if (grp == 0 || grp == 1)
	illegal_instruction_or_combination (cpu);
      TRACE_INSN (cpu, "%s = [SP++];", reg_name);
      /* Can't pop USP while in userspace.  */
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE
	  || (grp == 7 && reg == 0 && cec_is_user_mode(cpu)))
	illegal_instruction_combination (cpu);
      /* XXX: The valid register check is in reg_write(), so we might
              incorrectly do a GET_LONG() here ...  */
      value = GET_LONG (sp);
      reg_write (cpu, grp, reg, value);
      if (grp == 7 && reg == 3)
	cec_pop_reti (cpu);

      sp += 4;
    }
  else
    {
      TRACE_INSN (cpu, "[--SP] = %s;", reg_name);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);

      sp -= 4;
      value = reg_read (cpu, grp, reg);
      if (grp == 7 && reg == 3)
	cec_push_reti (cpu);

      PUT_LONG (sp, value);
    }

  /* Note: SP update must be delayed until after all reads/writes; see
           comments in decode_PushPopMultiple_0() for more info.  */
  SET_SPREG (sp);
}

static void
decode_PushPopMultiple_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* PushPopMultiple
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 0 | 0 | 0 | 0 | 1 | 0 |.d.|.p.|.W.|.dr........|.pr........|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int p  = ((iw0 >> PushPopMultiple_p_bits) & PushPopMultiple_p_mask);
  int d  = ((iw0 >> PushPopMultiple_d_bits) & PushPopMultiple_d_mask);
  int W  = ((iw0 >> PushPopMultiple_W_bits) & PushPopMultiple_W_mask);
  int dr = ((iw0 >> PushPopMultiple_dr_bits) & PushPopMultiple_dr_mask);
  int pr = ((iw0 >> PushPopMultiple_pr_bits) & PushPopMultiple_pr_mask);
  int i;
  bu32 sp = SPREG;

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_PushPopMultiple);
  TRACE_EXTRACT (cpu, "%s: d:%i p:%i W:%i dr:%i pr:%i",
		 __func__, d, p, W, dr, pr);

  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    illegal_instruction_combination (cpu);

  if ((d == 0 && p == 0) || (p && imm5 (pr) > 5)
      || (d && !p && pr) || (p && !d && dr))
    illegal_instruction (cpu);

  if (W == 1)
    {
      if (d && p)
	TRACE_INSN (cpu, "[--SP] = (R7:%i, P5:%i);", dr, pr);
      else if (d)
	TRACE_INSN (cpu, "[--SP] = (R7:%i);", dr);
      else
	TRACE_INSN (cpu, "[--SP] = (P5:%i);", pr);

      if (d)
	for (i = dr; i < 8; i++)
	  {
	    sp -= 4;
	    PUT_LONG (sp, DREG (i));
	  }
      if (p)
	for (i = pr; i < 6; i++)
	  {
	    sp -= 4;
	    PUT_LONG (sp, PREG (i));
	  }

      CYCLE_DELAY = 14;
    }
  else
    {
      if (d && p)
	TRACE_INSN (cpu, "(R7:%i, P5:%i) = [SP++];", dr, pr);
      else if (d)
	TRACE_INSN (cpu, "(R7:%i) = [SP++];", dr);
      else
	TRACE_INSN (cpu, "(P5:%i) = [SP++];", pr);

      if (p)
	for (i = 5; i >= pr; i--)
	  {
	    SET_PREG (i, GET_LONG (sp));
	    sp += 4;
	  }
      if (d)
	for (i = 7; i >= dr; i--)
	  {
	    SET_DREG (i, GET_LONG (sp));
	    sp += 4;
	  }

      CYCLE_DELAY = 11;
    }

  /* Note: SP update must be delayed until after all reads/writes so that
           if an exception does occur, the insn may be re-executed as the
           SP has not yet changed.  */
  SET_SPREG (sp);
}

static void
decode_ccMV_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* ccMV
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 0 | 0 | 0 | 0 | 1 | 1 |.T.|.d.|.s.|.dst.......|.src.......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int s  = ((iw0 >> CCmv_s_bits) & CCmv_s_mask);
  int d  = ((iw0 >> CCmv_d_bits) & CCmv_d_mask);
  int T  = ((iw0 >> CCmv_T_bits) & CCmv_T_mask);
  int src = ((iw0 >> CCmv_src_bits) & CCmv_src_mask);
  int dst = ((iw0 >> CCmv_dst_bits) & CCmv_dst_mask);
  int cond = T ? CCREG : ! CCREG;

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ccMV);
  TRACE_EXTRACT (cpu, "%s: T:%i d:%i s:%i dst:%i src:%i",
		 __func__, T, d, s, dst, src);

  TRACE_INSN (cpu, "IF %sCC %s = %s;", T ? "" : "! ",
	      get_allreg_name (d, dst),
	      get_allreg_name (s, src));
  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    illegal_instruction_combination (cpu);

  if (cond)
    reg_write (cpu, d, dst, reg_read (cpu, s, src));
}

static void
decode_CCflag_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* CCflag
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 0 | 0 | 0 | 1 |.I.|.opc.......|.G.|.y.........|.x.........|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int x = ((iw0 >> CCflag_x_bits) & CCflag_x_mask);
  int y = ((iw0 >> CCflag_y_bits) & CCflag_y_mask);
  int I = ((iw0 >> CCflag_I_bits) & CCflag_I_mask);
  int G = ((iw0 >> CCflag_G_bits) & CCflag_G_mask);
  int opc = ((iw0 >> CCflag_opc_bits) & CCflag_opc_mask);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_CCflag);
  TRACE_EXTRACT (cpu, "%s: I:%i opc:%i G:%i y:%i x:%i",
		 __func__, I, opc, G, y, x);

  if (opc > 4)
    {
      bs64 acc0 = get_extended_acc (cpu, 0);
      bs64 acc1 = get_extended_acc (cpu, 1);
      bs64 diff = acc0 - acc1;

      if (x != 0 || y != 0)
	illegal_instruction_or_combination (cpu);

      if (opc == 5 && I == 0 && G == 0)
	{
	  TRACE_INSN (cpu, "CC = A0 == A1;");
	  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	    illegal_instruction_combination (cpu);
	  SET_CCREG (acc0 == acc1);
	}
      else if (opc == 6 && I == 0 && G == 0)
	{
	  TRACE_INSN (cpu, "CC = A0 < A1");
	  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	    illegal_instruction_combination (cpu);
	  SET_CCREG (acc0 < acc1);
	}
      else if (opc == 7 && I == 0 && G == 0)
	{
	  TRACE_INSN (cpu, "CC = A0 <= A1");
	  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	    illegal_instruction_combination (cpu);
	  SET_CCREG (acc0 <= acc1);
	}
      else
	illegal_instruction_or_combination (cpu);

      SET_ASTATREG (az, diff == 0);
      SET_ASTATREG (an, diff < 0);
      SET_ASTATREG (ac0, (bu40)acc1 <= (bu40)acc0);
    }
  else
    {
      int issigned = opc < 3;
      const char *sign = issigned ? "" : " (IU)";
      bu32 srcop = G ? PREG (x) : DREG (x);
      char s = G ? 'P' : 'R';
      bu32 dstop = I ? (issigned ? imm3 (y) : uimm3 (y)) : G ? PREG (y) : DREG (y);
      const char *op;
      char d = G ? 'P' : 'R';
      int flgs = srcop >> 31;
      int flgo = dstop >> 31;

      bu32 result = srcop - dstop;
      int cc;
      int flgn = result >> 31;
      int overflow = (flgs ^ flgo) & (flgn ^ flgs);
      int az = result == 0;
      int ac0 = dstop <= srcop;
      int an;
      if (issigned)
	an = (flgn && !overflow) || (!flgn && overflow);
      else
	an = dstop > srcop;

      switch (opc)
	{
	default: /* Shutup useless gcc warnings.  */
	case 0: /* signed  */
	  op = "==";
	  cc = az;
	  break;
	case 1:	/* signed  */
	  op = "<";
	  cc = an;
	  break;
	case 2:	/* signed  */
	  op = "<=";
	  cc = an || az;
	  break;
	case 3:	/* unsigned  */
	  op = "<";
	  cc = !ac0;
	  break;
	case 4:	/* unsigned  */
	  op = "<=";
	  cc = !ac0 || az;
	  break;
	}

      if (I)
	TRACE_INSN (cpu, "CC = %c%i %s %s%s;", s, x, op,
		    issigned ? imm3_str (y) : uimm3_str (y), sign);
      else
	{
	  TRACE_DECODE (cpu, "%s %c%i:%x %c%i:%x", __func__,
			s, x, srcop,  d, y, dstop);
	  TRACE_INSN (cpu, "CC = %c%i %s %c%i%s;", s, x, op, d, y, sign);
	}

      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);

      SET_CCREG (cc);
      /* Pointer compares only touch CC.  */
      if (!G)
	{
	  SET_ASTATREG (az, az);
	  SET_ASTATREG (an, an);
	  SET_ASTATREG (ac0, ac0);
	}
    }
}

static void
decode_CC2dreg_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* CC2dreg
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 | 0 |.op....|.reg.......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int op  = ((iw0 >> CC2dreg_op_bits) & CC2dreg_op_mask);
  int reg = ((iw0 >> CC2dreg_reg_bits) & CC2dreg_reg_mask);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_CC2dreg);
  TRACE_EXTRACT (cpu, "%s: op:%i reg:%i", __func__, op, reg);

  if (op == 0)
    {
      TRACE_INSN (cpu, "R%i = CC;", reg);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      SET_DREG (reg, CCREG);
    }
  else if (op == 1)
    {
      TRACE_INSN (cpu, "CC = R%i;", reg);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      SET_CCREG (DREG (reg) != 0);
    }
  else if (op == 3 && reg == 0)
    {
      TRACE_INSN (cpu, "CC = !CC;");
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      SET_CCREG (!CCREG);
    }
  else
    illegal_instruction_or_combination (cpu);
}

static void
decode_CC2stat_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* CC2stat
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 1 |.D.|.op....|.cbit..............|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int D    = ((iw0 >> CC2stat_D_bits) & CC2stat_D_mask);
  int op   = ((iw0 >> CC2stat_op_bits) & CC2stat_op_mask);
  int cbit = ((iw0 >> CC2stat_cbit_bits) & CC2stat_cbit_mask);
  bu32 pval;

  const char * const op_names[] = { "", "|", "&", "^" } ;

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_CC2stat);
  TRACE_EXTRACT (cpu, "%s: D:%i op:%i cbit:%i", __func__, D, op, cbit);

  TRACE_INSN (cpu, "%s %s= %s;", D ? astat_names[cbit] : "CC",
	      op_names[op], D ? "CC" : astat_names[cbit]);

  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    illegal_instruction_combination (cpu);

  /* CC = CC; is invalid.  */
  if (cbit == 5)
    illegal_instruction (cpu);

  pval = !!(ASTAT & (1 << cbit));
  if (D == 0)
    switch (op)
      {
      case 0: SET_CCREG (pval); break;
      case 1: SET_CCREG (CCREG | pval); break;
      case 2: SET_CCREG (CCREG & pval); break;
      case 3: SET_CCREG (CCREG ^ pval); break;
      }
  else
    {
      switch (op)
	{
	case 0: pval  = CCREG; break;
	case 1: pval |= CCREG; break;
	case 2: pval &= CCREG; break;
	case 3: pval ^= CCREG; break;
	}
      TRACE_REGISTER (cpu, "wrote ASTAT[%s] = %i", astat_names[cbit], pval);
      SET_ASTAT ((ASTAT & ~(1 << cbit)) | (pval << cbit));
    }
}

static void
decode_BRCC_0 (SIM_CPU *cpu, bu16 iw0, bu32 pc)
{
  /* BRCC
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 0 | 0 | 1 |.T.|.B.|.offset................................|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int B = ((iw0 >> BRCC_B_bits) & BRCC_B_mask);
  int T = ((iw0 >> BRCC_T_bits) & BRCC_T_mask);
  int offset = ((iw0 >> BRCC_offset_bits) & BRCC_offset_mask);
  int cond = T ? CCREG : ! CCREG;
  int pcrel = pcrel10 (offset);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_BRCC);
  TRACE_EXTRACT (cpu, "%s: T:%i B:%i offset:%#x", __func__, T, B, offset);
  TRACE_DECODE (cpu, "%s: pcrel10:%#x", __func__, pcrel);

  TRACE_INSN (cpu, "IF %sCC JUMP %#x%s;", T ? "" : "! ",
	      pcrel, B ? " (bp)" : "");

  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    illegal_instruction_combination (cpu);

  if (cond)
    {
      bu32 newpc = pc + pcrel;
      BFIN_TRACE_BRANCH (cpu, pc, newpc, -1, "Conditional JUMP");
      SET_PCREG (newpc);
      BFIN_CPU_STATE.did_jump = true;
      PROFILE_BRANCH_TAKEN (cpu);
      CYCLE_DELAY = B ? 5 : 9;
    }
  else
    {
      PROFILE_BRANCH_UNTAKEN (cpu);
      CYCLE_DELAY = B ? 9 : 1;
    }
}

static void
decode_UJUMP_0 (SIM_CPU *cpu, bu16 iw0, bu32 pc)
{
  /* UJUMP
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 0 | 1 | 0 |.offset........................................|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int offset = ((iw0 >> UJump_offset_bits) & UJump_offset_mask);
  int pcrel = pcrel12 (offset);
  bu32 newpc = pc + pcrel;

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_UJUMP);
  TRACE_EXTRACT (cpu, "%s: offset:%#x", __func__, offset);
  TRACE_DECODE (cpu, "%s: pcrel12:%#x", __func__, pcrel);

  TRACE_INSN (cpu, "JUMP.S %#x;", pcrel);

  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    illegal_instruction_combination (cpu);

  BFIN_TRACE_BRANCH (cpu, pc, newpc, -1, "JUMP.S");

  SET_PCREG (newpc);
  BFIN_CPU_STATE.did_jump = true;
  PROFILE_BRANCH_TAKEN (cpu);
  CYCLE_DELAY = 5;
}

static void
decode_REGMV_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* REGMV
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 0 | 1 | 1 |.gd........|.gs........|.dst.......|.src.......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int gs  = ((iw0 >> RegMv_gs_bits) & RegMv_gs_mask);
  int gd  = ((iw0 >> RegMv_gd_bits) & RegMv_gd_mask);
  int src = ((iw0 >> RegMv_src_bits) & RegMv_src_mask);
  int dst = ((iw0 >> RegMv_dst_bits) & RegMv_dst_mask);
  const char *srcreg_name = get_allreg_name (gs, src);
  const char *dstreg_name = get_allreg_name (gd, dst);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_REGMV);
  TRACE_EXTRACT (cpu, "%s: gd:%i gs:%i dst:%i src:%i",
		 __func__, gd, gs, dst, src);
  TRACE_DECODE (cpu, "%s: dst:%s src:%s", __func__, dstreg_name, srcreg_name);

  TRACE_INSN (cpu, "%s = %s;", dstreg_name, srcreg_name);

  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    illegal_instruction_combination (cpu);

  /* Reserved slots cannot be a src/dst.  */
  if (reg_is_reserved (gs, src) || reg_is_reserved (gd, dst))
    goto invalid_move;

  /* Standard register moves.  */
  if ((gs < 2)						/* Dregs/Pregs src  */
      || (gd < 2)					/* Dregs/Pregs dst  */
      || (gs == 4 && src < 4)				/* Accumulators src  */
      || (gd == 4 && dst < 4 && (gs < 4))		/* Accumulators dst  */
      || (gs == 7 && src == 7 && !(gd == 4 && dst < 4))	/* EMUDAT src  */
      || (gd == 7 && dst == 7)) 			/* EMUDAT dst  */
    goto valid_move;

  /* dareg = dareg (IMBL)  */
  if (gs < 4 && gd < 4)
    goto valid_move;

  /* USP can be src to sysregs, but not dagregs.  */
  if ((gs == 7 && src == 0) && (gd >= 4))
    goto valid_move;

  /* USP can move between genregs (only check Accumulators).  */
  if (((gs == 7 && src == 0) && (gd == 4 && dst < 4))
      || ((gd == 7 && dst == 0) && (gs == 4 && src < 4)))
    goto valid_move;

  /* Still here ?  Invalid reg pair.  */
 invalid_move:
  illegal_instruction (cpu);

 valid_move:
  reg_write (cpu, gd, dst, reg_read (cpu, gs, src));
}

static void
decode_ALU2op_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* ALU2op
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 1 | 0 | 0 | 0 | 0 |.opc...........|.src.......|.dst.......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int src = ((iw0 >> ALU2op_src_bits) & ALU2op_src_mask);
  int opc = ((iw0 >> ALU2op_opc_bits) & ALU2op_opc_mask);
  int dst = ((iw0 >> ALU2op_dst_bits) & ALU2op_dst_mask);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_ALU2op);
  TRACE_EXTRACT (cpu, "%s: opc:%i src:%i dst:%i", __func__, opc, src, dst);

  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    illegal_instruction_combination (cpu);

  if (opc == 0)
    {
      TRACE_INSN (cpu, "R%i >>>= R%i;", dst, src);
      SET_DREG (dst, ashiftrt (cpu, DREG (dst), DREG (src), 32));
    }
  else if (opc == 1)
    {
      bu32 val;
      TRACE_INSN (cpu, "R%i >>= R%i;", dst, src);
      if (DREG (src) <= 0x1F)
	val = lshiftrt (cpu, DREG (dst), DREG (src), 32);
      else
	val = 0;
      SET_DREG (dst, val);
    }
  else if (opc == 2)
    {
      TRACE_INSN (cpu, "R%i <<= R%i;", dst, src);
      SET_DREG (dst, lshift (cpu, DREG (dst), DREG (src), 32, 0, 0));
    }
  else if (opc == 3)
    {
      TRACE_INSN (cpu, "R%i *= R%i;", dst, src);
      SET_DREG (dst, DREG (dst) * DREG (src));
      CYCLE_DELAY = 3;
    }
  else if (opc == 4)
    {
      TRACE_INSN (cpu, "R%i = (R%i + R%i) << 1;", dst, dst, src);
      SET_DREG (dst, add_and_shift (cpu, DREG (dst), DREG (src), 1));
    }
  else if (opc == 5)
    {
      TRACE_INSN (cpu, "R%i = (R%i + R%i) << 2;", dst, dst, src);
      SET_DREG (dst, add_and_shift (cpu, DREG (dst), DREG (src), 2));
    }
  else if (opc == 8)
    {
      TRACE_INSN (cpu, "DIVQ ( R%i, R%i );", dst, src);
      SET_DREG (dst, divq (cpu, DREG (dst), (bu16)DREG (src)));
    }
  else if (opc == 9)
    {
      TRACE_INSN (cpu, "DIVS ( R%i, R%i );", dst, src);
      SET_DREG (dst, divs (cpu, DREG (dst), (bu16)DREG (src)));
    }
  else if (opc == 10)
    {
      TRACE_INSN (cpu, "R%i = R%i.L (X);", dst, src);
      SET_DREG (dst, (bs32) (bs16) DREG (src));
      setflags_logical (cpu, DREG (dst));
    }
  else if (opc == 11)
    {
      TRACE_INSN (cpu, "R%i = R%i.L (Z);", dst, src);
      SET_DREG (dst, (bu32) (bu16) DREG (src));
      setflags_logical (cpu, DREG (dst));
    }
  else if (opc == 12)
    {
      TRACE_INSN (cpu, "R%i = R%i.B (X);", dst, src);
      SET_DREG (dst, (bs32) (bs8) DREG (src));
      setflags_logical (cpu, DREG (dst));
    }
  else if (opc == 13)
    {
      TRACE_INSN (cpu, "R%i = R%i.B (Z);", dst, src);
      SET_DREG (dst, (bu32) (bu8) DREG (src));
      setflags_logical (cpu, DREG (dst));
    }
  else if (opc == 14)
    {
      bu32 val = DREG (src);
      TRACE_INSN (cpu, "R%i = - R%i;", dst, src);
      SET_DREG (dst, -val);
      setflags_nz (cpu, DREG (dst));
      SET_ASTATREG (v, val == 0x80000000);
      if (ASTATREG (v))
	SET_ASTATREG (vs, 1);
      SET_ASTATREG (ac0, val == 0x0);
      /* XXX: Documentation isn't entirely clear about av0 and av1.  */
    }
  else if (opc == 15)
    {
      TRACE_INSN (cpu, "R%i = ~ R%i;", dst, src);
      SET_DREG (dst, ~DREG (src));
      setflags_logical (cpu, DREG (dst));
    }
  else
    illegal_instruction (cpu);
}

static void
decode_PTR2op_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* PTR2op
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 1 | 0 | 0 | 0 | 1 | 0 |.opc.......|.src.......|.dst.......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int src = ((iw0 >> PTR2op_src_bits) & PTR2op_dst_mask);
  int opc = ((iw0 >> PTR2op_opc_bits) & PTR2op_opc_mask);
  int dst = ((iw0 >> PTR2op_dst_bits) & PTR2op_dst_mask);
  const char *src_name = get_preg_name (src);
  const char *dst_name = get_preg_name (dst);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_PTR2op);
  TRACE_EXTRACT (cpu, "%s: opc:%i src:%i dst:%i", __func__, opc, src, dst);

  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    illegal_instruction_combination (cpu);

  if (opc == 0)
    {
      TRACE_INSN (cpu, "%s -= %s", dst_name, src_name);
      SET_PREG (dst, PREG (dst) - PREG (src));
    }
  else if (opc == 1)
    {
      TRACE_INSN (cpu, "%s = %s << 2", dst_name, src_name);
      SET_PREG (dst, PREG (src) << 2);
    }
  else if (opc == 3)
    {
      TRACE_INSN (cpu, "%s = %s >> 2", dst_name, src_name);
      SET_PREG (dst, PREG (src) >> 2);
    }
  else if (opc == 4)
    {
      TRACE_INSN (cpu, "%s = %s >> 1", dst_name, src_name);
      SET_PREG (dst, PREG (src) >> 1);
    }
  else if (opc == 5)
    {
      TRACE_INSN (cpu, "%s += %s (BREV)", dst_name, src_name);
      SET_PREG (dst, add_brev (PREG (dst), PREG (src)));
    }
  else if (opc == 6)
    {
      TRACE_INSN (cpu, "%s = (%s + %s) << 1", dst_name, dst_name, src_name);
      SET_PREG (dst, (PREG (dst) + PREG (src)) << 1);
    }
  else if (opc == 7)
    {
      TRACE_INSN (cpu, "%s = (%s + %s) << 2", dst_name, dst_name, src_name);
      SET_PREG (dst, (PREG (dst) + PREG (src)) << 2);
    }
  else
    illegal_instruction (cpu);
}

static void
decode_LOGI2op_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* LOGI2op
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 1 | 0 | 0 | 1 |.opc.......|.src...............|.dst.......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int src = ((iw0 >> LOGI2op_src_bits) & LOGI2op_src_mask);
  int opc = ((iw0 >> LOGI2op_opc_bits) & LOGI2op_opc_mask);
  int dst = ((iw0 >> LOGI2op_dst_bits) & LOGI2op_dst_mask);
  int uimm = uimm5 (src);
  const char *uimm_str = uimm5_str (uimm);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_LOGI2op);
  TRACE_EXTRACT (cpu, "%s: opc:%i src:%i dst:%i", __func__, opc, src, dst);
  TRACE_DECODE (cpu, "%s: uimm5:%#x", __func__, uimm);

  if (opc == 0)
    {
      TRACE_INSN (cpu, "CC = ! BITTST (R%i, %s);", dst, uimm_str);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      SET_CCREG ((~DREG (dst) >> uimm) & 1);
    }
  else if (opc == 1)
    {
      TRACE_INSN (cpu, "CC = BITTST (R%i, %s);", dst, uimm_str);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      SET_CCREG ((DREG (dst) >> uimm) & 1);
    }
  else if (opc == 2)
    {
      TRACE_INSN (cpu, "BITSET (R%i, %s);", dst, uimm_str);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      SET_DREG (dst, DREG (dst) | (1 << uimm));
      setflags_logical (cpu, DREG (dst));
    }
  else if (opc == 3)
    {
      TRACE_INSN (cpu, "BITTGL (R%i, %s);", dst, uimm_str);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      SET_DREG (dst, DREG (dst) ^ (1 << uimm));
      setflags_logical (cpu, DREG (dst));
    }
  else if (opc == 4)
    {
      TRACE_INSN (cpu, "BITCLR (R%i, %s);", dst, uimm_str);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      SET_DREG (dst, DREG (dst) & ~(1 << uimm));
      setflags_logical (cpu, DREG (dst));
    }
  else if (opc == 5)
    {
      TRACE_INSN (cpu, "R%i >>>= %s;", dst, uimm_str);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      SET_DREG (dst, ashiftrt (cpu, DREG (dst), uimm, 32));
    }
  else if (opc == 6)
    {
      TRACE_INSN (cpu, "R%i >>= %s;", dst, uimm_str);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      SET_DREG (dst, lshiftrt (cpu, DREG (dst), uimm, 32));
    }
  else if (opc == 7)
    {
      TRACE_INSN (cpu, "R%i <<= %s;", dst, uimm_str);
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      SET_DREG (dst, lshift (cpu, DREG (dst), uimm, 32, 0, 0));
    }
}

static void
decode_COMP3op_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* COMP3op
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 1 | 0 | 1 |.opc.......|.dst.......|.src1......|.src0......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int opc  = ((iw0 >> COMP3op_opc_bits) & COMP3op_opc_mask);
  int dst  = ((iw0 >> COMP3op_dst_bits) & COMP3op_dst_mask);
  int src0 = ((iw0 >> COMP3op_src0_bits) & COMP3op_src0_mask);
  int src1 = ((iw0 >> COMP3op_src1_bits) & COMP3op_src1_mask);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_COMP3op);
  TRACE_EXTRACT (cpu, "%s: opc:%i dst:%i src1:%i src0:%i",
		 __func__, opc, dst, src1, src0);

  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    illegal_instruction_combination (cpu);

  if (opc == 0)
    {
      TRACE_INSN (cpu, "R%i = R%i + R%i;", dst, src0, src1);
      SET_DREG (dst, add32 (cpu, DREG (src0), DREG (src1), 1, 0));
    }
  else if (opc == 1)
    {
      TRACE_INSN (cpu, "R%i = R%i - R%i;", dst, src0, src1);
      SET_DREG (dst, sub32 (cpu, DREG (src0), DREG (src1), 1, 0, 0));
    }
  else if (opc == 2)
    {
      TRACE_INSN (cpu, "R%i = R%i & R%i;", dst, src0, src1);
      SET_DREG (dst, DREG (src0) & DREG (src1));
      setflags_logical (cpu, DREG (dst));
    }
  else if (opc == 3)
    {
      TRACE_INSN (cpu, "R%i = R%i | R%i;", dst, src0, src1);
      SET_DREG (dst, DREG (src0) | DREG (src1));
      setflags_logical (cpu, DREG (dst));
    }
  else if (opc == 4)
    {
      TRACE_INSN (cpu, "R%i = R%i ^ R%i;", dst, src0, src1);
      SET_DREG (dst, DREG (src0) ^ DREG (src1));
      setflags_logical (cpu, DREG (dst));
    }
  else
    {
      int shift = opc - 5;
      const char *dst_name = get_preg_name (dst);
      const char *src0_name = get_preg_name (src0);
      const char *src1_name = get_preg_name (src1);

      /* If src0 == src1 this is disassembled as a shift by 1, but this
         distinction doesn't matter for our purposes.  */
      if (shift)
	TRACE_INSN (cpu, "%s = (%s + %s) << %#x;",
		    dst_name, src0_name, src1_name, shift);
      else
	TRACE_INSN (cpu, "%s = %s + %s",
		    dst_name, src0_name, src1_name);
      SET_PREG (dst, PREG (src0) + (PREG (src1) << shift));
    }
}

static void
decode_COMPI2opD_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* COMPI2opD
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 1 | 1 | 0 | 0 |.op|..src......................|.dst.......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int op  = ((iw0 >> COMPI2opD_op_bits) & COMPI2opD_op_mask);
  int dst = ((iw0 >> COMPI2opD_dst_bits) & COMPI2opD_dst_mask);
  int src = ((iw0 >> COMPI2opD_src_bits) & COMPI2opD_src_mask);
  int imm = imm7 (src);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_COMPI2opD);
  TRACE_EXTRACT (cpu, "%s: op:%i src:%i dst:%i", __func__, op, src, dst);
  TRACE_DECODE (cpu, "%s: imm7:%#x", __func__, imm);

  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    illegal_instruction_combination (cpu);

  if (op == 0)
    {
      TRACE_INSN (cpu, "R%i = %s (X);", dst, imm7_str (imm));
      SET_DREG (dst, imm);
    }
  else if (op == 1)
    {
      TRACE_INSN (cpu, "R%i += %s;", dst, imm7_str (imm));
      SET_DREG (dst, add32 (cpu, DREG (dst), imm, 1, 0));
    }
}

static void
decode_COMPI2opP_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* COMPI2opP
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 0 | 1 | 1 | 0 | 1 |.op|.src.......................|.dst.......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int op  = ((iw0 >> COMPI2opP_op_bits) & COMPI2opP_op_mask);
  int src = ((iw0 >> COMPI2opP_src_bits) & COMPI2opP_src_mask);
  int dst = ((iw0 >> COMPI2opP_dst_bits) & COMPI2opP_dst_mask);
  int imm = imm7 (src);
  const char *dst_name = get_preg_name (dst);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_COMPI2opP);
  TRACE_EXTRACT (cpu, "%s: op:%i src:%i dst:%i", __func__, op, src, dst);
  TRACE_DECODE (cpu, "%s: imm:%#x", __func__, imm);

  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    illegal_instruction_combination (cpu);

  if (op == 0)
    {
      TRACE_INSN (cpu, "%s = %s;", dst_name, imm7_str (imm));
      SET_PREG (dst, imm);
    }
  else if (op == 1)
    {
      TRACE_INSN (cpu, "%s += %s;", dst_name, imm7_str (imm));
      SET_PREG (dst, PREG (dst) + imm);
    }
}

static void
decode_LDSTpmod_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* LDSTpmod
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 0 | 0 | 0 |.W.|.aop...|.reg.......|.idx.......|.ptr.......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int W   = ((iw0 >> LDSTpmod_W_bits) & LDSTpmod_W_mask);
  int aop = ((iw0 >> LDSTpmod_aop_bits) & LDSTpmod_aop_mask);
  int idx = ((iw0 >> LDSTpmod_idx_bits) & LDSTpmod_idx_mask);
  int ptr = ((iw0 >> LDSTpmod_ptr_bits) & LDSTpmod_ptr_mask);
  int reg = ((iw0 >> LDSTpmod_reg_bits) & LDSTpmod_reg_mask);
  const char *ptr_name = get_preg_name (ptr);
  const char *idx_name = get_preg_name (idx);
  bu32 addr, val;

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_LDSTpmod);
  TRACE_EXTRACT (cpu, "%s: W:%i aop:%i reg:%i idx:%i ptr:%i",
		 __func__, W, aop, reg, idx, ptr);

  if (PARALLEL_GROUP == BFIN_PARALLEL_GROUP2)
    illegal_instruction_combination (cpu);

  if (aop == 1 && W == 0 && idx == ptr)
    {
      TRACE_INSN (cpu, "R%i.L = W[%s];", reg, ptr_name);
      addr = PREG (ptr);
      val = GET_WORD (addr);
      STORE (DREG (reg), (DREG (reg) & 0xFFFF0000) | val);
    }
  else if (aop == 2 && W == 0 && idx == ptr)
    {
      TRACE_INSN (cpu, "R%i.H = W[%s];", reg, ptr_name);
      addr = PREG (ptr);
      val = GET_WORD (addr);
      STORE (DREG (reg), (DREG (reg) & 0xFFFF) | (val << 16));
    }
  else if (aop == 1 && W == 1 && idx == ptr)
    {
      TRACE_INSN (cpu, "W[%s] = R%i.L;", ptr_name, reg);
      addr = PREG (ptr);
      PUT_WORD (addr, DREG (reg));
    }
  else if (aop == 2 && W == 1 && idx == ptr)
    {
      TRACE_INSN (cpu, "W[%s] = R%i.H;", ptr_name, reg);
      addr = PREG (ptr);
      PUT_WORD (addr, DREG (reg) >> 16);
    }
  else if (aop == 0 && W == 0)
    {
      TRACE_INSN (cpu, "R%i = [%s ++ %s];", reg, ptr_name, idx_name);
      addr = PREG (ptr);
      val = GET_LONG (addr);
      STORE (DREG (reg), val);
      if (ptr != idx)
	STORE (PREG (ptr), addr + PREG (idx));
    }
  else if (aop == 1 && W == 0)
    {
      TRACE_INSN (cpu, "R%i.L = W[%s ++ %s];", reg, ptr_name, idx_name);
      addr = PREG (ptr);
      val = GET_WORD (addr);
      STORE (DREG (reg), (DREG (reg) & 0xFFFF0000) | val);
      if (ptr != idx)
	STORE (PREG (ptr), addr + PREG (idx));
    }
  else if (aop == 2 && W == 0)
    {
      TRACE_INSN (cpu, "R%i.H = W[%s ++ %s];", reg, ptr_name, idx_name);
      addr = PREG (ptr);
      val = GET_WORD (addr);
      STORE (DREG (reg), (DREG (reg) & 0xFFFF) | (val << 16));
      if (ptr != idx)
	STORE (PREG (ptr), addr + PREG (idx));
    }
  else if (aop == 3 && W == 0)
    {
      TRACE_INSN (cpu, "R%i = W[%s ++ %s] (Z);", reg, ptr_name, idx_name);
      addr = PREG (ptr);
      val = GET_WORD (addr);
      STORE (DREG (reg), val);
      if (ptr != idx)
	STORE (PREG (ptr), addr + PREG (idx));
    }
  else if (aop == 3 && W == 1)
    {
      TRACE_INSN (cpu, "R%i = W[%s ++ %s] (X);", reg, ptr_name, idx_name);
      addr = PREG (ptr);
      val = GET_WORD (addr);
      STORE (DREG (reg), (bs32) (bs16) val);
      if (ptr != idx)
	STORE (PREG (ptr), addr + PREG (idx));
    }
  else if (aop == 0 && W == 1)
    {
      TRACE_INSN (cpu, "[%s ++ %s] = R%i;", ptr_name, idx_name, reg);
      addr = PREG (ptr);
      PUT_LONG (addr, DREG (reg));
      if (ptr != idx)
	STORE (PREG (ptr), addr + PREG (idx));
    }
  else if (aop == 1 && W == 1)
    {
      TRACE_INSN (cpu, "W[%s ++ %s] = R%i.L;", ptr_name, idx_name, reg);
      addr = PREG (ptr);
      PUT_WORD (addr, DREG (reg));
      if (ptr != idx)
	STORE (PREG (ptr), addr + PREG (idx));
    }
  else if (aop == 2 && W == 1)
    {
      TRACE_INSN (cpu, "W[%s ++ %s] = R%i.H;", ptr_name, idx_name, reg);
      addr = PREG (ptr);
      PUT_WORD (addr, DREG (reg) >> 16);
      if (ptr != idx)
	STORE (PREG (ptr), addr + PREG (idx));
    }
  else
    illegal_instruction_or_combination (cpu);
}

static void
decode_dagMODim_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* dagMODim
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 0 | 0 | 1 | 1 | 1 | 1 | 0 |.br| 1 | 1 |.op|.m.....|.i.....|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int i  = ((iw0 >> DagMODim_i_bits) & DagMODim_i_mask);
  int m  = ((iw0 >> DagMODim_m_bits) & DagMODim_m_mask);
  int br = ((iw0 >> DagMODim_br_bits) & DagMODim_br_mask);
  int op = ((iw0 >> DagMODim_op_bits) & DagMODim_op_mask);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_dagMODim);
  TRACE_EXTRACT (cpu, "%s: br:%i op:%i m:%i i:%i", __func__, br, op, m, i);

  if (PARALLEL_GROUP == BFIN_PARALLEL_GROUP2)
    illegal_instruction_combination (cpu);

  if (op == 0 && br == 1)
    {
      TRACE_INSN (cpu, "I%i += M%i (BREV);", i, m);
      SET_IREG (i, add_brev (IREG (i), MREG (m)));
    }
  else if (op == 0)
    {
      TRACE_INSN (cpu, "I%i += M%i;", i, m);
      dagadd (cpu, i, MREG (m));
    }
  else if (op == 1 && br == 0)
    {
      TRACE_INSN (cpu, "I%i -= M%i;", i, m);
      dagsub (cpu, i, MREG (m));
    }
  else
    illegal_instruction_or_combination (cpu);
}

static void
decode_dagMODik_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* dagMODik
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 0 | 0 | 1 | 1 | 1 | 1 | 1 | 0 | 1 | 1 | 0 |.op....|.i.....|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int i  = ((iw0 >> DagMODik_i_bits) & DagMODik_i_mask);
  int op = ((iw0 >> DagMODik_op_bits) & DagMODik_op_mask);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_dagMODik);
  TRACE_EXTRACT (cpu, "%s: op:%i i:%i", __func__, op, i);

  if (PARALLEL_GROUP == BFIN_PARALLEL_GROUP2)
    illegal_instruction_combination (cpu);

  if (op == 0)
    {
      TRACE_INSN (cpu, "I%i += 2;", i);
      dagadd (cpu, i, 2);
    }
  else if (op == 1)
    {
      TRACE_INSN (cpu, "I%i -= 2;", i);
      dagsub (cpu, i, 2);
    }
  else if (op == 2)
    {
      TRACE_INSN (cpu, "I%i += 4;", i);
      dagadd (cpu, i, 4);
    }
  else if (op == 3)
    {
      TRACE_INSN (cpu, "I%i -= 4;", i);
      dagsub (cpu, i, 4);
    }
  else
    illegal_instruction_or_combination (cpu);
}

static void
decode_dspLDST_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* dspLDST
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 0 | 0 | 1 | 1 | 1 |.W.|.aop...|.m.....|.i.....|.reg.......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int i   = ((iw0 >> DspLDST_i_bits) & DspLDST_i_mask);
  int m   = ((iw0 >> DspLDST_m_bits) & DspLDST_m_mask);
  int W   = ((iw0 >> DspLDST_W_bits) & DspLDST_W_mask);
  int aop = ((iw0 >> DspLDST_aop_bits) & DspLDST_aop_mask);
  int reg = ((iw0 >> DspLDST_reg_bits) & DspLDST_reg_mask);
  bu32 addr;

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_dspLDST);
  TRACE_EXTRACT (cpu, "%s: aop:%i m:%i i:%i reg:%i", __func__, aop, m, i, reg);

  if (aop == 0 && W == 0 && m == 0)
    {
      TRACE_INSN (cpu, "R%i = [I%i++];", reg, i);
      addr = IREG (i);
      if (DIS_ALGN_EXPT & 0x1)
	addr &= ~3;
      dagadd (cpu, i, 4);
      STORE (DREG (reg), GET_LONG (addr));
    }
  else if (aop == 0 && W == 0 && m == 1)
    {
      TRACE_INSN (cpu, "R%i.L = W[I%i++];", reg, i);
      addr = IREG (i);
      dagadd (cpu, i, 2);
      STORE (DREG (reg), (DREG (reg) & 0xFFFF0000) | GET_WORD (addr));
    }
  else if (aop == 0 && W == 0 && m == 2)
    {
      TRACE_INSN (cpu, "R%i.H = W[I%i++];", reg, i);
      addr = IREG (i);
      dagadd (cpu, i, 2);
      STORE (DREG (reg), (DREG (reg) & 0xFFFF) | (GET_WORD (addr) << 16));
    }
  else if (aop == 1 && W == 0 && m == 0)
    {
      TRACE_INSN (cpu, "R%i = [I%i--];", reg, i);
      addr = IREG (i);
      if (DIS_ALGN_EXPT & 0x1)
	addr &= ~3;
      dagsub (cpu, i, 4);
      STORE (DREG (reg), GET_LONG (addr));
    }
  else if (aop == 1 && W == 0 && m == 1)
    {
      TRACE_INSN (cpu, "R%i.L = W[I%i--];", reg, i);
      addr = IREG (i);
      dagsub (cpu, i, 2);
      STORE (DREG (reg), (DREG (reg) & 0xFFFF0000) | GET_WORD (addr));
    }
  else if (aop == 1 && W == 0 && m == 2)
    {
      TRACE_INSN (cpu, "R%i.H = W[I%i--];", reg, i);
      addr = IREG (i);
      dagsub (cpu, i, 2);
      STORE (DREG (reg), (DREG (reg) & 0xFFFF) | (GET_WORD (addr) << 16));
    }
  else if (aop == 2 && W == 0 && m == 0)
    {
      TRACE_INSN (cpu, "R%i = [I%i];", reg, i);
      addr = IREG (i);
      if (DIS_ALGN_EXPT & 0x1)
	addr &= ~3;
      STORE (DREG (reg), GET_LONG (addr));
    }
  else if (aop == 2 && W == 0 && m == 1)
    {
      TRACE_INSN (cpu, "R%i.L = W[I%i];", reg, i);
      addr = IREG (i);
      STORE (DREG (reg), (DREG (reg) & 0xFFFF0000) | GET_WORD (addr));
    }
  else if (aop == 2 && W == 0 && m == 2)
    {
      TRACE_INSN (cpu, "R%i.H = W[I%i];", reg, i);
      addr = IREG (i);
      STORE (DREG (reg), (DREG (reg) & 0xFFFF) | (GET_WORD (addr) << 16));
    }
  else if (aop == 0 && W == 1 && m == 0)
    {
      TRACE_INSN (cpu, "[I%i++] = R%i;", i, reg);
      addr = IREG (i);
      dagadd (cpu, i, 4);
      PUT_LONG (addr, DREG (reg));
    }
  else if (aop == 0 && W == 1 && m == 1)
    {
      TRACE_INSN (cpu, "W[I%i++] = R%i.L;", i, reg);
      addr = IREG (i);
      dagadd (cpu, i, 2);
      PUT_WORD (addr, DREG (reg));
    }
  else if (aop == 0 && W == 1 && m == 2)
    {
      TRACE_INSN (cpu, "W[I%i++] = R%i.H;", i, reg);
      addr = IREG (i);
      dagadd (cpu, i, 2);
      PUT_WORD (addr, DREG (reg) >> 16);
    }
  else if (aop == 1 && W == 1 && m == 0)
    {
      TRACE_INSN (cpu, "[I%i--] = R%i;", i, reg);
      addr = IREG (i);
      dagsub (cpu, i, 4);
      PUT_LONG (addr, DREG (reg));
    }
  else if (aop == 1 && W == 1 && m == 1)
    {
      TRACE_INSN (cpu, "W[I%i--] = R%i.L;", i, reg);
      addr = IREG (i);
      dagsub (cpu, i, 2);
      PUT_WORD (addr, DREG (reg));
    }
  else if (aop == 1 && W == 1 && m == 2)
    {
      TRACE_INSN (cpu, "W[I%i--] = R%i.H;", i, reg);
      addr = IREG (i);
      dagsub (cpu, i, 2);
      PUT_WORD (addr, DREG (reg) >> 16);
    }
  else if (aop == 2 && W == 1 && m == 0)
    {
      TRACE_INSN (cpu, "[I%i] = R%i;", i, reg);
      addr = IREG (i);
      PUT_LONG (addr, DREG (reg));
    }
  else if (aop == 2 && W == 1 && m == 1)
    {
      TRACE_INSN (cpu, "W[I%i] = R%i.L;", i, reg);
      addr = IREG (i);
      PUT_WORD (addr, DREG (reg));
    }
  else if (aop == 2 && W == 1 && m == 2)
    {
      TRACE_INSN (cpu, "W[I%i] = R%i.H;", i, reg);
      addr = IREG (i);
      PUT_WORD (addr, DREG (reg) >> 16);
    }
  else if (aop == 3 && W == 0)
    {
      TRACE_INSN (cpu, "R%i = [I%i ++ M%i];", reg, i, m);
      addr = IREG (i);
      if (DIS_ALGN_EXPT & 0x1)
	addr &= ~3;
      dagadd (cpu, i, MREG (m));
      STORE (DREG (reg), GET_LONG (addr));
    }
  else if (aop == 3 && W == 1)
    {
      TRACE_INSN (cpu, "[I%i ++ M%i] = R%i;", i, m, reg);
      addr = IREG (i);
      dagadd (cpu, i, MREG (m));
      PUT_LONG (addr, DREG (reg));
    }
  else
    illegal_instruction_or_combination (cpu);
}

static void
decode_LDST_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* LDST
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 0 | 0 | 1 |.sz....|.W.|.aop...|.Z.|.ptr.......|.reg.......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int Z   = ((iw0 >> LDST_Z_bits) & LDST_Z_mask);
  int W   = ((iw0 >> LDST_W_bits) & LDST_W_mask);
  int sz  = ((iw0 >> LDST_sz_bits) & LDST_sz_mask);
  int aop = ((iw0 >> LDST_aop_bits) & LDST_aop_mask);
  int reg = ((iw0 >> LDST_reg_bits) & LDST_reg_mask);
  int ptr = ((iw0 >> LDST_ptr_bits) & LDST_ptr_mask);
  const char * const posts[] = { "++", "--", "", "<INV>" };
  const char *post = posts[aop];
  const char *ptr_name = get_preg_name (ptr);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_LDST);
  TRACE_EXTRACT (cpu, "%s: sz:%i W:%i aop:%i Z:%i ptr:%i reg:%i",
		 __func__, sz, W, aop, Z, ptr, reg);

  if (aop == 3 || PARALLEL_GROUP == BFIN_PARALLEL_GROUP2)
    illegal_instruction_or_combination (cpu);

  if (W == 0)
    {
      if (sz == 0 && Z == 0)
	{
	  TRACE_INSN (cpu, "R%i = [%s%s];", reg, ptr_name, post);
	  SET_DREG (reg, GET_LONG (PREG (ptr)));
	}
      else if (sz == 0 && Z == 1)
	{
	  TRACE_INSN (cpu, "%s = [%s%s];", get_preg_name (reg), ptr_name, post);
	  if (aop < 2 && ptr == reg)
	    illegal_instruction_combination (cpu);
	  SET_PREG (reg, GET_LONG (PREG (ptr)));
	}
      else if (sz == 1 && Z == 0)
	{
	  TRACE_INSN (cpu, "R%i = W[%s%s] (Z);", reg, ptr_name, post);
	  SET_DREG (reg, GET_WORD (PREG (ptr)));
	}
      else if (sz == 1 && Z == 1)
	{
	  TRACE_INSN (cpu, "R%i = W[%s%s] (X);", reg, ptr_name, post);
	  SET_DREG (reg, (bs32) (bs16) GET_WORD (PREG (ptr)));
	}
      else if (sz == 2 && Z == 0)
	{
	  TRACE_INSN (cpu, "R%i = B[%s%s] (Z);", reg, ptr_name, post);
	  SET_DREG (reg, GET_BYTE (PREG (ptr)));
	}
      else if (sz == 2 && Z == 1)
	{
	  TRACE_INSN (cpu, "R%i = B[%s%s] (X);", reg, ptr_name, post);
	  SET_DREG (reg, (bs32) (bs8) GET_BYTE (PREG (ptr)));
	}
      else
	illegal_instruction_or_combination (cpu);
    }
  else
    {
      if (sz == 0 && Z == 0)
	{
	  TRACE_INSN (cpu, "[%s%s] = R%i;", ptr_name, post, reg);
	  PUT_LONG (PREG (ptr), DREG (reg));
	}
      else if (sz == 0 && Z == 1)
	{
	  TRACE_INSN (cpu, "[%s%s] = %s;", ptr_name, post, get_preg_name (reg));
	  PUT_LONG (PREG (ptr), PREG (reg));
	}
      else if (sz == 1 && Z == 0)
	{
	  TRACE_INSN (cpu, "W[%s%s] = R%i;", ptr_name, post, reg);
	  PUT_WORD (PREG (ptr), DREG (reg));
	}
      else if (sz == 2 && Z == 0)
	{
	  TRACE_INSN (cpu, "B[%s%s] = R%i;", ptr_name, post, reg);
	  PUT_BYTE (PREG (ptr), DREG (reg));
	}
      else
	illegal_instruction_or_combination (cpu);
    }

  if (aop == 0)
    SET_PREG (ptr, PREG (ptr) + (1 << (2 - sz)));
  if (aop == 1)
    SET_PREG (ptr, PREG (ptr) - (1 << (2 - sz)));
}

static void
decode_LDSTiiFP_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* LDSTiiFP
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 0 | 1 | 1 | 1 | 0 |.W.|.offset............|.reg...........|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  /* This isn't exactly a grp:reg as this insn only supports Dregs & Pregs,
     but for our usage, its functionality the same thing.  */
  int grp = ((iw0 >> 3) & 0x1);
  int reg = ((iw0 >> LDSTiiFP_reg_bits) & 0x7 /*LDSTiiFP_reg_mask*/);
  int offset = ((iw0 >> LDSTiiFP_offset_bits) & LDSTiiFP_offset_mask);
  int W = ((iw0 >> LDSTiiFP_W_bits) & LDSTiiFP_W_mask);
  bu32 imm = negimm5s4 (offset);
  bu32 ea = FPREG + imm;
  const char *imm_str = negimm5s4_str (offset);
  const char *reg_name = get_allreg_name (grp, reg);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_LDSTiiFP);
  TRACE_EXTRACT (cpu, "%s: W:%i offset:%#x grp:%i reg:%i", __func__,
		 W, offset, grp, reg);
  TRACE_DECODE (cpu, "%s: negimm5s4:%#x", __func__, imm);

  if (PARALLEL_GROUP == BFIN_PARALLEL_GROUP2)
    illegal_instruction_or_combination (cpu);

  if (W == 0)
    {
      TRACE_INSN (cpu, "%s = [FP + %s];", reg_name, imm_str);
      reg_write (cpu, grp, reg, GET_LONG (ea));
    }
  else
    {
      TRACE_INSN (cpu, "[FP + %s] = %s;", imm_str, reg_name);
      PUT_LONG (ea, reg_read (cpu, grp, reg));
    }
}

static void
decode_LDSTii_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* LDSTii
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 0 | 1 |.W.|.op....|.offset........|.ptr.......|.reg.......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int reg = ((iw0 >> LDSTii_reg_bit) & LDSTii_reg_mask);
  int ptr = ((iw0 >> LDSTii_ptr_bit) & LDSTii_ptr_mask);
  int offset = ((iw0 >> LDSTii_offset_bit) & LDSTii_offset_mask);
  int op = ((iw0 >> LDSTii_op_bit) & LDSTii_op_mask);
  int W = ((iw0 >> LDSTii_W_bit) & LDSTii_W_mask);
  bu32 imm, ea;
  const char *imm_str;
  const char *ptr_name = get_preg_name (ptr);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_LDSTii);
  TRACE_EXTRACT (cpu, "%s: W:%i op:%i offset:%#x ptr:%i reg:%i",
		 __func__, W, op, offset, ptr, reg);

  if (op == 0 || op == 3)
    imm = uimm4s4 (offset), imm_str = uimm4s4_str (offset);
  else
    imm = uimm4s2 (offset), imm_str = uimm4s2_str (offset);
  ea = PREG (ptr) + imm;

  TRACE_DECODE (cpu, "%s: uimm4s4/uimm4s2:%#x", __func__, imm);

  if (PARALLEL_GROUP == BFIN_PARALLEL_GROUP2)
    illegal_instruction_combination (cpu);

  if (W == 1 && op == 2)
    illegal_instruction (cpu);

  if (W == 0)
    {
      if (op == 0)
	{
	  TRACE_INSN (cpu, "R%i = [%s + %s];", reg, ptr_name, imm_str);
	  SET_DREG (reg, GET_LONG (ea));
	}
      else if (op == 1)
	{
	  TRACE_INSN (cpu, "R%i = W[%s + %s] (Z);", reg, ptr_name, imm_str);
	  SET_DREG (reg, GET_WORD (ea));
	}
      else if (op == 2)
	{
	  TRACE_INSN (cpu, "R%i = W[%s + %s] (X);", reg, ptr_name, imm_str);
	  SET_DREG (reg, (bs32) (bs16) GET_WORD (ea));
	}
      else if (op == 3)
	{
	  TRACE_INSN (cpu, "%s = [%s + %s];",
		      get_preg_name (reg), ptr_name, imm_str);
	  SET_PREG (reg, GET_LONG (ea));
	}
    }
  else
    {
      if (op == 0)
	{
	  TRACE_INSN (cpu, "[%s + %s] = R%i;", ptr_name, imm_str, reg);
	  PUT_LONG (ea, DREG (reg));
	}
      else if (op == 1)
	{
	  TRACE_INSN (cpu, "W[%s + %s] = R%i;", ptr_name, imm_str, reg);
	  PUT_WORD (ea, DREG (reg));
	}
      else if (op == 3)
	{
	  TRACE_INSN (cpu, "[%s + %s] = %s;",
		      ptr_name, imm_str, get_preg_name (reg));
	  PUT_LONG (ea, PREG (reg));
	}
    }
}

static void
decode_LoopSetup_0 (SIM_CPU *cpu, bu16 iw0, bu16 iw1, bu32 pc)
{
  /* LoopSetup
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 1 | 1 | 0 | 0 | 0 | 0 | 0 | 1 |.rop...|.c.|.soffset.......|
     |.reg...........| - | - |.eoffset...............................|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int c   = ((iw0 >> (LoopSetup_c_bits - 16)) & LoopSetup_c_mask);
  int reg = ((iw1 >> LoopSetup_reg_bits) & LoopSetup_reg_mask);
  int rop = ((iw0 >> (LoopSetup_rop_bits - 16)) & LoopSetup_rop_mask);
  int soffset = ((iw0 >> (LoopSetup_soffset_bits - 16)) & LoopSetup_soffset_mask);
  int eoffset = ((iw1 >> LoopSetup_eoffset_bits) & LoopSetup_eoffset_mask);
  int spcrel = pcrel4 (soffset);
  int epcrel = lppcrel10 (eoffset);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_LoopSetup);
  TRACE_EXTRACT (cpu, "%s: rop:%i c:%i soffset:%i reg:%i eoffset:%i",
		 __func__, rop, c, soffset, reg, eoffset);
  TRACE_DECODE (cpu, "%s: s_pcrel4:%#x e_lppcrel10:%#x",
		__func__, spcrel, epcrel);

  if (reg > 7)
    illegal_instruction (cpu);

  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    illegal_instruction_combination (cpu);

  if (rop == 0)
    {
      TRACE_INSN (cpu, "LSETUP (%#x, %#x) LC%i;", spcrel, epcrel, c);
    }
  else if (rop == 1 && reg <= 7)
    {
      TRACE_INSN (cpu, "LSETUP (%#x, %#x) LC%i = %s;",
		  spcrel, epcrel, c, get_preg_name (reg));
      SET_LCREG (c, PREG (reg));
    }
  else if (rop == 3 && reg <= 7)
    {
      TRACE_INSN (cpu, "LSETUP (%#x, %#x) LC%i = %s >> 1;",
		  spcrel, epcrel, c, get_preg_name (reg));
      SET_LCREG (c, PREG (reg) >> 1);
    }
  else
    illegal_instruction (cpu);

  SET_LTREG (c, pc + spcrel);
  SET_LBREG (c, pc + epcrel);
}

static void
decode_LDIMMhalf_0 (SIM_CPU *cpu, bu16 iw0, bu16 iw1)
{
  /* LDIMMhalf
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 1 | 1 | 0 | 0 | 0 | 0 | 1 |.Z.|.H.|.S.|.grp...|.reg.......|
     |.hword.........................................................|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int H = ((iw0 >> (LDIMMhalf_H_bits - 16)) & LDIMMhalf_H_mask);
  int Z = ((iw0 >> (LDIMMhalf_Z_bits - 16)) & LDIMMhalf_Z_mask);
  int S = ((iw0 >> (LDIMMhalf_S_bits - 16)) & LDIMMhalf_S_mask);
  int reg = ((iw0 >> (LDIMMhalf_reg_bits - 16)) & LDIMMhalf_reg_mask);
  int grp = ((iw0 >> (LDIMMhalf_grp_bits - 16)) & LDIMMhalf_grp_mask);
  int hword = ((iw1 >> LDIMMhalf_hword_bits) & LDIMMhalf_hword_mask);
  bu32 val;
  const char *val_str;
  const char *reg_name = get_allreg_name (grp, reg);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_LDIMMhalf);
  TRACE_EXTRACT (cpu, "%s: Z:%i H:%i S:%i grp:%i reg:%i hword:%#x",
		 __func__, Z, H, S, grp, reg, hword);

  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    illegal_instruction_combination (cpu);

  if (S == 1)
    val = imm16 (hword), val_str = imm16_str (hword);
  else
    val = luimm16 (hword), val_str = luimm16_str (hword);

  if (H == 0 && S == 1 && Z == 0)
    {
      TRACE_INSN (cpu, "%s = %s (X);", reg_name, val_str);
    }
  else if (H == 0 && S == 0 && Z == 1)
    {
      TRACE_INSN (cpu, "%s = %s (Z);", reg_name, val_str);
    }
  else if (H == 0 && S == 0 && Z == 0)
    {
      TRACE_INSN (cpu, "%s.L = %s;", reg_name, val_str);
      val = REG_H_L (reg_read (cpu, grp, reg), val);
    }
  else if (H == 1 && S == 0 && Z == 0)
    {
      TRACE_INSN (cpu, "%s.H = %s;", reg_name, val_str);
      val = REG_H_L (val << 16, reg_read (cpu, grp, reg));
    }
  else
    illegal_instruction (cpu);

  reg_write (cpu, grp, reg, val);
}

static void
decode_CALLa_0 (SIM_CPU *cpu, bu16 iw0, bu16 iw1, bu32 pc)
{
  /* CALLa
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 1 | 1 | 0 | 0 | 0 | 1 |.S.|.msw...........................|
     |.lsw...........................................................|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int S   = ((iw0 >> (CALLa_S_bits - 16)) & CALLa_S_mask);
  int lsw = ((iw1 >> 0) & 0xffff);
  int msw = ((iw0 >> 0) & 0xff);
  int pcrel = pcrel24 ((msw << 16) | lsw);
  bu32 newpc = pc + pcrel;

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_CALLa);
  TRACE_EXTRACT (cpu, "%s: S:%i msw:%#x lsw:%#x", __func__, S, msw, lsw);
  TRACE_DECODE (cpu, "%s: pcrel24:%#x", __func__, pcrel);

  TRACE_INSN (cpu, "%s %#x;", S ? "CALL" : "JUMP.L", pcrel);

  if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
    illegal_instruction_combination (cpu);

  if (S == 1)
    {
      BFIN_TRACE_BRANCH (cpu, pc, newpc, -1, "CALL");
      SET_RETSREG (hwloop_get_next_pc (cpu, pc, 4));
    }
  else
    BFIN_TRACE_BRANCH (cpu, pc, newpc, -1, "JUMP.L");

  SET_PCREG (newpc);
  BFIN_CPU_STATE.did_jump = true;
  PROFILE_BRANCH_TAKEN (cpu);
  CYCLE_DELAY = 5;
}

static void
decode_LDSTidxI_0 (SIM_CPU *cpu, bu16 iw0, bu16 iw1)
{
  /* LDSTidxI
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 1 | 1 | 0 | 0 | 1 |.W.|.Z.|.sz....|.ptr.......|.reg.......|
     |.offset........................................................|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int Z = ((iw0 >> (LDSTidxI_Z_bits - 16)) & LDSTidxI_Z_mask);
  int W = ((iw0 >> (LDSTidxI_W_bits - 16)) & LDSTidxI_W_mask);
  int sz = ((iw0 >> (LDSTidxI_sz_bits - 16)) & LDSTidxI_sz_mask);
  int reg = ((iw0 >> (LDSTidxI_reg_bits - 16)) & LDSTidxI_reg_mask);
  int ptr = ((iw0 >> (LDSTidxI_ptr_bits - 16)) & LDSTidxI_ptr_mask);
  int offset = ((iw1 >> LDSTidxI_offset_bits) & LDSTidxI_offset_mask);
  const char *ptr_name = get_preg_name (ptr);
  bu32 imm_16s4 = imm16s4 (offset);
  bu32 imm_16s2 = imm16s2 (offset);
  bu32 imm_16 = imm16 (offset);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_LDSTidxI);
  TRACE_EXTRACT (cpu, "%s: W:%i Z:%i sz:%i ptr:%i reg:%i offset:%#x",
		 __func__, W, Z, sz, ptr, reg, offset);

  if (sz == 3)
    illegal_instruction (cpu);

  if (W == 0)
    {
      if (sz == 0 && Z == 0)
	{
	  TRACE_INSN (cpu, "R%i = [%s + %s];",
		      reg, ptr_name, imm16s4_str (offset));
	  SET_DREG (reg, GET_LONG (PREG (ptr) + imm_16s4));
	}
      else if (sz == 0 && Z == 1)
	{
	  TRACE_INSN (cpu, "%s = [%s + %s];",
		      get_preg_name (reg), ptr_name, imm16s4_str (offset));
	  SET_PREG (reg, GET_LONG (PREG (ptr) + imm_16s4));
	}
      else if (sz == 1 && Z == 0)
	{
	  TRACE_INSN (cpu, "R%i = W[%s + %s] (Z);",
		      reg, ptr_name, imm16s2_str (offset));
	  SET_DREG (reg, GET_WORD (PREG (ptr) + imm_16s2));
	}
      else if (sz == 1 && Z == 1)
	{
	  TRACE_INSN (cpu, "R%i = W[%s + %s] (X);",
		      reg, ptr_name, imm16s2_str (offset));
	  SET_DREG (reg, (bs32) (bs16) GET_WORD (PREG (ptr) + imm_16s2));
	}
      else if (sz == 2 && Z == 0)
	{
	  TRACE_INSN (cpu, "R%i = B[%s + %s] (Z);",
		      reg, ptr_name, imm16_str (offset));
	  SET_DREG (reg, GET_BYTE (PREG (ptr) + imm_16));
	}
      else if (sz == 2 && Z == 1)
	{
	  TRACE_INSN (cpu, "R%i = B[%s + %s] (X);",
		      reg, ptr_name, imm16_str (offset));
	  SET_DREG (reg, (bs32) (bs8) GET_BYTE (PREG (ptr) + imm_16));
	}
    }
  else
    {
      if (sz != 0 && Z != 0)
	illegal_instruction (cpu);

      if (sz == 0 && Z == 0)
	{
	  TRACE_INSN (cpu, "[%s + %s] = R%i;", ptr_name,
		      imm16s4_str (offset), reg);
	  PUT_LONG (PREG (ptr) + imm_16s4, DREG (reg));
	}
      else if (sz == 0 && Z == 1)
	{
	  TRACE_INSN (cpu, "[%s + %s] = %s;",
		      ptr_name, imm16s4_str (offset), get_preg_name (reg));
	  PUT_LONG (PREG (ptr) + imm_16s4, PREG (reg));
	}
      else if (sz == 1 && Z == 0)
	{
	  TRACE_INSN (cpu, "W[%s + %s] = R%i;",
		      ptr_name, imm16s2_str (offset), reg);
	  PUT_WORD (PREG (ptr) + imm_16s2, DREG (reg));
	}
      else if (sz == 2 && Z == 0)
	{
	  TRACE_INSN (cpu, "B[%s + %s] = R%i;",
		      ptr_name, imm16_str (offset), reg);
	  PUT_BYTE (PREG (ptr) + imm_16, DREG (reg));
	}
    }
}

static void
decode_linkage_0 (SIM_CPU *cpu, bu16 iw0, bu16 iw1)
{
  /* linkage
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 1 | 1 | 0 | 1 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |.R.|
     |.framesize.....................................................|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int R = ((iw0 >> (Linkage_R_bits - 16)) & Linkage_R_mask);
  int framesize = ((iw1 >> Linkage_framesize_bits) & Linkage_framesize_mask);
  bu32 sp;

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_linkage);
  TRACE_EXTRACT (cpu, "%s: R:%i framesize:%#x", __func__, R, framesize);

  if (R == 0)
    {
      int size = uimm16s4 (framesize);
      sp = SPREG;
      TRACE_INSN (cpu, "LINK %s;", uimm16s4_str (framesize));
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      sp -= 4;
      PUT_LONG (sp, RETSREG);
      sp -= 4;
      PUT_LONG (sp, FPREG);
      SET_FPREG (sp);
      sp -= size;
      CYCLE_DELAY = 3;
    }
  else
    {
      /* Restore SP from FP.  */
      sp = FPREG;
      TRACE_INSN (cpu, "UNLINK;");
      if (PARALLEL_GROUP != BFIN_PARALLEL_NONE)
	illegal_instruction_combination (cpu);
      SET_FPREG (GET_LONG (sp));
      sp += 4;
      SET_RETSREG (GET_LONG (sp));
      sp += 4;
      CYCLE_DELAY = 2;
    }

  SET_SPREG (sp);
}

static void
decode_dsp32mac_0 (SIM_CPU *cpu, bu16 iw0, bu16 iw1)
{
  /* dsp32mac
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 1 | 0 | 0 |.M.| 0 | 0 |.mmod..........|.MM|.P.|.w1|.op1...|
     |.h01|.h11|.w0|.op0...|.h00|.h10|.dst.......|.src0......|.src1..|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int op1  = ((iw0 >> (DSP32Mac_op1_bits - 16)) & DSP32Mac_op1_mask);
  int w1   = ((iw0 >> (DSP32Mac_w1_bits - 16)) & DSP32Mac_w1_mask);
  int P    = ((iw0 >> (DSP32Mac_p_bits - 16)) & DSP32Mac_p_mask);
  int MM   = ((iw0 >> (DSP32Mac_MM_bits - 16)) & DSP32Mac_MM_mask);
  int mmod = ((iw0 >> (DSP32Mac_mmod_bits - 16)) & DSP32Mac_mmod_mask);
  int M    = ((iw0 >> (DSP32Mac_M_bits - 16)) & DSP32Mac_M_mask);
  int w0   = ((iw1 >> DSP32Mac_w0_bits) & DSP32Mac_w0_mask);
  int src0 = ((iw1 >> DSP32Mac_src0_bits) & DSP32Mac_src0_mask);
  int src1 = ((iw1 >> DSP32Mac_src1_bits) & DSP32Mac_src1_mask);
  int dst  = ((iw1 >> DSP32Mac_dst_bits) & DSP32Mac_dst_mask);
  int h10  = ((iw1 >> DSP32Mac_h10_bits) & DSP32Mac_h10_mask);
  int h00  = ((iw1 >> DSP32Mac_h00_bits) & DSP32Mac_h00_mask);
  int op0  = ((iw1 >> DSP32Mac_op0_bits) & DSP32Mac_op0_mask);
  int h11  = ((iw1 >> DSP32Mac_h11_bits) & DSP32Mac_h11_mask);
  int h01  = ((iw1 >> DSP32Mac_h01_bits) & DSP32Mac_h01_mask);

  bu32 res = DREG (dst);
  bu32 v_0 = 0, v_1 = 0, zero = 0, n_1 = 0, n_0 = 0;

  static const char * const ops[] = { "=", "+=", "-=" };
  char _buf[128], *buf = _buf;
  int _MM = MM;

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_dsp32mac);
  TRACE_EXTRACT (cpu, "%s: M:%i mmod:%i MM:%i P:%i w1:%i op1:%i h01:%i h11:%i "
		      "w0:%i op0:%i h00:%i h10:%i dst:%i src0:%i src1:%i",
		 __func__, M, mmod, MM, P, w1, op1, h01, h11, w0, op0, h00, h10,
		 dst, src0, src1);

  if (w0 == 0 && w1 == 0 && op1 == 3 && op0 == 3)
    illegal_instruction (cpu);

  if ((w1 || w0) && mmod == M_W32)
    illegal_instruction (cpu);

  if (((1 << mmod) & (P ? 0x131b : 0x1b5f)) == 0)
    illegal_instruction (cpu);

  /* First handle MAC1 side.  */
  if (w1 == 1 || op1 != 3)
    {
      bu32 res1 = decode_macfunc (cpu, 1, op1, h01, h11, src0,
				  src1, mmod, MM, P, &v_1, &n_1);

      if (w1)
	buf += sprintf (buf, P ? "R%i" : "R%i.H", dst + P);

      if (op1 == 3)
	{
	  buf += sprintf (buf, " = A1");
	  zero = !!(res1 == 0);
	}
      else
	{
	  if (w1)
	    buf += sprintf (buf, " = (");
	  buf += sprintf (buf, "A1 %s R%i.%c * R%i.%c", ops[op1],
			  src0, h01 ? 'H' : 'L',
			  src1, h11 ? 'H' : 'L');
	  if (w1)
	    buf += sprintf (buf, ")");
	}

      if (w1)
	{
	  if (P)
	    STORE (DREG (dst + 1), res1);
	  else
	    {
	      if (res1 & 0xffff0000)
		illegal_instruction (cpu);
	      res = REG_H_L (res1 << 16, res);
	    }
	}
      else
	v_1 = 0;

      if (w0 == 1 || op0 != 3)
	{
	  if (_MM)
	    buf += sprintf (buf, " (M)");
	  _MM = 0;
	  buf += sprintf (buf, ", ");
	}
    }

  /* Then handle MAC0 side.  */
  if (w0 == 1 || op0 != 3)
    {
      bu32 res0 = decode_macfunc (cpu, 0, op0, h00, h10, src0,
				  src1, mmod, 0, P, &v_0, &n_0);

      if (w0)
	buf += sprintf (buf, P ? "R%i" : "R%i.L", dst);

      if (op0 == 3)
	{
	  buf += sprintf (buf, " = A0");
	  zero |= !!(res0 == 0);
	}
      else
	{
	  if (w0)
	    buf += sprintf (buf, " = (");
	  buf += sprintf (buf, "A0 %s R%i.%c * R%i.%c", ops[op0],
			  src0, h00 ? 'H' : 'L',
			  src1, h10 ? 'H' : 'L');
	  if (w0)
	    buf += sprintf (buf, ")");
	}

      if (w0)
	{
	  if (P)
	    STORE (DREG (dst), res0);
	  else
	    {
	      if (res0 & 0xffff0000)
		illegal_instruction (cpu);
	      res = REG_H_L (res, res0);
	    }
	}
      else
	v_0 = 0;
    }

  TRACE_INSN (cpu, "%s%s;", _buf, mac_optmode (mmod, _MM));

  if (!P && (w0 || w1))
    {
      STORE (DREG (dst), res);
      SET_ASTATREG (v, v_0 | v_1);
      if (v_0 || v_1)
	SET_ASTATREG (vs, 1);
    }
  else if (P)
    {
      SET_ASTATREG (v, v_0 | v_1);
      if (v_0 || v_1)
	SET_ASTATREG (vs, 1);
    }

  if ((w0 == 1 && op0 == 3) || (w1 == 1 && op1 == 3))
    {
      SET_ASTATREG (az, zero);
      if (!(w0 == 1 && op0 == 3))
	n_0 = 0;
      if (!(w1 == 1 && op1 == 3))
	n_1 = 0;
      SET_ASTATREG (an, n_1 | n_0);
    }
}

static void
decode_dsp32mult_0 (SIM_CPU *cpu, bu16 iw0, bu16 iw1)
{
  /* dsp32mult
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 1 | 0 | 0 |.M.| 0 | 1 |.mmod..........|.MM|.P.|.w1|.op1...|
     |.h01|.h11|.w0|.op0...|.h00|.h10|.dst.......|.src0......|.src1..|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int op1  = ((iw0 >> (DSP32Mac_op1_bits - 16)) & DSP32Mac_op1_mask);
  int w1   = ((iw0 >> (DSP32Mac_w1_bits - 16)) & DSP32Mac_w1_mask);
  int P    = ((iw0 >> (DSP32Mac_p_bits - 16)) & DSP32Mac_p_mask);
  int MM   = ((iw0 >> (DSP32Mac_MM_bits - 16)) & DSP32Mac_MM_mask);
  int mmod = ((iw0 >> (DSP32Mac_mmod_bits - 16)) & DSP32Mac_mmod_mask);
  int M    = ((iw0 >> (DSP32Mac_M_bits - 16)) & DSP32Mac_M_mask);
  int w0   = ((iw1 >> DSP32Mac_w0_bits) & DSP32Mac_w0_mask);
  int src0 = ((iw1 >> DSP32Mac_src0_bits) & DSP32Mac_src0_mask);
  int src1 = ((iw1 >> DSP32Mac_src1_bits) & DSP32Mac_src1_mask);
  int dst  = ((iw1 >> DSP32Mac_dst_bits) & DSP32Mac_dst_mask);
  int h10  = ((iw1 >> DSP32Mac_h10_bits) & DSP32Mac_h10_mask);
  int h00  = ((iw1 >> DSP32Mac_h00_bits) & DSP32Mac_h00_mask);
  int op0  = ((iw1 >> DSP32Mac_op0_bits) & DSP32Mac_op0_mask);
  int h11  = ((iw1 >> DSP32Mac_h11_bits) & DSP32Mac_h11_mask);
  int h01  = ((iw1 >> DSP32Mac_h01_bits) & DSP32Mac_h01_mask);

  bu32 res = DREG (dst);
  bu32 sat0 = 0, sat1 = 0, v_i0 = 0, v_i1 = 0;
  char _buf[128], *buf = _buf;
  int _MM = MM;

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_dsp32mult);
  TRACE_EXTRACT (cpu, "%s: M:%i mmod:%i MM:%i P:%i w1:%i op1:%i h01:%i h11:%i "
		      "w0:%i op0:%i h00:%i h10:%i dst:%i src0:%i src1:%i",
		 __func__, M, mmod, MM, P, w1, op1, h01, h11, w0, op0, h00, h10,
		 dst, src0, src1);

  if (w1 == 0 && w0 == 0)
    illegal_instruction (cpu);
  if (((1 << mmod) & (P ? 0x313 : 0x1b57)) == 0)
    illegal_instruction (cpu);
  if (P && ((dst & 1) || (op1 != 0) || (op0 != 0) || !is_macmod_pmove (mmod)))
    illegal_instruction (cpu);
  if (!P && ((op1 != 0) || (op0 != 0) || !is_macmod_hmove (mmod)))
    illegal_instruction (cpu);

  /* First handle MAC1 side.  */
  if (w1)
    {
      bu64 r = decode_multfunc (cpu, h01, h11, src0, src1, mmod, MM, &sat1);
      bu32 res1 = extract_mult (cpu, r, mmod, MM, P, &v_i1);

      buf += sprintf (buf, P ? "R%i" : "R%i.H", dst + P);
      buf += sprintf (buf, " = R%i.%c * R%i.%c",
		      src0, h01 ? 'H' : 'L',
		      src1, h11 ? 'H' : 'L');
      if (w0)
	{
	  if (_MM)
	    buf += sprintf (buf, " (M)");
	  _MM = 0;
	  buf += sprintf (buf, ", ");
	}

      if (P)
	STORE (DREG (dst + 1), res1);
      else
	{
	  if (res1 & 0xFFFF0000)
	    illegal_instruction (cpu);
	  res = REG_H_L (res1 << 16, res);
	}
    }

  /* First handle MAC0 side.  */
  if (w0)
    {
      bu64 r = decode_multfunc (cpu, h00, h10, src0, src1, mmod, 0, &sat0);
      bu32 res0 = extract_mult (cpu, r, mmod, 0, P, &v_i0);

      buf += sprintf (buf, P ? "R%i" : "R%i.L", dst);
      buf += sprintf (buf, " = R%i.%c * R%i.%c",
		      src0, h01 ? 'H' : 'L',
		      src1, h11 ? 'H' : 'L');

      if (P)
	STORE (DREG (dst), res0);
      else
	{
	  if (res0 & 0xFFFF0000)
	    illegal_instruction (cpu);
	  res = REG_H_L (res, res0);
	}
    }

  TRACE_INSN (cpu, "%s%s;", _buf, mac_optmode (mmod, _MM));

  if (!P && (w0 || w1))
    STORE (DREG (dst), res);

  if (w0 || w1)
    {
      bu32 v = sat0 | sat1 | v_i0 | v_i1;

      STORE (ASTATREG (v), v);
      STORE (ASTATREG (v_copy), v);
      if (v)
	STORE (ASTATREG (vs), v);
    }
}

static void
decode_dsp32alu_0 (SIM_CPU *cpu, bu16 iw0, bu16 iw1)
{
  /* dsp32alu
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 1 | 0 | 0 |.M.| 1 | 0 | - | - | - |.HL|.aopcde............|
     |.aop...|.s.|.x.|.dst0......|.dst1......|.src0......|.src1......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int s    = ((iw1 >> DSP32Alu_s_bits) & DSP32Alu_s_mask);
  int x    = ((iw1 >> DSP32Alu_x_bits) & DSP32Alu_x_mask);
  int aop  = ((iw1 >> DSP32Alu_aop_bits) & DSP32Alu_aop_mask);
  int src0 = ((iw1 >> DSP32Alu_src0_bits) & DSP32Alu_src0_mask);
  int src1 = ((iw1 >> DSP32Alu_src1_bits) & DSP32Alu_src1_mask);
  int dst0 = ((iw1 >> DSP32Alu_dst0_bits) & DSP32Alu_dst0_mask);
  int dst1 = ((iw1 >> DSP32Alu_dst1_bits) & DSP32Alu_dst1_mask);
  int M    = ((iw0 >> (DSP32Alu_M_bits - 16)) & DSP32Alu_M_mask);
  int HL   = ((iw0 >> (DSP32Alu_HL_bits - 16)) & DSP32Alu_HL_mask);
  int aopcde = ((iw0 >> (DSP32Alu_aopcde_bits - 16)) & DSP32Alu_aopcde_mask);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_dsp32alu);
  TRACE_EXTRACT (cpu, "%s: M:%i HL:%i aopcde:%i aop:%i s:%i x:%i dst0:%i "
		      "dst1:%i src0:%i src1:%i",
		 __func__, M, HL, aopcde, aop, s, x, dst0, dst1, src0, src1);

  if ((aop == 0 || aop == 2) && aopcde == 9 && x == 0 && s == 0 && HL == 0)
    {
      int a = aop >> 1;
      TRACE_INSN (cpu, "A%i.L = R%i.L;", a, src0);
      SET_AWREG (a, REG_H_L (AWREG (a), DREG (src0)));
    }
  else if ((aop == 0 || aop == 2) && aopcde == 9 && x == 0 && s == 0 && HL == 1)
    {
      int a = aop >> 1;
      TRACE_INSN (cpu, "A%i.H = R%i.H;", a, src0);
      SET_AWREG (a, REG_H_L (DREG (src0), AWREG (a)));
    }
  else if ((aop == 1 || aop == 0) && aopcde == 5 && x == 0 && s == 0)
    {
      bs32 val0 = DREG (src0);
      bs32 val1 = DREG (src1);
      bs32 res;
      bs32 signRes;
      bs32 ovX, sBit1, sBit2, sBitRes1, sBitRes2;

      TRACE_INSN (cpu, "R%i.%s = R%i %s R%i (RND12)", dst0, HL ? "L" : "H",
		  src0, aop & 0x1 ? "-" : "+", src1);

      /* If subtract, just invert and add one.  */
      if (aop & 0x1)
	{
	  if (val1 == 0x80000000)
	    val1 = 0x7FFFFFFF;
	  else
	    val1 = ~val1 + 1;
	}

      /* Get the sign bits, since we need them later.  */
      sBit1 = !!(val0 & 0x80000000);
      sBit2 = !!(val1 & 0x80000000);

      res = val0 + val1;

      sBitRes1 = !!(res & 0x80000000);
      /* Round to the 12th bit.  */
      res += 0x0800;
      sBitRes2 = !!(res & 0x80000000);

      signRes = res;
      signRes >>= 27;

      /* Overflow if
           pos + pos = neg
           neg + neg = pos
           positive_res + positive_round = neg
         Shift and upper 4 bits where not the same.  */
      if ((!(sBit1 ^ sBit2) && (sBit1 ^ sBitRes1))
	  || (!sBit1 && !sBit2 && sBitRes2)
	  || ((signRes != 0) && (signRes != -1)))
	{
	  /* Both X1 and X2 Neg res is neg overflow.  */
	  if (sBit1 && sBit2)
	    res = 0x80000000;
	  /* Both X1 and X2 Pos res is pos overflow.  */
	  else if (!sBit1 && !sBit2)
	    res = 0x7FFFFFFF;
	  /* Pos+Neg or Neg+Pos take the sign of the result.  */
	  else if (sBitRes1)
	    res = 0x80000000;
	  else
	    res = 0x7FFFFFFF;

	  ovX = 1;
	}
      else
	{
	  /* Shift up now after overflow detection.  */
	  ovX = 0;
	  res <<= 4;
	}

      res >>= 16;

      if (HL)
	STORE (DREG (dst0), REG_H_L (res << 16, DREG (dst0)));
      else
	STORE (DREG (dst0), REG_H_L (DREG (dst0), res));

      SET_ASTATREG (az, res == 0);
      SET_ASTATREG (an, res & 0x8000);
      SET_ASTATREG (v, ovX);
      if (ovX)
	SET_ASTATREG (vs, ovX);
    }
  else if ((aop == 2 || aop == 3) && aopcde == 5 && x == 1 && s == 0)
    {
      bs32 val0 = DREG (src0);
      bs32 val1 = DREG (src1);
      bs32 res;

      TRACE_INSN (cpu, "R%i.%s = R%i %s R%i (RND20)", dst0, HL ? "L" : "H",
		  src0, aop & 0x1 ? "-" : "+", src1);

      /* If subtract, just invert and add one.  */
      if (aop & 0x1)
	val1 = ~val1 + 1;

      res = (val0 >> 4) + (val1 >> 4) + (((val0 & 0xf) + (val1 & 0xf)) >> 4);
      res += 0x8000;
      /* Don't sign extend during the shift.  */
      res = ((bu32)res >> 16);

      /* Don't worry about overflows, since we are shifting right.  */

      if (HL)
	STORE (DREG (dst0), REG_H_L (res << 16, DREG (dst0)));
      else
	STORE (DREG (dst0), REG_H_L (DREG (dst0), res));

      SET_ASTATREG (az, res == 0);
      SET_ASTATREG (an, res & 0x8000);
      SET_ASTATREG (v, 0);
    }
  else if ((aopcde == 2 || aopcde == 3) && x == 0)
    {
      bu32 s1, s2, val, ac0_i = 0, v_i = 0;

      TRACE_INSN (cpu, "R%i.%c = R%i.%c %c R%i.%c%s;",
		  dst0, HL ? 'H' : 'L',
		  src0, aop & 2 ? 'H' : 'L',
		  aopcde == 2 ? '+' : '-',
		  src1, aop & 1 ? 'H' : 'L',
		  amod1 (s, x));

      s1 = DREG (src0);
      s2 = DREG (src1);
      if (aop & 1)
	s2 >>= 16;
      if (aop & 2)
	s1 >>= 16;

      if (aopcde == 2)
	val = add16 (cpu, s1, s2, &ac0_i, &v_i, 0, 0, s, 0);
      else
	val = sub16 (cpu, s1, s2, &ac0_i, &v_i, 0, 0, s, 0);

      SET_ASTATREG (ac0, ac0_i);
      SET_ASTATREG (v, v_i);
      if (v_i)
	SET_ASTATREG (vs, v_i);

      if (HL)
	SET_DREG_H (dst0, val << 16);
      else
	SET_DREG_L (dst0, val);

      SET_ASTATREG (an, val & 0x8000);
      SET_ASTATREG (az, val == 0);
    }
  else if ((aop == 0 || aop == 2) && aopcde == 9 && x == 0 && s == 1 && HL == 0)
    {
      int a = aop >> 1;
      TRACE_INSN (cpu, "A%i = R%i;", a, src0);
      SET_AREG32 (a, DREG (src0));
    }
  else if ((aop == 1 || aop == 3) && aopcde == 9 && x == 0 && s == 0 && HL == 0)
    {
      int a = aop >> 1;
      TRACE_INSN (cpu, "A%i.X = R%i.L;", a, src0);
      SET_AXREG (a, (bs8)DREG (src0));
    }
  else if (aop == 3 && aopcde == 11 && x == 0 && HL == 0)
    {
      bu64 acc0 = get_extended_acc (cpu, 0);
      bu64 acc1 = get_extended_acc (cpu, 1);
      bu32 carry = (bu40)acc1 < (bu40)acc0;
      bu32 sat = 0;

      TRACE_INSN (cpu, "A0 -= A1%s;", s ? " (W32)" : "");

      acc0 -= acc1;
      if ((bs64)acc0 < -0x8000000000ll)
	acc0 = -0x8000000000ull, sat = 1;
      else if ((bs64)acc0 >= 0x7fffffffffll)
	acc0 = 0x7fffffffffull, sat = 1;

      if (s == 1)
	{
	  /* A0 -= A1 (W32)  */
	  if (acc0 & (bu64)0x8000000000ll)
	    acc0 &= 0x80ffffffffll, sat = 1;
	  else
	    acc0 &= 0xffffffffll;
	}
      STORE (AXREG (0), (acc0 >> 32) & 0xff);
      STORE (AWREG (0), acc0 & 0xffffffff);
      STORE (ASTATREG (az), acc0 == 0);
      STORE (ASTATREG (an), !!(acc0 & (bu64)0x8000000000ll));
      STORE (ASTATREG (ac0), carry);
      STORE (ASTATREG (ac0_copy), carry);
      STORE (ASTATREG (av0), sat);
      if (sat)
	STORE (ASTATREG (av0s), sat);
    }
  else if ((aop == 0 || aop == 1) && aopcde == 22 && x == 0)
    {
      bu32 s0, s0L, s0H, s1, s1L, s1H;
      bu32 tmp0, tmp1, i;
      const char * const opts[] = { "rndl", "rndh", "tl", "th" };

      TRACE_INSN (cpu, "R%i = BYTEOP2P (R%i:%i, R%i:%i) (%s%s);", dst0,
		  src0 + 1, src0, src1 + 1, src1, opts[HL + (aop << 1)],
		  s ? ", r" : "");

      if ((src1 != 0 && src1 != 2) || (src0 != 0 && src0 != 2))
	illegal_instruction (cpu);

      s0L = DREG (src0);
      s0H = DREG (src0 + 1);
      s1L = DREG (src1);
      s1H = DREG (src1 + 1);
      if (s)
	{
	  s0 = algn (s0H, s0L, IREG (0) & 3);
	  s1 = algn (s1H, s1L, IREG (0) & 3);
	}
      else
	{
	  s0 = algn (s0L, s0H, IREG (0) & 3);
	  s1 = algn (s1L, s1H, IREG (0) & 3);
	}

      i = !aop * 2;
      tmp0 = ((((s1 >>  8) & 0xff) + ((s1 >>  0) & 0xff) +
	       ((s0 >>  8) & 0xff) + ((s0 >>  0) & 0xff) + i) >> 2) & 0xff;
      tmp1 = ((((s1 >> 24) & 0xff) + ((s1 >> 16) & 0xff) +
	       ((s0 >> 24) & 0xff) + ((s0 >> 16) & 0xff) + i) >> 2) & 0xff;
      STORE (DREG (dst0), (tmp1 << (16 + (HL * 8))) | (tmp0 << (HL * 8)));

      /* Implicit DISALGNEXCPT in parallel.  */
      DIS_ALGN_EXPT |= 1;
    }
  else if ((aop == 0 || aop == 1) && aopcde == 8 && x == 0 && s == 0 && HL == 0)
    {
      TRACE_INSN (cpu, "A%i = 0;", aop);
      SET_AREG (aop, 0);
    }
  else if (aop == 2 && aopcde == 8 && x == 0 && s == 0 && HL == 0)
    {
      TRACE_INSN (cpu, "A1 = A0 = 0;");
      SET_AREG (0, 0);
      SET_AREG (1, 0);
    }
  else if ((aop == 0 || aop == 1 || aop == 2) && s == 1 && aopcde == 8
	   && x == 0 && HL == 0)
    {
      bs40 acc0 = get_extended_acc (cpu, 0);
      bs40 acc1 = get_extended_acc (cpu, 1);
      bu32 sat;

      if (aop == 0 || aop == 1)
	TRACE_INSN (cpu, "A%i = A%i (S);", aop, aop);
      else
	TRACE_INSN (cpu, "A1 = A1 (S), A0 = A0 (S);");

      if (aop == 0 || aop == 2)
	{
	  sat = 0;
	  acc0 = saturate_s32 (acc0, &sat);
	  acc0 |= -(acc0 & 0x80000000ull);
	  SET_AXREG (0, (acc0 >> 31) & 0xFF);
	  SET_AWREG (0, acc0 & 0xFFFFFFFF);
	  SET_ASTATREG (av0, sat);
	  if (sat)
	    SET_ASTATREG (av0s, sat);
	}
      else
	acc0 = 1;

      if (aop == 1 || aop == 2)
	{
	  sat = 0;
	  acc1 = saturate_s32 (acc1, &sat);
	  acc1 |= -(acc1 & 0x80000000ull);
	  SET_AXREG (1, (acc1 >> 31) & 0xFF);
	  SET_AWREG (1, acc1 & 0xFFFFFFFF);
	  SET_ASTATREG (av1, sat);
	  if (sat)
	    SET_ASTATREG (av1s, sat);
	}
      else
	acc1 = 1;

      SET_ASTATREG (az, (acc0 == 0) || (acc1 == 0));
      SET_ASTATREG (an, ((acc0 >> 31) & 1) || ((acc1 >> 31) & 1));
    }
  else if (aop == 3 && aopcde == 8 && x == 0 && HL == 0)
    {
      TRACE_INSN (cpu, "A%i = A%i;", s, !s);
      SET_AXREG (s, AXREG (!s));
      SET_AWREG (s, AWREG (!s));
    }
  else if (aop == 3 && HL == 0 && aopcde == 16 && x == 0 && s == 0)
    {
      int i;
      bu32 az;

      TRACE_INSN (cpu, "A1 = ABS A1 , A0 = ABS A0;");

      az = 0;
      for (i = 0; i < 2; ++i)
	{
	  bu32 av;
	  bs40 acc = get_extended_acc (cpu, i);

	  if (acc >> 39)
	    acc = -acc;
	  av = acc == ((bs40)1 << 39);
	  if (av)
	    acc = ((bs40)1 << 39) - 1;

	  SET_AREG (i, acc);
	  SET_ASTATREG (av[i], av);
	  if (av)
	    SET_ASTATREG (avs[i], av);
	  az |= (acc == 0);
	}
      SET_ASTATREG (az, az);
      SET_ASTATREG (an, 0);
    }
  else if (aop == 0 && aopcde == 23 && x == 0)
    {
      bu32 s0, s0L, s0H, s1, s1L, s1H;
      bs32 tmp0, tmp1;

      TRACE_INSN (cpu, "R%i = BYTEOP3P (R%i:%i, R%i:%i) (%s%s);", dst0,
		  src0 + 1, src0, src1 + 1, src1, HL ? "HI" : "LO",
		  s ? ", R" : "");

      if ((src1 != 0 && src1 != 2) || (src0 != 0 && src0 != 2))
	illegal_instruction (cpu);

      s0L = DREG (src0);
      s0H = DREG (src0 + 1);
      s1L = DREG (src1);
      s1H = DREG (src1 + 1);
      if (s)
	{
	  s0 = algn (s0H, s0L, IREG (0) & 3);
	  s1 = algn (s1H, s1L, IREG (1) & 3);
	}
      else
	{
	  s0 = algn (s0L, s0H, IREG (0) & 3);
	  s1 = algn (s1L, s1H, IREG (1) & 3);
	}

      tmp0 = (bs32)(bs16)(s0 >>  0) + ((s1 >> ( 0 + (8 * !HL))) & 0xff);
      tmp1 = (bs32)(bs16)(s0 >> 16) + ((s1 >> (16 + (8 * !HL))) & 0xff);
      STORE (DREG (dst0), (CLAMP (tmp0, 0, 255) << ( 0 + (8 * HL))) |
			  (CLAMP (tmp1, 0, 255) << (16 + (8 * HL))));

      /* Implicit DISALGNEXCPT in parallel.  */
      DIS_ALGN_EXPT |= 1;
    }
  else if ((aop == 0 || aop == 1) && aopcde == 16 && x == 0 && s == 0)
    {
      bu32 av;
      bs40 acc;

      TRACE_INSN (cpu, "A%i = ABS A%i;", HL, aop);

      acc = get_extended_acc (cpu, aop);
      if (acc >> 39)
	acc = -acc;
      av = acc == ((bs40)1 << 39);
      if (av)
	acc = ((bs40)1 << 39) - 1;
      SET_AREG (HL, acc);

      SET_ASTATREG (av[HL], av);
      if (av)
	SET_ASTATREG (avs[HL], av);
      SET_ASTATREG (az, acc == 0);
      SET_ASTATREG (an, 0);
    }
  else if (aop == 3 && aopcde == 12 && x == 0 && s == 0)
    {
      bs32 res = DREG (src0);
      bs32 ovX;
      bool sBit_a, sBit_b;

      TRACE_INSN (cpu, "R%i.%s = R%i (RND);", dst0, HL == 0 ? "L" : "H", src0);
      TRACE_DECODE (cpu, "R%i.%s = R%i:%#x (RND);", dst0,
		    HL == 0 ? "L" : "H", src0, res);

      sBit_b = !!(res & 0x80000000);

      res += 0x8000;
      sBit_a = !!(res & 0x80000000);

      /* Overflow if the sign bit changed when we rounded.  */
      if ((res >> 16) && (sBit_b != sBit_a))
	{
	  ovX = 1;
	  if (!sBit_b)
	    res = 0x7FFF;
	  else
	    res = 0x8000;
	}
      else
	{
	  res = res >> 16;
	  ovX = 0;
	}

      if (!HL)
	SET_DREG (dst0, REG_H_L (DREG (dst0), res));
      else
	SET_DREG (dst0, REG_H_L (res << 16, DREG (dst0)));

      SET_ASTATREG (az, res == 0);
      SET_ASTATREG (an, res < 0);
      SET_ASTATREG (v, ovX);
      if (ovX)
	SET_ASTATREG (vs, ovX);
    }
  else if (aop == 3 && HL == 0 && aopcde == 15 && x == 0 && s == 0)
    {
      bu32 hi = (-(bs16)(DREG (src0) >> 16)) << 16;
      bu32 lo = (-(bs16)(DREG (src0) & 0xFFFF)) & 0xFFFF;
      int v, ac0, ac1;

      TRACE_INSN (cpu, "R%i = -R%i (V);", dst0, src0);

      v = ac0 = ac1 = 0;

      if (hi == 0x80000000)
	{
	  hi = 0x7fff0000;
	  v = 1;
	}
      else if (hi == 0)
	ac1 = 1;

      if (lo == 0x8000)
	{
	  lo = 0x7fff;
	  v = 1;
	}
      else if (lo == 0)
	ac0 = 1;

      SET_DREG (dst0, hi | lo);

      SET_ASTATREG (v, v);
      if (v)
	SET_ASTATREG (vs, 1);
      SET_ASTATREG (ac0, ac0);
      SET_ASTATREG (ac1, ac1);
      setflags_nz_2x16 (cpu, DREG (dst0));
    }
  else if (aop == 3 && HL == 0 && aopcde == 14 && x == 0 && s == 0)
    {
      TRACE_INSN (cpu, "A1 = - A1 , A0 = - A0;");

      SET_AREG (0, saturate_s40 (-get_extended_acc (cpu, 0)));
      SET_AREG (1, saturate_s40 (-get_extended_acc (cpu, 1)));
      /* XXX: what ASTAT flags need updating ?  */
    }
  else if ((aop == 0 || aop == 1) && aopcde == 14 && x == 0 && s == 0)
    {
      bs40 src_acc = get_extended_acc (cpu, aop);
      bu32 v = 0;

      TRACE_INSN (cpu, "A%i = - A%i;", HL, aop);

      SET_AREG (HL, saturate_s40_astat (-src_acc, &v));

      SET_ASTATREG (az, AWREG (HL) == 0 && AXREG (HL) == 0);
      SET_ASTATREG (an, AXREG (HL) >> 7);
      if (HL == 0)
	{
	  SET_ASTATREG (ac0, !src_acc);
	  SET_ASTATREG (av0, v);
	  if (v)
	    SET_ASTATREG (av0s, 1);
	}
      else
	{
	  SET_ASTATREG (ac1, !src_acc);
	  SET_ASTATREG (av1, v);
	  if (v)
	    SET_ASTATREG (av1s, 1);
	}
    }
  else if (aop == 0 && aopcde == 12 && x == 0 && s == 0 && HL == 0)
    {
      bs16 tmp0_hi = DREG (src0) >> 16;
      bs16 tmp0_lo = DREG (src0);
      bs16 tmp1_hi = DREG (src1) >> 16;
      bs16 tmp1_lo = DREG (src1);

      TRACE_INSN (cpu, "R%i.L = R%i.H = SIGN(R%i.H) * R%i.H + SIGN(R%i.L) * R%i.L;",
		  dst0, dst0, src0, src1, src0, src1);

      if ((tmp0_hi >> 15) & 1)
	tmp1_hi = ~tmp1_hi + 1;

      if ((tmp0_lo >> 15) & 1)
	tmp1_lo = ~tmp1_lo + 1;

      tmp1_hi = tmp1_hi + tmp1_lo;

      STORE (DREG (dst0), REG_H_L (tmp1_hi << 16, tmp1_hi));
    }
  else if (aopcde == 0 && HL == 0)
    {
      bu32 s0 = DREG (src0);
      bu32 s1 = DREG (src1);
      bu32 s0h = s0 >> 16;
      bu32 s0l = s0 & 0xFFFF;
      bu32 s1h = s1 >> 16;
      bu32 s1l = s1 & 0xFFFF;
      bu32 t0, t1;
      bu32 ac1_i = 0, ac0_i = 0, v_i = 0, z_i = 0, n_i = 0;

      TRACE_INSN (cpu, "R%i = R%i %c|%c R%i%s;", dst0, src0,
		  (aop & 2) ? '-' : '+', (aop & 1) ? '-' : '+', src1,
		  amod0 (s, x));
      if (aop & 2)
	t0 = sub16 (cpu, s0h, s1h, &ac1_i, &v_i, &z_i, &n_i, s, 0);
      else
	t0 = add16 (cpu, s0h, s1h, &ac1_i, &v_i, &z_i, &n_i, s, 0);

      if (aop & 1)
	t1 = sub16 (cpu, s0l, s1l, &ac0_i, &v_i, &z_i, &n_i, s, 0);
      else
	t1 = add16 (cpu, s0l, s1l, &ac0_i, &v_i, &z_i, &n_i, s, 0);

      SET_ASTATREG (ac1, ac1_i);
      SET_ASTATREG (ac0, ac0_i);
      SET_ASTATREG (az, z_i);
      SET_ASTATREG (an, n_i);
      SET_ASTATREG (v, v_i);
      if (v_i)
	SET_ASTATREG (vs, v_i);

      t0 &= 0xFFFF;
      t1 &= 0xFFFF;
      if (x)
	SET_DREG (dst0, (t1 << 16) | t0);
      else
	SET_DREG (dst0, (t0 << 16) | t1);
    }
  else if (aop == 1 && aopcde == 12 && x == 0 && s == 0 && HL == 0)
    {
      bs32 val0 = (bs16)(AWREG (0) >> 16) + (bs16)AWREG (0);
      bs32 val1 = (bs16)(AWREG (1) >> 16) + (bs16)AWREG (1);

      TRACE_INSN (cpu, "R%i = A1.L + A1.H, R%i = A0.L + A0.H;", dst1, dst0);

      if (dst0 == dst1)
	illegal_instruction_combination (cpu);

      SET_DREG (dst0, val0);
      SET_DREG (dst1, val1);
    }
  else if ((aop == 0 || aop == 2 || aop == 3) && aopcde == 1)
    {
      bu32 d0, d1;
      bu32 x0, x1;
      bu16 s0L = DREG (src0);
      bu16 s0H = DREG (src0) >> 16;
      bu16 s1L = DREG (src1);
      bu16 s1H = DREG (src1) >> 16;
      bu32 v_i = 0, n_i = 0, z_i = 0;

      TRACE_INSN (cpu, "R%i = R%i %s R%i, R%i = R%i %s R%i%s;",
		  dst1, src0, HL ? "+|-" : "+|+", src1,
		  dst0, src0, HL ? "-|+" : "-|-", src1,
		  amod0amod2 (s, x, aop));

      if (dst0 == dst1)
	illegal_instruction_combination (cpu);

      if (HL == 0)
	{
	  x0 = add16 (cpu, s0H, s1H, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  x1 = add16 (cpu, s0L, s1L, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  d1 = (x0 << 16) | x1;

	  x0 = sub16 (cpu, s0H, s1H, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  x1 = sub16 (cpu, s0L, s1L, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  if (x == 0)
	    d0 = (x0 << 16) | x1;
	  else
	    d0 = (x1 << 16) | x0;
	}
      else
	{
	  x0 = add16 (cpu, s0H, s1H, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  x1 = sub16 (cpu, s0L, s1L, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  d1 = (x0 << 16) | x1;

	  x0 = sub16 (cpu, s0H, s1H, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  x1 = add16 (cpu, s0L, s1L, 0, &v_i, &z_i, &n_i, s, aop) & 0xffff;
	  if (x == 0)
	    d0 = (x0 << 16) | x1;
	  else
	    d0 = (x1 << 16) | x0;
	}
      SET_ASTATREG (az, z_i);
      SET_ASTATREG (an, n_i);
      SET_ASTATREG (v, v_i);
      if (v_i)
	SET_ASTATREG (vs, v_i);

      STORE (DREG (dst0), d0);
      STORE (DREG (dst1), d1);
    }
  else if ((aop == 0 || aop == 1 || aop == 2) && aopcde == 11 && x == 0)
    {
      bs40 acc0 = get_extended_acc (cpu, 0);
      bs40 acc1 = get_extended_acc (cpu, 1);
      bu32 v, dreg, sat = 0;
      bu32 carry = !!((bu40)~acc1 < (bu40)acc0);

      if (aop == 0)
	{
	  if (s != 0 || HL != 0)
	    illegal_instruction (cpu);
	  TRACE_INSN (cpu, "R%i = (A0 += A1);", dst0);
	}
      else if (aop == 1)
	{
	  if (s != 0)
	    illegal_instruction (cpu);
	  TRACE_INSN (cpu, "R%i.%c = (A0 += A1);", dst0, HL ? 'H' : 'L');
	}
      else
	{
	  if (HL != 0)
	    illegal_instruction (cpu);
	  TRACE_INSN (cpu, "A0 += A1%s;", s ? " (W32)" : "");
	}

      acc0 += acc1;
      acc0 = saturate_s40_astat (acc0, &v);

      if (aop == 2 && s == 1)   /* A0 += A1 (W32)  */
	{
	  if (acc0 & (bs40)0x8000000000ll)
	    acc0 &= 0x80ffffffffll;
	  else
	    acc0 &= 0xffffffffll;
	}

      STORE (AXREG (0), acc0 >> 32);
      STORE (AWREG (0), acc0);
      SET_ASTATREG (av0, v && acc1);
      if (v)
	SET_ASTATREG (av0s, v);

      if (aop == 0 || aop == 1)
	{
	  if (aop)	/* Dregs_lo = A0 += A1  */
	    {
	      dreg = saturate_s32 (rnd16 (acc0) << 16, &sat);
	      if (HL)
		STORE (DREG (dst0), REG_H_L (dreg, DREG (dst0)));
	      else
		STORE (DREG (dst0), REG_H_L (DREG (dst0), dreg >> 16));
	    }
	  else		/* Dregs = A0 += A1  */
	    {
	      dreg = saturate_s32 (acc0, &sat);
	      STORE (DREG (dst0), dreg);
	    }

	  STORE (ASTATREG (az), dreg == 0);
	  STORE (ASTATREG (an), !!(dreg & 0x80000000));
	  STORE (ASTATREG (ac0), carry);
	  STORE (ASTATREG (ac0_copy), carry);
	  STORE (ASTATREG (v), sat);
	  STORE (ASTATREG (v_copy), sat);
	  if (sat)
	    STORE (ASTATREG (vs), sat);
	}
      else
	{
	  STORE (ASTATREG (az), acc0 == 0);
	  STORE (ASTATREG (an), !!(acc0 & 0x8000000000ull));
	  STORE (ASTATREG (ac0), carry);
	  STORE (ASTATREG (ac0_copy), carry);
	}
    }
  else if ((aop == 0 || aop == 1) && aopcde == 10 && x == 0 && s == 0 && HL == 0)
    {
      TRACE_INSN (cpu, "R%i.L = A%i.X;", dst0, aop);
      SET_DREG_L (dst0, (bs8)AXREG (aop));
    }
  else if (aop == 0 && aopcde == 4 && x == 0 && HL == 0)
    {
      TRACE_INSN (cpu, "R%i = R%i + R%i%s;", dst0, src0, src1, amod1 (s, x));
      SET_DREG (dst0, add32 (cpu, DREG (src0), DREG (src1), 1, s));
    }
  else if (aop == 1 && aopcde == 4 && x == 0 && HL == 0)
    {
      TRACE_INSN (cpu, "R%i = R%i - R%i%s;", dst0, src0, src1, amod1 (s, x));
      SET_DREG (dst0, sub32 (cpu, DREG (src0), DREG (src1), 1, s, 0));
    }
  else if (aop == 2 && aopcde == 4 && x == 0 && HL == 0)
    {
      TRACE_INSN (cpu, "R%i = R%i + R%i, R%i = R%i - R%i%s;",
		  dst1, src0, src1, dst0, src0, src1, amod1 (s, x));

      if (dst0 == dst1)
	illegal_instruction_combination (cpu);

      STORE (DREG (dst1), add32 (cpu, DREG (src0), DREG (src1), 1, s));
      STORE (DREG (dst0), sub32 (cpu, DREG (src0), DREG (src1), 1, s, 1));
    }
  else if ((aop == 0 || aop == 1) && aopcde == 17 && x == 0 && HL == 0)
    {
      bs40 acc0 = get_extended_acc (cpu, 0);
      bs40 acc1 = get_extended_acc (cpu, 1);
      bs40 val0, val1, sval0, sval1;
      bu32 sat, sat_i;

      TRACE_INSN (cpu, "R%i = A%i + A%i, R%i = A%i - A%i%s",
		  dst1, !aop, aop, dst0, !aop, aop, amod1 (s, x));
      TRACE_DECODE (cpu, "R%i = A%i:%#"PRIx64" + A%i:%#"PRIx64", "
			 "R%i = A%i:%#"PRIx64" - A%i:%#"PRIx64"%s",
		    dst1, !aop, aop ? acc0 : acc1, aop, aop ? acc1 : acc0,
		    dst0, !aop, aop ? acc0 : acc1, aop, aop ? acc1 : acc0,
		    amod1 (s, x));

      if (dst0 == dst1)
	illegal_instruction_combination (cpu);

      val1 = acc0 + acc1;
      if (aop)
	val0 = acc0 - acc1;
      else
	val0 = acc1 - acc0;

      sval0 = saturate_s32 (val0, &sat);
      sat_i = sat;
      sval1 = saturate_s32 (val1, &sat);
      sat_i |= sat;
      if (s)
	{
	  val0 = sval0;
	  val1 = sval1;
	}

      STORE (DREG (dst0), val0);
      STORE (DREG (dst1), val1);
      SET_ASTATREG (v, sat_i);
      if (sat_i)
	SET_ASTATREG (vs, sat_i);
      SET_ASTATREG (an, val0 & 0x80000000 || val1 & 0x80000000);
      SET_ASTATREG (az, val0 == 0 || val1 == 0);
      SET_ASTATREG (ac1, (bu40)~acc0 < (bu40)acc1);
      if (aop)
	SET_ASTATREG (ac0, !!((bu40)acc1 <= (bu40)acc0));
      else
	SET_ASTATREG (ac0, !!((bu40)acc0 <= (bu40)acc1));
    }
  else if (aop == 0 && aopcde == 18 && x == 0 && HL == 0)
    {
      bu40 acc0 = get_extended_acc (cpu, 0);
      bu40 acc1 = get_extended_acc (cpu, 1);
      bu32 s0L = DREG (src0);
      bu32 s0H = DREG (src0 + 1);
      bu32 s1L = DREG (src1);
      bu32 s1H = DREG (src1 + 1);
      bu32 s0, s1;
      bs16 tmp0, tmp1, tmp2, tmp3;

      /* This instruction is only defined for register pairs R1:0 and R3:2.  */
      if (!((src0 == 0 || src0 == 2) && (src1 == 0 || src1 == 2)))
	illegal_instruction (cpu);

      TRACE_INSN (cpu, "SAA (R%i:%i, R%i:%i)%s", src0 + 1, src0,
		  src1 + 1, src1, s ? " (R)" :"");

      /* Bit s determines the order of the two registers from a pair:
         if s=0 the low-order bytes come from the low reg in the pair,
         and if s=1 the low-order bytes come from the high reg.  */

      if (s)
	{
	  s0 = algn (s0H, s0L, IREG (0) & 3);
	  s1 = algn (s1H, s1L, IREG (1) & 3);
	}
      else
	{
	  s0 = algn (s0L, s0H, IREG (0) & 3);
	  s1 = algn (s1L, s1H, IREG (1) & 3);
	}

      /* Find the absolute difference between pairs, make it
         absolute, then add it to the existing accumulator half.  */
      /* Byte 0  */
      tmp0  = ((s0 << 24) >> 24) - ((s1 << 24) >> 24);
      tmp1  = ((s0 << 16) >> 24) - ((s1 << 16) >> 24);
      tmp2  = ((s0 <<  8) >> 24) - ((s1 <<  8) >> 24);
      tmp3  = ((s0 <<  0) >> 24) - ((s1 <<  0) >> 24);

      tmp0  = (tmp0 < 0) ? -tmp0 : tmp0;
      tmp1  = (tmp1 < 0) ? -tmp1 : tmp1;
      tmp2  = (tmp2 < 0) ? -tmp2 : tmp2;
      tmp3  = (tmp3 < 0) ? -tmp3 : tmp3;

      s0L = saturate_u16 ((bu32)tmp0 + ((acc0 >>  0) & 0xffff), 0);
      s0H = saturate_u16 ((bu32)tmp1 + ((acc0 >> 16) & 0xffff), 0);
      s1L = saturate_u16 ((bu32)tmp2 + ((acc1 >>  0) & 0xffff), 0);
      s1H = saturate_u16 ((bu32)tmp3 + ((acc1 >> 16) & 0xffff), 0);

      STORE (AWREG (0), (s0H << 16) | (s0L & 0xFFFF));
      STORE (AXREG (0), 0);
      STORE (AWREG (1), (s1H << 16) | (s1L & 0xFFFF));
      STORE (AXREG (1), 0);

      /* Implicit DISALGNEXCPT in parallel.  */
      DIS_ALGN_EXPT |= 1;
    }
  else if (aop == 3 && aopcde == 18 && x == 0 && s == 0 && HL == 0)
    {
      TRACE_INSN (cpu, "DISALGNEXCPT");
      DIS_ALGN_EXPT |= 1;
    }
  else if ((aop == 0 || aop == 1) && aopcde == 20 && x == 0 && HL == 0)
    {
      bu32 s0, s0L, s0H, s1, s1L, s1H;
      const char * const opts[] = { "", " (R)", " (T)", " (T, R)" };

      TRACE_INSN (cpu, "R%i = BYTEOP1P (R%i:%i, R%i:%i)%s;", dst0,
		  src0 + 1, src0, src1 + 1, src1, opts[s + (aop << 1)]);

      if ((src1 != 0 && src1 != 2) || (src0 != 0 && src0 != 2))
	illegal_instruction (cpu);

      s0L = DREG (src0);
      s0H = DREG (src0 + 1);
      s1L = DREG (src1);
      s1H = DREG (src1 + 1);
      if (s)
	{
	  s0 = algn (s0H, s0L, IREG (0) & 3);
	  s1 = algn (s1H, s1L, IREG (1) & 3);
	}
      else
	{
	  s0 = algn (s0L, s0H, IREG (0) & 3);
	  s1 = algn (s1L, s1H, IREG (1) & 3);
	}

      STORE (DREG (dst0),
		(((((s0 >>  0) & 0xff) + ((s1 >>  0) & 0xff) + !aop) >> 1) <<  0) |
		(((((s0 >>  8) & 0xff) + ((s1 >>  8) & 0xff) + !aop) >> 1) <<  8) |
		(((((s0 >> 16) & 0xff) + ((s1 >> 16) & 0xff) + !aop) >> 1) << 16) |
		(((((s0 >> 24) & 0xff) + ((s1 >> 24) & 0xff) + !aop) >> 1) << 24));

      /* Implicit DISALGNEXCPT in parallel.  */
      DIS_ALGN_EXPT |= 1;
    }
  else if (aop == 0 && aopcde == 21 && x == 0 && HL == 0)
    {
      bu32 s0, s0L, s0H, s1, s1L, s1H;

      TRACE_INSN (cpu, "(R%i, R%i) = BYTEOP16P (R%i:%i, R%i:%i)%s;", dst1, dst0,
		  src0 + 1, src0, src1 + 1, src1, s ? " (R)" : "");

      if ((src1 != 0 && src1 != 2) || (src0 != 0 && src0 != 2))
	illegal_instruction (cpu);

      if (dst0 == dst1)
	illegal_instruction_combination (cpu);

      s0L = DREG (src0);
      s0H = DREG (src0 + 1);
      s1L = DREG (src1);
      s1H = DREG (src1 + 1);
      if (s)
	{
	  s0 = algn (s0H, s0L, IREG (0) & 3);
	  s1 = algn (s1H, s1L, IREG (1) & 3);
	}
      else
	{
	  s0 = algn (s0L, s0H, IREG (0) & 3);
	  s1 = algn (s1L, s1H, IREG (1) & 3);
	}

      STORE (DREG (dst0),
		((((s0 >>  0) & 0xff) + ((s1 >>  0) & 0xff)) <<  0) |
		((((s0 >>  8) & 0xff) + ((s1 >>  8) & 0xff)) << 16));
      STORE (DREG (dst1),
		((((s0 >> 16) & 0xff) + ((s1 >> 16) & 0xff)) <<  0) |
		((((s0 >> 24) & 0xff) + ((s1 >> 24) & 0xff)) << 16));

      /* Implicit DISALGNEXCPT in parallel.  */
      DIS_ALGN_EXPT |= 1;
    }
  else if (aop == 1 && aopcde == 21 && x == 0 && HL == 0)
    {
      bu32 s0, s0L, s0H, s1, s1L, s1H;

      TRACE_INSN (cpu, "(R%i, R%i) = BYTEOP16M (R%i:%i, R%i:%i)%s;", dst1, dst0,
		  src0 + 1, src0, src1 + 1, src1, s ? " (R)" : "");

      if ((src1 != 0 && src1 != 2) || (src0 != 0 && src0 != 2))
	illegal_instruction (cpu);

      if (dst0 == dst1)
	illegal_instruction_combination (cpu);

      s0L = DREG (src0);
      s0H = DREG (src0 + 1);
      s1L = DREG (src1);
      s1H = DREG (src1 + 1);
      if (s)
	{
	  s0 = algn (s0H, s0L, IREG (0) & 3);
	  s1 = algn (s1H, s1L, IREG (1) & 3);
	}
      else
	{
	  s0 = algn (s0L, s0H, IREG (0) & 3);
	  s1 = algn (s1L, s1H, IREG (1) & 3);
	}

      STORE (DREG (dst0),
		(((((s0 >>  0) & 0xff) - ((s1 >>  0) & 0xff)) <<  0) & 0xffff) |
		(((((s0 >>  8) & 0xff) - ((s1 >>  8) & 0xff)) << 16)));
      STORE (DREG (dst1),
		(((((s0 >> 16) & 0xff) - ((s1 >> 16) & 0xff)) <<  0) & 0xffff) |
		(((((s0 >> 24) & 0xff) - ((s1 >> 24) & 0xff)) << 16)));

      /* Implicit DISALGNEXCPT in parallel.  */
      DIS_ALGN_EXPT |= 1;
    }
  else if (aop == 1 && aopcde == 7 && x == 0 && s == 0 && HL == 0)
    {
      TRACE_INSN (cpu, "R%i = MIN (R%i, R%i);", dst0, src0, src1);
      SET_DREG (dst0, min32 (cpu, DREG (src0), DREG (src1)));
    }
  else if (aop == 0 && aopcde == 7 && x == 0 && s == 0 && HL == 0)
    {
      TRACE_INSN (cpu, "R%i = MAX (R%i, R%i);", dst0, src0, src1);
      SET_DREG (dst0, max32 (cpu, DREG (src0), DREG (src1)));
    }
  else if (aop == 2 && aopcde == 7 && x == 0 && s == 0 && HL == 0)
    {
      bu32 val = DREG (src0);
      int v;

      TRACE_INSN (cpu, "R%i = ABS R%i;", dst0, src0);

      if (val >> 31)
	val = -val;
      v = (val == 0x80000000);
      if (v)
	val = 0x7fffffff;
      SET_DREG (dst0, val);

      SET_ASTATREG (v, v);
      if (v)
	SET_ASTATREG (vs, 1);
      setflags_nz (cpu, val);
    }
  else if (aop == 3 && aopcde == 7 && x == 0 && HL == 0)
    {
      bu32 val = DREG (src0);

      TRACE_INSN (cpu, "R%i = - R%i%s;", dst0, src0, amod1 (s, 0));

      if (s && val == 0x80000000)
	{
	  val = 0x7fffffff;
	  SET_ASTATREG (v, 1);
	  SET_ASTATREG (vs, 1);
	}
      else if (val == 0x80000000)
	val = 0x80000000;
      else
	val = -val;
      SET_DREG (dst0, val);

      SET_ASTATREG (az, val == 0);
      SET_ASTATREG (an, val & 0x80000000);
    }
  else if (aop == 2 && aopcde == 6 && x == 0 && s == 0 && HL == 0)
    {
      bu32 in = DREG (src0);
      bu32 hi = (in & 0x80000000 ? (bu32)-(bs16)(in >> 16) : in >> 16) << 16;
      bu32 lo = (in & 0x8000 ? (bu32)-(bs16)(in & 0xFFFF) : in) & 0xFFFF;
      int v;

      TRACE_INSN (cpu, "R%i = ABS R%i (V);", dst0, src0);

      v = 0;
      if (hi == 0x80000000)
	{
	  hi = 0x7fff0000;
	  v = 1;
	}
      if (lo == 0x8000)
	{
	  lo = 0x7fff;
	  v = 1;
	}
      SET_DREG (dst0, hi | lo);

      SET_ASTATREG (v, v);
      if (v)
	SET_ASTATREG (vs, 1);
      setflags_nz_2x16 (cpu, DREG (dst0));
    }
  else if (aop == 1 && aopcde == 6 && x == 0 && s == 0 && HL == 0)
    {
      TRACE_INSN (cpu, "R%i = MIN (R%i, R%i) (V);", dst0, src0, src1);
      SET_DREG (dst0, min2x16 (cpu, DREG (src0), DREG (src1)));
    }
  else if (aop == 0 && aopcde == 6 && x == 0 && s == 0 && HL == 0)
    {
      TRACE_INSN (cpu, "R%i = MAX (R%i, R%i) (V);", dst0, src0, src1);
      SET_DREG (dst0, max2x16 (cpu, DREG (src0), DREG (src1)));
    }
  else if (aop == 0 && aopcde == 24 && x == 0 && s == 0 && HL == 0)
    {
      TRACE_INSN (cpu, "R%i = BYTEPACK (R%i, R%i);", dst0, src0, src1);
      STORE (DREG (dst0),
	(((DREG (src0) >>  0) & 0xff) <<  0) |
	(((DREG (src0) >> 16) & 0xff) <<  8) |
	(((DREG (src1) >>  0) & 0xff) << 16) |
	(((DREG (src1) >> 16) & 0xff) << 24));

      /* Implicit DISALGNEXCPT in parallel.  */
      DIS_ALGN_EXPT |= 1;
    }
  else if (aop == 1 && aopcde == 24 && x == 0 && HL == 0)
    {
      int order, lo, hi;
      bu64 comb_src;
      bu8 bytea, byteb, bytec, byted;

      TRACE_INSN (cpu, "(R%i, R%i) = BYTEUNPACK R%i:%i%s;",
		  dst1, dst0, src0 + 1, src0, s ? " (R)" : "");

      if ((src1 != 0 && src1 != 2) || (src0 != 0 && src0 != 2))
	illegal_instruction (cpu);

      if (dst0 == dst1)
	illegal_instruction_combination (cpu);

      order = IREG (0) & 0x3;
      if (s)
	hi = src0, lo = src0 + 1;
      else
	hi = src0 + 1, lo = src0;
      comb_src = (((bu64)DREG (hi)) << 32) | DREG (lo);
      bytea = (comb_src >> (0 + 8 * order));
      byteb = (comb_src >> (8 + 8 * order));
      bytec = (comb_src >> (16 + 8 * order));
      byted = (comb_src >> (24 + 8 * order));
      STORE (DREG (dst0), bytea | ((bu32)byteb << 16));
      STORE (DREG (dst1), bytec | ((bu32)byted << 16));

      /* Implicit DISALGNEXCPT in parallel.  */
      DIS_ALGN_EXPT |= 1;
    }
  else if (aopcde == 13 && HL == 0 && x == 0 && s == 0)
    {
      const char *searchmodes[] = { "GT", "GE", "LT", "LE" };
      bool up_hi, up_lo;
      bs16 a0_lo, a1_lo, src_hi, src_lo;

      TRACE_INSN (cpu, "(R%i, R%i) = SEARCH R%i (%s);",
		  dst1, dst0, src0, searchmodes[aop]);

      /* XXX: The parallel version is a bit weird in its limits:

         This instruction can be issued in parallel with the combination of one
         16-bit length load instruction to the P0 register and one 16-bit NOP.
         No other instructions can be issued in parallel with the Vector Search
         instruction. Note the following legal and illegal forms.
         (r1, r0) = search r2 (LT) || r2 = [p0++p3]; // ILLEGAL
         (r1, r0) = search r2 (LT) || r2 = [p0++];   // LEGAL
         (r1, r0) = search r2 (LT) || r2 = [p0++];   // LEGAL

         Unfortunately, our parallel insn state doesn't (currently) track enough
         details to be able to check this.  */

      if (dst0 == dst1)
	illegal_instruction_combination (cpu);

      up_hi = up_lo = false;
      a0_lo = AWREG (0);
      a1_lo = AWREG (1);
      src_lo = DREG (src0);
      src_hi = DREG (src0) >> 16;

      switch (aop)
	{
	case 0:
	  up_hi = (src_hi > a1_lo);
	  up_lo = (src_lo > a0_lo);
	  break;
	case 1:
	  up_hi = (src_hi >= a1_lo);
	  up_lo = (src_lo >= a0_lo);
	  break;
	case 2:
	  up_hi = (src_hi < a1_lo);
	  up_lo = (src_lo < a0_lo);
	  break;
	case 3:
	  up_hi = (src_hi <= a1_lo);
	  up_lo = (src_lo <= a0_lo);
	  break;
	}

      if (up_hi)
	{
	  SET_AREG (1, src_hi);
	  SET_DREG (dst1, PREG (0));
	}
      else
	SET_AREG (1, a1_lo);

      if (up_lo)
	{
	  SET_AREG (0, src_lo);
	  SET_DREG (dst0, PREG (0));
	}
      else
	SET_AREG (0, a0_lo);
    }
  else
    illegal_instruction (cpu);
}

static void
decode_dsp32shift_0 (SIM_CPU *cpu, bu16 iw0, bu16 iw1)
{
  /* dsp32shift
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 1 | 0 | 0 |.M.| 1 | 1 | 0 | 0 | - | - |.sopcde............|
     |.sop...|.HLs...|.dst0......| - | - | - |.src0......|.src1......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int HLs  = ((iw1 >> DSP32Shift_HLs_bits) & DSP32Shift_HLs_mask);
  int sop  = ((iw1 >> DSP32Shift_sop_bits) & DSP32Shift_sop_mask);
  int src0 = ((iw1 >> DSP32Shift_src0_bits) & DSP32Shift_src0_mask);
  int src1 = ((iw1 >> DSP32Shift_src1_bits) & DSP32Shift_src1_mask);
  int dst0 = ((iw1 >> DSP32Shift_dst0_bits) & DSP32Shift_dst0_mask);
  int sopcde = ((iw0 >> (DSP32Shift_sopcde_bits - 16)) & DSP32Shift_sopcde_mask);
  int M = ((iw0 >> (DSP32Shift_M_bits - 16)) & DSP32Shift_M_mask);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_dsp32shift);
  TRACE_EXTRACT (cpu, "%s: M:%i sopcde:%i sop:%i HLs:%i dst0:%i src0:%i src1:%i",
		 __func__, M, sopcde, sop, HLs, dst0, src0, src1);

  if ((sop == 0 || sop == 1) && sopcde == 0)
    {
      bu16 val;
      bs32 shft = (bs8)(DREG (src0) << 2) >> 2;

      TRACE_INSN (cpu, "R%i.%c = ASHIFT R%i.%c BY R%i.L%s;",
		  dst0, HLs < 2 ? 'L' : 'H',
		  src1, HLs & 1 ? 'H' : 'L',
		  src0, sop == 1 ? " (S)" : "");

      if ((HLs & 1) == 0)
	val = (bu16)(DREG (src1) & 0xFFFF);
      else
	val = (bu16)((DREG (src1) & 0xFFFF0000) >> 16);

      /* Positive shift magnitudes produce Logical Left shifts.
         Negative shift magnitudes produce Arithmetic Right shifts.  */
      if (shft <= 0)
	val = ashiftrt (cpu, val, -shft, 16);
      else
	{
	  int sgn = (val >> 15) & 0x1;

	  val = lshift (cpu, val, shft, 16, sop == 1, 1);
	  if (((val >> 15) & 0x1) != sgn)
	    {
	      SET_ASTATREG (v, 1);
	      SET_ASTATREG (vs, 1);
	    }
	}

      if ((HLs & 2) == 0)
	STORE (DREG (dst0), REG_H_L (DREG (dst0), val));
      else
	STORE (DREG (dst0), REG_H_L (val << 16, DREG (dst0)));
    }
  else if (sop == 2 && sopcde == 0)
    {
      bs32 shft = (bs8)(DREG (src0) << 2) >> 2;
      bu16 val;

      TRACE_INSN (cpu, "R%i.%c = LSHIFT R%i.%c BY R%i.L;",
		  dst0, HLs < 2 ? 'L' : 'H',
		  src1, HLs & 1 ? 'H' : 'L', src0);

      if ((HLs & 1) == 0)
	val = (bu16)(DREG (src1) & 0xFFFF);
      else
	val = (bu16)((DREG (src1) & 0xFFFF0000) >> 16);

      if (shft < 0)
	val = val >> (-1 * shft);
      else
	val = val << shft;

      if ((HLs & 2) == 0)
	SET_DREG (dst0, REG_H_L (DREG (dst0), val));
      else
	SET_DREG (dst0, REG_H_L (val << 16, DREG (dst0)));

      SET_ASTATREG (az, !((val & 0xFFFF0000) == 0) || ((val & 0xFFFF) == 0));
      SET_ASTATREG (an, (!!(val & 0x80000000)) ^ (!!(val & 0x8000)));
      SET_ASTATREG (v, 0);
    }
  else if (sop == 2 && sopcde == 3 && (HLs == 1 || HLs == 0))
    {
      int shift = imm6 (DREG (src0) & 0xFFFF);
      bu32 cc = CCREG;
      bu40 acc = get_unextended_acc (cpu, HLs);

      TRACE_INSN (cpu, "A%i = ROT A%i BY R%i.L;", HLs, HLs, src0);
      TRACE_DECODE (cpu, "A%i:%#"PRIx64" shift:%i CC:%i", HLs, acc, shift, cc);

      acc = rot40 (acc, shift, &cc);
      SET_AREG (HLs, acc);
      if (shift)
	SET_CCREG (cc);
    }
  else if (sop == 0 && sopcde == 3 && (HLs == 0 || HLs == 1))
    {
      bs32 shft = (bs8)(DREG (src0) << 2) >> 2;
      bu64 acc = get_extended_acc (cpu, HLs);
      bu64 val;

      HLs = !!HLs;
      TRACE_INSN (cpu, "A%i = ASHIFT A%i BY R%i.L;", HLs, HLs, src0);
      TRACE_DECODE (cpu, "A%i:%#"PRIx64" shift:%i", HLs, acc, shft);

      if (shft <= 0)
	val = ashiftrt (cpu, acc, -shft, 40);
      else
	val = lshift (cpu, acc, shft, 40, 0, 0);

      STORE (AXREG (HLs), (val >> 32) & 0xff);
      STORE (AWREG (HLs), (val & 0xffffffff));
      STORE (ASTATREG (av[HLs]), 0);
    }
  else if (sop == 1 && sopcde == 3 && (HLs == 0 || HLs == 1))
    {
      bs32 shft = (bs8)(DREG (src0) << 2) >> 2;
      bu64 acc = get_unextended_acc (cpu, HLs);
      bu64 val;

      HLs = !!HLs;
      TRACE_INSN (cpu, "A%i = LSHIFT A%i BY R%i.L;", HLs, HLs, src0);
      TRACE_DECODE (cpu, "A%i:%#"PRIx64" shift:%i", HLs, acc, shft);

      if (shft <= 0)
	val = lshiftrt (cpu, acc, -shft, 40);
      else
	val = lshift (cpu, acc, shft, 40, 0, 0);

      STORE (AXREG (HLs), (val >> 32) & 0xff);
      STORE (AWREG (HLs), (val & 0xffffffff));
      STORE (ASTATREG (av[HLs]), 0);
    }
  else if (HLs != 0)
    /* All the insns after this point don't use HLs.  */
    illegal_instruction (cpu);
  else if ((sop == 0 || sop == 1) && sopcde == 1 && HLs == 0)
    {
      bs32 shft = (bs8)(DREG (src0) << 2) >> 2;
      bu16 val0, val1;
      bu32 astat;

      TRACE_INSN (cpu, "R%i = ASHIFT R%i BY R%i.L (V%s);",
		  dst0, src1, src0, sop == 1 ? ",S" : "");

      val0 = (bu16)DREG (src1) & 0xFFFF;
      val1 = (bu16)((DREG (src1) & 0xFFFF0000) >> 16);

      if (shft <= 0)
	{
	  val0 = ashiftrt (cpu, val0, -shft, 16);
	  astat = ASTAT;
	  val1 = ashiftrt (cpu, val1, -shft, 16);
	}
      else
	{
	  int sgn0 = (val0 >> 15) & 0x1;
	  int sgn1 = (val1 >> 15) & 0x1;

	  val0 = lshift (cpu, val0, shft, 16, sop == 1, 1);
	  astat = ASTAT;
	  val1 = lshift (cpu, val1, shft, 16, sop == 1, 1);

	  if ((sgn0 != ((val0 >> 15) & 0x1)) || (sgn1 != ((val1 >> 15) & 0x1)))
	    {
	      SET_ASTATREG (v, 1);
	      SET_ASTATREG (vs, 1);
	    }
	}
      SET_ASTAT (ASTAT | astat);
      STORE (DREG (dst0), (val1 << 16) | val0);
    }
  else if ((sop == 0 || sop == 1 || sop == 2) && sopcde == 2)
    {
      /* dregs = [LA]SHIFT dregs BY dregs_lo (opt_S)  */
      /* sop == 1 : opt_S  */
      bu32 v = DREG (src1);
      /* LSHIFT uses sign extended low 6 bits of dregs_lo.  */
      bs32 shft = (bs8)(DREG (src0) << 2) >> 2;

      TRACE_INSN (cpu, "R%i = %cSHIFT R%i BY R%i.L%s;", dst0,
		  shft && sop != 2 ? 'A' : 'L', src1, src0,
		  sop == 1 ? " (S)" : "");

      if (shft < 0)
	{
	  if (sop == 2)
	    STORE (DREG (dst0), lshiftrt (cpu, v, -shft, 32));
	  else
	    STORE (DREG (dst0), ashiftrt (cpu, v, -shft, 32));
	}
      else
	{
	  bu32 val = lshift (cpu, v, shft, 32, sop == 1, 1);

	  STORE (DREG (dst0), val);
	  if (((v >> 31) & 0x1) != ((val >> 31) & 0x1))
	    {
	      SET_ASTATREG (v, 1);
	      SET_ASTATREG (vs, 1);
	    }
	}
    }
  else if (sop == 3 && sopcde == 2)
    {
      int shift = imm6 (DREG (src0) & 0xFFFF);
      bu32 src = DREG (src1);
      bu32 ret, cc = CCREG;

      TRACE_INSN (cpu, "R%i = ROT R%i BY R%i.L;", dst0, src1, src0);
      TRACE_DECODE (cpu, "R%i:%#x R%i:%#x shift:%i CC:%i",
		    dst0, DREG (dst0), src1, src, shift, cc);

      ret = rot32 (src, shift, &cc);
      STORE (DREG (dst0), ret);
      if (shift)
	SET_CCREG (cc);
    }
  else if (sop == 2 && sopcde == 1 && HLs == 0)
    {
      bs32 shft = (bs8)(DREG (src0) << 2) >> 2;
      bu16 val0, val1;
      bu32 astat;

      TRACE_INSN (cpu, "R%i = LSHIFT R%i BY R%i.L (V);", dst0, src1, src0);

      val0 = (bu16)DREG (src1) & 0xFFFF;
      val1 = (bu16)((DREG (src1) & 0xFFFF0000) >> 16);

      if (shft <= 0)
	{
	  val0 = lshiftrt (cpu, val0, -shft, 16);
	  astat = ASTAT;
	  val1 = lshiftrt (cpu, val1, -shft, 16);
	}
      else
	{
	  val0 = lshift (cpu, val0, shft, 16, 0, 0);
	  astat = ASTAT;
	  val1 = lshift (cpu, val1, shft, 16, 0, 0);
	}
      SET_ASTAT (ASTAT | astat);
      STORE (DREG (dst0), (val1 << 16) | val0);
    }
  else if (sopcde == 4)
    {
      bu32 sv0 = DREG (src0);
      bu32 sv1 = DREG (src1);
      TRACE_INSN (cpu, "R%i = PACK (R%i.%c, R%i.%c);", dst0,
		  src1, sop & 2 ? 'H' : 'L',
		  src0, sop & 1 ? 'H' : 'L');
      if (sop & 1)
	sv0 >>= 16;
      if (sop & 2)
	sv1 >>= 16;
      STORE (DREG (dst0), (sv1 << 16) | (sv0 & 0xFFFF));
    }
  else if (sop == 0 && sopcde == 5)
    {
      bu32 sv1 = DREG (src1);
      TRACE_INSN (cpu, "R%i.L = SIGNBITS R%i;", dst0, src1);
      SET_DREG_L (dst0, signbits (sv1, 32));
    }
  else if (sop == 1 && sopcde == 5)
    {
      bu32 sv1 = DREG (src1);
      TRACE_INSN (cpu, "R%i.L = SIGNBITS R%i.L;", dst0, src1);
      SET_DREG_L (dst0, signbits (sv1, 16));
    }
  else if (sop == 2 && sopcde == 5)
    {
      bu32 sv1 = DREG (src1);
      TRACE_INSN (cpu, "R%i.L = SIGNBITS R%i.H;", dst0, src1);
      SET_DREG_L (dst0, signbits (sv1 >> 16, 16));
    }
  else if ((sop == 0 || sop == 1) && sopcde == 6)
    {
      bu64 acc = AXREG (sop);
      TRACE_INSN (cpu, "R%i.L = SIGNBITS A%i;", dst0, sop);
      acc <<= 32;
      acc |= AWREG (sop);
      SET_DREG_L (dst0, signbits (acc, 40) & 0xFFFF);
    }
  else if (sop == 3 && sopcde == 6)
    {
      bu32 v = ones (DREG (src1));
      TRACE_INSN (cpu, "R%i.L = ONES R%i;", dst0, src1);
      SET_DREG_L (dst0, v);
    }
  else if (sop == 0 && sopcde == 7)
    {
      bu16 sv1 = (bu16)signbits (DREG (src1), 32);
      bu16 sv0 = (bu16)DREG (src0);
      bu16 dst_lo;

      TRACE_INSN (cpu, "R%i.L = EXPADJ (R%i, R%i.L);", dst0, src1, src0);

      if ((sv1 & 0x1f) < (sv0 & 0x1f))
	dst_lo = sv1;
      else
	dst_lo = sv0;
      STORE (DREG (dst0), REG_H_L (DREG (dst0), dst_lo));
    }
  else if (sop == 1 && sopcde == 7)
    {
      /* Exponent adjust on two 16-bit inputs.  Select
         smallest norm among 3 inputs.  */
      bs16 src1_hi = (DREG (src1) & 0xFFFF0000) >> 16;
      bs16 src1_lo = (DREG (src1) & 0xFFFF);
      bu16 src0_lo = (DREG (src0) & 0xFFFF);
      bu16 tmp_hi, tmp_lo, tmp;

      TRACE_INSN (cpu, "R%i.L = EXPADJ (R%i, R%i.L) (V);", dst0, src1, src0);

      tmp_hi = signbits (src1_hi, 16);
      tmp_lo = signbits (src1_lo, 16);

      if ((tmp_hi & 0xf) < (tmp_lo & 0xf))
	if ((tmp_hi & 0xf) < (src0_lo & 0xf))
	  tmp = tmp_hi;
	else
	  tmp = src0_lo;
      else
	if ((tmp_lo & 0xf) < (src0_lo & 0xf))
	  tmp = tmp_lo;
	else
	  tmp = src0_lo;
      STORE (DREG (dst0), REG_H_L (DREG (dst0), tmp));
    }
  else if (sop == 2 && sopcde == 7)
    {
      /* Exponent adjust on single 16-bit register.  */
      bu16 tmp;
      bu16 src0_lo = (bu16)(DREG (src0) & 0xFFFF);

      TRACE_INSN (cpu, "R%i.L = EXPADJ (R%i.L, R%i.L);", dst0, src1, src0);

      tmp = signbits (DREG (src1) & 0xFFFF, 16);

      if ((tmp & 0xf) < (src0_lo & 0xf))
	SET_DREG_L (dst0, tmp);
      else
	SET_DREG_L (dst0, src0_lo);
    }
  else if (sop == 3 && sopcde == 7)
    {
      bu16 tmp;
      bu16 src0_lo = (bu16)(DREG (src0) & 0xFFFF);

      TRACE_INSN (cpu, "R%i.L = EXPADJ (R%i.H, R%i.L);", dst0, src1, src0);

      tmp = signbits ((DREG (src1) & 0xFFFF0000) >> 16, 16);

      if ((tmp & 0xf) < (src0_lo & 0xf))
	SET_DREG_L (dst0, tmp);
      else
	SET_DREG_L (dst0, src0_lo);
    }
  else if (sop == 0 && sopcde == 8)
    {
      bu64 acc = get_unextended_acc (cpu, 0);
      bu32 s0, s1;

      TRACE_INSN (cpu, "BITMUX (R%i, R%i, A0) (ASR);", src0, src1);

      if (src0 == src1)
	illegal_instruction_combination (cpu);

      s0 = DREG (src0);
      s1 = DREG (src1);
      acc = (acc >> 2) |
	(((bu64)s0 & 1) << 38) |
	(((bu64)s1 & 1) << 39);
      STORE (DREG (src0), s0 >> 1);
      STORE (DREG (src1), s1 >> 1);

      SET_AREG (0, acc);
    }
  else if (sop == 1 && sopcde == 8)
    {
      bu64 acc = get_unextended_acc (cpu, 0);
      bu32 s0, s1;

      TRACE_INSN (cpu, "BITMUX (R%i, R%i, A0) (ASL);", src0, src1);

      if (src0 == src1)
	illegal_instruction_combination (cpu);

      s0 = DREG (src0);
      s1 = DREG (src1);
      acc = (acc << 2) |
	((s0 >> 31) & 1) |
	((s1 >> 30) & 2);
      STORE (DREG (src0), s0 << 1);
      STORE (DREG (src1), s1 << 1);

      SET_AREG (0, acc);
    }
  else if ((sop == 0 || sop == 1) && sopcde == 9)
    {
      bs40 acc0 = get_unextended_acc (cpu, 0);
      bs16 sL, sH, out;

      TRACE_INSN (cpu, "R%i.L = VIT_MAX (R%i) (AS%c);",
		  dst0, src1, sop & 1 ? 'R' : 'L');

      sL = DREG (src1);
      sH = DREG (src1) >> 16;

      if (sop & 1)
	acc0 = (acc0 & 0xfeffffffffull) >> 1;
      else
	acc0 <<= 1;

      if (((sH - sL) & 0x8000) == 0)
	{
	  out = sH;
	  acc0 |= (sop & 1) ? 0x80000000 : 1;
	}
      else
	out = sL;

      SET_AREG (0, acc0);
      STORE (DREG (dst0), REG_H_L (DREG (dst0), out));
    }
  else if ((sop == 2 || sop == 3) && sopcde == 9)
    {
      bs40 acc0 = get_extended_acc (cpu, 0);
      bs16 s0L, s0H, s1L, s1H, out0, out1;

      TRACE_INSN (cpu, "R%i = VIT_MAX (R%i, R%i) (AS%c);",
		  dst0, src1, src0, sop & 1 ? 'R' : 'L');

      s0L = DREG (src0);
      s0H = DREG (src0) >> 16;
      s1L = DREG (src1);
      s1H = DREG (src1) >> 16;

      if (sop & 1)
	acc0 >>= 2;
      else
	acc0 <<= 2;

      if (((s0H - s0L) & 0x8000) == 0)
	{
	  out0 = s0H;
	  acc0 |= (sop & 1) ? 0x40000000 : 2;
	}
      else
	out0 = s0L;

      if (((s1H - s1L) & 0x8000) == 0)
	{
	  out1 = s1H;
	  acc0 |= (sop & 1) ? 0x80000000 : 1;
	}
      else
	out1 = s1L;

      SET_AREG (0, acc0);
      STORE (DREG (dst0), REG_H_L (out1 << 16, out0));
    }
  else if (sop == 0 && sopcde == 10)
    {
      bu32 v = DREG (src0);
      bu32 x = DREG (src1);
      bu32 mask = (1 << (v & 0x1f)) - 1;

      TRACE_INSN (cpu, "R%i = EXTRACT (R%i, R%i.L) (Z);", dst0, src1, src0);

      x >>= ((v >> 8) & 0x1f);
      x &= mask;
      STORE (DREG (dst0), x);
      setflags_logical (cpu, x);
    }
  else if (sop == 1 && sopcde == 10)
    {
      bu32 v = DREG (src0);
      bu32 x = DREG (src1);
      bu32 sgn = (1 << (v & 0x1f)) >> 1;
      bu32 mask = (1 << (v & 0x1f)) - 1;

      TRACE_INSN (cpu, "R%i = EXTRACT (R%i, R%i.L) (X);", dst0, src1, src0);

      x >>= ((v >> 8) & 0x1f);
      x &= mask;
      if (x & sgn)
	x |= ~mask;
      STORE (DREG (dst0), x);
      setflags_logical (cpu, x);
    }
  else if ((sop == 2 || sop == 3) && sopcde == 10)
    {
      /* The first dregs is the "background" while the second dregs is the
         "foreground".  The fg reg is used to overlay the bg reg and is:
         | nnnn nnnn | nnnn nnnn | xxxp pppp | xxxL LLLL |
           n = the fg bit field
           p = bit position in bg reg to start LSB of fg field
           L = number of fg bits to extract
         Using (X) sign-extends the fg bit field.  */
      bu32 fg = DREG (src0);
      bu32 bg = DREG (src1);
      bu32 len = fg & 0x1f;
      bu32 mask = (1 << min (16, len)) - 1;
      bu32 fgnd = (fg >> 16) & mask;
      int shft = ((fg >> 8) & 0x1f);

      TRACE_INSN (cpu, "R%i = DEPOSIT (R%i, R%i)%s;", dst0, src1, src0,
		  sop == 3 ? " (X)" : "");

      if (sop == 3)
	{
	  /* Sign extend the fg bit field.  */
	  mask = -1;
	  fgnd = ((bs32)(bs16)(fgnd << (16 - len))) >> (16 - len);
	}
      fgnd <<= shft;
      mask <<= shft;
      bg &= ~mask;

      bg |= fgnd;
      STORE (DREG (dst0), bg);
      setflags_logical (cpu, bg);
    }
  else if (sop == 0 && sopcde == 11)
    {
      bu64 acc0 = get_unextended_acc (cpu, 0);

      TRACE_INSN (cpu, "R%i.L = CC = BXORSHIFT (A0, R%i);", dst0, src0);

      acc0 <<= 1;
      SET_CCREG (xor_reduce (acc0, DREG (src0)));
      SET_DREG (dst0, REG_H_L (DREG (dst0), CCREG));
      SET_AREG (0, acc0);
    }
  else if (sop == 1 && sopcde == 11)
    {
      bu64 acc0 = get_unextended_acc (cpu, 0);

      TRACE_INSN (cpu, "R%i.L = CC = BXOR (A0, R%i);", dst0, src0);

      SET_CCREG (xor_reduce (acc0, DREG (src0)));
      SET_DREG (dst0, REG_H_L (DREG (dst0), CCREG));
    }
  else if (sop == 0 && sopcde == 12)
    {
      bu64 acc0 = get_unextended_acc (cpu, 0);
      bu64 acc1 = get_unextended_acc (cpu, 1);

      TRACE_INSN (cpu, "A0 = BXORSHIFT (A0, A1, CC);");

      acc0 = (acc0 << 1) | (CCREG ^ xor_reduce (acc0, acc1));
      SET_AREG (0, acc0);
    }
  else if (sop == 1 && sopcde == 12)
    {
      bu64 acc0 = get_unextended_acc (cpu, 0);
      bu64 acc1 = get_unextended_acc (cpu, 1);

      TRACE_INSN (cpu, "R%i.L = CC = BXOR (A0, A1, CC);", dst0);

      SET_CCREG (CCREG ^ xor_reduce (acc0, acc1));
      acc0 = (acc0 << 1) | CCREG;
      SET_DREG (dst0, REG_H_L (DREG (dst0), CCREG));
    }
  else if ((sop == 0 || sop == 1 || sop == 2) && sopcde == 13)
    {
      int shift = (sop + 1) * 8;
      TRACE_INSN (cpu, "R%i = ALIGN%i (R%i, R%i);", dst0, shift, src1, src0);
      STORE (DREG (dst0), (DREG (src1) << (32 - shift)) | (DREG (src0) >> shift));
    }
  else
    illegal_instruction (cpu);
}

static bu64
sgn_extend (bu40 org, bu40 val, int size)
{
  bu64 ret = val;

  if (org & (1ULL << (size - 1)))
    {
      /* We need to shift in to the MSB which is set.  */
      int n;

      for (n = 40; n >= 0; n--)
	if (ret & (1ULL << n))
	  break;
      ret |= (-1ULL << n);
    }
  else
    ret &= ~(-1ULL << 39);

  return ret;
}
static void
decode_dsp32shiftimm_0 (SIM_CPU *cpu, bu16 iw0, bu16 iw1)
{
  /* dsp32shiftimm
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 1 | 0 | 0 |.M.| 1 | 1 | 0 | 1 | - | - |.sopcde............|
     |.sop...|.HLs...|.dst0......|.immag.................|.src1......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int src1     = ((iw1 >> DSP32ShiftImm_src1_bits) & DSP32ShiftImm_src1_mask);
  int sop      = ((iw1 >> DSP32ShiftImm_sop_bits) & DSP32ShiftImm_sop_mask);
  int bit8     = ((iw1 >> 8) & 0x1);
  int immag    = ((iw1 >> DSP32ShiftImm_immag_bits) & DSP32ShiftImm_immag_mask);
  int newimmag = (-(iw1 >> DSP32ShiftImm_immag_bits) & DSP32ShiftImm_immag_mask);
  int dst0     = ((iw1 >> DSP32ShiftImm_dst0_bits) & DSP32ShiftImm_dst0_mask);
  int M        = ((iw0 >> (DSP32ShiftImm_M_bits - 16)) & DSP32ShiftImm_M_mask);
  int sopcde   = ((iw0 >> (DSP32ShiftImm_sopcde_bits - 16)) & DSP32ShiftImm_sopcde_mask);
  int HLs      = ((iw1 >> DSP32ShiftImm_HLs_bits) & DSP32ShiftImm_HLs_mask);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_dsp32shiftimm);
  TRACE_EXTRACT (cpu, "%s: M:%i sopcde:%i sop:%i HLs:%i dst0:%i immag:%#x src1:%i",
		 __func__, M, sopcde, sop, HLs, dst0, immag, src1);

  if (sopcde == 0)
    {
      bu16 in = DREG (src1) >> ((HLs & 1) ? 16 : 0);
      bu16 result;
      bu32 v;

      if (sop == 0)
	{
	  TRACE_INSN (cpu, "R%i.%c = R%i.%c >>> %i;",
		      dst0, (HLs & 2) ? 'H' : 'L',
		      src1, (HLs & 1) ? 'H' : 'L', newimmag);
	  if (newimmag > 16)
	    {
	      result = lshift (cpu, in, 16 - (newimmag & 0xF), 16, 0, 1);
	      if (((result >> 15) & 0x1) != ((in >> 15) & 0x1))
		{
		  SET_ASTATREG (v, 1);
		  SET_ASTATREG (vs, 1);
		}
	    }
	  else
	    result = ashiftrt (cpu, in, newimmag, 16);
	}
      else if (sop == 1 && bit8 == 0)
	{
	  TRACE_INSN (cpu, "R%i.%c = R%i.%c << %i (S);",
		      dst0, (HLs & 2) ? 'H' : 'L',
		      src1, (HLs & 1) ? 'H' : 'L', immag);
	  result = lshift (cpu, in, immag, 16, 1, 1);
	}
      else if (sop == 1 && bit8)
	{
	  TRACE_INSN (cpu, "R%i.%c = R%i.%c >>> %i (S);",
		      dst0, (HLs & 2) ? 'H' : 'L',
		      src1, (HLs & 1) ? 'H' : 'L', newimmag);
	  if (newimmag > 16)
	    {
	      int shift = 32 - newimmag;
	      bu16 inshift = in << shift;

	      if (((inshift & ~0xFFFF)
		   && ((inshift & ~0xFFFF) >> 16) != ~((bu32)~0 << shift))
		  || (inshift & 0x8000) != (in & 0x8000))
		{
		  if (in & 0x8000)
		    result = 0x8000;
		  else
		    result = 0x7fff;
		  SET_ASTATREG (v, 1);
		  SET_ASTATREG (vs, 1);
		}
	      else
		{
		  result = inshift;
		  SET_ASTATREG (v, 0);
		}

	      SET_ASTATREG (az, !result);
	      SET_ASTATREG (an, !!(result & 0x8000));
	    }
	  else
	    {
	      result = ashiftrt (cpu, in, newimmag, 16);
	      result = sgn_extend (in, result, 16);
	    }
	}
      else if (sop == 2 && bit8)
	{
	  TRACE_INSN (cpu, "R%i.%c = R%i.%c >> %i;",
		      dst0, (HLs & 2) ? 'H' : 'L',
		      src1, (HLs & 1) ? 'H' : 'L', newimmag);
	  result = lshiftrt (cpu, in, newimmag, 16);
	}
      else if (sop == 2 && bit8 == 0)
	{
	  TRACE_INSN (cpu, "R%i.%c = R%i.%c << %i;",
		      dst0, (HLs & 2) ? 'H' : 'L',
		      src1, (HLs & 1) ? 'H' : 'L', immag);
	  result = lshift (cpu, in, immag, 16, 0, 1);
	}
      else
	illegal_instruction (cpu);

      v = DREG (dst0);
      if (HLs & 2)
	STORE (DREG (dst0), (v & 0xFFFF) | (result << 16));
      else
	STORE (DREG (dst0), (v & 0xFFFF0000) | result);
    }
  else if (sop == 2 && sopcde == 3 && (HLs == 1 || HLs == 0))
    {
      int shift = imm6 (immag);
      bu32 cc = CCREG;
      bu40 acc = get_unextended_acc (cpu, HLs);

      TRACE_INSN (cpu, "A%i = ROT A%i BY %i;", HLs, HLs, shift);
      TRACE_DECODE (cpu, "A%i:%#"PRIx64" shift:%i CC:%i", HLs, acc, shift, cc);

      acc = rot40 (acc, shift, &cc);
      SET_AREG (HLs, acc);
      if (shift)
	SET_CCREG (cc);
    }
  else if (sop == 0 && sopcde == 3 && bit8 == 1 && HLs < 2)
    {
      /* Arithmetic shift, so shift in sign bit copies.  */
      bu64 acc, val;
      int shift = uimm5 (newimmag);

      TRACE_INSN (cpu, "A%i = A%i >>> %i;", HLs, HLs, shift);

      acc = get_extended_acc (cpu, HLs);
      val = acc >> shift;

      /* Sign extend again.  */
      val = sgn_extend (acc, val, 40);

      STORE (AXREG (HLs), (val >> 32) & 0xFF);
      STORE (AWREG (HLs), val & 0xFFFFFFFF);
      STORE (ASTATREG (an), !!(val & (1ULL << 39)));
      STORE (ASTATREG (az), !val);
      STORE (ASTATREG (av[HLs]), 0);
    }
  else if (((sop == 0 && sopcde == 3 && bit8 == 0)
	   || (sop == 1 && sopcde == 3)) && HLs < 2)
    {
      bu64 acc;
      int shiftup = uimm5 (immag);
      int shiftdn = uimm5 (newimmag);

      TRACE_INSN (cpu, "A%i = A%i %s %i;", HLs, HLs,
		  sop == 0 ? "<<" : ">>",
		  sop == 0 ? shiftup : shiftdn);

      acc = AXREG (HLs);
      /* Logical shift, so shift in zeroes.  */
      acc &= 0xFF;
      acc <<= 32;
      acc |= AWREG (HLs);

      if (sop == 0)
	acc <<= shiftup;
      else
	{
	  if (shiftdn <= 32)
	    acc >>= shiftdn;
	  else
	    acc <<= 32 - (shiftdn & 0x1f);
	}

      SET_AREG (HLs, acc);
      SET_ASTATREG (av[HLs], 0);
      SET_ASTATREG (an, !!(acc & 0x8000000000ull));
      SET_ASTATREG (az, (acc & 0xFFFFFFFFFF) == 0);
    }
  else if (HLs != 0)
    /* All the insns after this point don't use HLs.  */
    illegal_instruction (cpu);
  else if (sop == 1 && sopcde == 1 && bit8 == 0)
    {
      int count = imm5 (immag);
      bu16 val0 = DREG (src1) >> 16;
      bu16 val1 = DREG (src1) & 0xFFFF;
      bu32 astat;

      TRACE_INSN (cpu, "R%i = R%i << %i (V,S);", dst0, src1, count);
      if (count >= 0)
	{
	  val0 = lshift (cpu, val0, count, 16, 1, 1);
	  astat = ASTAT;
	  val1 = lshift (cpu, val1, count, 16, 1, 1);
	}
      else
	{
	  val0 = ashiftrt (cpu, val0, -count, 16);
	  astat = ASTAT;
	  val1 = ashiftrt (cpu, val1, -count, 16);
	}
      SET_ASTAT (ASTAT | astat);

      STORE (DREG (dst0), (val0 << 16) | val1);
    }
  else if (sop == 2 && sopcde == 1 && bit8 == 1)
    {
      int count = imm5 (newimmag);
      bu16 val0 = DREG (src1) & 0xFFFF;
      bu16 val1 = DREG (src1) >> 16;
      bu32 astat;

      TRACE_INSN (cpu, "R%i = R%i >> %i (V);", dst0, src1, count);
      val0 = lshiftrt (cpu, val0, count, 16);
      astat = ASTAT;
      val1 = lshiftrt (cpu, val1, count, 16);
      SET_ASTAT (ASTAT | astat);

      STORE (DREG (dst0), val0 | (val1 << 16));
    }
  else if (sop == 2 && sopcde == 1 && bit8 == 0)
    {
      int count = imm5 (immag);
      bu16 val0 = DREG (src1) & 0xFFFF;
      bu16 val1 = DREG (src1) >> 16;
      bu32 astat;

      TRACE_INSN (cpu, "R%i = R%i << %i (V);", dst0, src1, count);
      val0 = lshift (cpu, val0, count, 16, 0, 1);
      astat = ASTAT;
      val1 = lshift (cpu, val1, count, 16, 0, 1);
      SET_ASTAT (ASTAT | astat);

      STORE (DREG (dst0), val0 | (val1 << 16));
    }
  else if (sopcde == 1 && (sop == 0 || (sop == 1 && bit8 == 1)))
    {
      int count = uimm5 (newimmag);
      bu16 val0 = DREG (src1) & 0xFFFF;
      bu16 val1 = DREG (src1) >> 16;
      bu32 astat;

      TRACE_INSN (cpu, "R%i = R%i >>> %i %s;", dst0, src1, count,
		  sop == 0 ? "(V)" : "(V,S)");

      if (count > 16)
	{
	  int sgn0 = (val0 >> 15) & 0x1;
	  int sgn1 = (val1 >> 15) & 0x1;

	  val0 = lshift (cpu, val0, 16 - (count & 0xF), 16, 0, 1);
	  astat = ASTAT;
	  val1 = lshift (cpu, val1, 16 - (count & 0xF), 16, 0, 1);

	  if ((sgn0 != ((val0 >> 15) & 0x1)) || (sgn1 != ((val1 >> 15) & 0x1)))
	    {
	      SET_ASTATREG (v, 1);
	      SET_ASTATREG (vs, 1);
	    }
	}
      else
	{
	  val0 = ashiftrt (cpu, val0, count, 16);
	  astat = ASTAT;
	  val1 = ashiftrt (cpu, val1, count, 16);
	}

      SET_ASTAT (ASTAT | astat);

      STORE (DREG (dst0), REG_H_L (val1 << 16, val0));
    }
  else if (sop == 1 && sopcde == 2)
    {
      int count = imm6 (immag);

      TRACE_INSN (cpu, "R%i = R%i << %i (S);", dst0, src1, count);

      if (count < 0)
	STORE (DREG (dst0), ashiftrt (cpu, DREG (src1), -count, 32));
      else
	STORE (DREG (dst0), lshift (cpu, DREG (src1), count, 32, 1, 1));
    }
  else if (sop == 2 && sopcde == 2)
    {
      int count = imm6 (newimmag);

      TRACE_INSN (cpu, "R%i = R%i >> %i;", dst0, src1, count);

      if (count < 0)
	STORE (DREG (dst0), lshift (cpu, DREG (src1), -count, 32, 0, 1));
      else
	STORE (DREG (dst0), lshiftrt (cpu, DREG (src1), count, 32));
    }
  else if (sop == 3 && sopcde == 2)
    {
      int shift = imm6 (immag);
      bu32 src = DREG (src1);
      bu32 ret, cc = CCREG;

      TRACE_INSN (cpu, "R%i = ROT R%i BY %i;", dst0, src1, shift);
      TRACE_DECODE (cpu, "R%i:%#x R%i:%#x shift:%i CC:%i",
		    dst0, DREG (dst0), src1, src, shift, cc);

      ret = rot32 (src, shift, &cc);
      STORE (DREG (dst0), ret);
      if (shift)
	SET_CCREG (cc);
    }
  else if (sop == 0 && sopcde == 2)
    {
      int count = imm6 (newimmag);

      TRACE_INSN (cpu, "R%i = R%i >>> %i;", dst0, src1, count);

      if (count < 0)
	STORE (DREG (dst0), lshift (cpu, DREG (src1), -count, 32, 0, 1));
      else
	STORE (DREG (dst0), ashiftrt (cpu, DREG (src1), count, 32));
    }
  else
    illegal_instruction (cpu);
}

static void
outc (SIM_CPU *cpu, char ch)
{
  SIM_DESC sd = CPU_STATE (cpu);
  sim_io_printf (sd, "%c", ch);
  if (ch == '\n')
    sim_io_flush_stdout (sd);
}

static void
decode_psedoDEBUG_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* psedoDEBUG
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 1 | 1 | 1 | 1 | 0 | 0 | 0 |.fn....|.grp.......|.reg.......|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  SIM_DESC sd = CPU_STATE (cpu);
  int fn  = ((iw0 >> PseudoDbg_fn_bits) & PseudoDbg_fn_mask);
  int grp = ((iw0 >> PseudoDbg_grp_bits) & PseudoDbg_grp_mask);
  int reg = ((iw0 >> PseudoDbg_reg_bits) & PseudoDbg_reg_mask);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_psedoDEBUG);
  TRACE_EXTRACT (cpu, "%s: fn:%i grp:%i reg:%i", __func__, fn, grp, reg);

  if ((reg == 0 || reg == 1) && fn == 3)
    {
      TRACE_INSN (cpu, "DBG A%i;", reg);
      sim_io_printf (sd, "DBG : A%i = %#"PRIx64"\n", reg,
		     get_unextended_acc (cpu, reg));
    }
  else if (reg == 3 && fn == 3)
    {
      TRACE_INSN (cpu, "ABORT;");
      cec_exception (cpu, VEC_SIM_ABORT);
      SET_DREG (0, 1);
    }
  else if (reg == 4 && fn == 3)
    {
      TRACE_INSN (cpu, "HLT;");
      cec_exception (cpu, VEC_SIM_HLT);
      SET_DREG (0, 0);
    }
  else if (reg == 5 && fn == 3)
    unhandled_instruction (cpu, "DBGHALT");
  else if (reg == 6 && fn == 3)
    unhandled_instruction (cpu, "DBGCMPLX (dregs)");
  else if (reg == 7 && fn == 3)
    unhandled_instruction (cpu, "DBG");
  else if (grp == 0 && fn == 2)
    {
      TRACE_INSN (cpu, "OUTC R%i;", reg);
      outc (cpu, DREG (reg));
    }
  else if (fn == 0)
    {
      const char *reg_name = get_allreg_name (grp, reg);
      TRACE_INSN (cpu, "DBG %s;", reg_name);
      sim_io_printf (sd, "DBG : %s = 0x%08x\n", reg_name,
		     reg_read (cpu, grp, reg));
    }
  else if (fn == 1)
    unhandled_instruction (cpu, "PRNT allregs");
  else
    illegal_instruction (cpu);
}

static void
decode_psedoOChar_0 (SIM_CPU *cpu, bu16 iw0)
{
  /* psedoOChar
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 1 | 1 | 1 | 1 | 0 | 0 | 1 |.ch............................|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  int ch = ((iw0 >> PseudoChr_ch_bits) & PseudoChr_ch_mask);

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_psedoOChar);
  TRACE_EXTRACT (cpu, "%s: ch:%#x", __func__, ch);
  TRACE_INSN (cpu, "OUTC %#x;", ch);

  outc (cpu, ch);
}

static void
decode_psedodbg_assert_0 (SIM_CPU *cpu, bu16 iw0, bu16 iw1, bu32 pc)
{
  /* psedodbg_assert
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+
     | 1 | 1 | 1 | 1 | 0 | - | - | - | dbgop |.grp.......|.regtest...|
     |.expected......................................................|
     +---+---+---+---|---+---+---+---|---+---+---+---|---+---+---+---+  */
  SIM_DESC sd = CPU_STATE (cpu);
  int expected = ((iw1 >> PseudoDbg_Assert_expected_bits) & PseudoDbg_Assert_expected_mask);
  int dbgop    = ((iw0 >> (PseudoDbg_Assert_dbgop_bits - 16)) & PseudoDbg_Assert_dbgop_mask);
  int grp      = ((iw0 >> (PseudoDbg_Assert_grp_bits - 16)) & PseudoDbg_Assert_grp_mask);
  int regtest  = ((iw0 >> (PseudoDbg_Assert_regtest_bits - 16)) & PseudoDbg_Assert_regtest_mask);
  int offset;
  bu16 actual;
  bu32 val = reg_read (cpu, grp, regtest);
  const char *reg_name = get_allreg_name (grp, regtest);
  const char *dbg_name, *dbg_appd;

  PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_psedodbg_assert);
  TRACE_EXTRACT (cpu, "%s: dbgop:%i grp:%i regtest:%i expected:%#x",
		 __func__, dbgop, grp, regtest, expected);

  if (dbgop == 0 || dbgop == 2)
    {
      dbg_name = dbgop == 0 ? "DBGA" : "DBGAL";
      dbg_appd = dbgop == 0 ? ".L" : "";
      offset = 0;
    }
  else if (dbgop == 1 || dbgop == 3)
    {
      dbg_name = dbgop == 1 ? "DBGA" : "DBGAH";
      dbg_appd = dbgop == 1 ? ".H" : "";
      offset = 16;
    }
  else
    illegal_instruction (cpu);

  actual = val >> offset;

  TRACE_INSN (cpu, "%s (%s%s, 0x%x);", dbg_name, reg_name, dbg_appd, expected);
  if (actual != expected)
    {
      sim_io_printf (sd, "FAIL at %#x: %s (%s%s, 0x%04x); actual value %#x\n",
		     pc, dbg_name, reg_name, dbg_appd, expected, actual);

      /* Decode the actual ASTAT bits that are different.  */
      if (grp == 4 && regtest == 6)
	{
	  int i;

	  sim_io_printf (sd, "Expected ASTAT:\n");
	  for (i = 0; i < 16; ++i)
	    sim_io_printf (sd, " %8s%c%i%s",
			   astat_names[i + offset],
			   (((expected >> i) & 1) != ((actual >> i) & 1))
				? '!' : ' ',
			   (expected >> i) & 1,
			   i == 7 ? "\n" : "");
	  sim_io_printf (sd, "\n");

	  sim_io_printf (sd, "Actual ASTAT:\n");
	  for (i = 0; i < 16; ++i)
	    sim_io_printf (sd, " %8s%c%i%s",
			   astat_names[i + offset],
			   (((expected >> i) & 1) != ((actual >> i) & 1))
				? '!' : ' ',
			   (actual >> i) & 1,
			   i == 7 ? "\n" : "");
	  sim_io_printf (sd, "\n");
	}

      cec_exception (cpu, VEC_SIM_DBGA);
      SET_DREG (0, 1);
    }
}

static bu32
_interp_insn_bfin (SIM_CPU *cpu, bu32 pc)
{
  bu32 insn_len;
  bu16 iw0, iw1;

  BFIN_CPU_STATE.multi_pc = pc;
  iw0 = IFETCH (pc);
  if ((iw0 & 0xc000) != 0xc000)
    {
      /* 16-bit opcode.  */
      insn_len = 2;
      if (INSN_LEN == 0)
	INSN_LEN = insn_len;

      TRACE_EXTRACT (cpu, "%s: iw0:%#x", __func__, iw0);
      if ((iw0 & 0xFF00) == 0x0000)
	decode_ProgCtrl_0 (cpu, iw0, pc);
      else if ((iw0 & 0xFFC0) == 0x0240)
	decode_CaCTRL_0 (cpu, iw0);
      else if ((iw0 & 0xFF80) == 0x0100)
	decode_PushPopReg_0 (cpu, iw0);
      else if ((iw0 & 0xFE00) == 0x0400)
	decode_PushPopMultiple_0 (cpu, iw0);
      else if ((iw0 & 0xFE00) == 0x0600)
	decode_ccMV_0 (cpu, iw0);
      else if ((iw0 & 0xF800) == 0x0800)
	decode_CCflag_0 (cpu, iw0);
      else if ((iw0 & 0xFFE0) == 0x0200)
	decode_CC2dreg_0 (cpu, iw0);
      else if ((iw0 & 0xFF00) == 0x0300)
	decode_CC2stat_0 (cpu, iw0);
      else if ((iw0 & 0xF000) == 0x1000)
	decode_BRCC_0 (cpu, iw0, pc);
      else if ((iw0 & 0xF000) == 0x2000)
	decode_UJUMP_0 (cpu, iw0, pc);
      else if ((iw0 & 0xF000) == 0x3000)
	decode_REGMV_0 (cpu, iw0);
      else if ((iw0 & 0xFC00) == 0x4000)
	decode_ALU2op_0 (cpu, iw0);
      else if ((iw0 & 0xFE00) == 0x4400)
	decode_PTR2op_0 (cpu, iw0);
      else if ((iw0 & 0xF800) == 0x4800)
	decode_LOGI2op_0 (cpu, iw0);
      else if ((iw0 & 0xF000) == 0x5000)
	decode_COMP3op_0 (cpu, iw0);
      else if ((iw0 & 0xF800) == 0x6000)
	decode_COMPI2opD_0 (cpu, iw0);
      else if ((iw0 & 0xF800) == 0x6800)
	decode_COMPI2opP_0 (cpu, iw0);
      else if ((iw0 & 0xF000) == 0x8000)
	decode_LDSTpmod_0 (cpu, iw0);
      else if ((iw0 & 0xFF60) == 0x9E60)
	decode_dagMODim_0 (cpu, iw0);
      else if ((iw0 & 0xFFF0) == 0x9F60)
	decode_dagMODik_0 (cpu, iw0);
      else if ((iw0 & 0xFC00) == 0x9C00)
	decode_dspLDST_0 (cpu, iw0);
      else if ((iw0 & 0xF000) == 0x9000)
	decode_LDST_0 (cpu, iw0);
      else if ((iw0 & 0xFC00) == 0xB800)
	decode_LDSTiiFP_0 (cpu, iw0);
      else if ((iw0 & 0xE000) == 0xA000)
	decode_LDSTii_0 (cpu, iw0);
      else
	{
	  TRACE_EXTRACT (cpu, "%s: no matching 16-bit pattern", __func__);
	  illegal_instruction_or_combination (cpu);
	}
      return insn_len;
    }

  /* Grab the next 16 bits to determine if it's a 32-bit or 64-bit opcode.  */
  iw1 = IFETCH (pc + 2);
  if ((iw0 & BIT_MULTI_INS) && (iw0 & 0xe800) != 0xe800 /* not linkage  */)
    {
      SIM_DESC sd = CPU_STATE (cpu);
      trace_prefix (sd, cpu, NULL_CIA, pc, TRACE_LINENUM_P (cpu),
			NULL, 0, "|| %#"PRIx64, sim_events_time (sd));
      insn_len = 8;
      PARALLEL_GROUP = BFIN_PARALLEL_GROUP0;
    }
  else
    insn_len = 4;

  TRACE_EXTRACT (cpu, "%s: iw0:%#x iw1:%#x insn_len:%i", __func__,
		     iw0, iw1, insn_len);

  /* Only cache on first run through (in case of parallel insns).  */
  if (INSN_LEN == 0)
    INSN_LEN = insn_len;
  else
    /* Once you're past the first slot, only 16bit insns are valid.  */
    illegal_instruction_combination (cpu);

  if ((iw0 & 0xf7ff) == 0xc003 && (iw1 & 0xfe00) == 0x1800)
    {
      PROFILE_COUNT_INSN (cpu, pc, BFIN_INSN_dsp32mac);
      TRACE_INSN (cpu, "MNOP;");
    }
  else if (((iw0 & 0xFF80) == 0xE080) && ((iw1 & 0x0C00) == 0x0000))
    decode_LoopSetup_0 (cpu, iw0, iw1, pc);
  else if (((iw0 & 0xFF00) == 0xE100) && ((iw1 & 0x0000) == 0x0000))
    decode_LDIMMhalf_0 (cpu, iw0, iw1);
  else if (((iw0 & 0xFE00) == 0xE200) && ((iw1 & 0x0000) == 0x0000))
    decode_CALLa_0 (cpu, iw0, iw1, pc);
  else if (((iw0 & 0xFC00) == 0xE400) && ((iw1 & 0x0000) == 0x0000))
    decode_LDSTidxI_0 (cpu, iw0, iw1);
  else if (((iw0 & 0xFFFE) == 0xE800) && ((iw1 & 0x0000) == 0x0000))
    decode_linkage_0 (cpu, iw0, iw1);
  else if (((iw0 & 0xF600) == 0xC000) && ((iw1 & 0x0000) == 0x0000))
    decode_dsp32mac_0 (cpu, iw0, iw1);
  else if (((iw0 & 0xF600) == 0xC200) && ((iw1 & 0x0000) == 0x0000))
    decode_dsp32mult_0 (cpu, iw0, iw1);
  else if (((iw0 & 0xF7C0) == 0xC400) && ((iw1 & 0x0000) == 0x0000))
    decode_dsp32alu_0 (cpu, iw0, iw1);
  else if (((iw0 & 0xF7E0) == 0xC600) && ((iw1 & 0x01C0) == 0x0000))
    decode_dsp32shift_0 (cpu, iw0, iw1);
  else if (((iw0 & 0xF7E0) == 0xC680) && ((iw1 & 0x0000) == 0x0000))
    decode_dsp32shiftimm_0 (cpu, iw0, iw1);
  else if ((iw0 & 0xFF00) == 0xF800)
    decode_psedoDEBUG_0 (cpu, iw0), insn_len = 2;
  else if ((iw0 & 0xFF00) == 0xF900)
    decode_psedoOChar_0 (cpu, iw0), insn_len = 2;
  else if (((iw0 & 0xFF00) == 0xF000) && ((iw1 & 0x0000) == 0x0000))
    decode_psedodbg_assert_0 (cpu, iw0, iw1, pc);
  else
    {
      TRACE_EXTRACT (cpu, "%s: no matching 32-bit pattern", __func__);
      illegal_instruction (cpu);
    }

  return insn_len;
}

bu32
interp_insn_bfin (SIM_CPU *cpu, bu32 pc)
{
  int i;
  bu32 insn_len;

  BFIN_CPU_STATE.n_stores = 0;
  PARALLEL_GROUP = BFIN_PARALLEL_NONE;
  DIS_ALGN_EXPT &= ~1;
  CYCLE_DELAY = 1;
  INSN_LEN = 0;

  insn_len = _interp_insn_bfin (cpu, pc);

  /* Proper display of multiple issue instructions.  */
  if (insn_len == 8)
    {
      PARALLEL_GROUP = BFIN_PARALLEL_GROUP1;
      _interp_insn_bfin (cpu, pc + 4);
      PARALLEL_GROUP = BFIN_PARALLEL_GROUP2;
      _interp_insn_bfin (cpu, pc + 6);
    }
  for (i = 0; i < BFIN_CPU_STATE.n_stores; i++)
    {
      bu32 *addr = BFIN_CPU_STATE.stores[i].addr;
      *addr = BFIN_CPU_STATE.stores[i].val;
      TRACE_REGISTER (cpu, "dequeuing write %s = %#x",
		      get_store_name (cpu, addr), *addr);
    }

  cycles_inc (cpu, CYCLE_DELAY);

  /* Set back to zero in case a pending CEC event occurs
     after this this insn.  */
  INSN_LEN = 0;

  return insn_len;
}
