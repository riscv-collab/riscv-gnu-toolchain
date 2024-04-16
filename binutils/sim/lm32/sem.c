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

#define WANT_CPU lm32bf
#define WANT_CPU_LM32BF

#include "sim-main.h"
#include "cgen-mem.h"
#include "cgen-ops.h"

#undef GET_ATTR
#define GET_ATTR(cpu, num, attr) CGEN_ATTR_VALUE (NULL, abuf->idesc->attrs, CGEN_INSN_##attr)

/* This is used so that we can compile two copies of the semantic code,
   one with full feature support and one without that runs fast(er).
   FAST_P, when desired, is defined on the command line, -DFAST_P=1.  */
#if FAST_P
#define SEM_FN_NAME(cpu,fn) XCONCAT3 (cpu,_semf_,fn)
#undef CGEN_TRACE_RESULT
#define CGEN_TRACE_RESULT(cpu, abuf, name, type, val)
#else
#define SEM_FN_NAME(cpu,fn) XCONCAT3 (cpu,_sem_,fn)
#endif

/* x-invalid: --invalid-- */

static SEM_PC
SEM_FN_NAME (lm32bf,x_invalid) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

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

  return vpc;
#undef FLD
}

/* x-after: --after-- */

static SEM_PC
SEM_FN_NAME (lm32bf,x_after) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_LM32BF
    lm32bf_pbb_after (current_cpu, sem_arg);
#endif
  }

  return vpc;
#undef FLD
}

/* x-before: --before-- */

static SEM_PC
SEM_FN_NAME (lm32bf,x_before) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_LM32BF
    lm32bf_pbb_before (current_cpu, sem_arg);
#endif
  }

  return vpc;
#undef FLD
}

/* x-cti-chain: --cti-chain-- */

static SEM_PC
SEM_FN_NAME (lm32bf,x_cti_chain) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

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

  return vpc;
#undef FLD
}

/* x-chain: --chain-- */

static SEM_PC
SEM_FN_NAME (lm32bf,x_chain) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_LM32BF
    vpc = lm32bf_pbb_chain (current_cpu, sem_arg);
#ifdef DEFINE_SWITCH
    BREAK (sem);
#endif
#endif
  }

  return vpc;
#undef FLD
}

/* x-begin: --begin-- */

static SEM_PC
SEM_FN_NAME (lm32bf,x_begin) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

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

  return vpc;
#undef FLD
}

/* add: add $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,add) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* addi: addi $r1,$r0,$imm */

static SEM_PC
SEM_FN_NAME (lm32bf,addi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* and: and $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,and) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* andi: andi $r1,$r0,$uimm */

static SEM_PC
SEM_FN_NAME (lm32bf,andi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_andi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (CPU (h_gr[FLD (f_r0)]), ZEXTSISI (FLD (f_uimm)));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* andhii: andhi $r1,$r0,$hi16 */

static SEM_PC
SEM_FN_NAME (lm32bf,andhii) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_andi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ANDSI (CPU (h_gr[FLD (f_r0)]), SLLSI (FLD (f_uimm), 16));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* b: b $r0 */

static SEM_PC
SEM_FN_NAME (lm32bf,b) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_be.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = lm32bf_b_insn (current_cpu, CPU (h_gr[FLD (f_r0)]), FLD (f_r0));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* bi: bi $call */

static SEM_PC
SEM_FN_NAME (lm32bf,bi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = EXTSISI (FLD (i_call));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* be: be $r0,$r1,$branch */

static SEM_PC
SEM_FN_NAME (lm32bf,be) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_be.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* bg: bg $r0,$r1,$branch */

static SEM_PC
SEM_FN_NAME (lm32bf,bg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_be.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* bge: bge $r0,$r1,$branch */

static SEM_PC
SEM_FN_NAME (lm32bf,bge) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_be.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* bgeu: bgeu $r0,$r1,$branch */

static SEM_PC
SEM_FN_NAME (lm32bf,bgeu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_be.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* bgu: bgu $r0,$r1,$branch */

static SEM_PC
SEM_FN_NAME (lm32bf,bgu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_be.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* bne: bne $r0,$r1,$branch */

static SEM_PC
SEM_FN_NAME (lm32bf,bne) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_be.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* call: call $r0 */

static SEM_PC
SEM_FN_NAME (lm32bf,call) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_be.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* calli: calli $call */

static SEM_PC
SEM_FN_NAME (lm32bf,calli) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_bi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* cmpe: cmpe $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,cmpe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EQSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpei: cmpei $r1,$r0,$imm */

static SEM_PC
SEM_FN_NAME (lm32bf,cmpei) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EQSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpg: cmpg $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,cmpg) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GTSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpgi: cmpgi $r1,$r0,$imm */

static SEM_PC
SEM_FN_NAME (lm32bf,cmpgi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GTSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpge: cmpge $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,cmpge) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GESI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpgei: cmpgei $r1,$r0,$imm */

static SEM_PC
SEM_FN_NAME (lm32bf,cmpgei) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GESI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpgeu: cmpgeu $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,cmpgeu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GEUSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpgeui: cmpgeui $r1,$r0,$uimm */

static SEM_PC
SEM_FN_NAME (lm32bf,cmpgeui) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_andi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GEUSI (CPU (h_gr[FLD (f_r0)]), ZEXTSISI (FLD (f_uimm)));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpgu: cmpgu $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,cmpgu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GTUSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpgui: cmpgui $r1,$r0,$uimm */

static SEM_PC
SEM_FN_NAME (lm32bf,cmpgui) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_andi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GTUSI (CPU (h_gr[FLD (f_r0)]), ZEXTSISI (FLD (f_uimm)));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpne: cmpne $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,cmpne) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* cmpnei: cmpnei $r1,$r0,$imm */

static SEM_PC
SEM_FN_NAME (lm32bf,cmpnei) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = NESI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* divu: divu $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,divu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = lm32bf_divu_insn (current_cpu, pc, FLD (f_r0), FLD (f_r1), FLD (f_r2));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* lb: lb $r1,($r0+$imm) */

static SEM_PC
SEM_FN_NAME (lm32bf,lb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTQISI (GETMEMQI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lbu: lbu $r1,($r0+$imm) */

static SEM_PC
SEM_FN_NAME (lm32bf,lbu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ZEXTQISI (GETMEMQI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lh: lh $r1,($r0+$imm) */

static SEM_PC
SEM_FN_NAME (lm32bf,lh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTHISI (GETMEMHI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lhu: lhu $r1,($r0+$imm) */

static SEM_PC
SEM_FN_NAME (lm32bf,lhu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ZEXTHISI (GETMEMHI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lw: lw $r1,($r0+$imm) */

static SEM_PC
SEM_FN_NAME (lm32bf,lw) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = GETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm)))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* modu: modu $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,modu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = lm32bf_modu_insn (current_cpu, pc, FLD (f_r0), FLD (f_r1), FLD (f_r2));
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* mul: mul $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,mul) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MULSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* muli: muli $r1,$r0,$imm */

static SEM_PC
SEM_FN_NAME (lm32bf,muli) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = MULSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* nor: nor $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,nor) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (ORSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)])));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* nori: nori $r1,$r0,$uimm */

static SEM_PC
SEM_FN_NAME (lm32bf,nori) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_andi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (ORSI (CPU (h_gr[FLD (f_r0)]), ZEXTSISI (FLD (f_uimm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* or: or $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,or) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* ori: ori $r1,$r0,$lo16 */

static SEM_PC
SEM_FN_NAME (lm32bf,ori) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_andi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (CPU (h_gr[FLD (f_r0)]), ZEXTSISI (FLD (f_uimm)));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* orhii: orhi $r1,$r0,$hi16 */

static SEM_PC
SEM_FN_NAME (lm32bf,orhii) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_andi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = ORSI (CPU (h_gr[FLD (f_r0)]), SLLSI (FLD (f_uimm), 16));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* rcsr: rcsr $r2,$csr */

static SEM_PC
SEM_FN_NAME (lm32bf,rcsr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_rcsr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = CPU (h_csr[FLD (f_csr)]);
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sb: sb ($r0+$imm),$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,sb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    QI opval = CPU (h_gr[FLD (f_r1)]);
    SETMEMQI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm)))), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sextb: sextb $r2,$r0 */

static SEM_PC
SEM_FN_NAME (lm32bf,sextb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTQISI (TRUNCSIQI (CPU (h_gr[FLD (f_r0)])));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sexth: sexth $r2,$r0 */

static SEM_PC
SEM_FN_NAME (lm32bf,sexth) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTHISI (TRUNCSIHI (CPU (h_gr[FLD (f_r0)])));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sh: sh ($r0+$imm),$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,sh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    HI opval = CPU (h_gr[FLD (f_r1)]);
    SETMEMHI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm)))), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sl: sl $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,sl) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SLLSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sli: sli $r1,$r0,$imm */

static SEM_PC
SEM_FN_NAME (lm32bf,sli) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SLLSI (CPU (h_gr[FLD (f_r0)]), FLD (f_imm));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sr: sr $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,sr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SRASI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sri: sri $r1,$r0,$imm */

static SEM_PC
SEM_FN_NAME (lm32bf,sri) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SRASI (CPU (h_gr[FLD (f_r0)]), FLD (f_imm));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sru: sru $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,sru) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SRLSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* srui: srui $r1,$r0,$imm */

static SEM_PC
SEM_FN_NAME (lm32bf,srui) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SRLSI (CPU (h_gr[FLD (f_r0)]), FLD (f_imm));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sub: sub $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,sub) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = SUBSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* sw: sw ($r0+$imm),$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,sw) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_addi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = CPU (h_gr[FLD (f_r1)]);
    SETMEMSI (current_cpu, pc, ADDSI (CPU (h_gr[FLD (f_r0)]), EXTHISI (TRUNCSIHI (FLD (f_imm)))), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "memory", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* user: user $r2,$r0,$r1,$user */

static SEM_PC
SEM_FN_NAME (lm32bf,user) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = lm32bf_user_insn (current_cpu, CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]), FLD (f_user));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* wcsr: wcsr $csr,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,wcsr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_wcsr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

lm32bf_wcsr_insn (current_cpu, FLD (f_csr), CPU (h_gr[FLD (f_r1)]));

  return vpc;
#undef FLD
}

/* xor: xor $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,xor) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)]));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* xori: xori $r1,$r0,$uimm */

static SEM_PC
SEM_FN_NAME (lm32bf,xori) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_andi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = XORSI (CPU (h_gr[FLD (f_r0)]), ZEXTSISI (FLD (f_uimm)));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* xnor: xnor $r2,$r0,$r1 */

static SEM_PC
SEM_FN_NAME (lm32bf,xnor) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_user.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (XORSI (CPU (h_gr[FLD (f_r0)]), CPU (h_gr[FLD (f_r1)])));
    CPU (h_gr[FLD (f_r2)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* xnori: xnori $r1,$r0,$uimm */

static SEM_PC
SEM_FN_NAME (lm32bf,xnori) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_andi.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = INVSI (XORSI (CPU (h_gr[FLD (f_r0)]), ZEXTSISI (FLD (f_uimm))));
    CPU (h_gr[FLD (f_r1)]) = opval;
    CGEN_TRACE_RESULT (current_cpu, abuf, "gr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* break: break */

static SEM_PC
SEM_FN_NAME (lm32bf,break) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = lm32bf_break_insn (current_cpu, pc);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* scall: scall */

static SEM_PC
SEM_FN_NAME (lm32bf,scall) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = lm32bf_scall_insn (current_cpu, pc);
    SEM_BRANCH_VIA_ADDR (current_cpu, sem_arg, opval, vpc);
    CGEN_TRACE_RESULT (current_cpu, abuf, "pc", 'x', opval);
  }

  SEM_BRANCH_FINI (vpc);
  return vpc;
#undef FLD
}

/* Table of all semantic fns.  */

static const struct sem_fn_desc sem_fns[] = {
  { LM32BF_INSN_X_INVALID, SEM_FN_NAME (lm32bf,x_invalid) },
  { LM32BF_INSN_X_AFTER, SEM_FN_NAME (lm32bf,x_after) },
  { LM32BF_INSN_X_BEFORE, SEM_FN_NAME (lm32bf,x_before) },
  { LM32BF_INSN_X_CTI_CHAIN, SEM_FN_NAME (lm32bf,x_cti_chain) },
  { LM32BF_INSN_X_CHAIN, SEM_FN_NAME (lm32bf,x_chain) },
  { LM32BF_INSN_X_BEGIN, SEM_FN_NAME (lm32bf,x_begin) },
  { LM32BF_INSN_ADD, SEM_FN_NAME (lm32bf,add) },
  { LM32BF_INSN_ADDI, SEM_FN_NAME (lm32bf,addi) },
  { LM32BF_INSN_AND, SEM_FN_NAME (lm32bf,and) },
  { LM32BF_INSN_ANDI, SEM_FN_NAME (lm32bf,andi) },
  { LM32BF_INSN_ANDHII, SEM_FN_NAME (lm32bf,andhii) },
  { LM32BF_INSN_B, SEM_FN_NAME (lm32bf,b) },
  { LM32BF_INSN_BI, SEM_FN_NAME (lm32bf,bi) },
  { LM32BF_INSN_BE, SEM_FN_NAME (lm32bf,be) },
  { LM32BF_INSN_BG, SEM_FN_NAME (lm32bf,bg) },
  { LM32BF_INSN_BGE, SEM_FN_NAME (lm32bf,bge) },
  { LM32BF_INSN_BGEU, SEM_FN_NAME (lm32bf,bgeu) },
  { LM32BF_INSN_BGU, SEM_FN_NAME (lm32bf,bgu) },
  { LM32BF_INSN_BNE, SEM_FN_NAME (lm32bf,bne) },
  { LM32BF_INSN_CALL, SEM_FN_NAME (lm32bf,call) },
  { LM32BF_INSN_CALLI, SEM_FN_NAME (lm32bf,calli) },
  { LM32BF_INSN_CMPE, SEM_FN_NAME (lm32bf,cmpe) },
  { LM32BF_INSN_CMPEI, SEM_FN_NAME (lm32bf,cmpei) },
  { LM32BF_INSN_CMPG, SEM_FN_NAME (lm32bf,cmpg) },
  { LM32BF_INSN_CMPGI, SEM_FN_NAME (lm32bf,cmpgi) },
  { LM32BF_INSN_CMPGE, SEM_FN_NAME (lm32bf,cmpge) },
  { LM32BF_INSN_CMPGEI, SEM_FN_NAME (lm32bf,cmpgei) },
  { LM32BF_INSN_CMPGEU, SEM_FN_NAME (lm32bf,cmpgeu) },
  { LM32BF_INSN_CMPGEUI, SEM_FN_NAME (lm32bf,cmpgeui) },
  { LM32BF_INSN_CMPGU, SEM_FN_NAME (lm32bf,cmpgu) },
  { LM32BF_INSN_CMPGUI, SEM_FN_NAME (lm32bf,cmpgui) },
  { LM32BF_INSN_CMPNE, SEM_FN_NAME (lm32bf,cmpne) },
  { LM32BF_INSN_CMPNEI, SEM_FN_NAME (lm32bf,cmpnei) },
  { LM32BF_INSN_DIVU, SEM_FN_NAME (lm32bf,divu) },
  { LM32BF_INSN_LB, SEM_FN_NAME (lm32bf,lb) },
  { LM32BF_INSN_LBU, SEM_FN_NAME (lm32bf,lbu) },
  { LM32BF_INSN_LH, SEM_FN_NAME (lm32bf,lh) },
  { LM32BF_INSN_LHU, SEM_FN_NAME (lm32bf,lhu) },
  { LM32BF_INSN_LW, SEM_FN_NAME (lm32bf,lw) },
  { LM32BF_INSN_MODU, SEM_FN_NAME (lm32bf,modu) },
  { LM32BF_INSN_MUL, SEM_FN_NAME (lm32bf,mul) },
  { LM32BF_INSN_MULI, SEM_FN_NAME (lm32bf,muli) },
  { LM32BF_INSN_NOR, SEM_FN_NAME (lm32bf,nor) },
  { LM32BF_INSN_NORI, SEM_FN_NAME (lm32bf,nori) },
  { LM32BF_INSN_OR, SEM_FN_NAME (lm32bf,or) },
  { LM32BF_INSN_ORI, SEM_FN_NAME (lm32bf,ori) },
  { LM32BF_INSN_ORHII, SEM_FN_NAME (lm32bf,orhii) },
  { LM32BF_INSN_RCSR, SEM_FN_NAME (lm32bf,rcsr) },
  { LM32BF_INSN_SB, SEM_FN_NAME (lm32bf,sb) },
  { LM32BF_INSN_SEXTB, SEM_FN_NAME (lm32bf,sextb) },
  { LM32BF_INSN_SEXTH, SEM_FN_NAME (lm32bf,sexth) },
  { LM32BF_INSN_SH, SEM_FN_NAME (lm32bf,sh) },
  { LM32BF_INSN_SL, SEM_FN_NAME (lm32bf,sl) },
  { LM32BF_INSN_SLI, SEM_FN_NAME (lm32bf,sli) },
  { LM32BF_INSN_SR, SEM_FN_NAME (lm32bf,sr) },
  { LM32BF_INSN_SRI, SEM_FN_NAME (lm32bf,sri) },
  { LM32BF_INSN_SRU, SEM_FN_NAME (lm32bf,sru) },
  { LM32BF_INSN_SRUI, SEM_FN_NAME (lm32bf,srui) },
  { LM32BF_INSN_SUB, SEM_FN_NAME (lm32bf,sub) },
  { LM32BF_INSN_SW, SEM_FN_NAME (lm32bf,sw) },
  { LM32BF_INSN_USER, SEM_FN_NAME (lm32bf,user) },
  { LM32BF_INSN_WCSR, SEM_FN_NAME (lm32bf,wcsr) },
  { LM32BF_INSN_XOR, SEM_FN_NAME (lm32bf,xor) },
  { LM32BF_INSN_XORI, SEM_FN_NAME (lm32bf,xori) },
  { LM32BF_INSN_XNOR, SEM_FN_NAME (lm32bf,xnor) },
  { LM32BF_INSN_XNORI, SEM_FN_NAME (lm32bf,xnori) },
  { LM32BF_INSN_BREAK, SEM_FN_NAME (lm32bf,break) },
  { LM32BF_INSN_SCALL, SEM_FN_NAME (lm32bf,scall) },
  { 0, 0 }
};

/* Add the semantic fns to IDESC_TABLE.  */

void
SEM_FN_NAME (lm32bf,init_idesc_table) (SIM_CPU *current_cpu)
{
  IDESC *idesc_table = CPU_IDESC (current_cpu);
  const struct sem_fn_desc *sf;
  int mach_num = MACH_NUM (CPU_MACH (current_cpu));

  for (sf = &sem_fns[0]; sf->fn != 0; ++sf)
    {
      const CGEN_INSN *insn = idesc_table[sf->index].idata;
      int valid_p = (CGEN_INSN_VIRTUAL_P (insn)
		     || CGEN_INSN_MACH_HAS_P (insn, mach_num));
#if FAST_P
      if (valid_p)
	idesc_table[sf->index].sem_fast = sf->fn;
      else
	idesc_table[sf->index].sem_fast = SEM_FN_NAME (lm32bf,x_invalid);
#else
      if (valid_p)
	idesc_table[sf->index].sem_full = sf->fn;
      else
	idesc_table[sf->index].sem_full = SEM_FN_NAME (lm32bf,x_invalid);
#endif
    }
}

