/* Simulator model support for or1k32bf.

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

/* The profiling data is recorded here, but is accessed via the profiling
   mechanism.  After all, this is information for profiling.  */

#if WITH_PROFILE_MODEL_P

/* Model handlers for each insn.  */

static int
model_or1200_l_j (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_j.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_adrp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_adrp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_jal (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_j.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_jr (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_jalr (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_bnf (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_j.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_bf (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_j.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_trap (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sys (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_msync (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_psync (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_csync (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_rfe (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_nop_imm (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_movhi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_macrc (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_adrp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_mfspr (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_mtspr (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mtspr.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_lwz (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_lws (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_lwa (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_lbz (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_lbs (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_lhz (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_lhs (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sw (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sw.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sb (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sw.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sh (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sw.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_swa (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sw.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sll (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_slli (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_srl (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_srli (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sra (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_srai (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_ror (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_rori (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_and (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_or (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_xor (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_add (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sub (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_addc (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_mul (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_muld (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_mulu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_muldu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_div (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_divu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_ff1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_fl1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_andi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_ori (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_xori (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_addi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_addic (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_muli (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_exths (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_extbs (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_exthz (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_extbz (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_extws (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_extwz (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_cmov (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfgts (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfgtsi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfgtu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfgtui (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfges (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfgesi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfgeu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfgeui (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sflts (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfltsi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfltu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfltui (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfles (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sflesi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfleu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfleui (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfeq (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfeqi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfne (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_sfnei (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_mac (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_maci (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_macu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_msb (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_msbu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_cust1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_cust2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_cust3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_cust4 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_cust5 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_cust6 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_cust7 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_l_cust8 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_add_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_add_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sub_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sub_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_mul_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_mul_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_div_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_div_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_rem_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_rem_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_itof_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_itof_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_ftoi_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_ftoi_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfeq_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfeq_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfne_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfne_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfge_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfge_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfgt_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfgt_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sflt_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sflt_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfle_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfle_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfueq_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfueq_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfune_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfune_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfugt_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfugt_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfuge_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfuge_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfult_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfult_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfule_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfule_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfun_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_sfun_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_madd_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_madd_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_cust1_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200_lf_cust1_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_j (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_j.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_adrp (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_adrp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_jal (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_j.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_jr (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_jalr (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_bnf (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_j.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_bf (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_j.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_trap (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sys (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_msync (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_psync (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_csync (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_rfe (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_nop_imm (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_movhi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_macrc (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_adrp.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_mfspr (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_mtspr (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mtspr.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_lwz (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_lws (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_lwa (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_lbz (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_lbs (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_lhz (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_lhs (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sw (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sw.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sb (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sw.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sh (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sw.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_swa (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sw.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sll (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_slli (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_srl (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_srli (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sra (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_srai (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_ror (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_rori (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_and (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_or (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_xor (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_add (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sub (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_addc (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_mul (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_muld (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_mulu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_muldu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_div (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_divu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_ff1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_fl1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_andi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_ori (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_xori (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_addi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_addic (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_muli (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_exths (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_extbs (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_exthz (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_extbz (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_extws (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_extwz (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_cmov (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfgts (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfgtsi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfgtu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfgtui (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfges (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfgesi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfgeu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfgeui (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sflts (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfltsi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfltu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfltui (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfles (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sflesi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfleu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfleui (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfeq (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfeqi (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfne (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_sfnei (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_mac (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_maci (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_lwz.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_macu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_msb (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_msbu (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_cust1 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_cust2 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_cust3 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_cust4 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_cust5 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_cust6 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_cust7 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_l_cust8 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_add_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_add_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sub_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sub_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_mul_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_mul_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_div_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_div_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_rem_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_rem_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_itof_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_itof_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_ftoi_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_slli.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_ftoi_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfeq_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfeq_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfne_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfne_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfge_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfge_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfgt_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfgt_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sflt_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sflt_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfle_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfle_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfueq_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfueq_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfune_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfune_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfugt_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfugt_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfuge_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfuge_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfult_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfult_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfule_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfule_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfun_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_sfun_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_madd_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_l_sll.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_madd_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_cust1_s (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

static int
model_or1200nd_lf_cust1_d32 (SIM_CPU *current_cpu, void *sem_arg)
{
#define FLD(f) abuf->fields.sfmt_empty.f
  const ARGBUF * UNUSED abuf = SEM_ARGBUF ((SEM_ARG) sem_arg);
  const IDESC * UNUSED idesc = abuf->idesc;
  int cycles = 0;
  {
    int referenced = 0;
    int UNUSED insn_referenced = abuf->written;
    cycles += or1k32bf_model_or1200nd_u_exec (current_cpu, idesc, 0, referenced);
  }
  return cycles;
#undef FLD
}

/* We assume UNIT_NONE == 0 because the tables don't always terminate
   entries with it.  */

/* Model timing data for `or1200'.  */

static const INSN_TIMING or1200_timing[] = {
  { OR1K32BF_INSN_X_INVALID, 0, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_X_AFTER, 0, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_X_BEFORE, 0, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_X_CTI_CHAIN, 0, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_X_CHAIN, 0, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_X_BEGIN, 0, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_J, model_or1200_l_j, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ADRP, model_or1200_l_adrp, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_JAL, model_or1200_l_jal, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_JR, model_or1200_l_jr, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_JALR, model_or1200_l_jalr, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_BNF, model_or1200_l_bnf, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_BF, model_or1200_l_bf, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_TRAP, model_or1200_l_trap, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SYS, model_or1200_l_sys, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MSYNC, model_or1200_l_msync, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_PSYNC, model_or1200_l_psync, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CSYNC, model_or1200_l_csync, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_RFE, model_or1200_l_rfe, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_NOP_IMM, model_or1200_l_nop_imm, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MOVHI, model_or1200_l_movhi, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MACRC, model_or1200_l_macrc, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MFSPR, model_or1200_l_mfspr, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MTSPR, model_or1200_l_mtspr, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_LWZ, model_or1200_l_lwz, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_LWS, model_or1200_l_lws, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_LWA, model_or1200_l_lwa, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_LBZ, model_or1200_l_lbz, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_LBS, model_or1200_l_lbs, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_LHZ, model_or1200_l_lhz, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_LHS, model_or1200_l_lhs, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SW, model_or1200_l_sw, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SB, model_or1200_l_sb, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SH, model_or1200_l_sh, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SWA, model_or1200_l_swa, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SLL, model_or1200_l_sll, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SLLI, model_or1200_l_slli, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SRL, model_or1200_l_srl, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SRLI, model_or1200_l_srli, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SRA, model_or1200_l_sra, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SRAI, model_or1200_l_srai, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ROR, model_or1200_l_ror, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_RORI, model_or1200_l_rori, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_AND, model_or1200_l_and, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_OR, model_or1200_l_or, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_XOR, model_or1200_l_xor, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ADD, model_or1200_l_add, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SUB, model_or1200_l_sub, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ADDC, model_or1200_l_addc, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MUL, model_or1200_l_mul, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MULD, model_or1200_l_muld, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MULU, model_or1200_l_mulu, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MULDU, model_or1200_l_muldu, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_DIV, model_or1200_l_div, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_DIVU, model_or1200_l_divu, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_FF1, model_or1200_l_ff1, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_FL1, model_or1200_l_fl1, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ANDI, model_or1200_l_andi, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ORI, model_or1200_l_ori, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_XORI, model_or1200_l_xori, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ADDI, model_or1200_l_addi, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ADDIC, model_or1200_l_addic, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MULI, model_or1200_l_muli, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_EXTHS, model_or1200_l_exths, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_EXTBS, model_or1200_l_extbs, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_EXTHZ, model_or1200_l_exthz, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_EXTBZ, model_or1200_l_extbz, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_EXTWS, model_or1200_l_extws, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_EXTWZ, model_or1200_l_extwz, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CMOV, model_or1200_l_cmov, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGTS, model_or1200_l_sfgts, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGTSI, model_or1200_l_sfgtsi, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGTU, model_or1200_l_sfgtu, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGTUI, model_or1200_l_sfgtui, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGES, model_or1200_l_sfges, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGESI, model_or1200_l_sfgesi, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGEU, model_or1200_l_sfgeu, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGEUI, model_or1200_l_sfgeui, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLTS, model_or1200_l_sflts, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLTSI, model_or1200_l_sfltsi, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLTU, model_or1200_l_sfltu, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLTUI, model_or1200_l_sfltui, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLES, model_or1200_l_sfles, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLESI, model_or1200_l_sflesi, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLEU, model_or1200_l_sfleu, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLEUI, model_or1200_l_sfleui, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFEQ, model_or1200_l_sfeq, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFEQI, model_or1200_l_sfeqi, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFNE, model_or1200_l_sfne, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFNEI, model_or1200_l_sfnei, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MAC, model_or1200_l_mac, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MACI, model_or1200_l_maci, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MACU, model_or1200_l_macu, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MSB, model_or1200_l_msb, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MSBU, model_or1200_l_msbu, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST1, model_or1200_l_cust1, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST2, model_or1200_l_cust2, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST3, model_or1200_l_cust3, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST4, model_or1200_l_cust4, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST5, model_or1200_l_cust5, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST6, model_or1200_l_cust6, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST7, model_or1200_l_cust7, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST8, model_or1200_l_cust8, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_ADD_S, model_or1200_lf_add_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_ADD_D32, model_or1200_lf_add_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SUB_S, model_or1200_lf_sub_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SUB_D32, model_or1200_lf_sub_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_MUL_S, model_or1200_lf_mul_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_MUL_D32, model_or1200_lf_mul_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_DIV_S, model_or1200_lf_div_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_DIV_D32, model_or1200_lf_div_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_REM_S, model_or1200_lf_rem_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_REM_D32, model_or1200_lf_rem_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_ITOF_S, model_or1200_lf_itof_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_ITOF_D32, model_or1200_lf_itof_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_FTOI_S, model_or1200_lf_ftoi_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_FTOI_D32, model_or1200_lf_ftoi_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFEQ_S, model_or1200_lf_sfeq_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFEQ_D32, model_or1200_lf_sfeq_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFNE_S, model_or1200_lf_sfne_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFNE_D32, model_or1200_lf_sfne_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFGE_S, model_or1200_lf_sfge_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFGE_D32, model_or1200_lf_sfge_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFGT_S, model_or1200_lf_sfgt_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFGT_D32, model_or1200_lf_sfgt_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFLT_S, model_or1200_lf_sflt_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFLT_D32, model_or1200_lf_sflt_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFLE_S, model_or1200_lf_sfle_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFLE_D32, model_or1200_lf_sfle_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUEQ_S, model_or1200_lf_sfueq_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUEQ_D32, model_or1200_lf_sfueq_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUNE_S, model_or1200_lf_sfune_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUNE_D32, model_or1200_lf_sfune_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUGT_S, model_or1200_lf_sfugt_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUGT_D32, model_or1200_lf_sfugt_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUGE_S, model_or1200_lf_sfuge_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUGE_D32, model_or1200_lf_sfuge_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFULT_S, model_or1200_lf_sfult_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFULT_D32, model_or1200_lf_sfult_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFULE_S, model_or1200_lf_sfule_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFULE_D32, model_or1200_lf_sfule_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUN_S, model_or1200_lf_sfun_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUN_D32, model_or1200_lf_sfun_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_MADD_S, model_or1200_lf_madd_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_MADD_D32, model_or1200_lf_madd_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_CUST1_S, model_or1200_lf_cust1_s, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_CUST1_D32, model_or1200_lf_cust1_d32, { { (int) UNIT_OR1200_U_EXEC, 1, 1 } } },
};

/* Model timing data for `or1200nd'.  */

static const INSN_TIMING or1200nd_timing[] = {
  { OR1K32BF_INSN_X_INVALID, 0, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_X_AFTER, 0, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_X_BEFORE, 0, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_X_CTI_CHAIN, 0, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_X_CHAIN, 0, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_X_BEGIN, 0, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_J, model_or1200nd_l_j, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ADRP, model_or1200nd_l_adrp, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_JAL, model_or1200nd_l_jal, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_JR, model_or1200nd_l_jr, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_JALR, model_or1200nd_l_jalr, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_BNF, model_or1200nd_l_bnf, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_BF, model_or1200nd_l_bf, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_TRAP, model_or1200nd_l_trap, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SYS, model_or1200nd_l_sys, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MSYNC, model_or1200nd_l_msync, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_PSYNC, model_or1200nd_l_psync, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CSYNC, model_or1200nd_l_csync, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_RFE, model_or1200nd_l_rfe, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_NOP_IMM, model_or1200nd_l_nop_imm, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MOVHI, model_or1200nd_l_movhi, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MACRC, model_or1200nd_l_macrc, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MFSPR, model_or1200nd_l_mfspr, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MTSPR, model_or1200nd_l_mtspr, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_LWZ, model_or1200nd_l_lwz, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_LWS, model_or1200nd_l_lws, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_LWA, model_or1200nd_l_lwa, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_LBZ, model_or1200nd_l_lbz, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_LBS, model_or1200nd_l_lbs, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_LHZ, model_or1200nd_l_lhz, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_LHS, model_or1200nd_l_lhs, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SW, model_or1200nd_l_sw, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SB, model_or1200nd_l_sb, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SH, model_or1200nd_l_sh, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SWA, model_or1200nd_l_swa, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SLL, model_or1200nd_l_sll, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SLLI, model_or1200nd_l_slli, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SRL, model_or1200nd_l_srl, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SRLI, model_or1200nd_l_srli, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SRA, model_or1200nd_l_sra, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SRAI, model_or1200nd_l_srai, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ROR, model_or1200nd_l_ror, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_RORI, model_or1200nd_l_rori, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_AND, model_or1200nd_l_and, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_OR, model_or1200nd_l_or, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_XOR, model_or1200nd_l_xor, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ADD, model_or1200nd_l_add, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SUB, model_or1200nd_l_sub, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ADDC, model_or1200nd_l_addc, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MUL, model_or1200nd_l_mul, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MULD, model_or1200nd_l_muld, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MULU, model_or1200nd_l_mulu, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MULDU, model_or1200nd_l_muldu, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_DIV, model_or1200nd_l_div, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_DIVU, model_or1200nd_l_divu, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_FF1, model_or1200nd_l_ff1, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_FL1, model_or1200nd_l_fl1, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ANDI, model_or1200nd_l_andi, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ORI, model_or1200nd_l_ori, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_XORI, model_or1200nd_l_xori, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ADDI, model_or1200nd_l_addi, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_ADDIC, model_or1200nd_l_addic, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MULI, model_or1200nd_l_muli, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_EXTHS, model_or1200nd_l_exths, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_EXTBS, model_or1200nd_l_extbs, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_EXTHZ, model_or1200nd_l_exthz, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_EXTBZ, model_or1200nd_l_extbz, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_EXTWS, model_or1200nd_l_extws, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_EXTWZ, model_or1200nd_l_extwz, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CMOV, model_or1200nd_l_cmov, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGTS, model_or1200nd_l_sfgts, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGTSI, model_or1200nd_l_sfgtsi, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGTU, model_or1200nd_l_sfgtu, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGTUI, model_or1200nd_l_sfgtui, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGES, model_or1200nd_l_sfges, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGESI, model_or1200nd_l_sfgesi, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGEU, model_or1200nd_l_sfgeu, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFGEUI, model_or1200nd_l_sfgeui, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLTS, model_or1200nd_l_sflts, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLTSI, model_or1200nd_l_sfltsi, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLTU, model_or1200nd_l_sfltu, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLTUI, model_or1200nd_l_sfltui, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLES, model_or1200nd_l_sfles, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLESI, model_or1200nd_l_sflesi, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLEU, model_or1200nd_l_sfleu, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFLEUI, model_or1200nd_l_sfleui, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFEQ, model_or1200nd_l_sfeq, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFEQI, model_or1200nd_l_sfeqi, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFNE, model_or1200nd_l_sfne, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_SFNEI, model_or1200nd_l_sfnei, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MAC, model_or1200nd_l_mac, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MACI, model_or1200nd_l_maci, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MACU, model_or1200nd_l_macu, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MSB, model_or1200nd_l_msb, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_MSBU, model_or1200nd_l_msbu, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST1, model_or1200nd_l_cust1, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST2, model_or1200nd_l_cust2, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST3, model_or1200nd_l_cust3, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST4, model_or1200nd_l_cust4, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST5, model_or1200nd_l_cust5, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST6, model_or1200nd_l_cust6, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST7, model_or1200nd_l_cust7, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_L_CUST8, model_or1200nd_l_cust8, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_ADD_S, model_or1200nd_lf_add_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_ADD_D32, model_or1200nd_lf_add_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SUB_S, model_or1200nd_lf_sub_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SUB_D32, model_or1200nd_lf_sub_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_MUL_S, model_or1200nd_lf_mul_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_MUL_D32, model_or1200nd_lf_mul_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_DIV_S, model_or1200nd_lf_div_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_DIV_D32, model_or1200nd_lf_div_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_REM_S, model_or1200nd_lf_rem_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_REM_D32, model_or1200nd_lf_rem_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_ITOF_S, model_or1200nd_lf_itof_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_ITOF_D32, model_or1200nd_lf_itof_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_FTOI_S, model_or1200nd_lf_ftoi_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_FTOI_D32, model_or1200nd_lf_ftoi_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFEQ_S, model_or1200nd_lf_sfeq_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFEQ_D32, model_or1200nd_lf_sfeq_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFNE_S, model_or1200nd_lf_sfne_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFNE_D32, model_or1200nd_lf_sfne_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFGE_S, model_or1200nd_lf_sfge_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFGE_D32, model_or1200nd_lf_sfge_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFGT_S, model_or1200nd_lf_sfgt_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFGT_D32, model_or1200nd_lf_sfgt_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFLT_S, model_or1200nd_lf_sflt_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFLT_D32, model_or1200nd_lf_sflt_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFLE_S, model_or1200nd_lf_sfle_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFLE_D32, model_or1200nd_lf_sfle_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUEQ_S, model_or1200nd_lf_sfueq_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUEQ_D32, model_or1200nd_lf_sfueq_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUNE_S, model_or1200nd_lf_sfune_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUNE_D32, model_or1200nd_lf_sfune_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUGT_S, model_or1200nd_lf_sfugt_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUGT_D32, model_or1200nd_lf_sfugt_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUGE_S, model_or1200nd_lf_sfuge_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUGE_D32, model_or1200nd_lf_sfuge_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFULT_S, model_or1200nd_lf_sfult_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFULT_D32, model_or1200nd_lf_sfult_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFULE_S, model_or1200nd_lf_sfule_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFULE_D32, model_or1200nd_lf_sfule_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUN_S, model_or1200nd_lf_sfun_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_SFUN_D32, model_or1200nd_lf_sfun_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_MADD_S, model_or1200nd_lf_madd_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_MADD_D32, model_or1200nd_lf_madd_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_CUST1_S, model_or1200nd_lf_cust1_s, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
  { OR1K32BF_INSN_LF_CUST1_D32, model_or1200nd_lf_cust1_d32, { { (int) UNIT_OR1200ND_U_EXEC, 1, 1 } } },
};

#endif /* WITH_PROFILE_MODEL_P */

static void
or1200_model_init (SIM_CPU *cpu)
{
  CPU_MODEL_DATA (cpu) = (void *) zalloc (sizeof (MODEL_OR1200_DATA));
}

static void
or1200nd_model_init (SIM_CPU *cpu)
{
  CPU_MODEL_DATA (cpu) = (void *) zalloc (sizeof (MODEL_OR1200ND_DATA));
}

#if WITH_PROFILE_MODEL_P
#define TIMING_DATA(td) td
#else
#define TIMING_DATA(td) 0
#endif

static const SIM_MODEL or32_models[] =
{
  { "or1200", & or32_mach, MODEL_OR1200, TIMING_DATA (& or1200_timing[0]), or1200_model_init },
  { 0 }
};

static const SIM_MODEL or32nd_models[] =
{
  { "or1200nd", & or32nd_mach, MODEL_OR1200ND, TIMING_DATA (& or1200nd_timing[0]), or1200nd_model_init },
  { 0 }
};

/* The properties of this cpu's implementation.  */

static const SIM_MACH_IMP_PROPERTIES or1k32bf_imp_properties =
{
  sizeof (SIM_CPU),
#if WITH_SCACHE
  sizeof (SCACHE)
#else
  0
#endif
};


static void
or1k32bf_prepare_run (SIM_CPU *cpu)
{
  if (CPU_IDESC (cpu) == NULL)
    or1k32bf_init_idesc_table (cpu);
}

static const CGEN_INSN *
or1k32bf_get_idata (SIM_CPU *cpu, int inum)
{
  return CPU_IDESC (cpu) [inum].idata;
}

static void
or32_init_cpu (SIM_CPU *cpu)
{
  CPU_REG_FETCH (cpu) = or1k32bf_fetch_register;
  CPU_REG_STORE (cpu) = or1k32bf_store_register;
  CPU_PC_FETCH (cpu) = or1k32bf_h_pc_get;
  CPU_PC_STORE (cpu) = or1k32bf_h_pc_set;
  CPU_GET_IDATA (cpu) = or1k32bf_get_idata;
  CPU_MAX_INSNS (cpu) = OR1K32BF_INSN__MAX;
  CPU_INSN_NAME (cpu) = cgen_insn_name;
  CPU_FULL_ENGINE_FN (cpu) = or1k32bf_engine_run_full;
#if WITH_FAST
  CPU_FAST_ENGINE_FN (cpu) = or1k32bf_engine_run_fast;
#else
  CPU_FAST_ENGINE_FN (cpu) = or1k32bf_engine_run_full;
#endif
}

const SIM_MACH or32_mach =
{
  "or32", "or1k", MACH_OR32,
  32, 32, & or32_models[0], & or1k32bf_imp_properties,
  or32_init_cpu,
  or1k32bf_prepare_run
};

static void
or32nd_init_cpu (SIM_CPU *cpu)
{
  CPU_REG_FETCH (cpu) = or1k32bf_fetch_register;
  CPU_REG_STORE (cpu) = or1k32bf_store_register;
  CPU_PC_FETCH (cpu) = or1k32bf_h_pc_get;
  CPU_PC_STORE (cpu) = or1k32bf_h_pc_set;
  CPU_GET_IDATA (cpu) = or1k32bf_get_idata;
  CPU_MAX_INSNS (cpu) = OR1K32BF_INSN__MAX;
  CPU_INSN_NAME (cpu) = cgen_insn_name;
  CPU_FULL_ENGINE_FN (cpu) = or1k32bf_engine_run_full;
#if WITH_FAST
  CPU_FAST_ENGINE_FN (cpu) = or1k32bf_engine_run_fast;
#else
  CPU_FAST_ENGINE_FN (cpu) = or1k32bf_engine_run_full;
#endif
}

const SIM_MACH or32nd_mach =
{
  "or32nd", "or1knd", MACH_OR32ND,
  32, 32, & or32nd_models[0], & or1k32bf_imp_properties,
  or32nd_init_cpu,
  or1k32bf_prepare_run
};

