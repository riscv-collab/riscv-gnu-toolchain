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

#include "gdbsupport/common-defs.h"
#include "aarch64-insn.h"

/* Toggle this file's internal debugging dump.  */
bool aarch64_debug = false;

/* Determine if specified bits within an instruction opcode matches a
   specific pattern.

   INSN is the instruction opcode.

   MASK specifies the bits within the opcode that are to be tested
   against for a match with PATTERN.  */

static int
decode_masked_match (uint32_t insn, uint32_t mask, uint32_t pattern)
{
  return (insn & mask) == pattern;
}

/* Decode an opcode if it represents an ADR or ADRP instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   IS_ADRP receives the 'op' field from the decoded instruction.
   RD receives the 'rd' field from the decoded instruction.
   OFFSET receives the 'immhi:immlo' field from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

int
aarch64_decode_adr (CORE_ADDR addr, uint32_t insn, int *is_adrp,
		    unsigned *rd, int32_t *offset)
{
  /* adr  0ii1 0000 iiii iiii iiii iiii iiir rrrr */
  /* adrp 1ii1 0000 iiii iiii iiii iiii iiir rrrr */
  if (decode_masked_match (insn, 0x1f000000, 0x10000000))
    {
      uint32_t immlo = (insn >> 29) & 0x3;
      int32_t immhi = sbits (insn, 5, 23) * 4;

      *is_adrp = (insn >> 31) & 0x1;
      *rd = (insn >> 0) & 0x1f;

      if (*is_adrp)
	{
	  /* The ADRP instruction has an offset with a -/+ 4GB range,
	     encoded as (immhi:immlo * 4096).  */
	  *offset = (immhi | immlo) * 4096;
	}
      else
	*offset = (immhi | immlo);

      aarch64_debug_printf ("decode: 0x%s 0x%x %s x%u, #?",
			    core_addr_to_string_nz (addr), insn,
			    *is_adrp ?  "adrp" : "adr", *rd);
      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents an branch immediate or branch
   and link immediate instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   IS_BL receives the 'op' bit from the decoded instruction.
   OFFSET receives the immediate offset from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

int
aarch64_decode_b (CORE_ADDR addr, uint32_t insn, int *is_bl,
		  int32_t *offset)
{
  /* b  0001 01ii iiii iiii iiii iiii iiii iiii */
  /* bl 1001 01ii iiii iiii iiii iiii iiii iiii */
  if (decode_masked_match (insn, 0x7c000000, 0x14000000))
    {
      *is_bl = (insn >> 31) & 0x1;
      *offset = sbits (insn, 0, 25) * 4;

      if (aarch64_debug)
	{
	  debug_printf ("decode: 0x%s 0x%x %s 0x%s\n",
			core_addr_to_string_nz (addr), insn,
			*is_bl ? "bl" : "b",
			core_addr_to_string_nz (addr + *offset));
	}

      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents a conditional branch instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   COND receives the branch condition field from the decoded
   instruction.
   OFFSET receives the immediate offset from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

int
aarch64_decode_bcond (CORE_ADDR addr, uint32_t insn, unsigned *cond,
		      int32_t *offset)
{
  /* b.cond  0101 0100 iiii iiii iiii iiii iii0 cccc */
  if (decode_masked_match (insn, 0xff000010, 0x54000000))
    {
      *cond = (insn >> 0) & 0xf;
      *offset = sbits (insn, 5, 23) * 4;

      if (aarch64_debug)
	{
	  debug_printf ("decode: 0x%s 0x%x b<%u> 0x%s\n",
			core_addr_to_string_nz (addr), insn, *cond,
			core_addr_to_string_nz (addr + *offset));
	}
      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents a CBZ or CBNZ instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   IS64 receives the 'sf' field from the decoded instruction.
   IS_CBNZ receives the 'op' field from the decoded instruction.
   RN receives the 'rn' field from the decoded instruction.
   OFFSET receives the 'imm19' field from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

int
aarch64_decode_cb (CORE_ADDR addr, uint32_t insn, int *is64, int *is_cbnz,
		   unsigned *rn, int32_t *offset)
{
  /* cbz  T011 010o iiii iiii iiii iiii iiir rrrr */
  /* cbnz T011 010o iiii iiii iiii iiii iiir rrrr */
  if (decode_masked_match (insn, 0x7e000000, 0x34000000))
    {
      *rn = (insn >> 0) & 0x1f;
      *is64 = (insn >> 31) & 0x1;
      *is_cbnz = (insn >> 24) & 0x1;
      *offset = sbits (insn, 5, 23) * 4;

      if (aarch64_debug)
	{
	  debug_printf ("decode: 0x%s 0x%x %s 0x%s\n",
			core_addr_to_string_nz (addr), insn,
			*is_cbnz ? "cbnz" : "cbz",
			core_addr_to_string_nz (addr + *offset));
	}
      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents a TBZ or TBNZ instruction.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   IS_TBNZ receives the 'op' field from the decoded instruction.
   BIT receives the bit position field from the decoded instruction.
   RT receives 'rt' field from the decoded instruction.
   IMM receives 'imm' field from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

int
aarch64_decode_tb (CORE_ADDR addr, uint32_t insn, int *is_tbnz,
		   unsigned *bit, unsigned *rt, int32_t *imm)
{
  /* tbz  b011 0110 bbbb biii iiii iiii iiir rrrr */
  /* tbnz B011 0111 bbbb biii iiii iiii iiir rrrr */
  if (decode_masked_match (insn, 0x7e000000, 0x36000000))
    {
      *rt = (insn >> 0) & 0x1f;
      *is_tbnz = (insn >> 24) & 0x1;
      *bit = ((insn >> (31 - 4)) & 0x20) | ((insn >> 19) & 0x1f);
      *imm = sbits (insn, 5, 18) * 4;

      if (aarch64_debug)
	{
	  debug_printf ("decode: 0x%s 0x%x %s x%u, #%u, 0x%s\n",
			core_addr_to_string_nz (addr), insn,
			*is_tbnz ? "tbnz" : "tbz", *rt, *bit,
			core_addr_to_string_nz (addr + *imm));
	}
      return 1;
    }
  return 0;
}

/* Decode an opcode if it represents an LDR or LDRSW instruction taking a
   literal offset from the current PC.

   ADDR specifies the address of the opcode.
   INSN specifies the opcode to test.
   IS_W is set if the instruction is LDRSW.
   IS64 receives size field from the decoded instruction.
   RT receives the 'rt' field from the decoded instruction.
   OFFSET receives the 'imm' field from the decoded instruction.

   Return 1 if the opcodes matches and is decoded, otherwise 0.  */

int
aarch64_decode_ldr_literal (CORE_ADDR addr, uint32_t insn, int *is_w,
			    int *is64, unsigned *rt, int32_t *offset)
{
  /* LDR    0T01 1000 iiii iiii iiii iiii iiir rrrr */
  /* LDRSW  1001 1000 iiii iiii iiii iiii iiir rrrr */
  if ((insn & 0x3f000000) == 0x18000000)
    {
      *is_w = (insn >> 31) & 0x1;

      if (*is_w)
	{
	  /* LDRSW always takes a 64-bit destination registers.  */
	  *is64 = 1;
	}
      else
	*is64 = (insn >> 30) & 0x1;

      *rt = (insn >> 0) & 0x1f;
      *offset = sbits (insn, 5, 23) * 4;

      if (aarch64_debug)
	debug_printf ("decode: %s 0x%x %s %s%u, #?\n",
		      core_addr_to_string_nz (addr), insn,
		      *is_w ? "ldrsw" : "ldr",
		      *is64 ? "x" : "w", *rt);

      return 1;
    }

  return 0;
}

/* Visit an instruction INSN by VISITOR with all needed information in DATA.

   PC relative instructions need to be handled specifically:

   - B/BL
   - B.COND
   - CBZ/CBNZ
   - TBZ/TBNZ
   - ADR/ADRP
   - LDR/LDRSW (literal)  */

void
aarch64_relocate_instruction (uint32_t insn,
			      const struct aarch64_insn_visitor *visitor,
			      struct aarch64_insn_data *data)
{
  int is_bl;
  int is64;
  int is_sw;
  int is_cbnz;
  int is_tbnz;
  int is_adrp;
  unsigned rn;
  unsigned rt;
  unsigned rd;
  unsigned cond;
  unsigned bit;
  int32_t offset;

  if (aarch64_decode_b (data->insn_addr, insn, &is_bl, &offset))
    visitor->b (is_bl, offset, data);
  else if (aarch64_decode_bcond (data->insn_addr, insn, &cond, &offset))
    visitor->b_cond (cond, offset, data);
  else if (aarch64_decode_cb (data->insn_addr, insn, &is64, &is_cbnz, &rn,
			      &offset))
    visitor->cb (offset, is_cbnz, rn, is64, data);
  else if (aarch64_decode_tb (data->insn_addr, insn, &is_tbnz, &bit, &rt,
			      &offset))
    visitor->tb (offset, is_tbnz, rt, bit, data);
  else if (aarch64_decode_adr (data->insn_addr, insn, &is_adrp, &rd, &offset))
    visitor->adr (offset, rd, is_adrp, data);
  else if (aarch64_decode_ldr_literal (data->insn_addr, insn, &is_sw, &is64,
				       &rt, &offset))
    visitor->ldr_literal (offset, is_sw, rt, is64, data);
  else
    visitor->others (insn, data);
}

/* Write a 32-bit unsigned integer INSN info *BUF.  Return the number of
   instructions written (aka. 1).  */

int
aarch64_emit_insn (uint32_t *buf, uint32_t insn)
{
  *buf = insn;
  return 1;
}

/* Helper function emitting a load or store instruction.  */

int
aarch64_emit_load_store (uint32_t *buf, uint32_t size,
			 enum aarch64_opcodes opcode,
			 struct aarch64_register rt,
			 struct aarch64_register rn,
			 struct aarch64_memory_operand operand)
{
  uint32_t op;

  switch (operand.type)
    {
    case MEMORY_OPERAND_OFFSET:
      {
	op = ENCODE (1, 1, 24);

	return aarch64_emit_insn (buf, opcode | ENCODE (size, 2, 30) | op
				  | ENCODE (operand.index >> 3, 12, 10)
				  | ENCODE (rn.num, 5, 5)
				  | ENCODE (rt.num, 5, 0));
      }
    case MEMORY_OPERAND_POSTINDEX:
      {
	uint32_t post_index = ENCODE (1, 2, 10);

	op = ENCODE (0, 1, 24);

	return aarch64_emit_insn (buf, opcode | ENCODE (size, 2, 30) | op
				  | post_index | ENCODE (operand.index, 9, 12)
				  | ENCODE (rn.num, 5, 5)
				  | ENCODE (rt.num, 5, 0));
      }
    case MEMORY_OPERAND_PREINDEX:
      {
	uint32_t pre_index = ENCODE (3, 2, 10);

	op = ENCODE (0, 1, 24);

	return aarch64_emit_insn (buf, opcode | ENCODE (size, 2, 30) | op
				  | pre_index | ENCODE (operand.index, 9, 12)
				  | ENCODE (rn.num, 5, 5)
				  | ENCODE (rt.num, 5, 0));
      }
    default:
      return 0;
    }
}
