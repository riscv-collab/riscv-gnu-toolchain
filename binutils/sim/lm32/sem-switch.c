/* Simulator instruction semantics for lm32bf.

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

#ifdef DEFINE_LABELS

  /* The labels have the case they have because the enum of insn types
     is all uppercase and in the non-stdc case the insn symbol is built
     into the enum name.  */

  static struct {
    int index;
    void *label;
  } labels[] = {
    { LM32BF_INSN_X_INVALID, && case_sem_INSN_X_INVALID },
    { LM32BF_INSN_X_AFTER, && case_sem_INSN_X_AFTER },
    { LM32BF_INSN_X_BEFORE, && case_sem_INSN_X_BEFORE },
    { LM32BF_INSN_X_CTI_CHAIN, && case_sem_INSN_X_CTI_CHAIN },
    { LM32BF_INSN_X_CHAIN, && case_sem_INSN_X_CHAIN },
    { LM32BF_INSN_X_BEGIN, && case_sem_INSN_X_BEGIN },
    { LM32BF_INSN_ADD, && case_sem_INSN_ADD },
    { LM32BF_INSN_ADDI, && case_sem_INSN_ADDI },
    { LM32BF_INSN_AND, && case_sem_INSN_AND },
    { LM32BF_INSN_ANDI, && case_sem_INSN_ANDI },
    { LM32BF_INSN_ANDHII, && case_sem_INSN_ANDHII },
    { LM32BF_INSN_B, && case_sem_INSN_B },
    { LM32BF_INSN_BI, && case_sem_INSN_BI },
    { LM32BF_INSN_BE, && case_sem_INSN_BE },
    { LM32BF_INSN_BG, && case_sem_INSN_BG },
    { LM32BF_INSN_BGE, && case_sem_INSN_BGE },
    { LM32BF_INSN_BGEU, && case_sem_INSN_BGEU },
    { LM32BF_INSN_BGU, && case_sem_INSN_BGU },
    { LM32BF_INSN_BNE, && case_sem_INSN_BNE },
    { LM32BF_INSN_CALL, && case_sem_INSN_CALL },
    { LM32BF_INSN_CALLI, && case_sem_INSN_CALLI },
    { LM32BF_INSN_CMPE, && case_sem_INSN_CMPE },
    { LM32BF_INSN_CMPEI, && case_sem_INSN_CMPEI },
    { LM32BF_INSN_CMPG, && case_sem_INSN_CMPG },
    { LM32BF_INSN_CMPGI, && case_sem_INSN_CMPGI },
    { LM32BF_INSN_CMPGE, && case_sem_INSN_CMPGE },
    { LM32BF_INSN_CMPGEI, && case_sem_INSN_CMPGEI },
    { LM32BF_INSN_CMPGEU, && case_sem_INSN_CMPGEU },
    { LM32BF_INSN_CMPGEUI, && case_sem_INSN_CMPGEUI },
    { LM32BF_INSN_CMPGU, && case_sem_INSN_CMPGU },
    { LM32BF_INSN_CMPGUI, && case_sem_INSN_CMPGUI },
    { LM32BF_INSN_CMPNE, && case_sem_INSN_CMPNE },
    { LM32BF_INSN_CMPNEI, && case_sem_INSN_CMPNEI },
    { LM32BF_INSN_DIVU, && case_sem_INSN_DIVU },
    { LM32BF_INSN_LB, && case_sem_INSN_LB },
    { LM32BF_INSN_LBU, && case_sem_INSN_LBU },
    { LM32BF_INSN_LH, && case_sem_INSN_LH },
    { LM32BF_INSN_LHU, && case_sem_INSN_LHU },
    { LM32BF_INSN_LW, && case_sem_INSN_LW },
    { LM32BF_INSN_MODU, && case_sem_INSN_MODU },
    { LM32BF_INSN_MUL, && case_sem_INSN_MUL },
    { LM32BF_INSN_MULI, && case_sem_INSN_MULI },
    { LM32BF_INSN_NOR, && case_sem_INSN_NOR },
    { LM32BF_INSN_NORI, && case_sem_INSN_NORI },
    { LM32BF_INSN_OR, && case_sem_INSN_OR },
    { LM32BF_INSN_ORI, && case_sem_INSN_ORI },
    { LM32BF_INSN_ORHII, && case_sem_INSN_ORHII },
    { LM32BF_INSN_RCSR, && case_sem_INSN_RCSR },
    { LM32BF_INSN_SB, && case_sem_INSN_SB },
    { LM32BF_INSN_SEXTB, && case_sem_INSN_SEXTB },
    { LM32BF_INSN_SEXTH, && case_sem_INSN_SEXTH },
    { LM32BF_INSN_SH, && case_sem_INSN_SH },
    { LM32BF_INSN_SL, && case_sem_INSN_SL },
    { LM32BF_INSN_SLI, && case_sem_INSN_SLI },
    { LM32BF_INSN_SR, && case_sem_INSN_SR },
    { LM32BF_INSN_SRI, && case_sem_INSN_SRI },
    { LM32BF_INSN_SRU, && case_sem_INSN_SRU },
    { LM32BF_INSN_SRUI, && case_sem_INSN_SRUI },
    { LM32BF_INSN_SUB, && case_sem_INSN_SUB },
    { LM32BF_INSN_SW, && case_sem_INSN_SW },
    { LM32BF_INSN_USER, && case_sem_INSN_USER },
    { LM32BF_INSN_WCSR, && case_sem_INSN_WCSR },
    { LM32BF_INSN_XOR, && case_sem_INSN_XOR },
    { LM32BF_INSN_XORI, && case_sem_INSN_XORI },
    { LM32BF_INSN_XNOR, && case_sem_INSN_XNOR },
    { LM32BF_INSN_XNORI, && case_sem_INSN_XNORI },
    { LM32BF_INSN_BREAK, && case_sem_INSN_BREAK },
    { LM32BF_INSN_SCALL, && case_sem_INSN_SCALL },
    { 0, 0 }
  };
  int i;

  for (i = 0; labels[i].label != 0; ++i)
    {
#if FAST_P
      CPU_IDESC (current_cpu) [labels[i].index].sem_fast_lab = labels[i].label;
#else
      CPU_IDESC (current_cpu) [labels[i].index].sem_full_lab = labels[i].label;
#endif
    }

#undef DEFINE_LABELS
#endif /* DEFINE_LABELS */

#ifdef DEFINE_SWITCH

/* If hyper-fast [well not unnecessarily slow] execution is selected, turn
   off frills like tracing and profiling.  */
/* FIXME: A better way would be to have CGEN_TRACE_RESULT check for something
   that can cause it to be optimized out.  Another way would be to emit
   special handlers into the instruction "stream".  */

#if FAST_P
#undef CGEN_TRACE_RESULT
#define CGEN_TRACE_RESULT(cpu, abuf, name, type, val)
#endif

#undef GET_ATTR
#define GET_ATTR(cpu, num, attr) CGEN_ATTR_VALUE (NULL, abuf->idesc->attrs, CGEN_INSN_##attr)

{

#if WITH_SCACHE_PBB

/* Branch to next handler without going around main loop.  */
#define NEXT(vpc) goto * SEM_ARGBUF (vpc) -> semantic.sem_case
SWITCH (sem, SEM_ARGBUF (vpc) -> semantic.sem_case)

#else /* ! WITH_SCACHE_PBB */

#define NEXT(vpc) BREAK (sem)
#ifdef __GNUC__
#if FAST_P
  SWITCH (sem, SEM_ARGBUF (sc) -> idesc->sem_fast_lab)
#else
  SWITCH (sem, SEM_ARGBUF (sc) -> idesc->sem_full_lab)
#endif
#else
  SWITCH (sem, SEM_ARGBUF (sc) -> idesc->num)
#endif

#endif /* ! WITH_SCACHE_PBB */

    {

  CASE (sem, INSN_X_INVALID) : /* --invalid-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
    /* Update the recorded pc in the cpu state struct.
       Only necessary for WITH_SCACHE case, but to avoid the
       conditional compilation ....  */
    SET_H_PC (pc);
    /* Virtual insns have zero size.  Overwrite vpc with address of next insn
       using the default-insn-bitsize spec.  When executing insns in parallel
       we may want to queue the fault and continue execution.  */
    vpc = SEM_NEXT_VPC (sem_arg, pc, 4);
    vpc = sim_engine_invalid_insn (current_cpu, pc, vpc);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_X_AFTER) : /* --after-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_LM32BF
    lm32bf_pbb_after (current_cpu, sem_arg);
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_X_BEFORE) : /* --before-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_LM32BF
    lm32bf_pbb_before (current_cpu, sem_arg);
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_X_CTI_CHAIN) : /* --cti-chain-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_LM32BF
#ifdef DEFINE_SWITCH
    vpc = lm32bf_pbb_cti_chain (current_cpu, sem_arg,
			       pbb_br_type, pbb_br_npc);
    BREAK (sem);
#else
    /* FIXME: Allow provision of explicit ifmt spec in insn spec.  */
    vpc = lm32bf_pbb_cti_chain (current_cpu, sem_arg,
			       CPU_PBB_BR_TYPE (current_cpu),
			       CPU_PBB_BR_NPC (current_cpu));
#endif
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_X_CHAIN) : /* --chain-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_LM32BF
    vpc = lm32bf_pbb_chain (current_cpu, sem_arg);
#ifdef DEFINE_SWITCH
    BREAK (sem);
#endif
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_X_BEGIN) : /* --begin-- */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_LM32BF
#if defined DEFINE_SWITCH || defined FAST_P
    /* In the switch case FAST_P is a constant, allowing several optimizations
       in any called inline functions.  */
    vpc = lm32bf_pbb_begin (current_cpu, FAST_P);
#else
#if 0 /* cgen engine can't handle dynamic fast/full switching yet.  */
    vpc = lm32bf_pbb_begin (current_cpu, STATE_RUN_FAST_P (CPU_STATE (current_cpu)));
#else
    vpc = lm32bf_pbb_begin (current_cpu, 0);
#endif
#endif
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ADD) : /* add $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ADDI) : /* addi $r1,$r0,$imm */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_AND) : /* and $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ANDI) : /* andi $r1,$r0,$uimm */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_andi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (CPU (h_gr[FLD (f_r0)]), ZEXTSISI (FLD (f_uimm)));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ANDHII) : /* andhi $r1,$r0,$hi16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_andi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (CPU (h_gr[FLD (f_r0)]), SLLSI (FLD (f_uimm), 16));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_B) : /* b $r0 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_be.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = lm32bf_b_insn (current_cpu, CPU (h_gr[FLD (f_r0)]), FLD (f_r0));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BI) : /* bi $call */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_bi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = EXTSISI (FLD (i_call));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BE) : /* be $r0,$r1,$branch */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_be.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (EQSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]))) {
  {
    USI opval = FLD (i_branch);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BG) : /* bg $r0,$r1,$branch */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_be.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GTSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]))) {
  {
    USI opval = FLD (i_branch);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BGE) : /* bge $r0,$r1,$branch */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_be.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GESI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]))) {
  {
    USI opval = FLD (i_branch);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BGEU) : /* bgeu $r0,$r1,$branch */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_be.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GEUSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]))) {
  {
    USI opval = FLD (i_branch);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BGU) : /* bgu $r0,$r1,$branch */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_be.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GTUSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]))) {
  {
    USI opval = FLD (i_branch);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BNE) : /* bne $r0,$r1,$branch */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_be.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]))) {
  {
    USI opval = FLD (i_branch);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 3);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CALL) : /* call $r0 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_be.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  {
    SI opval = ADDSI (pc, 4);
    CPU (h_gr[((UINT) 29)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }
  {
    USI opval = CPU (h_gr[FLD (f_r0)]);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CALLI) : /* calli $call */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_bi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  {
    SI opval = ADDSI (pc, 4);
    CPU (h_gr[((UINT) 29)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }
  {
    USI opval = EXTSISI (FLD (i_call));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPE) : /* cmpe $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EQSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPEI) : /* cmpei $r1,$r0,$imm */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EQSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPG) : /* cmpg $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GTSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPGI) : /* cmpgi $r1,$r0,$imm */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GTSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPGE) : /* cmpge $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GESI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPGEI) : /* cmpgei $r1,$r0,$imm */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GESI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPGEU) : /* cmpgeu $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GEUSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPGEUI) : /* cmpgeui $r1,$r0,$uimm */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_andi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GEUSI (CPU (h_gr[FLD (f_r0)]), ZEXTSISI (FLD (f_uimm)));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPGU) : /* cmpgu $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GTUSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPGUI) : /* cmpgui $r1,$r0,$uimm */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_andi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GTUSI (CPU (h_gr[FLD (f_r0)]), ZEXTSISI (FLD (f_uimm)));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPNE) : /* cmpne $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_CMPNEI) : /* cmpnei $r1,$r0,$imm */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_DIVU) : /* divu $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = lm32bf_divu_insn (current_cpu, pc, FLD (f_r0), FLD (f_r1), FLD (f_r2));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LB) : /* lb $r1,($r0+$imm) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTQISI (GETMEMQI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LBU) : /* lbu $r1,($r0+$imm) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ZEXTQISI (GETMEMQI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LH) : /* lh $r1,($r0+$imm) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTHISI (GETMEMHI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LHU) : /* lhu $r1,($r0+$imm) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ZEXTHISI (GETMEMHI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LW) : /* lw $r1,($r0+$imm) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm)))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MODU) : /* modu $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = lm32bf_modu_insn (current_cpu, pc, FLD (f_r0), FLD (f_r1), FLD (f_r2));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MUL) : /* mul $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MULSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_MULI) : /* muli $r1,$r0,$imm */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MULSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NOR) : /* nor $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (ORSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)])));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_NORI) : /* nori $r1,$r0,$uimm */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_andi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (ORSI (CPU (h_gr[FLD (f_r0)]), ZEXTSISI (FLD (f_uimm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_OR) : /* or $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ORI) : /* ori $r1,$r0,$lo16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_andi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (CPU (h_gr[FLD (f_r0)]), ZEXTSISI (FLD (f_uimm)));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_ORHII) : /* orhi $r1,$r0,$hi16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_andi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (CPU (h_gr[FLD (f_r0)]), SLLSI (FLD (f_uimm), 16));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_RCSR) : /* rcsr $r2,$csr */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_rcsr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = CPU (h_csr[FLD (f_csr)]);
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SB) : /* sb ($r0+$imm),$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    QI opval = CPU (h_gr[FLD (f_r1)]);
    SETMEMQI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm)))), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SEXTB) : /* sextb $r2,$r0 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTQISI (TRUNCSIQI (CPU (h_gr[FLD (f_r0)])));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SEXTH) : /* sexth $r2,$r0 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTHISI (TRUNCSIHI (CPU (h_gr[FLD (f_r0)])));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SH) : /* sh ($r0+$imm),$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    HI opval = CPU (h_gr[FLD (f_r1)]);
    SETMEMHI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm)))), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SL) : /* sl $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SLLSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SLI) : /* sli $r1,$r0,$imm */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SLLSI (CPU (h_gr[FLD (f_r0)]), FLD (f_imm));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SR) : /* sr $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SRASI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SRI) : /* sri $r1,$r0,$imm */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SRASI (CPU (h_gr[FLD (f_r0)]), FLD (f_imm));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SRU) : /* sru $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SRLSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SRUI) : /* srui $r1,$r0,$imm */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SRLSI (CPU (h_gr[FLD (f_r0)]), FLD (f_imm));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SUB) : /* sub $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SUBSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SW) : /* sw ($r0+$imm),$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_addi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = CPU (h_gr[FLD (f_r1)]);
    SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm)))), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_USER) : /* user $r2,$r0,$r1,$user */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = lm32bf_user_insn (current_cpu, CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]), FLD (f_user));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_WCSR) : /* wcsr $csr,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_wcsr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

lm32bf_wcsr_insn (current_cpu, FLD (f_csr), CPU (h_gr[FLD (f_r1)]));

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_XOR) : /* xor $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_XORI) : /* xori $r1,$r0,$uimm */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_andi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (CPU (h_gr[FLD (f_r0)]), ZEXTSISI (FLD (f_uimm)));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_XNOR) : /* xnor $r2,$r0,$r1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_user.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (XORSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)])));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_XNORI) : /* xnori $r1,$r0,$uimm */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_andi.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (XORSI (CPU (h_gr[FLD (f_r0)]), ZEXTSISI (FLD (f_uimm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_BREAK) : /* break */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = lm32bf_break_insn (current_cpu, pc);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_SCALL) : /* scall */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = lm32bf_scall_insn (current_cpu, pc);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);


    }
  ENDSWITCH (sem) /* End of semantic switch.  */

  /* At this point `vpc' contains the next insn to execute.  */
}

#undef DEFINE_SWITCH
#endif /* DEFINE_SWITCH */
