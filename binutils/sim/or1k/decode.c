/* Simulator instruction decoder for or1k32bf.

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
#include "sim-assert.h"
#include "cgen-mem.h"
#include "cgen-ops.h"

/* The instruction descriptor array.
   This is computed at runtime.  Space for it is not malloc'd to save a
   teensy bit of cpu in the decoder.  Moving it to malloc space is trivial
   but won't be done until necessary (we don't currently support the runtime
   addition of instructions nor an SMP machine with different cpus).  */
static IDESC or1k32bf_insn_data[OR1K32BF_INSN__MAX];

/* Commas between elements are contained in the macros.
   Some of these are conditionally compiled out.  */

static const struct insn_sem or1k32bf_insn_sem[] =
{
  { VIRTUAL_INSN_X_INVALID, OR1K32BF_INSN_X_INVALID, OR1K32BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_AFTER, OR1K32BF_INSN_X_AFTER, OR1K32BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEFORE, OR1K32BF_INSN_X_BEFORE, OR1K32BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CTI_CHAIN, OR1K32BF_INSN_X_CTI_CHAIN, OR1K32BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CHAIN, OR1K32BF_INSN_X_CHAIN, OR1K32BF_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEGIN, OR1K32BF_INSN_X_BEGIN, OR1K32BF_SFMT_EMPTY },
  { OR1K_INSN_L_J, OR1K32BF_INSN_L_J, OR1K32BF_SFMT_L_J },
  { OR1K_INSN_L_ADRP, OR1K32BF_INSN_L_ADRP, OR1K32BF_SFMT_L_ADRP },
  { OR1K_INSN_L_JAL, OR1K32BF_INSN_L_JAL, OR1K32BF_SFMT_L_JAL },
  { OR1K_INSN_L_JR, OR1K32BF_INSN_L_JR, OR1K32BF_SFMT_L_JR },
  { OR1K_INSN_L_JALR, OR1K32BF_INSN_L_JALR, OR1K32BF_SFMT_L_JALR },
  { OR1K_INSN_L_BNF, OR1K32BF_INSN_L_BNF, OR1K32BF_SFMT_L_BNF },
  { OR1K_INSN_L_BF, OR1K32BF_INSN_L_BF, OR1K32BF_SFMT_L_BNF },
  { OR1K_INSN_L_TRAP, OR1K32BF_INSN_L_TRAP, OR1K32BF_SFMT_L_TRAP },
  { OR1K_INSN_L_SYS, OR1K32BF_INSN_L_SYS, OR1K32BF_SFMT_L_TRAP },
  { OR1K_INSN_L_MSYNC, OR1K32BF_INSN_L_MSYNC, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_PSYNC, OR1K32BF_INSN_L_PSYNC, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CSYNC, OR1K32BF_INSN_L_CSYNC, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_RFE, OR1K32BF_INSN_L_RFE, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_NOP_IMM, OR1K32BF_INSN_L_NOP_IMM, OR1K32BF_SFMT_L_NOP_IMM },
  { OR1K_INSN_L_MOVHI, OR1K32BF_INSN_L_MOVHI, OR1K32BF_SFMT_L_MOVHI },
  { OR1K_INSN_L_MACRC, OR1K32BF_INSN_L_MACRC, OR1K32BF_SFMT_L_MACRC },
  { OR1K_INSN_L_MFSPR, OR1K32BF_INSN_L_MFSPR, OR1K32BF_SFMT_L_MFSPR },
  { OR1K_INSN_L_MTSPR, OR1K32BF_INSN_L_MTSPR, OR1K32BF_SFMT_L_MTSPR },
  { OR1K_INSN_L_LWZ, OR1K32BF_INSN_L_LWZ, OR1K32BF_SFMT_L_LWZ },
  { OR1K_INSN_L_LWS, OR1K32BF_INSN_L_LWS, OR1K32BF_SFMT_L_LWS },
  { OR1K_INSN_L_LWA, OR1K32BF_INSN_L_LWA, OR1K32BF_SFMT_L_LWA },
  { OR1K_INSN_L_LBZ, OR1K32BF_INSN_L_LBZ, OR1K32BF_SFMT_L_LBZ },
  { OR1K_INSN_L_LBS, OR1K32BF_INSN_L_LBS, OR1K32BF_SFMT_L_LBS },
  { OR1K_INSN_L_LHZ, OR1K32BF_INSN_L_LHZ, OR1K32BF_SFMT_L_LHZ },
  { OR1K_INSN_L_LHS, OR1K32BF_INSN_L_LHS, OR1K32BF_SFMT_L_LHS },
  { OR1K_INSN_L_SW, OR1K32BF_INSN_L_SW, OR1K32BF_SFMT_L_SW },
  { OR1K_INSN_L_SB, OR1K32BF_INSN_L_SB, OR1K32BF_SFMT_L_SB },
  { OR1K_INSN_L_SH, OR1K32BF_INSN_L_SH, OR1K32BF_SFMT_L_SH },
  { OR1K_INSN_L_SWA, OR1K32BF_INSN_L_SWA, OR1K32BF_SFMT_L_SWA },
  { OR1K_INSN_L_SLL, OR1K32BF_INSN_L_SLL, OR1K32BF_SFMT_L_SLL },
  { OR1K_INSN_L_SLLI, OR1K32BF_INSN_L_SLLI, OR1K32BF_SFMT_L_SLLI },
  { OR1K_INSN_L_SRL, OR1K32BF_INSN_L_SRL, OR1K32BF_SFMT_L_SLL },
  { OR1K_INSN_L_SRLI, OR1K32BF_INSN_L_SRLI, OR1K32BF_SFMT_L_SLLI },
  { OR1K_INSN_L_SRA, OR1K32BF_INSN_L_SRA, OR1K32BF_SFMT_L_SLL },
  { OR1K_INSN_L_SRAI, OR1K32BF_INSN_L_SRAI, OR1K32BF_SFMT_L_SLLI },
  { OR1K_INSN_L_ROR, OR1K32BF_INSN_L_ROR, OR1K32BF_SFMT_L_SLL },
  { OR1K_INSN_L_RORI, OR1K32BF_INSN_L_RORI, OR1K32BF_SFMT_L_SLLI },
  { OR1K_INSN_L_AND, OR1K32BF_INSN_L_AND, OR1K32BF_SFMT_L_AND },
  { OR1K_INSN_L_OR, OR1K32BF_INSN_L_OR, OR1K32BF_SFMT_L_AND },
  { OR1K_INSN_L_XOR, OR1K32BF_INSN_L_XOR, OR1K32BF_SFMT_L_AND },
  { OR1K_INSN_L_ADD, OR1K32BF_INSN_L_ADD, OR1K32BF_SFMT_L_ADD },
  { OR1K_INSN_L_SUB, OR1K32BF_INSN_L_SUB, OR1K32BF_SFMT_L_ADD },
  { OR1K_INSN_L_ADDC, OR1K32BF_INSN_L_ADDC, OR1K32BF_SFMT_L_ADDC },
  { OR1K_INSN_L_MUL, OR1K32BF_INSN_L_MUL, OR1K32BF_SFMT_L_MUL },
  { OR1K_INSN_L_MULD, OR1K32BF_INSN_L_MULD, OR1K32BF_SFMT_L_MULD },
  { OR1K_INSN_L_MULU, OR1K32BF_INSN_L_MULU, OR1K32BF_SFMT_L_MULU },
  { OR1K_INSN_L_MULDU, OR1K32BF_INSN_L_MULDU, OR1K32BF_SFMT_L_MULD },
  { OR1K_INSN_L_DIV, OR1K32BF_INSN_L_DIV, OR1K32BF_SFMT_L_DIV },
  { OR1K_INSN_L_DIVU, OR1K32BF_INSN_L_DIVU, OR1K32BF_SFMT_L_DIVU },
  { OR1K_INSN_L_FF1, OR1K32BF_INSN_L_FF1, OR1K32BF_SFMT_L_FF1 },
  { OR1K_INSN_L_FL1, OR1K32BF_INSN_L_FL1, OR1K32BF_SFMT_L_FF1 },
  { OR1K_INSN_L_ANDI, OR1K32BF_INSN_L_ANDI, OR1K32BF_SFMT_L_MFSPR },
  { OR1K_INSN_L_ORI, OR1K32BF_INSN_L_ORI, OR1K32BF_SFMT_L_MFSPR },
  { OR1K_INSN_L_XORI, OR1K32BF_INSN_L_XORI, OR1K32BF_SFMT_L_XORI },
  { OR1K_INSN_L_ADDI, OR1K32BF_INSN_L_ADDI, OR1K32BF_SFMT_L_ADDI },
  { OR1K_INSN_L_ADDIC, OR1K32BF_INSN_L_ADDIC, OR1K32BF_SFMT_L_ADDIC },
  { OR1K_INSN_L_MULI, OR1K32BF_INSN_L_MULI, OR1K32BF_SFMT_L_MULI },
  { OR1K_INSN_L_EXTHS, OR1K32BF_INSN_L_EXTHS, OR1K32BF_SFMT_L_EXTHS },
  { OR1K_INSN_L_EXTBS, OR1K32BF_INSN_L_EXTBS, OR1K32BF_SFMT_L_EXTHS },
  { OR1K_INSN_L_EXTHZ, OR1K32BF_INSN_L_EXTHZ, OR1K32BF_SFMT_L_EXTHS },
  { OR1K_INSN_L_EXTBZ, OR1K32BF_INSN_L_EXTBZ, OR1K32BF_SFMT_L_EXTHS },
  { OR1K_INSN_L_EXTWS, OR1K32BF_INSN_L_EXTWS, OR1K32BF_SFMT_L_EXTHS },
  { OR1K_INSN_L_EXTWZ, OR1K32BF_INSN_L_EXTWZ, OR1K32BF_SFMT_L_EXTHS },
  { OR1K_INSN_L_CMOV, OR1K32BF_INSN_L_CMOV, OR1K32BF_SFMT_L_CMOV },
  { OR1K_INSN_L_SFGTS, OR1K32BF_INSN_L_SFGTS, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFGTSI, OR1K32BF_INSN_L_SFGTSI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFGTU, OR1K32BF_INSN_L_SFGTU, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFGTUI, OR1K32BF_INSN_L_SFGTUI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFGES, OR1K32BF_INSN_L_SFGES, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFGESI, OR1K32BF_INSN_L_SFGESI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFGEU, OR1K32BF_INSN_L_SFGEU, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFGEUI, OR1K32BF_INSN_L_SFGEUI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFLTS, OR1K32BF_INSN_L_SFLTS, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFLTSI, OR1K32BF_INSN_L_SFLTSI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFLTU, OR1K32BF_INSN_L_SFLTU, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFLTUI, OR1K32BF_INSN_L_SFLTUI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFLES, OR1K32BF_INSN_L_SFLES, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFLESI, OR1K32BF_INSN_L_SFLESI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFLEU, OR1K32BF_INSN_L_SFLEU, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFLEUI, OR1K32BF_INSN_L_SFLEUI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFEQ, OR1K32BF_INSN_L_SFEQ, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFEQI, OR1K32BF_INSN_L_SFEQI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_SFNE, OR1K32BF_INSN_L_SFNE, OR1K32BF_SFMT_L_SFGTS },
  { OR1K_INSN_L_SFNEI, OR1K32BF_INSN_L_SFNEI, OR1K32BF_SFMT_L_SFGTSI },
  { OR1K_INSN_L_MAC, OR1K32BF_INSN_L_MAC, OR1K32BF_SFMT_L_MAC },
  { OR1K_INSN_L_MACI, OR1K32BF_INSN_L_MACI, OR1K32BF_SFMT_L_MACI },
  { OR1K_INSN_L_MACU, OR1K32BF_INSN_L_MACU, OR1K32BF_SFMT_L_MACU },
  { OR1K_INSN_L_MSB, OR1K32BF_INSN_L_MSB, OR1K32BF_SFMT_L_MAC },
  { OR1K_INSN_L_MSBU, OR1K32BF_INSN_L_MSBU, OR1K32BF_SFMT_L_MACU },
  { OR1K_INSN_L_CUST1, OR1K32BF_INSN_L_CUST1, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CUST2, OR1K32BF_INSN_L_CUST2, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CUST3, OR1K32BF_INSN_L_CUST3, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CUST4, OR1K32BF_INSN_L_CUST4, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CUST5, OR1K32BF_INSN_L_CUST5, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CUST6, OR1K32BF_INSN_L_CUST6, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CUST7, OR1K32BF_INSN_L_CUST7, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_L_CUST8, OR1K32BF_INSN_L_CUST8, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_LF_ADD_S, OR1K32BF_INSN_LF_ADD_S, OR1K32BF_SFMT_LF_ADD_S },
  { OR1K_INSN_LF_ADD_D32, OR1K32BF_INSN_LF_ADD_D32, OR1K32BF_SFMT_LF_ADD_D32 },
  { OR1K_INSN_LF_SUB_S, OR1K32BF_INSN_LF_SUB_S, OR1K32BF_SFMT_LF_ADD_S },
  { OR1K_INSN_LF_SUB_D32, OR1K32BF_INSN_LF_SUB_D32, OR1K32BF_SFMT_LF_ADD_D32 },
  { OR1K_INSN_LF_MUL_S, OR1K32BF_INSN_LF_MUL_S, OR1K32BF_SFMT_LF_ADD_S },
  { OR1K_INSN_LF_MUL_D32, OR1K32BF_INSN_LF_MUL_D32, OR1K32BF_SFMT_LF_ADD_D32 },
  { OR1K_INSN_LF_DIV_S, OR1K32BF_INSN_LF_DIV_S, OR1K32BF_SFMT_LF_ADD_S },
  { OR1K_INSN_LF_DIV_D32, OR1K32BF_INSN_LF_DIV_D32, OR1K32BF_SFMT_LF_ADD_D32 },
  { OR1K_INSN_LF_REM_S, OR1K32BF_INSN_LF_REM_S, OR1K32BF_SFMT_LF_ADD_S },
  { OR1K_INSN_LF_REM_D32, OR1K32BF_INSN_LF_REM_D32, OR1K32BF_SFMT_LF_ADD_D32 },
  { OR1K_INSN_LF_ITOF_S, OR1K32BF_INSN_LF_ITOF_S, OR1K32BF_SFMT_LF_ITOF_S },
  { OR1K_INSN_LF_ITOF_D32, OR1K32BF_INSN_LF_ITOF_D32, OR1K32BF_SFMT_LF_ITOF_D32 },
  { OR1K_INSN_LF_FTOI_S, OR1K32BF_INSN_LF_FTOI_S, OR1K32BF_SFMT_LF_FTOI_S },
  { OR1K_INSN_LF_FTOI_D32, OR1K32BF_INSN_LF_FTOI_D32, OR1K32BF_SFMT_LF_FTOI_D32 },
  { OR1K_INSN_LF_SFEQ_S, OR1K32BF_INSN_LF_SFEQ_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFEQ_D32, OR1K32BF_INSN_LF_SFEQ_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFNE_S, OR1K32BF_INSN_LF_SFNE_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFNE_D32, OR1K32BF_INSN_LF_SFNE_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFGE_S, OR1K32BF_INSN_LF_SFGE_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFGE_D32, OR1K32BF_INSN_LF_SFGE_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFGT_S, OR1K32BF_INSN_LF_SFGT_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFGT_D32, OR1K32BF_INSN_LF_SFGT_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFLT_S, OR1K32BF_INSN_LF_SFLT_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFLT_D32, OR1K32BF_INSN_LF_SFLT_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFLE_S, OR1K32BF_INSN_LF_SFLE_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFLE_D32, OR1K32BF_INSN_LF_SFLE_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFUEQ_S, OR1K32BF_INSN_LF_SFUEQ_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFUEQ_D32, OR1K32BF_INSN_LF_SFUEQ_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFUNE_S, OR1K32BF_INSN_LF_SFUNE_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFUNE_D32, OR1K32BF_INSN_LF_SFUNE_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFUGT_S, OR1K32BF_INSN_LF_SFUGT_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFUGT_D32, OR1K32BF_INSN_LF_SFUGT_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFUGE_S, OR1K32BF_INSN_LF_SFUGE_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFUGE_D32, OR1K32BF_INSN_LF_SFUGE_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFULT_S, OR1K32BF_INSN_LF_SFULT_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFULT_D32, OR1K32BF_INSN_LF_SFULT_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFULE_S, OR1K32BF_INSN_LF_SFULE_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFULE_D32, OR1K32BF_INSN_LF_SFULE_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_SFUN_S, OR1K32BF_INSN_LF_SFUN_S, OR1K32BF_SFMT_LF_SFEQ_S },
  { OR1K_INSN_LF_SFUN_D32, OR1K32BF_INSN_LF_SFUN_D32, OR1K32BF_SFMT_LF_SFEQ_D32 },
  { OR1K_INSN_LF_MADD_S, OR1K32BF_INSN_LF_MADD_S, OR1K32BF_SFMT_LF_MADD_S },
  { OR1K_INSN_LF_MADD_D32, OR1K32BF_INSN_LF_MADD_D32, OR1K32BF_SFMT_LF_MADD_D32 },
  { OR1K_INSN_LF_CUST1_S, OR1K32BF_INSN_LF_CUST1_S, OR1K32BF_SFMT_L_MSYNC },
  { OR1K_INSN_LF_CUST1_D32, OR1K32BF_INSN_LF_CUST1_D32, OR1K32BF_SFMT_L_MSYNC },
};

static const struct insn_sem or1k32bf_insn_sem_invalid =
{
  VIRTUAL_INSN_X_INVALID, OR1K32BF_INSN_X_INVALID, OR1K32BF_SFMT_EMPTY
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
or1k32bf_init_idesc_table (SIM_CPU *cpu)
{
  IDESC *id,*tabend;
  const struct insn_sem *t,*tend;
  int tabsize = OR1K32BF_INSN__MAX;
  IDESC *table = or1k32bf_insn_data;

  memset (table, 0, tabsize * sizeof (IDESC));

  /* First set all entries to the `invalid insn'.  */
  t = & or1k32bf_insn_sem_invalid;
  for (id = table, tabend = table + tabsize; id < tabend; ++id)
    init_idesc (cpu, id, t);

  /* Now fill in the values for the chosen cpu.  */
  for (t = or1k32bf_insn_sem, tend = t + ARRAY_SIZE (or1k32bf_insn_sem);
       t != tend; ++t)
    {
      init_idesc (cpu, & table[t->index], t);
    }

  /* Link the IDESC table into the cpu.  */
  CPU_IDESC (cpu) = table;
}

/* Given an instruction, return a pointer to its IDESC entry.  */

const IDESC *
or1k32bf_decode (SIM_CPU *current_cpu, IADDR pc,
              CGEN_INSN_WORD base_insn, CGEN_INSN_WORD entire_insn,
              ARGBUF *abuf)
{
  /* Result of decoder.  */
  OR1K32BF_INSN_TYPE itype;

  {
    CGEN_INSN_WORD insn = base_insn;

    {
      unsigned int val0 = (((insn >> 21) & (63 << 5)) | ((insn >> 0) & (31 << 0)));
      switch (val0)
      {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
      case 16:
      case 17:
      case 18:
      case 19:
      case 20:
      case 21:
      case 22:
      case 23:
      case 24:
      case 25:
      case 26:
      case 27:
      case 28:
      case 29:
      case 30:
      case 31: itype = OR1K32BF_INSN_L_J; goto extract_sfmt_l_j;
      case 32:
      case 33:
      case 34:
      case 35:
      case 36:
      case 37:
      case 38:
      case 39:
      case 40:
      case 41:
      case 42:
      case 43:
      case 44:
      case 45:
      case 46:
      case 47:
      case 48:
      case 49:
      case 50:
      case 51:
      case 52:
      case 53:
      case 54:
      case 55:
      case 56:
      case 57:
      case 58:
      case 59:
      case 60:
      case 61:
      case 62:
      case 63: itype = OR1K32BF_INSN_L_JAL; goto extract_sfmt_l_jal;
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
      case 79:
      case 80:
      case 81:
      case 82:
      case 83:
      case 84:
      case 85:
      case 86:
      case 87:
      case 88:
      case 89:
      case 90:
      case 91:
      case 92:
      case 93:
      case 94:
      case 95: itype = OR1K32BF_INSN_L_ADRP; goto extract_sfmt_l_adrp;
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
      case 111:
      case 112:
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
      case 127: itype = OR1K32BF_INSN_L_BNF; goto extract_sfmt_l_bnf;
      case 128:
      case 129:
      case 130:
      case 131:
      case 132:
      case 133:
      case 134:
      case 135:
      case 136:
      case 137:
      case 138:
      case 139:
      case 140:
      case 141:
      case 142:
      case 143:
      case 144:
      case 145:
      case 146:
      case 147:
      case 148:
      case 149:
      case 150:
      case 151:
      case 152:
      case 153:
      case 154:
      case 155:
      case 156:
      case 157:
      case 158:
      case 159: itype = OR1K32BF_INSN_L_BF; goto extract_sfmt_l_bnf;
      case 160:
      case 161:
      case 162:
      case 163:
      case 164:
      case 165:
      case 166:
      case 167:
      case 168:
      case 169:
      case 170:
      case 171:
      case 172:
      case 173:
      case 174:
      case 175:
      case 176:
      case 177:
      case 178:
      case 179:
      case 180:
      case 181:
      case 182:
      case 183:
      case 184:
      case 185:
      case 186:
      case 187:
      case 188:
      case 189:
      case 190:
      case 191:
        if ((entire_insn & 0xffff0000) == 0x15000000)
          { itype = OR1K32BF_INSN_L_NOP_IMM; goto extract_sfmt_l_nop_imm; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 192:
        {
          unsigned int val1 = (((insn >> 16) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xfc1f0000) == 0x18000000)
              { itype = OR1K32BF_INSN_L_MOVHI; goto extract_sfmt_l_movhi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xfc1fffff) == 0x18010000)
              { itype = OR1K32BF_INSN_L_MACRC; goto extract_sfmt_l_macrc; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 193:
      case 194:
      case 195:
      case 196:
      case 197:
      case 198:
      case 199:
      case 200:
      case 201:
      case 202:
      case 203:
      case 204:
      case 205:
      case 206:
      case 207:
      case 208:
      case 209:
      case 210:
      case 211:
      case 212:
      case 213:
      case 214:
      case 215:
      case 216:
      case 217:
      case 218:
      case 219:
      case 220:
      case 221:
      case 222:
      case 223:
        if ((entire_insn & 0xfc1f0000) == 0x18000000)
          { itype = OR1K32BF_INSN_L_MOVHI; goto extract_sfmt_l_movhi; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 256:
        {
          unsigned int val1 = (((insn >> 23) & (7 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffff0000) == 0x20000000)
              { itype = OR1K32BF_INSN_L_SYS; goto extract_sfmt_l_trap; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 2:
            if ((entire_insn & 0xffff0000) == 0x21000000)
              { itype = OR1K32BF_INSN_L_TRAP; goto extract_sfmt_l_trap; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 4:
            if ((entire_insn & 0xffffffff) == 0x22000000)
              { itype = OR1K32BF_INSN_L_MSYNC; goto extract_sfmt_l_msync; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 5:
            if ((entire_insn & 0xffffffff) == 0x22800000)
              { itype = OR1K32BF_INSN_L_PSYNC; goto extract_sfmt_l_msync; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 6:
            if ((entire_insn & 0xffffffff) == 0x23000000)
              { itype = OR1K32BF_INSN_L_CSYNC; goto extract_sfmt_l_msync; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 257:
      case 258:
      case 259:
      case 260:
      case 261:
      case 262:
      case 263:
      case 264:
      case 265:
      case 266:
      case 267:
      case 268:
      case 269:
      case 270:
      case 271:
      case 272:
      case 273:
      case 274:
      case 275:
      case 276:
      case 277:
      case 278:
      case 279:
      case 280:
      case 281:
      case 282:
      case 283:
      case 284:
      case 285:
      case 286:
      case 287:
        {
          unsigned int val1 = (((insn >> 24) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffff0000) == 0x20000000)
              { itype = OR1K32BF_INSN_L_SYS; goto extract_sfmt_l_trap; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xffff0000) == 0x21000000)
              { itype = OR1K32BF_INSN_L_TRAP; goto extract_sfmt_l_trap; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 288:
        if ((entire_insn & 0xffffffff) == 0x24000000)
          { itype = OR1K32BF_INSN_L_RFE; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 544:
        if ((entire_insn & 0xffff07ff) == 0x44000000)
          { itype = OR1K32BF_INSN_L_JR; goto extract_sfmt_l_jr; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 576:
        if ((entire_insn & 0xffff07ff) == 0x48000000)
          { itype = OR1K32BF_INSN_L_JALR; goto extract_sfmt_l_jalr; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 608:
      case 609:
      case 610:
      case 611:
      case 612:
      case 613:
      case 614:
      case 615:
      case 616:
      case 617:
      case 618:
      case 619:
      case 620:
      case 621:
      case 622:
      case 623:
      case 624:
      case 625:
      case 626:
      case 627:
      case 628:
      case 629:
      case 630:
      case 631:
      case 632:
      case 633:
      case 634:
      case 635:
      case 636:
      case 637:
      case 638:
      case 639:
        if ((entire_insn & 0xffe00000) == 0x4c000000)
          { itype = OR1K32BF_INSN_L_MACI; goto extract_sfmt_l_maci; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 864:
      case 865:
      case 866:
      case 867:
      case 868:
      case 869:
      case 870:
      case 871:
      case 872:
      case 873:
      case 874:
      case 875:
      case 876:
      case 877:
      case 878:
      case 879:
      case 880:
      case 881:
      case 882:
      case 883:
      case 884:
      case 885:
      case 886:
      case 887:
      case 888:
      case 889:
      case 890:
      case 891:
      case 892:
      case 893:
      case 894:
      case 895: itype = OR1K32BF_INSN_L_LWA; goto extract_sfmt_l_lwa;
      case 896:
        if ((entire_insn & 0xffffffff) == 0x70000000)
          { itype = OR1K32BF_INSN_L_CUST1; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 928:
        if ((entire_insn & 0xffffffff) == 0x74000000)
          { itype = OR1K32BF_INSN_L_CUST2; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 960:
        if ((entire_insn & 0xffffffff) == 0x78000000)
          { itype = OR1K32BF_INSN_L_CUST3; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 992:
        if ((entire_insn & 0xffffffff) == 0x7c000000)
          { itype = OR1K32BF_INSN_L_CUST4; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1056:
      case 1057:
      case 1058:
      case 1059:
      case 1060:
      case 1061:
      case 1062:
      case 1063:
      case 1064:
      case 1065:
      case 1066:
      case 1067:
      case 1068:
      case 1069:
      case 1070:
      case 1071:
      case 1072:
      case 1073:
      case 1074:
      case 1075:
      case 1076:
      case 1077:
      case 1078:
      case 1079:
      case 1080:
      case 1081:
      case 1082:
      case 1083:
      case 1084:
      case 1085:
      case 1086:
      case 1087: itype = OR1K32BF_INSN_L_LWZ; goto extract_sfmt_l_lwz;
      case 1088:
      case 1089:
      case 1090:
      case 1091:
      case 1092:
      case 1093:
      case 1094:
      case 1095:
      case 1096:
      case 1097:
      case 1098:
      case 1099:
      case 1100:
      case 1101:
      case 1102:
      case 1103:
      case 1104:
      case 1105:
      case 1106:
      case 1107:
      case 1108:
      case 1109:
      case 1110:
      case 1111:
      case 1112:
      case 1113:
      case 1114:
      case 1115:
      case 1116:
      case 1117:
      case 1118:
      case 1119: itype = OR1K32BF_INSN_L_LWS; goto extract_sfmt_l_lws;
      case 1120:
      case 1121:
      case 1122:
      case 1123:
      case 1124:
      case 1125:
      case 1126:
      case 1127:
      case 1128:
      case 1129:
      case 1130:
      case 1131:
      case 1132:
      case 1133:
      case 1134:
      case 1135:
      case 1136:
      case 1137:
      case 1138:
      case 1139:
      case 1140:
      case 1141:
      case 1142:
      case 1143:
      case 1144:
      case 1145:
      case 1146:
      case 1147:
      case 1148:
      case 1149:
      case 1150:
      case 1151: itype = OR1K32BF_INSN_L_LBZ; goto extract_sfmt_l_lbz;
      case 1152:
      case 1153:
      case 1154:
      case 1155:
      case 1156:
      case 1157:
      case 1158:
      case 1159:
      case 1160:
      case 1161:
      case 1162:
      case 1163:
      case 1164:
      case 1165:
      case 1166:
      case 1167:
      case 1168:
      case 1169:
      case 1170:
      case 1171:
      case 1172:
      case 1173:
      case 1174:
      case 1175:
      case 1176:
      case 1177:
      case 1178:
      case 1179:
      case 1180:
      case 1181:
      case 1182:
      case 1183: itype = OR1K32BF_INSN_L_LBS; goto extract_sfmt_l_lbs;
      case 1184:
      case 1185:
      case 1186:
      case 1187:
      case 1188:
      case 1189:
      case 1190:
      case 1191:
      case 1192:
      case 1193:
      case 1194:
      case 1195:
      case 1196:
      case 1197:
      case 1198:
      case 1199:
      case 1200:
      case 1201:
      case 1202:
      case 1203:
      case 1204:
      case 1205:
      case 1206:
      case 1207:
      case 1208:
      case 1209:
      case 1210:
      case 1211:
      case 1212:
      case 1213:
      case 1214:
      case 1215: itype = OR1K32BF_INSN_L_LHZ; goto extract_sfmt_l_lhz;
      case 1216:
      case 1217:
      case 1218:
      case 1219:
      case 1220:
      case 1221:
      case 1222:
      case 1223:
      case 1224:
      case 1225:
      case 1226:
      case 1227:
      case 1228:
      case 1229:
      case 1230:
      case 1231:
      case 1232:
      case 1233:
      case 1234:
      case 1235:
      case 1236:
      case 1237:
      case 1238:
      case 1239:
      case 1240:
      case 1241:
      case 1242:
      case 1243:
      case 1244:
      case 1245:
      case 1246:
      case 1247: itype = OR1K32BF_INSN_L_LHS; goto extract_sfmt_l_lhs;
      case 1248:
      case 1249:
      case 1250:
      case 1251:
      case 1252:
      case 1253:
      case 1254:
      case 1255:
      case 1256:
      case 1257:
      case 1258:
      case 1259:
      case 1260:
      case 1261:
      case 1262:
      case 1263:
      case 1264:
      case 1265:
      case 1266:
      case 1267:
      case 1268:
      case 1269:
      case 1270:
      case 1271:
      case 1272:
      case 1273:
      case 1274:
      case 1275:
      case 1276:
      case 1277:
      case 1278:
      case 1279: itype = OR1K32BF_INSN_L_ADDI; goto extract_sfmt_l_addi;
      case 1280:
      case 1281:
      case 1282:
      case 1283:
      case 1284:
      case 1285:
      case 1286:
      case 1287:
      case 1288:
      case 1289:
      case 1290:
      case 1291:
      case 1292:
      case 1293:
      case 1294:
      case 1295:
      case 1296:
      case 1297:
      case 1298:
      case 1299:
      case 1300:
      case 1301:
      case 1302:
      case 1303:
      case 1304:
      case 1305:
      case 1306:
      case 1307:
      case 1308:
      case 1309:
      case 1310:
      case 1311: itype = OR1K32BF_INSN_L_ADDIC; goto extract_sfmt_l_addic;
      case 1312:
      case 1313:
      case 1314:
      case 1315:
      case 1316:
      case 1317:
      case 1318:
      case 1319:
      case 1320:
      case 1321:
      case 1322:
      case 1323:
      case 1324:
      case 1325:
      case 1326:
      case 1327:
      case 1328:
      case 1329:
      case 1330:
      case 1331:
      case 1332:
      case 1333:
      case 1334:
      case 1335:
      case 1336:
      case 1337:
      case 1338:
      case 1339:
      case 1340:
      case 1341:
      case 1342:
      case 1343: itype = OR1K32BF_INSN_L_ANDI; goto extract_sfmt_l_mfspr;
      case 1344:
      case 1345:
      case 1346:
      case 1347:
      case 1348:
      case 1349:
      case 1350:
      case 1351:
      case 1352:
      case 1353:
      case 1354:
      case 1355:
      case 1356:
      case 1357:
      case 1358:
      case 1359:
      case 1360:
      case 1361:
      case 1362:
      case 1363:
      case 1364:
      case 1365:
      case 1366:
      case 1367:
      case 1368:
      case 1369:
      case 1370:
      case 1371:
      case 1372:
      case 1373:
      case 1374:
      case 1375: itype = OR1K32BF_INSN_L_ORI; goto extract_sfmt_l_mfspr;
      case 1376:
      case 1377:
      case 1378:
      case 1379:
      case 1380:
      case 1381:
      case 1382:
      case 1383:
      case 1384:
      case 1385:
      case 1386:
      case 1387:
      case 1388:
      case 1389:
      case 1390:
      case 1391:
      case 1392:
      case 1393:
      case 1394:
      case 1395:
      case 1396:
      case 1397:
      case 1398:
      case 1399:
      case 1400:
      case 1401:
      case 1402:
      case 1403:
      case 1404:
      case 1405:
      case 1406:
      case 1407: itype = OR1K32BF_INSN_L_XORI; goto extract_sfmt_l_xori;
      case 1408:
      case 1409:
      case 1410:
      case 1411:
      case 1412:
      case 1413:
      case 1414:
      case 1415:
      case 1416:
      case 1417:
      case 1418:
      case 1419:
      case 1420:
      case 1421:
      case 1422:
      case 1423:
      case 1424:
      case 1425:
      case 1426:
      case 1427:
      case 1428:
      case 1429:
      case 1430:
      case 1431:
      case 1432:
      case 1433:
      case 1434:
      case 1435:
      case 1436:
      case 1437:
      case 1438:
      case 1439: itype = OR1K32BF_INSN_L_MULI; goto extract_sfmt_l_muli;
      case 1440:
      case 1441:
      case 1442:
      case 1443:
      case 1444:
      case 1445:
      case 1446:
      case 1447:
      case 1448:
      case 1449:
      case 1450:
      case 1451:
      case 1452:
      case 1453:
      case 1454:
      case 1455:
      case 1456:
      case 1457:
      case 1458:
      case 1459:
      case 1460:
      case 1461:
      case 1462:
      case 1463:
      case 1464:
      case 1465:
      case 1466:
      case 1467:
      case 1468:
      case 1469:
      case 1470:
      case 1471: itype = OR1K32BF_INSN_L_MFSPR; goto extract_sfmt_l_mfspr;
      case 1472:
      case 1473:
      case 1474:
      case 1475:
      case 1476:
      case 1477:
      case 1478:
      case 1479:
      case 1480:
      case 1481:
      case 1482:
      case 1483:
      case 1484:
      case 1485:
      case 1486:
      case 1487:
      case 1488:
      case 1489:
      case 1490:
      case 1491:
      case 1492:
      case 1493:
      case 1494:
      case 1495:
      case 1496:
      case 1497:
      case 1498:
      case 1499:
      case 1500:
      case 1501:
      case 1502:
      case 1503:
        {
          unsigned int val1 = (((insn >> 6) & (3 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xfc00ffc0) == 0xb8000000)
              { itype = OR1K32BF_INSN_L_SLLI; goto extract_sfmt_l_slli; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xfc00ffc0) == 0xb8000040)
              { itype = OR1K32BF_INSN_L_SRLI; goto extract_sfmt_l_slli; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 2:
            if ((entire_insn & 0xfc00ffc0) == 0xb8000080)
              { itype = OR1K32BF_INSN_L_SRAI; goto extract_sfmt_l_slli; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 3:
            if ((entire_insn & 0xfc00ffc0) == 0xb80000c0)
              { itype = OR1K32BF_INSN_L_RORI; goto extract_sfmt_l_slli; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1504:
      case 1505:
      case 1506:
      case 1507:
      case 1508:
      case 1509:
      case 1510:
      case 1511:
      case 1512:
      case 1513:
      case 1514:
      case 1515:
      case 1516:
      case 1517:
      case 1518:
      case 1519:
      case 1520:
      case 1521:
      case 1522:
      case 1523:
      case 1524:
      case 1525:
      case 1526:
      case 1527:
      case 1528:
      case 1529:
      case 1530:
      case 1531:
      case 1532:
      case 1533:
      case 1534:
      case 1535:
        {
          unsigned int val1 = (((insn >> 21) & (15 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffe00000) == 0xbc000000)
              { itype = OR1K32BF_INSN_L_SFEQI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xffe00000) == 0xbc200000)
              { itype = OR1K32BF_INSN_L_SFNEI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 2:
            if ((entire_insn & 0xffe00000) == 0xbc400000)
              { itype = OR1K32BF_INSN_L_SFGTUI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 3:
            if ((entire_insn & 0xffe00000) == 0xbc600000)
              { itype = OR1K32BF_INSN_L_SFGEUI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 4:
            if ((entire_insn & 0xffe00000) == 0xbc800000)
              { itype = OR1K32BF_INSN_L_SFLTUI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 5:
            if ((entire_insn & 0xffe00000) == 0xbca00000)
              { itype = OR1K32BF_INSN_L_SFLEUI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 10:
            if ((entire_insn & 0xffe00000) == 0xbd400000)
              { itype = OR1K32BF_INSN_L_SFGTSI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 11:
            if ((entire_insn & 0xffe00000) == 0xbd600000)
              { itype = OR1K32BF_INSN_L_SFGESI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 12:
            if ((entire_insn & 0xffe00000) == 0xbd800000)
              { itype = OR1K32BF_INSN_L_SFLTSI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 13:
            if ((entire_insn & 0xffe00000) == 0xbda00000)
              { itype = OR1K32BF_INSN_L_SFLESI; goto extract_sfmt_l_sfgtsi; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1536:
      case 1537:
      case 1538:
      case 1539:
      case 1540:
      case 1541:
      case 1542:
      case 1543:
      case 1544:
      case 1545:
      case 1546:
      case 1547:
      case 1548:
      case 1549:
      case 1550:
      case 1551:
      case 1552:
      case 1553:
      case 1554:
      case 1555:
      case 1556:
      case 1557:
      case 1558:
      case 1559:
      case 1560:
      case 1561:
      case 1562:
      case 1563:
      case 1564:
      case 1565:
      case 1566:
      case 1567: itype = OR1K32BF_INSN_L_MTSPR; goto extract_sfmt_l_mtspr;
      case 1569:
        if ((entire_insn & 0xffe007ff) == 0xc4000001)
          { itype = OR1K32BF_INSN_L_MAC; goto extract_sfmt_l_mac; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1570:
        if ((entire_insn & 0xffe007ff) == 0xc4000002)
          { itype = OR1K32BF_INSN_L_MSB; goto extract_sfmt_l_mac; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1571:
        if ((entire_insn & 0xffe007ff) == 0xc4000003)
          { itype = OR1K32BF_INSN_L_MACU; goto extract_sfmt_l_macu; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1572:
        if ((entire_insn & 0xffe007ff) == 0xc4000004)
          { itype = OR1K32BF_INSN_L_MSBU; goto extract_sfmt_l_macu; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1600:
        {
          unsigned int val1 = (((insn >> 5) & (7 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xfc0007ff) == 0xc8000000)
              { itype = OR1K32BF_INSN_LF_ADD_S; goto extract_sfmt_lf_add_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 7:
            if ((entire_insn & 0xffe004ff) == 0xc80000e0)
              { itype = OR1K32BF_INSN_LF_CUST1_D32; goto extract_sfmt_l_msync; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1601:
        if ((entire_insn & 0xfc0007ff) == 0xc8000001)
          { itype = OR1K32BF_INSN_LF_SUB_S; goto extract_sfmt_lf_add_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1602:
        if ((entire_insn & 0xfc0007ff) == 0xc8000002)
          { itype = OR1K32BF_INSN_LF_MUL_S; goto extract_sfmt_lf_add_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1603:
        if ((entire_insn & 0xfc0007ff) == 0xc8000003)
          { itype = OR1K32BF_INSN_LF_DIV_S; goto extract_sfmt_lf_add_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1604:
        if ((entire_insn & 0xfc00ffff) == 0xc8000004)
          { itype = OR1K32BF_INSN_LF_ITOF_S; goto extract_sfmt_lf_itof_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1605:
        if ((entire_insn & 0xfc00ffff) == 0xc8000005)
          { itype = OR1K32BF_INSN_LF_FTOI_S; goto extract_sfmt_lf_ftoi_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1606:
        if ((entire_insn & 0xfc0007ff) == 0xc8000006)
          { itype = OR1K32BF_INSN_LF_REM_S; goto extract_sfmt_lf_add_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1607:
        if ((entire_insn & 0xfc0007ff) == 0xc8000007)
          { itype = OR1K32BF_INSN_LF_MADD_S; goto extract_sfmt_lf_madd_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1608:
        {
          unsigned int val1 = (((insn >> 5) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffe007ff) == 0xc8000008)
              { itype = OR1K32BF_INSN_LF_SFEQ_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xffe007ff) == 0xc8000028)
              { itype = OR1K32BF_INSN_LF_SFUEQ_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1609:
        {
          unsigned int val1 = (((insn >> 5) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffe007ff) == 0xc8000009)
              { itype = OR1K32BF_INSN_LF_SFNE_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xffe007ff) == 0xc8000029)
              { itype = OR1K32BF_INSN_LF_SFUNE_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1610:
        {
          unsigned int val1 = (((insn >> 5) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffe007ff) == 0xc800000a)
              { itype = OR1K32BF_INSN_LF_SFGT_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xffe007ff) == 0xc800002a)
              { itype = OR1K32BF_INSN_LF_SFUGT_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1611:
        {
          unsigned int val1 = (((insn >> 5) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffe007ff) == 0xc800000b)
              { itype = OR1K32BF_INSN_LF_SFGE_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xffe007ff) == 0xc800002b)
              { itype = OR1K32BF_INSN_LF_SFUGE_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1612:
        {
          unsigned int val1 = (((insn >> 5) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffe007ff) == 0xc800000c)
              { itype = OR1K32BF_INSN_LF_SFLT_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xffe007ff) == 0xc800002c)
              { itype = OR1K32BF_INSN_LF_SFULT_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1613:
        {
          unsigned int val1 = (((insn >> 5) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffe007ff) == 0xc800000d)
              { itype = OR1K32BF_INSN_LF_SFLE_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xffe007ff) == 0xc800002d)
              { itype = OR1K32BF_INSN_LF_SFULE_S; goto extract_sfmt_lf_sfeq_s; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1614:
        if ((entire_insn & 0xffe007ff) == 0xc800002e)
          { itype = OR1K32BF_INSN_LF_SFUN_S; goto extract_sfmt_lf_sfeq_s; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1616:
        {
          unsigned int val1 = (((insn >> 6) & (3 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xfc0000ff) == 0xc8000010)
              { itype = OR1K32BF_INSN_LF_ADD_D32; goto extract_sfmt_lf_add_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 3:
            if ((entire_insn & 0xffe007ff) == 0xc80000d0)
              { itype = OR1K32BF_INSN_LF_CUST1_S; goto extract_sfmt_l_msync; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1617:
        if ((entire_insn & 0xfc0000ff) == 0xc8000011)
          { itype = OR1K32BF_INSN_LF_SUB_D32; goto extract_sfmt_lf_add_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1618:
        if ((entire_insn & 0xfc0000ff) == 0xc8000012)
          { itype = OR1K32BF_INSN_LF_MUL_D32; goto extract_sfmt_lf_add_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1619:
        if ((entire_insn & 0xfc0000ff) == 0xc8000013)
          { itype = OR1K32BF_INSN_LF_DIV_D32; goto extract_sfmt_lf_add_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1620:
        if ((entire_insn & 0xfc00f9ff) == 0xc8000014)
          { itype = OR1K32BF_INSN_LF_ITOF_D32; goto extract_sfmt_lf_itof_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1621:
        if ((entire_insn & 0xfc00f9ff) == 0xc8000015)
          { itype = OR1K32BF_INSN_LF_FTOI_D32; goto extract_sfmt_lf_ftoi_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1622:
        if ((entire_insn & 0xfc0000ff) == 0xc8000016)
          { itype = OR1K32BF_INSN_LF_REM_D32; goto extract_sfmt_lf_add_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1623:
        if ((entire_insn & 0xfc0000ff) == 0xc8000017)
          { itype = OR1K32BF_INSN_LF_MADD_D32; goto extract_sfmt_lf_madd_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1624:
        {
          unsigned int val1 = (((insn >> 5) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffe004ff) == 0xc8000018)
              { itype = OR1K32BF_INSN_LF_SFEQ_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xffe004ff) == 0xc8000038)
              { itype = OR1K32BF_INSN_LF_SFUEQ_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1625:
        {
          unsigned int val1 = (((insn >> 5) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffe004ff) == 0xc8000019)
              { itype = OR1K32BF_INSN_LF_SFNE_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xffe004ff) == 0xc8000039)
              { itype = OR1K32BF_INSN_LF_SFUNE_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1626:
        {
          unsigned int val1 = (((insn >> 5) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffe004ff) == 0xc800001a)
              { itype = OR1K32BF_INSN_LF_SFGT_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xffe004ff) == 0xc800003a)
              { itype = OR1K32BF_INSN_LF_SFUGT_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1627:
        {
          unsigned int val1 = (((insn >> 5) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffe004ff) == 0xc800001b)
              { itype = OR1K32BF_INSN_LF_SFGE_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xffe004ff) == 0xc800003b)
              { itype = OR1K32BF_INSN_LF_SFUGE_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1628:
        {
          unsigned int val1 = (((insn >> 5) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffe004ff) == 0xc800001c)
              { itype = OR1K32BF_INSN_LF_SFLT_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xffe004ff) == 0xc800003c)
              { itype = OR1K32BF_INSN_LF_SFULT_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1629:
        {
          unsigned int val1 = (((insn >> 5) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffe004ff) == 0xc800001d)
              { itype = OR1K32BF_INSN_LF_SFLE_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xffe004ff) == 0xc800003d)
              { itype = OR1K32BF_INSN_LF_SFULE_D32; goto extract_sfmt_lf_sfeq_d32; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1630:
        if ((entire_insn & 0xffe004ff) == 0xc800003e)
          { itype = OR1K32BF_INSN_LF_SFUN_D32; goto extract_sfmt_lf_sfeq_d32; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1632:
      case 1633:
      case 1634:
      case 1635:
      case 1636:
      case 1637:
      case 1638:
      case 1639:
      case 1640:
      case 1641:
      case 1642:
      case 1643:
      case 1644:
      case 1645:
      case 1646:
      case 1647:
      case 1648:
      case 1649:
      case 1650:
      case 1651:
      case 1652:
      case 1653:
      case 1654:
      case 1655:
      case 1656:
      case 1657:
      case 1658:
      case 1659:
      case 1660:
      case 1661:
      case 1662:
      case 1663: itype = OR1K32BF_INSN_L_SWA; goto extract_sfmt_l_swa;
      case 1696:
      case 1697:
      case 1698:
      case 1699:
      case 1700:
      case 1701:
      case 1702:
      case 1703:
      case 1704:
      case 1705:
      case 1706:
      case 1707:
      case 1708:
      case 1709:
      case 1710:
      case 1711:
      case 1712:
      case 1713:
      case 1714:
      case 1715:
      case 1716:
      case 1717:
      case 1718:
      case 1719:
      case 1720:
      case 1721:
      case 1722:
      case 1723:
      case 1724:
      case 1725:
      case 1726:
      case 1727: itype = OR1K32BF_INSN_L_SW; goto extract_sfmt_l_sw;
      case 1728:
      case 1729:
      case 1730:
      case 1731:
      case 1732:
      case 1733:
      case 1734:
      case 1735:
      case 1736:
      case 1737:
      case 1738:
      case 1739:
      case 1740:
      case 1741:
      case 1742:
      case 1743:
      case 1744:
      case 1745:
      case 1746:
      case 1747:
      case 1748:
      case 1749:
      case 1750:
      case 1751:
      case 1752:
      case 1753:
      case 1754:
      case 1755:
      case 1756:
      case 1757:
      case 1758:
      case 1759: itype = OR1K32BF_INSN_L_SB; goto extract_sfmt_l_sb;
      case 1760:
      case 1761:
      case 1762:
      case 1763:
      case 1764:
      case 1765:
      case 1766:
      case 1767:
      case 1768:
      case 1769:
      case 1770:
      case 1771:
      case 1772:
      case 1773:
      case 1774:
      case 1775:
      case 1776:
      case 1777:
      case 1778:
      case 1779:
      case 1780:
      case 1781:
      case 1782:
      case 1783:
      case 1784:
      case 1785:
      case 1786:
      case 1787:
      case 1788:
      case 1789:
      case 1790:
      case 1791: itype = OR1K32BF_INSN_L_SH; goto extract_sfmt_l_sh;
      case 1792:
        if ((entire_insn & 0xfc0007ff) == 0xe0000000)
          { itype = OR1K32BF_INSN_L_ADD; goto extract_sfmt_l_add; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1793:
        if ((entire_insn & 0xfc0007ff) == 0xe0000001)
          { itype = OR1K32BF_INSN_L_ADDC; goto extract_sfmt_l_addc; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1794:
        if ((entire_insn & 0xfc0007ff) == 0xe0000002)
          { itype = OR1K32BF_INSN_L_SUB; goto extract_sfmt_l_add; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1795:
        if ((entire_insn & 0xfc0007ff) == 0xe0000003)
          { itype = OR1K32BF_INSN_L_AND; goto extract_sfmt_l_and; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1796:
        if ((entire_insn & 0xfc0007ff) == 0xe0000004)
          { itype = OR1K32BF_INSN_L_OR; goto extract_sfmt_l_and; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1797:
        if ((entire_insn & 0xfc0007ff) == 0xe0000005)
          { itype = OR1K32BF_INSN_L_XOR; goto extract_sfmt_l_and; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1798:
        if ((entire_insn & 0xfc0007ff) == 0xe0000306)
          { itype = OR1K32BF_INSN_L_MUL; goto extract_sfmt_l_mul; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1799:
        if ((entire_insn & 0xffe007ff) == 0xe0000307)
          { itype = OR1K32BF_INSN_L_MULD; goto extract_sfmt_l_muld; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1800:
        {
          unsigned int val1 = (((insn >> 6) & (3 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xfc0007ff) == 0xe0000008)
              { itype = OR1K32BF_INSN_L_SLL; goto extract_sfmt_l_sll; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xfc0007ff) == 0xe0000048)
              { itype = OR1K32BF_INSN_L_SRL; goto extract_sfmt_l_sll; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 2:
            if ((entire_insn & 0xfc0007ff) == 0xe0000088)
              { itype = OR1K32BF_INSN_L_SRA; goto extract_sfmt_l_sll; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 3:
            if ((entire_insn & 0xfc0007ff) == 0xe00000c8)
              { itype = OR1K32BF_INSN_L_ROR; goto extract_sfmt_l_sll; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1801:
        if ((entire_insn & 0xfc0007ff) == 0xe0000309)
          { itype = OR1K32BF_INSN_L_DIV; goto extract_sfmt_l_div; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1802:
        if ((entire_insn & 0xfc0007ff) == 0xe000030a)
          { itype = OR1K32BF_INSN_L_DIVU; goto extract_sfmt_l_divu; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1803:
        if ((entire_insn & 0xfc0007ff) == 0xe000030b)
          { itype = OR1K32BF_INSN_L_MULU; goto extract_sfmt_l_mulu; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1804:
        {
          unsigned int val1 = (((insn >> 6) & (3 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xfc00ffff) == 0xe000000c)
              { itype = OR1K32BF_INSN_L_EXTHS; goto extract_sfmt_l_exths; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xfc00ffff) == 0xe000004c)
              { itype = OR1K32BF_INSN_L_EXTBS; goto extract_sfmt_l_exths; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 2:
            if ((entire_insn & 0xfc00ffff) == 0xe000008c)
              { itype = OR1K32BF_INSN_L_EXTHZ; goto extract_sfmt_l_exths; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 3:
            if ((entire_insn & 0xfc00ffff) == 0xe00000cc)
              { itype = OR1K32BF_INSN_L_EXTBZ; goto extract_sfmt_l_exths; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1805:
        {
          unsigned int val1 = (((insn >> 7) & (3 << 1)) | ((insn >> 6) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xfc00ffff) == 0xe000000d)
              { itype = OR1K32BF_INSN_L_EXTWS; goto extract_sfmt_l_exths; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xfc00ffff) == 0xe000004d)
              { itype = OR1K32BF_INSN_L_EXTWZ; goto extract_sfmt_l_exths; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 6:
            if ((entire_insn & 0xffe007ff) == 0xe000030d)
              { itype = OR1K32BF_INSN_L_MULDU; goto extract_sfmt_l_muld; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1806:
        if ((entire_insn & 0xfc0007ff) == 0xe000000e)
          { itype = OR1K32BF_INSN_L_CMOV; goto extract_sfmt_l_cmov; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1807:
        {
          unsigned int val1 = (((insn >> 8) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xfc0007ff) == 0xe000000f)
              { itype = OR1K32BF_INSN_L_FF1; goto extract_sfmt_l_ff1; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xfc0007ff) == 0xe000010f)
              { itype = OR1K32BF_INSN_L_FL1; goto extract_sfmt_l_ff1; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1824:
        {
          unsigned int val1 = (((insn >> 21) & (15 << 0)));
          switch (val1)
          {
          case 0:
            if ((entire_insn & 0xffe007ff) == 0xe4000000)
              { itype = OR1K32BF_INSN_L_SFEQ; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((entire_insn & 0xffe007ff) == 0xe4200000)
              { itype = OR1K32BF_INSN_L_SFNE; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 2:
            if ((entire_insn & 0xffe007ff) == 0xe4400000)
              { itype = OR1K32BF_INSN_L_SFGTU; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 3:
            if ((entire_insn & 0xffe007ff) == 0xe4600000)
              { itype = OR1K32BF_INSN_L_SFGEU; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 4:
            if ((entire_insn & 0xffe007ff) == 0xe4800000)
              { itype = OR1K32BF_INSN_L_SFLTU; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 5:
            if ((entire_insn & 0xffe007ff) == 0xe4a00000)
              { itype = OR1K32BF_INSN_L_SFLEU; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 10:
            if ((entire_insn & 0xffe007ff) == 0xe5400000)
              { itype = OR1K32BF_INSN_L_SFGTS; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 11:
            if ((entire_insn & 0xffe007ff) == 0xe5600000)
              { itype = OR1K32BF_INSN_L_SFGES; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 12:
            if ((entire_insn & 0xffe007ff) == 0xe5800000)
              { itype = OR1K32BF_INSN_L_SFLTS; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          case 13:
            if ((entire_insn & 0xffe007ff) == 0xe5a00000)
              { itype = OR1K32BF_INSN_L_SFLES; goto extract_sfmt_l_sfgts; }
            itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 1920:
        if ((entire_insn & 0xffffffff) == 0xf0000000)
          { itype = OR1K32BF_INSN_L_CUST5; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1952:
        if ((entire_insn & 0xffffffff) == 0xf4000000)
          { itype = OR1K32BF_INSN_L_CUST6; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 1984:
        if ((entire_insn & 0xffffffff) == 0xf8000000)
          { itype = OR1K32BF_INSN_L_CUST7; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      case 2016:
        if ((entire_insn & 0xffffffff) == 0xfc000000)
          { itype = OR1K32BF_INSN_L_CUST8; goto extract_sfmt_l_msync; }
        itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      default: itype = OR1K32BF_INSN_X_INVALID; goto extract_sfmt_empty;
      }
    }
  }

  /* The instruction has been decoded, now extract the fields.  */

 extract_sfmt_empty:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_empty", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_j:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_j.f
    USI f_disp26;

    f_disp26 = ((((EXTRACT_LSB0_SINT (insn, 32, 25, 26)) * (4))) + (pc));

  /* Record the fields for the semantic handler.  */
  FLD (i_disp26) = f_disp26;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_j", "disp26 0x%x", 'x', f_disp26, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_adrp:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_adrp.f
    UINT f_r1;
    USI f_disp21;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_disp21 = ((((EXTRACT_LSB0_SINT (insn, 32, 20, 21)) + (((SI) (pc) >> (13))))) * (8192));

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  FLD (i_disp21) = f_disp21;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_adrp", "f_r1 0x%x", 'x', f_r1, "disp21 0x%x", 'x', f_disp21, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_jal:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_j.f
    USI f_disp26;

    f_disp26 = ((((EXTRACT_LSB0_SINT (insn, 32, 25, 26)) * (4))) + (pc));

  /* Record the fields for the semantic handler.  */
  FLD (i_disp26) = f_disp26;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_jal", "disp26 0x%x", 'x', f_disp26, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_jr:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r3;

    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r3) = f_r3;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_jr", "f_r3 0x%x", 'x', f_r3, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_jalr:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r3;

    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r3) = f_r3;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_jalr", "f_r3 0x%x", 'x', f_r3, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_bnf:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_j.f
    USI f_disp26;

    f_disp26 = ((((EXTRACT_LSB0_SINT (insn, 32, 25, 26)) * (4))) + (pc));

  /* Record the fields for the semantic handler.  */
  FLD (i_disp26) = f_disp26;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_bnf", "disp26 0x%x", 'x', f_disp26, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_trap:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_trap", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_msync:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_msync", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_nop_imm:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
    UINT f_uimm16;

    f_uimm16 = EXTRACT_LSB0_UINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_uimm16) = f_uimm16;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_nop_imm", "f_uimm16 0x%x", 'x', f_uimm16, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_movhi:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
    UINT f_r1;
    UINT f_uimm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_uimm16 = EXTRACT_LSB0_UINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_uimm16) = f_uimm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_movhi", "f_uimm16 0x%x", 'x', f_uimm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_macrc:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_adrp.f
    UINT f_r1;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_macrc", "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_mfspr:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_mfspr.f
    UINT f_r1;
    UINT f_r2;
    UINT f_uimm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_uimm16 = EXTRACT_LSB0_UINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_uimm16) = f_uimm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_mfspr", "f_r2 0x%x", 'x', f_r2, "f_uimm16 0x%x", 'x', f_uimm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_mtspr:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_mtspr.f
    UINT f_imm16_25_5;
    UINT f_r2;
    UINT f_r3;
    UINT f_imm16_10_11;
    UINT f_uimm16_split;

    f_imm16_25_5 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_imm16_10_11 = EXTRACT_LSB0_UINT (insn, 32, 10, 11);
  f_uimm16_split = ((UHI) (UINT) (((((f_imm16_25_5) << (11))) | (f_imm16_10_11))));

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_uimm16_split) = f_uimm16_split;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_mtspr", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_uimm16_split 0x%x", 'x', f_uimm16_split, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_lwz:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_lwz", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_lws:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_lws", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_lwa:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_lwa", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_lbz:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_lbz", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_lbs:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_lbs", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_lhz:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_lhz", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_lhs:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_lhs", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_sw:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sw.f
    UINT f_imm16_25_5;
    UINT f_r2;
    UINT f_r3;
    UINT f_imm16_10_11;
    INT f_simm16_split;

    f_imm16_25_5 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_imm16_10_11 = EXTRACT_LSB0_UINT (insn, 32, 10, 11);
  f_simm16_split = ((HI) (UINT) (((((f_imm16_25_5) << (11))) | (f_imm16_10_11))));

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_simm16_split) = f_simm16_split;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_sw", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_simm16_split 0x%x", 'x', f_simm16_split, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_sb:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sw.f
    UINT f_imm16_25_5;
    UINT f_r2;
    UINT f_r3;
    UINT f_imm16_10_11;
    INT f_simm16_split;

    f_imm16_25_5 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_imm16_10_11 = EXTRACT_LSB0_UINT (insn, 32, 10, 11);
  f_simm16_split = ((HI) (UINT) (((((f_imm16_25_5) << (11))) | (f_imm16_10_11))));

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_simm16_split) = f_simm16_split;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_sb", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_simm16_split 0x%x", 'x', f_simm16_split, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_sh:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sw.f
    UINT f_imm16_25_5;
    UINT f_r2;
    UINT f_r3;
    UINT f_imm16_10_11;
    INT f_simm16_split;

    f_imm16_25_5 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_imm16_10_11 = EXTRACT_LSB0_UINT (insn, 32, 10, 11);
  f_simm16_split = ((HI) (UINT) (((((f_imm16_25_5) << (11))) | (f_imm16_10_11))));

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_simm16_split) = f_simm16_split;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_sh", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_simm16_split 0x%x", 'x', f_simm16_split, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_swa:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sw.f
    UINT f_imm16_25_5;
    UINT f_r2;
    UINT f_r3;
    UINT f_imm16_10_11;
    INT f_simm16_split;

    f_imm16_25_5 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_imm16_10_11 = EXTRACT_LSB0_UINT (insn, 32, 10, 11);
  f_simm16_split = ((HI) (UINT) (((((f_imm16_25_5) << (11))) | (f_imm16_10_11))));

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_simm16_split) = f_simm16_split;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_swa", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_simm16_split 0x%x", 'x', f_simm16_split, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_sll:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_sll", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_slli:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_slli.f
    UINT f_r1;
    UINT f_r2;
    UINT f_uimm6;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_uimm6 = EXTRACT_LSB0_UINT (insn, 32, 5, 6);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_uimm6) = f_uimm6;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_slli", "f_r2 0x%x", 'x', f_r2, "f_uimm6 0x%x", 'x', f_uimm6, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_and:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_and", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_add:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_add", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_addc:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_addc", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_mul:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_mul", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_muld:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r2;
    UINT f_r3;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_muld", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_mulu:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_mulu", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_div:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_div", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_divu:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_divu", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_ff1:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_slli.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_ff1", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_xori:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_xori", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_addi:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_addi", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_addic:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_addic", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_muli:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r1;
    UINT f_r2;
    INT f_simm16;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_muli", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_exths:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_slli.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_exths", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_cmov:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_cmov", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_sfgts:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r2;
    UINT f_r3;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_sfgts", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_sfgtsi:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r2;
    INT f_simm16;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_sfgtsi", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_mac:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r2;
    UINT f_r3;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_mac", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_maci:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_lwz.f
    UINT f_r2;
    INT f_simm16;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_simm16 = EXTRACT_LSB0_SINT (insn, 32, 15, 16);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_simm16) = f_simm16;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_maci", "f_r2 0x%x", 'x', f_r2, "f_simm16 0x%x", 'x', f_simm16, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_l_macu:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r2;
    UINT f_r3;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_l_macu", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_add_s:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_add_s", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_add_d32:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;
    UINT f_rdoff_10_1;
    UINT f_raoff_9_1;
    UINT f_rboff_8_1;
    SI f_rdd32;
    SI f_rad32;
    SI f_rbd32;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_rdoff_10_1 = EXTRACT_LSB0_UINT (insn, 32, 10, 1);
    f_raoff_9_1 = EXTRACT_LSB0_UINT (insn, 32, 9, 1);
    f_rboff_8_1 = EXTRACT_LSB0_UINT (insn, 32, 8, 1);
  f_rdd32 = ((f_r1) | (((f_rdoff_10_1) << (5))));
  f_rad32 = ((f_r2) | (((f_raoff_9_1) << (5))));
  f_rbd32 = ((f_r3) | (((f_rboff_8_1) << (5))));

  /* Record the fields for the semantic handler.  */
  FLD (f_rad32) = f_rad32;
  FLD (f_rbd32) = f_rbd32;
  FLD (f_rdd32) = f_rdd32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_add_d32", "f_rad32 0x%x", 'x', f_rad32, "f_rbd32 0x%x", 'x', f_rbd32, "f_rdd32 0x%x", 'x', f_rdd32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_itof_s:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_slli.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_itof_s", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_itof_d32:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
    UINT f_r1;
    UINT f_r2;
    UINT f_rdoff_10_1;
    UINT f_raoff_9_1;
    SI f_rdd32;
    SI f_rad32;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_rdoff_10_1 = EXTRACT_LSB0_UINT (insn, 32, 10, 1);
    f_raoff_9_1 = EXTRACT_LSB0_UINT (insn, 32, 9, 1);
  f_rdd32 = ((f_r1) | (((f_rdoff_10_1) << (5))));
  f_rad32 = ((f_r2) | (((f_raoff_9_1) << (5))));

  /* Record the fields for the semantic handler.  */
  FLD (f_rad32) = f_rad32;
  FLD (f_rdd32) = f_rdd32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_itof_d32", "f_rad32 0x%x", 'x', f_rad32, "f_rdd32 0x%x", 'x', f_rdd32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_ftoi_s:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_slli.f
    UINT f_r1;
    UINT f_r2;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_ftoi_s", "f_r2 0x%x", 'x', f_r2, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_ftoi_d32:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
    UINT f_r1;
    UINT f_r2;
    UINT f_rdoff_10_1;
    UINT f_raoff_9_1;
    SI f_rdd32;
    SI f_rad32;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_rdoff_10_1 = EXTRACT_LSB0_UINT (insn, 32, 10, 1);
    f_raoff_9_1 = EXTRACT_LSB0_UINT (insn, 32, 9, 1);
  f_rdd32 = ((f_r1) | (((f_rdoff_10_1) << (5))));
  f_rad32 = ((f_r2) | (((f_raoff_9_1) << (5))));

  /* Record the fields for the semantic handler.  */
  FLD (f_rad32) = f_rad32;
  FLD (f_rdd32) = f_rdd32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_ftoi_d32", "f_rad32 0x%x", 'x', f_rad32, "f_rdd32 0x%x", 'x', f_rdd32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_sfeq_s:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r2;
    UINT f_r3;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_sfeq_s", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_sfeq_d32:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
    UINT f_r2;
    UINT f_r3;
    UINT f_raoff_9_1;
    UINT f_rboff_8_1;
    SI f_rad32;
    SI f_rbd32;

    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_raoff_9_1 = EXTRACT_LSB0_UINT (insn, 32, 9, 1);
    f_rboff_8_1 = EXTRACT_LSB0_UINT (insn, 32, 8, 1);
  f_rad32 = ((f_r2) | (((f_raoff_9_1) << (5))));
  f_rbd32 = ((f_r3) | (((f_rboff_8_1) << (5))));

  /* Record the fields for the semantic handler.  */
  FLD (f_rad32) = f_rad32;
  FLD (f_rbd32) = f_rbd32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_sfeq_d32", "f_rad32 0x%x", 'x', f_rad32, "f_rbd32 0x%x", 'x', f_rbd32, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_madd_s:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_l_sll.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_r2) = f_r2;
  FLD (f_r3) = f_r3;
  FLD (f_r1) = f_r1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_madd_s", "f_r2 0x%x", 'x', f_r2, "f_r3 0x%x", 'x', f_r3, "f_r1 0x%x", 'x', f_r1, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_lf_madd_d32:
  {
    const IDESC *idesc = &or1k32bf_insn_data[itype];
    CGEN_INSN_WORD insn = entire_insn;
#define FLD(f) abuf->fields.sfmt_lf_add_d32.f
    UINT f_r1;
    UINT f_r2;
    UINT f_r3;
    UINT f_rdoff_10_1;
    UINT f_raoff_9_1;
    UINT f_rboff_8_1;
    SI f_rdd32;
    SI f_rad32;
    SI f_rbd32;

    f_r1 = EXTRACT_LSB0_UINT (insn, 32, 25, 5);
    f_r2 = EXTRACT_LSB0_UINT (insn, 32, 20, 5);
    f_r3 = EXTRACT_LSB0_UINT (insn, 32, 15, 5);
    f_rdoff_10_1 = EXTRACT_LSB0_UINT (insn, 32, 10, 1);
    f_raoff_9_1 = EXTRACT_LSB0_UINT (insn, 32, 9, 1);
    f_rboff_8_1 = EXTRACT_LSB0_UINT (insn, 32, 8, 1);
  f_rdd32 = ((f_r1) | (((f_rdoff_10_1) << (5))));
  f_rad32 = ((f_r2) | (((f_raoff_9_1) << (5))));
  f_rbd32 = ((f_r3) | (((f_rboff_8_1) << (5))));

  /* Record the fields for the semantic handler.  */
  FLD (f_rad32) = f_rad32;
  FLD (f_rbd32) = f_rbd32;
  FLD (f_rdd32) = f_rdd32;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lf_madd_d32", "f_rad32 0x%x", 'x', f_rad32, "f_rbd32 0x%x", 'x', f_rbd32, "f_rdd32 0x%x", 'x', f_rdd32, (char *) 0));

#undef FLD
    return idesc;
  }

}
