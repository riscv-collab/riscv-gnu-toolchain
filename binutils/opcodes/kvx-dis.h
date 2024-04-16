/* kvx-dis.h -- Header file kvx-dis.c
   Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Contributed by Kalray SA.

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
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#ifndef _KV3_DIS_H_
#define _KV3_DIS_H_

#include "dis-asm.h"

#define KVX_GPR_REG_SP 12
#define KVX_GPR_REG_FP 14

enum kvx_prologue_epilogue_insn_type
{
  KVX_PROL_EPIL_INSN_SD,
  KVX_PROL_EPIL_INSN_SQ,
  KVX_PROL_EPIL_INSN_SO,
  KVX_PROL_EPIL_INSN_GET_RA,
  KVX_PROL_EPIL_INSN_ADD_FP,
  KVX_PROL_EPIL_INSN_ADD_SP,
  KVX_PROL_EPIL_INSN_RESTORE_SP_FROM_FP,
  KVX_PROL_EPIL_INSN_GOTO,
  KVX_PROL_EPIL_INSN_IGOTO,
  KVX_PROL_EPIL_INSN_CB,
  KVX_PROL_EPIL_INSN_RET,
  KVX_PROL_EPIL_INSN_CALL,
};

struct kvx_prologue_epilogue_insn
{
  enum kvx_prologue_epilogue_insn_type insn_type;
  uint64_t immediate;
  int gpr_reg[3];
  int nb_gprs;
};

struct kvx_prologue_epilogue_bundle
{
  struct kvx_prologue_epilogue_insn insn[6];
  int nb_insn;
};

int decode_prologue_epilogue_bundle (bfd_vma memaddr,
				     struct disassemble_info *info,
				     struct kvx_prologue_epilogue_bundle *pb);

void parse_kvx_dis_option (const char *option);

#endif
