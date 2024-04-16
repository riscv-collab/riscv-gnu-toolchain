/* Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef ARCH_AARCH64_INSN_H
#define ARCH_AARCH64_INSN_H

extern bool aarch64_debug;

/* Print an "aarch64" debug statement.  */

#define aarch64_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (aarch64_debug, "aarch64", fmt, ##__VA_ARGS__)

/* Support routines for instruction parsing.  */

/* Create a mask of X bits.  */
#define submask(x) ((1L << ((x) + 1)) - 1)

/* Extract the bitfield from OBJ starting at bit ST and ending at bit FN.  */
#define bits(obj,st,fn) (((obj) >> (st)) & submask ((fn) - (st)))

/* Extract bit ST from OBJ.  */
#define bit(obj,st) (((obj) >> (st)) & 1)

/* Extract the signed bitfield from OBJ starting at bit ST and ending at
   bit FN.  The result is sign-extended.  */
#define sbits(obj,st,fn) \
  ((long) (bits(obj,st,fn) | ((long) bit(obj,fn) * ~ submask (fn - st))))

/* Prologue analyzer helper macros.  */

/* Is the instruction "bti"?  */
#define IS_BTI(instruction) ((instruction & 0xffffff3f) == 0xd503241f)

/* List of opcodes that we need for building the jump pad and relocating
   an instruction.  */

enum aarch64_opcodes
{
  /* B              0001 01ii iiii iiii iiii iiii iiii iiii */
  /* BL             1001 01ii iiii iiii iiii iiii iiii iiii */
  /* B.COND         0101 0100 iiii iiii iiii iiii iii0 cccc */
  /* CBZ            s011 0100 iiii iiii iiii iiii iiir rrrr */
  /* CBNZ           s011 0101 iiii iiii iiii iiii iiir rrrr */
  /* TBZ            b011 0110 bbbb biii iiii iiii iiir rrrr */
  /* TBNZ           b011 0111 bbbb biii iiii iiii iiir rrrr */
  B               = 0x14000000,
  BL              = 0x80000000 | B,
  BCOND           = 0x40000000 | B,
  CBZ             = 0x20000000 | B,
  CBNZ            = 0x21000000 | B,
  TBZ             = 0x36000000 | B,
  TBNZ            = 0x37000000 | B,
  /* BR             1101 0110 0001 1111 0000 00rr rrr0 0000 */
  /* BLR            1101 0110 0011 1111 0000 00rr rrr0 0000 */
  BR              = 0xd61f0000,
  BLR             = 0xd63f0000,
  /* RET            1101 0110 0101 1111 0000 00rr rrr0 0000 */
  RET             = 0xd65f0000,
  /* STP            s010 100o o0ii iiii irrr rrrr rrrr rrrr */
  /* LDP            s010 100o o1ii iiii irrr rrrr rrrr rrrr */
  /* STP (SIMD&VFP) ss10 110o o0ii iiii irrr rrrr rrrr rrrr */
  /* LDP (SIMD&VFP) ss10 110o o1ii iiii irrr rrrr rrrr rrrr */
  STP             = 0x28000000,
  LDP             = 0x28400000,
  STP_SIMD_VFP    = 0x04000000 | STP,
  LDP_SIMD_VFP    = 0x04000000 | LDP,
  /* STR            ss11 100o 00xi iiii iiii xxrr rrrr rrrr */
  /* LDR            ss11 100o 01xi iiii iiii xxrr rrrr rrrr */
  /* LDRSW          1011 100o 10xi iiii iiii xxrr rrrr rrrr */
  STR             = 0x38000000,
  LDR             = 0x00400000 | STR,
  LDRSW           = 0x80800000 | STR,
  /* LDAXR          ss00 1000 0101 1111 1111 11rr rrrr rrrr */
  LDAXR           = 0x085ffc00,
  /* STXR           ss00 1000 000r rrrr 0111 11rr rrrr rrrr */
  STXR            = 0x08007c00,
  /* STLR           ss00 1000 1001 1111 1111 11rr rrrr rrrr */
  STLR            = 0x089ffc00,
  /* MOV            s101 0010 1xxi iiii iiii iiii iiir rrrr */
  /* MOVK           s111 0010 1xxi iiii iiii iiii iiir rrrr */
  MOV             = 0x52800000,
  MOVK            = 0x20000000 | MOV,
  /* ADD            s00o ooo1 xxxx xxxx xxxx xxxx xxxx xxxx */
  /* SUB            s10o ooo1 xxxx xxxx xxxx xxxx xxxx xxxx */
  /* SUBS           s11o ooo1 xxxx xxxx xxxx xxxx xxxx xxxx */
  ADD             = 0x01000000,
  SUB             = 0x40000000 | ADD,
  SUBS            = 0x20000000 | SUB,
  /* AND            s000 1010 xx0x xxxx xxxx xxxx xxxx xxxx */
  /* ORR            s010 1010 xx0x xxxx xxxx xxxx xxxx xxxx */
  /* ORN            s010 1010 xx1x xxxx xxxx xxxx xxxx xxxx */
  /* EOR            s100 1010 xx0x xxxx xxxx xxxx xxxx xxxx */
  AND             = 0x0a000000,
  ORR             = 0x20000000 | AND,
  ORN             = 0x00200000 | ORR,
  EOR             = 0x40000000 | AND,
  /* LSLV           s001 1010 110r rrrr 0010 00rr rrrr rrrr */
  /* LSRV           s001 1010 110r rrrr 0010 01rr rrrr rrrr */
  /* ASRV           s001 1010 110r rrrr 0010 10rr rrrr rrrr */
  LSLV             = 0x1ac02000,
  LSRV             = 0x00000400 | LSLV,
  ASRV             = 0x00000800 | LSLV,
  /* SBFM           s001 0011 0nii iiii iiii iirr rrrr rrrr */
  SBFM            = 0x13000000,
  /* UBFM           s101 0011 0nii iiii iiii iirr rrrr rrrr */
  UBFM            = 0x40000000 | SBFM,
  /* CSINC          s001 1010 100r rrrr cccc 01rr rrrr rrrr */
  CSINC           = 0x9a800400,
  /* MUL            s001 1011 000r rrrr 0111 11rr rrrr rrrr */
  MUL             = 0x1b007c00,
  /* MSR (register) 1101 0101 0001 oooo oooo oooo ooor rrrr */
  /* MRS            1101 0101 0011 oooo oooo oooo ooor rrrr */
  MSR             = 0xd5100000,
  MRS             = 0x00200000 | MSR,
  /* HINT           1101 0101 0000 0011 0010 oooo ooo1 1111 */
  HINT            = 0xd503201f,
  SEVL            = (5 << 5) | HINT,
  WFE             = (2 << 5) | HINT,
  NOP             = (0 << 5) | HINT,
};

/* List of useful masks.  */
enum aarch64_masks
{
  /* Used for masking out an Rn argument from an opcode.  */
  CLEAR_Rn_MASK = 0xfffffc1f,
};

/* Representation of a general purpose register of the form xN or wN.

   This type is used by emitting functions that take registers as operands.  */

struct aarch64_register
{
  unsigned num;
  int is64;
};

enum aarch64_memory_operand_type
{
  MEMORY_OPERAND_OFFSET,
  MEMORY_OPERAND_PREINDEX,
  MEMORY_OPERAND_POSTINDEX,
};

/* Representation of a memory operand, used for load and store
   instructions.

   The types correspond to the following variants:

   MEMORY_OPERAND_OFFSET:    LDR rt, [rn, #offset]
   MEMORY_OPERAND_PREINDEX:  LDR rt, [rn, #index]!
   MEMORY_OPERAND_POSTINDEX: LDR rt, [rn], #index  */

struct aarch64_memory_operand
{
  /* Type of the operand.  */
  enum aarch64_memory_operand_type type;

  /* Index from the base register.  */
  int32_t index;
};

/* Helper macro to mask and shift a value into a bitfield.  */

#define ENCODE(val, size, offset) \
  ((uint32_t) ((val & ((1ULL << size) - 1)) << offset))

int aarch64_decode_adr (CORE_ADDR addr, uint32_t insn, int *is_adrp,
			unsigned *rd, int32_t *offset);

int aarch64_decode_b (CORE_ADDR addr, uint32_t insn, int *is_bl,
		      int32_t *offset);

int aarch64_decode_bcond (CORE_ADDR addr, uint32_t insn, unsigned *cond,
			  int32_t *offset);

int aarch64_decode_cb (CORE_ADDR addr, uint32_t insn, int *is64,
		       int *is_cbnz, unsigned *rn, int32_t *offset);

int aarch64_decode_tb (CORE_ADDR addr, uint32_t insn, int *is_tbnz,
		       unsigned *bit, unsigned *rt, int32_t *imm);

int aarch64_decode_ldr_literal (CORE_ADDR addr, uint32_t insn, int *is_w,
				int *is64, unsigned *rt, int32_t *offset);

/* Data passed to each method of aarch64_insn_visitor.  */

struct aarch64_insn_data
{
  /* The instruction address.  */
  CORE_ADDR insn_addr;
};

/* Visit different instructions by different methods.  */

struct aarch64_insn_visitor
{
  /* Visit instruction B/BL OFFSET.  */
  void (*b) (const int is_bl, const int32_t offset,
	     struct aarch64_insn_data *data);

  /* Visit instruction B.COND OFFSET.  */
  void (*b_cond) (const unsigned cond, const int32_t offset,
		  struct aarch64_insn_data *data);

  /* Visit instruction CBZ/CBNZ Rn, OFFSET.  */
  void (*cb) (const int32_t offset, const int is_cbnz,
	      const unsigned rn, int is64,
	      struct aarch64_insn_data *data);

  /* Visit instruction TBZ/TBNZ Rt, #BIT, OFFSET.  */
  void (*tb) (const int32_t offset, int is_tbnz,
	      const unsigned rt, unsigned bit,
	      struct aarch64_insn_data *data);

  /* Visit instruction ADR/ADRP Rd, OFFSET.  */
  void (*adr) (const int32_t offset, const unsigned rd,
	       const int is_adrp, struct aarch64_insn_data *data);

  /* Visit instruction LDR/LDRSW Rt, OFFSET.  */
  void (*ldr_literal) (const int32_t offset, const int is_sw,
		       const unsigned rt, const int is64,
		       struct aarch64_insn_data *data);

  /* Visit instruction INSN of other kinds.  */
  void (*others) (const uint32_t insn, struct aarch64_insn_data *data);
};

void aarch64_relocate_instruction (uint32_t insn,
				   const struct aarch64_insn_visitor *visitor,
				   struct aarch64_insn_data *data);

#define can_encode_int32(val, bits)			\
  (((val) >> (bits)) == 0 || ((val) >> (bits)) == -1)

/* Write a B or BL instruction into *BUF.

     B  #offset
     BL #offset

   IS_BL specifies if the link register should be updated.
   OFFSET is the immediate offset from the current PC.  It is
   byte-addressed but should be 4 bytes aligned.  It has a limited range of
   +/- 128MB (26 bits << 2).  */

#define emit_b(buf, is_bl, offset) \
  aarch64_emit_insn (buf, ((is_bl) ? BL : B) | (ENCODE ((offset) >> 2, 26, 0)))

/* Write a BCOND instruction into *BUF.

     B.COND #offset

   COND specifies the condition field.
   OFFSET is the immediate offset from the current PC.  It is
   byte-addressed but should be 4 bytes aligned.  It has a limited range of
   +/- 1MB (19 bits << 2).  */

#define emit_bcond(buf, cond, offset)				\
  aarch64_emit_insn (buf,					\
		     BCOND | ENCODE ((offset) >> 2, 19, 5)	\
		     | ENCODE ((cond), 4, 0))

/* Write a CBZ or CBNZ instruction into *BUF.

     CBZ  rt, #offset
     CBNZ rt, #offset

   IS_CBNZ distinguishes between CBZ and CBNZ instructions.
   RN is the register to test.
   OFFSET is the immediate offset from the current PC.  It is
   byte-addressed but should be 4 bytes aligned.  It has a limited range of
   +/- 1MB (19 bits << 2).  */

#define emit_cb(buf, is_cbnz, rt, offset)			\
  aarch64_emit_insn (buf,					\
		     ((is_cbnz) ? CBNZ : CBZ)			\
		     | ENCODE (rt.is64, 1, 31)  /* sf */	\
		     | ENCODE (offset >> 2, 19, 5) /* imm19 */	\
		     | ENCODE (rt.num, 5, 0))

/* Write a LDR instruction into *BUF.

     LDR rt, [rn, #offset]
     LDR rt, [rn, #index]!
     LDR rt, [rn], #index

   RT is the register to store.
   RN is the base address register.
   OFFSET is the immediate to add to the base address.  It is limited to
   0 .. 32760 range (12 bits << 3).  */

#define emit_ldr(buf, rt, rn, operand) \
  aarch64_emit_load_store (buf, rt.is64 ? 3 : 2, LDR, rt, rn, operand)

/* Write a LDRSW instruction into *BUF.  The register size is 64-bit.

     LDRSW xt, [rn, #offset]
     LDRSW xt, [rn, #index]!
     LDRSW xt, [rn], #index

   RT is the register to store.
   RN is the base address register.
   OFFSET is the immediate to add to the base address.  It is limited to
   0 .. 16380 range (12 bits << 2).  */

#define emit_ldrsw(buf, rt, rn, operand)		\
  aarch64_emit_load_store (buf, 3, LDRSW, rt, rn, operand)


/* Write a TBZ or TBNZ instruction into *BUF.

   TBZ  rt, #bit, #offset
   TBNZ rt, #bit, #offset

   IS_TBNZ distinguishes between TBZ and TBNZ instructions.
   RT is the register to test.
   BIT is the index of the bit to test in register RT.
   OFFSET is the immediate offset from the current PC.  It is
   byte-addressed but should be 4 bytes aligned.  It has a limited range of
   +/- 32KB (14 bits << 2).  */

#define emit_tb(buf, is_tbnz, bit, rt, offset)		       \
  aarch64_emit_insn (buf,				       \
		     ((is_tbnz) ? TBNZ: TBZ)		       \
		     | ENCODE (bit >> 5, 1, 31) /* b5 */       \
		     | ENCODE (bit, 5, 19) /* b40 */	       \
		     | ENCODE (offset >> 2, 14, 5) /* imm14 */ \
		     | ENCODE (rt.num, 5, 0))

/* Write a NOP instruction into *BUF.  */

#define emit_nop(buf) aarch64_emit_insn (buf, NOP)

int aarch64_emit_insn (uint32_t *buf, uint32_t insn);

int aarch64_emit_load_store (uint32_t *buf, uint32_t size,
			     enum aarch64_opcodes opcode,
			     struct aarch64_register rt,
			     struct aarch64_register rn,
			     struct aarch64_memory_operand operand);

#endif /* ARCH_AARCH64_INSN_H */
