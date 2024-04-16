/* bpf-opc.c - BPF opcodes.
   Copyright (C) 2023-2024 Free Software Foundation, Inc.

   Contributed by Oracle Inc.

   This file is part of the GNU binutils.

   This is free software; you can redistribute them and/or modify them
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include <stdlib.h>
#include "opcode/bpf.h"

/* Note that the entries in the opcodes table below are accessed
   sequentially when matching instructions per opcode, and also when
   parsing.  Please take care to keep the entries sorted
   accordingly!  */

const struct bpf_opcode bpf_opcodes[] =
{
  /* id, normal, pseudoc, version, mask, opcodes */

  /* ALU instructions.  */
  {BPF_INSN_ADDR, "add%W%dr , %sr", "%dr += %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_ADD|BPF_SRC_X},
  {BPF_INSN_ADDI, "add%W%dr , %i32", "%dr += %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_ADD|BPF_SRC_K},
  {BPF_INSN_SUBR, "sub%W%dr , %sr", "%dr -= %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_SUB|BPF_SRC_X},
  {BPF_INSN_SUBI, "sub%W%dr , %i32", "%dr -= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_SUB|BPF_SRC_K},
  {BPF_INSN_MULR, "mul%W%dr , %sr", "%dr *= %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_MUL|BPF_SRC_X},
  {BPF_INSN_MULI, "mul%W%dr , %i32", "%dr *= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_MUL|BPF_SRC_K},
  {BPF_INSN_SDIVR, "sdiv%W%dr, %sr", "%dr s/= %sr",
   BPF_V4, BPF_CODE|BPF_OFFSET16, BPF_CLASS_ALU64|BPF_CODE_DIV|BPF_SRC_X|BPF_OFFSET16_SDIVMOD},
  {BPF_INSN_SDIVI, "sdiv%W%dr , %i32","%dr s/= %i32",
   BPF_V4, BPF_CODE|BPF_OFFSET16, BPF_CLASS_ALU64|BPF_CODE_DIV|BPF_SRC_K|BPF_OFFSET16_SDIVMOD},
  {BPF_INSN_SMODR, "smod%W%dr , %sr", "%dr s%%= %sr",
   BPF_V4, BPF_CODE|BPF_OFFSET16, BPF_CLASS_ALU64|BPF_CODE_MOD|BPF_SRC_X|BPF_OFFSET16_SDIVMOD},
  {BPF_INSN_SMODI, "smod%W%dr , %i32", "%dr s%%= %i32",
   BPF_V4, BPF_CODE|BPF_OFFSET16, BPF_CLASS_ALU64|BPF_CODE_MOD|BPF_SRC_K|BPF_OFFSET16_SDIVMOD},
  {BPF_INSN_DIVR, "div%W%dr , %sr", "%dr /= %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_DIV|BPF_SRC_X},
  {BPF_INSN_DIVI, "div%W%dr , %i32", "%dr /= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_DIV|BPF_SRC_K},
  {BPF_INSN_MODR, "mod%W%dr , %sr", "%dr %%= %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_MOD|BPF_SRC_X},
  {BPF_INSN_MODI, "mod%W%dr , %i32", "%dr %%= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_MOD|BPF_SRC_K},
  {BPF_INSN_ORR, "or%W%dr , %sr", "%dr |= %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_OR|BPF_SRC_X},
  {BPF_INSN_ORI, "or%W%dr , %i32", "%dr |= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_OR|BPF_SRC_K},
  {BPF_INSN_ANDR, "and%W%dr , %sr", "%dr &= %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_AND|BPF_SRC_X},
  {BPF_INSN_ANDI, "and%W%dr , %i32", "%dr &= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_AND|BPF_SRC_K},
  {BPF_INSN_XORR, "xor%W%dr , %sr", "%dr ^= %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_XOR|BPF_SRC_X},
  {BPF_INSN_XORI, "xor%W%dr , %i32", "%dr ^= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_XOR|BPF_SRC_K},
  {BPF_INSN_NEGR, "neg%W%dr", "%dr = - %dr",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_NEG|BPF_SRC_K},
  {BPF_INSN_LSHR, "lsh%W%dr , %sr", "%dr <<= %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_LSH|BPF_SRC_X},
  {BPF_INSN_LSHI, "lsh%W%dr , %i32", "%dr <<= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_LSH|BPF_SRC_K},
  {BPF_INSN_RSHR, "rsh%W%dr , %sr", "%dr >>= %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_RSH|BPF_SRC_X},
  {BPF_INSN_RSHI, "rsh%W%dr , %i32", "%dr >>= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_RSH|BPF_SRC_K},
  {BPF_INSN_ARSHR, "arsh%W%dr , %sr", "%dr%ws>>= %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_ARSH|BPF_SRC_X},
  {BPF_INSN_ARSHI, "arsh%W%dr , %i32", "%dr%ws>>= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_ARSH|BPF_SRC_K},
  {BPF_INSN_MOVS8R, "movs%W%dr , %sr , 8", "%dr%w=%w( s8 )%w%sr",
   BPF_V4, BPF_CODE|BPF_OFFSET16, BPF_CLASS_ALU64|BPF_CODE_MOV|BPF_SRC_X|BPF_OFFSET16_MOVS8},
  {BPF_INSN_MOVS16R, "movs%W%dr , %sr , 16", "%dr%w=%w( s16 )%w%sr",
   BPF_V4, BPF_CODE|BPF_OFFSET16, BPF_CLASS_ALU64|BPF_CODE_MOV|BPF_SRC_X|BPF_OFFSET16_MOVS16},
  {BPF_INSN_MOVS32R, "movs%W%dr , %sr , 32", "%dr%w=%w( s32 )%w%sr",
   BPF_V4, BPF_CODE|BPF_OFFSET16, BPF_CLASS_ALU64|BPF_CODE_MOV|BPF_SRC_X|BPF_OFFSET16_MOVS32},
  {BPF_INSN_MOVR, "mov%W%dr , %sr", "%dr = %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_MOV|BPF_SRC_X},
  {BPF_INSN_MOVI, "mov%W%dr , %i32", "%dr = %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU64|BPF_CODE_MOV|BPF_SRC_K},

  /* ALU32 instructions.  */
  {BPF_INSN_ADD32R, "add32%W%dr , %sr", "%dw += %sw",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_ADD|BPF_SRC_X},
  {BPF_INSN_ADD32I, "add32%W%dr , %i32", "%dw += %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_ADD|BPF_SRC_K},
  {BPF_INSN_SUB32R, "sub32%W%dr , %sr", "%dw -= %sw",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_SUB|BPF_SRC_X},
  {BPF_INSN_SUB32I, "sub32%W%dr , %i32", "%dw -= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_SUB|BPF_SRC_K},
  {BPF_INSN_MUL32R, "mul32%W%dr , %sr", "%dw *= %sw",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_MUL|BPF_SRC_X},
  {BPF_INSN_MUL32I, "mul32%W%dr , %i32", "%dw *= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_MUL|BPF_SRC_K},
  {BPF_INSN_SDIV32R, "sdiv32%W%dr , %sr", "%dw s/= %sw",
   BPF_V4, BPF_CODE|BPF_OFFSET16, BPF_CLASS_ALU|BPF_CODE_DIV|BPF_SRC_X|BPF_OFFSET16_SDIVMOD},
  {BPF_INSN_SDIV32I, "sdiv32%W%dr , %i32", "%dw s/= %i32",
   BPF_V4, BPF_CODE|BPF_OFFSET16, BPF_CLASS_ALU|BPF_CODE_DIV|BPF_SRC_K|BPF_OFFSET16_SDIVMOD},
  {BPF_INSN_SMOD32R, "smod32%W%dr , %sr", "%dw s%%= %sw",
   BPF_V4, BPF_CODE|BPF_OFFSET16, BPF_CLASS_ALU|BPF_CODE_MOD|BPF_SRC_X|BPF_OFFSET16_SDIVMOD},
  {BPF_INSN_SMOD32I, "smod32%W%dr , %i32", "%dw s%%= %i32",
   BPF_V4, BPF_CODE|BPF_OFFSET16, BPF_CLASS_ALU|BPF_CODE_MOD|BPF_SRC_K|BPF_OFFSET16_SDIVMOD},
  {BPF_INSN_DIV32R, "div32%W%dr , %sr", "%dw /= %sw",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_DIV|BPF_SRC_X},
  {BPF_INSN_DIV32I, "div32%W%dr , %i32", "%dw /= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_DIV|BPF_SRC_K},
  {BPF_INSN_MOD32R, "mod32%W%dr , %sr", "%dw %%= %sw",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_MOD|BPF_SRC_X},
  {BPF_INSN_MOD32I, "mod32%W%dr , %i32", "%dw %%= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_MOD|BPF_SRC_K},
  {BPF_INSN_OR32R, "or32%W%dr , %sr",  "%dw |= %sw",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_OR|BPF_SRC_X},
  {BPF_INSN_OR32I, "or32%W%dr , %i32", "%dw |= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_OR|BPF_SRC_K},
  {BPF_INSN_AND32R, "and32%W%dr , %sr", "%dw &= %sw",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_AND|BPF_SRC_X},
  {BPF_INSN_AND32I, "and32%W%dr , %i32", "%dw &= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_AND|BPF_SRC_K},
  {BPF_INSN_XOR32R, "xor32%W%dr , %sr", "%dw ^= %sw",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_XOR|BPF_SRC_X},
  {BPF_INSN_XOR32I, "xor32%W%dr , %i32", "%dw ^= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_XOR|BPF_SRC_K},
  {BPF_INSN_NEG32R, "neg32%W%dr", "%dw = - %dw",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_NEG|BPF_SRC_K},
  {BPF_INSN_LSH32R, "lsh32%W%dr , %sr", "%dw <<= %sw",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_LSH|BPF_SRC_X},
  {BPF_INSN_LSH32I, "lsh32%W%dr , %i32", "%dw <<= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_LSH|BPF_SRC_K},
  {BPF_INSN_RSH32R, "rsh32%W%dr , %sr", "%dw >>= %sw",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_RSH|BPF_SRC_X},
  {BPF_INSN_RSH32I, "rsh32%W%dr , %i32", "%dw >>= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_RSH|BPF_SRC_K},
  {BPF_INSN_ARSH32R, "arsh32%W%dr , %sr", "%dw%ws>>= %sw",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_ARSH|BPF_SRC_X},
  {BPF_INSN_ARSH32I, "arsh32%W%dr , %i32", "%dw%Ws>>= %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_ARSH|BPF_SRC_K},
  {BPF_INSN_MOVS328R, "movs32%W%dr , %sr , 8", "%dw%w=%w( s8 )%w%sw",
   BPF_V4, BPF_CODE|BPF_OFFSET16, BPF_CLASS_ALU|BPF_CODE_MOV|BPF_SRC_X|BPF_OFFSET16_MOVS8},
  {BPF_INSN_MOVS3216R, "movs32%W%dr , %sr , 16", "%dw%w=%w( s16 )%w%sw",
   BPF_V4, BPF_CODE|BPF_OFFSET16, BPF_CLASS_ALU|BPF_CODE_MOV|BPF_SRC_X|BPF_OFFSET16_MOVS16},
  {BPF_INSN_MOVS3232R, "movs32%W%dr , %sr , 32", "%dw%w=%w( s32 )%w%sw",
   BPF_V4, BPF_CODE|BPF_OFFSET16, BPF_CLASS_ALU|BPF_CODE_MOV|BPF_SRC_X|BPF_OFFSET16_MOVS32},
  {BPF_INSN_MOV32R, "mov32%W%dr , %sr", "%dw = %sw",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_MOV|BPF_SRC_X},
  {BPF_INSN_MOV32I, "mov32%W%dr , %i32", "%dw = %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ALU|BPF_CODE_MOV|BPF_SRC_K},

  /* Endianness conversion instructions.  */
  {BPF_INSN_ENDLE16, "endle%W%dr , 16", "%dr = le16%w%dr",
   BPF_V1, BPF_CODE|BPF_IMM32, BPF_CLASS_ALU|BPF_CODE_END|BPF_SRC_K|BPF_IMM32_END16},
  {BPF_INSN_ENDLE32, "endle%W%dr , 32", "%dr = le32%w%dr",
   BPF_V1, BPF_CODE|BPF_IMM32, BPF_CLASS_ALU|BPF_CODE_END|BPF_SRC_K|BPF_IMM32_END32},
  {BPF_INSN_ENDLE64, "endle%W%dr , 64", "%dr = le64%w%dr",
   BPF_V1, BPF_CODE|BPF_IMM32, BPF_CLASS_ALU|BPF_CODE_END|BPF_SRC_K|BPF_IMM32_END64},
  {BPF_INSN_ENDBE16, "endbe%W%dr , 16", "%dr = be16%w%dr",
   BPF_V1, BPF_CODE|BPF_IMM32, BPF_CLASS_ALU|BPF_CODE_END|BPF_SRC_X|BPF_IMM32_END16},
  {BPF_INSN_ENDBE32, "endbe%W%dr , 32", "%dr = be32%w%dr",
   BPF_V1, BPF_CODE|BPF_IMM32, BPF_CLASS_ALU|BPF_CODE_END|BPF_SRC_X|BPF_IMM32_END32},
  {BPF_INSN_ENDBE64, "endbe%W%dr , 64", "%dr = be64%w%dr",
   BPF_V1, BPF_CODE|BPF_IMM32, BPF_CLASS_ALU|BPF_CODE_END|BPF_SRC_X|BPF_IMM32_END64},

  /* Byte-swap instructions.  */
  {BPF_INSN_BSWAP16, "bswap%W%dr , 16", "%dr%w=%wbswap16%w%dr",
   BPF_V4, BPF_CODE|BPF_IMM32, BPF_CLASS_ALU64|BPF_CODE_END|BPF_SRC_K|BPF_IMM32_BSWAP16},
  {BPF_INSN_BSWAP32, "bswap%W%dr , 32", "%dr%w=%wbswap32%w%dr",
   BPF_V4, BPF_CODE|BPF_IMM32, BPF_CLASS_ALU64|BPF_CODE_END|BPF_SRC_K|BPF_IMM32_BSWAP32},
  {BPF_INSN_BSWAP64, "bswap%W%dr , 64", "%dr%w=%wbswap64%w%dr",
   BPF_V4, BPF_CODE|BPF_IMM32, BPF_CLASS_ALU64|BPF_CODE_END|BPF_SRC_K|BPF_IMM32_BSWAP64},

  /* 64-bit load instruction.  */
  {BPF_INSN_LDDW, "lddw%W%dr , %i64", "%dr = %i64%wll",
   BPF_V1, BPF_CODE, BPF_CLASS_LD|BPF_SIZE_DW|BPF_MODE_IMM},

  /* Indirect load instructions, designed to be used in socket
     filters.  */
  {BPF_INSN_LDINDB, "ldindb%W%sr , %i32", "r0 = * ( u8 * ) skb [ %sr %I32 ]",
   BPF_V1, BPF_CODE, BPF_CLASS_LD|BPF_SIZE_B|BPF_MODE_IND},
  {BPF_INSN_LDINDH, "ldindh%W%sr , %i32", "r0 = * ( u16 * ) skb [ %sr %I32 ]",
   BPF_V1, BPF_CODE, BPF_CLASS_LD|BPF_SIZE_H|BPF_MODE_IND},
  {BPF_INSN_LDINDW, "ldindw%W%sr , %i32", "r0 = * ( u32 * ) skb [ %sr %I32 ]",
   BPF_V1, BPF_CODE, BPF_CLASS_LD|BPF_SIZE_W|BPF_MODE_IND},
  {BPF_INSN_LDINDDW, "ldinddw%W%sr , %i32", "r0 = * ( u64 * ) skb [ %sr %I32 ]",
   BPF_V1, BPF_CODE, BPF_CLASS_LD|BPF_SIZE_DW|BPF_MODE_IND},

  /* Absolute load instructions, designed to be used in socket filters.  */
  {BPF_INSN_LDABSB, "ldabsb%W%i32", "r0 = * ( u8 * ) skb [ %i32 ]",
   BPF_V1, BPF_CODE, BPF_CLASS_LD|BPF_SIZE_B|BPF_MODE_ABS},
  {BPF_INSN_LDABSH, "ldabsh%W%i32", "r0 = * ( u16 * ) skb [ %i32 ]",
   BPF_V1, BPF_CODE, BPF_CLASS_LD|BPF_SIZE_H|BPF_MODE_ABS},
  {BPF_INSN_LDABSW, "ldabsw%W%i32", "r0 = * ( u32 * ) skb [ %i32 ]",
   BPF_V1, BPF_CODE, BPF_CLASS_LD|BPF_SIZE_W|BPF_MODE_ABS},
  {BPF_INSN_LDABSDW, "ldabsdw%W%i32", "r0 = * ( u64 * ) skb [ %i32 ]",
   BPF_V1, BPF_CODE, BPF_CLASS_LD|BPF_SIZE_DW|BPF_MODE_ABS},

  /* Generic load instructions (to register.)  */
  {BPF_INSN_LDXB, "ldxb%W%dr , [ %sr %o16 ]", "%dr = * ( u8 * ) ( %sr %o16 )",
   BPF_V1, BPF_CODE, BPF_CLASS_LDX|BPF_SIZE_B|BPF_MODE_MEM},
  {BPF_INSN_LDXH, "ldxh%W%dr , [ %sr %o16 ]", "%dr = * ( u16 * ) ( %sr %o16 )",
   BPF_V1, BPF_CODE, BPF_CLASS_LDX|BPF_SIZE_H|BPF_MODE_MEM},
  {BPF_INSN_LDXW, "ldxw%W%dr , [ %sr %o16 ]", "%dr = * ( u32 * ) ( %sr %o16 )",
   BPF_V1, BPF_CODE, BPF_CLASS_LDX|BPF_SIZE_W|BPF_MODE_MEM},
  {BPF_INSN_LDXDW, "ldxdw%W%dr , [ %sr %o16 ]","%dr = * ( u64 * ) ( %sr %o16 )",
   BPF_V1, BPF_CODE, BPF_CLASS_LDX|BPF_SIZE_DW|BPF_MODE_MEM},

  /* Generic signed load instructions (to register.)  */
  {BPF_INSN_LDXSB, "ldxsb%W%dr , [ %sr %o16 ]", "%dr = * ( s8 * ) ( %sr %o16 )",
   BPF_V4, BPF_CODE, BPF_CLASS_LDX|BPF_SIZE_B|BPF_MODE_SMEM},
  {BPF_INSN_LDXSH, "ldxsh%W%dr , [ %sr %o16 ]", "%dr = * ( s16 * ) ( %sr %o16 )",
   BPF_V4, BPF_CODE, BPF_CLASS_LDX|BPF_SIZE_H|BPF_MODE_SMEM},
  {BPF_INSN_LDXSW, "ldxsw%W%dr , [ %sr %o16 ]", "%dr = * ( s32 * ) ( %sr %o16 )",
   BPF_V4, BPF_CODE, BPF_CLASS_LDX|BPF_SIZE_W|BPF_MODE_SMEM},
  {BPF_INSN_LDXSDW, "ldxsdw%W%dr , [ %sr %o16 ]","%dr = * ( s64 * ) ( %sr %o16 )",
   BPF_V4, BPF_CODE, BPF_CLASS_LDX|BPF_SIZE_DW|BPF_MODE_SMEM},

  /* Generic store instructions (from register.)  */
  {BPF_INSN_STXBR, "stxb%W[ %dr %o16 ] , %sr", "* ( u8 * ) ( %dr %o16 ) = %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_STX|BPF_SIZE_B|BPF_MODE_MEM},
  {BPF_INSN_STXHR, "stxh%W[ %dr %o16 ] , %sr", "* ( u16 * ) ( %dr %o16 ) = %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_STX|BPF_SIZE_H|BPF_MODE_MEM},
  {BPF_INSN_STXWR, "stxw%W[ %dr %o16 ], %sr", "* ( u32 * ) ( %dr %o16 ) = %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_STX|BPF_SIZE_W|BPF_MODE_MEM},
  {BPF_INSN_STXDWR, "stxdw%W[ %dr %o16 ] , %sr", "* ( u64 * ) ( %dr %o16 ) = %sr",
   BPF_V1, BPF_CODE, BPF_CLASS_STX|BPF_SIZE_DW|BPF_MODE_MEM},

  /* Generic store instructions (from 32-bit immediate.)  */
  {BPF_INSN_STXBI, "stb%W[ %dr %o16 ] , %i32", "* ( u8 * ) ( %dr %o16 ) = %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ST|BPF_SIZE_B|BPF_MODE_MEM},
  {BPF_INSN_STXHI, "sth%W[ %dr %o16 ] , %i32", "* ( u16 * ) ( %dr %o16 ) = %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ST|BPF_SIZE_H|BPF_MODE_MEM},
  {BPF_INSN_STXWI, "stw%W[ %dr %o16 ] , %i32", "* ( u32 * ) ( %dr %o16 ) = %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ST|BPF_SIZE_W|BPF_MODE_MEM},
  {BPF_INSN_STXDWI, "stdw%W[ %dr %o16 ] , %i32", "* ( u64 * ) ( %dr %o16 ) = %i32",
   BPF_V1, BPF_CODE, BPF_CLASS_ST|BPF_SIZE_DW|BPF_MODE_MEM},

  /* Compare-and-jump instructions (reg OP reg).  */
  {BPF_INSN_JAR, "ja%W%d16", "goto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JA|BPF_SRC_K},
  {BPF_INSN_JEQR, "jeq%W%dr , %sr , %d16", "if%w%dr == %sr%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JEQ|BPF_SRC_X},
  {BPF_INSN_JGTR, "jgt%W%dr , %sr , %d16", "if%w%dr > %sr%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JGT|BPF_SRC_X},
  {BPF_INSN_JSGTR, "jsgt%W%dr, %sr , %d16", "if%w%dr s> %sr%wgoto%w%d16",
      BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JSGT|BPF_SRC_X},
  {BPF_INSN_JGER, "jge%W%dr , %sr , %d16", "if%w%dr >= %sr%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JGE|BPF_SRC_X},
  {BPF_INSN_JSGER, "jsge%W%dr , %sr , %d16", "if%w%dr s>= %sr%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JSGE|BPF_SRC_X},
  {BPF_INSN_JLTR, "jlt%W%dr , %sr , %d16", "if%w%dr < %sr%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JLT|BPF_SRC_X},
  {BPF_INSN_JSLTR, "jslt%W%dr , %sr , %d16", "if%w%dr s< %sr%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JSLT|BPF_SRC_X},
  {BPF_INSN_JLER, "jle%W%dr , %sr , %d16", "if%w%dr <= %sr%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JLE|BPF_SRC_X},
  {BPF_INSN_JSLER, "jsle%W%dr , %sr , %d16", "if%w%dr s<= %sr%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JSLE|BPF_SRC_X},
  {BPF_INSN_JSETR, "jset%W%dr , %sr , %d16", "if%w%dr & %sr%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JSET|BPF_SRC_X},
  {BPF_INSN_JNER, "jne%W%dr , %sr , %d16", "if%w%dr != %sr%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JNE|BPF_SRC_X},
  {BPF_INSN_CALLR, "call%W%dr", "callx%w%dr",
   BPF_XBPF, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_CALL|BPF_SRC_X},
  {BPF_INSN_CALL, "call%W%d32", "call%w%d32",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_CALL|BPF_SRC_K},
  {BPF_INSN_EXIT, "exit", "exit",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_EXIT|BPF_SRC_K},

  /* Compare-and-jump instructions (reg OP imm).  */
  {BPF_INSN_JEQI, "jeq%W%dr , %i32 , %d16", "if%w%dr == %i32%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JEQ|BPF_SRC_K},
  {BPF_INSN_JGTI, "jgt%W%dr , %i32 , %d16", "if%w%dr > %i32%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JGT|BPF_SRC_K},
  {BPF_INSN_JSGTI, "jsgt%W%dr, %i32 , %d16", "if%w%dr s> %i32%wgoto%w%d16",
      BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JSGT|BPF_SRC_K},
  {BPF_INSN_JGEI, "jge%W%dr , %i32 , %d16", "if%w%dr >= %i32%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JGE|BPF_SRC_K},
  {BPF_INSN_JSGEI, "jsge%W%dr , %i32 , %d16", "if%w%dr s>= %i32%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JSGE|BPF_SRC_K},
  {BPF_INSN_JLTI, "jlt%W%dr , %i32 , %d16", "if%w%dr < %i32%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JLT|BPF_SRC_K},
  {BPF_INSN_JSLTI, "jslt%W%dr , %i32, %d16", "if%w%dr s< %i32%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JSLT|BPF_SRC_K},
  {BPF_INSN_JLEI, "jle%W%dr , %i32 , %d16", "if%w%dr <= %i32%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JLE|BPF_SRC_K},
  {BPF_INSN_JSLEI, "jsle%W%dr , %i32 , %d16", "if%w%dr s<= %i32%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JSLE|BPF_SRC_K},
  {BPF_INSN_JSETI, "jset%W%dr , %i32 , %d16", "if%w%dr & %i32%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JSET|BPF_SRC_K},
  {BPF_INSN_JNEI, "jne%W%dr , %i32 , %d16", "if%w%dr != %i32%wgoto%w%d16",
   BPF_V1, BPF_CODE, BPF_CLASS_JMP|BPF_CODE_JNE|BPF_SRC_K},

  /* 32-bit jump-always.  */
  {BPF_INSN_JAL, "jal%W%d32", "gotol%w%d32",
   BPF_V4, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JA|BPF_SRC_K},

  /* 32-bit compare-and-jump instructions (reg OP reg).  */
  {BPF_INSN_JEQ32R, "jeq32%W%dr , %sr , %d16", "if%w%dw == %sw%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JEQ|BPF_SRC_X},
  {BPF_INSN_JGT32R, "jgt32%W%dr , %sr , %d16", "if%w%dw > %sw%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JGT|BPF_SRC_X},
  {BPF_INSN_JSGT32R, "jsgt32%W%dr, %sr , %d16", "if%w%dw s> %sw%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JSGT|BPF_SRC_X},
  {BPF_INSN_JGE32R, "jge32%W%dr , %sr , %d16", "if%w%dw >= %sw%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JGE|BPF_SRC_X},
  {BPF_INSN_JSGE32R, "jsge32%W%dr , %sr , %d16", "if%w%dw s>= %sw%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JSGE|BPF_SRC_X},
  {BPF_INSN_JLT32R, "jlt32%W%dr , %sr , %d16", "if%w%dw < %sw%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JLT|BPF_SRC_X},
  {BPF_INSN_JSLT32R, "jslt32%W%dr , %sr , %d16", "if%w%dw s< %sw%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JSLT|BPF_SRC_X},
  {BPF_INSN_JLE32R, "jle32%W%dr , %sr , %d16", "if%w%dw <= %sw%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JLE|BPF_SRC_X},
  {BPF_INSN_JSLE32R, "jsle32%W%dr , %sr , %d16", "if%w%dw s<= %sw%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JSLE|BPF_SRC_X},
  {BPF_INSN_JSET32R, "jset32%W%dr , %sr , %d16", "if%w%dw & %sw%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JSET|BPF_SRC_X},
  {BPF_INSN_JNE32R, "jne32%W%dr , %sr , %d16", "if%w%dw != %sw%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JNE|BPF_SRC_X},

  /* 32-bit compare-and-jump instructions (reg OP imm).  */
  {BPF_INSN_JEQ32I, "jeq32%W%dr , %i32 , %d16", "if%w%dw == %i32%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JEQ|BPF_SRC_K},
  {BPF_INSN_JGT32I, "jgt32%W%dr , %i32 , %d16", "if%w%dw > %i32%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JGT|BPF_SRC_K},
  {BPF_INSN_JSGT32I, "jsgt32%W%dr, %i32 , %d16", "if%w%dw s> %i32%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JSGT|BPF_SRC_K},
  {BPF_INSN_JGE32I, "jge32%W%dr , %i32 , %d16", "if%w%dw >= %i32%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JGE|BPF_SRC_K},
  {BPF_INSN_JSGE32I, "jsge32%W%dr , %i32 , %d16", "if%w%dw s>= %i32%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JSGE|BPF_SRC_K},
  {BPF_INSN_JLT32I, "jlt32%W%dr , %i32 , %d16", "if%w%dw < %i32%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JLT|BPF_SRC_K},
  {BPF_INSN_JSLT32I, "jslt32%W%dr , %i32, %d16", "if%w%dw s< %i32%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JSLT|BPF_SRC_K},
  {BPF_INSN_JLE32I, "jle32%W%dr , %i32 , %d16", "if%w%dw <= %i32%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JLE|BPF_SRC_K},
  {BPF_INSN_JSLE32I, "jsle32%W%dr , %i32 , %d16", "if%w%dw s<= %i32%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JSLE|BPF_SRC_K},
  {BPF_INSN_JSET32I, "jset32%W%dr , %i32 , %d16", "if%w%dw & %i32%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JSET|BPF_SRC_K},
  {BPF_INSN_JNE32I, "jne32%W%dr , %i32 , %d16", "if%w%dw != %i32%wgoto%w%d16",
   BPF_V3, BPF_CODE, BPF_CLASS_JMP32|BPF_CODE_JNE|BPF_SRC_K},

  /* Atomic instructions.  */
  {BPF_INSN_AADD, "aadd%W[ %dr %o16 ] , %sr", "lock%w* ( u64 * ) ( %dr %o16 ) += %sr",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_DW|BPF_MODE_ATOMIC|BPF_IMM32_AADD},
  {BPF_INSN_AOR, "aor%W[ %dr %o16 ] , %sr", "lock%w* ( u64 * ) ( %dr %o16 ) |= %sr",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_DW|BPF_MODE_ATOMIC|BPF_IMM32_AOR},
  {BPF_INSN_AAND, "aand%W[ %dr %o16 ] , %sr", "lock%w* ( u64 * ) ( %dr %o16 ) &= %sr",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_DW|BPF_MODE_ATOMIC|BPF_IMM32_AAND},
  {BPF_INSN_AXOR, "axor%W[ %dr %o16 ] , %sr", "lock%w* ( u64 * ) ( %dr %o16 ) ^= %sr",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_DW|BPF_MODE_ATOMIC|BPF_IMM32_AXOR},

  /* Atomic instructions with fetching.  */
  {BPF_INSN_AFADD, "afadd%W[ %dr %o16 ] , %sr", "%sr = atomic_fetch_add ( ( u64 * ) ( %dr %o16 ) , %sr )",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_DW|BPF_MODE_ATOMIC|BPF_IMM32_AFADD},
  {BPF_INSN_AFOR, "afor%W[ %dr %o16 ] , %sr", "%sr = atomic_fetch_or ( ( u64 * ) ( %dr %o16 ) , %sr )",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_DW|BPF_MODE_ATOMIC|BPF_IMM32_AFOR},
  {BPF_INSN_AFAND, "afand%W[ %dr %o16 ] , %sr", "%sr = atomic_fetch_and ( ( u64 * ) ( %dr %o16 ) , %sr )",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_DW|BPF_MODE_ATOMIC|BPF_IMM32_AFAND},
  {BPF_INSN_AFXOR, "afxor%W[ %dr %o16 ] , %sr", "%sr = atomic_fetch_xor ( ( u64 * ) ( %dr %o16 ) , %sr )",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_DW|BPF_MODE_ATOMIC|BPF_IMM32_AFXOR},

  /* Atomic instructions (32-bit.) */
  {BPF_INSN_AADD32, "aadd32%W[ %dr %o16 ] , %sr", "lock%w* ( u32 * ) ( %dr %o16 ) += %sw",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_W|BPF_MODE_ATOMIC|BPF_IMM32_AADD},
  {BPF_INSN_AOR32, "aor32%W[ %dr %o16 ] , %sr", "lock%w* ( u32 * ) ( %dr %o16 ) |= %sw",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_W|BPF_MODE_ATOMIC|BPF_IMM32_AOR},
  {BPF_INSN_AAND32, "aand32%W[ %dr %o16 ] , %sr", "lock%w* ( u32 * ) ( %dr %o16 ) &= %sw",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_W|BPF_MODE_ATOMIC|BPF_IMM32_AAND},
  {BPF_INSN_AXOR32, "axor32%W[ %dr %o16 ] , %sr", "lock%w* ( u32 * ) ( %dr %o16 ) ^= %sw",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_W|BPF_MODE_ATOMIC|BPF_IMM32_AXOR},

  /* Atomic instructions with fetching (32-bit.) */
  {BPF_INSN_AFADD32, "afadd32%W[ %dr %o16 ] , %sr", "%sw = atomic_fetch_add ( ( u32 * ) ( %dr %o16 ) , %sw )",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_W|BPF_MODE_ATOMIC|BPF_IMM32_AFADD},
  {BPF_INSN_AFOR32, "afor32%W[ %dr %o16 ] , %sr", "%sw = atomic_fetch_or ( ( u32 * ) ( %dr %o16 ) , %sw )",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_W|BPF_MODE_ATOMIC|BPF_IMM32_AFOR},
  {BPF_INSN_AFAND32, "afand32%W[ %dr %o16 ] , %sr", "%sw = atomic_fetch_and ( ( u32 * ) ( %dr %o16 ) , %sw )",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_W|BPF_MODE_ATOMIC|BPF_IMM32_AFAND},
  {BPF_INSN_AFXOR32, "afxor32%W[ %dr %o16 ] , %sr", "%sw = atomic_fetch_xor ( ( u32 * ) ( %dr %o16 ) , %sw )",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_W|BPF_MODE_ATOMIC|BPF_IMM32_AFXOR},

  /* Atomic compare-and-swap, atomic exchange.  */
  {BPF_INSN_ACMP, "acmp%W[ %dr %o16 ] , %sr", "r0 = cmpxchg_64 ( %dr %o16 , r0 , %sr )",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_DW|BPF_MODE_ATOMIC|BPF_IMM32_ACMP},
  {BPF_INSN_AXCHG, "axchg%W[ %dr %o16 ] , %sr", "%sr = xchg_64 ( %dr %o16 , %sr )",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_DW|BPF_MODE_ATOMIC|BPF_IMM32_AXCHG},

  /* Atomic compare-and-swap, atomic exchange (32-bit).  */
  {BPF_INSN_ACMP32, "acmp32%W[ %dr %o16 ], %sr", "w0 = cmpxchg32_32 ( %dr %o16 , w0 , %sw )",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_W|BPF_MODE_ATOMIC|BPF_IMM32_ACMP},
  {BPF_INSN_AXCHG32, "axchg32%W[ %dr %o16 ], %sr", "%sw = xchg32_32 ( %dr %o16 , %sw )",
   BPF_V3, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_W|BPF_MODE_ATOMIC|BPF_IMM32_AXCHG},

  /* Old versions of aadd and aadd32.  */
  {BPF_INSN_AADD, "xadddw%W[ %dr %o16 ] , %sr", "* ( u64 * ) ( %dr %o16 ) += %sr",
   BPF_V1, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_DW|BPF_MODE_ATOMIC|BPF_IMM32_AADD},
  {BPF_INSN_AADD32, "xaddw%W[ %dr %o16 ] , %sr", "* ( u32 * ) ( %dr %o16 ) += %sr",
   BPF_V1, BPF_CODE|BPF_IMM32, BPF_CLASS_STX|BPF_SIZE_W|BPF_MODE_ATOMIC|BPF_IMM32_AADD},

  /* the brkpt instruction is used by the BPF simulator and it doesn't
     really belong to the BPF instruction set.  */
  {BPF_INSN_BRKPT, "brkpt", "brkpt",
   BPF_XBPF, BPF_CODE, BPF_CLASS_ALU|BPF_SRC_X|BPF_CODE_NEG},

  /* Sentinel.  */
  {BPF_NOINSN, NULL, NULL, 0, 0UL, 0UL},
};

static bpf_insn_word
bpf_handle_endianness (bpf_insn_word word, enum bpf_endian endian)
{
  if (endian == BPF_ENDIAN_LITTLE)
    {
      /* Endianness groups: 8 | 4 | 4 | 16 | 32  */

      bpf_insn_word code = (word >> 56) & 0xff;
      bpf_insn_word dst = (word >> 48) & 0xf;
      bpf_insn_word src = (word >> 52) & 0xf;
      bpf_insn_word offset16 = (word >> 32) & 0xffff;
      bpf_insn_word imm32 = word & 0xffffffff;

      return ((code << 56)
              | dst << 52
              | src << 48
              | (offset16 & 0xff) << 40
              | ((offset16 >> 8) & 0xff) << 32
              | (imm32 & 0xff) << 24
              | ((imm32 >> 8) & 0xff) << 16
              | ((imm32 >> 16) & 0xff) << 8
              | ((imm32 >> 24) & 0xff));
    }

  return word;
}

const struct bpf_opcode *
bpf_match_insn (bpf_insn_word word,
                enum bpf_endian endian,
                int version)
{
  unsigned int i = 0;

  while (bpf_opcodes[i].normal != NULL)
    {
      bpf_insn_word cword
        = bpf_handle_endianness (word, endian);

      /* Attempt match using mask and opcodes.  */
      if (bpf_opcodes[i].version <= version
          && (cword & bpf_opcodes[i].mask) == bpf_opcodes[i].opcode)
        return &bpf_opcodes[i];
      i++;
    }

  /* No maching instruction found.  */
  return NULL;
}

uint8_t
bpf_extract_src (bpf_insn_word word, enum bpf_endian endian)
{
  word = bpf_handle_endianness (word, endian);
  return (uint8_t) ((word >> 48) & 0xf);
}

uint8_t
bpf_extract_dst (bpf_insn_word word, enum bpf_endian endian)
{
  word = bpf_handle_endianness (word, endian);
  return (uint8_t) ((word >> 52) & 0xf);
}

int16_t
bpf_extract_offset16 (bpf_insn_word word, enum bpf_endian endian)
{
  word = bpf_handle_endianness (word, endian);
  return (int16_t) ((word >> 32) & 0xffff);
}

int32_t
bpf_extract_imm32 (bpf_insn_word word, enum bpf_endian endian)
{
  word = bpf_handle_endianness (word, endian);
  return (int32_t) (word & 0xffffffff);
}

int64_t
bpf_extract_imm64 (bpf_insn_word word1, bpf_insn_word word2,
                   enum bpf_endian endian)
{
  word1 = bpf_handle_endianness (word1, endian);
  word2 = bpf_handle_endianness (word2, endian);
  return (int64_t) (((word2 & 0xffffffff) << 32) | (word1 & 0xffffffff));
}

const struct bpf_opcode *
bpf_get_opcode (unsigned int index)
{
  unsigned int i = 0;

  while (bpf_opcodes[i].normal != NULL && i < index)
    ++i;
  return (bpf_opcodes[i].normal == NULL
          ? NULL
          : &bpf_opcodes[i]);
}
