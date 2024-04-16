/* Simulator instruction decoder for m32rbf.

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

#define WANT_CPU m32rbf
#define WANT_CPU_M32RBF

#include "sim-main.h"
#include "sim-assert.h"
#include "cgen-mem.h"
#include "cgen-ops.h"

/* The instruction descriptor array.
   This is computed at runtime.  Space for it is not malloc'd to save a
   teensy bit of cpu in the decoder.  Moving it to malloc space is trivial
   but won't be done until necessary (we don't currently support the runtime
   addition of instructions nor an SMP machine with different cpus).  */
static IDESC m32rbf_insn_data[M32RBF_INSN__MAX];

/* Commas between elements are contained in the macros.
   Some of these are conditionally compiled out.  */

static const struct insn_sem m32rbf_insn_sem[] =
{
  { VIRTUAL_INSN_X_INVALID, M32RBF_INSN_X_INVALID, M32RBF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_AFTER, M32RBF_INSN_X_AFTER, M32RBF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEFORE, M32RBF_INSN_X_BEFORE, M32RBF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CTI_CHAIN, M32RBF_INSN_X_CTI_CHAIN, M32RBF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CHAIN, M32RBF_INSN_X_CHAIN, M32RBF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEGIN, M32RBF_INSN_X_BEGIN, M32RBF_SFMT_EMPTY },
  { M32R_INSN_ADD, M32RBF_INSN_ADD, M32RBF_SFMT_ADD },
  { M32R_INSN_ADD3, M32RBF_INSN_ADD3, M32RBF_SFMT_ADD3 },
  { M32R_INSN_AND, M32RBF_INSN_AND, M32RBF_SFMT_ADD },
  { M32R_INSN_AND3, M32RBF_INSN_AND3, M32RBF_SFMT_AND3 },
  { M32R_INSN_OR, M32RBF_INSN_OR, M32RBF_SFMT_ADD },
  { M32R_INSN_OR3, M32RBF_INSN_OR3, M32RBF_SFMT_OR3 },
  { M32R_INSN_XOR, M32RBF_INSN_XOR, M32RBF_SFMT_ADD },
  { M32R_INSN_XOR3, M32RBF_INSN_XOR3, M32RBF_SFMT_AND3 },
  { M32R_INSN_ADDI, M32RBF_INSN_ADDI, M32RBF_SFMT_ADDI },
  { M32R_INSN_ADDV, M32RBF_INSN_ADDV, M32RBF_SFMT_ADDV },
  { M32R_INSN_ADDV3, M32RBF_INSN_ADDV3, M32RBF_SFMT_ADDV3 },
  { M32R_INSN_ADDX, M32RBF_INSN_ADDX, M32RBF_SFMT_ADDX },
  { M32R_INSN_BC8, M32RBF_INSN_BC8, M32RBF_SFMT_BC8 },
  { M32R_INSN_BC24, M32RBF_INSN_BC24, M32RBF_SFMT_BC24 },
  { M32R_INSN_BEQ, M32RBF_INSN_BEQ, M32RBF_SFMT_BEQ },
  { M32R_INSN_BEQZ, M32RBF_INSN_BEQZ, M32RBF_SFMT_BEQZ },
  { M32R_INSN_BGEZ, M32RBF_INSN_BGEZ, M32RBF_SFMT_BEQZ },
  { M32R_INSN_BGTZ, M32RBF_INSN_BGTZ, M32RBF_SFMT_BEQZ },
  { M32R_INSN_BLEZ, M32RBF_INSN_BLEZ, M32RBF_SFMT_BEQZ },
  { M32R_INSN_BLTZ, M32RBF_INSN_BLTZ, M32RBF_SFMT_BEQZ },
  { M32R_INSN_BNEZ, M32RBF_INSN_BNEZ, M32RBF_SFMT_BEQZ },
  { M32R_INSN_BL8, M32RBF_INSN_BL8, M32RBF_SFMT_BL8 },
  { M32R_INSN_BL24, M32RBF_INSN_BL24, M32RBF_SFMT_BL24 },
  { M32R_INSN_BNC8, M32RBF_INSN_BNC8, M32RBF_SFMT_BC8 },
  { M32R_INSN_BNC24, M32RBF_INSN_BNC24, M32RBF_SFMT_BC24 },
  { M32R_INSN_BNE, M32RBF_INSN_BNE, M32RBF_SFMT_BEQ },
  { M32R_INSN_BRA8, M32RBF_INSN_BRA8, M32RBF_SFMT_BRA8 },
  { M32R_INSN_BRA24, M32RBF_INSN_BRA24, M32RBF_SFMT_BRA24 },
  { M32R_INSN_CMP, M32RBF_INSN_CMP, M32RBF_SFMT_CMP },
  { M32R_INSN_CMPI, M32RBF_INSN_CMPI, M32RBF_SFMT_CMPI },
  { M32R_INSN_CMPU, M32RBF_INSN_CMPU, M32RBF_SFMT_CMP },
  { M32R_INSN_CMPUI, M32RBF_INSN_CMPUI, M32RBF_SFMT_CMPI },
  { M32R_INSN_DIV, M32RBF_INSN_DIV, M32RBF_SFMT_DIV },
  { M32R_INSN_DIVU, M32RBF_INSN_DIVU, M32RBF_SFMT_DIV },
  { M32R_INSN_REM, M32RBF_INSN_REM, M32RBF_SFMT_DIV },
  { M32R_INSN_REMU, M32RBF_INSN_REMU, M32RBF_SFMT_DIV },
  { M32R_INSN_JL, M32RBF_INSN_JL, M32RBF_SFMT_JL },
  { M32R_INSN_JMP, M32RBF_INSN_JMP, M32RBF_SFMT_JMP },
  { M32R_INSN_LD, M32RBF_INSN_LD, M32RBF_SFMT_LD },
  { M32R_INSN_LD_D, M32RBF_INSN_LD_D, M32RBF_SFMT_LD_D },
  { M32R_INSN_LDB, M32RBF_INSN_LDB, M32RBF_SFMT_LDB },
  { M32R_INSN_LDB_D, M32RBF_INSN_LDB_D, M32RBF_SFMT_LDB_D },
  { M32R_INSN_LDH, M32RBF_INSN_LDH, M32RBF_SFMT_LDH },
  { M32R_INSN_LDH_D, M32RBF_INSN_LDH_D, M32RBF_SFMT_LDH_D },
  { M32R_INSN_LDUB, M32RBF_INSN_LDUB, M32RBF_SFMT_LDB },
  { M32R_INSN_LDUB_D, M32RBF_INSN_LDUB_D, M32RBF_SFMT_LDB_D },
  { M32R_INSN_LDUH, M32RBF_INSN_LDUH, M32RBF_SFMT_LDH },
  { M32R_INSN_LDUH_D, M32RBF_INSN_LDUH_D, M32RBF_SFMT_LDH_D },
  { M32R_INSN_LD_PLUS, M32RBF_INSN_LD_PLUS, M32RBF_SFMT_LD_PLUS },
  { M32R_INSN_LD24, M32RBF_INSN_LD24, M32RBF_SFMT_LD24 },
  { M32R_INSN_LDI8, M32RBF_INSN_LDI8, M32RBF_SFMT_LDI8 },
  { M32R_INSN_LDI16, M32RBF_INSN_LDI16, M32RBF_SFMT_LDI16 },
  { M32R_INSN_LOCK, M32RBF_INSN_LOCK, M32RBF_SFMT_LOCK },
  { M32R_INSN_MACHI, M32RBF_INSN_MACHI, M32RBF_SFMT_MACHI },
  { M32R_INSN_MACLO, M32RBF_INSN_MACLO, M32RBF_SFMT_MACHI },
  { M32R_INSN_MACWHI, M32RBF_INSN_MACWHI, M32RBF_SFMT_MACHI },
  { M32R_INSN_MACWLO, M32RBF_INSN_MACWLO, M32RBF_SFMT_MACHI },
  { M32R_INSN_MUL, M32RBF_INSN_MUL, M32RBF_SFMT_ADD },
  { M32R_INSN_MULHI, M32RBF_INSN_MULHI, M32RBF_SFMT_MULHI },
  { M32R_INSN_MULLO, M32RBF_INSN_MULLO, M32RBF_SFMT_MULHI },
  { M32R_INSN_MULWHI, M32RBF_INSN_MULWHI, M32RBF_SFMT_MULHI },
  { M32R_INSN_MULWLO, M32RBF_INSN_MULWLO, M32RBF_SFMT_MULHI },
  { M32R_INSN_MV, M32RBF_INSN_MV, M32RBF_SFMT_MV },
  { M32R_INSN_MVFACHI, M32RBF_INSN_MVFACHI, M32RBF_SFMT_MVFACHI },
  { M32R_INSN_MVFACLO, M32RBF_INSN_MVFACLO, M32RBF_SFMT_MVFACHI },
  { M32R_INSN_MVFACMI, M32RBF_INSN_MVFACMI, M32RBF_SFMT_MVFACHI },
  { M32R_INSN_MVFC, M32RBF_INSN_MVFC, M32RBF_SFMT_MVFC },
  { M32R_INSN_MVTACHI, M32RBF_INSN_MVTACHI, M32RBF_SFMT_MVTACHI },
  { M32R_INSN_MVTACLO, M32RBF_INSN_MVTACLO, M32RBF_SFMT_MVTACHI },
  { M32R_INSN_MVTC, M32RBF_INSN_MVTC, M32RBF_SFMT_MVTC },
  { M32R_INSN_NEG, M32RBF_INSN_NEG, M32RBF_SFMT_MV },
  { M32R_INSN_NOP, M32RBF_INSN_NOP, M32RBF_SFMT_NOP },
  { M32R_INSN_NOT, M32RBF_INSN_NOT, M32RBF_SFMT_MV },
  { M32R_INSN_RAC, M32RBF_INSN_RAC, M32RBF_SFMT_RAC },
  { M32R_INSN_RACH, M32RBF_INSN_RACH, M32RBF_SFMT_RAC },
  { M32R_INSN_RTE, M32RBF_INSN_RTE, M32RBF_SFMT_RTE },
  { M32R_INSN_SETH, M32RBF_INSN_SETH, M32RBF_SFMT_SETH },
  { M32R_INSN_SLL, M32RBF_INSN_SLL, M32RBF_SFMT_ADD },
  { M32R_INSN_SLL3, M32RBF_INSN_SLL3, M32RBF_SFMT_SLL3 },
  { M32R_INSN_SLLI, M32RBF_INSN_SLLI, M32RBF_SFMT_SLLI },
  { M32R_INSN_SRA, M32RBF_INSN_SRA, M32RBF_SFMT_ADD },
  { M32R_INSN_SRA3, M32RBF_INSN_SRA3, M32RBF_SFMT_SLL3 },
  { M32R_INSN_SRAI, M32RBF_INSN_SRAI, M32RBF_SFMT_SLLI },
  { M32R_INSN_SRL, M32RBF_INSN_SRL, M32RBF_SFMT_ADD },
  { M32R_INSN_SRL3, M32RBF_INSN_SRL3, M32RBF_SFMT_SLL3 },
  { M32R_INSN_SRLI, M32RBF_INSN_SRLI, M32RBF_SFMT_SLLI },
  { M32R_INSN_ST, M32RBF_INSN_ST, M32RBF_SFMT_ST },
  { M32R_INSN_ST_D, M32RBF_INSN_ST_D, M32RBF_SFMT_ST_D },
  { M32R_INSN_STB, M32RBF_INSN_STB, M32RBF_SFMT_STB },
  { M32R_INSN_STB_D, M32RBF_INSN_STB_D, M32RBF_SFMT_STB_D },
  { M32R_INSN_STH, M32RBF_INSN_STH, M32RBF_SFMT_STH },
  { M32R_INSN_STH_D, M32RBF_INSN_STH_D, M32RBF_SFMT_STH_D },
  { M32R_INSN_ST_PLUS, M32RBF_INSN_ST_PLUS, M32RBF_SFMT_ST_PLUS },
  { M32R_INSN_ST_MINUS, M32RBF_INSN_ST_MINUS, M32RBF_SFMT_ST_PLUS },
  { M32R_INSN_SUB, M32RBF_INSN_SUB, M32RBF_SFMT_ADD },
  { M32R_INSN_SUBV, M32RBF_INSN_SUBV, M32RBF_SFMT_ADDV },
  { M32R_INSN_SUBX, M32RBF_INSN_SUBX, M32RBF_SFMT_ADDX },
  { M32R_INSN_TRAP, M32RBF_INSN_TRAP, M32RBF_SFMT_TRAP },
  { M32R_INSN_UNLOCK, M32RBF_INSN_UNLOCK, M32RBF_SFMT_UNLOCK },
  { M32R_INSN_CLRPSW, M32RBF_INSN_CLRPSW, M32RBF_SFMT_CLRPSW },
  { M32R_INSN_SETPSW, M32RBF_INSN_SETPSW, M32RBF_SFMT_SETPSW },
  { M32R_INSN_BSET, M32RBF_INSN_BSET, M32RBF_SFMT_BSET },
  { M32R_INSN_BCLR, M32RBF_INSN_BCLR, M32RBF_SFMT_BSET },
  { M32R_INSN_BTST, M32RBF_INSN_BTST, M32RBF_SFMT_BTST },
};

static const struct insn_sem m32rbf_insn_sem_invalid =
{
  VIRTUAL_INSN_X_INVALID, M32RBF_INSN_X_INVALID, M32RBF_SFMT_EMPTY
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
m32rbf_init_idesc_table (SIM_CPU *cpu)
{
  IDESC *id,*tabend;
  const struct insn_sem *t,*tend;
  int tabsize = M32RBF_INSN__MAX;
  IDESC *table = m32rbf_insn_data;

  memset (table, 0, tabsize * sizeof (IDESC));

  /* First set all entries to the `invalid insn'.  */
  t = & m32rbf_insn_sem_invalid;
  for (id = table, tabend = table + tabsize; id < tabend; ++id)
    init_idesc (cpu, id, t);

  /* Now fill in the values for the chosen cpu.  */
  for (t = m32rbf_insn_sem, tend = t + ARRAY_SIZE (m32rbf_insn_sem);
       t != tend; ++t)
    {
      init_idesc (cpu, & table[t->index], t);
    }

  /* Link the IDESC table into the cpu.  */
  CPU_IDESC (cpu) = table;
}

/* Given an instruction, return a pointer to its IDESC entry.  */

const IDESC *
m32rbf_decode (SIM_CPU *current_cpu, IADDR pc,
              CGEN_INSN_WORD base_insn, CGEN_INSN_WORD entire_insn,
              ARGBUF *abuf)
{
  /* Result of decoder.  */
  M32RBF_INSN_TYPE itype;

  {
    CGEN_INSN_WORD insn = base_insn;

    {
      unsigned int val0 = (((insn >> 8) & (15 << 4)) | ((insn >> 4) & (15 << 0)));
      switch (val0)
      {
      case 0: itype = M32RBF_INSN_SUBV; goto extract_sfmt_addv;
      case 1: itype = M32RBF_INSN_SUBX; goto extract_sfmt_addx;
      case 2: itype = M32RBF_INSN_SUB; goto extract_sfmt_add;
      case 3: itype = M32RBF_INSN_NEG; goto extract_sfmt_mv;
      case 4: itype = M32RBF_INSN_CMP; goto extract_sfmt_cmp;
      case 5: itype = M32RBF_INSN_CMPU; goto extract_sfmt_cmp;
      case 8: itype = M32RBF_INSN_ADDV; goto extract_sfmt_addv;
      case 9: itype = M32RBF_INSN_ADDX; goto extract_sfmt_addx;
      case 10: itype = M32RBF_INSN_ADD; goto extract_sfmt_add;
      case 11: itype = M32RBF_INSN_NOT; goto extract_sfmt_mv;
      case 12: itype = M32RBF_INSN_AND; goto extract_sfmt_add;
      case 13: itype = M32RBF_INSN_XOR; goto extract_sfmt_add;
      case 14: itype = M32RBF_INSN_OR; goto extract_sfmt_add;
      case 15:
        if ((entire_insn & 0xf8f0) == 0xf0)
          { itype = M32RBF_INSN_BTST; goto extract_sfmt_btst; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 16: itype = M32RBF_INSN_SRL; goto extract_sfmt_add;
      case 18: itype = M32RBF_INSN_SRA; goto extract_sfmt_add;
      case 20: itype = M32RBF_INSN_SLL; goto extract_sfmt_add;
      case 22: itype = M32RBF_INSN_MUL; goto extract_sfmt_add;
      case 24: itype = M32RBF_INSN_MV; goto extract_sfmt_mv;
      case 25: itype = M32RBF_INSN_MVFC; goto extract_sfmt_mvfc;
      case 26: itype = M32RBF_INSN_MVTC; goto extract_sfmt_mvtc;
      case 28:
        {
          unsigned int val1 = (((insn >> 8) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xfff0) == 0x1ec0)
              { itype = M32RBF_INSN_JL; goto extract_sfmt_jl; }
            itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xfff0) == 0x1fc0)
              { itype = M32RBF_INSN_JMP; goto extract_sfmt_jmp; }
            itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 29:
        if ((entire_insn & 0xffff) == 0x10d6)
          { itype = M32RBF_INSN_RTE; goto extract_sfmt_rte; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 31:
        if ((entire_insn & 0xfff0) == 0x10f0)
          { itype = M32RBF_INSN_TRAP; goto extract_sfmt_trap; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 32: itype = M32RBF_INSN_STB; goto extract_sfmt_stb;
      case 34: itype = M32RBF_INSN_STH; goto extract_sfmt_sth;
      case 36: itype = M32RBF_INSN_ST; goto extract_sfmt_st;
      case 37: itype = M32RBF_INSN_UNLOCK; goto extract_sfmt_unlock;
      case 38: itype = M32RBF_INSN_ST_PLUS; goto extract_sfmt_st_plus;
      case 39: itype = M32RBF_INSN_ST_MINUS; goto extract_sfmt_st_plus;
      case 40: itype = M32RBF_INSN_LDB; goto extract_sfmt_ldb;
      case 41: itype = M32RBF_INSN_LDUB; goto extract_sfmt_ldb;
      case 42: itype = M32RBF_INSN_LDH; goto extract_sfmt_ldh;
      case 43: itype = M32RBF_INSN_LDUH; goto extract_sfmt_ldh;
      case 44: itype = M32RBF_INSN_LD; goto extract_sfmt_ld;
      case 45: itype = M32RBF_INSN_LOCK; goto extract_sfmt_lock;
      case 46: itype = M32RBF_INSN_LD_PLUS; goto extract_sfmt_ld_plus;
      case 48: itype = M32RBF_INSN_MULHI; goto extract_sfmt_mulhi;
      case 49: itype = M32RBF_INSN_MULLO; goto extract_sfmt_mulhi;
      case 50: itype = M32RBF_INSN_MULWHI; goto extract_sfmt_mulhi;
      case 51: itype = M32RBF_INSN_MULWLO; goto extract_sfmt_mulhi;
      case 52: itype = M32RBF_INSN_MACHI; goto extract_sfmt_machi;
      case 53: itype = M32RBF_INSN_MACLO; goto extract_sfmt_machi;
      case 54: itype = M32RBF_INSN_MACWHI; goto extract_sfmt_machi;
      case 55: itype = M32RBF_INSN_MACWLO; goto extract_sfmt_machi;
      case 64:
      case 65:
      case 66:
      case 67:
      case 68:
      case 69:
      case 70:
      case 71:
      case 72:
      case 73:
      case 74:
      case 75:
      case 76:
      case 77:
      case 78:
      case 79: itype = M32RBF_INSN_ADDI; goto extract_sfmt_addi;
      case 80:
      case 81: itype = M32RBF_INSN_SRLI; goto extract_sfmt_slli;
      case 82:
      case 83: itype = M32RBF_INSN_SRAI; goto extract_sfmt_slli;
      case 84:
      case 85: itype = M32RBF_INSN_SLLI; goto extract_sfmt_slli;
      case 87:
        {
          unsigned int val1 = (((insn >> 0) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xf0ff) == 0x5070)
              { itype = M32RBF_INSN_MVTACHI; goto extract_sfmt_mvtachi; }
            itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xf0ff) == 0x5071)
              { itype = M32RBF_INSN_MVTACLO; goto extract_sfmt_mvtachi; }
            itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 88:
        if ((entire_insn & 0xffff) == 0x5080)
          { itype = M32RBF_INSN_RACH; goto extract_sfmt_rac; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 89:
        if ((entire_insn & 0xffff) == 0x5090)
          { itype = M32RBF_INSN_RAC; goto extract_sfmt_rac; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 95:
        {
          unsigned int val1 = (((insn >> 0) & (3 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xf0ff) == 0x50f0)
              { itype = M32RBF_INSN_MVFACHI; goto extract_sfmt_mvfachi; }
            itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xf0ff) == 0x50f1)
              { itype = M32RBF_INSN_MVFACLO; goto extract_sfmt_mvfachi; }
            itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 2:
            if ((entire_insn & 0xf0ff) == 0x50f2)
              { itype = M32RBF_INSN_MVFACMI; goto extract_sfmt_mvfachi; }
            itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 96:
      case 97:
      case 98:
      case 99:
      case 100:
      case 101:
      case 102:
      case 103:
      case 104:
      case 105:
      case 106:
      case 107:
      case 108:
      case 109:
      case 110:
      case 111: itype = M32RBF_INSN_LDI8; goto extract_sfmt_ldi8;
      case 112:
        {
          unsigned int val1 = (((insn >> 8) & (15 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffff) == 0x7000)
              { itype = M32RBF_INSN_NOP; goto extract_sfmt_nop; }
            itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1: itype = M32RBF_INSN_SETPSW; goto extract_sfmt_setpsw;
          case 2: itype = M32RBF_INSN_CLRPSW; goto extract_sfmt_clrpsw;
          case 12: itype = M32RBF_INSN_BC8; goto extract_sfmt_bc8;
          case 13: itype = M32RBF_INSN_BNC8; goto extract_sfmt_bc8;
          case 14: itype = M32RBF_INSN_BL8; goto extract_sfmt_bl8;
          case 15: itype = M32RBF_INSN_BRA8; goto extract_sfmt_bra8;
          default: itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 113:
      case 114:
      case 115:
      case 116:
      case 117:
      case 118:
      case 119:
      case 120:
      case 121:
      case 122:
      case 123:
      case 124:
      case 125:
      case 126:
      case 127:
        {
          unsigned int val1 = (((insn >> 8) & (15 << 0)));
          switch (val1)
          {
          case 1: itype = M32RBF_INSN_SETPSW; goto extract_sfmt_setpsw;
          case 2: itype = M32RBF_INSN_CLRPSW; goto extract_sfmt_clrpsw;
          case 12: itype = M32RBF_INSN_BC8; goto extract_sfmt_bc8;
          case 13: itype = M32RBF_INSN_BNC8; goto extract_sfmt_bc8;
          case 14: itype = M32RBF_INSN_BL8; goto extract_sfmt_bl8;
          case 15: itype = M32RBF_INSN_BRA8; goto extract_sfmt_bra8;
          default: itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 132:
        if ((entire_insn & 0xfff00000) == 0x80400000)
          { itype = M32RBF_INSN_CMPI; goto extract_sfmt_cmpi; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 133:
        if ((entire_insn & 0xfff00000) == 0x80500000)
          { itype = M32RBF_INSN_CMPUI; goto extract_sfmt_cmpi; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 136: itype = M32RBF_INSN_ADDV3; goto extract_sfmt_addv3;
      case 138: itype = M32RBF_INSN_ADD3; goto extract_sfmt_add3;
      case 140: itype = M32RBF_INSN_AND3; goto extract_sfmt_and3;
      case 141: itype = M32RBF_INSN_XOR3; goto extract_sfmt_and3;
      case 142: itype = M32RBF_INSN_OR3; goto extract_sfmt_or3;
      case 144:
        if ((entire_insn & 0xf0f0ffff) == 0x90000000)
          { itype = M32RBF_INSN_DIV; goto extract_sfmt_div; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 145:
        if ((entire_insn & 0xf0f0ffff) == 0x90100000)
          { itype = M32RBF_INSN_DIVU; goto extract_sfmt_div; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 146:
        if ((entire_insn & 0xf0f0ffff) == 0x90200000)
          { itype = M32RBF_INSN_REM; goto extract_sfmt_div; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 147:
        if ((entire_insn & 0xf0f0ffff) == 0x90300000)
          { itype = M32RBF_INSN_REMU; goto extract_sfmt_div; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 152: itype = M32RBF_INSN_SRL3; goto extract_sfmt_sll3;
      case 154: itype = M32RBF_INSN_SRA3; goto extract_sfmt_sll3;
      case 156: itype = M32RBF_INSN_SLL3; goto extract_sfmt_sll3;
      case 159:
        if ((entire_insn & 0xf0ff0000) == 0x90f00000)
          { itype = M32RBF_INSN_LDI16; goto extract_sfmt_ldi16; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 160: itype = M32RBF_INSN_STB_D; goto extract_sfmt_stb_d;
      case 162: itype = M32RBF_INSN_STH_D; goto extract_sfmt_sth_d;
      case 164: itype = M32RBF_INSN_ST_D; goto extract_sfmt_st_d;
      case 166:
        if ((entire_insn & 0xf8f00000) == 0xa0600000)
          { itype = M32RBF_INSN_BSET; goto extract_sfmt_bset; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 167:
        if ((entire_insn & 0xf8f00000) == 0xa0700000)
          { itype = M32RBF_INSN_BCLR; goto extract_sfmt_bset; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 168: itype = M32RBF_INSN_LDB_D; goto extract_sfmt_ldb_d;
      case 169: itype = M32RBF_INSN_LDUB_D; goto extract_sfmt_ldb_d;
      case 170: itype = M32RBF_INSN_LDH_D; goto extract_sfmt_ldh_d;
      case 171: itype = M32RBF_INSN_LDUH_D; goto extract_sfmt_ldh_d;
      case 172: itype = M32RBF_INSN_LD_D; goto extract_sfmt_ld_d;
      case 176: itype = M32RBF_INSN_BEQ; goto extract_sfmt_beq;
      case 177: itype = M32RBF_INSN_BNE; goto extract_sfmt_beq;
      case 184:
        if ((entire_insn & 0xfff00000) == 0xb0800000)
          { itype = M32RBF_INSN_BEQZ; goto extract_sfmt_beqz; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 185:
        if ((entire_insn & 0xfff00000) == 0xb0900000)
          { itype = M32RBF_INSN_BNEZ; goto extract_sfmt_beqz; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 186:
        if ((entire_insn & 0xfff00000) == 0xb0a00000)
          { itype = M32RBF_INSN_BLTZ; goto extract_sfmt_beqz; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 187:
        if ((entire_insn & 0xfff00000) == 0xb0b00000)
          { itype = M32RBF_INSN_BGEZ; goto extract_sfmt_beqz; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 188:
        if ((entire_insn & 0xfff00000) == 0xb0c00000)
          { itype = M32RBF_INSN_BLEZ; goto extract_sfmt_beqz; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 189:
        if ((entire_insn & 0xfff00000) == 0xb0d00000)
          { itype = M32RBF_INSN_BGTZ; goto extract_sfmt_beqz; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 220:
        if ((entire_insn & 0xf0ff0000) == 0xd0c00000)
          { itype = M32RBF_INSN_SETH; goto extract_sfmt_seth; }
        itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 224:
      case 225:
      case 226:
      case 227:
      case 228:
      case 229:
      case 230:
      case 231:
      case 232:
      case 233:
      case 234:
      case 235:
      case 236:
      case 237:
      case 238:
      case 239: itype = M32RBF_INSN_LD24; goto extract_sfmt_ld24;
      case 240:
      case 241:
      case 242:
      case 243:
      case 244:
      case 245:
      case 246:
      case 247:
      case 248:
      case 249:
      case 250:
      case 251:
      case 252:
      case 253:
      case 254:
      case 255:
        {
          unsigned int val1 = (((insn >> 8) & (3 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xff000000) == 0xfc000000)
              { itype = M32RBF_INSN_BC24; goto extract_sfmt_bc24; }
            itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xff000000) == 0xfd000000)
              { itype = M32RBF_INSN_BNC24; goto extract_sfmt_bc24; }
            itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 2:
            if ((entire_insn & 0xff000000) == 0xfe000000)
              { itype = M32RBF_INSN_BL24; goto extract_sfmt_bl24; }
            itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 3:
            if ((entire_insn & 0xff000000) == 0xff000000)
              { itype = M32RBF_INSN_BRA24; goto extract_sfmt_bra24; }
            itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      default: itype = M32RBF_INSN_X_INVALID; goto extract_sfmt_empty;
      }
    }
  }

  /* The instruction has been decoded, now extract the fields.  */

 extract_sfmt_empty:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_empty", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_add:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_add.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_add", "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_dr) = f_r1;
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_add3:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_add3.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);
    f_simm16 = EXTRACT_MSB0_SINT (insn, 32, 16, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_simm16) = f_simm16;
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_add3", "f_simm16 0x%x", 'x', f_simm16, "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_and3:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_and3.f
    UINT f_r1;
    UINT f_r2;
    UINT f_uimm16;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);
    f_uimm16 = EXTRACT_MSB0_UINT (insn, 32, 16, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_uimm16) = f_uimm16;
  FLD (f_r1) = f_r1;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_and3", "f_r2 0x%x", 'x', f_r2, "f_uimm16 0x%x", 'x', f_uimm16, "f_r1 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_or3:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_and3.f
    UINT f_r1;
    UINT f_r2;
    UINT f_uimm16;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);
    f_uimm16 = EXTRACT_MSB0_UINT (insn, 32, 16, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_uimm16) = f_uimm16;
  FLD (f_r1) = f_r1;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_or3", "f_r2 0x%x", 'x', f_r2, "f_uimm16 0x%x", 'x', f_uimm16, "f_r1 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addi:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_addi.f
    UINT f_r1;
    INT f_simm8;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_simm8 = EXTRACT_MSB0_SINT (insn, 16, 8, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (f_simm8) = f_simm8;
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addi", "f_r1 0x%x", 'x', f_r1, "f_simm8 0x%x", 'x', f_simm8, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_dr) = f_r1;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addv:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_add.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addv", "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_dr) = f_r1;
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addv3:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_add3.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);
    f_simm16 = EXTRACT_MSB0_SINT (insn, 32, 16, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_simm16) = f_simm16;
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addv3", "f_simm16 0x%x", 'x', f_simm16, "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addx:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_add.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addx", "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_dr) = f_r1;
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bc8:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_bl8.f
    SI f_disp8;

    f_disp8 = ((((EXTRACT_MSB0_SINT (insn, 16, 8, 8)) * (4))) + (((pc) & (-4))));

  /* Record the fields for the semantic handler.  */
  FLD (i_disp8) = f_disp8;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bc8", "disp8 0x%x", 'x', f_disp8, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bc24:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_bl24.f
    SI f_disp24;

    f_disp24 = ((((EXTRACT_MSB0_SINT (insn, 32, 8, 24)) * (4))) + (pc));

  /* Record the fields for the semantic handler.  */
  FLD (i_disp24) = f_disp24;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bc24", "disp24 0x%x", 'x', f_disp24, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_beq:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_beq.f
    UINT f_r1;
    UINT f_r2;
    SI f_disp16;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);
    f_disp16 = ((((EXTRACT_MSB0_SINT (insn, 32, 16, 16)) * (4))) + (pc));

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_disp16) = f_disp16;
  FLD (i_src1) = & CPU (h_gr)[f_r1];
  FLD (i_src2) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_beq", "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "disp16 0x%x", 'x', f_disp16, "src1 0x%x", 'x', f_r1, "src2 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_src1) = f_r1;
      FLD (in_src2) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_beqz:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_beq.f
    UINT f_r2;
    SI f_disp16;

    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);
    f_disp16 = ((((EXTRACT_MSB0_SINT (insn, 32, 16, 16)) * (4))) + (pc));

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (i_disp16) = f_disp16;
  FLD (i_src2) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_beqz", "f_r2 0x%x", 'x', f_r2, "disp16 0x%x", 'x', f_disp16, "src2 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_src2) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bl8:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_bl8.f
    SI f_disp8;

    f_disp8 = ((((EXTRACT_MSB0_SINT (insn, 16, 8, 8)) * (4))) + (((pc) & (-4))));

  /* Record the fields for the semantic handler.  */
  FLD (i_disp8) = f_disp8;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bl8", "disp8 0x%x", 'x', f_disp8, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_h_gr_SI_14) = 14;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bl24:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_bl24.f
    SI f_disp24;

    f_disp24 = ((((EXTRACT_MSB0_SINT (insn, 32, 8, 24)) * (4))) + (pc));

  /* Record the fields for the semantic handler.  */
  FLD (i_disp24) = f_disp24;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bl24", "disp24 0x%x", 'x', f_disp24, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_h_gr_SI_14) = 14;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bra8:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_bl8.f
    SI f_disp8;

    f_disp8 = ((((EXTRACT_MSB0_SINT (insn, 16, 8, 8)) * (4))) + (((pc) & (-4))));

  /* Record the fields for the semantic handler.  */
  FLD (i_disp8) = f_disp8;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bra8", "disp8 0x%x", 'x', f_disp8, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bra24:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_bl24.f
    SI f_disp24;

    f_disp24 = ((((EXTRACT_MSB0_SINT (insn, 32, 8, 24)) * (4))) + (pc));

  /* Record the fields for the semantic handler.  */
  FLD (i_disp24) = f_disp24;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bra24", "disp24 0x%x", 'x', f_disp24, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_cmp:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_st_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_src1) = & CPU (h_gr)[f_r1];
  FLD (i_src2) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmp", "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "src1 0x%x", 'x', f_r1, "src2 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_src1) = f_r1;
      FLD (in_src2) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_cmpi:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_st_d.f
    UINT f_r2;
    INT f_simm16;

    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);
    f_simm16 = EXTRACT_MSB0_SINT (insn, 32, 16, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_simm16) = f_simm16;
  FLD (f_r2) = f_r2;
  FLD (i_src2) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmpi", "f_simm16 0x%x", 'x', f_simm16, "f_r2 0x%x", 'x', f_r2, "src2 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_src2) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_div:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_add.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_div", "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_dr) = f_r1;
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_jl:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_jl.f
    UINT f_r2;

    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_jl", "f_r2 0x%x", 'x', f_r2, "sr 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
      FLD (out_h_gr_SI_14) = 14;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_jmp:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_jl.f
    UINT f_r2;

    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_jmp", "f_r2 0x%x", 'x', f_r2, "sr 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ld:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_ld_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ld", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ld_d:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_add3.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);
    f_simm16 = EXTRACT_MSB0_SINT (insn, 32, 16, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_simm16) = f_simm16;
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ld_d", "f_simm16 0x%x", 'x', f_simm16, "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldb:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_ld_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldb", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldb_d:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_add3.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);
    f_simm16 = EXTRACT_MSB0_SINT (insn, 32, 16, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_simm16) = f_simm16;
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldb_d", "f_simm16 0x%x", 'x', f_simm16, "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldh:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_ld_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldh", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldh_d:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_add3.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);
    f_simm16 = EXTRACT_MSB0_SINT (insn, 32, 16, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_simm16) = f_simm16;
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldh_d", "f_simm16 0x%x", 'x', f_simm16, "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ld_plus:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_ld_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ld_plus", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
      FLD (out_sr) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ld24:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_ld24.f
    UINT f_r1;
    UINT f_uimm24;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_uimm24 = EXTRACT_MSB0_UINT (insn, 32, 8, 24);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (i_uimm24) = f_uimm24;
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ld24", "f_r1 0x%x", 'x', f_r1, "uimm24 0x%x", 'x', f_uimm24, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldi8:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_addi.f
    UINT f_r1;
    INT f_simm8;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_simm8 = EXTRACT_MSB0_SINT (insn, 16, 8, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_simm8) = f_simm8;
  FLD (f_r1) = f_r1;
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldi8", "f_simm8 0x%x", 'x', f_simm8, "f_r1 0x%x", 'x', f_r1, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ldi16:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_add3.f
    UINT f_r1;
    INT f_simm16;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_simm16 = EXTRACT_MSB0_SINT (insn, 32, 16, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ldi16", "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_lock:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_ld_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lock", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_machi:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_st_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_src1) = & CPU (h_gr)[f_r1];
  FLD (i_src2) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_machi", "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "src1 0x%x", 'x', f_r1, "src2 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_src1) = f_r1;
      FLD (in_src2) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_mulhi:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_st_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_src1) = & CPU (h_gr)[f_r1];
  FLD (i_src2) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mulhi", "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "src1 0x%x", 'x', f_r1, "src2 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_src1) = f_r1;
      FLD (in_src2) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_mv:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_ld_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mv", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_mvfachi:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_seth.f
    UINT f_r1;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mvfachi", "f_r1 0x%x", 'x', f_r1, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_mvfc:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_ld_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mvfc", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_mvtachi:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_st_plus.f
    UINT f_r1;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (i_src1) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mvtachi", "f_r1 0x%x", 'x', f_r1, "src1 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_src1) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_mvtc:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_ld_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mvtc", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_nop:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_nop", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_rac:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_rac", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_rte:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_rte", (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_seth:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_seth.f
    UINT f_r1;
    UINT f_hi16;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_hi16 = EXTRACT_MSB0_UINT (insn, 32, 16, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_hi16) = f_hi16;
  FLD (f_r1) = f_r1;
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_seth", "f_hi16 0x%x", 'x', f_hi16, "f_r1 0x%x", 'x', f_r1, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_sll3:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_add3.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);
    f_simm16 = EXTRACT_MSB0_SINT (insn, 32, 16, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_simm16) = f_simm16;
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_sll3", "f_simm16 0x%x", 'x', f_simm16, "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, "sr 0x%x", 'x', f_r2, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_slli:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_slli.f
    UINT f_r1;
    UINT f_uimm5;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_uimm5 = EXTRACT_MSB0_UINT (insn, 16, 11, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (f_uimm5) = f_uimm5;
  FLD (i_dr) = & CPU (h_gr)[f_r1];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_slli", "f_r1 0x%x", 'x', f_r1, "f_uimm5 0x%x", 'x', f_uimm5, "dr 0x%x", 'x', f_r1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_dr) = f_r1;
      FLD (out_dr) = f_r1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_st:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_st_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_src1) = & CPU (h_gr)[f_r1];
  FLD (i_src2) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_st", "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "src1 0x%x", 'x', f_r1, "src2 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_src1) = f_r1;
      FLD (in_src2) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_st_d:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_st_d.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);
    f_simm16 = EXTRACT_MSB0_SINT (insn, 32, 16, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_src1) = & CPU (h_gr)[f_r1];
  FLD (i_src2) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_st_d", "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "src1 0x%x", 'x', f_r1, "src2 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_src1) = f_r1;
      FLD (in_src2) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_stb:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_st_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_src1) = & CPU (h_gr)[f_r1];
  FLD (i_src2) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stb", "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "src1 0x%x", 'x', f_r1, "src2 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_src1) = f_r1;
      FLD (in_src2) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_stb_d:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_st_d.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);
    f_simm16 = EXTRACT_MSB0_SINT (insn, 32, 16, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_src1) = & CPU (h_gr)[f_r1];
  FLD (i_src2) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_stb_d", "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "src1 0x%x", 'x', f_r1, "src2 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_src1) = f_r1;
      FLD (in_src2) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_sth:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_st_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_src1) = & CPU (h_gr)[f_r1];
  FLD (i_src2) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_sth", "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "src1 0x%x", 'x', f_r1, "src2 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_src1) = f_r1;
      FLD (in_src2) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_sth_d:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_st_d.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_MSB0_UINT (insn, 32, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);
    f_simm16 = EXTRACT_MSB0_SINT (insn, 32, 16, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_src1) = & CPU (h_gr)[f_r1];
  FLD (i_src2) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_sth_d", "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "src1 0x%x", 'x', f_r1, "src2 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_src1) = f_r1;
      FLD (in_src2) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_st_plus:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_st_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_src1) = & CPU (h_gr)[f_r1];
  FLD (i_src2) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_st_plus", "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "src1 0x%x", 'x', f_r1, "src2 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_src1) = f_r1;
      FLD (in_src2) = f_r2;
      FLD (out_src2) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_trap:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_trap.f
    UINT f_uimm4;

    f_uimm4 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_uimm4) = f_uimm4;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_trap", "f_uimm4 0x%x", 'x', f_uimm4, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_unlock:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_st_plus.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_MSB0_UINT (insn, 16, 4, 4);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (f_r2) = f_r2;
  FLD (i_src1) = & CPU (h_gr)[f_r1];
  FLD (i_src2) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_unlock", "f_r1 0x%x", 'x', f_r1, "f_r2 0x%x", 'x', f_r2, "src1 0x%x", 'x', f_r1, "src2 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_src1) = f_r1;
      FLD (in_src2) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_clrpsw:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_clrpsw.f
    UINT f_uimm8;

    f_uimm8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_uimm8) = f_uimm8;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_clrpsw", "f_uimm8 0x%x", 'x', f_uimm8, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_setpsw:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_clrpsw.f
    UINT f_uimm8;

    f_uimm8 = EXTRACT_MSB0_UINT (insn, 16, 8, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_uimm8) = f_uimm8;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_setpsw", "f_uimm8 0x%x", 'x', f_uimm8, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_bset:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_bset.f
    UINT f_uimm3;
    UINT f_r2;
    INT f_simm16;

    f_uimm3 = EXTRACT_MSB0_UINT (insn, 32, 5, 3);
    f_r2 = EXTRACT_MSB0_UINT (insn, 32, 12, 4);
    f_simm16 = EXTRACT_MSB0_SINT (insn, 32, 16, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_simm16) = f_simm16;
  FLD (f_r2) = f_r2;
  FLD (f_uimm3) = f_uimm3;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bset", "f_simm16 0x%x", 'x', f_simm16, "f_r2 0x%x", 'x', f_r2, "f_uimm3 0x%x", 'x', f_uimm3, "sr 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_btst:
  {
    const IDESC *idesc = &m32rbf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_bset.f
    UINT f_uimm3;
    UINT f_r2;

    f_uimm3 = EXTRACT_MSB0_UINT (insn, 16, 5, 3);
    f_r2 = EXTRACT_MSB0_UINT (insn, 16, 12, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_uimm3) = f_uimm3;
  FLD (i_sr) = & CPU (h_gr)[f_r2];
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_btst", "f_r2 0x%x", 'x', f_r2, "f_uimm3 0x%x", 'x', f_uimm3, "sr 0x%x", 'x', f_r2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_sr) = f_r2;
    }
#endif
#undef FLD
    return idesc;
  }

}
