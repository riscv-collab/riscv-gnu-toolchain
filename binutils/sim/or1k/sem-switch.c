/* Simulator instruction semantics for or1k32bf.

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
    { OR1K32BF_INSN_X_INVALID, && case_sem_INSN_X_INVALID },
    { OR1K32BF_INSN_X_AFTER, && case_sem_INSN_X_AFTER },
    { OR1K32BF_INSN_X_BEFORE, && case_sem_INSN_X_BEFORE },
    { OR1K32BF_INSN_X_CTI_CHAIN, && case_sem_INSN_X_CTI_CHAIN },
    { OR1K32BF_INSN_X_CHAIN, && case_sem_INSN_X_CHAIN },
    { OR1K32BF_INSN_X_BEGIN, && case_sem_INSN_X_BEGIN },
    { OR1K32BF_INSN_L_J, && case_sem_INSN_L_J },
    { OR1K32BF_INSN_L_ADRP, && case_sem_INSN_L_ADRP },
    { OR1K32BF_INSN_L_JAL, && case_sem_INSN_L_JAL },
    { OR1K32BF_INSN_L_JR, && case_sem_INSN_L_JR },
    { OR1K32BF_INSN_L_JALR, && case_sem_INSN_L_JALR },
    { OR1K32BF_INSN_L_BNF, && case_sem_INSN_L_BNF },
    { OR1K32BF_INSN_L_BF, && case_sem_INSN_L_BF },
    { OR1K32BF_INSN_L_TRAP, && case_sem_INSN_L_TRAP },
    { OR1K32BF_INSN_L_SYS, && case_sem_INSN_L_SYS },
    { OR1K32BF_INSN_L_MSYNC, && case_sem_INSN_L_MSYNC },
    { OR1K32BF_INSN_L_PSYNC, && case_sem_INSN_L_PSYNC },
    { OR1K32BF_INSN_L_CSYNC, && case_sem_INSN_L_CSYNC },
    { OR1K32BF_INSN_L_RFE, && case_sem_INSN_L_RFE },
    { OR1K32BF_INSN_L_NOP_IMM, && case_sem_INSN_L_NOP_IMM },
    { OR1K32BF_INSN_L_MOVHI, && case_sem_INSN_L_MOVHI },
    { OR1K32BF_INSN_L_MACRC, && case_sem_INSN_L_MACRC },
    { OR1K32BF_INSN_L_MFSPR, && case_sem_INSN_L_MFSPR },
    { OR1K32BF_INSN_L_MTSPR, && case_sem_INSN_L_MTSPR },
    { OR1K32BF_INSN_L_LWZ, && case_sem_INSN_L_LWZ },
    { OR1K32BF_INSN_L_LWS, && case_sem_INSN_L_LWS },
    { OR1K32BF_INSN_L_LWA, && case_sem_INSN_L_LWA },
    { OR1K32BF_INSN_L_LBZ, && case_sem_INSN_L_LBZ },
    { OR1K32BF_INSN_L_LBS, && case_sem_INSN_L_LBS },
    { OR1K32BF_INSN_L_LHZ, && case_sem_INSN_L_LHZ },
    { OR1K32BF_INSN_L_LHS, && case_sem_INSN_L_LHS },
    { OR1K32BF_INSN_L_SW, && case_sem_INSN_L_SW },
    { OR1K32BF_INSN_L_SB, && case_sem_INSN_L_SB },
    { OR1K32BF_INSN_L_SH, && case_sem_INSN_L_SH },
    { OR1K32BF_INSN_L_SWA, && case_sem_INSN_L_SWA },
    { OR1K32BF_INSN_L_SLL, && case_sem_INSN_L_SLL },
    { OR1K32BF_INSN_L_SLLI, && case_sem_INSN_L_SLLI },
    { OR1K32BF_INSN_L_SRL, && case_sem_INSN_L_SRL },
    { OR1K32BF_INSN_L_SRLI, && case_sem_INSN_L_SRLI },
    { OR1K32BF_INSN_L_SRA, && case_sem_INSN_L_SRA },
    { OR1K32BF_INSN_L_SRAI, && case_sem_INSN_L_SRAI },
    { OR1K32BF_INSN_L_ROR, && case_sem_INSN_L_ROR },
    { OR1K32BF_INSN_L_RORI, && case_sem_INSN_L_RORI },
    { OR1K32BF_INSN_L_AND, && case_sem_INSN_L_AND },
    { OR1K32BF_INSN_L_OR, && case_sem_INSN_L_OR },
    { OR1K32BF_INSN_L_XOR, && case_sem_INSN_L_XOR },
    { OR1K32BF_INSN_L_ADD, && case_sem_INSN_L_ADD },
    { OR1K32BF_INSN_L_SUB, && case_sem_INSN_L_SUB },
    { OR1K32BF_INSN_L_ADDC, && case_sem_INSN_L_ADDC },
    { OR1K32BF_INSN_L_MUL, && case_sem_INSN_L_MUL },
    { OR1K32BF_INSN_L_MULD, && case_sem_INSN_L_MULD },
    { OR1K32BF_INSN_L_MULU, && case_sem_INSN_L_MULU },
    { OR1K32BF_INSN_L_MULDU, && case_sem_INSN_L_MULDU },
    { OR1K32BF_INSN_L_DIV, && case_sem_INSN_L_DIV },
    { OR1K32BF_INSN_L_DIVU, && case_sem_INSN_L_DIVU },
    { OR1K32BF_INSN_L_FF1, && case_sem_INSN_L_FF1 },
    { OR1K32BF_INSN_L_FL1, && case_sem_INSN_L_FL1 },
    { OR1K32BF_INSN_L_ANDI, && case_sem_INSN_L_ANDI },
    { OR1K32BF_INSN_L_ORI, && case_sem_INSN_L_ORI },
    { OR1K32BF_INSN_L_XORI, && case_sem_INSN_L_XORI },
    { OR1K32BF_INSN_L_ADDI, && case_sem_INSN_L_ADDI },
    { OR1K32BF_INSN_L_ADDIC, && case_sem_INSN_L_ADDIC },
    { OR1K32BF_INSN_L_MULI, && case_sem_INSN_L_MULI },
    { OR1K32BF_INSN_L_EXTHS, && case_sem_INSN_L_EXTHS },
    { OR1K32BF_INSN_L_EXTBS, && case_sem_INSN_L_EXTBS },
    { OR1K32BF_INSN_L_EXTHZ, && case_sem_INSN_L_EXTHZ },
    { OR1K32BF_INSN_L_EXTBZ, && case_sem_INSN_L_EXTBZ },
    { OR1K32BF_INSN_L_EXTWS, && case_sem_INSN_L_EXTWS },
    { OR1K32BF_INSN_L_EXTWZ, && case_sem_INSN_L_EXTWZ },
    { OR1K32BF_INSN_L_CMOV, && case_sem_INSN_L_CMOV },
    { OR1K32BF_INSN_L_SFGTS, && case_sem_INSN_L_SFGTS },
    { OR1K32BF_INSN_L_SFGTSI, && case_sem_INSN_L_SFGTSI },
    { OR1K32BF_INSN_L_SFGTU, && case_sem_INSN_L_SFGTU },
    { OR1K32BF_INSN_L_SFGTUI, && case_sem_INSN_L_SFGTUI },
    { OR1K32BF_INSN_L_SFGES, && case_sem_INSN_L_SFGES },
    { OR1K32BF_INSN_L_SFGESI, && case_sem_INSN_L_SFGESI },
    { OR1K32BF_INSN_L_SFGEU, && case_sem_INSN_L_SFGEU },
    { OR1K32BF_INSN_L_SFGEUI, && case_sem_INSN_L_SFGEUI },
    { OR1K32BF_INSN_L_SFLTS, && case_sem_INSN_L_SFLTS },
    { OR1K32BF_INSN_L_SFLTSI, && case_sem_INSN_L_SFLTSI },
    { OR1K32BF_INSN_L_SFLTU, && case_sem_INSN_L_SFLTU },
    { OR1K32BF_INSN_L_SFLTUI, && case_sem_INSN_L_SFLTUI },
    { OR1K32BF_INSN_L_SFLES, && case_sem_INSN_L_SFLES },
    { OR1K32BF_INSN_L_SFLESI, && case_sem_INSN_L_SFLESI },
    { OR1K32BF_INSN_L_SFLEU, && case_sem_INSN_L_SFLEU },
    { OR1K32BF_INSN_L_SFLEUI, && case_sem_INSN_L_SFLEUI },
    { OR1K32BF_INSN_L_SFEQ, && case_sem_INSN_L_SFEQ },
    { OR1K32BF_INSN_L_SFEQI, && case_sem_INSN_L_SFEQI },
    { OR1K32BF_INSN_L_SFNE, && case_sem_INSN_L_SFNE },
    { OR1K32BF_INSN_L_SFNEI, && case_sem_INSN_L_SFNEI },
    { OR1K32BF_INSN_L_MAC, && case_sem_INSN_L_MAC },
    { OR1K32BF_INSN_L_MACI, && case_sem_INSN_L_MACI },
    { OR1K32BF_INSN_L_MACU, && case_sem_INSN_L_MACU },
    { OR1K32BF_INSN_L_MSB, && case_sem_INSN_L_MSB },
    { OR1K32BF_INSN_L_MSBU, && case_sem_INSN_L_MSBU },
    { OR1K32BF_INSN_L_CUST1, && case_sem_INSN_L_CUST1 },
    { OR1K32BF_INSN_L_CUST2, && case_sem_INSN_L_CUST2 },
    { OR1K32BF_INSN_L_CUST3, && case_sem_INSN_L_CUST3 },
    { OR1K32BF_INSN_L_CUST4, && case_sem_INSN_L_CUST4 },
    { OR1K32BF_INSN_L_CUST5, && case_sem_INSN_L_CUST5 },
    { OR1K32BF_INSN_L_CUST6, && case_sem_INSN_L_CUST6 },
    { OR1K32BF_INSN_L_CUST7, && case_sem_INSN_L_CUST7 },
    { OR1K32BF_INSN_L_CUST8, && case_sem_INSN_L_CUST8 },
    { OR1K32BF_INSN_LF_ADD_S, && case_sem_INSN_LF_ADD_S },
    { OR1K32BF_INSN_LF_ADD_D32, && case_sem_INSN_LF_ADD_D32 },
    { OR1K32BF_INSN_LF_SUB_S, && case_sem_INSN_LF_SUB_S },
    { OR1K32BF_INSN_LF_SUB_D32, && case_sem_INSN_LF_SUB_D32 },
    { OR1K32BF_INSN_LF_MUL_S, && case_sem_INSN_LF_MUL_S },
    { OR1K32BF_INSN_LF_MUL_D32, && case_sem_INSN_LF_MUL_D32 },
    { OR1K32BF_INSN_LF_DIV_S, && case_sem_INSN_LF_DIV_S },
    { OR1K32BF_INSN_LF_DIV_D32, && case_sem_INSN_LF_DIV_D32 },
    { OR1K32BF_INSN_LF_REM_S, && case_sem_INSN_LF_REM_S },
    { OR1K32BF_INSN_LF_REM_D32, && case_sem_INSN_LF_REM_D32 },
    { OR1K32BF_INSN_LF_ITOF_S, && case_sem_INSN_LF_ITOF_S },
    { OR1K32BF_INSN_LF_ITOF_D32, && case_sem_INSN_LF_ITOF_D32 },
    { OR1K32BF_INSN_LF_FTOI_S, && case_sem_INSN_LF_FTOI_S },
    { OR1K32BF_INSN_LF_FTOI_D32, && case_sem_INSN_LF_FTOI_D32 },
    { OR1K32BF_INSN_LF_SFEQ_S, && case_sem_INSN_LF_SFEQ_S },
    { OR1K32BF_INSN_LF_SFEQ_D32, && case_sem_INSN_LF_SFEQ_D32 },
    { OR1K32BF_INSN_LF_SFNE_S, && case_sem_INSN_LF_SFNE_S },
    { OR1K32BF_INSN_LF_SFNE_D32, && case_sem_INSN_LF_SFNE_D32 },
    { OR1K32BF_INSN_LF_SFGE_S, && case_sem_INSN_LF_SFGE_S },
    { OR1K32BF_INSN_LF_SFGE_D32, && case_sem_INSN_LF_SFGE_D32 },
    { OR1K32BF_INSN_LF_SFGT_S, && case_sem_INSN_LF_SFGT_S },
    { OR1K32BF_INSN_LF_SFGT_D32, && case_sem_INSN_LF_SFGT_D32 },
    { OR1K32BF_INSN_LF_SFLT_S, && case_sem_INSN_LF_SFLT_S },
    { OR1K32BF_INSN_LF_SFLT_D32, && case_sem_INSN_LF_SFLT_D32 },
    { OR1K32BF_INSN_LF_SFLE_S, && case_sem_INSN_LF_SFLE_S },
    { OR1K32BF_INSN_LF_SFLE_D32, && case_sem_INSN_LF_SFLE_D32 },
    { OR1K32BF_INSN_LF_SFUEQ_S, && case_sem_INSN_LF_SFUEQ_S },
    { OR1K32BF_INSN_LF_SFUEQ_D32, && case_sem_INSN_LF_SFUEQ_D32 },
    { OR1K32BF_INSN_LF_SFUNE_S, && case_sem_INSN_LF_SFUNE_S },
    { OR1K32BF_INSN_LF_SFUNE_D32, && case_sem_INSN_LF_SFUNE_D32 },
    { OR1K32BF_INSN_LF_SFUGT_S, && case_sem_INSN_LF_SFUGT_S },
    { OR1K32BF_INSN_LF_SFUGT_D32, && case_sem_INSN_LF_SFUGT_D32 },
    { OR1K32BF_INSN_LF_SFUGE_S, && case_sem_INSN_LF_SFUGE_S },
    { OR1K32BF_INSN_LF_SFUGE_D32, && case_sem_INSN_LF_SFUGE_D32 },
    { OR1K32BF_INSN_LF_SFULT_S, && case_sem_INSN_LF_SFULT_S },
    { OR1K32BF_INSN_LF_SFULT_D32, && case_sem_INSN_LF_SFULT_D32 },
    { OR1K32BF_INSN_LF_SFULE_S, && case_sem_INSN_LF_SFULE_S },
    { OR1K32BF_INSN_LF_SFULE_D32, && case_sem_INSN_LF_SFULE_D32 },
    { OR1K32BF_INSN_LF_SFUN_S, && case_sem_INSN_LF_SFUN_S },
    { OR1K32BF_INSN_LF_SFUN_D32, && case_sem_INSN_LF_SFUN_D32 },
    { OR1K32BF_INSN_LF_MADD_S, && case_sem_INSN_LF_MADD_S },
    { OR1K32BF_INSN_LF_MADD_D32, && case_sem_INSN_LF_MADD_D32 },
    { OR1K32BF_INSN_LF_CUST1_S, && case_sem_INSN_LF_CUST1_S },
    { OR1K32BF_INSN_LF_CUST1_D32, && case_sem_INSN_LF_CUST1_D32 },
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
#if WITH_SCACHE_PBB_OR1K32BF
    or1k32bf_pbb_after (current_cpu, sem_arg);
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
#if WITH_SCACHE_PBB_OR1K32BF
    or1k32bf_pbb_before (current_cpu, sem_arg);
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
#if WITH_SCACHE_PBB_OR1K32BF
#ifdef DEFINE_SWITCH
    vpc = or1k32bf_pbb_cti_chain (current_cpu, sem_arg,
			       pbb_br_type, pbb_br_npc);
    BREAK (sem);
#else
    /* FIXME: Allow provision of explicit ifmt spec in insn spec.  */
    vpc = or1k32bf_pbb_cti_chain (current_cpu, sem_arg,
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
#if WITH_SCACHE_PBB_OR1K32BF
    vpc = or1k32bf_pbb_chain (current_cpu, sem_arg);
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
#if WITH_SCACHE_PBB_OR1K32BF
#if defined DEFINE_SWITCH || defined FAST_P
    /* In the switch case FAST_P is a constant, allowing several optimizations
       in any called inline functions.  */
    vpc = or1k32bf_pbb_begin (current_cpu, FAST_P);
#else
#if 0 /* cgen engine can't handle dynamic fast/full switching yet.  */
    vpc = or1k32bf_pbb_begin (current_cpu, STATE_RUN_FAST_P (CPU_STATE (current_cpu)));
#else
    vpc = or1k32bf_pbb_begin (current_cpu, 0);
#endif
#endif
#endif
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_J) : /* l.j ${disp26} */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_j.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
{
  {
    USI opval = FLD (i_disp26);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
if (GET_H_SYS_CPUCFGR_ND ()) {
if (1)
  SEM_SKIP_INSN (current_cpu, sem_arg, vpc);
}
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_ADRP) : /* l.adrp $rD,${disp21} */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_adrp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = FLD (i_disp21);
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_JAL) : /* l.jal ${disp26} */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_j.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  {
    USI opval = ADDSI (pc, ((GET_H_SYS_CPUCFGR_ND ()) ? (4) : (8)));
    SET_H_GPR (((UINT) 9), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
{
{
  {
    USI opval = FLD (i_disp26);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
if (GET_H_SYS_CPUCFGR_ND ()) {
if (1)
  SEM_SKIP_INSN (current_cpu, sem_arg, vpc);
}
}
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_JR) : /* l.jr $rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
{
  {
    USI opval = GET_H_GPR (FLD (f_r3));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
if (GET_H_SYS_CPUCFGR_ND ()) {
if (1)
  SEM_SKIP_INSN (current_cpu, sem_arg, vpc);
}
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_JALR) : /* l.jalr $rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  {
    USI opval = ADDSI (pc, ((GET_H_SYS_CPUCFGR_ND ()) ? (4) : (8)));
    SET_H_GPR (((UINT) 9), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
{
{
  {
    USI opval = GET_H_GPR (FLD (f_r3));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
if (GET_H_SYS_CPUCFGR_ND ()) {
if (1)
  SEM_SKIP_INSN (current_cpu, sem_arg, vpc);
}
}
}

  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_BNF) : /* l.bnf ${disp26} */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_j.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
if (NOTSI (GET_H_SYS_SR_F ())) {
{
  {
    USI opval = FLD (i_disp26);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
} else {
if (GET_H_SYS_CPUCFGR_ND ()) {
{
  {
    USI opval = ADDSI (pc, 4);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}
}
if (GET_H_SYS_CPUCFGR_ND ()) {
if (1)
  SEM_SKIP_INSN (current_cpu, sem_arg, vpc);
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_BF) : /* l.bf ${disp26} */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_j.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
if (GET_H_SYS_SR_F ()) {
{
  {
    USI opval = FLD (i_disp26);
    SEM_BRANCH_VIA_CACHE (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
} else {
if (GET_H_SYS_CPUCFGR_ND ()) {
{
  {
    USI opval = ADDSI (pc, 4);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }
}
}
}
if (GET_H_SYS_CPUCFGR_ND ()) {
if (1)
  SEM_SKIP_INSN (current_cpu, sem_arg, vpc);
}
}

  abuf->written = written;
  SEM_BRANCH_FINI (vpc);
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_TRAP) : /* l.trap ${uimm16} */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

or1k32bf_exception (current_cpu, pc, EXCEPT_TRAP);

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SYS) : /* l.sys ${uimm16} */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

or1k32bf_exception (current_cpu, pc, EXCEPT_SYSCALL);

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_MSYNC) : /* l.msync */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_PSYNC) : /* l.psync */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_CSYNC) : /* l.csync */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_RFE) : /* l.rfe */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

or1k32bf_rfe (current_cpu);

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_NOP_IMM) : /* l.nop ${uimm16} */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

or1k32bf_nop (current_cpu, ZEXTSISI (FLD (f_uimm16)));

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_MOVHI) : /* l.movhi $rD,$uimm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = SLLSI (ZEXTSISI (FLD (f_uimm16)), 16);
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_MACRC) : /* l.macrc $rD */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_adrp.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  {
    USI opval = GET_H_MAC_MACLO ();
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
  {
    USI opval = 0;
    SET_H_MAC_MACLO (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-maclo", 'x', opval);
  }
  {
    USI opval = 0;
    SET_H_MAC_MACHI (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-machi", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_MFSPR) : /* l.mfspr $rD,$rA,${uimm16} */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = or1k32bf_mfspr (current_cpu, ORSI (GET_H_GPR (FLD (f_r2)), ZEXTSISI (FLD (f_uimm16))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_MTSPR) : /* l.mtspr $rA,$rB,${uimm16-split} */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_mtspr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

or1k32bf_mtspr (current_cpu, ORSI (GET_H_GPR (FLD (f_r2)), ZEXTSISI (FLD (f_uimm16_split))), GET_H_GPR (FLD (f_r3)));

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_LWZ) : /* l.lwz $rD,${simm16}($rA) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ZEXTSISI (GETMEMUSI (current_cpu, pc, or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 4)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_LWS) : /* l.lws $rD,${simm16}($rA) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTSISI (GETMEMSI (current_cpu, pc, or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 4)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_LWA) : /* l.lwa $rD,${simm16}($rA) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  {
    USI opval = ZEXTSISI (GETMEMUSI (current_cpu, pc, or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 4)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
  {
    BI opval = 1;
    CPU (h_atomic_reserve) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "atomic-reserve", 'x', opval);
  }
  {
    SI opval = or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 4);
    CPU (h_atomic_address) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "atomic-address", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_LBZ) : /* l.lbz $rD,${simm16}($rA) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ZEXTQISI (GETMEMUQI (current_cpu, pc, or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 1)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_LBS) : /* l.lbs $rD,${simm16}($rA) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTQISI (GETMEMQI (current_cpu, pc, or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 1)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_LHZ) : /* l.lhz $rD,${simm16}($rA) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ZEXTHISI (GETMEMUHI (current_cpu, pc, or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 2)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_LHS) : /* l.lhs $rD,${simm16}($rA) */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTHISI (GETMEMHI (current_cpu, pc, or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 2)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SW) : /* l.sw ${simm16-split}($rA),$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sw.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_addr;
  tmp_addr = or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16_split)), 4);
  {
    USI opval = TRUNCSISI (GET_H_GPR (FLD (f_r3)));
    SETMEMUSI (current_cpu, pc, tmp_addr, opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
if (EQSI (ANDSI (tmp_addr, 268435452), CPU (h_atomic_address))) {
  {
    BI opval = 0;
    CPU (h_atomic_reserve) = opval;
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "atomic-reserve", 'x', opval);
  }
}
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SB) : /* l.sb ${simm16-split}($rA),$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sw.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_addr;
  tmp_addr = or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16_split)), 1);
  {
    UQI opval = TRUNCSIQI (GET_H_GPR (FLD (f_r3)));
    SETMEMUQI (current_cpu, pc, tmp_addr, opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
if (EQSI (ANDSI (tmp_addr, 268435452), CPU (h_atomic_address))) {
  {
    BI opval = 0;
    CPU (h_atomic_reserve) = opval;
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "atomic-reserve", 'x', opval);
  }
}
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SH) : /* l.sh ${simm16-split}($rA),$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sw.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_addr;
  tmp_addr = or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16_split)), 2);
  {
    UHI opval = TRUNCSIHI (GET_H_GPR (FLD (f_r3)));
    SETMEMUHI (current_cpu, pc, tmp_addr, opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
if (EQSI (ANDSI (tmp_addr, 268435452), CPU (h_atomic_address))) {
  {
    BI opval = 0;
    CPU (h_atomic_reserve) = opval;
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "atomic-reserve", 'x', opval);
  }
}
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SWA) : /* l.swa ${simm16-split}($rA),$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sw.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  SI tmp_addr;
  tmp_addr = or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16_split)), 4);
  {
    USI opval = ANDBI (CPU (h_atomic_reserve), EQSI (tmp_addr, CPU (h_atomic_address)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }
if (GET_H_SYS_SR_F ()) {
  {
    USI opval = TRUNCSISI (GET_H_GPR (FLD (f_r3)));
    SETMEMUSI (current_cpu, pc, tmp_addr, opval);
    written |= (1 << 7);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }
}
  {
    BI opval = 0;
    CPU (h_atomic_reserve) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "atomic-reserve", 'x', opval);
  }
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SLL) : /* l.sll $rD,$rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = SLLSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SLLI) : /* l.slli $rD,$rA,${uimm6} */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_slli.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = SLLSI (GET_H_GPR (FLD (f_r2)), FLD (f_uimm6));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SRL) : /* l.srl $rD,$rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = SRLSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SRLI) : /* l.srli $rD,$rA,${uimm6} */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_slli.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = SRLSI (GET_H_GPR (FLD (f_r2)), FLD (f_uimm6));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SRA) : /* l.sra $rD,$rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = SRASI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SRAI) : /* l.srai $rD,$rA,${uimm6} */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_slli.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = SRASI (GET_H_GPR (FLD (f_r2)), FLD (f_uimm6));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_ROR) : /* l.ror $rD,$rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = RORSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_RORI) : /* l.rori $rD,$rA,${uimm6} */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_slli.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = RORSI (GET_H_GPR (FLD (f_r2)), FLD (f_uimm6));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_AND) : /* l.and $rD,$rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ANDSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_OR) : /* l.or $rD,$rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ORSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_XOR) : /* l.xor $rD,$rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = XORSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_ADD) : /* l.add $rD,$rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
{
  {
    BI opval = ADDCFSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)), 0);
    SET_H_SYS_SR_CY (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-cy", 'x', opval);
  }
  {
    BI opval = ADDOFSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)), 0);
    SET_H_SYS_SR_OV (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-ov", 'x', opval);
  }
  {
    USI opval = ADDSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
}
if (ANDIF (GET_H_SYS_SR_OV (), GET_H_SYS_SR_OVE ())) {
or1k32bf_exception (current_cpu, pc, EXCEPT_RANGE);
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SUB) : /* l.sub $rD,$rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
{
  {
    BI opval = SUBCFSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)), 0);
    SET_H_SYS_SR_CY (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-cy", 'x', opval);
  }
  {
    BI opval = SUBOFSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)), 0);
    SET_H_SYS_SR_OV (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-ov", 'x', opval);
  }
  {
    USI opval = SUBSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
}
if (ANDIF (GET_H_SYS_SR_OV (), GET_H_SYS_SR_OVE ())) {
or1k32bf_exception (current_cpu, pc, EXCEPT_RANGE);
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_ADDC) : /* l.addc $rD,$rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
{
  BI tmp_tmp_sys_sr_cy;
  tmp_tmp_sys_sr_cy = GET_H_SYS_SR_CY ();
  {
    BI opval = ADDCFSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)), tmp_tmp_sys_sr_cy);
    SET_H_SYS_SR_CY (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-cy", 'x', opval);
  }
  {
    BI opval = ADDOFSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)), tmp_tmp_sys_sr_cy);
    SET_H_SYS_SR_OV (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-ov", 'x', opval);
  }
  {
    USI opval = ADDCSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)), tmp_tmp_sys_sr_cy);
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
}
if (ANDIF (GET_H_SYS_SR_OV (), GET_H_SYS_SR_OVE ())) {
or1k32bf_exception (current_cpu, pc, EXCEPT_RANGE);
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_MUL) : /* l.mul $rD,$rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
{
  {
    BI opval = MUL2OFSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_OV (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-ov", 'x', opval);
  }
  {
    USI opval = MULSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
}
if (ANDIF (GET_H_SYS_SR_OV (), GET_H_SYS_SR_OVE ())) {
or1k32bf_exception (current_cpu, pc, EXCEPT_RANGE);
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_MULD) : /* l.muld $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  DI tmp_result;
  tmp_result = MULDI (EXTSIDI (GET_H_GPR (FLD (f_r2))), EXTSIDI (GET_H_GPR (FLD (f_r3))));
  {
    SI opval = SUBWORDDISI (tmp_result, 0);
    SET_H_MAC_MACHI (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-machi", 'x', opval);
  }
  {
    SI opval = SUBWORDDISI (tmp_result, 1);
    SET_H_MAC_MACLO (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-maclo", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_MULU) : /* l.mulu $rD,$rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
{
  {
    BI opval = MUL1OFSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_CY (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-cy", 'x', opval);
  }
  {
    USI opval = MULSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
}
if (ANDIF (GET_H_SYS_SR_CY (), GET_H_SYS_SR_OVE ())) {
or1k32bf_exception (current_cpu, pc, EXCEPT_RANGE);
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_MULDU) : /* l.muldu $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
  DI tmp_result;
  tmp_result = MULDI (ZEXTSIDI (GET_H_GPR (FLD (f_r2))), ZEXTSIDI (GET_H_GPR (FLD (f_r3))));
  {
    SI opval = SUBWORDDISI (tmp_result, 0);
    SET_H_MAC_MACHI (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-machi", 'x', opval);
  }
  {
    SI opval = SUBWORDDISI (tmp_result, 1);
    SET_H_MAC_MACLO (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-maclo", 'x', opval);
  }
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_DIV) : /* l.div $rD,$rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (GET_H_GPR (FLD (f_r3)), 0)) {
{
  {
    BI opval = 0;
    SET_H_SYS_SR_OV (opval);
    written |= (1 << 5);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-ov", 'x', opval);
  }
  {
    SI opval = DIVSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
}
} else {
{
  {
    BI opval = 1;
    SET_H_SYS_SR_OV (opval);
    written |= (1 << 5);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-ov", 'x', opval);
  }
if (GET_H_SYS_SR_OVE ()) {
or1k32bf_exception (current_cpu, pc, EXCEPT_RANGE);
}
}
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_DIVU) : /* l.divu $rD,$rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (NESI (GET_H_GPR (FLD (f_r3)), 0)) {
{
  {
    BI opval = 0;
    SET_H_SYS_SR_CY (opval);
    written |= (1 << 5);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-cy", 'x', opval);
  }
  {
    USI opval = UDIVSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    written |= (1 << 4);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
}
} else {
{
  {
    BI opval = 1;
    SET_H_SYS_SR_CY (opval);
    written |= (1 << 5);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-cy", 'x', opval);
  }
if (GET_H_SYS_SR_OVE ()) {
or1k32bf_exception (current_cpu, pc, EXCEPT_RANGE);
}
}
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_FF1) : /* l.ff1 $rD,$rA */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_slli.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = or1k32bf_ff1 (current_cpu, GET_H_GPR (FLD (f_r2)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_FL1) : /* l.fl1 $rD,$rA */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_slli.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = or1k32bf_fl1 (current_cpu, GET_H_GPR (FLD (f_r2)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_ANDI) : /* l.andi $rD,$rA,$uimm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ANDSI (GET_H_GPR (FLD (f_r2)), ZEXTSISI (FLD (f_uimm16)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_ORI) : /* l.ori $rD,$rA,$uimm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ORSI (GET_H_GPR (FLD (f_r2)), ZEXTSISI (FLD (f_uimm16)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_XORI) : /* l.xori $rD,$rA,$simm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = XORSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_ADDI) : /* l.addi $rD,$rA,$simm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
{
  {
    BI opval = ADDCFSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 0);
    SET_H_SYS_SR_CY (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-cy", 'x', opval);
  }
  {
    BI opval = ADDOFSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 0);
    SET_H_SYS_SR_OV (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-ov", 'x', opval);
  }
  {
    USI opval = ADDSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
}
if (ANDIF (GET_H_SYS_SR_OV (), GET_H_SYS_SR_OVE ())) {
or1k32bf_exception (current_cpu, pc, EXCEPT_RANGE);
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_ADDIC) : /* l.addic $rD,$rA,$simm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
{
  BI tmp_tmp_sys_sr_cy;
  tmp_tmp_sys_sr_cy = GET_H_SYS_SR_CY ();
  {
    BI opval = ADDCFSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), tmp_tmp_sys_sr_cy);
    SET_H_SYS_SR_CY (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-cy", 'x', opval);
  }
  {
    BI opval = ADDOFSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), tmp_tmp_sys_sr_cy);
    SET_H_SYS_SR_OV (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-ov", 'x', opval);
  }
  {
    SI opval = ADDCSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), tmp_tmp_sys_sr_cy);
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
}
if (ANDIF (GET_H_SYS_SR_OV (), GET_H_SYS_SR_OVE ())) {
or1k32bf_exception (current_cpu, pc, EXCEPT_RANGE);
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_MULI) : /* l.muli $rD,$rA,$simm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
{
  {
    USI opval = MUL2OFSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_OV (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-ov", 'x', opval);
  }
  {
    USI opval = MULSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
}
if (ANDIF (GET_H_SYS_SR_OV (), GET_H_SYS_SR_OVE ())) {
or1k32bf_exception (current_cpu, pc, EXCEPT_RANGE);
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_EXTHS) : /* l.exths $rD,$rA */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_slli.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = EXTHISI (TRUNCSIHI (GET_H_GPR (FLD (f_r2))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_EXTBS) : /* l.extbs $rD,$rA */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_slli.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = EXTQISI (TRUNCSIQI (GET_H_GPR (FLD (f_r2))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_EXTHZ) : /* l.exthz $rD,$rA */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_slli.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ZEXTHISI (TRUNCSIHI (GET_H_GPR (FLD (f_r2))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_EXTBZ) : /* l.extbz $rD,$rA */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_slli.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ZEXTQISI (TRUNCSIQI (GET_H_GPR (FLD (f_r2))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_EXTWS) : /* l.extws $rD,$rA */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_slli.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = EXTSISI (TRUNCSISI (GET_H_GPR (FLD (f_r2))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_EXTWZ) : /* l.extwz $rD,$rA */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_slli.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ZEXTSISI (TRUNCSISI (GET_H_GPR (FLD (f_r2))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_CMOV) : /* l.cmov $rD,$rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

if (GET_H_SYS_SR_F ()) {
  {
    USI opval = GET_H_GPR (FLD (f_r2));
    SET_H_GPR (FLD (f_r1), opval);
    written |= (1 << 3);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
} else {
  {
    USI opval = GET_H_GPR (FLD (f_r3));
    SET_H_GPR (FLD (f_r1), opval);
    written |= (1 << 3);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }
}

  abuf->written = written;
#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFGTS) : /* l.sfgts $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GTSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFGTSI) : /* l.sfgtsi $rA,$simm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GTSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFGTU) : /* l.sfgtu $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GTUSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFGTUI) : /* l.sfgtui $rA,$simm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GTUSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFGES) : /* l.sfges $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GESI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFGESI) : /* l.sfgesi $rA,$simm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GESI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFGEU) : /* l.sfgeu $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GEUSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFGEUI) : /* l.sfgeui $rA,$simm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GEUSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFLTS) : /* l.sflts $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LTSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFLTSI) : /* l.sfltsi $rA,$simm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LTSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFLTU) : /* l.sfltu $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LTUSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFLTUI) : /* l.sfltui $rA,$simm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LTUSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFLES) : /* l.sfles $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LESI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFLESI) : /* l.sflesi $rA,$simm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LESI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFLEU) : /* l.sfleu $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LEUSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFLEUI) : /* l.sfleui $rA,$simm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LEUSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFEQ) : /* l.sfeq $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = EQSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFEQI) : /* l.sfeqi $rA,$simm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = EQSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFNE) : /* l.sfne $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = NESI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_SFNEI) : /* l.sfnei $rA,$simm16 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = NESI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_MAC) : /* l.mac $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
{
  DI tmp_prod;
  DI tmp_mac;
  DI tmp_result;
  tmp_prod = MULDI (EXTSIDI (GET_H_GPR (FLD (f_r2))), EXTSIDI (GET_H_GPR (FLD (f_r3))));
  tmp_mac = JOINSIDI (GET_H_MAC_MACHI (), GET_H_MAC_MACLO ());
  tmp_result = ADDDI (tmp_prod, tmp_mac);
  {
    SI opval = SUBWORDDISI (tmp_result, 0);
    SET_H_MAC_MACHI (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-machi", 'x', opval);
  }
  {
    SI opval = SUBWORDDISI (tmp_result, 1);
    SET_H_MAC_MACLO (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-maclo", 'x', opval);
  }
  {
    BI opval = ADDOFDI (tmp_prod, tmp_mac, 0);
    SET_H_SYS_SR_OV (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-ov", 'x', opval);
  }
}
if (ANDIF (GET_H_SYS_SR_OV (), GET_H_SYS_SR_OVE ())) {
or1k32bf_exception (current_cpu, pc, EXCEPT_RANGE);
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_MACI) : /* l.maci $rA,${simm16} */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
{
  DI tmp_prod;
  DI tmp_mac;
  DI tmp_result;
  tmp_prod = MULDI (EXTSIDI (GET_H_GPR (FLD (f_r2))), EXTSIDI (FLD (f_simm16)));
  tmp_mac = JOINSIDI (GET_H_MAC_MACHI (), GET_H_MAC_MACLO ());
  tmp_result = ADDDI (tmp_mac, tmp_prod);
  {
    SI opval = SUBWORDDISI (tmp_result, 0);
    SET_H_MAC_MACHI (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-machi", 'x', opval);
  }
  {
    SI opval = SUBWORDDISI (tmp_result, 1);
    SET_H_MAC_MACLO (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-maclo", 'x', opval);
  }
  {
    BI opval = ADDOFDI (tmp_prod, tmp_mac, 0);
    SET_H_SYS_SR_OV (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-ov", 'x', opval);
  }
}
if (ANDIF (GET_H_SYS_SR_OV (), GET_H_SYS_SR_OVE ())) {
or1k32bf_exception (current_cpu, pc, EXCEPT_RANGE);
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_MACU) : /* l.macu $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
{
  DI tmp_prod;
  DI tmp_mac;
  DI tmp_result;
  tmp_prod = MULDI (ZEXTSIDI (GET_H_GPR (FLD (f_r2))), ZEXTSIDI (GET_H_GPR (FLD (f_r3))));
  tmp_mac = JOINSIDI (GET_H_MAC_MACHI (), GET_H_MAC_MACLO ());
  tmp_result = ADDDI (tmp_prod, tmp_mac);
  {
    SI opval = SUBWORDDISI (tmp_result, 0);
    SET_H_MAC_MACHI (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-machi", 'x', opval);
  }
  {
    SI opval = SUBWORDDISI (tmp_result, 1);
    SET_H_MAC_MACLO (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-maclo", 'x', opval);
  }
  {
    BI opval = ADDCFDI (tmp_prod, tmp_mac, 0);
    SET_H_SYS_SR_CY (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-cy", 'x', opval);
  }
}
if (ANDIF (GET_H_SYS_SR_CY (), GET_H_SYS_SR_OVE ())) {
or1k32bf_exception (current_cpu, pc, EXCEPT_RANGE);
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_MSB) : /* l.msb $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
{
  DI tmp_prod;
  DI tmp_mac;
  DI tmp_result;
  tmp_prod = MULDI (EXTSIDI (GET_H_GPR (FLD (f_r2))), EXTSIDI (GET_H_GPR (FLD (f_r3))));
  tmp_mac = JOINSIDI (GET_H_MAC_MACHI (), GET_H_MAC_MACLO ());
  tmp_result = SUBDI (tmp_mac, tmp_prod);
  {
    SI opval = SUBWORDDISI (tmp_result, 0);
    SET_H_MAC_MACHI (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-machi", 'x', opval);
  }
  {
    SI opval = SUBWORDDISI (tmp_result, 1);
    SET_H_MAC_MACLO (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-maclo", 'x', opval);
  }
  {
    BI opval = SUBOFDI (tmp_mac, tmp_result, 0);
    SET_H_SYS_SR_OV (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-ov", 'x', opval);
  }
}
if (ANDIF (GET_H_SYS_SR_OV (), GET_H_SYS_SR_OVE ())) {
or1k32bf_exception (current_cpu, pc, EXCEPT_RANGE);
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_MSBU) : /* l.msbu $rA,$rB */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

{
{
  DI tmp_prod;
  DI tmp_mac;
  DI tmp_result;
  tmp_prod = MULDI (ZEXTSIDI (GET_H_GPR (FLD (f_r2))), ZEXTSIDI (GET_H_GPR (FLD (f_r3))));
  tmp_mac = JOINSIDI (GET_H_MAC_MACHI (), GET_H_MAC_MACLO ());
  tmp_result = SUBDI (tmp_mac, tmp_prod);
  {
    SI opval = SUBWORDDISI (tmp_result, 0);
    SET_H_MAC_MACHI (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-machi", 'x', opval);
  }
  {
    SI opval = SUBWORDDISI (tmp_result, 1);
    SET_H_MAC_MACLO (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "mac-maclo", 'x', opval);
  }
  {
    BI opval = SUBCFDI (tmp_mac, tmp_result, 0);
    SET_H_SYS_SR_CY (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-cy", 'x', opval);
  }
}
if (ANDIF (GET_H_SYS_SR_CY (), GET_H_SYS_SR_OVE ())) {
or1k32bf_exception (current_cpu, pc, EXCEPT_RANGE);
}
}

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_CUST1) : /* l.cust1 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_CUST2) : /* l.cust2 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_CUST3) : /* l.cust3 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_CUST4) : /* l.cust4 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_CUST5) : /* l.cust5 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_CUST6) : /* l.cust6 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_CUST7) : /* l.cust7 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_L_CUST8) : /* l.cust8 */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_ADD_S) : /* lf.add.s $rDSF,$rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SF opval = CGEN_CPU_FPU (current_cpu)->ops->addsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_FSR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fsr", 'f', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_ADD_D32) : /* lf.add.d $rDD32F,$rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DF opval = CGEN_CPU_FPU (current_cpu)->ops->adddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_FD32R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fd32r", 'f', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SUB_S) : /* lf.sub.s $rDSF,$rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SF opval = CGEN_CPU_FPU (current_cpu)->ops->subsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_FSR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fsr", 'f', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SUB_D32) : /* lf.sub.d $rDD32F,$rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DF opval = CGEN_CPU_FPU (current_cpu)->ops->subdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_FD32R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fd32r", 'f', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_MUL_S) : /* lf.mul.s $rDSF,$rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SF opval = CGEN_CPU_FPU (current_cpu)->ops->mulsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_FSR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fsr", 'f', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_MUL_D32) : /* lf.mul.d $rDD32F,$rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DF opval = CGEN_CPU_FPU (current_cpu)->ops->muldf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_FD32R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fd32r", 'f', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_DIV_S) : /* lf.div.s $rDSF,$rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SF opval = CGEN_CPU_FPU (current_cpu)->ops->divsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_FSR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fsr", 'f', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_DIV_D32) : /* lf.div.d $rDD32F,$rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DF opval = CGEN_CPU_FPU (current_cpu)->ops->divdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_FD32R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fd32r", 'f', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_REM_S) : /* lf.rem.s $rDSF,$rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SF opval = CGEN_CPU_FPU (current_cpu)->ops->remsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_FSR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fsr", 'f', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_REM_D32) : /* lf.rem.d $rDD32F,$rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DF opval = CGEN_CPU_FPU (current_cpu)->ops->remdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_FD32R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fd32r", 'f', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_ITOF_S) : /* lf.itof.s $rDSF,$rA */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_slli.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SF opval = CGEN_CPU_FPU (current_cpu)->ops->floatsisf (CGEN_CPU_FPU (current_cpu), (GET_H_SYS_FPCSR_RM () == 0) ? (1) : (GET_H_SYS_FPCSR_RM () == 1) ? (3) : (GET_H_SYS_FPCSR_RM () == 2) ? (4) : (5), TRUNCSISI (GET_H_GPR (FLD (f_r2))));
    SET_H_FSR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fsr", 'f', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_ITOF_D32) : /* lf.itof.d $rDD32F,$rADI */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DF opval = CGEN_CPU_FPU (current_cpu)->ops->floatdidf (CGEN_CPU_FPU (current_cpu), (GET_H_SYS_FPCSR_RM () == 0) ? (1) : (GET_H_SYS_FPCSR_RM () == 1) ? (3) : (GET_H_SYS_FPCSR_RM () == 2) ? (4) : (5), GET_H_I64R (FLD (f_rad32)));
    SET_H_FD32R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fd32r", 'f', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_FTOI_S) : /* lf.ftoi.s $rD,$rASF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_slli.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTSISI (CGEN_CPU_FPU (current_cpu)->ops->fixsfsi (CGEN_CPU_FPU (current_cpu), (GET_H_SYS_FPCSR_RM () == 0) ? (1) : (GET_H_SYS_FPCSR_RM () == 1) ? (3) : (GET_H_SYS_FPCSR_RM () == 2) ? (4) : (5), GET_H_FSR (FLD (f_r2))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_FTOI_D32) : /* lf.ftoi.d $rDDI,$rAD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DI opval = CGEN_CPU_FPU (current_cpu)->ops->fixdfdi (CGEN_CPU_FPU (current_cpu), (GET_H_SYS_FPCSR_RM () == 0) ? (1) : (GET_H_SYS_FPCSR_RM () == 1) ? (3) : (GET_H_SYS_FPCSR_RM () == 2) ? (4) : (5), GET_H_FD32R (FLD (f_rad32)));
    SET_H_I64R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "i64r", 'D', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFEQ_S) : /* lf.sfeq.s $rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->eqsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFEQ_D32) : /* lf.sfeq.d $rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->eqdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFNE_S) : /* lf.sfne.s $rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->nesf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFNE_D32) : /* lf.sfne.d $rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->nedf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFGE_S) : /* lf.sfge.s $rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->gesf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFGE_D32) : /* lf.sfge.d $rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->gedf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFGT_S) : /* lf.sfgt.s $rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->gtsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFGT_D32) : /* lf.sfgt.d $rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->gtdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFLT_S) : /* lf.sflt.s $rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->ltsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFLT_D32) : /* lf.sflt.d $rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->ltdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFLE_S) : /* lf.sfle.s $rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->lesf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFLE_D32) : /* lf.sfle.d $rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->ledf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFUEQ_S) : /* lf.sfueq.s $rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unorderedsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))), CGEN_CPU_FPU (current_cpu)->ops->eqsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFUEQ_D32) : /* lf.sfueq.d $rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unordereddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))), CGEN_CPU_FPU (current_cpu)->ops->eqdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFUNE_S) : /* lf.sfune.s $rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unorderedsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))), CGEN_CPU_FPU (current_cpu)->ops->nesf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFUNE_D32) : /* lf.sfune.d $rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unordereddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))), CGEN_CPU_FPU (current_cpu)->ops->nedf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFUGT_S) : /* lf.sfugt.s $rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unorderedsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))), CGEN_CPU_FPU (current_cpu)->ops->gtsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFUGT_D32) : /* lf.sfugt.d $rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unordereddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))), CGEN_CPU_FPU (current_cpu)->ops->gtdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFUGE_S) : /* lf.sfuge.s $rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unorderedsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))), CGEN_CPU_FPU (current_cpu)->ops->gesf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFUGE_D32) : /* lf.sfuge.d $rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unordereddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))), CGEN_CPU_FPU (current_cpu)->ops->gedf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFULT_S) : /* lf.sfult.s $rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unorderedsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))), CGEN_CPU_FPU (current_cpu)->ops->ltsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFULT_D32) : /* lf.sfult.d $rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unordereddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))), CGEN_CPU_FPU (current_cpu)->ops->ltdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFULE_S) : /* lf.sfule.s $rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unorderedsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))), CGEN_CPU_FPU (current_cpu)->ops->lesf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFULE_D32) : /* lf.sfule.d $rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unordereddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))), CGEN_CPU_FPU (current_cpu)->ops->ledf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFUN_S) : /* lf.sfun.s $rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->unorderedsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_SFUN_D32) : /* lf.sfun.d $rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->unordereddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_MADD_S) : /* lf.madd.s $rDSF,$rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_l_sll.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SF opval = CGEN_CPU_FPU (current_cpu)->ops->addsf (CGEN_CPU_FPU (current_cpu), CGEN_CPU_FPU (current_cpu)->ops->mulsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))), GET_H_FSR (FLD (f_r1)));
    SET_H_FSR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fsr", 'f', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_MADD_D32) : /* lf.madd.d $rDD32F,$rAD32F,$rBD32F */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DF opval = CGEN_CPU_FPU (current_cpu)->ops->adddf (CGEN_CPU_FPU (current_cpu), CGEN_CPU_FPU (current_cpu)->ops->muldf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))), GET_H_FD32R (FLD (f_rdd32)));
    SET_H_FD32R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fd32r", 'f', opval);
  }

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_CUST1_S) : /* lf.cust1.s $rASF,$rBSF */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);

  CASE (sem, INSN_LF_CUST1_D32) : /* lf.cust1.d */
{
  SEM_ARG sem_arg = SEM_SEM_ARG (vpc, sc);
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
#define FLD(f) abuf->fields.sfmt_empty.f
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

#undef FLD
}
  NEXT (vpc);


    }
  ENDSWITCH (sem) /* End of semantic switch.  */

  /* At this point `vpc' contains the next insn to execute.  */
}

#undef DEFINE_SWITCH
#endif /* DEFINE_SWITCH */
