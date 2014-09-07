/* RISC-V ISA encoding.
   Copyright (C) 2011-2014 Free Software Foundation, Inc.
   Contributed by Andrew Waterman (waterman@cs.berkeley.edu) at UC Berkeley.
   Based on MIPS target for GNU compiler.

This file is part of GDB, GAS, and the GNU binutils.

GDB, GAS, and the GNU binutils are free software; you can redistribute
them and/or modify them under the terms of the GNU General Public
License as published by the Free Software Foundation; either version
1, or (at your option) any later version.

GDB, GAS, and the GNU binutils are distributed in the hope that they
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this file; see the file COPYING.  If not, write to the Free
Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _RISCV_H_
#define _RISCV_H_

#define RV_X(x, s, n) (((x) >> (s)) & ((1<<(n))-1))
#define RV_IMM_SIGN(x) (-(((x) >> 31) & 1))

#define EXTRACT_ITYPE_IMM(x) \
  (RV_X(x, 20, 12) | (RV_IMM_SIGN(x) << 12))
#define EXTRACT_STYPE_IMM(x) \
  (RV_X(x, 7, 5) | (RV_X(x, 25, 7) << 5) | (RV_IMM_SIGN(x) << 12))
#define EXTRACT_SBTYPE_IMM(x) \
  ((RV_X(x, 8, 4) << 1) | (RV_X(x, 25, 6) << 5) | (RV_X(x, 7, 1) << 11) | (RV_IMM_SIGN(x) << 12))
#define EXTRACT_UTYPE_IMM(x) \
  (RV_X(x, 12, 20) | (RV_IMM_SIGN(x) << 20))
#define EXTRACT_UJTYPE_IMM(x) \
  ((RV_X(x, 21, 10) << 1) | (RV_X(x, 20, 1) << 11) | (RV_X(x, 12, 8) << 12) | (RV_IMM_SIGN(x) << 20))

#define ENCODE_ITYPE_IMM(x) \
  (RV_X(x, 0, 12) << 20)
#define ENCODE_STYPE_IMM(x) \
  ((RV_X(x, 0, 5) << 7) | (RV_X(x, 5, 7) << 25))
#define ENCODE_SBTYPE_IMM(x) \
  ((RV_X(x, 1, 4) << 8) | (RV_X(x, 5, 6) << 25) | (RV_X(x, 11, 1) << 7) | (RV_X(x, 12, 1) << 31))
#define ENCODE_UTYPE_IMM(x) \
  (RV_X(x, 0, 20) << 12)
#define ENCODE_UJTYPE_IMM(x) \
  ((RV_X(x, 1, 10) << 21) | (RV_X(x, 11, 1) << 20) | (RV_X(x, 12, 8) << 12) | (RV_X(x, 20, 1) << 31))

#define VALID_ITYPE_IMM(x) (EXTRACT_ITYPE_IMM(ENCODE_ITYPE_IMM(x)) == (x))
#define VALID_STYPE_IMM(x) (EXTRACT_STYPE_IMM(ENCODE_STYPE_IMM(x)) == (x))
#define VALID_SBTYPE_IMM(x) (EXTRACT_SBTYPE_IMM(ENCODE_SBTYPE_IMM(x)) == (x))
#define VALID_UTYPE_IMM(x) (EXTRACT_UTYPE_IMM(ENCODE_UTYPE_IMM(x)) == (x))
#define VALID_UJTYPE_IMM(x) (EXTRACT_UJTYPE_IMM(ENCODE_UJTYPE_IMM(x)) == (x))

#define RISCV_RTYPE(insn, rd, rs1, rs2) \
  ((MATCH_ ## insn) | ((rd) << OP_SH_RD) | ((rs1) << OP_SH_RS1) | ((rs2) << OP_SH_RS2))
#define RISCV_ITYPE(insn, rd, rs1, imm) \
  ((MATCH_ ## insn) | ((rd) << OP_SH_RD) | ((rs1) << OP_SH_RS1) | ENCODE_ITYPE_IMM(imm))
#define RISCV_STYPE(insn, rs1, rs2, imm) \
  ((MATCH_ ## insn) | ((rs1) << OP_SH_RS1) | ((rs2) << OP_SH_RS2) | ENCODE_STYPE_IMM(imm))
#define RISCV_SBTYPE(insn, rs1, rs2, target) \
  ((MATCH_ ## insn) | ((rs1) << OP_SH_RS1) | ((rs2) << OP_SH_RS2) | ENCODE_SBTYPE_IMM(target))
#define RISCV_UTYPE(insn, rd, bigimm) \
  ((MATCH_ ## insn) | ((rd) << OP_SH_RD) | ENCODE_UTYPE_IMM(bigimm))
#define RISCV_UJTYPE(insn, rd, target) \
  ((MATCH_ ## insn) | ((rd) << OP_SH_RD) | ENCODE_UJTYPE_IMM(target))

#define RISCV_NOP RISCV_ITYPE(ADDI, 0, 0, 0)

#define RISCV_CONST_HIGH_PART(VALUE) \
  (((VALUE) + (RISCV_IMM_REACH/2)) & ~(RISCV_IMM_REACH-1))
#define RISCV_CONST_LOW_PART(VALUE) ((VALUE) - RISCV_CONST_HIGH_PART (VALUE))
#define RISCV_LUI_HIGH_PART(VALUE) (RISCV_CONST_HIGH_PART(VALUE) >> RISCV_IMM_BITS)

/* RV fields */

#define OP_MASK_OP		0x7f
#define OP_SH_OP		0
#define OP_MASK_RS2		0x1f
#define OP_SH_RS2		20
#define OP_MASK_RS1		0x1f
#define OP_SH_RS1		15
#define OP_MASK_RS3		0x1f
#define OP_SH_RS3		27
#define OP_MASK_RD		0x1f
#define OP_SH_RD		7
#define OP_MASK_SHAMT		0x3f
#define OP_SH_SHAMT		20
#define OP_MASK_SHAMTW		0x1f
#define OP_SH_SHAMTW		20
#define OP_MASK_RM		0x7
#define OP_SH_RM		12
#define OP_MASK_PRED		0xf
#define OP_SH_PRED		24
#define OP_MASK_SUCC		0xf
#define OP_SH_SUCC		20
#define OP_MASK_AQ		0x1
#define OP_SH_AQ		26
#define OP_MASK_RL		0x1
#define OP_SH_RL		25

#define OP_MASK_VRD		0x1f
#define OP_SH_VRD		7
#define OP_MASK_VRS		0x1f
#define OP_SH_VRS		15
#define OP_MASK_VRT		0x1f
#define OP_SH_VRT		20
#define OP_MASK_VRR		0x1f
#define OP_SH_VRR		25

#define OP_MASK_VFD		0x1f
#define OP_SH_VFD		7
#define OP_MASK_VFS		0x1f
#define OP_SH_VFS		15
#define OP_MASK_VFT		0x1f
#define OP_SH_VFT		20
#define OP_MASK_VFR		0x1f
#define OP_SH_VFR		25

#define OP_MASK_IMMNGPR         0x3f
#define OP_SH_IMMNGPR           20
#define OP_MASK_IMMNFPR         0x3f
#define OP_SH_IMMNFPR           26
#define OP_MASK_IMMSEGNELM      0x1f
#define OP_SH_IMMSEGNELM        17
#define OP_MASK_IMMSEGSTNELM    0x1f
#define OP_SH_IMMSEGSTNELM      12
#define OP_MASK_CUSTOM_IMM      0x7f
#define OP_SH_CUSTOM_IMM        25

#define LINK_REG 1

#define RISCV_JUMP_BITS RISCV_BIGIMM_BITS
#define RISCV_JUMP_ALIGN_BITS 1
#define RISCV_JUMP_ALIGN (1 << RISCV_JUMP_ALIGN_BITS)
#define RISCV_JUMP_REACH ((1ULL<<RISCV_JUMP_BITS)*RISCV_JUMP_ALIGN)

#define RISCV_IMM_BITS 12
#define RISCV_BIGIMM_BITS (32-RISCV_IMM_BITS)
#define RISCV_IMM_REACH (1LL<<RISCV_IMM_BITS)
#define RISCV_BIGIMM_REACH (1LL<<RISCV_BIGIMM_BITS)
#define RISCV_BRANCH_BITS RISCV_IMM_BITS
#define RISCV_BRANCH_ALIGN_BITS RISCV_JUMP_ALIGN_BITS
#define RISCV_BRANCH_ALIGN (1 << RISCV_BRANCH_ALIGN_BITS)
#define RISCV_BRANCH_REACH (RISCV_IMM_REACH*RISCV_BRANCH_ALIGN)

#include "riscv-opc.h"

#endif /* _RISCV_H_ */
