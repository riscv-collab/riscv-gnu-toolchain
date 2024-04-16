/* Decode header for lm32bf.

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

#ifndef LM32BF_DECODE_H
#define LM32BF_DECODE_H

extern const IDESC *lm32bf_decode (SIM_CPU *, IADDR,
                                  CGEN_INSN_WORD, CGEN_INSN_WORD,
                                  ARGBUF *);
extern void lm32bf_init_idesc_table (SIM_CPU *);
extern void lm32bf_sem_init_idesc_table (SIM_CPU *);
extern void lm32bf_semf_init_idesc_table (SIM_CPU *);

/* Enum declaration for instructions in cpu family lm32bf.  */
typedef enum lm32bf_insn_type {
  LM32BF_INSN_X_INVALID, LM32BF_INSN_X_AFTER, LM32BF_INSN_X_BEFORE, LM32BF_INSN_X_CTI_CHAIN
 , LM32BF_INSN_X_CHAIN, LM32BF_INSN_X_BEGIN, LM32BF_INSN_ADD, LM32BF_INSN_ADDI
 , LM32BF_INSN_AND, LM32BF_INSN_ANDI, LM32BF_INSN_ANDHII, LM32BF_INSN_B
 , LM32BF_INSN_BI, LM32BF_INSN_BE, LM32BF_INSN_BG, LM32BF_INSN_BGE
 , LM32BF_INSN_BGEU, LM32BF_INSN_BGU, LM32BF_INSN_BNE, LM32BF_INSN_CALL
 , LM32BF_INSN_CALLI, LM32BF_INSN_CMPE, LM32BF_INSN_CMPEI, LM32BF_INSN_CMPG
 , LM32BF_INSN_CMPGI, LM32BF_INSN_CMPGE, LM32BF_INSN_CMPGEI, LM32BF_INSN_CMPGEU
 , LM32BF_INSN_CMPGEUI, LM32BF_INSN_CMPGU, LM32BF_INSN_CMPGUI, LM32BF_INSN_CMPNE
 , LM32BF_INSN_CMPNEI, LM32BF_INSN_DIVU, LM32BF_INSN_LB, LM32BF_INSN_LBU
 , LM32BF_INSN_LH, LM32BF_INSN_LHU, LM32BF_INSN_LW, LM32BF_INSN_MODU
 , LM32BF_INSN_MUL, LM32BF_INSN_MULI, LM32BF_INSN_NOR, LM32BF_INSN_NORI
 , LM32BF_INSN_OR, LM32BF_INSN_ORI, LM32BF_INSN_ORHII, LM32BF_INSN_RCSR
 , LM32BF_INSN_SB, LM32BF_INSN_SEXTB, LM32BF_INSN_SEXTH, LM32BF_INSN_SH
 , LM32BF_INSN_SL, LM32BF_INSN_SLI, LM32BF_INSN_SR, LM32BF_INSN_SRI
 , LM32BF_INSN_SRU, LM32BF_INSN_SRUI, LM32BF_INSN_SUB, LM32BF_INSN_SW
 , LM32BF_INSN_USER, LM32BF_INSN_WCSR, LM32BF_INSN_XOR, LM32BF_INSN_XORI
 , LM32BF_INSN_XNOR, LM32BF_INSN_XNORI, LM32BF_INSN_BREAK, LM32BF_INSN_SCALL
 , LM32BF_INSN__MAX
} LM32BF_INSN_TYPE;

/* Enum declaration for semantic formats in cpu family lm32bf.  */
typedef enum lm32bf_sfmt_type {
  LM32BF_SFMT_EMPTY, LM32BF_SFMT_ADD, LM32BF_SFMT_ADDI, LM32BF_SFMT_ANDI
 , LM32BF_SFMT_ANDHII, LM32BF_SFMT_B, LM32BF_SFMT_BI, LM32BF_SFMT_BE
 , LM32BF_SFMT_CALL, LM32BF_SFMT_CALLI, LM32BF_SFMT_DIVU, LM32BF_SFMT_LB
 , LM32BF_SFMT_LH, LM32BF_SFMT_LW, LM32BF_SFMT_ORI, LM32BF_SFMT_RCSR
 , LM32BF_SFMT_SB, LM32BF_SFMT_SEXTB, LM32BF_SFMT_SH, LM32BF_SFMT_SW
 , LM32BF_SFMT_USER, LM32BF_SFMT_WCSR, LM32BF_SFMT_BREAK
} LM32BF_SFMT_TYPE;

/* Function unit handlers (user written).  */

extern int lm32bf_model_lm32_u_exec (SIM_CPU *, const IDESC *, int /*unit_num*/, int /*referenced*/);

/* Profiling before/after handlers (user written) */

extern void lm32bf_model_insn_before (SIM_CPU *, int /*first_p*/);
extern void lm32bf_model_insn_after (SIM_CPU *, int /*last_p*/, int /*cycles*/);

#endif /* LM32BF_DECODE_H */
