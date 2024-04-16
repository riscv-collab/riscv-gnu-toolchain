/* Simulator instruction decoder for lm32bf.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996-2024 Free Software Foundation, Inc.

This file is part of the GNU simulators.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.

*/

#define WANT_CPU lm32bf
#define WANT_CPU_LM32BF

#include "sim-main.h"
#include "sim-assert.h"
#include "cgen-mem.h"
#include "cgen-ops.h"

/* The instruction descriptor array.
   This is computed at runtime.  Space for it is not malloc'd to save a
   teensy bit of cpu in the decoder.  Moving it to malloc space is trivial
   but won't be done until necessary (we don't currently support the runtime
   addition of instructions nor an SMP machine with different cpus).  */
static IDESC lm32bf_insn_data[LM32BF_INSN__MAX];

/* Commas between elements are contained in the macros.
   Some of these are conditionally compiled out.  */

static const struct insn_sem lm32bf_insn_sem[] =
{
  { VIRTUAL_INSN_X_INVALID, LM32BF_INSN_X_INVALID, LM32BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_AFTER, LM32BF_INSN_X_AFTER, LM32BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEFORE, LM32BF_INSN_X_BEFORE, LM32BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CTI_CHAIN, LM32BF_INSN_X_CTI_CHAIN, LM32BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CHAIN, LM32BF_INSN_X_CHAIN, LM32BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEGIN, LM32BF_INSN_X_BEGIN, LM32BF_SFMT_EMPTY },
  { LM32_INSN_ADD, LM32BF_INSN_ADD, LM32BF_SFMT_ADD },
  { LM32_INSN_ADDI, LM32BF_INSN_ADDI, LM32BF_SFMT_ADDI },
  { LM32_INSN_AND, LM32BF_INSN_AND, LM32BF_SFMT_ADD },
  { LM32_INSN_ANDI, LM32BF_INSN_ANDI, LM32BF_SFMT_ANDI },
  { LM32_INSN_ANDHII, LM32BF_INSN_ANDHII, LM32BF_SFMT_ANDHII },
  { LM32_INSN_B, LM32BF_INSN_B, LM32BF_SFMT_B },
  { LM32_INSN_BI, LM32BF_INSN_BI, LM32BF_SFMT_BI },
  { LM32_INSN_BE, LM32BF_INSN_BE, LM32BF_SFMT_BE },
  { LM32_INSN_BG, LM32BF_INSN_BG, LM32BF_SFMT_BE },
  { LM32_INSN_BGE, LM32BF_INSN_BGE, LM32BF_SFMT_BE },
  { LM32_INSN_BGEU, LM32BF_INSN_BGEU, LM32BF_SFMT_BE },
  { LM32_INSN_BGU, LM32BF_INSN_BGU, LM32BF_SFMT_BE },
  { LM32_INSN_BNE, LM32BF_INSN_BNE, LM32BF_SFMT_BE },
  { LM32_INSN_CALL, LM32BF_INSN_CALL, LM32BF_SFMT_CALL },
  { LM32_INSN_CALLI, LM32BF_INSN_CALLI, LM32BF_SFMT_CALLI },
  { LM32_INSN_CMPE, LM32BF_INSN_CMPE, LM32BF_SFMT_ADD },
  { LM32_INSN_CMPEI, LM32BF_INSN_CMPEI, LM32BF_SFMT_ADDI },
  { LM32_INSN_CMPG, LM32BF_INSN_CMPG, LM32BF_SFMT_ADD },
  { LM32_INSN_CMPGI, LM32BF_INSN_CMPGI, LM32BF_SFMT_ADDI },
  { LM32_INSN_CMPGE, LM32BF_INSN_CMPGE, LM32BF_SFMT_ADD },
  { LM32_INSN_CMPGEI, LM32BF_INSN_CMPGEI, LM32BF_SFMT_ADDI },
  { LM32_INSN_CMPGEU, LM32BF_INSN_CMPGEU, LM32BF_SFMT_ADD },
  { LM32_INSN_CMPGEUI, LM32BF_INSN_CMPGEUI, LM32BF_SFMT_ANDI },
  { LM32_INSN_CMPGU, LM32BF_INSN_CMPGU, LM32BF_SFMT_ADD },
  { LM32_INSN_CMPGUI, LM32BF_INSN_CMPGUI, LM32BF_SFMT_ANDI },
  { LM32_INSN_CMPNE, LM32BF_INSN_CMPNE, LM32BF_SFMT_ADD },
  { LM32_INSN_CMPNEI, LM32BF_INSN_CMPNEI, LM32BF_SFMT_ADDI },
  { LM32_INSN_DIVU, LM32BF_INSN_DIVU, LM32BF_SFMT_DIVU },
  { LM32_INSN_LB, LM32BF_INSN_LB, LM32BF_SFMT_LB },
  { LM32_INSN_LBU, LM32BF_INSN_LBU, LM32BF_SFMT_LB },
  { LM32_INSN_LH, LM32BF_INSN_LH, LM32BF_SFMT_LH },
  { LM32_INSN_LHU, LM32BF_INSN_LHU, LM32BF_SFMT_LH },
  { LM32_INSN_LW, LM32BF_INSN_LW, LM32BF_SFMT_LW },
  { LM32_INSN_MODU, LM32BF_INSN_MODU, LM32BF_SFMT_DIVU },
  { LM32_INSN_MUL, LM32BF_INSN_MUL, LM32BF_SFMT_ADD },
  { LM32_INSN_MULI, LM32BF_INSN_MULI, LM32BF_SFMT_ADDI },
  { LM32_INSN_NOR, LM32BF_INSN_NOR, LM32BF_SFMT_ADD },
  { LM32_INSN_NORI, LM32BF_INSN_NORI, LM32BF_SFMT_ANDI },
  { LM32_INSN_OR, LM32BF_INSN_OR, LM32BF_SFMT_ADD },
  { LM32_INSN_ORI, LM32BF_INSN_ORI, LM32BF_SFMT_ORI },
  { LM32_INSN_ORHII, LM32BF_INSN_ORHII, LM32BF_SFMT_ANDHII },
  { LM32_INSN_RCSR, LM32BF_INSN_RCSR, LM32BF_SFMT_RCSR },
  { LM32_INSN_SB, LM32BF_INSN_SB, LM32BF_SFMT_SB },
  { LM32_INSN_SEXTB, LM32BF_INSN_SEXTB, LM32BF_SFMT_SEXTB },
  { LM32_INSN_SEXTH, LM32BF_INSN_SEXTH, LM32BF_SFMT_SEXTB },
  { LM32_INSN_SH, LM32BF_INSN_SH, LM32BF_SFMT_SH },
  { LM32_INSN_SL, LM32BF_INSN_SL, LM32BF_SFMT_ADD },
  { LM32_INSN_SLI, LM32BF_INSN_SLI, LM32BF_SFMT_ADDI },
  { LM32_INSN_SR, LM32BF_INSN_SR, LM32BF_SFMT_ADD },
  { LM32_INSN_SRI, LM32BF_INSN_SRI, LM32BF_SFMT_ADDI },
  { LM32_INSN_SRU, LM32BF_INSN_SRU, LM32BF_SFMT_ADD },
  { LM32_INSN_SRUI, LM32BF_INSN_SRUI, LM32BF_SFMT_ADDI },
  { LM32_INSN_SUB, LM32BF_INSN_SUB, LM32BF_SFMT_ADD },
  { LM32_INSN_SW, LM32BF_INSN_SW, LM32BF_SFMT_SW },
  { LM32_INSN_USER, LM32BF_INSN_USER, LM32BF_SFMT_USER },
  { LM32_INSN_WCSR, LM32BF_INSN_WCSR, LM32BF_SFMT_WCSR },
  { LM32_INSN_XOR, LM32BF_INSN_XOR, LM32BF_SFMT_ADD },
  { LM32_INSN_XORI, LM32BF_INSN_XORI, LM32BF_SFMT_ANDI },
  { LM32_INSN_XNOR, LM32BF_INSN_XNOR, LM32BF_SFMT_ADD },
  { LM32_INSN_XNORI, LM32BF_INSN_XNORI, LM32BF_SFMT_ANDI },
  { LM32_INSN_BREAK, LM32BF_INSN_BREAK, LM32BF_SFMT_BREAK },
  { LM32_INSN_SCALL, LM32BF_INSN_SCALL, LM32BF_SFMT_BREAK },
};

static const struct insn_sem lm32bf_insn_sem_invalid =
{
  VIRTUAL_INSN_X_INVALID, LM32BF_INSN_X_INVALID, LM32BF_SFMT_EMPTY
};

/* Initialize an IDESC from the compile-time computable parts.  */

static INLINE void
init_idesc (SIM_CPU *cpu, IDESC *id, const struct insn_sem *t)
{
  const CGEN_INSN *insn_table = CGEN_CPU_INSN_TABLE (CPU_CPU_DESC (cpu))->init_entries;

  id->num = t->index;
  id->sfmt = t->sfmt;
  if ((int) t->type <= 0)
    id->idata = & cgen_virtual_insn_table[- (int) t->type];
  else
    id->idata = & insn_table[t->type];
  id->attrs = CGEN_INSN_ATTRS (id->idata);
  /* Oh my god, a magic number.  */
  id->length = CGEN_INSN_BITSIZE (id->idata) / 8;

#if WITH_PROFILE_MODEL_P
  id->timing = & MODEL_TIMING (CPU_MODEL (cpu)) [t->index];
  {
    SIM_DESC sd = CPU_STATE (cpu);
    SIM_ASSERT (t->index == id->timing->num);
  }
#endif

  /* Semantic pointers are initialized elsewhere.  */
}

/* Initialize the instruction descriptor table.  */

void
lm32bf_init_idesc_table (SIM_CPU *cpu)
{
  IDESC *id,*tabend;
  const struct insn_sem *t,*tend;
  int tabsize = LM32BF_INSN__MAX;
  IDESC *table = lm32bf_insn_data;

  memset (table, 0, tabsize * sizeof (IDESC));

  /* First set all entries to the `invalid insn'.  */
  t = & lm32bf_insn_sem_invalid;
  for (id = table, tabend = table + tabsize; id < tabend; ++id)
    init_idesc (cpu, id, t);

  /* Now fill in the values for the chosen cpu.  */
  for (t = lm32bf_insn_sem, tend = t + ARRAY_SIZE (lm32bf_insn_sem);
       t != tend; ++t)
    {
      init_idesc (cpu, & table[t->index], t);
    }

  /* Link the IDESC table into the cpu.  */
  CPU_IDESC (cpu) = table;
}

/* Given an instruction, return a pointer to its IDESC entry.  */

const IDESC *
lm32bf_decode (SIM_CPU *current_cpu, IADDR pc,
              CGEN_INSN_WORD base_insn, CGEN_INSN_WORD entire_insn,
              ARGBUF *abuf)
{
  /* Result of decoder.  */
  LM32BF_INSN_TYPE itype;

  {
    CGEN_INSN_WORD insn = base_insn;

    {
      unsigned int val0 = (((insn >> 26) & (63 << 0)));
      switch (val0)
      {
      case 0: itype = LM32BF_INSN_SRUI; goto extract_sfmt_addi;
      case 1: itype = LM32BF_INSN_NORI; goto extract_sfmt_andi;
      case 2: itype = LM32BF_INSN_MULI; goto extract_sfmt_addi;
      case 3: itype = LM32BF_INSN_SH; goto extract_sfmt_sh;
      case 4: itype = LM32BF_INSN_LB; goto extract_sfmt_lb;
      case 5: itype = LM32BF_INSN_SRI; goto extract_sfmt_addi;
      case 6: itype = LM32BF_INSN_XORI; goto extract_sfmt_andi;
      case 7: itype = LM32BF_INSN_LH; goto extract_sfmt_lh;
      case 8: itype = LM32BF_INSN_ANDI; goto extract_sfmt_andi;
      case 9: itype = LM32BF_INSN_XNORI; goto extract_sfmt_andi;
      case 10: itype = LM32BF_INSN_LW; goto extract_sfmt_lw;
      case 11: itype = LM32BF_INSN_LHU; goto extract_sfmt_lh;
      case 12: itype = LM32BF_INSN_SB; goto extract_sfmt_sb;
      case 13: itype = LM32BF_INSN_ADDI; goto extract_sfmt_addi;
      case 14: itype = LM32BF_INSN_ORI; goto extract_sfmt_ori;
      case 15: itype = LM32BF_INSN_SLI; goto extract_sfmt_addi;
      case 16: itype = LM32BF_INSN_LBU; goto extract_sfmt_lb;
      case 17: itype = LM32BF_INSN_BE; goto extract_sfmt_be;
      case 18: itype = LM32BF_INSN_BG; goto extract_sfmt_be;
      case 19: itype = LM32BF_INSN_BGE; goto extract_sfmt_be;
      case 20: itype = LM32BF_INSN_BGEU; goto extract_sfmt_be;
      case 21: itype = LM32BF_INSN_BGU; goto extract_sfmt_be;
      case 22: itype = LM32BF_INSN_SW; goto extract_sfmt_sw;
      case 23: itype = LM32BF_INSN_BNE; goto extract_sfmt_be;
      case 24: itype = LM32BF_INSN_ANDHII; goto extract_sfmt_andhii;
      case 25: itype = LM32BF_INSN_CMPEI; goto extract_sfmt_addi;
      case 26: itype = LM32BF_INSN_CMPGI; goto extract_sfmt_addi;
      case 27: itype = LM32BF_INSN_CMPGEI; goto extract_sfmt_addi;
      case 28: itype = LM32BF_INSN_CMPGEUI; goto extract_sfmt_andi;
      case 29: itype = LM32BF_INSN_CMPGUI; goto extract_sfmt_andi;
      case 30: itype = LM32BF_INSN_ORHII; goto extract_sfmt_andhii;
      case 31: itype = LM32BF_INSN_CMPNEI; goto extract_sfmt_addi;
      case 32:
        if ((entire_insn & 0xfc0007ff) == 0x80000000)
          { itype = LM32BF_INSN_SRU; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 33:
        if ((entire_insn & 0xfc0007ff) == 0x84000000)
          { itype = LM32BF_INSN_NOR; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 34:
        if ((entire_insn & 0xfc0007ff) == 0x88000000)
          { itype = LM32BF_INSN_MUL; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 35:
        if ((entire_insn & 0xfc0007ff) == 0x8c000000)
          { itype = LM32BF_INSN_DIVU; goto extract_sfmt_divu; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 36:
        if ((entire_insn & 0xfc1f07ff) == 0x90000000)
          { itype = LM32BF_INSN_RCSR; goto extract_sfmt_rcsr; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 37:
        if ((entire_insn & 0xfc0007ff) == 0x94000000)
          { itype = LM32BF_INSN_SR; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 38:
        if ((entire_insn & 0xfc0007ff) == 0x98000000)
          { itype = LM32BF_INSN_XOR; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 40:
        if ((entire_insn & 0xfc0007ff) == 0xa0000000)
          { itype = LM32BF_INSN_AND; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 41:
        if ((entire_insn & 0xfc0007ff) == 0xa4000000)
          { itype = LM32BF_INSN_XNOR; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 43:
        {
          unsigned int val1 = (((insn >> 1) & (1 << 1)) | ((insn >> 0) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffffffff) == 0xac000002)
              { itype = LM32BF_INSN_BREAK; goto extract_sfmt_break; }
            itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 3:
            if ((entire_insn & 0xffffffff) == 0xac000007)
              { itype = LM32BF_INSN_SCALL; goto extract_sfmt_break; }
            itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 44:
        if ((entire_insn & 0xfc1f07ff) == 0xb0000000)
          { itype = LM32BF_INSN_SEXTB; goto extract_sfmt_sextb; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 45:
        if ((entire_insn & 0xfc0007ff) == 0xb4000000)
          { itype = LM32BF_INSN_ADD; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 46:
        if ((entire_insn & 0xfc0007ff) == 0xb8000000)
          { itype = LM32BF_INSN_OR; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 47:
        if ((entire_insn & 0xfc0007ff) == 0xbc000000)
          { itype = LM32BF_INSN_SL; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 48:
        if ((entire_insn & 0xfc1fffff) == 0xc0000000)
          { itype = LM32BF_INSN_B; goto extract_sfmt_b; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 49:
        if ((entire_insn & 0xfc0007ff) == 0xc4000000)
          { itype = LM32BF_INSN_MODU; goto extract_sfmt_divu; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 50:
        if ((entire_insn & 0xfc0007ff) == 0xc8000000)
          { itype = LM32BF_INSN_SUB; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 51: itype = LM32BF_INSN_USER; goto extract_sfmt_user;
      case 52:
        if ((entire_insn & 0xfc00ffff) == 0xd0000000)
          { itype = LM32BF_INSN_WCSR; goto extract_sfmt_wcsr; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 54:
        if ((entire_insn & 0xfc1fffff) == 0xd8000000)
          { itype = LM32BF_INSN_CALL; goto extract_sfmt_call; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 55:
        if ((entire_insn & 0xfc1f07ff) == 0xdc000000)
          { itype = LM32BF_INSN_SEXTH; goto extract_sfmt_sextb; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 56: itype = LM32BF_INSN_BI; goto extract_sfmt_bi;
      case 57:
        if ((entire_insn & 0xfc0007ff) == 0xe4000000)
          { itype = LM32BF_INSN_CMPE; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 58:
        if ((entire_insn & 0xfc0007ff) == 0xe8000000)
          { itype = LM32BF_INSN_CMPG; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 59:
        if ((entire_insn & 0xfc0007ff) == 0xec000000)
          { itype = LM32BF_INSN_CMPGE; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 60:
        if ((entire_insn & 0xfc0007ff) == 0xf0000000)
          { itype = LM32BF_INSN_CMPGEU; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 61:
        if ((entire_insn & 0xfc0007ff) == 0xf4000000)
          { itype = LM32BF_INSN_CMPGU; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 62: itype = LM32BF_INSN_CALLI; goto extract_sfmt_calli;
      case 63:
        if ((entire_insn & 0xfc0007ff) == 0xfc000000)
          { itype = LM32BF_INSN_CMPNE; goto extract_sfmt_add; }
        itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      default: itype = LM32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      }
    }
  }

  /* The instruction has been decoded, now extract the fields.  */

 extract_sfmt_empty:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_empty", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_add:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_user.f
    UINT f_r0;
    UINT f_r1;
    UINT f_r2;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r0) = f_r0;
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_add", "f_r0 0x%x", 'x', f_r0, "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_addi:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_addi.f
    UINT f_r0;
    UINT f_r1;
    INT f_imm;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_imm = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_imm) = f_imm;
  FLD (f_r0) = f_r0;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addi", "f_imm 0x%x", 'x', f_imm, "f_r0 0x%x", 'x', f_r0, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_andi:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_andi.f
    UINT f_r0;
    UINT f_r1;
    UINT f_uimm;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_uimm = EXTRACT_LSB0_UINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r0) = f_r0;
  FLD (f_uimm) = f_uimm;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_andi", "f_r0 0x%x", 'x', f_r0, "f_uimm 0x%x", 'x', f_uimm, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_andhii:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_andi.f
    UINT f_r0;
    UINT f_r1;
    UINT f_uimm;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_uimm = EXTRACT_LSB0_UINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_uimm) = f_uimm;
  FLD (f_r0) = f_r0;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_andhii", "f_uimm 0x%x", 'x', f_uimm, "f_r0 0x%x", 'x', f_r0, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_b:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_be.f
    UINT f_r0;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r0) = f_r0;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_b", "f_r0 0x%x", 'x', f_r0, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_bi:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_bi.f
    SI f_call;

    f_call = ((pc) + (((((((((EXTRACT_LSB0_SINT (insn, 32, 25, 26)) & (67108863))) << (2))) ^ (134217728))) - (134217728))));

  /* Record the fields for the semantic handler.  */
  FLD (i_call) = f_call;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bi", "call 0x%x", 'x', f_call, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_be:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_be.f
    UINT f_r0;
    UINT f_r1;
    SI f_branch;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_branch = ((pc) + (((((((((EXTRACT_LSB0_SINT (insn, 32, 15, 16)) & (65535))) << (2))) ^ (131072))) - (131072))));

  /* Record the fields for the semantic handler.  */
  FLD (f_r0) = f_r0;
  FLD (f_r1) = f_r1;
  FLD (i_branch) = f_branch;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_be", "f_r0 0x%x", 'x', f_r0, "f_r1 0x%x", 'x', f_r1, "branch 0x%x", 'x', f_branch, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_call:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_be.f
    UINT f_r0;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r0) = f_r0;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_call", "f_r0 0x%x", 'x', f_r0, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_calli:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_bi.f
    SI f_call;

    f_call = ((pc) + (((((((((EXTRACT_LSB0_SINT (insn, 32, 25, 26)) & (67108863))) << (2))) ^ (134217728))) - (134217728))));

  /* Record the fields for the semantic handler.  */
  FLD (i_call) = f_call;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_calli", "call 0x%x", 'x', f_call, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_divu:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_user.f
    UINT f_r0;
    UINT f_r1;
    UINT f_r2;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r0) = f_r0;
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_divu", "f_r0 0x%x", 'x', f_r0, "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lb:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_addi.f
    UINT f_r0;
    UINT f_r1;
    INT f_imm;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_imm = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_imm) = f_imm;
  FLD (f_r0) = f_r0;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lb", "f_imm 0x%x", 'x', f_imm, "f_r0 0x%x", 'x', f_r0, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lh:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_addi.f
    UINT f_r0;
    UINT f_r1;
    INT f_imm;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_imm = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_imm) = f_imm;
  FLD (f_r0) = f_r0;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lh", "f_imm 0x%x", 'x', f_imm, "f_r0 0x%x", 'x', f_r0, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lw:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_addi.f
    UINT f_r0;
    UINT f_r1;
    INT f_imm;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_imm = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_imm) = f_imm;
  FLD (f_r0) = f_r0;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lw", "f_imm 0x%x", 'x', f_imm, "f_r0 0x%x", 'x', f_r0, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_ori:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_andi.f
    UINT f_r0;
    UINT f_r1;
    UINT f_uimm;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_uimm = EXTRACT_LSB0_UINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_uimm) = f_uimm;
  FLD (f_r0) = f_r0;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ori", "f_uimm 0x%x", 'x', f_uimm, "f_r0 0x%x", 'x', f_r0, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_rcsr:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_rcsr.f
    UINT f_csr;
    UINT f_r2;

    f_csr = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_csr) = f_csr;
  FLD (f_r2) = f_r2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_rcsr", "f_csr 0x%x", 'x', f_csr, "f_r2 0x%x", 'x', f_r2, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_sb:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_addi.f
    UINT f_r0;
    UINT f_r1;
    INT f_imm;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_imm = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_imm) = f_imm;
  FLD (f_r0) = f_r0;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_sb", "f_imm 0x%x", 'x', f_imm, "f_r0 0x%x", 'x', f_r0, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_sextb:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_user.f
    UINT f_r0;
    UINT f_r2;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r0) = f_r0;
  FLD (f_r2) = f_r2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_sextb", "f_r0 0x%x", 'x', f_r0, "f_r2 0x%x", 'x', f_r2, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_sh:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_addi.f
    UINT f_r0;
    UINT f_r1;
    INT f_imm;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_imm = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_imm) = f_imm;
  FLD (f_r0) = f_r0;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_sh", "f_imm 0x%x", 'x', f_imm, "f_r0 0x%x", 'x', f_r0, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_sw:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_addi.f
    UINT f_r0;
    UINT f_r1;
    INT f_imm;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_imm = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_imm) = f_imm;
  FLD (f_r0) = f_r0;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_sw", "f_imm 0x%x", 'x', f_imm, "f_r0 0x%x", 'x', f_r0, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_user:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_user.f
    UINT f_r0;
    UINT f_r1;
    UINT f_r2;
    UINT f_user;

    f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_user = EXTRACT_LSB0_UINT (insn, 32, 10, 11);

  /* Record the fields for the semantic handler.  */
  FLD (f_r0) = f_r0;
  FLD (f_r1) = f_r1;
  FLD (f_user) = f_user;
  FLD (f_r2) = f_r2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_user", "f_r0 0x%x", 'x', f_r0, "f_r1 0x%x", 'x', f_r1, "f_user 0x%x", 'x', f_user, "f_r2 0x%x", 'x', f_r2, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_wcsr:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_wcsr.f
    UINT f_csr;
    UINT f_r1;

    f_csr = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_csr) = f_csr;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_wcsr", "f_csr 0x%x", 'x', f_csr, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_break:
  {
    const IDESC *idesc = &lm32bf_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_break", (char *) 0));

#undef FLD
    return idesc;
  }

}
