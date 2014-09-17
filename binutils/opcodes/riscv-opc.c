/* RISC-V opcode list
   Copyright 2011-2014 Free Software Foundation, Inc.

   Contributed by Andrew Waterman (waterman@cs.berkeley.edu) at UC Berkeley.
   Based on MIPS target.

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this file; see the file COPYING.  If not, write to the
   Free Software Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "opcode/riscv.h"
#include <stdio.h>

/* Short hand so the lines aren't too long.  */

/* The order of overloaded instructions matters.  Label arguments and
   register arguments look the same. Instructions that can have either
   for arguments must apear in the correct order in this table for the
   assembler to pick the right one. In other words, entries with
   immediate operands must apear after the same instruction with
   registers.

   Because of the lookup algorithm used, entries with the same opcode
   name must be contiguous. */

#define WR_xd INSN_WRITE_GPR_D
#define WR_fd INSN_WRITE_FPR_D
#define RD_xs1 INSN_READ_GPR_S
#define RD_xs2 INSN_READ_GPR_T
#define RD_fs1 INSN_READ_FPR_S
#define RD_fs2 INSN_READ_FPR_T
#define RD_fs3 INSN_READ_FPR_R

#define MASK_RS1 (OP_MASK_RS1 << OP_SH_RS1)
#define MASK_RS2 (OP_MASK_RS2 << OP_SH_RS2)
#define MASK_RD (OP_MASK_RD << OP_SH_RD)
#define MASK_IMM ENCODE_ITYPE_IMM(-1U)
#define MASK_UIMM ENCODE_UTYPE_IMM(-1U)
#define MASK_RM (OP_MASK_RM << OP_SH_RM)
#define MASK_PRED (OP_MASK_PRED << OP_SH_PRED)
#define MASK_SUCC (OP_MASK_SUCC << OP_SH_SUCC)
#define MASK_AQ (OP_MASK_AQ << OP_SH_AQ)
#define MASK_RL (OP_MASK_RL << OP_SH_RL)
#define MASK_AQRL (MASK_AQ | MASK_RL)

static int match_opcode(const struct riscv_opcode *op, insn_t insn)
{
  return (insn & op->mask) == op->match;
}

static int match_never(const struct riscv_opcode *op ATTRIBUTE_UNUSED,
		       insn_t insn ATTRIBUTE_UNUSED)
{
  return 0;
}

static int match_rs1_eq_rs2(const struct riscv_opcode *op, insn_t insn)
{
  return match_opcode(op, insn) &&
    ((insn & MASK_RS1) >> OP_SH_RS1) == ((insn & MASK_RS2) >> OP_SH_RS2);
}

const struct riscv_opcode riscv_builtin_opcodes[] =
{
/* These instructions appear first so that the disassembler will find
   them first.  The assemblers uses a hash table based on the
   instruction name anyhow.  */
/* name,      isa,   operands, match, mask, pinfo */
{"unimp",     "I",   "",         0, 0xffff,  match_opcode, 0 },
{"nop",       "I",   "",         MATCH_ADDI, MASK_ADDI | MASK_RD | MASK_RS1 | MASK_IMM, match_opcode,  INSN_ALIAS },
{"li",        "I",   "d,j",      MATCH_ADDI, MASK_ADDI | MASK_RS1, match_opcode,  INSN_ALIAS|WR_xd }, /* addi */
{"li",        "I",   "d,I",  0,    (int) M_LI,  match_never, INSN_MACRO },
{"mv",        "I",   "d,s",  MATCH_ADDI, MASK_ADDI | MASK_IMM, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"move",      "I",   "d,s",  MATCH_ADDI, MASK_ADDI | MASK_IMM, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"b",         "I",   "p",  MATCH_BEQ, MASK_BEQ | MASK_RS1 | MASK_RS2, match_opcode,   0 },/* beq 0,0 */
{"andi",      "I",   "d,s,j",  MATCH_ANDI, MASK_ANDI, match_opcode,   WR_xd|RD_xs1 },
{"and",       "I",   "d,s,t",  MATCH_AND, MASK_AND, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"and",       "I",   "d,s,j",  MATCH_ANDI, MASK_ANDI, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"beqz",      "I",   "s,p",  MATCH_BEQ, MASK_BEQ | MASK_RS2, match_opcode,   INSN_ALIAS|RD_xs1 },
{"beq",       "I",   "s,t,p",  MATCH_BEQ, MASK_BEQ, match_opcode,   RD_xs1|RD_xs2 },
{"blez",      "I",   "t,p",  MATCH_BGE, MASK_BGE | MASK_RS1, match_opcode,   INSN_ALIAS|RD_xs2 },
{"bgez",      "I",   "s,p",  MATCH_BGE, MASK_BGE | MASK_RS2, match_opcode,   INSN_ALIAS|RD_xs1 },
{"ble",       "I",   "t,s,p",  MATCH_BGE, MASK_BGE, match_opcode,   INSN_ALIAS|RD_xs1|RD_xs2 },
{"bleu",      "I",   "t,s,p",  MATCH_BGEU, MASK_BGEU, match_opcode,   INSN_ALIAS|RD_xs1|RD_xs2 },
{"bge",       "I",   "s,t,p",  MATCH_BGE, MASK_BGE, match_opcode,   RD_xs1|RD_xs2 },
{"bgeu",      "I",   "s,t,p",  MATCH_BGEU, MASK_BGEU, match_opcode,   RD_xs1|RD_xs2 },
{"bltz",      "I",   "s,p",  MATCH_BLT, MASK_BLT | MASK_RS2, match_opcode,   INSN_ALIAS|RD_xs1 },
{"bgtz",      "I",   "t,p",  MATCH_BLT, MASK_BLT | MASK_RS1, match_opcode,   INSN_ALIAS|RD_xs2 },
{"blt",       "I",   "s,t,p",  MATCH_BLT, MASK_BLT, match_opcode,   RD_xs1|RD_xs2 },
{"bltu",      "I",   "s,t,p",  MATCH_BLTU, MASK_BLTU, match_opcode,   RD_xs1|RD_xs2 },
{"bgt",       "I",   "t,s,p",  MATCH_BLT, MASK_BLT, match_opcode,   INSN_ALIAS|RD_xs1|RD_xs2 },
{"bgtu",      "I",   "t,s,p",  MATCH_BLTU, MASK_BLTU, match_opcode,   INSN_ALIAS|RD_xs1|RD_xs2 },
{"bnez",      "I",   "s,p",  MATCH_BNE, MASK_BNE | MASK_RS2, match_opcode,   INSN_ALIAS|RD_xs1 },
{"bne",       "I",   "s,t,p",  MATCH_BNE, MASK_BNE, match_opcode,   RD_xs1|RD_xs2 },
{"addi",      "I",   "d,s,j",  MATCH_ADDI, MASK_ADDI, match_opcode,  WR_xd|RD_xs1 },
{"add",       "I",   "d,s,t",  MATCH_ADD, MASK_ADD, match_opcode,  WR_xd|RD_xs1|RD_xs2 },
{"add",       "I",   "d,s,t,0",MATCH_ADD, MASK_ADD, match_opcode,  WR_xd|RD_xs1|RD_xs2 },
{"add",       "I",   "d,s,j",  MATCH_ADDI, MASK_ADDI, match_opcode,  INSN_ALIAS|WR_xd|RD_xs1 },
{"la",        "I",   "d,A",  0,    (int) M_LA,  match_never, INSN_MACRO },
{"lla",       "I",   "d,A",  0,    (int) M_LLA,  match_never, INSN_MACRO },
{"la.tls.gd", "I",   "d,A",  0,    (int) M_LA_TLS_GD,  match_never, INSN_MACRO },
{"la.tls.ie", "I",   "d,A",  0,    (int) M_LA_TLS_IE,  match_never, INSN_MACRO },
{"neg",       "I",   "d,t",  MATCH_SUB, MASK_SUB | MASK_RS1, match_opcode,   INSN_ALIAS|WR_xd|RD_xs2 }, /* sub 0 */
{"slli",      "I",   "d,s,>",   MATCH_SLLI, MASK_SLLI, match_opcode,   WR_xd|RD_xs1 },
{"sll",       "I",   "d,s,t",   MATCH_SLL, MASK_SLL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"sll",       "I",   "d,s,>",   MATCH_SLLI, MASK_SLLI, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"srli",      "I",   "d,s,>",   MATCH_SRLI, MASK_SRLI, match_opcode,   WR_xd|RD_xs1 },
{"srl",       "I",   "d,s,t",   MATCH_SRL, MASK_SRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"srl",       "I",   "d,s,>",   MATCH_SRLI, MASK_SRLI, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"srai",      "I",   "d,s,>",   MATCH_SRAI, MASK_SRAI, match_opcode,   WR_xd|RD_xs1 },
{"sra",       "I",   "d,s,t",   MATCH_SRA, MASK_SRA, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"sra",       "I",   "d,s,>",   MATCH_SRAI, MASK_SRAI, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"sub",       "I",   "d,s,t",  MATCH_SUB, MASK_SUB, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"ret",       "I",   "",  MATCH_JALR | (LINK_REG << OP_SH_RS1), MASK_JALR | MASK_RD | MASK_RS1 | MASK_IMM, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"j",         "I",   "a",  MATCH_JAL, MASK_JAL | MASK_RD, match_opcode,   INSN_ALIAS },
{"jal",       "I",   "a",  MATCH_JAL | (LINK_REG << OP_SH_RD), MASK_JAL | MASK_RD, match_opcode,   INSN_ALIAS|WR_xd },
{"jal",       "I",   "d,a",  MATCH_JAL, MASK_JAL, match_opcode,   WR_xd },
{"call",      "I",   "c",  0,    (int) M_CALL,  match_never, INSN_MACRO },
{"jump",      "I",   "c",  0,    (int) M_JUMP,  match_never, INSN_MACRO },
{"jr",        "I",   "s",  MATCH_JALR, MASK_JALR | MASK_RD | MASK_IMM, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"jr",        "I",   "s,j",  MATCH_JALR, MASK_JALR | MASK_RD, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"jalr",      "I",   "s",  MATCH_JALR | (LINK_REG << OP_SH_RD), MASK_JALR | MASK_RD | MASK_IMM, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"jalr",      "I",   "s,j",  MATCH_JALR | (LINK_REG << OP_SH_RD), MASK_JALR | MASK_RD, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"jalr",      "I",   "d,s",  MATCH_JALR, MASK_JALR | MASK_IMM, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"jalr",      "I",   "d,s,j",  MATCH_JALR, MASK_JALR, match_opcode,   WR_xd|RD_xs1 },
{"lb",        "I",   "d,o(s)",  MATCH_LB, MASK_LB, match_opcode,   WR_xd|RD_xs1 },
{"lb",        "I",   "d,A",  0, (int) M_LB, match_never, INSN_MACRO },
{"lbu",       "I",   "d,o(s)",  MATCH_LBU, MASK_LBU, match_opcode,   WR_xd|RD_xs1 },
{"lbu",       "I",   "d,A",  0, (int) M_LBU, match_never, INSN_MACRO },
{"lh",        "I",   "d,o(s)",  MATCH_LH, MASK_LH, match_opcode,   WR_xd|RD_xs1 },
{"lh",        "I",   "d,A",  0, (int) M_LH, match_never, INSN_MACRO },
{"lhu",       "I",   "d,o(s)",  MATCH_LHU, MASK_LHU, match_opcode,   WR_xd|RD_xs1 },
{"lhu",       "I",   "d,A",  0, (int) M_LHU, match_never, INSN_MACRO },
{"lw",        "I",   "d,o(s)",  MATCH_LW, MASK_LW, match_opcode,   WR_xd|RD_xs1 },
{"lw",        "I",   "d,A",  0, (int) M_LW, match_never, INSN_MACRO },
{"lui",       "I",   "d,u",  MATCH_LUI, MASK_LUI, match_opcode,   WR_xd },
{"not",       "I",   "d,s",  MATCH_XORI | MASK_IMM, MASK_XORI | MASK_IMM, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"ori",       "I",   "d,s,j",  MATCH_ORI, MASK_ORI, match_opcode,   WR_xd|RD_xs1 },
{"or",        "I",   "d,s,t",  MATCH_OR, MASK_OR, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"or",        "I",   "d,s,j",  MATCH_ORI, MASK_ORI, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"auipc",     "I",   "d,u",  MATCH_AUIPC, MASK_AUIPC, match_opcode,  WR_xd },
{"seqz",      "I",   "d,s",  MATCH_SLTIU | ENCODE_ITYPE_IMM(1), MASK_SLTIU | MASK_IMM, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"snez",      "I",   "d,t",  MATCH_SLTU, MASK_SLTU | MASK_RS1, match_opcode,   INSN_ALIAS|WR_xd|RD_xs2 },
{"sltz",      "I",   "d,s",  MATCH_SLT, MASK_SLT | MASK_RS2, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"sgtz",      "I",   "d,t",  MATCH_SLT, MASK_SLT | MASK_RS1, match_opcode,   INSN_ALIAS|WR_xd|RD_xs2 },
{"slti",      "I",   "d,s,j",  MATCH_SLTI, MASK_SLTI, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"slt",       "I",   "d,s,t",  MATCH_SLT, MASK_SLT, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"slt",       "I",   "d,s,j",  MATCH_SLTI, MASK_SLTI, match_opcode,   WR_xd|RD_xs1 },
{"sltiu",     "I",   "d,s,j",  MATCH_SLTIU, MASK_SLTIU, match_opcode,   WR_xd|RD_xs1 },
{"sltu",      "I",   "d,s,t",  MATCH_SLTU, MASK_SLTU, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"sltu",      "I",   "d,s,j",  MATCH_SLTIU, MASK_SLTIU, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"sgt",       "I",   "d,t,s",  MATCH_SLT, MASK_SLT, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1|RD_xs2 },
{"sgtu",      "I",   "d,t,s",  MATCH_SLTU, MASK_SLTU, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1|RD_xs2 },
{"sb",        "I",   "t,q(s)",  MATCH_SB, MASK_SB, match_opcode,   RD_xs1|RD_xs2 },
{"sb",        "I",   "t,A,s",  0, (int) M_SB, match_never,  INSN_MACRO },
{"sh",        "I",   "t,q(s)",  MATCH_SH, MASK_SH, match_opcode,   RD_xs1|RD_xs2 },
{"sh",        "I",   "t,A,s",  0, (int) M_SH, match_never,  INSN_MACRO },
{"sw",        "I",   "t,q(s)",  MATCH_SW, MASK_SW, match_opcode,   RD_xs1|RD_xs2 },
{"sw",        "I",   "t,A,s",  0, (int) M_SW, match_never,  INSN_MACRO },
{"fence",     "I",   "",  MATCH_FENCE | MASK_PRED | MASK_SUCC, MASK_FENCE | MASK_RD | MASK_RS1 | MASK_IMM, match_opcode,   INSN_ALIAS },
{"fence",     "I",   "P,Q",  MATCH_FENCE, MASK_FENCE | MASK_RD | MASK_RS1 | (MASK_IMM & ~MASK_PRED & ~MASK_SUCC), match_opcode,   0 },
{"fence.i",   "I",   "",  MATCH_FENCE_I, MASK_FENCE | MASK_RD | MASK_RS1 | MASK_IMM, match_opcode,   0 },
{"rdcycle",   "I",   "d",  MATCH_RDCYCLE, MASK_RDCYCLE, match_opcode,  WR_xd },
{"rdinstret", "I",   "d",  MATCH_RDINSTRET, MASK_RDINSTRET, match_opcode,  WR_xd },
{"rdtime",    "I",   "d",  MATCH_RDTIME, MASK_RDTIME, match_opcode,  WR_xd },
{"rdcycleh",  "32I", "d",  MATCH_RDCYCLEH, MASK_RDCYCLEH, match_opcode,  WR_xd },
{"rdinstreth","32I", "d",  MATCH_RDINSTRETH, MASK_RDINSTRETH, match_opcode,  WR_xd },
{"rdtimeh",   "32I", "d",  MATCH_RDTIMEH, MASK_RDTIMEH, match_opcode,  WR_xd },
{"sbreak",    "I",   "",    MATCH_SBREAK, MASK_SBREAK, match_opcode,   0 },
{"scall",     "I",   "",    MATCH_SCALL, MASK_SCALL, match_opcode,   0 },
{"xori",      "I",   "d,s,j",  MATCH_XORI, MASK_XORI, match_opcode,   WR_xd|RD_xs1 },
{"xor",       "I",   "d,s,t",  MATCH_XOR, MASK_XOR, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"xor",       "I",   "d,s,j",  MATCH_XORI, MASK_XORI, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"lwu",       "64I", "d,o(s)",  MATCH_LWU, MASK_LWU, match_opcode,   WR_xd|RD_xs1 },
{"lwu",       "64I", "d,A",  0, (int) M_LWU, match_never, INSN_MACRO },
{"ld",        "64I", "d,o(s)", MATCH_LD, MASK_LD, match_opcode,  WR_xd|RD_xs1 },
{"ld",        "64I", "d,A",  0, (int) M_LD, match_never, INSN_MACRO },
{"sd",        "64I", "t,q(s)",  MATCH_SD, MASK_SD, match_opcode,   RD_xs1|RD_xs2 },
{"sd",        "64I", "t,A,s",  0, (int) M_SD, match_never,  INSN_MACRO },
{"sext.w",    "64I", "d,s",  MATCH_ADDIW, MASK_ADDIW | MASK_IMM, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"addiw",     "64I", "d,s,j",  MATCH_ADDIW, MASK_ADDIW, match_opcode,   WR_xd|RD_xs1 },
{"addw",      "64I", "d,s,t",  MATCH_ADDW, MASK_ADDW, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"addw",      "64I", "d,s,j",  MATCH_ADDIW, MASK_ADDIW, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"negw",      "64I", "d,t",  MATCH_SUBW, MASK_SUBW | MASK_RS1, match_opcode,   INSN_ALIAS|WR_xd|RD_xs2 }, /* sub 0 */
{"slliw",     "64I", "d,s,<",   MATCH_SLLIW, MASK_SLLIW, match_opcode,   WR_xd|RD_xs1 },
{"sllw",      "64I", "d,s,t",   MATCH_SLLW, MASK_SLLW, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"sllw",      "64I", "d,s,<",   MATCH_SLLIW, MASK_SLLIW, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"srliw",     "64I", "d,s,<",   MATCH_SRLIW, MASK_SRLIW, match_opcode,   WR_xd|RD_xs1 },
{"srlw",      "64I", "d,s,t",   MATCH_SRLW, MASK_SRLW, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"srlw",      "64I", "d,s,<",   MATCH_SRLIW, MASK_SRLIW, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"sraiw",     "64I", "d,s,<",   MATCH_SRAIW, MASK_SRAIW, match_opcode,   WR_xd|RD_xs1 },
{"sraw",      "64I", "d,s,t",   MATCH_SRAW, MASK_SRAW, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"sraw",      "64I", "d,s,<",   MATCH_SRAIW, MASK_SRAIW, match_opcode,   INSN_ALIAS|WR_xd|RD_xs1 },
{"subw",      "64I", "d,s,t",  MATCH_SUBW, MASK_SUBW, match_opcode,   WR_xd|RD_xs1|RD_xs2 },

/* Atomic memory operation instruction subset */
{"lr.w",         "A",   "d,0(s)",    MATCH_LR_W, MASK_LR_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1 },
{"sc.w",         "A",   "d,t,0(s)",  MATCH_SC_W, MASK_SC_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoadd.w",     "A",   "d,t,0(s)",  MATCH_AMOADD_W, MASK_AMOADD_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoswap.w",    "A",   "d,t,0(s)",  MATCH_AMOSWAP_W, MASK_AMOSWAP_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoand.w",     "A",   "d,t,0(s)",  MATCH_AMOAND_W, MASK_AMOAND_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoor.w",      "A",   "d,t,0(s)",  MATCH_AMOOR_W, MASK_AMOOR_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoxor.w",     "A",   "d,t,0(s)",  MATCH_AMOXOR_W, MASK_AMOXOR_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomax.w",     "A",   "d,t,0(s)",  MATCH_AMOMAX_W, MASK_AMOMAX_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomaxu.w",    "A",   "d,t,0(s)",  MATCH_AMOMAXU_W, MASK_AMOMAXU_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomin.w",     "A",   "d,t,0(s)",  MATCH_AMOMIN_W, MASK_AMOMIN_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amominu.w",    "A",   "d,t,0(s)",  MATCH_AMOMINU_W, MASK_AMOMINU_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"lr.w.aq",      "A",   "d,0(s)",    MATCH_LR_W | MASK_AQ, MASK_LR_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1 },
{"sc.w.aq",      "A",   "d,t,0(s)",  MATCH_SC_W | MASK_AQ, MASK_SC_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoadd.w.aq",  "A",   "d,t,0(s)",  MATCH_AMOADD_W | MASK_AQ, MASK_AMOADD_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoswap.w.aq", "A",   "d,t,0(s)",  MATCH_AMOSWAP_W | MASK_AQ, MASK_AMOSWAP_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoand.w.aq",  "A",   "d,t,0(s)",  MATCH_AMOAND_W | MASK_AQ, MASK_AMOAND_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoor.w.aq",   "A",   "d,t,0(s)",  MATCH_AMOOR_W | MASK_AQ, MASK_AMOOR_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoxor.w.aq",  "A",   "d,t,0(s)",  MATCH_AMOXOR_W | MASK_AQ, MASK_AMOXOR_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomax.w.aq",  "A",   "d,t,0(s)",  MATCH_AMOMAX_W | MASK_AQ, MASK_AMOMAX_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomaxu.w.aq", "A",   "d,t,0(s)",  MATCH_AMOMAXU_W | MASK_AQ, MASK_AMOMAXU_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomin.w.aq",  "A",   "d,t,0(s)",  MATCH_AMOMIN_W | MASK_AQ, MASK_AMOMIN_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amominu.w.aq", "A",   "d,t,0(s)",  MATCH_AMOMINU_W | MASK_AQ, MASK_AMOMINU_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"lr.w.rl",      "A",   "d,0(s)",    MATCH_LR_W | MASK_RL, MASK_LR_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1 },
{"sc.w.rl",      "A",   "d,t,0(s)",  MATCH_SC_W | MASK_RL, MASK_SC_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoadd.w.rl",  "A",   "d,t,0(s)",  MATCH_AMOADD_W | MASK_RL, MASK_AMOADD_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoswap.w.rl", "A",   "d,t,0(s)",  MATCH_AMOSWAP_W | MASK_RL, MASK_AMOSWAP_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoand.w.rl",  "A",   "d,t,0(s)",  MATCH_AMOAND_W | MASK_RL, MASK_AMOAND_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoor.w.rl",   "A",   "d,t,0(s)",  MATCH_AMOOR_W | MASK_RL, MASK_AMOOR_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoxor.w.rl",  "A",   "d,t,0(s)",  MATCH_AMOXOR_W | MASK_RL, MASK_AMOXOR_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomax.w.rl",  "A",   "d,t,0(s)",  MATCH_AMOMAX_W | MASK_RL, MASK_AMOMAX_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomaxu.w.rl", "A",   "d,t,0(s)",  MATCH_AMOMAXU_W | MASK_RL, MASK_AMOMAXU_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomin.w.rl",  "A",   "d,t,0(s)",  MATCH_AMOMIN_W | MASK_RL, MASK_AMOMIN_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amominu.w.rl", "A",   "d,t,0(s)",  MATCH_AMOMINU_W | MASK_RL, MASK_AMOMINU_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"lr.w.sc",      "A",   "d,0(s)",    MATCH_LR_W | MASK_AQRL, MASK_LR_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1 },
{"sc.w.sc",      "A",   "d,t,0(s)",  MATCH_SC_W | MASK_AQRL, MASK_SC_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoadd.w.sc",  "A",   "d,t,0(s)",  MATCH_AMOADD_W | MASK_AQRL, MASK_AMOADD_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoswap.w.sc", "A",   "d,t,0(s)",  MATCH_AMOSWAP_W | MASK_AQRL, MASK_AMOSWAP_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoand.w.sc",  "A",   "d,t,0(s)",  MATCH_AMOAND_W | MASK_AQRL, MASK_AMOAND_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoor.w.sc",   "A",   "d,t,0(s)",  MATCH_AMOOR_W | MASK_AQRL, MASK_AMOOR_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoxor.w.sc",  "A",   "d,t,0(s)",  MATCH_AMOXOR_W | MASK_AQRL, MASK_AMOXOR_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomax.w.sc",  "A",   "d,t,0(s)",  MATCH_AMOMAX_W | MASK_AQRL, MASK_AMOMAX_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomaxu.w.sc", "A",   "d,t,0(s)",  MATCH_AMOMAXU_W | MASK_AQRL, MASK_AMOMAXU_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomin.w.sc",  "A",   "d,t,0(s)",  MATCH_AMOMIN_W | MASK_AQRL, MASK_AMOMIN_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amominu.w.sc", "A",   "d,t,0(s)",  MATCH_AMOMINU_W | MASK_AQRL, MASK_AMOMINU_W | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"lr.d",         "64A", "d,0(s)",    MATCH_LR_D, MASK_LR_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1 },
{"sc.d",         "64A", "d,t,0(s)",  MATCH_SC_D, MASK_SC_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoadd.d",     "64A", "d,t,0(s)",  MATCH_AMOADD_D, MASK_AMOADD_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoswap.d",    "64A", "d,t,0(s)",  MATCH_AMOSWAP_D, MASK_AMOSWAP_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoand.d",     "64A", "d,t,0(s)",  MATCH_AMOAND_D, MASK_AMOAND_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoor.d",      "64A", "d,t,0(s)",  MATCH_AMOOR_D, MASK_AMOOR_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoxor.d",     "64A", "d,t,0(s)",  MATCH_AMOXOR_D, MASK_AMOXOR_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomax.d",     "64A", "d,t,0(s)",  MATCH_AMOMAX_D, MASK_AMOMAX_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomaxu.d",    "64A", "d,t,0(s)",  MATCH_AMOMAXU_D, MASK_AMOMAXU_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomin.d",     "64A", "d,t,0(s)",  MATCH_AMOMIN_D, MASK_AMOMIN_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amominu.d",    "64A", "d,t,0(s)",  MATCH_AMOMINU_D, MASK_AMOMINU_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"lr.d.aq",      "64A", "d,0(s)",    MATCH_LR_D | MASK_AQ, MASK_LR_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1 },
{"sc.d.aq",      "64A", "d,t,0(s)",  MATCH_SC_D | MASK_AQ, MASK_SC_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoadd.d.aq",  "64A", "d,t,0(s)",  MATCH_AMOADD_D | MASK_AQ, MASK_AMOADD_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoswap.d.aq", "64A", "d,t,0(s)",  MATCH_AMOSWAP_D | MASK_AQ, MASK_AMOSWAP_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoand.d.aq",  "64A", "d,t,0(s)",  MATCH_AMOAND_D | MASK_AQ, MASK_AMOAND_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoor.d.aq",   "64A", "d,t,0(s)",  MATCH_AMOOR_D | MASK_AQ, MASK_AMOOR_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoxor.d.aq",  "64A", "d,t,0(s)",  MATCH_AMOXOR_D | MASK_AQ, MASK_AMOXOR_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomax.d.aq",  "64A", "d,t,0(s)",  MATCH_AMOMAX_D | MASK_AQ, MASK_AMOMAX_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomaxu.d.aq", "64A", "d,t,0(s)",  MATCH_AMOMAXU_D | MASK_AQ, MASK_AMOMAXU_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomin.d.aq",  "64A", "d,t,0(s)",  MATCH_AMOMIN_D | MASK_AQ, MASK_AMOMIN_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amominu.d.aq", "64A", "d,t,0(s)",  MATCH_AMOMINU_D | MASK_AQ, MASK_AMOMINU_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"lr.d.rl",      "64A", "d,0(s)",    MATCH_LR_D | MASK_RL, MASK_LR_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1 },
{"sc.d.rl",      "64A", "d,t,0(s)",  MATCH_SC_D | MASK_RL, MASK_SC_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoadd.d.rl",  "64A", "d,t,0(s)",  MATCH_AMOADD_D | MASK_RL, MASK_AMOADD_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoswap.d.rl", "64A", "d,t,0(s)",  MATCH_AMOSWAP_D | MASK_RL, MASK_AMOSWAP_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoand.d.rl",  "64A", "d,t,0(s)",  MATCH_AMOAND_D | MASK_RL, MASK_AMOAND_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoor.d.rl",   "64A", "d,t,0(s)",  MATCH_AMOOR_D | MASK_RL, MASK_AMOOR_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoxor.d.rl",  "64A", "d,t,0(s)",  MATCH_AMOXOR_D | MASK_RL, MASK_AMOXOR_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomax.d.rl",  "64A", "d,t,0(s)",  MATCH_AMOMAX_D | MASK_RL, MASK_AMOMAX_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomaxu.d.rl", "64A", "d,t,0(s)",  MATCH_AMOMAXU_D | MASK_RL, MASK_AMOMAXU_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomin.d.rl",  "64A", "d,t,0(s)",  MATCH_AMOMIN_D | MASK_RL, MASK_AMOMIN_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amominu.d.rl", "64A", "d,t,0(s)",  MATCH_AMOMINU_D | MASK_RL, MASK_AMOMINU_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"lr.d.sc",      "64A", "d,0(s)",    MATCH_LR_D | MASK_AQRL, MASK_LR_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1 },
{"sc.d.sc",      "64A", "d,t,0(s)",  MATCH_SC_D | MASK_AQRL, MASK_SC_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoadd.d.sc",  "64A", "d,t,0(s)",  MATCH_AMOADD_D | MASK_AQRL, MASK_AMOADD_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoswap.d.sc", "64A", "d,t,0(s)",  MATCH_AMOSWAP_D | MASK_AQRL, MASK_AMOSWAP_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoand.d.sc",  "64A", "d,t,0(s)",  MATCH_AMOAND_D | MASK_AQRL, MASK_AMOAND_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoor.d.sc",   "64A", "d,t,0(s)",  MATCH_AMOOR_D | MASK_AQRL, MASK_AMOOR_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amoxor.d.sc",  "64A", "d,t,0(s)",  MATCH_AMOXOR_D | MASK_AQRL, MASK_AMOXOR_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomax.d.sc",  "64A", "d,t,0(s)",  MATCH_AMOMAX_D | MASK_AQRL, MASK_AMOMAX_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomaxu.d.sc", "64A", "d,t,0(s)",  MATCH_AMOMAXU_D | MASK_AQRL, MASK_AMOMAXU_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amomin.d.sc",  "64A", "d,t,0(s)",  MATCH_AMOMIN_D | MASK_AQRL, MASK_AMOMIN_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },
{"amominu.d.sc", "64A", "d,t,0(s)",  MATCH_AMOMINU_D | MASK_AQRL, MASK_AMOMINU_D | MASK_AQRL, match_opcode,   WR_xd|RD_xs1|RD_xs2 },

/* Multiply/Divide instruction subset */
{"mul",       "M",   "d,s,t",  MATCH_MUL, MASK_MUL, match_opcode,  WR_xd|RD_xs1|RD_xs2 },
{"mulh",      "M",   "d,s,t",  MATCH_MULH, MASK_MULH, match_opcode,  WR_xd|RD_xs1|RD_xs2 },
{"mulhu",     "M",   "d,s,t",  MATCH_MULHU, MASK_MULHU, match_opcode,  WR_xd|RD_xs1|RD_xs2 },
{"mulhsu",    "M",   "d,s,t",  MATCH_MULHSU, MASK_MULHSU, match_opcode,  WR_xd|RD_xs1|RD_xs2 },
{"div",       "M",   "d,s,t",  MATCH_DIV, MASK_DIV, match_opcode,  WR_xd|RD_xs1|RD_xs2 },
{"divu",      "M",   "d,s,t",  MATCH_DIVU, MASK_DIVU, match_opcode,  WR_xd|RD_xs1|RD_xs2 },
{"rem",       "M",   "d,s,t",  MATCH_REM, MASK_REM, match_opcode,  WR_xd|RD_xs1|RD_xs2 },
{"remu",      "M",   "d,s,t",  MATCH_REMU, MASK_REMU, match_opcode,  WR_xd|RD_xs1|RD_xs2 },
{"mulw",      "64M", "d,s,t",  MATCH_MULW, MASK_MULW, match_opcode,  WR_xd|RD_xs1|RD_xs2 },
{"divw",      "64M", "d,s,t",  MATCH_DIVW, MASK_DIVW, match_opcode,  WR_xd|RD_xs1|RD_xs2 },
{"divuw",     "64M", "d,s,t",  MATCH_DIVUW, MASK_DIVUW, match_opcode,  WR_xd|RD_xs1|RD_xs2 },
{"remw",      "64M", "d,s,t",  MATCH_REMW, MASK_REMW, match_opcode,  WR_xd|RD_xs1|RD_xs2 },
{"remuw",     "64M", "d,s,t",  MATCH_REMUW, MASK_REMUW, match_opcode,  WR_xd|RD_xs1|RD_xs2 },

/* Single-precision floating-point instruction subset */
{"frsr",      "F",   "d",  MATCH_FRCSR, MASK_FRCSR, match_opcode,  WR_xd },
{"fssr",      "F",   "s",  MATCH_FSCSR, MASK_FSCSR | MASK_RD, match_opcode,  RD_xs1 },
{"fssr",      "F",   "d,s",  MATCH_FSCSR, MASK_FSCSR, match_opcode,  WR_xd|RD_xs1 },
{"frcsr",     "F",   "d",  MATCH_FRCSR, MASK_FRCSR, match_opcode,  WR_xd },
{"fscsr",     "F",   "s",  MATCH_FSCSR, MASK_FSCSR | MASK_RD, match_opcode,  RD_xs1 },
{"fscsr",     "F",   "d,s",  MATCH_FSCSR, MASK_FSCSR, match_opcode,  WR_xd|RD_xs1 },
{"frrm",      "F",   "d",  MATCH_FRRM, MASK_FRRM, match_opcode,  WR_xd },
{"fsrm",      "F",   "s",  MATCH_FSRM, MASK_FSRM | MASK_RD, match_opcode,  RD_xs1 },
{"fsrm",      "F",   "d,s",  MATCH_FSRM, MASK_FSRM, match_opcode,  WR_xd|RD_xs1 },
{"frflags",   "F",   "d",  MATCH_FRFLAGS, MASK_FRFLAGS, match_opcode,  WR_xd },
{"fsflags",   "F",   "s",  MATCH_FSFLAGS, MASK_FSFLAGS | MASK_RD, match_opcode,  RD_xs1 },
{"fsflags",   "F",   "d,s",  MATCH_FSFLAGS, MASK_FSFLAGS, match_opcode,  WR_xd|RD_xs1 },
{"flw",       "F",   "D,o(s)",  MATCH_FLW, MASK_FLW, match_opcode,   WR_fd|RD_xs1 },
{"flw",       "F",   "D,A,s",  0, (int) M_FLW, match_never,  INSN_MACRO },
{"fsw",       "F",   "T,q(s)",  MATCH_FSW, MASK_FSW, match_opcode,   RD_xs1|RD_fs2 },
{"fsw",       "F",   "T,A,s",  0, (int) M_FSW, match_never,  INSN_MACRO },
{"fmv.x.s",   "F",   "d,S",  MATCH_FMV_X_S, MASK_FMV_X_S, match_opcode,  WR_xd|RD_fs1 },
{"fmv.s.x",   "F",   "D,s",  MATCH_FMV_S_X, MASK_FMV_S_X, match_opcode,  WR_fd|RD_xs1 },
{"fmv.s",     "F",   "D,U",  MATCH_FSGNJ_S, MASK_FSGNJ_S, match_rs1_eq_rs2,   INSN_ALIAS|WR_fd|RD_fs1|RD_fs2 },
{"fneg.s",    "F",   "D,U",  MATCH_FSGNJN_S, MASK_FSGNJN_S, match_rs1_eq_rs2,   INSN_ALIAS|WR_fd|RD_fs1|RD_fs2 },
{"fabs.s",    "F",   "D,U",  MATCH_FSGNJX_S, MASK_FSGNJX_S, match_rs1_eq_rs2,   INSN_ALIAS|WR_fd|RD_fs1|RD_fs2 },
{"fsgnj.s",   "F",   "D,S,T",  MATCH_FSGNJ_S, MASK_FSGNJ_S, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fsgnjn.s",  "F",   "D,S,T",  MATCH_FSGNJN_S, MASK_FSGNJN_S, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fsgnjx.s",  "F",   "D,S,T",  MATCH_FSGNJX_S, MASK_FSGNJX_S, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fadd.s",    "F",   "D,S,T",  MATCH_FADD_S | MASK_RM, MASK_FADD_S | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fadd.s",    "F",   "D,S,T,m",  MATCH_FADD_S, MASK_FADD_S, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fsub.s",    "F",   "D,S,T",  MATCH_FSUB_S | MASK_RM, MASK_FSUB_S | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fsub.s",    "F",   "D,S,T,m",  MATCH_FSUB_S, MASK_FSUB_S, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fmul.s",    "F",   "D,S,T",  MATCH_FMUL_S | MASK_RM, MASK_FMUL_S | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fmul.s",    "F",   "D,S,T,m",  MATCH_FMUL_S, MASK_FMUL_S, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fdiv.s",    "F",   "D,S,T",  MATCH_FDIV_S | MASK_RM, MASK_FDIV_S | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fdiv.s",    "F",   "D,S,T,m",  MATCH_FDIV_S, MASK_FDIV_S, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fsqrt.s",   "F",   "D,S",  MATCH_FSQRT_S | MASK_RM, MASK_FSQRT_S | MASK_RM, match_opcode,  WR_fd|RD_fs1 },
{"fsqrt.s",   "F",   "D,S,m",  MATCH_FSQRT_S, MASK_FSQRT_S, match_opcode,  WR_fd|RD_fs1 },
{"fmin.s",    "F",   "D,S,T",  MATCH_FMIN_S, MASK_FMIN_S, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fmax.s",    "F",   "D,S,T",  MATCH_FMAX_S, MASK_FMAX_S, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fmadd.s",   "F",   "D,S,T,R",  MATCH_FMADD_S | MASK_RM, MASK_FMADD_S | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fmadd.s",   "F",   "D,S,T,R,m",  MATCH_FMADD_S, MASK_FMADD_S, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fnmadd.s",  "F",   "D,S,T,R",  MATCH_FNMADD_S | MASK_RM, MASK_FNMADD_S | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fnmadd.s",  "F",   "D,S,T,R,m",  MATCH_FNMADD_S, MASK_FNMADD_S, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fmsub.s",   "F",   "D,S,T,R",  MATCH_FMSUB_S | MASK_RM, MASK_FMSUB_S | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fmsub.s",   "F",   "D,S,T,R,m",  MATCH_FMSUB_S, MASK_FMSUB_S, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fnmsub.s",  "F",   "D,S,T,R",  MATCH_FNMSUB_S | MASK_RM, MASK_FNMSUB_S | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fnmsub.s",  "F",   "D,S,T,R,m",  MATCH_FNMSUB_S, MASK_FNMSUB_S, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fcvt.w.s",  "F",   "d,S",  MATCH_FCVT_W_S | MASK_RM, MASK_FCVT_W_S | MASK_RM, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.w.s",  "F",   "d,S,m",  MATCH_FCVT_W_S, MASK_FCVT_W_S, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.wu.s", "F",   "d,S",  MATCH_FCVT_WU_S | MASK_RM, MASK_FCVT_WU_S | MASK_RM, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.wu.s", "F",   "d,S,m",  MATCH_FCVT_WU_S, MASK_FCVT_WU_S, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.s.w",  "F",   "D,s",  MATCH_FCVT_S_W | MASK_RM, MASK_FCVT_S_W | MASK_RM, match_opcode,   WR_fd|RD_xs1 },
{"fcvt.s.w",  "F",   "D,s,m",  MATCH_FCVT_S_W, MASK_FCVT_S_W, match_opcode,   WR_fd|RD_xs1 },
{"fcvt.s.wu", "F",   "D,s",  MATCH_FCVT_S_WU | MASK_RM, MASK_FCVT_S_W | MASK_RM, match_opcode,   WR_fd|RD_xs1 },
{"fcvt.s.wu", "F",   "D,s,m",  MATCH_FCVT_S_WU, MASK_FCVT_S_WU, match_opcode,   WR_fd|RD_xs1 },
{"fclass.s",  "F",   "d,S",  MATCH_FCLASS_S, MASK_FCLASS_S, match_opcode,   WR_xd|RD_fs1 },
{"feq.s",     "F",   "d,S,T",    MATCH_FEQ_S, MASK_FEQ_S, match_opcode,  WR_xd|RD_fs1|RD_fs2 },
{"flt.s",     "F",   "d,S,T",    MATCH_FLT_S, MASK_FLT_S, match_opcode,  WR_xd|RD_fs1|RD_fs2 },
{"fle.s",     "F",   "d,S,T",    MATCH_FLE_S, MASK_FLE_S, match_opcode,  WR_xd|RD_fs1|RD_fs2 },
{"fgt.s",     "F",   "d,T,S",    MATCH_FLT_S, MASK_FLT_S, match_opcode,  WR_xd|RD_fs1|RD_fs2 },
{"fge.s",     "F",   "d,T,S",    MATCH_FLE_S, MASK_FLE_S, match_opcode,  WR_xd|RD_fs1|RD_fs2 },
{"fcvt.l.s",  "64F", "d,S",  MATCH_FCVT_L_S | MASK_RM, MASK_FCVT_L_S | MASK_RM, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.l.s",  "64F", "d,S,m",  MATCH_FCVT_L_S, MASK_FCVT_L_S, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.lu.s", "64F", "d,S",  MATCH_FCVT_LU_S | MASK_RM, MASK_FCVT_LU_S | MASK_RM, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.lu.s", "64F", "d,S,m",  MATCH_FCVT_LU_S, MASK_FCVT_LU_S, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.s.l",  "64F", "D,s",  MATCH_FCVT_S_L | MASK_RM, MASK_FCVT_S_L | MASK_RM, match_opcode,   WR_fd|RD_xs1 },
{"fcvt.s.l",  "64F", "D,s,m",  MATCH_FCVT_S_L, MASK_FCVT_S_L, match_opcode,   WR_fd|RD_xs1 },
{"fcvt.s.lu", "64F", "D,s",  MATCH_FCVT_S_LU | MASK_RM, MASK_FCVT_S_L | MASK_RM, match_opcode,   WR_fd|RD_xs1 },
{"fcvt.s.lu", "64F", "D,s,m",  MATCH_FCVT_S_LU, MASK_FCVT_S_LU, match_opcode,   WR_fd|RD_xs1 },

/* Double-precision floating-point instruction subset */
{"fld",       "D",   "D,o(s)",  MATCH_FLD, MASK_FLD, match_opcode,  WR_fd|RD_xs1 },
{"fld",       "D",   "D,A,s",  0, (int) M_FLD, match_never,  INSN_MACRO },
{"fsd",       "D",   "T,q(s)",  MATCH_FSD, MASK_FSD, match_opcode,  RD_xs1|RD_fs2 },
{"fsd",       "D",   "T,A,s",  0, (int) M_FSD, match_never,  INSN_MACRO },
{"fmv.d",     "D",   "D,U",  MATCH_FSGNJ_D, MASK_FSGNJ_D, match_rs1_eq_rs2,   INSN_ALIAS|WR_fd|RD_fs1|RD_fs2 },
{"fneg.d",    "D",   "D,U",  MATCH_FSGNJN_D, MASK_FSGNJN_D, match_rs1_eq_rs2,   INSN_ALIAS|WR_fd|RD_fs1|RD_fs2 },
{"fabs.d",    "D",   "D,U",  MATCH_FSGNJX_D, MASK_FSGNJX_D, match_rs1_eq_rs2,   INSN_ALIAS|WR_fd|RD_fs1|RD_fs2 },
{"fsgnj.d",   "D",   "D,S,T",  MATCH_FSGNJ_D, MASK_FSGNJ_D, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fsgnjn.d",  "D",   "D,S,T",  MATCH_FSGNJN_D, MASK_FSGNJN_D, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fsgnjx.d",  "D",   "D,S,T",  MATCH_FSGNJX_D, MASK_FSGNJX_D, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fadd.d",    "D",   "D,S,T",  MATCH_FADD_D | MASK_RM, MASK_FADD_D | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fadd.d",    "D",   "D,S,T,m",  MATCH_FADD_D, MASK_FADD_D, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fsub.d",    "D",   "D,S,T",  MATCH_FSUB_D | MASK_RM, MASK_FSUB_D | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fsub.d",    "D",   "D,S,T,m",  MATCH_FSUB_D, MASK_FSUB_D, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fmul.d",    "D",   "D,S,T",  MATCH_FMUL_D | MASK_RM, MASK_FMUL_D | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fmul.d",    "D",   "D,S,T,m",  MATCH_FMUL_D, MASK_FMUL_D, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fdiv.d",    "D",   "D,S,T",  MATCH_FDIV_D | MASK_RM, MASK_FDIV_D | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fdiv.d",    "D",   "D,S,T,m",  MATCH_FDIV_D, MASK_FDIV_D, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fsqrt.d",   "D",   "D,S",  MATCH_FSQRT_D | MASK_RM, MASK_FSQRT_D | MASK_RM, match_opcode,  WR_fd|RD_fs1 },
{"fsqrt.d",   "D",   "D,S,m",  MATCH_FSQRT_D, MASK_FSQRT_D, match_opcode,  WR_fd|RD_fs1 },
{"fmin.d",    "D",   "D,S,T",  MATCH_FMIN_D, MASK_FMIN_D, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fmax.d",    "D",   "D,S,T",  MATCH_FMAX_D, MASK_FMAX_D, match_opcode,   WR_fd|RD_fs1|RD_fs2 },
{"fmadd.d",   "D",   "D,S,T,R",  MATCH_FMADD_D | MASK_RM, MASK_FMADD_D | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fmadd.d",   "D",   "D,S,T,R,m",  MATCH_FMADD_D, MASK_FMADD_D, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fnmadd.d",  "D",   "D,S,T,R",  MATCH_FNMADD_D | MASK_RM, MASK_FNMADD_D | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fnmadd.d",  "D",   "D,S,T,R,m",  MATCH_FNMADD_D, MASK_FNMADD_D, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fmsub.d",   "D",   "D,S,T,R",  MATCH_FMSUB_D | MASK_RM, MASK_FMSUB_D | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fmsub.d",   "D",   "D,S,T,R,m",  MATCH_FMSUB_D, MASK_FMSUB_D, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fnmsub.d",  "D",   "D,S,T,R",  MATCH_FNMSUB_D | MASK_RM, MASK_FNMSUB_D | MASK_RM, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fnmsub.d",  "D",   "D,S,T,R,m",  MATCH_FNMSUB_D, MASK_FNMSUB_D, match_opcode,   WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fcvt.w.d",  "D",   "d,S",  MATCH_FCVT_W_D | MASK_RM, MASK_FCVT_W_D | MASK_RM, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.w.d",  "D",   "d,S,m",  MATCH_FCVT_W_D, MASK_FCVT_W_D, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.wu.d", "D",   "d,S",  MATCH_FCVT_WU_D | MASK_RM, MASK_FCVT_WU_D | MASK_RM, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.wu.d", "D",   "d,S,m",  MATCH_FCVT_WU_D, MASK_FCVT_WU_D, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.d.w",  "D",   "D,s",  MATCH_FCVT_D_W, MASK_FCVT_D_W | MASK_RM, match_opcode,   WR_fd|RD_xs1 },
{"fcvt.d.wu", "D",   "D,s",  MATCH_FCVT_D_WU, MASK_FCVT_D_WU | MASK_RM, match_opcode,   WR_fd|RD_xs1 },
{"fcvt.d.s",  "D",   "D,S",  MATCH_FCVT_D_S, MASK_FCVT_D_S | MASK_RM, match_opcode,   WR_fd|RD_fs1 },
{"fcvt.s.d",  "D",   "D,S",  MATCH_FCVT_S_D | MASK_RM, MASK_FCVT_S_D | MASK_RM, match_opcode,   WR_fd|RD_fs1 },
{"fcvt.s.d",  "D",   "D,S,m",  MATCH_FCVT_S_D, MASK_FCVT_S_D, match_opcode,   WR_fd|RD_fs1 },
{"fclass.d",  "D",   "d,S",  MATCH_FCLASS_D, MASK_FCLASS_D, match_opcode,   WR_xd|RD_fs1 },
{"feq.d",     "D",   "d,S,T",    MATCH_FEQ_D, MASK_FEQ_D, match_opcode,  WR_xd|RD_fs1|RD_fs2 },
{"flt.d",     "D",   "d,S,T",    MATCH_FLT_D, MASK_FLT_D, match_opcode,  WR_xd|RD_fs1|RD_fs2 },
{"fle.d",     "D",   "d,S,T",    MATCH_FLE_D, MASK_FLE_D, match_opcode,  WR_xd|RD_fs1|RD_fs2 },
{"fgt.d",     "D",   "d,T,S",    MATCH_FLT_D, MASK_FLT_D, match_opcode,  WR_xd|RD_fs1|RD_fs2 },
{"fge.d",     "D",   "d,T,S",    MATCH_FLE_D, MASK_FLE_D, match_opcode,  WR_xd|RD_fs1|RD_fs2 },
{"fmv.x.d",   "64D", "d,S",  MATCH_FMV_X_D, MASK_FMV_X_D, match_opcode,  WR_xd|RD_fs1 },
{"fmv.d.x",   "64D", "D,s",  MATCH_FMV_D_X, MASK_FMV_D_X, match_opcode,  WR_fd|RD_xs1 },
{"fcvt.l.d",  "64D", "d,S",  MATCH_FCVT_L_D | MASK_RM, MASK_FCVT_L_D | MASK_RM, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.l.d",  "64D", "d,S,m",  MATCH_FCVT_L_D, MASK_FCVT_L_D, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.lu.d", "64D", "d,S",  MATCH_FCVT_LU_D | MASK_RM, MASK_FCVT_LU_D | MASK_RM, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.lu.d", "64D", "d,S,m",  MATCH_FCVT_LU_D, MASK_FCVT_LU_D, match_opcode,  WR_xd|RD_fs1 },
{"fcvt.d.l",  "64D", "D,s",  MATCH_FCVT_D_L | MASK_RM, MASK_FCVT_D_L | MASK_RM, match_opcode,   WR_fd|RD_xs1 },
{"fcvt.d.l",  "64D", "D,s,m",  MATCH_FCVT_D_L, MASK_FCVT_D_L, match_opcode,   WR_fd|RD_xs1 },
{"fcvt.d.lu", "64D", "D,s",  MATCH_FCVT_D_LU | MASK_RM, MASK_FCVT_D_L | MASK_RM, match_opcode,   WR_fd|RD_xs1 },
{"fcvt.d.lu", "64D", "D,s,m",  MATCH_FCVT_D_LU, MASK_FCVT_D_LU, match_opcode,   WR_fd|RD_xs1 },

/* Supervisor instructions */
{"csrr",      "I",   "d,E",  MATCH_CSRRS, MASK_CSRRS | MASK_RS1, match_opcode,  WR_xd },
{"csrwi",     "I",   "E,Z",  MATCH_CSRRWI, MASK_CSRRWI | MASK_RD, match_opcode,  WR_xd|RD_xs1 },
{"csrw",      "I",   "E,s",  MATCH_CSRRW, MASK_CSRRW | MASK_RD, match_opcode,  RD_xs1 },
{"csrw",      "I",   "E,Z",  MATCH_CSRRWI, MASK_CSRRWI | MASK_RD, match_opcode,  WR_xd|RD_xs1 },
{"csrsi",     "I",   "E,Z",  MATCH_CSRRSI, MASK_CSRRSI | MASK_RD, match_opcode,  WR_xd|RD_xs1 },
{"csrs",      "I",   "E,s",  MATCH_CSRRS, MASK_CSRRS | MASK_RD, match_opcode,  WR_xd|RD_xs1 },
{"csrs",      "I",   "E,Z",  MATCH_CSRRSI, MASK_CSRRSI | MASK_RD, match_opcode,  WR_xd|RD_xs1 },
{"csrci",     "I",   "E,Z",  MATCH_CSRRCI, MASK_CSRRCI | MASK_RD, match_opcode,  WR_xd|RD_xs1 },
{"csrc",      "I",   "E,s",  MATCH_CSRRC, MASK_CSRRC | MASK_RD, match_opcode,  WR_xd|RD_xs1 },
{"csrc",      "I",   "E,Z",  MATCH_CSRRCI, MASK_CSRRCI | MASK_RD, match_opcode,  WR_xd|RD_xs1 },
{"csrrw",     "I",   "d,E,s",  MATCH_CSRRW, MASK_CSRRW, match_opcode,  WR_xd|RD_xs1 },
{"csrrw",     "I",   "d,E,Z",  MATCH_CSRRWI, MASK_CSRRWI, match_opcode,  WR_xd|RD_xs1 },
{"csrrs",     "I",   "d,E,s",  MATCH_CSRRS, MASK_CSRRS, match_opcode,  WR_xd|RD_xs1 },
{"csrrs",     "I",   "d,E,Z",  MATCH_CSRRSI, MASK_CSRRSI, match_opcode,  WR_xd|RD_xs1 },
{"csrrc",     "I",   "d,E,s",  MATCH_CSRRC, MASK_CSRRC, match_opcode,  WR_xd|RD_xs1 },
{"csrrc",     "I",   "d,E,Z",  MATCH_CSRRCI, MASK_CSRRCI, match_opcode,  WR_xd|RD_xs1 },
{"csrrwi",    "I",   "d,E,Z",  MATCH_CSRRWI, MASK_CSRRWI, match_opcode,  WR_xd|RD_xs1 },
{"csrrsi",    "I",   "d,E,Z",  MATCH_CSRRSI, MASK_CSRRSI, match_opcode,  WR_xd|RD_xs1 },
{"csrrci",    "I",   "d,E,Z",  MATCH_CSRRCI, MASK_CSRRCI, match_opcode,  WR_xd|RD_xs1 },
{"sret",      "I",   "",     MATCH_SRET, MASK_SRET, match_opcode,  0 },

/* Half-precision floating-point instruction subset */
{"flh",       "Xhwacha",   "D,o(s)",  MATCH_FLH, MASK_FLH, match_opcode, WR_fd|RD_xs1 },
{"fsh",       "Xhwacha",   "T,q(s)",  MATCH_FSH, MASK_FSH, match_opcode, RD_xs1|RD_fs2 },
{"fsgnj.h",   "Xhwacha",   "D,S,T",  MATCH_FSGNJ_H, MASK_FSGNJ_H, match_opcode,  WR_fd|RD_fs1|RD_fs2 },
{"fsgnjn.h",  "Xhwacha",   "D,S,T",  MATCH_FSGNJN_H, MASK_FSGNJN_H, match_opcode,  WR_fd|RD_fs1|RD_fs2 },
{"fsgnjx.h",  "Xhwacha",   "D,S,T",  MATCH_FSGNJX_H, MASK_FSGNJX_H, match_opcode,  WR_fd|RD_fs1|RD_fs2 },
{"fadd.h",    "Xhwacha",   "D,S,T",  MATCH_FADD_H | MASK_RM, MASK_FADD_H | MASK_RM, match_opcode,  WR_fd|RD_fs1|RD_fs2 },
{"fadd.h",    "Xhwacha",   "D,S,T,m",  MATCH_FADD_H, MASK_FADD_H, match_opcode,  WR_fd|RD_fs1|RD_fs2 },
{"fsub.h",    "Xhwacha",   "D,S,T",  MATCH_FSUB_H | MASK_RM, MASK_FSUB_H | MASK_RM, match_opcode,  WR_fd|RD_fs1|RD_fs2 },
{"fsub.h",    "Xhwacha",   "D,S,T,m",  MATCH_FSUB_H, MASK_FSUB_H, match_opcode,  WR_fd|RD_fs1|RD_fs2 },
{"fmul.h",    "Xhwacha",   "D,S,T",  MATCH_FMUL_H | MASK_RM, MASK_FMUL_H | MASK_RM, match_opcode,  WR_fd|RD_fs1|RD_fs2 },
{"fmul.h",    "Xhwacha",   "D,S,T,m",  MATCH_FMUL_H, MASK_FMUL_H, match_opcode,  WR_fd|RD_fs1|RD_fs2 },
{"fdiv.h",    "Xhwacha",   "D,S,T",  MATCH_FDIV_H | MASK_RM, MASK_FDIV_H | MASK_RM, match_opcode,  WR_fd|RD_fs1|RD_fs2 },
{"fdiv.h",    "Xhwacha",   "D,S,T,m",  MATCH_FDIV_H, MASK_FDIV_H, match_opcode,  WR_fd|RD_fs1|RD_fs2 },
{"fsqrt.h",   "Xhwacha",   "D,S",  MATCH_FSQRT_H | MASK_RM, MASK_FSQRT_H | MASK_RM, match_opcode, WR_fd|RD_fs1 },
{"fsqrt.h",   "Xhwacha",   "D,S,m",  MATCH_FSQRT_H, MASK_FSQRT_H, match_opcode, WR_fd|RD_fs1 },
{"fmin.h",    "Xhwacha",   "D,S,T",  MATCH_FMIN_H, MASK_FMIN_H, match_opcode,  WR_fd|RD_fs1|RD_fs2 },
{"fmax.h",    "Xhwacha",   "D,S,T",  MATCH_FMAX_H, MASK_FMAX_H, match_opcode,  WR_fd|RD_fs1|RD_fs2 },
{"fmadd.h",   "Xhwacha",   "D,S,T,R",  MATCH_FMADD_H | MASK_RM, MASK_FMADD_H | MASK_RM, match_opcode,  WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fmadd.h",   "Xhwacha",   "D,S,T,R,m",  MATCH_FMADD_H, MASK_FMADD_H, match_opcode,  WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fnmadd.h",  "Xhwacha",   "D,S,T,R",  MATCH_FNMADD_H | MASK_RM, MASK_FNMADD_H | MASK_RM, match_opcode,  WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fnmadd.h",  "Xhwacha",   "D,S,T,R,m",  MATCH_FNMADD_H, MASK_FNMADD_H, match_opcode,  WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fmsub.h",   "Xhwacha",   "D,S,T,R",  MATCH_FMSUB_H | MASK_RM, MASK_FMSUB_H | MASK_RM, match_opcode,  WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fmsub.h",   "Xhwacha",   "D,S,T,R,m",  MATCH_FMSUB_H, MASK_FMSUB_H, match_opcode,  WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fnmsub.h",  "Xhwacha",   "D,S,T,R",  MATCH_FNMSUB_H | MASK_RM, MASK_FNMSUB_H | MASK_RM, match_opcode,  WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fnmsub.h",  "Xhwacha",   "D,S,T,R,m",  MATCH_FNMSUB_H, MASK_FNMSUB_H, match_opcode,  WR_fd|RD_fs1|RD_fs2|RD_fs3 },
{"fcvt.s.h",  "Xhwacha",   "D,S",  MATCH_FCVT_S_H, MASK_FCVT_S_H | MASK_RM, match_opcode,  WR_fd|RD_fs1 },
{"fcvt.h.s",  "Xhwacha",   "D,S",  MATCH_FCVT_H_S | MASK_RM, MASK_FCVT_H_S | MASK_RM, match_opcode,  WR_fd|RD_fs1 },
{"fcvt.h.s",  "Xhwacha",   "D,S,m",  MATCH_FCVT_H_S, MASK_FCVT_H_S, match_opcode,  WR_fd|RD_fs1 },
{"fcvt.d.h",  "Xhwacha",   "D,S",  MATCH_FCVT_D_H, MASK_FCVT_D_H | MASK_RM, match_opcode,  WR_fd|RD_fs1 },
{"fcvt.h.d",  "Xhwacha",   "D,S",  MATCH_FCVT_H_D | MASK_RM, MASK_FCVT_H_D | MASK_RM, match_opcode,  WR_fd|RD_fs1 },
{"fcvt.h.d",  "Xhwacha",   "D,S,m",  MATCH_FCVT_H_D, MASK_FCVT_H_D, match_opcode,  WR_fd|RD_fs1 },
{"feq.h",     "Xhwacha",   "d,S,T",    MATCH_FEQ_H, MASK_FEQ_H, match_opcode, WR_xd|RD_fs1|RD_fs2 },
{"flt.h",     "Xhwacha",   "d,S,T",    MATCH_FLT_H, MASK_FLT_H, match_opcode, WR_xd|RD_fs1|RD_fs2 },
{"fle.h",     "Xhwacha",   "d,S,T",    MATCH_FLE_H, MASK_FLE_H, match_opcode, WR_xd|RD_fs1|RD_fs2 },
{"fgt.h",     "Xhwacha",   "d,T,S",    MATCH_FLT_H, MASK_FLT_H, match_opcode, WR_xd|RD_fs1|RD_fs2 },
{"fge.h",     "Xhwacha",   "d,T,S",    MATCH_FLE_H, MASK_FLE_H, match_opcode, WR_xd|RD_fs1|RD_fs2 },
{"fmv.x.h",   "Xhwacha",   "d,S",  MATCH_FMV_X_H, MASK_FMV_X_H, match_opcode, WR_xd|RD_fs1 },
{"fmv.h.x",   "Xhwacha",   "D,s",  MATCH_FMV_H_X, MASK_FMV_H_X, match_opcode, WR_fd|RD_xs1 },
{"fcvt.w.h",  "Xhwacha",   "d,S",  MATCH_FCVT_W_H | MASK_RM, MASK_FCVT_W_H | MASK_RM, match_opcode, WR_xd|RD_fs1 },
{"fcvt.w.h",  "Xhwacha",   "d,S,m",  MATCH_FCVT_W_H, MASK_FCVT_W_H, match_opcode, WR_xd|RD_fs1 },
{"fcvt.wu.h", "Xhwacha",   "d,S",  MATCH_FCVT_WU_H | MASK_RM, MASK_FCVT_WU_H | MASK_RM, match_opcode, WR_xd|RD_fs1 },
{"fcvt.wu.h", "Xhwacha",   "d,S,m",  MATCH_FCVT_WU_H, MASK_FCVT_WU_H, match_opcode, WR_xd|RD_fs1 },
{"fcvt.h.w",  "Xhwacha",   "D,s",  MATCH_FCVT_H_W, MASK_FCVT_H_W | MASK_RM, match_opcode,  WR_fd|RD_xs1 },
{"fcvt.h.wu", "Xhwacha",   "D,s",  MATCH_FCVT_H_WU, MASK_FCVT_H_WU | MASK_RM, match_opcode,  WR_fd|RD_xs1 },
{"fcvt.l.h",  "Xhwacha", "d,S",  MATCH_FCVT_L_H | MASK_RM, MASK_FCVT_L_H | MASK_RM, match_opcode, WR_xd|RD_fs1 },
{"fcvt.l.h",  "Xhwacha", "d,S,m",  MATCH_FCVT_L_H, MASK_FCVT_L_H, match_opcode, WR_xd|RD_fs1 },
{"fcvt.lu.h", "Xhwacha", "d,S",  MATCH_FCVT_LU_H | MASK_RM, MASK_FCVT_LU_H | MASK_RM, match_opcode, WR_xd|RD_fs1 },
{"fcvt.lu.h", "Xhwacha", "d,S,m",  MATCH_FCVT_LU_H, MASK_FCVT_LU_H, match_opcode, WR_xd|RD_fs1 },
{"fcvt.h.l",  "Xhwacha", "D,s",  MATCH_FCVT_H_L | MASK_RM, MASK_FCVT_H_L | MASK_RM, match_opcode,  WR_fd|RD_xs1 },
{"fcvt.h.l",  "Xhwacha", "D,s,m",  MATCH_FCVT_H_L, MASK_FCVT_H_L, match_opcode,  WR_fd|RD_xs1 },
{"fcvt.h.lu", "Xhwacha", "D,s",  MATCH_FCVT_H_LU | MASK_RM, MASK_FCVT_H_L | MASK_RM, match_opcode,  WR_fd|RD_xs1 },
{"fcvt.h.lu", "Xhwacha", "D,s,m",  MATCH_FCVT_H_LU, MASK_FCVT_H_LU, match_opcode,  WR_fd|RD_xs1 },

/* Rocket Custom Coprocessor extension */
{"custom0",   "Xcustom", "d,s,t,^j", MATCH_CUSTOM0_RD_RS1_RS2, MASK_CUSTOM0_RD_RS1_RS2, match_opcode, 0},
{"custom0",   "Xcustom", "d,s,^t,^j", MATCH_CUSTOM0_RD_RS1, MASK_CUSTOM0_RD_RS1, match_opcode, 0},
{"custom0",   "Xcustom", "d,^s,^t,^j", MATCH_CUSTOM0_RD, MASK_CUSTOM0_RD, match_opcode, 0},
{"custom0",   "Xcustom", "^d,s,t,^j", MATCH_CUSTOM0_RS1_RS2, MASK_CUSTOM0_RS1_RS2, match_opcode, 0},
{"custom0",   "Xcustom", "^d,s,^t,^j", MATCH_CUSTOM0_RS1, MASK_CUSTOM0_RS1, match_opcode, 0},
{"custom0",   "Xcustom", "^d,^s,^t,^j", MATCH_CUSTOM0, MASK_CUSTOM0, match_opcode, 0},
{"custom1",   "Xcustom", "d,s,t,^j", MATCH_CUSTOM1_RD_RS1_RS2, MASK_CUSTOM1_RD_RS1_RS2, match_opcode, 0},
{"custom1",   "Xcustom", "d,s,^t,^j", MATCH_CUSTOM1_RD_RS1, MASK_CUSTOM1_RD_RS1, match_opcode, 0},
{"custom1",   "Xcustom", "d,^s,^t,^j", MATCH_CUSTOM1_RD, MASK_CUSTOM1_RD, match_opcode, 0},
{"custom1",   "Xcustom", "^d,s,t,^j", MATCH_CUSTOM1_RS1_RS2, MASK_CUSTOM1_RS1_RS2, match_opcode, 0},
{"custom1",   "Xcustom", "^d,s,^t,^j", MATCH_CUSTOM1_RS1, MASK_CUSTOM1_RS1, match_opcode, 0},
{"custom1",   "Xcustom", "^d,^s,^t,^j", MATCH_CUSTOM1, MASK_CUSTOM1, match_opcode, 0},
{"custom2",   "Xcustom", "d,s,t,^j", MATCH_CUSTOM2_RD_RS1_RS2, MASK_CUSTOM2_RD_RS1_RS2, match_opcode, 0},
{"custom2",   "Xcustom", "d,s,^t,^j", MATCH_CUSTOM2_RD_RS1, MASK_CUSTOM2_RD_RS1, match_opcode, 0},
{"custom2",   "Xcustom", "d,^s,^t,^j", MATCH_CUSTOM2_RD, MASK_CUSTOM2_RD, match_opcode, 0},
{"custom2",   "Xcustom", "^d,s,t,^j", MATCH_CUSTOM2_RS1_RS2, MASK_CUSTOM2_RS1_RS2, match_opcode, 0},
{"custom2",   "Xcustom", "^d,s,^t,^j", MATCH_CUSTOM2_RS1, MASK_CUSTOM2_RS1, match_opcode, 0},
{"custom2",   "Xcustom", "^d,^s,^t,^j", MATCH_CUSTOM2, MASK_CUSTOM2, match_opcode, 0},
{"custom3",   "Xcustom", "d,s,t,^j", MATCH_CUSTOM3_RD_RS1_RS2, MASK_CUSTOM3_RD_RS1_RS2, match_opcode, 0},
{"custom3",   "Xcustom", "d,s,^t,^j", MATCH_CUSTOM3_RD_RS1, MASK_CUSTOM3_RD_RS1, match_opcode, 0},
{"custom3",   "Xcustom", "d,^s,^t,^j", MATCH_CUSTOM3_RD, MASK_CUSTOM3_RD, match_opcode, 0},
{"custom3",   "Xcustom", "^d,s,t,^j", MATCH_CUSTOM3_RS1_RS2, MASK_CUSTOM3_RS1_RS2, match_opcode, 0},
{"custom3",   "Xcustom", "^d,s,^t,^j", MATCH_CUSTOM3_RS1, MASK_CUSTOM3_RS1, match_opcode, 0},
{"custom3",   "Xcustom", "^d,^s,^t,^j", MATCH_CUSTOM3, MASK_CUSTOM3, match_opcode, 0},

/* Xhwacha extension */
{"stop",      "Xhwacha", "", MATCH_STOP, MASK_STOP, match_opcode, 0},
{"utidx",     "Xhwacha", "d", MATCH_UTIDX, MASK_UTIDX, match_opcode, WR_xd},
{"movz",      "Xhwacha", "d,s,t", MATCH_MOVZ, MASK_MOVZ, match_opcode, WR_xd|RD_xs1|RD_xs2},
{"movn",      "Xhwacha", "d,s,t", MATCH_MOVN, MASK_MOVN, match_opcode, WR_xd|RD_xs1|RD_xs2},
{"fmovz",     "Xhwacha", "D,s,T", MATCH_FMOVZ, MASK_FMOVZ, match_opcode, WR_fd|RD_xs1|RD_fs2},
{"fmovn",     "Xhwacha", "D,s,T", MATCH_FMOVN, MASK_FMOVN, match_opcode, WR_fd|RD_xs1|RD_fs2},

/* unit stride */
/* xloads */
{"vld",       "Xhwacha", "#d,s", MATCH_VLD, MASK_VLD, match_opcode, 0},
{"vlw",       "Xhwacha", "#d,s", MATCH_VLW, MASK_VLW, match_opcode, 0},
{"vlwu",      "Xhwacha", "#d,s", MATCH_VLWU, MASK_VLWU, match_opcode, 0},
{"vlh",       "Xhwacha", "#d,s", MATCH_VLH, MASK_VLH, match_opcode, 0},
{"vlhu",      "Xhwacha", "#d,s", MATCH_VLHU, MASK_VLHU, match_opcode, 0},
{"vlb",       "Xhwacha", "#d,s", MATCH_VLB, MASK_VLB, match_opcode, 0},
{"vlbu",      "Xhwacha", "#d,s", MATCH_VLBU, MASK_VLBU, match_opcode, 0},
/* floads */
{"vfld",      "Xhwacha", "#D,s", MATCH_VFLD, MASK_VFLD, match_opcode, 0},
{"vflw",      "Xhwacha", "#D,s", MATCH_VFLW, MASK_VFLW, match_opcode, 0},

/* stride */
/* xloads */
{"vlstd",     "Xhwacha", "#d,s,t", MATCH_VLSTD, MASK_VLSTD, match_opcode, 0},
{"vlstw",     "Xhwacha", "#d,s,t", MATCH_VLSTW, MASK_VLSTW, match_opcode, 0},
{"vlstwu",    "Xhwacha", "#d,s,t", MATCH_VLSTWU, MASK_VLSTWU, match_opcode, 0},
{"vlsth",     "Xhwacha", "#d,s,t", MATCH_VLSTH, MASK_VLSTH, match_opcode, 0},
{"vlsthu",    "Xhwacha", "#d,s,t", MATCH_VLSTHU, MASK_VLSTHU, match_opcode, 0},
{"vlstb",     "Xhwacha", "#d,s,t", MATCH_VLSTB, MASK_VLSTB, match_opcode, 0},
{"vlstbu",    "Xhwacha", "#d,s,t", MATCH_VLSTBU, MASK_VLSTBU, match_opcode, 0},
/* floads */
{"vflstd",    "Xhwacha", "#D,s,t", MATCH_VFLSTD, MASK_VFLSTD, match_opcode, 0},
{"vflstw",    "Xhwacha", "#D,s,t", MATCH_VFLSTW, MASK_VFLSTW, match_opcode, 0},

/* segment */
/* xloads */
{"vlsegd",    "Xhwacha", "#d,s,#n", MATCH_VLSEGD, MASK_VLSEGD, match_opcode, 0},
{"vlsegw",    "Xhwacha", "#d,s,#n", MATCH_VLSEGW, MASK_VLSEGW, match_opcode, 0},
{"vlsegwu",   "Xhwacha", "#d,s,#n", MATCH_VLSEGWU, MASK_VLSEGWU, match_opcode, 0},
{"vlsegh",    "Xhwacha", "#d,s,#n", MATCH_VLSEGH, MASK_VLSEGH, match_opcode, 0},
{"vlseghu",   "Xhwacha", "#d,s,#n", MATCH_VLSEGHU, MASK_VLSEGHU, match_opcode, 0},
{"vlsegb",    "Xhwacha", "#d,s,#n", MATCH_VLSEGB, MASK_VLSEGB, match_opcode, 0},
{"vlsegbu",   "Xhwacha", "#d,s,#n", MATCH_VLSEGBU, MASK_VLSEGBU, match_opcode, 0},
/* floads */
{"vflsegd",   "Xhwacha", "#D,s,#n", MATCH_VFLSEGD, MASK_VFLSEGD, match_opcode, 0},
{"vflsegw",   "Xhwacha", "#D,s,#n", MATCH_VFLSEGW, MASK_VFLSEGW, match_opcode, 0},

/* stride segment */
/* xloads */
{"vlsegstd",  "Xhwacha", "#d,s,t,#n", MATCH_VLSEGSTD, MASK_VLSEGSTD, match_opcode, 0},
{"vlsegstw",  "Xhwacha", "#d,s,t,#n", MATCH_VLSEGSTW, MASK_VLSEGSTW, match_opcode, 0},
{"vlsegstwu", "Xhwacha", "#d,s,t,#n", MATCH_VLSEGSTWU, MASK_VLSEGSTWU, match_opcode, 0},
{"vlsegsth",  "Xhwacha", "#d,s,t,#n", MATCH_VLSEGSTH, MASK_VLSEGSTH, match_opcode, 0},
{"vlsegsthu", "Xhwacha", "#d,s,t,#n", MATCH_VLSEGSTHU, MASK_VLSEGSTHU, match_opcode, 0},
{"vlsegstb",  "Xhwacha", "#d,s,t,#n", MATCH_VLSEGSTB, MASK_VLSEGSTB, match_opcode, 0},
{"vlsegstbu", "Xhwacha", "#d,s,t,#n", MATCH_VLSEGSTBU, MASK_VLSEGSTBU, match_opcode, 0},
/* floads */
{"vflsegstd", "Xhwacha", "#D,s,t,#n", MATCH_VFLSEGSTD, MASK_VFLSEGSTD, match_opcode, 0},
{"vflsegstw", "Xhwacha", "#D,s,t,#n", MATCH_VFLSEGSTW, MASK_VFLSEGSTW, match_opcode, 0},

/* unit stride */
/* xstores */
{"vsd",       "Xhwacha", "#d,s", MATCH_VSD, MASK_VSD, match_opcode, 0},
{"vsw",       "Xhwacha", "#d,s", MATCH_VSW, MASK_VSW, match_opcode, 0},
{"vsh",       "Xhwacha", "#d,s", MATCH_VSH, MASK_VSH, match_opcode, 0},
{"vsb",       "Xhwacha", "#d,s", MATCH_VSB, MASK_VSB, match_opcode, 0},
/* fstores */
{"vfsd",      "Xhwacha", "#D,s", MATCH_VFSD, MASK_VFSD, match_opcode, 0},
{"vfsw",      "Xhwacha", "#D,s", MATCH_VFSW, MASK_VFSW, match_opcode, 0},

/* stride */
/* xstores */
{"vsstd",     "Xhwacha", "#d,s,t", MATCH_VSSTD, MASK_VSSTD, match_opcode, 0},
{"vsstw",     "Xhwacha", "#d,s,t", MATCH_VSSTW, MASK_VSSTW, match_opcode, 0},
{"vssth",     "Xhwacha", "#d,s,t", MATCH_VSSTH, MASK_VSSTH, match_opcode, 0},
{"vsstb",     "Xhwacha", "#d,s,t", MATCH_VSSTB, MASK_VSSTB, match_opcode, 0},
/* fstores */
{"vfsstd",    "Xhwacha", "#D,s,t", MATCH_VFSSTD, MASK_VFSSTD, match_opcode, 0},
{"vfsstw",    "Xhwacha", "#D,s,t", MATCH_VFSSTW, MASK_VFSSTW, match_opcode, 0},

/* segment */
/* xstores */
{"vssegd",    "Xhwacha", "#d,s,#n", MATCH_VSSEGD, MASK_VSSEGD, match_opcode, 0},
{"vssegw",    "Xhwacha", "#d,s,#n", MATCH_VSSEGW, MASK_VSSEGW, match_opcode, 0},
{"vssegh",    "Xhwacha", "#d,s,#n", MATCH_VSSEGH, MASK_VSSEGH, match_opcode, 0},
{"vssegb",    "Xhwacha", "#d,s,#n", MATCH_VSSEGB, MASK_VSSEGB, match_opcode, 0},
/* fstores */
{"vfssegd",   "Xhwacha", "#D,s,#n", MATCH_VFSSEGD, MASK_VFSSEGD, match_opcode, 0},
{"vfssegw",   "Xhwacha", "#D,s,#n", MATCH_VFSSEGW, MASK_VFSSEGW, match_opcode, 0},

/* stride segment */
/* xsegstores */
{"vssegstd",  "Xhwacha", "#d,s,t,#n", MATCH_VSSEGSTD, MASK_VSSEGSTD, match_opcode, 0},
{"vssegstw",  "Xhwacha", "#d,s,t,#n", MATCH_VSSEGSTW, MASK_VSSEGSTW, match_opcode, 0},
{"vssegsth",  "Xhwacha", "#d,s,t,#n", MATCH_VSSEGSTH, MASK_VSSEGSTH, match_opcode, 0},
{"vssegstb",  "Xhwacha", "#d,s,t,#n", MATCH_VSSEGSTB, MASK_VSSEGSTB, match_opcode, 0},
/* fsegstores */
{"vfssegstd", "Xhwacha", "#D,s,t,#n", MATCH_VFSSEGSTD, MASK_VFSSEGSTD, match_opcode, 0},
{"vfssegstw", "Xhwacha", "#D,s,t,#n", MATCH_VFSSEGSTW, MASK_VFSSEGSTW, match_opcode, 0},

{"vsetcfg",   "Xhwacha", "s", MATCH_VSETCFG, MASK_VSETCFG | MASK_IMM, match_opcode, 0},
{"vsetcfg",   "Xhwacha", "#g,#f", MATCH_VSETCFG, MASK_VSETCFG | MASK_RS1, match_opcode, 0},
{"vsetcfg",   "Xhwacha", "s,#g,#f", MATCH_VSETCFG, MASK_VSETCFG, match_opcode, 0},
{"vsetucfg",  "Xhwacha", "d,u", MATCH_LUI, MASK_LUI, match_opcode, INSN_ALIAS | WR_xd},
{"vsetvl",    "Xhwacha", "d,s", MATCH_VSETVL, MASK_VSETVL, match_opcode, 0},
{"vgetcfg",   "Xhwacha", "d", MATCH_VGETCFG, MASK_VGETCFG, match_opcode, 0},
{"vgetvl",    "Xhwacha", "d", MATCH_VGETVL, MASK_VGETVL, match_opcode, 0},

{"vmvv",      "Xhwacha", "#d,#s", MATCH_VMVV, MASK_VMVV, match_opcode, 0},
{"vmsv",      "Xhwacha", "#d,s", MATCH_VMSV, MASK_VMSV, match_opcode, 0},
{"vfmvv",     "Xhwacha", "#D,#S", MATCH_VFMVV, MASK_VFMVV, match_opcode, 0},
{"vfmsv.d",   "Xhwacha", "#D,s", MATCH_VFMSV_D, MASK_VFMSV_D, match_opcode, 0},
{"vfmsv.s",   "Xhwacha", "#D,s", MATCH_VFMSV_S, MASK_VFMSV_S, match_opcode, 0},

{"vf",        "Xhwacha", "q(s)", MATCH_VF, MASK_VF, match_opcode, 0},
{"vf",        "Xhwacha", "A,s", 0, (int) M_VF, match_never, INSN_MACRO },

{"vxcptcause",   "Xhwacha", "d", MATCH_VXCPTCAUSE, MASK_VXCPTCAUSE, match_opcode, 0},
{"vxcptaux",     "Xhwacha", "d", MATCH_VXCPTAUX, MASK_VXCPTAUX, match_opcode, 0},

{"vxcptsave",    "Xhwacha", "s", MATCH_VXCPTSAVE, MASK_VXCPTSAVE, match_opcode, 0},
{"vxcptrestore", "Xhwacha", "s", MATCH_VXCPTRESTORE, MASK_VXCPTRESTORE, match_opcode, 0},
{"vxcptkill",    "Xhwacha", "", MATCH_VXCPTKILL, MASK_VXCPTKILL, match_opcode, 0},

{"vxcptevac",    "Xhwacha", "s", MATCH_VXCPTEVAC, MASK_VXCPTEVAC, match_opcode, 0},
{"vxcpthold",    "Xhwacha", "", MATCH_VXCPTHOLD, MASK_VXCPTHOLD, match_opcode, 0},
{"venqcmd",      "Xhwacha", "s,t", MATCH_VENQCMD, MASK_VENQCMD, match_opcode, 0},
{"venqimm1",     "Xhwacha", "s,t", MATCH_VENQIMM1, MASK_VENQIMM1, match_opcode, 0},
{"venqimm2",     "Xhwacha", "s,t", MATCH_VENQIMM2, MASK_VENQIMM2, match_opcode, 0},
{"venqcnt",      "Xhwacha", "s,t", MATCH_VENQCNT, MASK_VENQCNT, match_opcode, 0},
};

#define RISCV_NUM_OPCODES \
  ((sizeof riscv_builtin_opcodes) / (sizeof (riscv_builtin_opcodes[0])))
const int bfd_riscv_num_builtin_opcodes = RISCV_NUM_OPCODES;

/* const removed from the following to allow for dynamic extensions to the
 * built-in instruction set. */
struct riscv_opcode *riscv_opcodes =
  (struct riscv_opcode *) riscv_builtin_opcodes;
int bfd_riscv_num_opcodes = RISCV_NUM_OPCODES;
#undef RISCV_NUM_OPCODES
