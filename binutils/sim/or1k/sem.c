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

#define WANT_CPU or1k32bf
#define WANT_CPU_OR1K32BF

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
SEM_FN_NAME (or1k32bf,x_invalid) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
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
SEM_FN_NAME (or1k32bf,x_after) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_OR1K32BF
    or1k32bf_pbb_after (current_cpu, sem_arg);
#endif
  }

  return vpc;
#undef FLD
}

/* x-before: --before-- */

static SEM_PC
SEM_FN_NAME (or1k32bf,x_before) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_OR1K32BF
    or1k32bf_pbb_before (current_cpu, sem_arg);
#endif
  }

  return vpc;
#undef FLD
}

/* x-cti-chain: --cti-chain-- */

static SEM_PC
SEM_FN_NAME (or1k32bf,x_cti_chain) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

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

  return vpc;
#undef FLD
}

/* x-chain: --chain-- */

static SEM_PC
SEM_FN_NAME (or1k32bf,x_chain) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

  {
#if WITH_SCACHE_PBB_OR1K32BF
    vpc = or1k32bf_pbb_chain (current_cpu, sem_arg);
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
SEM_FN_NAME (or1k32bf,x_begin) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 0);

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

  return vpc;
#undef FLD
}

/* l-j: l.j ${disp26} */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_j) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_j.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* l-adrp: l.adrp $rD,${disp21} */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_adrp) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_adrp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = FLD (i_disp21);
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-jal: l.jal ${disp26} */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_jal) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_j.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* l-jr: l.jr $rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_jr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* l-jalr: l.jalr $rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_jalr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* l-bnf: l.bnf ${disp26} */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_bnf) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_j.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* l-bf: l.bf ${disp26} */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_bf) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_j.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_BRANCH_INIT
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* l-trap: l.trap ${uimm16} */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_trap) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

or1k32bf_exception (current_cpu, pc, EXCEPT_TRAP);

  return vpc;
#undef FLD
}

/* l-sys: l.sys ${uimm16} */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sys) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

or1k32bf_exception (current_cpu, pc, EXCEPT_SYSCALL);

  return vpc;
#undef FLD
}

/* l-msync: l.msync */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_msync) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* l-psync: l.psync */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_psync) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* l-csync: l.csync */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_csync) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* l-rfe: l.rfe */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_rfe) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

or1k32bf_rfe (current_cpu);

  return vpc;
#undef FLD
}

/* l-nop-imm: l.nop ${uimm16} */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_nop_imm) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

or1k32bf_nop (current_cpu, ZEXTSISI (FLD (f_uimm16)));

  return vpc;
#undef FLD
}

/* l-movhi: l.movhi $rD,$uimm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_movhi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = SLLSI (ZEXTSISI (FLD (f_uimm16)), 16);
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-macrc: l.macrc $rD */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_macrc) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_adrp.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-mfspr: l.mfspr $rD,$rA,${uimm16} */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_mfspr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = or1k32bf_mfspr (current_cpu, ORSI (GET_H_GPR (FLD (f_r2)), ZEXTSISI (FLD (f_uimm16))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-mtspr: l.mtspr $rA,$rB,${uimm16-split} */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_mtspr) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mtspr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

or1k32bf_mtspr (current_cpu, ORSI (GET_H_GPR (FLD (f_r2)), ZEXTSISI (FLD (f_uimm16_split))), GET_H_GPR (FLD (f_r3)));

  return vpc;
#undef FLD
}

/* l-lwz: l.lwz $rD,${simm16}($rA) */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_lwz) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ZEXTSISI (GETMEMUSI (current_cpu, pc, or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 4)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-lws: l.lws $rD,${simm16}($rA) */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_lws) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTSISI (GETMEMSI (current_cpu, pc, or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 4)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-lwa: l.lwa $rD,${simm16}($rA) */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_lwa) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-lbz: l.lbz $rD,${simm16}($rA) */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_lbz) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ZEXTQISI (GETMEMUQI (current_cpu, pc, or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 1)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-lbs: l.lbs $rD,${simm16}($rA) */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_lbs) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTQISI (GETMEMQI (current_cpu, pc, or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 1)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-lhz: l.lhz $rD,${simm16}($rA) */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_lhz) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ZEXTHISI (GETMEMUHI (current_cpu, pc, or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 2)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-lhs: l.lhs $rD,${simm16}($rA) */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_lhs) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTHISI (GETMEMHI (current_cpu, pc, or1k32bf_make_load_store_addr (current_cpu, GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)), 2)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sw: l.sw ${simm16-split}($rA),$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sw) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sw.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* l-sb: l.sb ${simm16-split}($rA),$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sw.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* l-sh: l.sh ${simm16-split}($rA),$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sh) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sw.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* l-swa: l.swa ${simm16-split}($rA),$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_swa) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sw.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* l-sll: l.sll $rD,$rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sll) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = SLLSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-slli: l.slli $rD,$rA,${uimm6} */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_slli) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = SLLSI (GET_H_GPR (FLD (f_r2)), FLD (f_uimm6));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-srl: l.srl $rD,$rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_srl) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = SRLSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-srli: l.srli $rD,$rA,${uimm6} */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_srli) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = SRLSI (GET_H_GPR (FLD (f_r2)), FLD (f_uimm6));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sra: l.sra $rD,$rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sra) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = SRASI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-srai: l.srai $rD,$rA,${uimm6} */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_srai) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = SRASI (GET_H_GPR (FLD (f_r2)), FLD (f_uimm6));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-ror: l.ror $rD,$rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_ror) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = RORSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-rori: l.rori $rD,$rA,${uimm6} */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_rori) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = RORSI (GET_H_GPR (FLD (f_r2)), FLD (f_uimm6));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-and: l.and $rD,$rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_and) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ANDSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-or: l.or $rD,$rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_or) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ORSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-xor: l.xor $rD,$rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_xor) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = XORSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-add: l.add $rD,$rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_add) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-sub: l.sub $rD,$rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sub) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-addc: l.addc $rD,$rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_addc) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-mul: l.mul $rD,$rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_mul) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-muld: l.muld $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_muld) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-mulu: l.mulu $rD,$rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_mulu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-muldu: l.muldu $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_muldu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-div: l.div $rD,$rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_div) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* l-divu: l.divu $rD,$rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_divu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* l-ff1: l.ff1 $rD,$rA */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_ff1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = or1k32bf_ff1 (current_cpu, GET_H_GPR (FLD (f_r2)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-fl1: l.fl1 $rD,$rA */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_fl1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = or1k32bf_fl1 (current_cpu, GET_H_GPR (FLD (f_r2)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-andi: l.andi $rD,$rA,$uimm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_andi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ANDSI (GET_H_GPR (FLD (f_r2)), ZEXTSISI (FLD (f_uimm16)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-ori: l.ori $rD,$rA,$uimm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_ori) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ORSI (GET_H_GPR (FLD (f_r2)), ZEXTSISI (FLD (f_uimm16)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-xori: l.xori $rD,$rA,$simm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_xori) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = XORSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-addi: l.addi $rD,$rA,$simm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_addi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-addic: l.addic $rD,$rA,$simm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_addic) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-muli: l.muli $rD,$rA,$simm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_muli) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-exths: l.exths $rD,$rA */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_exths) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = EXTHISI (TRUNCSIHI (GET_H_GPR (FLD (f_r2))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-extbs: l.extbs $rD,$rA */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_extbs) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = EXTQISI (TRUNCSIQI (GET_H_GPR (FLD (f_r2))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-exthz: l.exthz $rD,$rA */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_exthz) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ZEXTHISI (TRUNCSIHI (GET_H_GPR (FLD (f_r2))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-extbz: l.extbz $rD,$rA */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_extbz) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ZEXTQISI (TRUNCSIQI (GET_H_GPR (FLD (f_r2))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-extws: l.extws $rD,$rA */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_extws) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = EXTSISI (TRUNCSISI (GET_H_GPR (FLD (f_r2))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-extwz: l.extwz $rD,$rA */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_extwz) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = ZEXTSISI (TRUNCSISI (GET_H_GPR (FLD (f_r2))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-cmov: l.cmov $rD,$rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_cmov) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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
  return vpc;
#undef FLD
}

/* l-sfgts: l.sfgts $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfgts) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GTSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfgtsi: l.sfgtsi $rA,$simm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfgtsi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GTSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfgtu: l.sfgtu $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfgtu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GTUSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfgtui: l.sfgtui $rA,$simm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfgtui) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GTUSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfges: l.sfges $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfges) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GESI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfgesi: l.sfgesi $rA,$simm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfgesi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GESI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfgeu: l.sfgeu $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfgeu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GEUSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfgeui: l.sfgeui $rA,$simm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfgeui) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = GEUSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sflts: l.sflts $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sflts) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LTSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfltsi: l.sfltsi $rA,$simm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfltsi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LTSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfltu: l.sfltu $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfltu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LTUSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfltui: l.sfltui $rA,$simm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfltui) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LTUSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfles: l.sfles $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfles) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LESI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sflesi: l.sflesi $rA,$simm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sflesi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LESI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfleu: l.sfleu $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfleu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LEUSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfleui: l.sfleui $rA,$simm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfleui) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = LEUSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfeq: l.sfeq $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfeq) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = EQSI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfeqi: l.sfeqi $rA,$simm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfeqi) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = EQSI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfne: l.sfne $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfne) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = NESI (GET_H_GPR (FLD (f_r2)), GET_H_GPR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-sfnei: l.sfnei $rA,$simm16 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_sfnei) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    USI opval = NESI (GET_H_GPR (FLD (f_r2)), EXTSISI (FLD (f_simm16)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* l-mac: l.mac $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_mac) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-maci: l.maci $rA,${simm16} */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_maci) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-macu: l.macu $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_macu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-msb: l.msb $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_msb) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-msbu: l.msbu $rA,$rB */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_msbu) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

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

  return vpc;
#undef FLD
}

/* l-cust1: l.cust1 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_cust1) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* l-cust2: l.cust2 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_cust2) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* l-cust3: l.cust3 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_cust3) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* l-cust4: l.cust4 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_cust4) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* l-cust5: l.cust5 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_cust5) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* l-cust6: l.cust6 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_cust6) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* l-cust7: l.cust7 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_cust7) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* l-cust8: l.cust8 */

static SEM_PC
SEM_FN_NAME (or1k32bf,l_cust8) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* lf-add-s: lf.add.s $rDSF,$rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_add_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SF opval = CGEN_CPU_FPU (current_cpu)->ops->addsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_FSR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fsr", 'f', opval);
  }

  return vpc;
#undef FLD
}

/* lf-add-d32: lf.add.d $rDD32F,$rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_add_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DF opval = CGEN_CPU_FPU (current_cpu)->ops->adddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_FD32R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fd32r", 'f', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sub-s: lf.sub.s $rDSF,$rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sub_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SF opval = CGEN_CPU_FPU (current_cpu)->ops->subsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_FSR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fsr", 'f', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sub-d32: lf.sub.d $rDD32F,$rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sub_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DF opval = CGEN_CPU_FPU (current_cpu)->ops->subdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_FD32R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fd32r", 'f', opval);
  }

  return vpc;
#undef FLD
}

/* lf-mul-s: lf.mul.s $rDSF,$rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_mul_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SF opval = CGEN_CPU_FPU (current_cpu)->ops->mulsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_FSR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fsr", 'f', opval);
  }

  return vpc;
#undef FLD
}

/* lf-mul-d32: lf.mul.d $rDD32F,$rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_mul_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DF opval = CGEN_CPU_FPU (current_cpu)->ops->muldf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_FD32R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fd32r", 'f', opval);
  }

  return vpc;
#undef FLD
}

/* lf-div-s: lf.div.s $rDSF,$rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_div_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SF opval = CGEN_CPU_FPU (current_cpu)->ops->divsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_FSR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fsr", 'f', opval);
  }

  return vpc;
#undef FLD
}

/* lf-div-d32: lf.div.d $rDD32F,$rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_div_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DF opval = CGEN_CPU_FPU (current_cpu)->ops->divdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_FD32R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fd32r", 'f', opval);
  }

  return vpc;
#undef FLD
}

/* lf-rem-s: lf.rem.s $rDSF,$rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_rem_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SF opval = CGEN_CPU_FPU (current_cpu)->ops->remsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_FSR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fsr", 'f', opval);
  }

  return vpc;
#undef FLD
}

/* lf-rem-d32: lf.rem.d $rDD32F,$rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_rem_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DF opval = CGEN_CPU_FPU (current_cpu)->ops->remdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_FD32R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fd32r", 'f', opval);
  }

  return vpc;
#undef FLD
}

/* lf-itof-s: lf.itof.s $rDSF,$rA */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_itof_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SF opval = CGEN_CPU_FPU (current_cpu)->ops->floatsisf (CGEN_CPU_FPU (current_cpu), (GET_H_SYS_FPCSR_RM () == 0) ? (1) : (GET_H_SYS_FPCSR_RM () == 1) ? (3) : (GET_H_SYS_FPCSR_RM () == 2) ? (4) : (5), TRUNCSISI (GET_H_GPR (FLD (f_r2))));
    SET_H_FSR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fsr", 'f', opval);
  }

  return vpc;
#undef FLD
}

/* lf-itof-d32: lf.itof.d $rDD32F,$rADI */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_itof_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DF opval = CGEN_CPU_FPU (current_cpu)->ops->floatdidf (CGEN_CPU_FPU (current_cpu), (GET_H_SYS_FPCSR_RM () == 0) ? (1) : (GET_H_SYS_FPCSR_RM () == 1) ? (3) : (GET_H_SYS_FPCSR_RM () == 2) ? (4) : (5), GET_H_I64R (FLD (f_rad32)));
    SET_H_FD32R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fd32r", 'f', opval);
  }

  return vpc;
#undef FLD
}

/* lf-ftoi-s: lf.ftoi.s $rD,$rASF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_ftoi_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SI opval = EXTSISI (CGEN_CPU_FPU (current_cpu)->ops->fixsfsi (CGEN_CPU_FPU (current_cpu), (GET_H_SYS_FPCSR_RM () == 0) ? (1) : (GET_H_SYS_FPCSR_RM () == 1) ? (3) : (GET_H_SYS_FPCSR_RM () == 2) ? (4) : (5), GET_H_FSR (FLD (f_r2))));
    SET_H_GPR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "gpr", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-ftoi-d32: lf.ftoi.d $rDDI,$rAD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_ftoi_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DI opval = CGEN_CPU_FPU (current_cpu)->ops->fixdfdi (CGEN_CPU_FPU (current_cpu), (GET_H_SYS_FPCSR_RM () == 0) ? (1) : (GET_H_SYS_FPCSR_RM () == 1) ? (3) : (GET_H_SYS_FPCSR_RM () == 2) ? (4) : (5), GET_H_FD32R (FLD (f_rad32)));
    SET_H_I64R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "i64r", 'D', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfeq-s: lf.sfeq.s $rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfeq_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->eqsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfeq-d32: lf.sfeq.d $rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfeq_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->eqdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfne-s: lf.sfne.s $rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfne_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->nesf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfne-d32: lf.sfne.d $rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfne_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->nedf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfge-s: lf.sfge.s $rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfge_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->gesf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfge-d32: lf.sfge.d $rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfge_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->gedf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfgt-s: lf.sfgt.s $rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfgt_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->gtsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfgt-d32: lf.sfgt.d $rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfgt_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->gtdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sflt-s: lf.sflt.s $rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sflt_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->ltsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sflt-d32: lf.sflt.d $rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sflt_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->ltdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfle-s: lf.sfle.s $rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfle_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->lesf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfle-d32: lf.sfle.d $rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfle_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->ledf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfueq-s: lf.sfueq.s $rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfueq_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unorderedsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))), CGEN_CPU_FPU (current_cpu)->ops->eqsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfueq-d32: lf.sfueq.d $rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfueq_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unordereddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))), CGEN_CPU_FPU (current_cpu)->ops->eqdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfune-s: lf.sfune.s $rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfune_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unorderedsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))), CGEN_CPU_FPU (current_cpu)->ops->nesf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfune-d32: lf.sfune.d $rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfune_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unordereddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))), CGEN_CPU_FPU (current_cpu)->ops->nedf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfugt-s: lf.sfugt.s $rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfugt_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unorderedsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))), CGEN_CPU_FPU (current_cpu)->ops->gtsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfugt-d32: lf.sfugt.d $rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfugt_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unordereddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))), CGEN_CPU_FPU (current_cpu)->ops->gtdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfuge-s: lf.sfuge.s $rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfuge_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unorderedsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))), CGEN_CPU_FPU (current_cpu)->ops->gesf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfuge-d32: lf.sfuge.d $rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfuge_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unordereddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))), CGEN_CPU_FPU (current_cpu)->ops->gedf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfult-s: lf.sfult.s $rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfult_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unorderedsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))), CGEN_CPU_FPU (current_cpu)->ops->ltsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfult-d32: lf.sfult.d $rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfult_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unordereddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))), CGEN_CPU_FPU (current_cpu)->ops->ltdf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfule-s: lf.sfule.s $rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfule_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unorderedsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))), CGEN_CPU_FPU (current_cpu)->ops->lesf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfule-d32: lf.sfule.d $rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfule_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = ORBI (CGEN_CPU_FPU (current_cpu)->ops->unordereddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))), CGEN_CPU_FPU (current_cpu)->ops->ledf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfun-s: lf.sfun.s $rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfun_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->unorderedsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-sfun-d32: lf.sfun.d $rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_sfun_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    BI opval = CGEN_CPU_FPU (current_cpu)->ops->unordereddf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32)));
    SET_H_SYS_SR_F (opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "sys-sr-f", 'x', opval);
  }

  return vpc;
#undef FLD
}

/* lf-madd-s: lf.madd.s $rDSF,$rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_madd_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    SF opval = CGEN_CPU_FPU (current_cpu)->ops->addsf (CGEN_CPU_FPU (current_cpu), CGEN_CPU_FPU (current_cpu)->ops->mulsf (CGEN_CPU_FPU (current_cpu), GET_H_FSR (FLD (f_r2)), GET_H_FSR (FLD (f_r3))), GET_H_FSR (FLD (f_r1)));
    SET_H_FSR (FLD (f_r1), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fsr", 'f', opval);
  }

  return vpc;
#undef FLD
}

/* lf-madd-d32: lf.madd.d $rDD32F,$rAD32F,$rBD32F */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_madd_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

  {
    DF opval = CGEN_CPU_FPU (current_cpu)->ops->adddf (CGEN_CPU_FPU (current_cpu), CGEN_CPU_FPU (current_cpu)->ops->muldf (CGEN_CPU_FPU (current_cpu), GET_H_FD32R (FLD (f_rad32)), GET_H_FD32R (FLD (f_rbd32))), GET_H_FD32R (FLD (f_rdd32)));
    SET_H_FD32R (FLD (f_rdd32), opval);
    CGEN_TRACE_RESULT (current_cpu, abuf, "fd32r", 'f', opval);
  }

  return vpc;
#undef FLD
}

/* lf-cust1-s: lf.cust1.s $rASF,$rBSF */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_cust1_s) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* lf-cust1-d32: lf.cust1.d */

static SEM_PC
SEM_FN_NAME (or1k32bf,lf_cust1_d32) (SIM_CPU *current_cpu, SEM_ARG sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  ARGBUF *abuf = SEM_ARGBUF (sem_arg);
  int UNUSED written = 0;
  IADDR UNUSED pc = abuf->addr;
  SEM_PC vpc = SEM_NEXT_VPC (sem_arg, pc, 4);

((void) 0); /*nop*/

  return vpc;
#undef FLD
}

/* Table of all semantic fns.  */

static const struct sem_fn_desc sem_fns[] = {
  { OR1K32BF_INSN_X_INVALID, SEM_FN_NAME (or1k32bf,x_invalid) },
  { OR1K32BF_INSN_X_AFTER, SEM_FN_NAME (or1k32bf,x_after) },
  { OR1K32BF_INSN_X_BEFORE, SEM_FN_NAME (or1k32bf,x_before) },
  { OR1K32BF_INSN_X_CTI_CHAIN, SEM_FN_NAME (or1k32bf,x_cti_chain) },
  { OR1K32BF_INSN_X_CHAIN, SEM_FN_NAME (or1k32bf,x_chain) },
  { OR1K32BF_INSN_X_BEGIN, SEM_FN_NAME (or1k32bf,x_begin) },
  { OR1K32BF_INSN_L_J, SEM_FN_NAME (or1k32bf,l_j) },
  { OR1K32BF_INSN_L_ADRP, SEM_FN_NAME (or1k32bf,l_adrp) },
  { OR1K32BF_INSN_L_JAL, SEM_FN_NAME (or1k32bf,l_jal) },
  { OR1K32BF_INSN_L_JR, SEM_FN_NAME (or1k32bf,l_jr) },
  { OR1K32BF_INSN_L_JALR, SEM_FN_NAME (or1k32bf,l_jalr) },
  { OR1K32BF_INSN_L_BNF, SEM_FN_NAME (or1k32bf,l_bnf) },
  { OR1K32BF_INSN_L_BF, SEM_FN_NAME (or1k32bf,l_bf) },
  { OR1K32BF_INSN_L_TRAP, SEM_FN_NAME (or1k32bf,l_trap) },
  { OR1K32BF_INSN_L_SYS, SEM_FN_NAME (or1k32bf,l_sys) },
  { OR1K32BF_INSN_L_MSYNC, SEM_FN_NAME (or1k32bf,l_msync) },
  { OR1K32BF_INSN_L_PSYNC, SEM_FN_NAME (or1k32bf,l_psync) },
  { OR1K32BF_INSN_L_CSYNC, SEM_FN_NAME (or1k32bf,l_csync) },
  { OR1K32BF_INSN_L_RFE, SEM_FN_NAME (or1k32bf,l_rfe) },
  { OR1K32BF_INSN_L_NOP_IMM, SEM_FN_NAME (or1k32bf,l_nop_imm) },
  { OR1K32BF_INSN_L_MOVHI, SEM_FN_NAME (or1k32bf,l_movhi) },
  { OR1K32BF_INSN_L_MACRC, SEM_FN_NAME (or1k32bf,l_macrc) },
  { OR1K32BF_INSN_L_MFSPR, SEM_FN_NAME (or1k32bf,l_mfspr) },
  { OR1K32BF_INSN_L_MTSPR, SEM_FN_NAME (or1k32bf,l_mtspr) },
  { OR1K32BF_INSN_L_LWZ, SEM_FN_NAME (or1k32bf,l_lwz) },
  { OR1K32BF_INSN_L_LWS, SEM_FN_NAME (or1k32bf,l_lws) },
  { OR1K32BF_INSN_L_LWA, SEM_FN_NAME (or1k32bf,l_lwa) },
  { OR1K32BF_INSN_L_LBZ, SEM_FN_NAME (or1k32bf,l_lbz) },
  { OR1K32BF_INSN_L_LBS, SEM_FN_NAME (or1k32bf,l_lbs) },
  { OR1K32BF_INSN_L_LHZ, SEM_FN_NAME (or1k32bf,l_lhz) },
  { OR1K32BF_INSN_L_LHS, SEM_FN_NAME (or1k32bf,l_lhs) },
  { OR1K32BF_INSN_L_SW, SEM_FN_NAME (or1k32bf,l_sw) },
  { OR1K32BF_INSN_L_SB, SEM_FN_NAME (or1k32bf,l_sb) },
  { OR1K32BF_INSN_L_SH, SEM_FN_NAME (or1k32bf,l_sh) },
  { OR1K32BF_INSN_L_SWA, SEM_FN_NAME (or1k32bf,l_swa) },
  { OR1K32BF_INSN_L_SLL, SEM_FN_NAME (or1k32bf,l_sll) },
  { OR1K32BF_INSN_L_SLLI, SEM_FN_NAME (or1k32bf,l_slli) },
  { OR1K32BF_INSN_L_SRL, SEM_FN_NAME (or1k32bf,l_srl) },
  { OR1K32BF_INSN_L_SRLI, SEM_FN_NAME (or1k32bf,l_srli) },
  { OR1K32BF_INSN_L_SRA, SEM_FN_NAME (or1k32bf,l_sra) },
  { OR1K32BF_INSN_L_SRAI, SEM_FN_NAME (or1k32bf,l_srai) },
  { OR1K32BF_INSN_L_ROR, SEM_FN_NAME (or1k32bf,l_ror) },
  { OR1K32BF_INSN_L_RORI, SEM_FN_NAME (or1k32bf,l_rori) },
  { OR1K32BF_INSN_L_AND, SEM_FN_NAME (or1k32bf,l_and) },
  { OR1K32BF_INSN_L_OR, SEM_FN_NAME (or1k32bf,l_or) },
  { OR1K32BF_INSN_L_XOR, SEM_FN_NAME (or1k32bf,l_xor) },
  { OR1K32BF_INSN_L_ADD, SEM_FN_NAME (or1k32bf,l_add) },
  { OR1K32BF_INSN_L_SUB, SEM_FN_NAME (or1k32bf,l_sub) },
  { OR1K32BF_INSN_L_ADDC, SEM_FN_NAME (or1k32bf,l_addc) },
  { OR1K32BF_INSN_L_MUL, SEM_FN_NAME (or1k32bf,l_mul) },
  { OR1K32BF_INSN_L_MULD, SEM_FN_NAME (or1k32bf,l_muld) },
  { OR1K32BF_INSN_L_MULU, SEM_FN_NAME (or1k32bf,l_mulu) },
  { OR1K32BF_INSN_L_MULDU, SEM_FN_NAME (or1k32bf,l_muldu) },
  { OR1K32BF_INSN_L_DIV, SEM_FN_NAME (or1k32bf,l_div) },
  { OR1K32BF_INSN_L_DIVU, SEM_FN_NAME (or1k32bf,l_divu) },
  { OR1K32BF_INSN_L_FF1, SEM_FN_NAME (or1k32bf,l_ff1) },
  { OR1K32BF_INSN_L_FL1, SEM_FN_NAME (or1k32bf,l_fl1) },
  { OR1K32BF_INSN_L_ANDI, SEM_FN_NAME (or1k32bf,l_andi) },
  { OR1K32BF_INSN_L_ORI, SEM_FN_NAME (or1k32bf,l_ori) },
  { OR1K32BF_INSN_L_XORI, SEM_FN_NAME (or1k32bf,l_xori) },
  { OR1K32BF_INSN_L_ADDI, SEM_FN_NAME (or1k32bf,l_addi) },
  { OR1K32BF_INSN_L_ADDIC, SEM_FN_NAME (or1k32bf,l_addic) },
  { OR1K32BF_INSN_L_MULI, SEM_FN_NAME (or1k32bf,l_muli) },
  { OR1K32BF_INSN_L_EXTHS, SEM_FN_NAME (or1k32bf,l_exths) },
  { OR1K32BF_INSN_L_EXTBS, SEM_FN_NAME (or1k32bf,l_extbs) },
  { OR1K32BF_INSN_L_EXTHZ, SEM_FN_NAME (or1k32bf,l_exthz) },
  { OR1K32BF_INSN_L_EXTBZ, SEM_FN_NAME (or1k32bf,l_extbz) },
  { OR1K32BF_INSN_L_EXTWS, SEM_FN_NAME (or1k32bf,l_extws) },
  { OR1K32BF_INSN_L_EXTWZ, SEM_FN_NAME (or1k32bf,l_extwz) },
  { OR1K32BF_INSN_L_CMOV, SEM_FN_NAME (or1k32bf,l_cmov) },
  { OR1K32BF_INSN_L_SFGTS, SEM_FN_NAME (or1k32bf,l_sfgts) },
  { OR1K32BF_INSN_L_SFGTSI, SEM_FN_NAME (or1k32bf,l_sfgtsi) },
  { OR1K32BF_INSN_L_SFGTU, SEM_FN_NAME (or1k32bf,l_sfgtu) },
  { OR1K32BF_INSN_L_SFGTUI, SEM_FN_NAME (or1k32bf,l_sfgtui) },
  { OR1K32BF_INSN_L_SFGES, SEM_FN_NAME (or1k32bf,l_sfges) },
  { OR1K32BF_INSN_L_SFGESI, SEM_FN_NAME (or1k32bf,l_sfgesi) },
  { OR1K32BF_INSN_L_SFGEU, SEM_FN_NAME (or1k32bf,l_sfgeu) },
  { OR1K32BF_INSN_L_SFGEUI, SEM_FN_NAME (or1k32bf,l_sfgeui) },
  { OR1K32BF_INSN_L_SFLTS, SEM_FN_NAME (or1k32bf,l_sflts) },
  { OR1K32BF_INSN_L_SFLTSI, SEM_FN_NAME (or1k32bf,l_sfltsi) },
  { OR1K32BF_INSN_L_SFLTU, SEM_FN_NAME (or1k32bf,l_sfltu) },
  { OR1K32BF_INSN_L_SFLTUI, SEM_FN_NAME (or1k32bf,l_sfltui) },
  { OR1K32BF_INSN_L_SFLES, SEM_FN_NAME (or1k32bf,l_sfles) },
  { OR1K32BF_INSN_L_SFLESI, SEM_FN_NAME (or1k32bf,l_sflesi) },
  { OR1K32BF_INSN_L_SFLEU, SEM_FN_NAME (or1k32bf,l_sfleu) },
  { OR1K32BF_INSN_L_SFLEUI, SEM_FN_NAME (or1k32bf,l_sfleui) },
  { OR1K32BF_INSN_L_SFEQ, SEM_FN_NAME (or1k32bf,l_sfeq) },
  { OR1K32BF_INSN_L_SFEQI, SEM_FN_NAME (or1k32bf,l_sfeqi) },
  { OR1K32BF_INSN_L_SFNE, SEM_FN_NAME (or1k32bf,l_sfne) },
  { OR1K32BF_INSN_L_SFNEI, SEM_FN_NAME (or1k32bf,l_sfnei) },
  { OR1K32BF_INSN_L_MAC, SEM_FN_NAME (or1k32bf,l_mac) },
  { OR1K32BF_INSN_L_MACI, SEM_FN_NAME (or1k32bf,l_maci) },
  { OR1K32BF_INSN_L_MACU, SEM_FN_NAME (or1k32bf,l_macu) },
  { OR1K32BF_INSN_L_MSB, SEM_FN_NAME (or1k32bf,l_msb) },
  { OR1K32BF_INSN_L_MSBU, SEM_FN_NAME (or1k32bf,l_msbu) },
  { OR1K32BF_INSN_L_CUST1, SEM_FN_NAME (or1k32bf,l_cust1) },
  { OR1K32BF_INSN_L_CUST2, SEM_FN_NAME (or1k32bf,l_cust2) },
  { OR1K32BF_INSN_L_CUST3, SEM_FN_NAME (or1k32bf,l_cust3) },
  { OR1K32BF_INSN_L_CUST4, SEM_FN_NAME (or1k32bf,l_cust4) },
  { OR1K32BF_INSN_L_CUST5, SEM_FN_NAME (or1k32bf,l_cust5) },
  { OR1K32BF_INSN_L_CUST6, SEM_FN_NAME (or1k32bf,l_cust6) },
  { OR1K32BF_INSN_L_CUST7, SEM_FN_NAME (or1k32bf,l_cust7) },
  { OR1K32BF_INSN_L_CUST8, SEM_FN_NAME (or1k32bf,l_cust8) },
  { OR1K32BF_INSN_LF_ADD_S, SEM_FN_NAME (or1k32bf,lf_add_s) },
  { OR1K32BF_INSN_LF_ADD_D32, SEM_FN_NAME (or1k32bf,lf_add_d32) },
  { OR1K32BF_INSN_LF_SUB_S, SEM_FN_NAME (or1k32bf,lf_sub_s) },
  { OR1K32BF_INSN_LF_SUB_D32, SEM_FN_NAME (or1k32bf,lf_sub_d32) },
  { OR1K32BF_INSN_LF_MUL_S, SEM_FN_NAME (or1k32bf,lf_mul_s) },
  { OR1K32BF_INSN_LF_MUL_D32, SEM_FN_NAME (or1k32bf,lf_mul_d32) },
  { OR1K32BF_INSN_LF_DIV_S, SEM_FN_NAME (or1k32bf,lf_div_s) },
  { OR1K32BF_INSN_LF_DIV_D32, SEM_FN_NAME (or1k32bf,lf_div_d32) },
  { OR1K32BF_INSN_LF_REM_S, SEM_FN_NAME (or1k32bf,lf_rem_s) },
  { OR1K32BF_INSN_LF_REM_D32, SEM_FN_NAME (or1k32bf,lf_rem_d32) },
  { OR1K32BF_INSN_LF_ITOF_S, SEM_FN_NAME (or1k32bf,lf_itof_s) },
  { OR1K32BF_INSN_LF_ITOF_D32, SEM_FN_NAME (or1k32bf,lf_itof_d32) },
  { OR1K32BF_INSN_LF_FTOI_S, SEM_FN_NAME (or1k32bf,lf_ftoi_s) },
  { OR1K32BF_INSN_LF_FTOI_D32, SEM_FN_NAME (or1k32bf,lf_ftoi_d32) },
  { OR1K32BF_INSN_LF_SFEQ_S, SEM_FN_NAME (or1k32bf,lf_sfeq_s) },
  { OR1K32BF_INSN_LF_SFEQ_D32, SEM_FN_NAME (or1k32bf,lf_sfeq_d32) },
  { OR1K32BF_INSN_LF_SFNE_S, SEM_FN_NAME (or1k32bf,lf_sfne_s) },
  { OR1K32BF_INSN_LF_SFNE_D32, SEM_FN_NAME (or1k32bf,lf_sfne_d32) },
  { OR1K32BF_INSN_LF_SFGE_S, SEM_FN_NAME (or1k32bf,lf_sfge_s) },
  { OR1K32BF_INSN_LF_SFGE_D32, SEM_FN_NAME (or1k32bf,lf_sfge_d32) },
  { OR1K32BF_INSN_LF_SFGT_S, SEM_FN_NAME (or1k32bf,lf_sfgt_s) },
  { OR1K32BF_INSN_LF_SFGT_D32, SEM_FN_NAME (or1k32bf,lf_sfgt_d32) },
  { OR1K32BF_INSN_LF_SFLT_S, SEM_FN_NAME (or1k32bf,lf_sflt_s) },
  { OR1K32BF_INSN_LF_SFLT_D32, SEM_FN_NAME (or1k32bf,lf_sflt_d32) },
  { OR1K32BF_INSN_LF_SFLE_S, SEM_FN_NAME (or1k32bf,lf_sfle_s) },
  { OR1K32BF_INSN_LF_SFLE_D32, SEM_FN_NAME (or1k32bf,lf_sfle_d32) },
  { OR1K32BF_INSN_LF_SFUEQ_S, SEM_FN_NAME (or1k32bf,lf_sfueq_s) },
  { OR1K32BF_INSN_LF_SFUEQ_D32, SEM_FN_NAME (or1k32bf,lf_sfueq_d32) },
  { OR1K32BF_INSN_LF_SFUNE_S, SEM_FN_NAME (or1k32bf,lf_sfune_s) },
  { OR1K32BF_INSN_LF_SFUNE_D32, SEM_FN_NAME (or1k32bf,lf_sfune_d32) },
  { OR1K32BF_INSN_LF_SFUGT_S, SEM_FN_NAME (or1k32bf,lf_sfugt_s) },
  { OR1K32BF_INSN_LF_SFUGT_D32, SEM_FN_NAME (or1k32bf,lf_sfugt_d32) },
  { OR1K32BF_INSN_LF_SFUGE_S, SEM_FN_NAME (or1k32bf,lf_sfuge_s) },
  { OR1K32BF_INSN_LF_SFUGE_D32, SEM_FN_NAME (or1k32bf,lf_sfuge_d32) },
  { OR1K32BF_INSN_LF_SFULT_S, SEM_FN_NAME (or1k32bf,lf_sfult_s) },
  { OR1K32BF_INSN_LF_SFULT_D32, SEM_FN_NAME (or1k32bf,lf_sfult_d32) },
  { OR1K32BF_INSN_LF_SFULE_S, SEM_FN_NAME (or1k32bf,lf_sfule_s) },
  { OR1K32BF_INSN_LF_SFULE_D32, SEM_FN_NAME (or1k32bf,lf_sfule_d32) },
  { OR1K32BF_INSN_LF_SFUN_S, SEM_FN_NAME (or1k32bf,lf_sfun_s) },
  { OR1K32BF_INSN_LF_SFUN_D32, SEM_FN_NAME (or1k32bf,lf_sfun_d32) },
  { OR1K32BF_INSN_LF_MADD_S, SEM_FN_NAME (or1k32bf,lf_madd_s) },
  { OR1K32BF_INSN_LF_MADD_D32, SEM_FN_NAME (or1k32bf,lf_madd_d32) },
  { OR1K32BF_INSN_LF_CUST1_S, SEM_FN_NAME (or1k32bf,lf_cust1_s) },
  { OR1K32BF_INSN_LF_CUST1_D32, SEM_FN_NAME (or1k32bf,lf_cust1_d32) },
  { 0, 0 }
};

/* Add the semantic fns to IDESC_TABLE.  */

void
SEM_FN_NAME (or1k32bf,init_idesc_table) (SIM_CPU *current_cpu)
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
	idesc_table[sf->index].sem_fast = SEM_FN_NAME (or1k32bf,x_invalid);
#else
      if (valid_p)
	idesc_table[sf->index].sem_full = sf->fn;
      else
	idesc_table[sf->index].sem_full = SEM_FN_NAME (or1k32bf,x_invalid);
#endif
    }
}

