/* bpf.h - BPF opcode list for binutils.
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

#ifndef _BPF_H_
#define _BPF_H_

#include <stdint.h>

/* The BPF ISA has little-endian and big-endian variants.  */

enum bpf_endian
{
  BPF_ENDIAN_LITTLE,
  BPF_ENDIAN_BIG
};

/* Most BPF instructions are conformed by a single 64-bit instruction
   word.  The lddw instruction is conformed by two consecutive 64-bit
   instruction words.  */

typedef uint64_t bpf_insn_word;

/* There are several versions of the BPF ISA.  */

#define BPF_V1 0x1
#define BPF_V2 0x2
#define BPF_V3 0x3
#define BPF_V4 0x4
#define BPF_XBPF 0xf

/* Masks for the several instruction fields in a BPF instruction.
   These assume big-endian BPF instructions.  */

#define BPF_CODE     0xff00000000000000UL
#define BPF_REGS     0x00ff000000000000UL
#define BPF_DST      0x00f0000000000000UL
#define BPF_SRC      0x000f000000000000UL
#define BPF_OFFSET16 0x0000ffff00000000UL
#define BPF_IMM32    0x00000000ffffffffUL

/* The BPF opcode instruction field is eight bits long and its
   interpretation depends on the instruction class.

   For arithmetic and jump instructions the 8-bit opcode field is
   subdivided in:

     op-code:4 op-src:1 op-class:3

   For load/store instructions, the 8-bit opcode field is subdivided
   in:

     op-mode:3 op-size:2 op-class:3

   All the constants defined below are to be applied on the first
   64-bit word of a BPF instruction.  Please define them assuming
   big-endian instructions; the matching and writing routines using
   the instruction table know how to handle the endianness groups.  */

#define BPF_SRC_X ((uint64_t)0x08 << 56)
#define BPF_SRC_K ((uint64_t)0x00 << 56)

#define BPF_CODE_ADD ((uint64_t)0x00 << 56)
#define BPF_CODE_SUB ((uint64_t)0x10 << 56)
#define BPF_CODE_MUL ((uint64_t)0x20 << 56)
#define BPF_CODE_DIV ((uint64_t)0x30 << 56)
#define BPF_CODE_OR  ((uint64_t)0x40 << 56)
#define BPF_CODE_AND ((uint64_t)0x50 << 56)
#define BPF_CODE_LSH ((uint64_t)0x60 << 56)
#define BPF_CODE_RSH ((uint64_t)0x70 << 56)
#define BPF_CODE_NEG ((uint64_t)0x80 << 56)
#define BPF_CODE_MOD ((uint64_t)0x90 << 56)
#define BPF_CODE_XOR ((uint64_t)0xa0 << 56)
#define BPF_CODE_MOV ((uint64_t)0xb0 << 56)
#define BPF_CODE_ARSH ((uint64_t)0xc0 << 56)
#define BPF_CODE_END ((uint64_t)0xd0 << 56)

#define BPF_CODE_JA   ((uint64_t)0x00 << 56)
#define BPF_CODE_JEQ  ((uint64_t)0x10 << 56)
#define BPF_CODE_JGT  ((uint64_t)0x20 << 56)
#define BPF_CODE_JGE  ((uint64_t)0x30 << 56)
#define BPF_CODE_JSET ((uint64_t)0x40 << 56)
#define BPF_CODE_JNE  ((uint64_t)0x50 << 56)
#define BPF_CODE_JSGT ((uint64_t)0x60 << 56)
#define BPF_CODE_JSGE ((uint64_t)0x70 << 56)
#define BPF_CODE_CALL ((uint64_t)0x80 << 56)
#define BPF_CODE_EXIT ((uint64_t)0x90 << 56)
#define BPF_CODE_JLT  ((uint64_t)0xa0 << 56)
#define BPF_CODE_JLE  ((uint64_t)0xb0 << 56)
#define BPF_CODE_JSLT ((uint64_t)0xc0 << 56)
#define BPF_CODE_JSLE ((uint64_t)0xd0 << 56)

#define BPF_MODE_IMM  ((uint64_t)0x00 << 56)
#define BPF_MODE_ABS  ((uint64_t)0x20 << 56)
#define BPF_MODE_IND  ((uint64_t)0x40 << 56)
#define BPF_MODE_MEM  ((uint64_t)0x60 << 56)
#define BPF_MODE_ATOMIC ((uint64_t)0xc0 << 56)
#define BPF_MODE_SMEM ((uint64_t)0x80 << 56)

#define BPF_SIZE_W  ((uint64_t)0x00 << 56)
#define BPF_SIZE_H  ((uint64_t)0x08 << 56)
#define BPF_SIZE_B  ((uint64_t)0x10 << 56)
#define BPF_SIZE_DW ((uint64_t)0x18 << 56)

#define BPF_CLASS_LD    ((uint64_t)0x00 << 56)
#define BPF_CLASS_LDX   ((uint64_t)0x01 << 56)
#define BPF_CLASS_ST    ((uint64_t)0x02 << 56)
#define BPF_CLASS_STX   ((uint64_t)0x03 << 56)
#define BPF_CLASS_ALU   ((uint64_t)0x04 << 56)
#define BPF_CLASS_JMP   ((uint64_t)0x05 << 56)
#define BPF_CLASS_JMP32 ((uint64_t)0x06 << 56)
#define BPF_CLASS_ALU64 ((uint64_t)0x07 << 56)

/* Certain instructions (ab)use other instruction fields as opcodes,
   even if these are multi-byte or infra-byte.  Bleh.  */

#define BPF_OFFSET16_SDIVMOD ((uint64_t)0x1 << 32)
#define BPF_OFFSET16_MOVS8 ((uint64_t)8 << 32)
#define BPF_OFFSET16_MOVS16 ((uint64_t)16 << 32)
#define BPF_OFFSET16_MOVS32 ((uint64_t)32 << 32)

#define BPF_IMM32_END16 ((uint64_t)0x00000010)
#define BPF_IMM32_END32 ((uint64_t)0x00000020)
#define BPF_IMM32_END64 ((uint64_t)0x00000040)

#define BPF_IMM32_BSWAP16 ((uint64_t)0x00000010)
#define BPF_IMM32_BSWAP32 ((uint64_t)0x00000020)
#define BPF_IMM32_BSWAP64 ((uint64_t)0x00000040)

#define BPF_IMM32_AADD ((uint64_t)0x00000000)
#define BPF_IMM32_AOR  ((uint64_t)0x00000040)
#define BPF_IMM32_AAND ((uint64_t)0x00000050)
#define BPF_IMM32_AXOR ((uint64_t)0x000000a0)
#define BPF_IMM32_AFADD ((uint64_t)0x00000001)
#define BPF_IMM32_AFOR  ((uint64_t)0x00000041)
#define BPF_IMM32_AFAND ((uint64_t)0x00000051)
#define BPF_IMM32_AFXOR ((uint64_t)0x000000a1)
#define BPF_IMM32_AXCHG ((uint64_t)0x000000e1)
#define BPF_IMM32_ACMP  ((uint64_t)0x000000f1)

/* Unique identifiers for BPF instructions.  */

enum bpf_insn_id
{
  BPF_NOINSN = 0,
  /* 64-bit load instruction.  */
  BPF_INSN_LDDW,
  /* ALU instructions.  */
  BPF_INSN_ADDR, BPF_INSN_ADDI, BPF_INSN_SUBR, BPF_INSN_SUBI,
  BPF_INSN_MULR, BPF_INSN_MULI, BPF_INSN_SDIVR, BPF_INSN_SDIVI,
  BPF_INSN_SMODR, BPF_INSN_SMODI, BPF_INSN_DIVR, BPF_INSN_DIVI,
  BPF_INSN_MODR, BPF_INSN_MODI, BPF_INSN_ORR, BPF_INSN_ORI,
  BPF_INSN_ANDR, BPF_INSN_ANDI, BPF_INSN_XORR, BPF_INSN_XORI,
  BPF_INSN_NEGR, BPF_INSN_LSHR, BPF_INSN_LSHI,
  BPF_INSN_RSHR, BPF_INSN_RSHI, BPF_INSN_ARSHR, BPF_INSN_ARSHI,
  BPF_INSN_MOVS8R, BPF_INSN_MOVS16R, BPF_INSN_MOVS32R,
  BPF_INSN_MOVR, BPF_INSN_MOVI,
  /* ALU32 instructions.  */
  BPF_INSN_ADD32R, BPF_INSN_ADD32I, BPF_INSN_SUB32R, BPF_INSN_SUB32I,
  BPF_INSN_MUL32R, BPF_INSN_MUL32I, BPF_INSN_SDIV32R, BPF_INSN_SDIV32I,
  BPF_INSN_SMOD32R, BPF_INSN_SMOD32I, BPF_INSN_DIV32R, BPF_INSN_DIV32I,
  BPF_INSN_MOD32R, BPF_INSN_MOD32I, BPF_INSN_OR32R, BPF_INSN_OR32I,
  BPF_INSN_AND32R, BPF_INSN_AND32I, BPF_INSN_XOR32R, BPF_INSN_XOR32I,
  BPF_INSN_NEG32R, BPF_INSN_LSH32R, BPF_INSN_LSH32I,
  BPF_INSN_RSH32R, BPF_INSN_RSH32I, BPF_INSN_ARSH32R, BPF_INSN_ARSH32I,
  BPF_INSN_MOVS328R, BPF_INSN_MOVS3216R, BPF_INSN_MOVS3232R,
  BPF_INSN_MOV32R, BPF_INSN_MOV32I,
  /* Byte swap instructions.  */
  BPF_INSN_BSWAP16, BPF_INSN_BSWAP32, BPF_INSN_BSWAP64,
  /* Endianness conversion instructions.  */
  BPF_INSN_ENDLE16, BPF_INSN_ENDLE32, BPF_INSN_ENDLE64,
  BPF_INSN_ENDBE16, BPF_INSN_ENDBE32, BPF_INSN_ENDBE64,
  /* Absolute load instructions.  */
  BPF_INSN_LDABSB, BPF_INSN_LDABSH, BPF_INSN_LDABSW, BPF_INSN_LDABSDW,
  /* Indirect load instructions.  */
  BPF_INSN_LDINDB, BPF_INSN_LDINDH, BPF_INSN_LDINDW, BPF_INSN_LDINDDW,
  /* Generic load instructions (to register.)  */
  BPF_INSN_LDXB, BPF_INSN_LDXH, BPF_INSN_LDXW, BPF_INSN_LDXDW,
  /* Generic signed load instructions.  */
  BPF_INSN_LDXSB, BPF_INSN_LDXSH, BPF_INSN_LDXSW, BPF_INSN_LDXSDW,
  /* Generic store instructions (from register.)  */
  BPF_INSN_STXBR, BPF_INSN_STXHR, BPF_INSN_STXWR, BPF_INSN_STXDWR,
  BPF_INSN_STXBI, BPF_INSN_STXHI, BPF_INSN_STXWI, BPF_INSN_STXDWI,
  /* Compare-and-jump instructions (reg OP reg.)  */
  BPF_INSN_JAR, BPF_INSN_JEQR, BPF_INSN_JGTR, BPF_INSN_JSGTR,
  BPF_INSN_JGER, BPF_INSN_JSGER, BPF_INSN_JLTR, BPF_INSN_JSLTR,
  BPF_INSN_JSLER, BPF_INSN_JLER, BPF_INSN_JSETR, BPF_INSN_JNER,
  BPF_INSN_CALLR, BPF_INSN_CALL, BPF_INSN_EXIT,
  /* Compare-and-jump instructions (reg OP imm.)  */
  BPF_INSN_JEQI, BPF_INSN_JGTI, BPF_INSN_JSGTI,
  BPF_INSN_JGEI, BPF_INSN_JSGEI, BPF_INSN_JLTI, BPF_INSN_JSLTI,
  BPF_INSN_JSLEI, BPF_INSN_JLEI, BPF_INSN_JSETI, BPF_INSN_JNEI,
  /* jump-always with 32-bit offset.  */
  BPF_INSN_JAL,
  /* 32-bit compare-and-jump instructions (reg OP reg.)  */
  BPF_INSN_JEQ32R, BPF_INSN_JGT32R, BPF_INSN_JSGT32R,
  BPF_INSN_JGE32R, BPF_INSN_JSGE32R, BPF_INSN_JLT32R, BPF_INSN_JSLT32R,
  BPF_INSN_JSLE32R, BPF_INSN_JLE32R, BPF_INSN_JSET32R, BPF_INSN_JNE32R,
  /* 32-bit compare-and-jump instructions (reg OP imm.)  */
  BPF_INSN_JEQ32I, BPF_INSN_JGT32I, BPF_INSN_JSGT32I,
  BPF_INSN_JGE32I, BPF_INSN_JSGE32I, BPF_INSN_JLT32I, BPF_INSN_JSLT32I,
  BPF_INSN_JSLE32I, BPF_INSN_JLE32I, BPF_INSN_JSET32I, BPF_INSN_JNE32I,
  /* Atomic instructions.  */
  BPF_INSN_AADD, BPF_INSN_AOR, BPF_INSN_AAND, BPF_INSN_AXOR,
  /* Atomic instructions with fetching.  */
  BPF_INSN_AFADD, BPF_INSN_AFOR, BPF_INSN_AFAND, BPF_INSN_AFXOR,
  /* Atomic instructions (32-bit.)  */
  BPF_INSN_AADD32, BPF_INSN_AOR32, BPF_INSN_AAND32, BPF_INSN_AXOR32,
  /* Atomic instructions with fetching (32-bit.)  */
  BPF_INSN_AFADD32, BPF_INSN_AFOR32, BPF_INSN_AFAND32, BPF_INSN_AFXOR32,
  /* Atomic compare-and-swap, atomic exchange.  */
  BPF_INSN_ACMP, BPF_INSN_AXCHG,
  /* Atomic compare-and-swap, atomic exchange (32-bit).  */
  BPF_INSN_ACMP32, BPF_INSN_AXCHG32,
  /* GNU simulator specific instruction.  */
  BPF_INSN_BRKPT,
};

/* Entry for a BPF instruction in the opcodes table.  */

struct bpf_opcode
{
  /* Unique numerical code for the instruction.  */
  enum bpf_insn_id id;

  /* The instruction template defines both the syntax of the
     instruction and the set of the different operands that appear in
     the instruction.

     Tags:
     %% - literal %.
     %dr - destination 64-bit register.
     %dw - destination 32-bit register.
     %sr - source 64-bit register.
     %sw - source 32-bit register.
     %d32 - 32-bit signed displacement (in 64-bit words minus one.)
     %d16 - 16-bit signed displacement (in 64-bit words minus one.)
     %o16 - 16-bit signed offset (in bytes.)
     %i32 - 32-bit signed immediate.
     %I32 - Like %i32.
     %i64 - 64-bit signed immediate.
     %w - expect zero or more white spaces and print a single space.
     %W - expect one or more white spaces and print a single space.

     When parsing and printing %o16 and %I32 (but not %i32) an
     explicit sign is always expected and included.  Therefore, to
     denote something like `[%r3 + 10]', please use a template like `[
     %sr %o16]' instead of `[ %sr + %o16 ]'.
     
     If %dr, %dw, %sr or %sw are found multiple times in a template,
     they refer to the same register, i.e. `%rd = le64 %rd' denotes
     `r2 = le64 r2', but not `r2 = le64 r1'.

     If %i64 appears in a template then the instruction is 128-bits
     long and composed by two consecutive 64-bit instruction words.

     A white space character means to expect zero or more white
     spaces, and to print no space.

     There are two templates defined per instruction, corresponding to
     two used different dialects: a "normal" assembly-like syntax and
     a "pseudo-c" syntax.  Some toolchains support just one of these
     dialects.  The GNU Toolchain supports both.  */
  const char *normal;
  const char *pseudoc;

  /* The version that introduced this instruction.  Instructions are
     generally not removed once they get introduced.  */
  uint8_t version;

  /* Maks marking the opcode fields in the instruction, and the
     opcodes characterizing it.

     In multi-word instructions these apply to the first word in the
     instruction.  Note that these values assumes big-endian
     instructions; code using these field must be aware of the
     endianness groups to which BPF instructions must conform to and
     DTRT.  */
  bpf_insn_word mask;
  bpf_insn_word opcode;
};

/* Try to match a BPF instruction given its first instruction word.
   If no matching instruction is found, return NULL.  */

const struct bpf_opcode *bpf_match_insn (bpf_insn_word word,
                                         enum bpf_endian endian,
                                         int version);

/* Operand extractors.

   These all get big-endian instruction words.  Note how the extractor
   for 64-bit signed immediates requires two instruction words.  */

uint8_t bpf_extract_src (bpf_insn_word word, enum bpf_endian endian);
uint8_t bpf_extract_dst (bpf_insn_word word, enum bpf_endian endian);
int16_t bpf_extract_offset16 (bpf_insn_word word, enum bpf_endian endian);
int32_t bpf_extract_imm32 (bpf_insn_word word, enum bpf_endian endian);
int64_t bpf_extract_imm64 (bpf_insn_word word1, bpf_insn_word word2,
                           enum bpf_endian endian);

/* Get the opcode occupying the INDEX position in the opcodes table.
   The INDEX is zero based.  If the provided index overflows the
   opcodes table then NULL is returned.  */

const struct bpf_opcode *bpf_get_opcode (unsigned int index);

#endif /* !_BPF_H_ */
