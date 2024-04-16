/* CPU family header for lm32bf.

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

#ifndef CPU_LM32BF_H
#define CPU_LM32BF_H

/* Maximum number of instructions that are fetched at a time.
   This is for LIW type instructions sets (e.g. m32r).  */
#define MAX_LIW_INSNS 1

/* Maximum number of instructions that can be executed in parallel.  */
#define MAX_PARALLEL_INSNS 1

/* The size of an "int" needed to hold an instruction word.
   This is usually 32 bits, but some architectures needs 64 bits.  */
typedef CGEN_INSN_INT CGEN_INSN_WORD;

#include "cgen-engine.h"

/* CPU state information.  */
typedef struct {
  /* Hardware elements.  */
  struct {
  /* Program counter */
  USI h_pc;
#define GET_H_PC() CPU (h_pc)
#define SET_H_PC(x) (CPU (h_pc) = (x))
  /* General purpose registers */
  SI h_gr[32];
#define GET_H_GR(a1) CPU (h_gr)[a1]
#define SET_H_GR(a1, x) (CPU (h_gr)[a1] = (x))
  /* Control and status registers */
  SI h_csr[32];
#define GET_H_CSR(a1) CPU (h_csr)[a1]
#define SET_H_CSR(a1, x) (CPU (h_csr)[a1] = (x))
  } hardware;
#define CPU_CGEN_HW(cpu) (& LM32_SIM_CPU (cpu)->cpu_data.hardware)
} LM32BF_CPU_DATA;

/* Cover fns for register access.  */
USI lm32bf_h_pc_get (SIM_CPU *);
void lm32bf_h_pc_set (SIM_CPU *, USI);
SI lm32bf_h_gr_get (SIM_CPU *, UINT);
void lm32bf_h_gr_set (SIM_CPU *, UINT, SI);
SI lm32bf_h_csr_get (SIM_CPU *, UINT);
void lm32bf_h_csr_set (SIM_CPU *, UINT, SI);

/* These must be hand-written.  */
extern CPUREG_FETCH_FN lm32bf_fetch_register;
extern CPUREG_STORE_FN lm32bf_store_register;

typedef struct {
  int empty;
} MODEL_LM32_DATA;

/* Instruction argument buffer.  */

union sem_fields {
  struct { /* no operands */
    int empty;
  } sfmt_empty;
  struct { /*  */
    IADDR i_call;
  } sfmt_bi;
  struct { /*  */
    UINT f_csr;
    UINT f_r1;
  } sfmt_wcsr;
  struct { /*  */
    UINT f_csr;
    UINT f_r2;
  } sfmt_rcsr;
  struct { /*  */
    IADDR i_branch;
    UINT f_r0;
    UINT f_r1;
  } sfmt_be;
  struct { /*  */
    UINT f_r0;
    UINT f_r1;
    UINT f_uimm;
  } sfmt_andi;
  struct { /*  */
    INT f_imm;
    UINT f_r0;
    UINT f_r1;
  } sfmt_addi;
  struct { /*  */
    UINT f_r0;
    UINT f_r1;
    UINT f_r2;
    UINT f_user;
  } sfmt_user;
#if WITH_SCACHE_PBB
  /* Writeback handler.  */
  struct {
    /* Pointer to argbuf entry for insn whose results need writing back.  */
    const struct argbuf *abuf;
  } write;
  /* x-before handler */
  struct {
    /*const SCACHE *insns[MAX_PARALLEL_INSNS];*/
    int first_p;
  } before;
  /* x-after handler */
  struct {
    int empty;
  } after;
  /* This entry is used to terminate each pbb.  */
  struct {
    /* Number of insns in pbb.  */
    int insn_count;
    /* Next pbb to execute.  */
    SCACHE *next;
    SCACHE *branch_target;
  } chain;
#endif
};

/* The ARGBUF struct.  */
struct argbuf {
  /* These are the baseclass definitions.  */
  IADDR addr;
  const IDESC *idesc;
  char trace_p;
  char profile_p;
  /* ??? Temporary hack for skip insns.  */
  char skip_count;
  char unused;
  /* cpu specific data follows */
  union sem semantic;
  int written;
  union sem_fields fields;
};

/* A cached insn.

   ??? SCACHE used to contain more than just argbuf.  We could delete the
   type entirely and always just use ARGBUF, but for future concerns and as
   a level of abstraction it is left in.  */

struct scache {
  struct argbuf argbuf;
};

/* Macros to simplify extraction, reading and semantic code.
   These define and assign the local vars that contain the insn's fields.  */

#define EXTRACT_IFMT_EMPTY_VARS \
  unsigned int length;
#define EXTRACT_IFMT_EMPTY_CODE \
  length = 0; \

#define EXTRACT_IFMT_ADD_VARS \
  UINT f_opcode; \
  UINT f_r0; \
  UINT f_r1; \
  UINT f_r2; \
  UINT f_resv0; \
  unsigned int length;
#define EXTRACT_IFMT_ADD_CODE \
  length = 4; \
  f_opcode = EXTRACT_LSB0_UINT (insn, 32, 31, 6); \
  f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5); \
  f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5); \
  f_r2 = EXTRACT_LSB0_UINT (insn, 32, 15, 5); \
  f_resv0 = EXTRACT_LSB0_UINT (insn, 32, 10, 11); \

#define EXTRACT_IFMT_ADDI_VARS \
  UINT f_opcode; \
  UINT f_r0; \
  UINT f_r1; \
  INT f_imm; \
  unsigned int length;
#define EXTRACT_IFMT_ADDI_CODE \
  length = 4; \
  f_opcode = EXTRACT_LSB0_UINT (insn, 32, 31, 6); \
  f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5); \
  f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5); \
  f_imm = EXTRACT_LSB0_SINT (insn, 32, 15, 16); \

#define EXTRACT_IFMT_ANDI_VARS \
  UINT f_opcode; \
  UINT f_r0; \
  UINT f_r1; \
  UINT f_uimm; \
  unsigned int length;
#define EXTRACT_IFMT_ANDI_CODE \
  length = 4; \
  f_opcode = EXTRACT_LSB0_UINT (insn, 32, 31, 6); \
  f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5); \
  f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5); \
  f_uimm = EXTRACT_LSB0_UINT (insn, 32, 15, 16); \

#define EXTRACT_IFMT_ANDHII_VARS \
  UINT f_opcode; \
  UINT f_r0; \
  UINT f_r1; \
  UINT f_uimm; \
  unsigned int length;
#define EXTRACT_IFMT_ANDHII_CODE \
  length = 4; \
  f_opcode = EXTRACT_LSB0_UINT (insn, 32, 31, 6); \
  f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5); \
  f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5); \
  f_uimm = EXTRACT_LSB0_UINT (insn, 32, 15, 16); \

#define EXTRACT_IFMT_B_VARS \
  UINT f_opcode; \
  UINT f_r0; \
  UINT f_r1; \
  UINT f_r2; \
  UINT f_resv0; \
  unsigned int length;
#define EXTRACT_IFMT_B_CODE \
  length = 4; \
  f_opcode = EXTRACT_LSB0_UINT (insn, 32, 31, 6); \
  f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5); \
  f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5); \
  f_r2 = EXTRACT_LSB0_UINT (insn, 32, 15, 5); \
  f_resv0 = EXTRACT_LSB0_UINT (insn, 32, 10, 11); \

#define EXTRACT_IFMT_BI_VARS \
  UINT f_opcode; \
  SI f_call; \
  unsigned int length;
#define EXTRACT_IFMT_BI_CODE \
  length = 4; \
  f_opcode = EXTRACT_LSB0_UINT (insn, 32, 31, 6); \
  f_call = ((pc) + (((((((((EXTRACT_LSB0_SINT (insn, 32, 25, 26)) & (67108863))) << (2))) ^ (134217728))) - (134217728)))); \

#define EXTRACT_IFMT_BE_VARS \
  UINT f_opcode; \
  UINT f_r0; \
  UINT f_r1; \
  SI f_branch; \
  unsigned int length;
#define EXTRACT_IFMT_BE_CODE \
  length = 4; \
  f_opcode = EXTRACT_LSB0_UINT (insn, 32, 31, 6); \
  f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5); \
  f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5); \
  f_branch = ((pc) + (((((((((EXTRACT_LSB0_SINT (insn, 32, 15, 16)) & (65535))) << (2))) ^ (131072))) - (131072)))); \

#define EXTRACT_IFMT_ORI_VARS \
  UINT f_opcode; \
  UINT f_r0; \
  UINT f_r1; \
  UINT f_uimm; \
  unsigned int length;
#define EXTRACT_IFMT_ORI_CODE \
  length = 4; \
  f_opcode = EXTRACT_LSB0_UINT (insn, 32, 31, 6); \
  f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5); \
  f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5); \
  f_uimm = EXTRACT_LSB0_UINT (insn, 32, 15, 16); \

#define EXTRACT_IFMT_RCSR_VARS \
  UINT f_opcode; \
  UINT f_csr; \
  UINT f_r1; \
  UINT f_r2; \
  UINT f_resv0; \
  unsigned int length;
#define EXTRACT_IFMT_RCSR_CODE \
  length = 4; \
  f_opcode = EXTRACT_LSB0_UINT (insn, 32, 31, 6); \
  f_csr = EXTRACT_LSB0_UINT (insn, 32, 25, 5); \
  f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5); \
  f_r2 = EXTRACT_LSB0_UINT (insn, 32, 15, 5); \
  f_resv0 = EXTRACT_LSB0_UINT (insn, 32, 10, 11); \

#define EXTRACT_IFMT_SEXTB_VARS \
  UINT f_opcode; \
  UINT f_r0; \
  UINT f_r1; \
  UINT f_r2; \
  UINT f_resv0; \
  unsigned int length;
#define EXTRACT_IFMT_SEXTB_CODE \
  length = 4; \
  f_opcode = EXTRACT_LSB0_UINT (insn, 32, 31, 6); \
  f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5); \
  f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5); \
  f_r2 = EXTRACT_LSB0_UINT (insn, 32, 15, 5); \
  f_resv0 = EXTRACT_LSB0_UINT (insn, 32, 10, 11); \

#define EXTRACT_IFMT_USER_VARS \
  UINT f_opcode; \
  UINT f_r0; \
  UINT f_r1; \
  UINT f_r2; \
  UINT f_user; \
  unsigned int length;
#define EXTRACT_IFMT_USER_CODE \
  length = 4; \
  f_opcode = EXTRACT_LSB0_UINT (insn, 32, 31, 6); \
  f_r0 = EXTRACT_LSB0_UINT (insn, 32, 25, 5); \
  f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5); \
  f_r2 = EXTRACT_LSB0_UINT (insn, 32, 15, 5); \
  f_user = EXTRACT_LSB0_UINT (insn, 32, 10, 11); \

#define EXTRACT_IFMT_WCSR_VARS \
  UINT f_opcode; \
  UINT f_csr; \
  UINT f_r1; \
  UINT f_r2; \
  UINT f_resv0; \
  unsigned int length;
#define EXTRACT_IFMT_WCSR_CODE \
  length = 4; \
  f_opcode = EXTRACT_LSB0_UINT (insn, 32, 31, 6); \
  f_csr = EXTRACT_LSB0_UINT (insn, 32, 25, 5); \
  f_r1 = EXTRACT_LSB0_UINT (insn, 32, 20, 5); \
  f_r2 = EXTRACT_LSB0_UINT (insn, 32, 15, 5); \
  f_resv0 = EXTRACT_LSB0_UINT (insn, 32, 10, 11); \

#define EXTRACT_IFMT_BREAK_VARS \
  UINT f_opcode; \
  UINT f_exception; \
  unsigned int length;
#define EXTRACT_IFMT_BREAK_CODE \
  length = 4; \
  f_opcode = EXTRACT_LSB0_UINT (insn, 32, 31, 6); \
  f_exception = EXTRACT_LSB0_UINT (insn, 32, 25, 26); \

/* Collection of various things for the trace handler to use.  */

typedef struct trace_record {
  IADDR pc;
  /* FIXME:wip */
} TRACE_RECORD;

#endif /* CPU_LM32BF_H */
