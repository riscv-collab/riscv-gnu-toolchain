/* Simulator instruction decoder for crisv32f.

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

#define WANT_CPU crisv32f
#define WANT_CPU_CRISV32F

#include "sim-main.h"
#include "sim-assert.h"
#include "cgen-mem.h"
#include "cgen-ops.h"

/* The instruction descriptor array.
   This is computed at runtime.  Space for it is not malloc'd to save a
   teensy bit of cpu in the decoder.  Moving it to malloc space is trivial
   but won't be done until necessary (we don't currently support the runtime
   addition of instructions nor an SMP machine with different cpus).  */
static IDESC crisv32f_insn_data[CRISV32F_INSN__MAX];

/* Commas between elements are contained in the macros.
   Some of these are conditionally compiled out.  */

static const struct insn_sem crisv32f_insn_sem[] =
{
  { VIRTUAL_INSN_X_INVALID, CRISV32F_INSN_X_INVALID, CRISV32F_SFMT_EMPTY },
  { VIRTUAL_INSN_X_AFTER, CRISV32F_INSN_X_AFTER, CRISV32F_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEFORE, CRISV32F_INSN_X_BEFORE, CRISV32F_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CTI_CHAIN, CRISV32F_INSN_X_CTI_CHAIN, CRISV32F_SFMT_EMPTY },
  { VIRTUAL_INSN_X_CHAIN, CRISV32F_INSN_X_CHAIN, CRISV32F_SFMT_EMPTY },
  { VIRTUAL_INSN_X_BEGIN, CRISV32F_INSN_X_BEGIN, CRISV32F_SFMT_EMPTY },
  { CRIS_INSN_MOVE_B_R, CRISV32F_INSN_MOVE_B_R, CRISV32F_SFMT_MOVE_B_R },
  { CRIS_INSN_MOVE_W_R, CRISV32F_INSN_MOVE_W_R, CRISV32F_SFMT_MOVE_B_R },
  { CRIS_INSN_MOVE_D_R, CRISV32F_INSN_MOVE_D_R, CRISV32F_SFMT_MOVE_D_R },
  { CRIS_INSN_MOVEQ, CRISV32F_INSN_MOVEQ, CRISV32F_SFMT_MOVEQ },
  { CRIS_INSN_MOVS_B_R, CRISV32F_INSN_MOVS_B_R, CRISV32F_SFMT_MOVS_B_R },
  { CRIS_INSN_MOVS_W_R, CRISV32F_INSN_MOVS_W_R, CRISV32F_SFMT_MOVS_B_R },
  { CRIS_INSN_MOVU_B_R, CRISV32F_INSN_MOVU_B_R, CRISV32F_SFMT_MOVS_B_R },
  { CRIS_INSN_MOVU_W_R, CRISV32F_INSN_MOVU_W_R, CRISV32F_SFMT_MOVS_B_R },
  { CRIS_INSN_MOVECBR, CRISV32F_INSN_MOVECBR, CRISV32F_SFMT_MOVECBR },
  { CRIS_INSN_MOVECWR, CRISV32F_INSN_MOVECWR, CRISV32F_SFMT_MOVECWR },
  { CRIS_INSN_MOVECDR, CRISV32F_INSN_MOVECDR, CRISV32F_SFMT_MOVECDR },
  { CRIS_INSN_MOVSCBR, CRISV32F_INSN_MOVSCBR, CRISV32F_SFMT_MOVSCBR },
  { CRIS_INSN_MOVSCWR, CRISV32F_INSN_MOVSCWR, CRISV32F_SFMT_MOVSCWR },
  { CRIS_INSN_MOVUCBR, CRISV32F_INSN_MOVUCBR, CRISV32F_SFMT_MOVUCBR },
  { CRIS_INSN_MOVUCWR, CRISV32F_INSN_MOVUCWR, CRISV32F_SFMT_MOVUCWR },
  { CRIS_INSN_ADDQ, CRISV32F_INSN_ADDQ, CRISV32F_SFMT_ADDQ },
  { CRIS_INSN_SUBQ, CRISV32F_INSN_SUBQ, CRISV32F_SFMT_ADDQ },
  { CRIS_INSN_CMP_R_B_R, CRISV32F_INSN_CMP_R_B_R, CRISV32F_SFMT_CMP_R_B_R },
  { CRIS_INSN_CMP_R_W_R, CRISV32F_INSN_CMP_R_W_R, CRISV32F_SFMT_CMP_R_B_R },
  { CRIS_INSN_CMP_R_D_R, CRISV32F_INSN_CMP_R_D_R, CRISV32F_SFMT_CMP_R_B_R },
  { CRIS_INSN_CMP_M_B_M, CRISV32F_INSN_CMP_M_B_M, CRISV32F_SFMT_CMP_M_B_M },
  { CRIS_INSN_CMP_M_W_M, CRISV32F_INSN_CMP_M_W_M, CRISV32F_SFMT_CMP_M_W_M },
  { CRIS_INSN_CMP_M_D_M, CRISV32F_INSN_CMP_M_D_M, CRISV32F_SFMT_CMP_M_D_M },
  { CRIS_INSN_CMPCBR, CRISV32F_INSN_CMPCBR, CRISV32F_SFMT_CMPCBR },
  { CRIS_INSN_CMPCWR, CRISV32F_INSN_CMPCWR, CRISV32F_SFMT_CMPCWR },
  { CRIS_INSN_CMPCDR, CRISV32F_INSN_CMPCDR, CRISV32F_SFMT_CMPCDR },
  { CRIS_INSN_CMPQ, CRISV32F_INSN_CMPQ, CRISV32F_SFMT_CMPQ },
  { CRIS_INSN_CMPS_M_B_M, CRISV32F_INSN_CMPS_M_B_M, CRISV32F_SFMT_CMP_M_B_M },
  { CRIS_INSN_CMPS_M_W_M, CRISV32F_INSN_CMPS_M_W_M, CRISV32F_SFMT_CMP_M_W_M },
  { CRIS_INSN_CMPSCBR, CRISV32F_INSN_CMPSCBR, CRISV32F_SFMT_CMPCBR },
  { CRIS_INSN_CMPSCWR, CRISV32F_INSN_CMPSCWR, CRISV32F_SFMT_CMPCWR },
  { CRIS_INSN_CMPU_M_B_M, CRISV32F_INSN_CMPU_M_B_M, CRISV32F_SFMT_CMP_M_B_M },
  { CRIS_INSN_CMPU_M_W_M, CRISV32F_INSN_CMPU_M_W_M, CRISV32F_SFMT_CMP_M_W_M },
  { CRIS_INSN_CMPUCBR, CRISV32F_INSN_CMPUCBR, CRISV32F_SFMT_CMPUCBR },
  { CRIS_INSN_CMPUCWR, CRISV32F_INSN_CMPUCWR, CRISV32F_SFMT_CMPUCWR },
  { CRIS_INSN_MOVE_M_B_M, CRISV32F_INSN_MOVE_M_B_M, CRISV32F_SFMT_MOVE_M_B_M },
  { CRIS_INSN_MOVE_M_W_M, CRISV32F_INSN_MOVE_M_W_M, CRISV32F_SFMT_MOVE_M_W_M },
  { CRIS_INSN_MOVE_M_D_M, CRISV32F_INSN_MOVE_M_D_M, CRISV32F_SFMT_MOVE_M_D_M },
  { CRIS_INSN_MOVS_M_B_M, CRISV32F_INSN_MOVS_M_B_M, CRISV32F_SFMT_MOVS_M_B_M },
  { CRIS_INSN_MOVS_M_W_M, CRISV32F_INSN_MOVS_M_W_M, CRISV32F_SFMT_MOVS_M_W_M },
  { CRIS_INSN_MOVU_M_B_M, CRISV32F_INSN_MOVU_M_B_M, CRISV32F_SFMT_MOVS_M_B_M },
  { CRIS_INSN_MOVU_M_W_M, CRISV32F_INSN_MOVU_M_W_M, CRISV32F_SFMT_MOVS_M_W_M },
  { CRIS_INSN_MOVE_R_SPRV32, CRISV32F_INSN_MOVE_R_SPRV32, CRISV32F_SFMT_MOVE_R_SPRV32 },
  { CRIS_INSN_MOVE_SPR_RV32, CRISV32F_INSN_MOVE_SPR_RV32, CRISV32F_SFMT_MOVE_SPR_RV32 },
  { CRIS_INSN_MOVE_M_SPRV32, CRISV32F_INSN_MOVE_M_SPRV32, CRISV32F_SFMT_MOVE_M_SPRV32 },
  { CRIS_INSN_MOVE_C_SPRV32_P2, CRISV32F_INSN_MOVE_C_SPRV32_P2, CRISV32F_SFMT_MOVE_C_SPRV32_P2 },
  { CRIS_INSN_MOVE_C_SPRV32_P3, CRISV32F_INSN_MOVE_C_SPRV32_P3, CRISV32F_SFMT_MOVE_C_SPRV32_P2 },
  { CRIS_INSN_MOVE_C_SPRV32_P5, CRISV32F_INSN_MOVE_C_SPRV32_P5, CRISV32F_SFMT_MOVE_C_SPRV32_P2 },
  { CRIS_INSN_MOVE_C_SPRV32_P6, CRISV32F_INSN_MOVE_C_SPRV32_P6, CRISV32F_SFMT_MOVE_C_SPRV32_P2 },
  { CRIS_INSN_MOVE_C_SPRV32_P7, CRISV32F_INSN_MOVE_C_SPRV32_P7, CRISV32F_SFMT_MOVE_C_SPRV32_P2 },
  { CRIS_INSN_MOVE_C_SPRV32_P9, CRISV32F_INSN_MOVE_C_SPRV32_P9, CRISV32F_SFMT_MOVE_C_SPRV32_P2 },
  { CRIS_INSN_MOVE_C_SPRV32_P10, CRISV32F_INSN_MOVE_C_SPRV32_P10, CRISV32F_SFMT_MOVE_C_SPRV32_P2 },
  { CRIS_INSN_MOVE_C_SPRV32_P11, CRISV32F_INSN_MOVE_C_SPRV32_P11, CRISV32F_SFMT_MOVE_C_SPRV32_P2 },
  { CRIS_INSN_MOVE_C_SPRV32_P12, CRISV32F_INSN_MOVE_C_SPRV32_P12, CRISV32F_SFMT_MOVE_C_SPRV32_P2 },
  { CRIS_INSN_MOVE_C_SPRV32_P13, CRISV32F_INSN_MOVE_C_SPRV32_P13, CRISV32F_SFMT_MOVE_C_SPRV32_P2 },
  { CRIS_INSN_MOVE_C_SPRV32_P14, CRISV32F_INSN_MOVE_C_SPRV32_P14, CRISV32F_SFMT_MOVE_C_SPRV32_P2 },
  { CRIS_INSN_MOVE_C_SPRV32_P15, CRISV32F_INSN_MOVE_C_SPRV32_P15, CRISV32F_SFMT_MOVE_C_SPRV32_P2 },
  { CRIS_INSN_MOVE_SPR_MV32, CRISV32F_INSN_MOVE_SPR_MV32, CRISV32F_SFMT_MOVE_SPR_MV32 },
  { CRIS_INSN_MOVE_SS_R, CRISV32F_INSN_MOVE_SS_R, CRISV32F_SFMT_MOVE_SS_R },
  { CRIS_INSN_MOVE_R_SS, CRISV32F_INSN_MOVE_R_SS, CRISV32F_SFMT_MOVE_R_SS },
  { CRIS_INSN_MOVEM_R_M_V32, CRISV32F_INSN_MOVEM_R_M_V32, CRISV32F_SFMT_MOVEM_R_M_V32 },
  { CRIS_INSN_MOVEM_M_R_V32, CRISV32F_INSN_MOVEM_M_R_V32, CRISV32F_SFMT_MOVEM_M_R_V32 },
  { CRIS_INSN_ADD_B_R, CRISV32F_INSN_ADD_B_R, CRISV32F_SFMT_ADD_B_R },
  { CRIS_INSN_ADD_W_R, CRISV32F_INSN_ADD_W_R, CRISV32F_SFMT_ADD_B_R },
  { CRIS_INSN_ADD_D_R, CRISV32F_INSN_ADD_D_R, CRISV32F_SFMT_ADD_D_R },
  { CRIS_INSN_ADD_M_B_M, CRISV32F_INSN_ADD_M_B_M, CRISV32F_SFMT_ADD_M_B_M },
  { CRIS_INSN_ADD_M_W_M, CRISV32F_INSN_ADD_M_W_M, CRISV32F_SFMT_ADD_M_W_M },
  { CRIS_INSN_ADD_M_D_M, CRISV32F_INSN_ADD_M_D_M, CRISV32F_SFMT_ADD_M_D_M },
  { CRIS_INSN_ADDCBR, CRISV32F_INSN_ADDCBR, CRISV32F_SFMT_ADDCBR },
  { CRIS_INSN_ADDCWR, CRISV32F_INSN_ADDCWR, CRISV32F_SFMT_ADDCWR },
  { CRIS_INSN_ADDCDR, CRISV32F_INSN_ADDCDR, CRISV32F_SFMT_ADDCDR },
  { CRIS_INSN_ADDS_B_R, CRISV32F_INSN_ADDS_B_R, CRISV32F_SFMT_ADD_D_R },
  { CRIS_INSN_ADDS_W_R, CRISV32F_INSN_ADDS_W_R, CRISV32F_SFMT_ADD_D_R },
  { CRIS_INSN_ADDS_M_B_M, CRISV32F_INSN_ADDS_M_B_M, CRISV32F_SFMT_ADDS_M_B_M },
  { CRIS_INSN_ADDS_M_W_M, CRISV32F_INSN_ADDS_M_W_M, CRISV32F_SFMT_ADDS_M_W_M },
  { CRIS_INSN_ADDSCBR, CRISV32F_INSN_ADDSCBR, CRISV32F_SFMT_ADDSCBR },
  { CRIS_INSN_ADDSCWR, CRISV32F_INSN_ADDSCWR, CRISV32F_SFMT_ADDSCWR },
  { CRIS_INSN_ADDU_B_R, CRISV32F_INSN_ADDU_B_R, CRISV32F_SFMT_ADD_D_R },
  { CRIS_INSN_ADDU_W_R, CRISV32F_INSN_ADDU_W_R, CRISV32F_SFMT_ADD_D_R },
  { CRIS_INSN_ADDU_M_B_M, CRISV32F_INSN_ADDU_M_B_M, CRISV32F_SFMT_ADDS_M_B_M },
  { CRIS_INSN_ADDU_M_W_M, CRISV32F_INSN_ADDU_M_W_M, CRISV32F_SFMT_ADDS_M_W_M },
  { CRIS_INSN_ADDUCBR, CRISV32F_INSN_ADDUCBR, CRISV32F_SFMT_ADDSCBR },
  { CRIS_INSN_ADDUCWR, CRISV32F_INSN_ADDUCWR, CRISV32F_SFMT_ADDSCWR },
  { CRIS_INSN_SUB_B_R, CRISV32F_INSN_SUB_B_R, CRISV32F_SFMT_ADD_B_R },
  { CRIS_INSN_SUB_W_R, CRISV32F_INSN_SUB_W_R, CRISV32F_SFMT_ADD_B_R },
  { CRIS_INSN_SUB_D_R, CRISV32F_INSN_SUB_D_R, CRISV32F_SFMT_ADD_D_R },
  { CRIS_INSN_SUB_M_B_M, CRISV32F_INSN_SUB_M_B_M, CRISV32F_SFMT_ADD_M_B_M },
  { CRIS_INSN_SUB_M_W_M, CRISV32F_INSN_SUB_M_W_M, CRISV32F_SFMT_ADD_M_W_M },
  { CRIS_INSN_SUB_M_D_M, CRISV32F_INSN_SUB_M_D_M, CRISV32F_SFMT_ADD_M_D_M },
  { CRIS_INSN_SUBCBR, CRISV32F_INSN_SUBCBR, CRISV32F_SFMT_ADDCBR },
  { CRIS_INSN_SUBCWR, CRISV32F_INSN_SUBCWR, CRISV32F_SFMT_ADDCWR },
  { CRIS_INSN_SUBCDR, CRISV32F_INSN_SUBCDR, CRISV32F_SFMT_ADDCDR },
  { CRIS_INSN_SUBS_B_R, CRISV32F_INSN_SUBS_B_R, CRISV32F_SFMT_ADD_D_R },
  { CRIS_INSN_SUBS_W_R, CRISV32F_INSN_SUBS_W_R, CRISV32F_SFMT_ADD_D_R },
  { CRIS_INSN_SUBS_M_B_M, CRISV32F_INSN_SUBS_M_B_M, CRISV32F_SFMT_ADDS_M_B_M },
  { CRIS_INSN_SUBS_M_W_M, CRISV32F_INSN_SUBS_M_W_M, CRISV32F_SFMT_ADDS_M_W_M },
  { CRIS_INSN_SUBSCBR, CRISV32F_INSN_SUBSCBR, CRISV32F_SFMT_ADDSCBR },
  { CRIS_INSN_SUBSCWR, CRISV32F_INSN_SUBSCWR, CRISV32F_SFMT_ADDSCWR },
  { CRIS_INSN_SUBU_B_R, CRISV32F_INSN_SUBU_B_R, CRISV32F_SFMT_ADD_D_R },
  { CRIS_INSN_SUBU_W_R, CRISV32F_INSN_SUBU_W_R, CRISV32F_SFMT_ADD_D_R },
  { CRIS_INSN_SUBU_M_B_M, CRISV32F_INSN_SUBU_M_B_M, CRISV32F_SFMT_ADDS_M_B_M },
  { CRIS_INSN_SUBU_M_W_M, CRISV32F_INSN_SUBU_M_W_M, CRISV32F_SFMT_ADDS_M_W_M },
  { CRIS_INSN_SUBUCBR, CRISV32F_INSN_SUBUCBR, CRISV32F_SFMT_ADDSCBR },
  { CRIS_INSN_SUBUCWR, CRISV32F_INSN_SUBUCWR, CRISV32F_SFMT_ADDSCWR },
  { CRIS_INSN_ADDC_R, CRISV32F_INSN_ADDC_R, CRISV32F_SFMT_ADD_D_R },
  { CRIS_INSN_ADDC_M, CRISV32F_INSN_ADDC_M, CRISV32F_SFMT_ADDC_M },
  { CRIS_INSN_ADDC_C, CRISV32F_INSN_ADDC_C, CRISV32F_SFMT_ADDCDR },
  { CRIS_INSN_LAPC_D, CRISV32F_INSN_LAPC_D, CRISV32F_SFMT_LAPC_D },
  { CRIS_INSN_LAPCQ, CRISV32F_INSN_LAPCQ, CRISV32F_SFMT_LAPCQ },
  { CRIS_INSN_ADDI_B_R, CRISV32F_INSN_ADDI_B_R, CRISV32F_SFMT_ADDI_B_R },
  { CRIS_INSN_ADDI_W_R, CRISV32F_INSN_ADDI_W_R, CRISV32F_SFMT_ADDI_B_R },
  { CRIS_INSN_ADDI_D_R, CRISV32F_INSN_ADDI_D_R, CRISV32F_SFMT_ADDI_B_R },
  { CRIS_INSN_NEG_B_R, CRISV32F_INSN_NEG_B_R, CRISV32F_SFMT_NEG_B_R },
  { CRIS_INSN_NEG_W_R, CRISV32F_INSN_NEG_W_R, CRISV32F_SFMT_NEG_B_R },
  { CRIS_INSN_NEG_D_R, CRISV32F_INSN_NEG_D_R, CRISV32F_SFMT_NEG_D_R },
  { CRIS_INSN_TEST_M_B_M, CRISV32F_INSN_TEST_M_B_M, CRISV32F_SFMT_TEST_M_B_M },
  { CRIS_INSN_TEST_M_W_M, CRISV32F_INSN_TEST_M_W_M, CRISV32F_SFMT_TEST_M_W_M },
  { CRIS_INSN_TEST_M_D_M, CRISV32F_INSN_TEST_M_D_M, CRISV32F_SFMT_TEST_M_D_M },
  { CRIS_INSN_MOVE_R_M_B_M, CRISV32F_INSN_MOVE_R_M_B_M, CRISV32F_SFMT_MOVE_R_M_B_M },
  { CRIS_INSN_MOVE_R_M_W_M, CRISV32F_INSN_MOVE_R_M_W_M, CRISV32F_SFMT_MOVE_R_M_W_M },
  { CRIS_INSN_MOVE_R_M_D_M, CRISV32F_INSN_MOVE_R_M_D_M, CRISV32F_SFMT_MOVE_R_M_D_M },
  { CRIS_INSN_MULS_B, CRISV32F_INSN_MULS_B, CRISV32F_SFMT_MULS_B },
  { CRIS_INSN_MULS_W, CRISV32F_INSN_MULS_W, CRISV32F_SFMT_MULS_B },
  { CRIS_INSN_MULS_D, CRISV32F_INSN_MULS_D, CRISV32F_SFMT_MULS_B },
  { CRIS_INSN_MULU_B, CRISV32F_INSN_MULU_B, CRISV32F_SFMT_MULS_B },
  { CRIS_INSN_MULU_W, CRISV32F_INSN_MULU_W, CRISV32F_SFMT_MULS_B },
  { CRIS_INSN_MULU_D, CRISV32F_INSN_MULU_D, CRISV32F_SFMT_MULS_B },
  { CRIS_INSN_MCP, CRISV32F_INSN_MCP, CRISV32F_SFMT_MCP },
  { CRIS_INSN_DSTEP, CRISV32F_INSN_DSTEP, CRISV32F_SFMT_DSTEP },
  { CRIS_INSN_ABS, CRISV32F_INSN_ABS, CRISV32F_SFMT_MOVS_B_R },
  { CRIS_INSN_AND_B_R, CRISV32F_INSN_AND_B_R, CRISV32F_SFMT_AND_B_R },
  { CRIS_INSN_AND_W_R, CRISV32F_INSN_AND_W_R, CRISV32F_SFMT_AND_B_R },
  { CRIS_INSN_AND_D_R, CRISV32F_INSN_AND_D_R, CRISV32F_SFMT_AND_D_R },
  { CRIS_INSN_AND_M_B_M, CRISV32F_INSN_AND_M_B_M, CRISV32F_SFMT_AND_M_B_M },
  { CRIS_INSN_AND_M_W_M, CRISV32F_INSN_AND_M_W_M, CRISV32F_SFMT_AND_M_W_M },
  { CRIS_INSN_AND_M_D_M, CRISV32F_INSN_AND_M_D_M, CRISV32F_SFMT_AND_M_D_M },
  { CRIS_INSN_ANDCBR, CRISV32F_INSN_ANDCBR, CRISV32F_SFMT_ANDCBR },
  { CRIS_INSN_ANDCWR, CRISV32F_INSN_ANDCWR, CRISV32F_SFMT_ANDCWR },
  { CRIS_INSN_ANDCDR, CRISV32F_INSN_ANDCDR, CRISV32F_SFMT_ANDCDR },
  { CRIS_INSN_ANDQ, CRISV32F_INSN_ANDQ, CRISV32F_SFMT_ANDQ },
  { CRIS_INSN_ORR_B_R, CRISV32F_INSN_ORR_B_R, CRISV32F_SFMT_AND_B_R },
  { CRIS_INSN_ORR_W_R, CRISV32F_INSN_ORR_W_R, CRISV32F_SFMT_AND_B_R },
  { CRIS_INSN_ORR_D_R, CRISV32F_INSN_ORR_D_R, CRISV32F_SFMT_AND_D_R },
  { CRIS_INSN_OR_M_B_M, CRISV32F_INSN_OR_M_B_M, CRISV32F_SFMT_AND_M_B_M },
  { CRIS_INSN_OR_M_W_M, CRISV32F_INSN_OR_M_W_M, CRISV32F_SFMT_AND_M_W_M },
  { CRIS_INSN_OR_M_D_M, CRISV32F_INSN_OR_M_D_M, CRISV32F_SFMT_AND_M_D_M },
  { CRIS_INSN_ORCBR, CRISV32F_INSN_ORCBR, CRISV32F_SFMT_ANDCBR },
  { CRIS_INSN_ORCWR, CRISV32F_INSN_ORCWR, CRISV32F_SFMT_ANDCWR },
  { CRIS_INSN_ORCDR, CRISV32F_INSN_ORCDR, CRISV32F_SFMT_ANDCDR },
  { CRIS_INSN_ORQ, CRISV32F_INSN_ORQ, CRISV32F_SFMT_ANDQ },
  { CRIS_INSN_XOR, CRISV32F_INSN_XOR, CRISV32F_SFMT_DSTEP },
  { CRIS_INSN_SWAP, CRISV32F_INSN_SWAP, CRISV32F_SFMT_SWAP },
  { CRIS_INSN_ASRR_B_R, CRISV32F_INSN_ASRR_B_R, CRISV32F_SFMT_AND_B_R },
  { CRIS_INSN_ASRR_W_R, CRISV32F_INSN_ASRR_W_R, CRISV32F_SFMT_AND_B_R },
  { CRIS_INSN_ASRR_D_R, CRISV32F_INSN_ASRR_D_R, CRISV32F_SFMT_AND_D_R },
  { CRIS_INSN_ASRQ, CRISV32F_INSN_ASRQ, CRISV32F_SFMT_ASRQ },
  { CRIS_INSN_LSRR_B_R, CRISV32F_INSN_LSRR_B_R, CRISV32F_SFMT_LSRR_B_R },
  { CRIS_INSN_LSRR_W_R, CRISV32F_INSN_LSRR_W_R, CRISV32F_SFMT_LSRR_B_R },
  { CRIS_INSN_LSRR_D_R, CRISV32F_INSN_LSRR_D_R, CRISV32F_SFMT_LSRR_D_R },
  { CRIS_INSN_LSRQ, CRISV32F_INSN_LSRQ, CRISV32F_SFMT_ASRQ },
  { CRIS_INSN_LSLR_B_R, CRISV32F_INSN_LSLR_B_R, CRISV32F_SFMT_LSRR_B_R },
  { CRIS_INSN_LSLR_W_R, CRISV32F_INSN_LSLR_W_R, CRISV32F_SFMT_LSRR_B_R },
  { CRIS_INSN_LSLR_D_R, CRISV32F_INSN_LSLR_D_R, CRISV32F_SFMT_LSRR_D_R },
  { CRIS_INSN_LSLQ, CRISV32F_INSN_LSLQ, CRISV32F_SFMT_ASRQ },
  { CRIS_INSN_BTST, CRISV32F_INSN_BTST, CRISV32F_SFMT_BTST },
  { CRIS_INSN_BTSTQ, CRISV32F_INSN_BTSTQ, CRISV32F_SFMT_BTSTQ },
  { CRIS_INSN_SETF, CRISV32F_INSN_SETF, CRISV32F_SFMT_SETF },
  { CRIS_INSN_CLEARF, CRISV32F_INSN_CLEARF, CRISV32F_SFMT_SETF },
  { CRIS_INSN_RFE, CRISV32F_INSN_RFE, CRISV32F_SFMT_RFE },
  { CRIS_INSN_SFE, CRISV32F_INSN_SFE, CRISV32F_SFMT_SFE },
  { CRIS_INSN_RFG, CRISV32F_INSN_RFG, CRISV32F_SFMT_RFG },
  { CRIS_INSN_RFN, CRISV32F_INSN_RFN, CRISV32F_SFMT_RFN },
  { CRIS_INSN_HALT, CRISV32F_INSN_HALT, CRISV32F_SFMT_HALT },
  { CRIS_INSN_BCC_B, CRISV32F_INSN_BCC_B, CRISV32F_SFMT_BCC_B },
  { CRIS_INSN_BA_B, CRISV32F_INSN_BA_B, CRISV32F_SFMT_BA_B },
  { CRIS_INSN_BCC_W, CRISV32F_INSN_BCC_W, CRISV32F_SFMT_BCC_W },
  { CRIS_INSN_BA_W, CRISV32F_INSN_BA_W, CRISV32F_SFMT_BA_W },
  { CRIS_INSN_JAS_R, CRISV32F_INSN_JAS_R, CRISV32F_SFMT_JAS_R },
  { CRIS_INSN_JAS_C, CRISV32F_INSN_JAS_C, CRISV32F_SFMT_JAS_C },
  { CRIS_INSN_JUMP_P, CRISV32F_INSN_JUMP_P, CRISV32F_SFMT_JUMP_P },
  { CRIS_INSN_BAS_C, CRISV32F_INSN_BAS_C, CRISV32F_SFMT_BAS_C },
  { CRIS_INSN_JASC_R, CRISV32F_INSN_JASC_R, CRISV32F_SFMT_JASC_R },
  { CRIS_INSN_JASC_C, CRISV32F_INSN_JASC_C, CRISV32F_SFMT_JAS_C },
  { CRIS_INSN_BASC_C, CRISV32F_INSN_BASC_C, CRISV32F_SFMT_BAS_C },
  { CRIS_INSN_BREAK, CRISV32F_INSN_BREAK, CRISV32F_SFMT_BREAK },
  { CRIS_INSN_BOUND_R_B_R, CRISV32F_INSN_BOUND_R_B_R, CRISV32F_SFMT_DSTEP },
  { CRIS_INSN_BOUND_R_W_R, CRISV32F_INSN_BOUND_R_W_R, CRISV32F_SFMT_DSTEP },
  { CRIS_INSN_BOUND_R_D_R, CRISV32F_INSN_BOUND_R_D_R, CRISV32F_SFMT_DSTEP },
  { CRIS_INSN_BOUND_CB, CRISV32F_INSN_BOUND_CB, CRISV32F_SFMT_BOUND_CB },
  { CRIS_INSN_BOUND_CW, CRISV32F_INSN_BOUND_CW, CRISV32F_SFMT_BOUND_CW },
  { CRIS_INSN_BOUND_CD, CRISV32F_INSN_BOUND_CD, CRISV32F_SFMT_BOUND_CD },
  { CRIS_INSN_SCC, CRISV32F_INSN_SCC, CRISV32F_SFMT_SCC },
  { CRIS_INSN_LZ, CRISV32F_INSN_LZ, CRISV32F_SFMT_MOVS_B_R },
  { CRIS_INSN_ADDOQ, CRISV32F_INSN_ADDOQ, CRISV32F_SFMT_ADDOQ },
  { CRIS_INSN_ADDO_M_B_M, CRISV32F_INSN_ADDO_M_B_M, CRISV32F_SFMT_ADDO_M_B_M },
  { CRIS_INSN_ADDO_M_W_M, CRISV32F_INSN_ADDO_M_W_M, CRISV32F_SFMT_ADDO_M_W_M },
  { CRIS_INSN_ADDO_M_D_M, CRISV32F_INSN_ADDO_M_D_M, CRISV32F_SFMT_ADDO_M_D_M },
  { CRIS_INSN_ADDO_CB, CRISV32F_INSN_ADDO_CB, CRISV32F_SFMT_ADDO_CB },
  { CRIS_INSN_ADDO_CW, CRISV32F_INSN_ADDO_CW, CRISV32F_SFMT_ADDO_CW },
  { CRIS_INSN_ADDO_CD, CRISV32F_INSN_ADDO_CD, CRISV32F_SFMT_ADDO_CD },
  { CRIS_INSN_ADDI_ACR_B_R, CRISV32F_INSN_ADDI_ACR_B_R, CRISV32F_SFMT_ADDI_ACR_B_R },
  { CRIS_INSN_ADDI_ACR_W_R, CRISV32F_INSN_ADDI_ACR_W_R, CRISV32F_SFMT_ADDI_ACR_B_R },
  { CRIS_INSN_ADDI_ACR_D_R, CRISV32F_INSN_ADDI_ACR_D_R, CRISV32F_SFMT_ADDI_ACR_B_R },
  { CRIS_INSN_FIDXI, CRISV32F_INSN_FIDXI, CRISV32F_SFMT_FIDXI },
  { CRIS_INSN_FTAGI, CRISV32F_INSN_FTAGI, CRISV32F_SFMT_FIDXI },
  { CRIS_INSN_FIDXD, CRISV32F_INSN_FIDXD, CRISV32F_SFMT_FIDXI },
  { CRIS_INSN_FTAGD, CRISV32F_INSN_FTAGD, CRISV32F_SFMT_FIDXI },
};

static const struct insn_sem crisv32f_insn_sem_invalid =
{
  VIRTUAL_INSN_X_INVALID, CRISV32F_INSN_X_INVALID, CRISV32F_SFMT_EMPTY
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
crisv32f_init_idesc_table (SIM_CPU *cpu)
{
  IDESC *id,*tabend;
  const struct insn_sem *t,*tend;
  int tabsize = CRISV32F_INSN__MAX;
  IDESC *table = crisv32f_insn_data;

  memset (table, 0, tabsize * sizeof (IDESC));

  /* First set all entries to the `invalid insn'.  */
  t = & crisv32f_insn_sem_invalid;
  for (id = table, tabend = table + tabsize; id < tabend; ++id)
    init_idesc (cpu, id, t);

  /* Now fill in the values for the chosen cpu.  */
  for (t = crisv32f_insn_sem, tend = t + ARRAY_SIZE (crisv32f_insn_sem);
       t != tend; ++t)
    {
      init_idesc (cpu, & table[t->index], t);
    }

  /* Link the IDESC table into the cpu.  */
  CPU_IDESC (cpu) = table;
}

/* Given an instruction, return a pointer to its IDESC entry.  */

const IDESC *
crisv32f_decode (SIM_CPU *current_cpu, IADDR pc,
              CGEN_INSN_WORD base_insn,
              ARGBUF *abuf)
{
  /* Result of decoder.  */
  CRISV32F_INSN_TYPE itype;

  {
    CGEN_INSN_WORD insn = base_insn;

    {
      unsigned int val0 = (((insn >> 4) & (255 << 0)));
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
        {
          unsigned int val1 = (((insn >> 12) & (15 << 0)));
          switch (val1)
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
          case 15: itype = CRISV32F_INSN_BCC_B; goto extract_sfmt_bcc_b;
          case 14: itype = CRISV32F_INSN_BA_B; goto extract_sfmt_ba_b;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
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
      case 31: itype = CRISV32F_INSN_ADDOQ; goto extract_sfmt_addoq;
      case 32:
      case 33:
      case 34:
      case 35: itype = CRISV32F_INSN_ADDQ; goto extract_sfmt_addq;
      case 36:
      case 37:
      case 38:
      case 39: itype = CRISV32F_INSN_MOVEQ; goto extract_sfmt_moveq;
      case 40:
      case 41:
      case 42:
      case 43: itype = CRISV32F_INSN_SUBQ; goto extract_sfmt_addq;
      case 44:
      case 45:
      case 46:
      case 47: itype = CRISV32F_INSN_CMPQ; goto extract_sfmt_cmpq;
      case 48:
      case 49:
      case 50:
      case 51: itype = CRISV32F_INSN_ANDQ; goto extract_sfmt_andq;
      case 52:
      case 53:
      case 54:
      case 55: itype = CRISV32F_INSN_ORQ; goto extract_sfmt_andq;
      case 56:
      case 57: itype = CRISV32F_INSN_BTSTQ; goto extract_sfmt_btstq;
      case 58:
      case 59: itype = CRISV32F_INSN_ASRQ; goto extract_sfmt_asrq;
      case 60:
      case 61: itype = CRISV32F_INSN_LSLQ; goto extract_sfmt_asrq;
      case 62:
      case 63: itype = CRISV32F_INSN_LSRQ; goto extract_sfmt_asrq;
      case 64: itype = CRISV32F_INSN_ADDU_B_R; goto extract_sfmt_add_d_r;
      case 65: itype = CRISV32F_INSN_ADDU_W_R; goto extract_sfmt_add_d_r;
      case 66: itype = CRISV32F_INSN_ADDS_B_R; goto extract_sfmt_add_d_r;
      case 67: itype = CRISV32F_INSN_ADDS_W_R; goto extract_sfmt_add_d_r;
      case 68: itype = CRISV32F_INSN_MOVU_B_R; goto extract_sfmt_movs_b_r;
      case 69: itype = CRISV32F_INSN_MOVU_W_R; goto extract_sfmt_movs_b_r;
      case 70: itype = CRISV32F_INSN_MOVS_B_R; goto extract_sfmt_movs_b_r;
      case 71: itype = CRISV32F_INSN_MOVS_W_R; goto extract_sfmt_movs_b_r;
      case 72: itype = CRISV32F_INSN_SUBU_B_R; goto extract_sfmt_add_d_r;
      case 73: itype = CRISV32F_INSN_SUBU_W_R; goto extract_sfmt_add_d_r;
      case 74: itype = CRISV32F_INSN_SUBS_B_R; goto extract_sfmt_add_d_r;
      case 75: itype = CRISV32F_INSN_SUBS_W_R; goto extract_sfmt_add_d_r;
      case 76: itype = CRISV32F_INSN_LSLR_B_R; goto extract_sfmt_lsrr_b_r;
      case 77: itype = CRISV32F_INSN_LSLR_W_R; goto extract_sfmt_lsrr_b_r;
      case 78: itype = CRISV32F_INSN_LSLR_D_R; goto extract_sfmt_lsrr_d_r;
      case 79: itype = CRISV32F_INSN_BTST; goto extract_sfmt_btst;
      case 80: itype = CRISV32F_INSN_ADDI_B_R; goto extract_sfmt_addi_b_r;
      case 81: itype = CRISV32F_INSN_ADDI_W_R; goto extract_sfmt_addi_b_r;
      case 82: itype = CRISV32F_INSN_ADDI_D_R; goto extract_sfmt_addi_b_r;
      case 83: itype = CRISV32F_INSN_SCC; goto extract_sfmt_scc;
      case 84: itype = CRISV32F_INSN_ADDI_ACR_B_R; goto extract_sfmt_addi_acr_b_r;
      case 85: itype = CRISV32F_INSN_ADDI_ACR_W_R; goto extract_sfmt_addi_acr_b_r;
      case 86: itype = CRISV32F_INSN_ADDI_ACR_D_R; goto extract_sfmt_addi_acr_b_r;
      case 87: itype = CRISV32F_INSN_ADDC_R; goto extract_sfmt_add_d_r;
      case 88: itype = CRISV32F_INSN_NEG_B_R; goto extract_sfmt_neg_b_r;
      case 89: itype = CRISV32F_INSN_NEG_W_R; goto extract_sfmt_neg_b_r;
      case 90: itype = CRISV32F_INSN_NEG_D_R; goto extract_sfmt_neg_d_r;
      case 91: itype = CRISV32F_INSN_SETF; goto extract_sfmt_setf;
      case 92: itype = CRISV32F_INSN_BOUND_R_B_R; goto extract_sfmt_dstep;
      case 93: itype = CRISV32F_INSN_BOUND_R_W_R; goto extract_sfmt_dstep;
      case 94: itype = CRISV32F_INSN_BOUND_R_D_R; goto extract_sfmt_dstep;
      case 95: itype = CRISV32F_INSN_CLEARF; goto extract_sfmt_setf;
      case 96: itype = CRISV32F_INSN_ADD_B_R; goto extract_sfmt_add_b_r;
      case 97: itype = CRISV32F_INSN_ADD_W_R; goto extract_sfmt_add_b_r;
      case 98: itype = CRISV32F_INSN_ADD_D_R; goto extract_sfmt_add_d_r;
      case 99: itype = CRISV32F_INSN_MOVE_R_SPRV32; goto extract_sfmt_move_r_sprv32;
      case 100: itype = CRISV32F_INSN_MOVE_B_R; goto extract_sfmt_move_b_r;
      case 101: itype = CRISV32F_INSN_MOVE_W_R; goto extract_sfmt_move_b_r;
      case 102: itype = CRISV32F_INSN_MOVE_D_R; goto extract_sfmt_move_d_r;
      case 103: itype = CRISV32F_INSN_MOVE_SPR_RV32; goto extract_sfmt_move_spr_rv32;
      case 104: itype = CRISV32F_INSN_SUB_B_R; goto extract_sfmt_add_b_r;
      case 105: itype = CRISV32F_INSN_SUB_W_R; goto extract_sfmt_add_b_r;
      case 106: itype = CRISV32F_INSN_SUB_D_R; goto extract_sfmt_add_d_r;
      case 107: itype = CRISV32F_INSN_ABS; goto extract_sfmt_movs_b_r;
      case 108: itype = CRISV32F_INSN_CMP_R_B_R; goto extract_sfmt_cmp_r_b_r;
      case 109: itype = CRISV32F_INSN_CMP_R_W_R; goto extract_sfmt_cmp_r_b_r;
      case 110: itype = CRISV32F_INSN_CMP_R_D_R; goto extract_sfmt_cmp_r_b_r;
      case 111: itype = CRISV32F_INSN_DSTEP; goto extract_sfmt_dstep;
      case 112: itype = CRISV32F_INSN_AND_B_R; goto extract_sfmt_and_b_r;
      case 113: itype = CRISV32F_INSN_AND_W_R; goto extract_sfmt_and_b_r;
      case 114: itype = CRISV32F_INSN_AND_D_R; goto extract_sfmt_and_d_r;
      case 115: itype = CRISV32F_INSN_LZ; goto extract_sfmt_movs_b_r;
      case 116: itype = CRISV32F_INSN_ORR_B_R; goto extract_sfmt_and_b_r;
      case 117: itype = CRISV32F_INSN_ORR_W_R; goto extract_sfmt_and_b_r;
      case 118: itype = CRISV32F_INSN_ORR_D_R; goto extract_sfmt_and_d_r;
      case 119: itype = CRISV32F_INSN_SWAP; goto extract_sfmt_swap;
      case 120: itype = CRISV32F_INSN_ASRR_B_R; goto extract_sfmt_and_b_r;
      case 121: itype = CRISV32F_INSN_ASRR_W_R; goto extract_sfmt_and_b_r;
      case 122: itype = CRISV32F_INSN_ASRR_D_R; goto extract_sfmt_and_d_r;
      case 123: itype = CRISV32F_INSN_XOR; goto extract_sfmt_dstep;
      case 124: itype = CRISV32F_INSN_LSRR_B_R; goto extract_sfmt_lsrr_b_r;
      case 125: itype = CRISV32F_INSN_LSRR_W_R; goto extract_sfmt_lsrr_b_r;
      case 126: itype = CRISV32F_INSN_LSRR_D_R; goto extract_sfmt_lsrr_d_r;
      case 127: itype = CRISV32F_INSN_MCP; goto extract_sfmt_mcp;
      case 128: itype = CRISV32F_INSN_ADDU_M_B_M; goto extract_sfmt_adds_m_b_m;
      case 129: itype = CRISV32F_INSN_ADDU_M_W_M; goto extract_sfmt_adds_m_w_m;
      case 130: itype = CRISV32F_INSN_ADDS_M_B_M; goto extract_sfmt_adds_m_b_m;
      case 131: itype = CRISV32F_INSN_ADDS_M_W_M; goto extract_sfmt_adds_m_w_m;
      case 132: itype = CRISV32F_INSN_MOVU_M_B_M; goto extract_sfmt_movs_m_b_m;
      case 133: itype = CRISV32F_INSN_MOVU_M_W_M; goto extract_sfmt_movs_m_w_m;
      case 134: itype = CRISV32F_INSN_MOVS_M_B_M; goto extract_sfmt_movs_m_b_m;
      case 135: itype = CRISV32F_INSN_MOVS_M_W_M; goto extract_sfmt_movs_m_w_m;
      case 136: itype = CRISV32F_INSN_SUBU_M_B_M; goto extract_sfmt_adds_m_b_m;
      case 137: itype = CRISV32F_INSN_SUBU_M_W_M; goto extract_sfmt_adds_m_w_m;
      case 138: itype = CRISV32F_INSN_SUBS_M_B_M; goto extract_sfmt_adds_m_b_m;
      case 139: itype = CRISV32F_INSN_SUBS_M_W_M; goto extract_sfmt_adds_m_w_m;
      case 140: itype = CRISV32F_INSN_CMPU_M_B_M; goto extract_sfmt_cmp_m_b_m;
      case 141: itype = CRISV32F_INSN_CMPU_M_W_M; goto extract_sfmt_cmp_m_w_m;
      case 142: itype = CRISV32F_INSN_CMPS_M_B_M; goto extract_sfmt_cmp_m_b_m;
      case 143: itype = CRISV32F_INSN_CMPS_M_W_M; goto extract_sfmt_cmp_m_w_m;
      case 144: itype = CRISV32F_INSN_MULU_B; goto extract_sfmt_muls_b;
      case 145: itype = CRISV32F_INSN_MULU_W; goto extract_sfmt_muls_b;
      case 146: itype = CRISV32F_INSN_MULU_D; goto extract_sfmt_muls_b;
      case 147:
        {
          unsigned int val1 = (((insn >> 12) & (15 << 0)));
          switch (val1)
          {
          case 2:
            if ((base_insn & 0xffff) == 0x2930)
              { itype = CRISV32F_INSN_RFE; goto extract_sfmt_rfe; }
            itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          case 3:
            if ((base_insn & 0xffff) == 0x3930)
              { itype = CRISV32F_INSN_SFE; goto extract_sfmt_sfe; }
            itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          case 4:
            if ((base_insn & 0xffff) == 0x4930)
              { itype = CRISV32F_INSN_RFG; goto extract_sfmt_rfg; }
            itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          case 5:
            if ((base_insn & 0xffff) == 0x5930)
              { itype = CRISV32F_INSN_RFN; goto extract_sfmt_rfn; }
            itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          case 14: itype = CRISV32F_INSN_BREAK; goto extract_sfmt_break;
          case 15:
            if ((base_insn & 0xffff) == 0xf930)
              { itype = CRISV32F_INSN_HALT; goto extract_sfmt_halt; }
            itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 148: itype = CRISV32F_INSN_ADDO_M_B_M; goto extract_sfmt_addo_m_b_m;
      case 149: itype = CRISV32F_INSN_ADDO_M_W_M; goto extract_sfmt_addo_m_w_m;
      case 150: itype = CRISV32F_INSN_ADDO_M_D_M; goto extract_sfmt_addo_m_d_m;
      case 151: itype = CRISV32F_INSN_LAPCQ; goto extract_sfmt_lapcq;
      case 154: itype = CRISV32F_INSN_ADDC_M; goto extract_sfmt_addc_m;
      case 155: itype = CRISV32F_INSN_JAS_R; goto extract_sfmt_jas_r;
      case 159:
        if ((base_insn & 0xfff) == 0x9f0)
          { itype = CRISV32F_INSN_JUMP_P; goto extract_sfmt_jump_p; }
        itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
      case 160: itype = CRISV32F_INSN_ADD_M_B_M; goto extract_sfmt_add_m_b_m;
      case 161: itype = CRISV32F_INSN_ADD_M_W_M; goto extract_sfmt_add_m_w_m;
      case 162: itype = CRISV32F_INSN_ADD_M_D_M; goto extract_sfmt_add_m_d_m;
      case 163: itype = CRISV32F_INSN_MOVE_M_SPRV32; goto extract_sfmt_move_m_sprv32;
      case 164: itype = CRISV32F_INSN_MOVE_M_B_M; goto extract_sfmt_move_m_b_m;
      case 165: itype = CRISV32F_INSN_MOVE_M_W_M; goto extract_sfmt_move_m_w_m;
      case 166: itype = CRISV32F_INSN_MOVE_M_D_M; goto extract_sfmt_move_m_d_m;
      case 167:
      case 231: itype = CRISV32F_INSN_MOVE_SPR_MV32; goto extract_sfmt_move_spr_mv32;
      case 168: itype = CRISV32F_INSN_SUB_M_B_M; goto extract_sfmt_add_m_b_m;
      case 169: itype = CRISV32F_INSN_SUB_M_W_M; goto extract_sfmt_add_m_w_m;
      case 170: itype = CRISV32F_INSN_SUB_M_D_M; goto extract_sfmt_add_m_d_m;
      case 171:
        {
          unsigned int val1 = (((insn >> 12) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((base_insn & 0xfff0) == 0xab0)
              { itype = CRISV32F_INSN_FIDXD; goto extract_sfmt_fidxi; }
            itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((base_insn & 0xfff0) == 0x1ab0)
              { itype = CRISV32F_INSN_FTAGD; goto extract_sfmt_fidxi; }
            itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 172: itype = CRISV32F_INSN_CMP_M_B_M; goto extract_sfmt_cmp_m_b_m;
      case 173: itype = CRISV32F_INSN_CMP_M_W_M; goto extract_sfmt_cmp_m_w_m;
      case 174: itype = CRISV32F_INSN_CMP_M_D_M; goto extract_sfmt_cmp_m_d_m;
      case 176: itype = CRISV32F_INSN_AND_M_B_M; goto extract_sfmt_and_m_b_m;
      case 177: itype = CRISV32F_INSN_AND_M_W_M; goto extract_sfmt_and_m_w_m;
      case 178: itype = CRISV32F_INSN_AND_M_D_M; goto extract_sfmt_and_m_d_m;
      case 179: itype = CRISV32F_INSN_JASC_R; goto extract_sfmt_jasc_r;
      case 180: itype = CRISV32F_INSN_OR_M_B_M; goto extract_sfmt_and_m_b_m;
      case 181: itype = CRISV32F_INSN_OR_M_W_M; goto extract_sfmt_and_m_w_m;
      case 182: itype = CRISV32F_INSN_OR_M_D_M; goto extract_sfmt_and_m_d_m;
      case 183: itype = CRISV32F_INSN_MOVE_R_SS; goto extract_sfmt_move_r_ss;
      case 184:
      case 248:
        if ((base_insn & 0xfbf0) == 0xb80)
          { itype = CRISV32F_INSN_TEST_M_B_M; goto extract_sfmt_test_m_b_m; }
        itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
      case 185:
      case 249:
        if ((base_insn & 0xfbf0) == 0xb90)
          { itype = CRISV32F_INSN_TEST_M_W_M; goto extract_sfmt_test_m_w_m; }
        itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
      case 186:
      case 250:
        if ((base_insn & 0xfbf0) == 0xba0)
          { itype = CRISV32F_INSN_TEST_M_D_M; goto extract_sfmt_test_m_d_m; }
        itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
      case 187:
      case 251: itype = CRISV32F_INSN_MOVEM_M_R_V32; goto extract_sfmt_movem_m_r_v32;
      case 188:
      case 252: itype = CRISV32F_INSN_MOVE_R_M_B_M; goto extract_sfmt_move_r_m_b_m;
      case 189:
      case 253: itype = CRISV32F_INSN_MOVE_R_M_W_M; goto extract_sfmt_move_r_m_w_m;
      case 190:
      case 254: itype = CRISV32F_INSN_MOVE_R_M_D_M; goto extract_sfmt_move_r_m_d_m;
      case 191:
      case 255: itype = CRISV32F_INSN_MOVEM_R_M_V32; goto extract_sfmt_movem_r_m_v32;
      case 192:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_ADDU_M_B_M; goto extract_sfmt_adds_m_b_m;
          case 15: itype = CRISV32F_INSN_ADDUCBR; goto extract_sfmt_addscbr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 193:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_ADDU_M_W_M; goto extract_sfmt_adds_m_w_m;
          case 15: itype = CRISV32F_INSN_ADDUCWR; goto extract_sfmt_addscwr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 194:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_ADDS_M_B_M; goto extract_sfmt_adds_m_b_m;
          case 15: itype = CRISV32F_INSN_ADDSCBR; goto extract_sfmt_addscbr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 195:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_ADDS_M_W_M; goto extract_sfmt_adds_m_w_m;
          case 15: itype = CRISV32F_INSN_ADDSCWR; goto extract_sfmt_addscwr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 196:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_MOVU_M_B_M; goto extract_sfmt_movs_m_b_m;
          case 15: itype = CRISV32F_INSN_MOVUCBR; goto extract_sfmt_movucbr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 197:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_MOVU_M_W_M; goto extract_sfmt_movs_m_w_m;
          case 15: itype = CRISV32F_INSN_MOVUCWR; goto extract_sfmt_movucwr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 198:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_MOVS_M_B_M; goto extract_sfmt_movs_m_b_m;
          case 15: itype = CRISV32F_INSN_MOVSCBR; goto extract_sfmt_movscbr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 199:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_MOVS_M_W_M; goto extract_sfmt_movs_m_w_m;
          case 15: itype = CRISV32F_INSN_MOVSCWR; goto extract_sfmt_movscwr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 200:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_SUBU_M_B_M; goto extract_sfmt_adds_m_b_m;
          case 15: itype = CRISV32F_INSN_SUBUCBR; goto extract_sfmt_addscbr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 201:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_SUBU_M_W_M; goto extract_sfmt_adds_m_w_m;
          case 15: itype = CRISV32F_INSN_SUBUCWR; goto extract_sfmt_addscwr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 202:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_SUBS_M_B_M; goto extract_sfmt_adds_m_b_m;
          case 15: itype = CRISV32F_INSN_SUBSCBR; goto extract_sfmt_addscbr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 203:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_SUBS_M_W_M; goto extract_sfmt_adds_m_w_m;
          case 15: itype = CRISV32F_INSN_SUBSCWR; goto extract_sfmt_addscwr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 204:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_CMPU_M_B_M; goto extract_sfmt_cmp_m_b_m;
          case 15: itype = CRISV32F_INSN_CMPUCBR; goto extract_sfmt_cmpucbr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 205:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_CMPU_M_W_M; goto extract_sfmt_cmp_m_w_m;
          case 15: itype = CRISV32F_INSN_CMPUCWR; goto extract_sfmt_cmpucwr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 206:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_CMPS_M_B_M; goto extract_sfmt_cmp_m_b_m;
          case 15: itype = CRISV32F_INSN_CMPSCBR; goto extract_sfmt_cmpcbr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 207:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_CMPS_M_W_M; goto extract_sfmt_cmp_m_w_m;
          case 15: itype = CRISV32F_INSN_CMPSCWR; goto extract_sfmt_cmpcwr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 208: itype = CRISV32F_INSN_MULS_B; goto extract_sfmt_muls_b;
      case 209: itype = CRISV32F_INSN_MULS_W; goto extract_sfmt_muls_b;
      case 210: itype = CRISV32F_INSN_MULS_D; goto extract_sfmt_muls_b;
      case 211:
        {
          unsigned int val1 = (((insn >> 12) & (1 << 0)));
          switch (val1)
          {
          case 0:
            if ((base_insn & 0xfff0) == 0xd30)
              { itype = CRISV32F_INSN_FIDXI; goto extract_sfmt_fidxi; }
            itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          case 1:
            if ((base_insn & 0xfff0) == 0x1d30)
              { itype = CRISV32F_INSN_FTAGI; goto extract_sfmt_fidxi; }
            itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 212:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_ADDO_M_B_M; goto extract_sfmt_addo_m_b_m;
          case 15: itype = CRISV32F_INSN_ADDO_CB; goto extract_sfmt_addo_cb;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 213:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_ADDO_M_W_M; goto extract_sfmt_addo_m_w_m;
          case 15: itype = CRISV32F_INSN_ADDO_CW; goto extract_sfmt_addo_cw;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 214:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_ADDO_M_D_M; goto extract_sfmt_addo_m_d_m;
          case 15: itype = CRISV32F_INSN_ADDO_CD; goto extract_sfmt_addo_cd;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 215:
        if ((base_insn & 0xfff) == 0xd7f)
          { itype = CRISV32F_INSN_LAPC_D; goto extract_sfmt_lapc_d; }
        itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
      case 218:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_ADDC_M; goto extract_sfmt_addc_m;
          case 15: itype = CRISV32F_INSN_ADDC_C; goto extract_sfmt_addcdr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 219:
        if ((base_insn & 0xfff) == 0xdbf)
          { itype = CRISV32F_INSN_JAS_C; goto extract_sfmt_jas_c; }
        itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
      case 220:
        if ((base_insn & 0xfff) == 0xdcf)
          { itype = CRISV32F_INSN_BOUND_CB; goto extract_sfmt_bound_cb; }
        itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
      case 221:
        if ((base_insn & 0xfff) == 0xddf)
          { itype = CRISV32F_INSN_BOUND_CW; goto extract_sfmt_bound_cw; }
        itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
      case 222:
        if ((base_insn & 0xfff) == 0xdef)
          { itype = CRISV32F_INSN_BOUND_CD; goto extract_sfmt_bound_cd; }
        itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
      case 223:
        {
          unsigned int val1 = (((insn >> 12) & (15 << 0)));
          switch (val1)
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
          case 15:
            if ((base_insn & 0xfff) == 0xdff)
              { itype = CRISV32F_INSN_BCC_W; goto extract_sfmt_bcc_w; }
            itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          case 14:
            if ((base_insn & 0xffff) == 0xedff)
              { itype = CRISV32F_INSN_BA_W; goto extract_sfmt_ba_w; }
            itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 224:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_ADD_M_B_M; goto extract_sfmt_add_m_b_m;
          case 15: itype = CRISV32F_INSN_ADDCBR; goto extract_sfmt_addcbr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 225:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_ADD_M_W_M; goto extract_sfmt_add_m_w_m;
          case 15: itype = CRISV32F_INSN_ADDCWR; goto extract_sfmt_addcwr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 226:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_ADD_M_D_M; goto extract_sfmt_add_m_d_m;
          case 15: itype = CRISV32F_INSN_ADDCDR; goto extract_sfmt_addcdr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 227:
        {
          unsigned int val1 = (((insn >> 12) & (15 << 0)));
          switch (val1)
          {
          case 0:
          case 1:
          case 4:
          case 8: itype = CRISV32F_INSN_MOVE_M_SPRV32; goto extract_sfmt_move_m_sprv32;
          case 2:
            {
              unsigned int val2 = (((insn >> 0) & (15 << 0)));
              switch (val2)
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
              case 14: itype = CRISV32F_INSN_MOVE_M_SPRV32; goto extract_sfmt_move_m_sprv32;
              case 15: itype = CRISV32F_INSN_MOVE_C_SPRV32_P2; goto extract_sfmt_move_c_sprv32_p2;
              default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
              }
            }
          case 3:
            {
              unsigned int val2 = (((insn >> 0) & (15 << 0)));
              switch (val2)
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
              case 14: itype = CRISV32F_INSN_MOVE_M_SPRV32; goto extract_sfmt_move_m_sprv32;
              case 15: itype = CRISV32F_INSN_MOVE_C_SPRV32_P3; goto extract_sfmt_move_c_sprv32_p2;
              default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
              }
            }
          case 5:
            {
              unsigned int val2 = (((insn >> 0) & (15 << 0)));
              switch (val2)
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
              case 14: itype = CRISV32F_INSN_MOVE_M_SPRV32; goto extract_sfmt_move_m_sprv32;
              case 15: itype = CRISV32F_INSN_MOVE_C_SPRV32_P5; goto extract_sfmt_move_c_sprv32_p2;
              default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
              }
            }
          case 6:
            {
              unsigned int val2 = (((insn >> 0) & (15 << 0)));
              switch (val2)
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
              case 14: itype = CRISV32F_INSN_MOVE_M_SPRV32; goto extract_sfmt_move_m_sprv32;
              case 15: itype = CRISV32F_INSN_MOVE_C_SPRV32_P6; goto extract_sfmt_move_c_sprv32_p2;
              default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
              }
            }
          case 7:
            {
              unsigned int val2 = (((insn >> 0) & (15 << 0)));
              switch (val2)
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
              case 14: itype = CRISV32F_INSN_MOVE_M_SPRV32; goto extract_sfmt_move_m_sprv32;
              case 15: itype = CRISV32F_INSN_MOVE_C_SPRV32_P7; goto extract_sfmt_move_c_sprv32_p2;
              default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
              }
            }
          case 9:
            {
              unsigned int val2 = (((insn >> 0) & (15 << 0)));
              switch (val2)
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
              case 14: itype = CRISV32F_INSN_MOVE_M_SPRV32; goto extract_sfmt_move_m_sprv32;
              case 15: itype = CRISV32F_INSN_MOVE_C_SPRV32_P9; goto extract_sfmt_move_c_sprv32_p2;
              default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
              }
            }
          case 10:
            {
              unsigned int val2 = (((insn >> 0) & (15 << 0)));
              switch (val2)
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
              case 14: itype = CRISV32F_INSN_MOVE_M_SPRV32; goto extract_sfmt_move_m_sprv32;
              case 15: itype = CRISV32F_INSN_MOVE_C_SPRV32_P10; goto extract_sfmt_move_c_sprv32_p2;
              default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
              }
            }
          case 11:
            {
              unsigned int val2 = (((insn >> 0) & (15 << 0)));
              switch (val2)
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
              case 14: itype = CRISV32F_INSN_MOVE_M_SPRV32; goto extract_sfmt_move_m_sprv32;
              case 15: itype = CRISV32F_INSN_MOVE_C_SPRV32_P11; goto extract_sfmt_move_c_sprv32_p2;
              default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
              }
            }
          case 12:
            {
              unsigned int val2 = (((insn >> 0) & (15 << 0)));
              switch (val2)
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
              case 14: itype = CRISV32F_INSN_MOVE_M_SPRV32; goto extract_sfmt_move_m_sprv32;
              case 15: itype = CRISV32F_INSN_MOVE_C_SPRV32_P12; goto extract_sfmt_move_c_sprv32_p2;
              default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
              }
            }
          case 13:
            {
              unsigned int val2 = (((insn >> 0) & (15 << 0)));
              switch (val2)
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
              case 14: itype = CRISV32F_INSN_MOVE_M_SPRV32; goto extract_sfmt_move_m_sprv32;
              case 15: itype = CRISV32F_INSN_MOVE_C_SPRV32_P13; goto extract_sfmt_move_c_sprv32_p2;
              default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
              }
            }
          case 14:
            {
              unsigned int val2 = (((insn >> 0) & (15 << 0)));
              switch (val2)
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
              case 14: itype = CRISV32F_INSN_MOVE_M_SPRV32; goto extract_sfmt_move_m_sprv32;
              case 15: itype = CRISV32F_INSN_MOVE_C_SPRV32_P14; goto extract_sfmt_move_c_sprv32_p2;
              default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
              }
            }
          case 15:
            {
              unsigned int val2 = (((insn >> 0) & (15 << 0)));
              switch (val2)
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
              case 14: itype = CRISV32F_INSN_MOVE_M_SPRV32; goto extract_sfmt_move_m_sprv32;
              case 15: itype = CRISV32F_INSN_MOVE_C_SPRV32_P15; goto extract_sfmt_move_c_sprv32_p2;
              default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
              }
            }
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 228:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_MOVE_M_B_M; goto extract_sfmt_move_m_b_m;
          case 15: itype = CRISV32F_INSN_MOVECBR; goto extract_sfmt_movecbr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 229:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_MOVE_M_W_M; goto extract_sfmt_move_m_w_m;
          case 15: itype = CRISV32F_INSN_MOVECWR; goto extract_sfmt_movecwr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 230:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_MOVE_M_D_M; goto extract_sfmt_move_m_d_m;
          case 15: itype = CRISV32F_INSN_MOVECDR; goto extract_sfmt_movecdr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 232:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_SUB_M_B_M; goto extract_sfmt_add_m_b_m;
          case 15: itype = CRISV32F_INSN_SUBCBR; goto extract_sfmt_addcbr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 233:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_SUB_M_W_M; goto extract_sfmt_add_m_w_m;
          case 15: itype = CRISV32F_INSN_SUBCWR; goto extract_sfmt_addcwr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 234:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_SUB_M_D_M; goto extract_sfmt_add_m_d_m;
          case 15: itype = CRISV32F_INSN_SUBCDR; goto extract_sfmt_addcdr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 235:
        if ((base_insn & 0xfff) == 0xebf)
          { itype = CRISV32F_INSN_BAS_C; goto extract_sfmt_bas_c; }
        itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
      case 236:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_CMP_M_B_M; goto extract_sfmt_cmp_m_b_m;
          case 15: itype = CRISV32F_INSN_CMPCBR; goto extract_sfmt_cmpcbr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 237:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_CMP_M_W_M; goto extract_sfmt_cmp_m_w_m;
          case 15: itype = CRISV32F_INSN_CMPCWR; goto extract_sfmt_cmpcwr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 238:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_CMP_M_D_M; goto extract_sfmt_cmp_m_d_m;
          case 15: itype = CRISV32F_INSN_CMPCDR; goto extract_sfmt_cmpcdr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 239:
        if ((base_insn & 0xfff) == 0xeff)
          { itype = CRISV32F_INSN_BASC_C; goto extract_sfmt_bas_c; }
        itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
      case 240:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_AND_M_B_M; goto extract_sfmt_and_m_b_m;
          case 15: itype = CRISV32F_INSN_ANDCBR; goto extract_sfmt_andcbr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 241:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_AND_M_W_M; goto extract_sfmt_and_m_w_m;
          case 15: itype = CRISV32F_INSN_ANDCWR; goto extract_sfmt_andcwr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 242:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_AND_M_D_M; goto extract_sfmt_and_m_d_m;
          case 15: itype = CRISV32F_INSN_ANDCDR; goto extract_sfmt_andcdr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 243:
        if ((base_insn & 0xfff) == 0xf3f)
          { itype = CRISV32F_INSN_JASC_C; goto extract_sfmt_jas_c; }
        itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
      case 244:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_OR_M_B_M; goto extract_sfmt_and_m_b_m;
          case 15: itype = CRISV32F_INSN_ORCBR; goto extract_sfmt_andcbr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 245:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_OR_M_W_M; goto extract_sfmt_and_m_w_m;
          case 15: itype = CRISV32F_INSN_ORCWR; goto extract_sfmt_andcwr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 246:
        {
          unsigned int val1 = (((insn >> 0) & (15 << 0)));
          switch (val1)
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
          case 14: itype = CRISV32F_INSN_OR_M_D_M; goto extract_sfmt_and_m_d_m;
          case 15: itype = CRISV32F_INSN_ORCDR; goto extract_sfmt_andcdr;
          default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
          }
        }
      case 247: itype = CRISV32F_INSN_MOVE_SS_R; goto extract_sfmt_move_ss_r;
      default: itype = CRISV32F_INSN_X_INVALID; goto extract_sfmt_empty;
      }
    }
  }

  /* The instruction has been decoded, now extract the fields.  */

 extract_sfmt_empty:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_empty", (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_move_b_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_move_b_r", "f_operand1 0x%x", 'x', f_operand1, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_move_d_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_move_d_r", "f_operand1 0x%x", 'x', f_operand1, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_moveq:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_moveq.f
    UINT f_operand2;
    INT f_s6;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_s6 = EXTRACT_LSB0_SINT (insn, 16, 5, 6);

  /* Record the fields for the semantic handler.  */
  FLD (f_s6) = f_s6;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_moveq", "f_s6 0x%x", 'x', f_s6, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_movs_b_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_muls_b.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movs_b_r", "f_operand1 0x%x", 'x', f_operand1, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_movecbr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addcbr.f
    INT f_indir_pc__byte;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__byte = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__byte) = f_indir_pc__byte;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movecbr", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__byte 0x%x", 'x', f_indir_pc__byte, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_movecwr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addcwr.f
    INT f_indir_pc__word;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__word = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__word) = f_indir_pc__word;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movecwr", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__word 0x%x", 'x', f_indir_pc__word, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_movecdr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cd.f
    INT f_indir_pc__dword;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__dword = (0|(EXTRACT_LSB0_UINT (word_1, 32, 31, 32) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_indir_pc__dword) = f_indir_pc__dword;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movecdr", "f_indir_pc__dword 0x%x", 'x', f_indir_pc__dword, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_movscbr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cb.f
    UINT f_operand2;
    INT f_indir_pc__byte;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_indir_pc__byte = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_indir_pc__byte) = f_indir_pc__byte;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movscbr", "f_indir_pc__byte 0x%x", 'x', f_indir_pc__byte, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_movscwr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cw.f
    UINT f_operand2;
    INT f_indir_pc__word;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_indir_pc__word = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_indir_pc__word) = f_indir_pc__word;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movscwr", "f_indir_pc__word 0x%x", 'x', f_indir_pc__word, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_movucbr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cb.f
    UINT f_operand2;
    INT f_indir_pc__byte;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_indir_pc__byte = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_indir_pc__byte) = f_indir_pc__byte;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movucbr", "f_indir_pc__byte 0x%x", 'x', f_indir_pc__byte, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_movucwr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cw.f
    UINT f_operand2;
    INT f_indir_pc__word;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_indir_pc__word = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));

  /* Record the fields for the semantic handler.  */
  FLD (f_indir_pc__word) = f_indir_pc__word;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movucwr", "f_indir_pc__word 0x%x", 'x', f_indir_pc__word, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addq:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addq.f
    UINT f_operand2;
    UINT f_u6;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_u6 = EXTRACT_LSB0_UINT (insn, 16, 5, 6);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_u6) = f_u6;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addq", "f_operand2 0x%x", 'x', f_operand2, "f_u6 0x%x", 'x', f_u6, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_cmp_r_b_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_muls_b.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmp_r_b_r", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_cmp_m_b_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmp_m_b_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_cmp_m_w_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmp_m_w_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_cmp_m_d_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmp_m_d_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_cmpcbr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cb.f
    INT f_indir_pc__byte;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__byte = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__byte) = f_indir_pc__byte;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmpcbr", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__byte 0x%x", 'x', f_indir_pc__byte, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_cmpcwr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cw.f
    INT f_indir_pc__word;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__word = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__word) = f_indir_pc__word;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmpcwr", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__word 0x%x", 'x', f_indir_pc__word, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_cmpcdr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cd.f
    INT f_indir_pc__dword;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__dword = (0|(EXTRACT_LSB0_UINT (word_1, 32, 31, 32) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__dword) = f_indir_pc__dword;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmpcdr", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__dword 0x%x", 'x', f_indir_pc__dword, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_cmpq:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_andq.f
    UINT f_operand2;
    INT f_s6;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_s6 = EXTRACT_LSB0_SINT (insn, 16, 5, 6);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_s6) = f_s6;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmpq", "f_operand2 0x%x", 'x', f_operand2, "f_s6 0x%x", 'x', f_s6, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_cmpucbr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cb.f
    INT f_indir_pc__byte;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__byte = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__byte) = f_indir_pc__byte;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmpucbr", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__byte 0x%x", 'x', f_indir_pc__byte, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_cmpucwr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cw.f
    INT f_indir_pc__word;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__word = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__word) = f_indir_pc__word;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_cmpucwr", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__word 0x%x", 'x', f_indir_pc__word, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_move_m_b_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_add_m_b_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_operand2) = f_operand2;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_move_m_b_m", "f_operand1 0x%x", 'x', f_operand1, "f_operand2 0x%x", 'x', f_operand2, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
      FLD (out_h_gr_SI_if__SI_andif__DFLT_prefix_set_not__UINT_inc_index_of__INT_Rs_index_of__INT_Rd) = ((ANDIF (GET_H_INSN_PREFIXED_P (), (! (FLD (f_memmode))))) ? (FLD (f_operand1)) : (FLD (f_operand2)));
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_move_m_w_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_add_m_b_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_operand2) = f_operand2;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_move_m_w_m", "f_operand1 0x%x", 'x', f_operand1, "f_operand2 0x%x", 'x', f_operand2, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
      FLD (out_h_gr_SI_if__SI_andif__DFLT_prefix_set_not__UINT_inc_index_of__INT_Rs_index_of__INT_Rd) = ((ANDIF (GET_H_INSN_PREFIXED_P (), (! (FLD (f_memmode))))) ? (FLD (f_operand1)) : (FLD (f_operand2)));
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_move_m_d_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_add_m_b_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_operand2) = f_operand2;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_move_m_d_m", "f_operand1 0x%x", 'x', f_operand1, "f_operand2 0x%x", 'x', f_operand2, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
      FLD (out_h_gr_SI_if__SI_andif__DFLT_prefix_set_not__UINT_inc_index_of__INT_Rs_index_of__INT_Rd) = ((ANDIF (GET_H_INSN_PREFIXED_P (), (! (FLD (f_memmode))))) ? (FLD (f_operand1)) : (FLD (f_operand2)));
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_movs_m_b_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_movs_m_b_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movs_m_b_m", "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_Rd) = f_operand2;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_movs_m_w_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_movs_m_b_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movs_m_w_m", "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_Rd) = f_operand2;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_move_r_sprv32:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_move_m_sprv32.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_move_r_sprv32", "f_operand1 0x%x", 'x', f_operand1, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_Pd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_move_spr_rv32:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_mcp.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_move_spr_rv32", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ps) = f_operand2;
      FLD (out_h_gr_SI_index_of__INT_Rs) = FLD (f_operand1);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_move_m_sprv32:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_move_m_sprv32.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_operand2) = f_operand2;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_move_m_sprv32", "f_operand1 0x%x", 'x', f_operand1, "f_operand2 0x%x", 'x', f_operand2, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_Pd) = f_operand2;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_move_c_sprv32_p2:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_move_c_sprv32_p2.f
    INT f_indir_pc__dword;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__dword = (0|(EXTRACT_LSB0_UINT (word_1, 32, 31, 32) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_indir_pc__dword) = f_indir_pc__dword;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_move_c_sprv32_p2", "f_indir_pc__dword 0x%x", 'x', f_indir_pc__dword, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Pd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_move_spr_mv32:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_move_spr_mv32.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_move_spr_mv32", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ps) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_move_ss_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_move_spr_mv32.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_move_ss_r", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_move_r_ss:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_mcp.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_move_r_ss", "f_operand1 0x%x", 'x', f_operand1, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_movem_r_m_v32:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_movem_r_m_v32.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movem_r_m_v32", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (in_h_gr_SI_0) = 0;
      FLD (in_h_gr_SI_1) = 1;
      FLD (in_h_gr_SI_10) = 10;
      FLD (in_h_gr_SI_11) = 11;
      FLD (in_h_gr_SI_12) = 12;
      FLD (in_h_gr_SI_13) = 13;
      FLD (in_h_gr_SI_14) = 14;
      FLD (in_h_gr_SI_15) = 15;
      FLD (in_h_gr_SI_2) = 2;
      FLD (in_h_gr_SI_3) = 3;
      FLD (in_h_gr_SI_4) = 4;
      FLD (in_h_gr_SI_5) = 5;
      FLD (in_h_gr_SI_6) = 6;
      FLD (in_h_gr_SI_7) = 7;
      FLD (in_h_gr_SI_8) = 8;
      FLD (in_h_gr_SI_9) = 9;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_movem_m_r_v32:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_movem_m_r_v32.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_movem_m_r_v32", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
      FLD (out_h_gr_SI_0) = 0;
      FLD (out_h_gr_SI_1) = 1;
      FLD (out_h_gr_SI_10) = 10;
      FLD (out_h_gr_SI_11) = 11;
      FLD (out_h_gr_SI_12) = 12;
      FLD (out_h_gr_SI_13) = 13;
      FLD (out_h_gr_SI_14) = 14;
      FLD (out_h_gr_SI_15) = 15;
      FLD (out_h_gr_SI_2) = 2;
      FLD (out_h_gr_SI_3) = 3;
      FLD (out_h_gr_SI_4) = 4;
      FLD (out_h_gr_SI_5) = 5;
      FLD (out_h_gr_SI_6) = 6;
      FLD (out_h_gr_SI_7) = 7;
      FLD (out_h_gr_SI_8) = 8;
      FLD (out_h_gr_SI_9) = 9;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_add_b_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_add_b_r", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_add_d_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_add_d_r", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_add_m_b_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_add_m_b_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_add_m_b_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
      FLD (out_h_gr_SI_if__SI_andif__DFLT_prefix_set_not__UINT_inc_index_of__INT_Rs_index_of__INT_Rd) = ((ANDIF (GET_H_INSN_PREFIXED_P (), (! (FLD (f_memmode))))) ? (FLD (f_operand1)) : (FLD (f_operand2)));
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_add_m_w_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_add_m_b_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_add_m_w_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
      FLD (out_h_gr_SI_if__SI_andif__DFLT_prefix_set_not__UINT_inc_index_of__INT_Rs_index_of__INT_Rd) = ((ANDIF (GET_H_INSN_PREFIXED_P (), (! (FLD (f_memmode))))) ? (FLD (f_operand1)) : (FLD (f_operand2)));
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_add_m_d_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_add_m_b_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_add_m_d_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
      FLD (out_h_gr_SI_if__SI_andif__DFLT_prefix_set_not__UINT_inc_index_of__INT_Rs_index_of__INT_Rd) = ((ANDIF (GET_H_INSN_PREFIXED_P (), (! (FLD (f_memmode))))) ? (FLD (f_operand1)) : (FLD (f_operand2)));
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addcbr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addcbr.f
    INT f_indir_pc__byte;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__byte = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__byte) = f_indir_pc__byte;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addcbr", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__byte 0x%x", 'x', f_indir_pc__byte, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addcwr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addcwr.f
    INT f_indir_pc__word;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__word = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__word) = f_indir_pc__word;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addcwr", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__word 0x%x", 'x', f_indir_pc__word, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addcdr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addcdr.f
    INT f_indir_pc__dword;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__dword = (0|(EXTRACT_LSB0_UINT (word_1, 32, 31, 32) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__dword) = f_indir_pc__dword;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addcdr", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__dword 0x%x", 'x', f_indir_pc__dword, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_adds_m_b_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_add_m_b_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_adds_m_b_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
      FLD (out_h_gr_SI_if__SI_andif__DFLT_prefix_set_not__UINT_inc_index_of__INT_Rs_index_of__INT_Rd) = ((ANDIF (GET_H_INSN_PREFIXED_P (), (! (FLD (f_memmode))))) ? (FLD (f_operand1)) : (FLD (f_operand2)));
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_adds_m_w_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_add_m_b_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_adds_m_w_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
      FLD (out_h_gr_SI_if__SI_andif__DFLT_prefix_set_not__UINT_inc_index_of__INT_Rs_index_of__INT_Rd) = ((ANDIF (GET_H_INSN_PREFIXED_P (), (! (FLD (f_memmode))))) ? (FLD (f_operand1)) : (FLD (f_operand2)));
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addscbr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addcbr.f
    INT f_indir_pc__byte;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__byte = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__byte) = f_indir_pc__byte;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addscbr", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__byte 0x%x", 'x', f_indir_pc__byte, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addscwr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addcwr.f
    INT f_indir_pc__word;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__word = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__word) = f_indir_pc__word;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addscwr", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__word 0x%x", 'x', f_indir_pc__word, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addc_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addc_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_lapc_d:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_lapc_d.f
    SI f_indir_pc__dword_pcrel;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__dword_pcrel = ((pc) + ((0|(EXTRACT_LSB0_UINT (word_1, 32, 31, 32) << 0))));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (i_const32_pcrel) = f_indir_pc__dword_pcrel;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lapc_d", "f_operand2 0x%x", 'x', f_operand2, "const32_pcrel 0x%x", 'x', f_indir_pc__dword_pcrel, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_lapcq:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_lapcq.f
    UINT f_operand2;
    SI f_qo;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_qo = ((pc) + (((EXTRACT_LSB0_UINT (insn, 16, 3, 4)) << (1))));

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (i_qo) = f_qo;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lapcq", "f_operand2 0x%x", 'x', f_operand2, "qo 0x%x", 'x', f_qo, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addi_b_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addi_b_r", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_neg_b_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_neg_b_r", "f_operand1 0x%x", 'x', f_operand1, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_neg_d_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_neg_d_r", "f_operand1 0x%x", 'x', f_operand1, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_test_m_b_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_move_spr_mv32.f
    UINT f_memmode;
    UINT f_operand1;

    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_test_m_b_m", "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_test_m_w_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_move_spr_mv32.f
    UINT f_memmode;
    UINT f_operand1;

    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_test_m_w_m", "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_test_m_d_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_move_spr_mv32.f
    UINT f_memmode;
    UINT f_operand1;

    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_test_m_d_m", "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_move_r_m_b_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_move_r_m_b_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_move_r_m_w_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_move_r_m_w_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_move_r_m_d_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_move_r_m_d_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_muls_b:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_muls_b.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_muls_b", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rd) = f_operand2;
      FLD (out_h_sr_SI_7) = 7;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_mcp:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_mcp.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_mcp", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ps) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_h_gr_SI_index_of__INT_Rs) = FLD (f_operand1);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_dstep:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_muls_b.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_dstep", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_and_b_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_and_b_r", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_and_d_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_and_d_r", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_and_m_b_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_add_m_b_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_and_m_b_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
      FLD (out_h_gr_SI_if__SI_andif__DFLT_prefix_set_not__UINT_inc_index_of__INT_Rs_index_of__INT_Rd) = ((ANDIF (GET_H_INSN_PREFIXED_P (), (! (FLD (f_memmode))))) ? (FLD (f_operand1)) : (FLD (f_operand2)));
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_and_m_w_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_add_m_b_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_and_m_w_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
      FLD (out_h_gr_SI_if__SI_andif__DFLT_prefix_set_not__UINT_inc_index_of__INT_Rs_index_of__INT_Rd) = ((ANDIF (GET_H_INSN_PREFIXED_P (), (! (FLD (f_memmode))))) ? (FLD (f_operand1)) : (FLD (f_operand2)));
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_and_m_d_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_add_m_b_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_and_m_d_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
      FLD (out_h_gr_SI_if__SI_andif__DFLT_prefix_set_not__UINT_inc_index_of__INT_Rs_index_of__INT_Rd) = ((ANDIF (GET_H_INSN_PREFIXED_P (), (! (FLD (f_memmode))))) ? (FLD (f_operand1)) : (FLD (f_operand2)));
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_andcbr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addcbr.f
    INT f_indir_pc__byte;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__byte = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__byte) = f_indir_pc__byte;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_andcbr", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__byte 0x%x", 'x', f_indir_pc__byte, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_andcwr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addcwr.f
    INT f_indir_pc__word;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__word = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__word) = f_indir_pc__word;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_andcwr", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__word 0x%x", 'x', f_indir_pc__word, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_andcdr:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addcdr.f
    INT f_indir_pc__dword;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__dword = (0|(EXTRACT_LSB0_UINT (word_1, 32, 31, 32) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__dword) = f_indir_pc__dword;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_andcdr", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__dword 0x%x", 'x', f_indir_pc__dword, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_andq:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_andq.f
    UINT f_operand2;
    INT f_s6;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_s6 = EXTRACT_LSB0_SINT (insn, 16, 5, 6);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_s6) = f_s6;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_andq", "f_operand2 0x%x", 'x', f_operand2, "f_s6 0x%x", 'x', f_s6, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_swap:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_move_spr_mv32.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_swap", "f_operand1 0x%x", 'x', f_operand1, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_asrq:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_asrq.f
    UINT f_operand2;
    UINT f_u5;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_u5 = EXTRACT_LSB0_UINT (insn, 16, 4, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_u5) = f_u5;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_asrq", "f_operand2 0x%x", 'x', f_operand2, "f_u5 0x%x", 'x', f_u5, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (out_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_lsrr_b_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lsrr_b_r", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_lsrr_d_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_lsrr_d_r", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_h_gr_SI_index_of__INT_Rd) = FLD (f_operand2);
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_btst:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_muls_b.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_btst", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_btstq:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_asrq.f
    UINT f_operand2;
    UINT f_u5;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_u5 = EXTRACT_LSB0_UINT (insn, 16, 4, 5);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_u5) = f_u5;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_btstq", "f_operand2 0x%x", 'x', f_operand2, "f_u5 0x%x", 'x', f_u5, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_setf:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_setf.f
    UINT f_operand2;
    UINT f_operand1;
    UINT f_dstsrc;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);
  f_dstsrc = ((((f_operand1) | (((f_operand2) << (4))))) & (255));

  /* Record the fields for the semantic handler.  */
  FLD (f_dstsrc) = f_dstsrc;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_setf", "f_dstsrc 0x%x", 'x', f_dstsrc, (char *) 0));

#undef FLD
    return idesc;
  }

 extract_sfmt_rfe:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_rfe.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_rfe", (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_sr_SI_13) = 13;
      FLD (out_h_sr_SI_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_sfe:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_rfe.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_sfe", (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_sr_SI_13) = 13;
      FLD (out_h_sr_SI_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_rfg:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_rfg", (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_rfn:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_rfe.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_rfn", (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_h_sr_SI_13) = 13;
      FLD (out_h_sr_SI_13) = 13;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_halt:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
#define FLD(f) abuf->fields.sfmt_empty.f


  /* Record the fields for the semantic handler.  */
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_halt", (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bcc_b:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bcc_b.f
    UINT f_operand2;
    UINT f_disp9_lo;
    INT f_disp9_hi;
    INT f_disp9;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_disp9_lo = EXTRACT_LSB0_UINT (insn, 16, 7, 7);
    f_disp9_hi = EXTRACT_LSB0_SINT (insn, 16, 0, 1);
{
  SI tmp_abslo;
  SI tmp_absval;
  tmp_abslo = ((f_disp9_lo) << (1));
  tmp_absval = ((((((f_disp9_hi) != (0))) ? ((~ (255))) : (0))) | (tmp_abslo));
  f_disp9 = ((((pc) + (tmp_absval))) + (((GET_H_V32_V32 ()) ? (0) : (2))));
}

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (i_o_pcrel) = f_disp9;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bcc_b", "f_operand2 0x%x", 'x', f_operand2, "o_pcrel 0x%x", 'x', f_disp9, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ba_b:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bcc_b.f
    UINT f_disp9_lo;
    INT f_disp9_hi;
    INT f_disp9;

    f_disp9_lo = EXTRACT_LSB0_UINT (insn, 16, 7, 7);
    f_disp9_hi = EXTRACT_LSB0_SINT (insn, 16, 0, 1);
{
  SI tmp_abslo;
  SI tmp_absval;
  tmp_abslo = ((f_disp9_lo) << (1));
  tmp_absval = ((((((f_disp9_hi) != (0))) ? ((~ (255))) : (0))) | (tmp_abslo));
  f_disp9 = ((((pc) + (tmp_absval))) + (((GET_H_V32_V32 ()) ? (0) : (2))));
}

  /* Record the fields for the semantic handler.  */
  FLD (i_o_pcrel) = f_disp9;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ba_b", "o_pcrel 0x%x", 'x', f_disp9, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bcc_w:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bcc_w.f
    SI f_indir_pc__word_pcrel;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__word_pcrel = ((EXTHISI (((HI) (UINT) ((0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0)))))) + (((pc) + (((GET_H_V32_V32 ()) ? (0) : (4))))));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (i_o_word_pcrel) = f_indir_pc__word_pcrel;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bcc_w", "f_operand2 0x%x", 'x', f_operand2, "o_word_pcrel 0x%x", 'x', f_indir_pc__word_pcrel, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_ba_w:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bcc_w.f
    SI f_indir_pc__word_pcrel;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__word_pcrel = ((EXTHISI (((HI) (UINT) ((0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0)))))) + (((pc) + (((GET_H_V32_V32 ()) ? (0) : (4))))));

  /* Record the fields for the semantic handler.  */
  FLD (i_o_word_pcrel) = f_indir_pc__word_pcrel;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_ba_w", "o_word_pcrel 0x%x", 'x', f_indir_pc__word_pcrel, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_jas_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_move_m_sprv32.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_jas_r", "f_operand1 0x%x", 'x', f_operand1, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_Pd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_jas_c:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_move_c_sprv32_p2.f
    INT f_indir_pc__dword;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__dword = (0|(EXTRACT_LSB0_UINT (word_1, 32, 31, 32) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_indir_pc__dword) = f_indir_pc__dword;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_jas_c", "f_indir_pc__dword 0x%x", 'x', f_indir_pc__dword, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Pd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_jump_p:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_mcp.f
    UINT f_operand2;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_jump_p", "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Ps) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bas_c:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bas_c.f
    SI f_indir_pc__dword_pcrel;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__dword_pcrel = ((pc) + ((0|(EXTRACT_LSB0_UINT (word_1, 32, 31, 32) << 0))));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (i_const32_pcrel) = f_indir_pc__dword_pcrel;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bas_c", "f_operand2 0x%x", 'x', f_operand2, "const32_pcrel 0x%x", 'x', f_indir_pc__dword_pcrel, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Pd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_jasc_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_move_m_sprv32.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  FLD (f_operand2) = f_operand2;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_jasc_r", "f_operand1 0x%x", 'x', f_operand1, "f_operand2 0x%x", 'x', f_operand2, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
      FLD (out_Pd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_break:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_break.f
    UINT f_u4;

    f_u4 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_u4) = f_u4;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_break", "f_u4 0x%x", 'x', f_u4, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bound_cb:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cb.f
    INT f_indir_pc__byte;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__byte = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__byte) = f_indir_pc__byte;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bound_cb", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__byte 0x%x", 'x', f_indir_pc__byte, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (out_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bound_cw:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cw.f
    INT f_indir_pc__word;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__word = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__word) = f_indir_pc__word;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bound_cw", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__word 0x%x", 'x', f_indir_pc__word, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (out_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_bound_cd:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cd.f
    INT f_indir_pc__dword;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__dword = (0|(EXTRACT_LSB0_UINT (word_1, 32, 31, 32) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__dword) = f_indir_pc__dword;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_bound_cd", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__dword 0x%x", 'x', f_indir_pc__dword, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (out_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_scc:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_move_spr_mv32.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_scc", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addoq:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addoq.f
    UINT f_operand2;
    INT f_s8;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_s8 = EXTRACT_LSB0_SINT (insn, 16, 7, 8);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_s8) = f_s8;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addoq", "f_operand2 0x%x", 'x', f_operand2, "f_s8 0x%x", 'x', f_s8, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addo_m_b_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addo_m_b_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addo_m_w_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addo_m_w_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addo_m_d_m:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_addc_m.f
    UINT f_operand2;
    UINT f_memmode;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_memmode = EXTRACT_LSB0_UINT (insn, 16, 10, 1);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  FLD (f_memmode) = f_memmode;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addo_m_d_m", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, "f_memmode 0x%x", 'x', f_memmode, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
      FLD (out_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addo_cb:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cb.f
    INT f_indir_pc__byte;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__byte = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__byte) = f_indir_pc__byte;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addo_cb", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__byte 0x%x", 'x', f_indir_pc__byte, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addo_cw:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cw.f
    INT f_indir_pc__word;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__word = (0|(EXTRACT_LSB0_UINT (word_1, 32, 15, 16) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__word) = f_indir_pc__word;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addo_cw", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__word 0x%x", 'x', f_indir_pc__word, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addo_cd:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_bound_cd.f
    INT f_indir_pc__dword;
    UINT f_operand2;
    /* Contents of trailing part of insn.  */
    UINT word_1;

  word_1 = GETIMEMUSI (current_cpu, pc + 2);
    f_indir_pc__dword = (0|(EXTRACT_LSB0_UINT (word_1, 32, 31, 32) << 0));
    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_indir_pc__dword) = f_indir_pc__dword;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addo_cd", "f_operand2 0x%x", 'x', f_operand2, "f_indir_pc__dword 0x%x", 'x', f_indir_pc__dword, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_addi_acr_b_r:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_muls_b.f
    UINT f_operand2;
    UINT f_operand1;

    f_operand2 = EXTRACT_LSB0_UINT (insn, 16, 15, 4);
    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand2) = f_operand2;
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_addi_acr_b_r", "f_operand2 0x%x", 'x', f_operand2, "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rd) = f_operand2;
      FLD (in_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

 extract_sfmt_fidxi:
  {
    const IDESC *idesc = &crisv32f_insn_data[itype];
    CGEN_INSN_WORD insn ATTRIBUTE_UNUSED = base_insn;
#define FLD(f) abuf->fields.sfmt_mcp.f
    UINT f_operand1;

    f_operand1 = EXTRACT_LSB0_UINT (insn, 16, 3, 4);

  /* Record the fields for the semantic handler.  */
  FLD (f_operand1) = f_operand1;
  CGEN_TRACE_EXTRACT (current_cpu, abuf, (current_cpu, pc, "sfmt_fidxi", "f_operand1 0x%x", 'x', f_operand1, (char *) 0));

#if WITH_PROFILE_MODEL_P
  /* Record the fields for profiling.  */
  if (PROFILE_MODEL_P (current_cpu))
    {
      FLD (in_Rs) = f_operand1;
    }
#endif
#undef FLD
    return idesc;
  }

}
