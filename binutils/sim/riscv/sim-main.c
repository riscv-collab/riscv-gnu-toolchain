/* RISC-V simulator.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.
   Contributed by Mike Frysinger.

   This file is part of simulators.

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

/* This file contains the main simulator decoding logic.  i.e. everything that
   is architecture specific.  */

/* This must come before any other includes.  */
#include "defs.h"

#include <inttypes.h>
#include <time.h>

#include "sim-main.h"
#include "sim-signal.h"
#include "sim-syscall.h"

#include "opcode/riscv.h"

#include "sim/sim-riscv.h"

#include "riscv-sim.h"

#define TRACE_REG(cpu, reg) \
  TRACE_REGISTER (cpu, "wrote %s = %#" PRIxTW, riscv_gpr_names_abi[reg], \
		  RISCV_SIM_CPU (cpu)->regs[reg])

static const struct riscv_opcode *riscv_hash[OP_MASK_OP + 1];
#define OP_HASH_IDX(i) ((i) & (riscv_insn_length (i) == 2 ? 0x3 : 0x7f))

#define RISCV_ASSERT_RV32(cpu, fmt, args...) \
  do { \
    if (RISCV_XLEN (cpu) != 32) \
      { \
	TRACE_INSN (cpu, "RV32I-only " fmt, ## args); \
	sim_engine_halt (CPU_STATE (cpu), cpu, NULL, sim_pc_get (cpu), \
			 sim_signalled, SIM_SIGILL); \
      } \
  } while (0)

#define RISCV_ASSERT_RV64(cpu, fmt, args...) \
  do { \
    if (RISCV_XLEN (cpu) != 64) \
      { \
	TRACE_INSN (cpu, "RV64I-only " fmt, ## args); \
	sim_engine_halt (CPU_STATE (cpu), cpu, NULL, sim_pc_get (cpu), \
			 sim_signalled, SIM_SIGILL); \
      } \
  } while (0)

static INLINE void
store_rd (SIM_CPU *cpu, int rd, unsigned_word val)
{
  struct riscv_sim_cpu *riscv_cpu = RISCV_SIM_CPU (cpu);

  if (rd)
    {
      riscv_cpu->regs[rd] = val;
      TRACE_REG (cpu, rd);
    }
}

static INLINE unsigned_word
fetch_csr (SIM_CPU *cpu, const char *name, int csr, unsigned_word *reg)
{
  /* Handle pseudo registers.  */
  switch (csr)
    {
    /* Allow certain registers only in respective modes.  */
    case CSR_CYCLEH:
    case CSR_INSTRETH:
    case CSR_TIMEH:
      RISCV_ASSERT_RV32 (cpu, "CSR: %s", name);
      break;
    }

  return *reg;
}

static INLINE void
store_csr (SIM_CPU *cpu, const char *name, int csr, unsigned_word *reg,
	   unsigned_word val)
{
  struct riscv_sim_cpu *riscv_cpu = RISCV_SIM_CPU (cpu);

  switch (csr)
    {
    /* These are pseudo registers that modify sub-fields of fcsr.  */
    case CSR_FRM:
      val &= 0x7;
      *reg = val;
      riscv_cpu->csr.fcsr = (riscv_cpu->csr.fcsr & ~0xe0) | (val << 5);
      break;
    case CSR_FFLAGS:
      val &= 0x1f;
      *reg = val;
      riscv_cpu->csr.fcsr = (riscv_cpu->csr.fcsr & ~0x1f) | val;
      break;
    /* Keep the sub-fields in sync.  */
    case CSR_FCSR:
      *reg = val;
      riscv_cpu->csr.frm = (val >> 5) & 0x7;
      riscv_cpu->csr.fflags = val & 0x1f;
      break;

    /* Allow certain registers only in respective modes.  */
    case CSR_CYCLEH:
    case CSR_INSTRETH:
    case CSR_TIMEH:
      RISCV_ASSERT_RV32 (cpu, "CSR: %s", name);
      ATTRIBUTE_FALLTHROUGH;

    /* All the rest are immutable.  */
    default:
      val = *reg;
      break;
    }

  TRACE_REGISTER (cpu, "wrote CSR %s = %#" PRIxTW, name, val);
}

static inline unsigned_word
ashiftrt (unsigned_word val, unsigned_word shift)
{
  uint32_t sign = (val & 0x80000000) ? ~(0xfffffffful >> shift) : 0;
  return (val >> shift) | sign;
}

static inline unsigned_word
ashiftrt64 (unsigned_word val, unsigned_word shift)
{
  uint64_t sign =
    (val & 0x8000000000000000ull) ? ~(0xffffffffffffffffull >> shift) : 0;
  return (val >> shift) | sign;
}

static sim_cia
execute_i (SIM_CPU *cpu, unsigned_word iw, const struct riscv_opcode *op)
{
  SIM_DESC sd = CPU_STATE (cpu);
  struct riscv_sim_cpu *riscv_cpu = RISCV_SIM_CPU (cpu);
  int rd = (iw >> OP_SH_RD) & OP_MASK_RD;
  int rs1 = (iw >> OP_SH_RS1) & OP_MASK_RS1;
  int rs2 = (iw >> OP_SH_RS2) & OP_MASK_RS2;
  const char *rd_name = riscv_gpr_names_abi[rd];
  const char *rs1_name = riscv_gpr_names_abi[rs1];
  const char *rs2_name = riscv_gpr_names_abi[rs2];
  unsigned int csr = (iw >> OP_SH_CSR) & OP_MASK_CSR;
  unsigned_word i_imm = EXTRACT_ITYPE_IMM (iw);
  unsigned_word u_imm = EXTRACT_UTYPE_IMM ((uint64_t) iw);
  unsigned_word s_imm = EXTRACT_STYPE_IMM (iw);
  unsigned_word sb_imm = EXTRACT_BTYPE_IMM (iw);
  unsigned_word shamt_imm = ((iw >> OP_SH_SHAMT) & OP_MASK_SHAMT);
  unsigned_word tmp;
  sim_cia pc = riscv_cpu->pc + 4;

  TRACE_EXTRACT (cpu,
		 "rd:%-2i:%-4s  "
		 "rs1:%-2i:%-4s %0*" PRIxTW "  "
		 "rs2:%-2i:%-4s %0*" PRIxTW "  "
		 "match:%#x mask:%#x",
		 rd, rd_name,
		 rs1, rs1_name, (int) sizeof (unsigned_word) * 2,
		 riscv_cpu->regs[rs1],
		 rs2, rs2_name, (int) sizeof (unsigned_word) * 2,
		 riscv_cpu->regs[rs2],
		 (unsigned) op->match, (unsigned) op->mask);

  switch (op->match)
    {
    case MATCH_ADD:
      TRACE_INSN (cpu, "add %s, %s, %s;  // %s = %s + %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      store_rd (cpu, rd, riscv_cpu->regs[rs1] + riscv_cpu->regs[rs2]);
      break;
    case MATCH_ADDW:
      TRACE_INSN (cpu, "addw %s, %s, %s;  // %s = %s + %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      store_rd (cpu, rd,
		EXTEND32 (riscv_cpu->regs[rs1] + riscv_cpu->regs[rs2]));
      break;
    case MATCH_ADDI:
      TRACE_INSN (cpu, "addi %s, %s, %#" PRIxTW ";  // %s = %s + %#" PRIxTW,
		  rd_name, rs1_name, i_imm, rd_name, rs1_name, i_imm);
      store_rd (cpu, rd, riscv_cpu->regs[rs1] + i_imm);
      break;
    case MATCH_ADDIW:
      TRACE_INSN (cpu, "addiw %s, %s, %#" PRIxTW ";  // %s = %s + %#" PRIxTW,
		  rd_name, rs1_name, i_imm, rd_name, rs1_name, i_imm);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      store_rd (cpu, rd, EXTEND32 (riscv_cpu->regs[rs1] + i_imm));
      break;
    case MATCH_AND:
      TRACE_INSN (cpu, "and %s, %s, %s;  // %s = %s & %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      store_rd (cpu, rd, riscv_cpu->regs[rs1] & riscv_cpu->regs[rs2]);
      break;
    case MATCH_ANDI:
      TRACE_INSN (cpu, "andi %s, %s, %" PRIiTW ";  // %s = %s & %#" PRIxTW,
		  rd_name, rs1_name, i_imm, rd_name, rs1_name, i_imm);
      store_rd (cpu, rd, riscv_cpu->regs[rs1] & i_imm);
      break;
    case MATCH_OR:
      TRACE_INSN (cpu, "or %s, %s, %s;  // %s = %s | %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      store_rd (cpu, rd, riscv_cpu->regs[rs1] | riscv_cpu->regs[rs2]);
      break;
    case MATCH_ORI:
      TRACE_INSN (cpu, "ori %s, %s, %" PRIiTW ";  // %s = %s | %#" PRIxTW,
		  rd_name, rs1_name, i_imm, rd_name, rs1_name, i_imm);
      store_rd (cpu, rd, riscv_cpu->regs[rs1] | i_imm);
      break;
    case MATCH_XOR:
      TRACE_INSN (cpu, "xor %s, %s, %s;  // %s = %s ^ %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      store_rd (cpu, rd, riscv_cpu->regs[rs1] ^ riscv_cpu->regs[rs2]);
      break;
    case MATCH_XORI:
      TRACE_INSN (cpu, "xori %s, %s, %" PRIiTW ";  // %s = %s ^ %#" PRIxTW,
		  rd_name, rs1_name, i_imm, rd_name, rs1_name, i_imm);
      store_rd (cpu, rd, riscv_cpu->regs[rs1] ^ i_imm);
      break;
    case MATCH_SUB:
      TRACE_INSN (cpu, "sub %s, %s, %s;  // %s = %s - %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      store_rd (cpu, rd, riscv_cpu->regs[rs1] - riscv_cpu->regs[rs2]);
      break;
    case MATCH_SUBW:
      TRACE_INSN (cpu, "subw %s, %s, %s;  // %s = %s - %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      store_rd (cpu, rd,
		EXTEND32 (riscv_cpu->regs[rs1] - riscv_cpu->regs[rs2]));
      break;
    case MATCH_LUI:
      TRACE_INSN (cpu, "lui %s, %#" PRIxTW ";", rd_name, u_imm);
      store_rd (cpu, rd, u_imm);
      break;
    case MATCH_SLL:
      TRACE_INSN (cpu, "sll %s, %s, %s;  // %s = %s << %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      u_imm = RISCV_XLEN (cpu) == 32 ? 0x1f : 0x3f;
      store_rd (cpu, rd,
		riscv_cpu->regs[rs1] << (riscv_cpu->regs[rs2] & u_imm));
      break;
    case MATCH_SLLW:
      TRACE_INSN (cpu, "sllw %s, %s, %s;  // %s = %s << %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      store_rd (cpu, rd, EXTEND32 (
	(uint32_t) riscv_cpu->regs[rs1] << (riscv_cpu->regs[rs2] & 0x1f)));
      break;
    case MATCH_SLLI:
      TRACE_INSN (cpu, "slli %s, %s, %" PRIiTW ";  // %s = %s << %#" PRIxTW,
		  rd_name, rs1_name, shamt_imm, rd_name, rs1_name, shamt_imm);
      if (RISCV_XLEN (cpu) == 32 && shamt_imm > 0x1f)
	sim_engine_halt (sd, cpu, NULL, riscv_cpu->pc, sim_signalled,
			 SIM_SIGILL);
      store_rd (cpu, rd, riscv_cpu->regs[rs1] << shamt_imm);
      break;
    case MATCH_SLLIW:
      TRACE_INSN (cpu, "slliw %s, %s, %" PRIiTW ";  // %s = %s << %#" PRIxTW,
		  rd_name, rs1_name, shamt_imm, rd_name, rs1_name, shamt_imm);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      store_rd (cpu, rd,
		EXTEND32 ((uint32_t) riscv_cpu->regs[rs1] << shamt_imm));
      break;
    case MATCH_SRL:
      TRACE_INSN (cpu, "srl %s, %s, %s;  // %s = %s >> %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      u_imm = RISCV_XLEN (cpu) == 32 ? 0x1f : 0x3f;
      store_rd (cpu, rd,
		riscv_cpu->regs[rs1] >> (riscv_cpu->regs[rs2] & u_imm));
      break;
    case MATCH_SRLW:
      TRACE_INSN (cpu, "srlw %s, %s, %s;  // %s = %s >> %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      store_rd (cpu, rd, EXTEND32 (
	(uint32_t) riscv_cpu->regs[rs1] >> (riscv_cpu->regs[rs2] & 0x1f)));
      break;
    case MATCH_SRLI:
      TRACE_INSN (cpu, "srli %s, %s, %" PRIiTW ";  // %s = %s >> %#" PRIxTW,
		  rd_name, rs1_name, shamt_imm, rd_name, rs1_name, shamt_imm);
      if (RISCV_XLEN (cpu) == 32 && shamt_imm > 0x1f)
	sim_engine_halt (sd, cpu, NULL, riscv_cpu->pc, sim_signalled,
			 SIM_SIGILL);
      store_rd (cpu, rd, riscv_cpu->regs[rs1] >> shamt_imm);
      break;
    case MATCH_SRLIW:
      TRACE_INSN (cpu, "srliw %s, %s, %" PRIiTW ";  // %s = %s >> %#" PRIxTW,
		  rd_name, rs1_name, shamt_imm, rd_name, rs1_name, shamt_imm);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      store_rd (cpu, rd,
		EXTEND32 ((uint32_t) riscv_cpu->regs[rs1] >> shamt_imm));
      break;
    case MATCH_SRA:
      TRACE_INSN (cpu, "sra %s, %s, %s;  // %s = %s >>> %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      if (RISCV_XLEN (cpu) == 32)
	tmp = ashiftrt (riscv_cpu->regs[rs1], riscv_cpu->regs[rs2] & 0x1f);
      else
	tmp = ashiftrt64 (riscv_cpu->regs[rs1], riscv_cpu->regs[rs2] & 0x3f);
      store_rd (cpu, rd, tmp);
      break;
    case MATCH_SRAW:
      TRACE_INSN (cpu, "sraw %s, %s, %s;  // %s = %s >>> %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      store_rd (cpu, rd, EXTEND32 (
	ashiftrt ((int32_t) riscv_cpu->regs[rs1],
		  riscv_cpu->regs[rs2] & 0x1f)));
      break;
    case MATCH_SRAI:
      TRACE_INSN (cpu, "srai %s, %s, %" PRIiTW ";  // %s = %s >>> %#" PRIxTW,
		  rd_name, rs1_name, shamt_imm, rd_name, rs1_name, shamt_imm);
      if (RISCV_XLEN (cpu) == 32)
	{
	  if (shamt_imm > 0x1f)
	    sim_engine_halt (sd, cpu, NULL, riscv_cpu->pc, sim_signalled,
			     SIM_SIGILL);
	  tmp = ashiftrt (riscv_cpu->regs[rs1], shamt_imm);
	}
      else
	tmp = ashiftrt64 (riscv_cpu->regs[rs1], shamt_imm);
      store_rd (cpu, rd, tmp);
      break;
    case MATCH_SRAIW:
      TRACE_INSN (cpu, "sraiw %s, %s, %" PRIiTW ";  // %s = %s >>> %#" PRIxTW,
		  rd_name, rs1_name, shamt_imm, rd_name, rs1_name, shamt_imm);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      store_rd (cpu, rd, EXTEND32 (
	ashiftrt ((int32_t) riscv_cpu->regs[rs1], shamt_imm)));
      break;
    case MATCH_SLT:
      TRACE_INSN (cpu, "slt");
      store_rd (cpu, rd,
		!!((signed_word) riscv_cpu->regs[rs1] <
		   (signed_word) riscv_cpu->regs[rs2]));
      break;
    case MATCH_SLTU:
      TRACE_INSN (cpu, "sltu");
      store_rd (cpu, rd, !!((unsigned_word) riscv_cpu->regs[rs1] <
			    (unsigned_word) riscv_cpu->regs[rs2]));
      break;
    case MATCH_SLTI:
      TRACE_INSN (cpu, "slti");
      store_rd (cpu, rd, !!((signed_word) riscv_cpu->regs[rs1] <
			    (signed_word) i_imm));
      break;
    case MATCH_SLTIU:
      TRACE_INSN (cpu, "sltiu");
      store_rd (cpu, rd, !!((unsigned_word) riscv_cpu->regs[rs1] <
			    (unsigned_word) i_imm));
      break;
    case MATCH_AUIPC:
      TRACE_INSN (cpu, "auipc %s, %" PRIiTW ";  // %s = pc + %" PRIiTW,
		  rd_name, u_imm, rd_name, u_imm);
      store_rd (cpu, rd, riscv_cpu->pc + u_imm);
      break;
    case MATCH_BEQ:
      TRACE_INSN (cpu, "beq %s, %s, %#" PRIxTW ";  "
		       "// if (%s == %s) goto %#" PRIxTW,
		  rs1_name, rs2_name, sb_imm, rs1_name, rs2_name, sb_imm);
      if (riscv_cpu->regs[rs1] == riscv_cpu->regs[rs2])
	{
	  pc = riscv_cpu->pc + sb_imm;
	  TRACE_BRANCH (cpu, "to %#" PRIxTW, pc);
	}
      break;
    case MATCH_BLT:
      TRACE_INSN (cpu, "blt %s, %s, %#" PRIxTW ";  "
		       "// if (%s < %s) goto %#" PRIxTW,
		  rs1_name, rs2_name, sb_imm, rs1_name, rs2_name, sb_imm);
      if ((signed_word) riscv_cpu->regs[rs1] <
	  (signed_word) riscv_cpu->regs[rs2])
	{
	  pc = riscv_cpu->pc + sb_imm;
	  TRACE_BRANCH (cpu, "to %#" PRIxTW, pc);
	}
      break;
    case MATCH_BLTU:
      TRACE_INSN (cpu, "bltu %s, %s, %#" PRIxTW ";  "
		       "// if (%s < %s) goto %#" PRIxTW,
		  rs1_name, rs2_name, sb_imm, rs1_name, rs2_name, sb_imm);
      if ((unsigned_word) riscv_cpu->regs[rs1] <
	  (unsigned_word) riscv_cpu->regs[rs2])
	{
	  pc = riscv_cpu->pc + sb_imm;
	  TRACE_BRANCH (cpu, "to %#" PRIxTW, pc);
	}
      break;
    case MATCH_BGE:
      TRACE_INSN (cpu, "bge %s, %s, %#" PRIxTW ";  "
		       "// if (%s >= %s) goto %#" PRIxTW,
		  rs1_name, rs2_name, sb_imm, rs1_name, rs2_name, sb_imm);
      if ((signed_word) riscv_cpu->regs[rs1] >=
	  (signed_word) riscv_cpu->regs[rs2])
	{
	  pc = riscv_cpu->pc + sb_imm;
	  TRACE_BRANCH (cpu, "to %#" PRIxTW, pc);
	}
      break;
    case MATCH_BGEU:
      TRACE_INSN (cpu, "bgeu %s, %s, %#" PRIxTW ";  "
		       "// if (%s >= %s) goto %#" PRIxTW,
		  rs1_name, rs2_name, sb_imm, rs1_name, rs2_name, sb_imm);
      if ((unsigned_word) riscv_cpu->regs[rs1] >=
	  (unsigned_word) riscv_cpu->regs[rs2])
	{
	  pc = riscv_cpu->pc + sb_imm;
	  TRACE_BRANCH (cpu, "to %#" PRIxTW, pc);
	}
      break;
    case MATCH_BNE:
      TRACE_INSN (cpu, "bne %s, %s, %#" PRIxTW ";  "
		       "// if (%s != %s) goto %#" PRIxTW,
		  rs1_name, rs2_name, sb_imm, rs1_name, rs2_name, sb_imm);
      if (riscv_cpu->regs[rs1] != riscv_cpu->regs[rs2])
	{
	  pc = riscv_cpu->pc + sb_imm;
	  TRACE_BRANCH (cpu, "to %#" PRIxTW, pc);
	}
      break;
    case MATCH_JAL:
      TRACE_INSN (cpu, "jal %s, %" PRIiTW ";", rd_name,
		  EXTRACT_JTYPE_IMM (iw));
      store_rd (cpu, rd, riscv_cpu->pc + 4);
      pc = riscv_cpu->pc + EXTRACT_JTYPE_IMM (iw);
      TRACE_BRANCH (cpu, "to %#" PRIxTW, pc);
      break;
    case MATCH_JALR:
      TRACE_INSN (cpu, "jalr %s, %s, %" PRIiTW ";", rd_name, rs1_name, i_imm);
      pc = riscv_cpu->regs[rs1] + i_imm;
      store_rd (cpu, rd, riscv_cpu->pc + 4);
      TRACE_BRANCH (cpu, "to %#" PRIxTW, pc);
      break;

    case MATCH_LD:
      TRACE_INSN (cpu, "ld %s, %" PRIiTW "(%s);",
		  rd_name, i_imm, rs1_name);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      store_rd (cpu, rd,
	sim_core_read_unaligned_8 (cpu, riscv_cpu->pc, read_map,
				   riscv_cpu->regs[rs1] + i_imm));
      break;
    case MATCH_LW:
      TRACE_INSN (cpu, "lw %s, %" PRIiTW "(%s);",
		  rd_name, i_imm, rs1_name);
      store_rd (cpu, rd, EXTEND32 (
	sim_core_read_unaligned_4 (cpu, riscv_cpu->pc, read_map,
				   riscv_cpu->regs[rs1] + i_imm)));
      break;
    case MATCH_LWU:
      TRACE_INSN (cpu, "lwu %s, %" PRIiTW "(%s);",
		  rd_name, i_imm, rs1_name);
      store_rd (cpu, rd,
	sim_core_read_unaligned_4 (cpu, riscv_cpu->pc, read_map,
				   riscv_cpu->regs[rs1] + i_imm));
      break;
    case MATCH_LH:
      TRACE_INSN (cpu, "lh %s, %" PRIiTW "(%s);",
		  rd_name, i_imm, rs1_name);
      store_rd (cpu, rd, EXTEND16 (
	sim_core_read_unaligned_2 (cpu, riscv_cpu->pc, read_map,
				   riscv_cpu->regs[rs1] + i_imm)));
      break;
    case MATCH_LHU:
      TRACE_INSN (cpu, "lbu %s, %" PRIiTW "(%s);",
		  rd_name, i_imm, rs1_name);
      store_rd (cpu, rd,
	sim_core_read_unaligned_2 (cpu, riscv_cpu->pc, read_map,
				   riscv_cpu->regs[rs1] + i_imm));
      break;
    case MATCH_LB:
      TRACE_INSN (cpu, "lb %s, %" PRIiTW "(%s);",
		  rd_name, i_imm, rs1_name);
      store_rd (cpu, rd, EXTEND8 (
	sim_core_read_unaligned_1 (cpu, riscv_cpu->pc, read_map,
				   riscv_cpu->regs[rs1] + i_imm)));
      break;
    case MATCH_LBU:
      TRACE_INSN (cpu, "lbu %s, %" PRIiTW "(%s);",
		  rd_name, i_imm, rs1_name);
      store_rd (cpu, rd,
	sim_core_read_unaligned_1 (cpu, riscv_cpu->pc, read_map,
				   riscv_cpu->regs[rs1] + i_imm));
      break;
    case MATCH_SD:
      TRACE_INSN (cpu, "sd %s, %" PRIiTW "(%s);",
		  rs2_name, s_imm, rs1_name);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      sim_core_write_unaligned_8 (cpu, riscv_cpu->pc, write_map,
				  riscv_cpu->regs[rs1] + s_imm,
				  riscv_cpu->regs[rs2]);
      break;
    case MATCH_SW:
      TRACE_INSN (cpu, "sw %s, %" PRIiTW "(%s);",
		  rs2_name, s_imm, rs1_name);
      sim_core_write_unaligned_4 (cpu, riscv_cpu->pc, write_map,
				  riscv_cpu->regs[rs1] + s_imm,
				  riscv_cpu->regs[rs2]);
      break;
    case MATCH_SH:
      TRACE_INSN (cpu, "sh %s, %" PRIiTW "(%s);",
		  rs2_name, s_imm, rs1_name);
      sim_core_write_unaligned_2 (cpu, riscv_cpu->pc, write_map,
				  riscv_cpu->regs[rs1] + s_imm,
				  riscv_cpu->regs[rs2]);
      break;
    case MATCH_SB:
      TRACE_INSN (cpu, "sb %s, %" PRIiTW "(%s);",
		  rs2_name, s_imm, rs1_name);
      sim_core_write_unaligned_1 (cpu, riscv_cpu->pc, write_map,
				  riscv_cpu->regs[rs1] + s_imm,
				  riscv_cpu->regs[rs2]);
      break;

    case MATCH_CSRRC:
      TRACE_INSN (cpu, "csrrc");
      switch (csr)
	{
#define DECLARE_CSR(name, num, ...) \
	case num: \
	  store_rd (cpu, rd, \
		    fetch_csr (cpu, #name, num, &riscv_cpu->csr.name)); \
	  store_csr (cpu, #name, num, &riscv_cpu->csr.name, \
		     riscv_cpu->csr.name & !riscv_cpu->regs[rs1]); \
	  break;
#include "opcode/riscv-opc.h"
#undef DECLARE_CSR
	}
      break;
    case MATCH_CSRRS:
      TRACE_INSN (cpu, "csrrs");
      switch (csr)
	{
#define DECLARE_CSR(name, num, ...) \
	case num: \
	  store_rd (cpu, rd, \
		    fetch_csr (cpu, #name, num, &riscv_cpu->csr.name)); \
	  store_csr (cpu, #name, num, &riscv_cpu->csr.name, \
		     riscv_cpu->csr.name | riscv_cpu->regs[rs1]); \
	  break;
#include "opcode/riscv-opc.h"
#undef DECLARE_CSR
	}
      break;
    case MATCH_CSRRW:
      TRACE_INSN (cpu, "csrrw");
      switch (csr)
	{
#define DECLARE_CSR(name, num, ...) \
	case num: \
	  store_rd (cpu, rd, \
		    fetch_csr (cpu, #name, num, &riscv_cpu->csr.name)); \
	  store_csr (cpu, #name, num, &riscv_cpu->csr.name, \
		     riscv_cpu->regs[rs1]); \
	  break;
#include "opcode/riscv-opc.h"
#undef DECLARE_CSR
	}
      break;

    case MATCH_RDCYCLE:
      TRACE_INSN (cpu, "rdcycle %s;", rd_name);
      store_rd (cpu, rd,
		fetch_csr (cpu, "cycle", CSR_CYCLE, &riscv_cpu->csr.cycle));
      break;
    case MATCH_RDCYCLEH:
      TRACE_INSN (cpu, "rdcycleh %s;", rd_name);
      RISCV_ASSERT_RV32 (cpu, "insn: %s", op->name);
      store_rd (cpu, rd,
		fetch_csr (cpu, "cycleh", CSR_CYCLEH, &riscv_cpu->csr.cycleh));
      break;
    case MATCH_RDINSTRET:
      TRACE_INSN (cpu, "rdinstret %s;", rd_name);
      store_rd (cpu, rd,
		fetch_csr (cpu, "instret", CSR_INSTRET,
			   &riscv_cpu->csr.instret));
      break;
    case MATCH_RDINSTRETH:
      TRACE_INSN (cpu, "rdinstreth %s;", rd_name);
      RISCV_ASSERT_RV32 (cpu, "insn: %s", op->name);
      store_rd (cpu, rd,
		fetch_csr (cpu, "instreth", CSR_INSTRETH,
			   &riscv_cpu->csr.instreth));
      break;
    case MATCH_RDTIME:
      TRACE_INSN (cpu, "rdtime %s;", rd_name);
      store_rd (cpu, rd,
		fetch_csr (cpu, "time", CSR_TIME, &riscv_cpu->csr.time));
      break;
    case MATCH_RDTIMEH:
      TRACE_INSN (cpu, "rdtimeh %s;", rd_name);
      RISCV_ASSERT_RV32 (cpu, "insn: %s", op->name);
      store_rd (cpu, rd,
		fetch_csr (cpu, "timeh", CSR_TIMEH, &riscv_cpu->csr.timeh));
      break;

    case MATCH_FENCE:
      TRACE_INSN (cpu, "fence;");
      break;
    case MATCH_FENCE_I:
      TRACE_INSN (cpu, "fence.i;");
      break;
    case MATCH_EBREAK:
      TRACE_INSN (cpu, "ebreak;");
      /* GDB expects us to step over EBREAK.  */
      sim_engine_halt (sd, cpu, NULL, riscv_cpu->pc + 4, sim_stopped,
		       SIM_SIGTRAP);
      break;
    case MATCH_ECALL:
      TRACE_INSN (cpu, "ecall;");
      riscv_cpu->a0 = sim_syscall (cpu, riscv_cpu->a7, riscv_cpu->a0,
				   riscv_cpu->a1, riscv_cpu->a2, riscv_cpu->a3);
      break;
    default:
      TRACE_INSN (cpu, "UNHANDLED INSN: %s", op->name);
      sim_engine_halt (sd, cpu, NULL, riscv_cpu->pc, sim_signalled, SIM_SIGILL);
    }

  return pc;
}

static uint64_t
mulhu (uint64_t a, uint64_t b)
{
#ifdef HAVE___INT128
  return ((__int128)a * b) >> 64;
#else
  uint64_t t;
  uint32_t y1, y2, y3;
  uint64_t a0 = (uint32_t)a, a1 = a >> 32;
  uint64_t b0 = (uint32_t)b, b1 = b >> 32;

  t = a1*b0 + ((a0*b0) >> 32);
  y1 = t;
  y2 = t >> 32;

  t = a0*b1 + y1;
  y1 = t;

  t = a1*b1 + y2 + (t >> 32);
  y2 = t;
  y3 = t >> 32;

  return ((uint64_t)y3 << 32) | y2;
#endif
}

static uint64_t
mulh (int64_t a, int64_t b)
{
  int negate = (a < 0) != (b < 0);
  uint64_t res = mulhu (a < 0 ? -a : a, b < 0 ? -b : b);
  return negate ? ~res + (a * b == 0) : res;
}

static uint64_t
mulhsu (int64_t a, uint64_t b)
{
  int negate = a < 0;
  uint64_t res = mulhu (a < 0 ? -a : a, b);
  return negate ? ~res + (a * b == 0) : res;
}

static sim_cia
execute_m (SIM_CPU *cpu, unsigned_word iw, const struct riscv_opcode *op)
{
  struct riscv_sim_cpu *riscv_cpu = RISCV_SIM_CPU (cpu);
  SIM_DESC sd = CPU_STATE (cpu);
  int rd = (iw >> OP_SH_RD) & OP_MASK_RD;
  int rs1 = (iw >> OP_SH_RS1) & OP_MASK_RS1;
  int rs2 = (iw >> OP_SH_RS2) & OP_MASK_RS2;
  const char *rd_name = riscv_gpr_names_abi[rd];
  const char *rs1_name = riscv_gpr_names_abi[rs1];
  const char *rs2_name = riscv_gpr_names_abi[rs2];
  unsigned_word tmp, dividend_max;
  sim_cia pc = riscv_cpu->pc + 4;

  dividend_max = -((unsigned_word) 1 << (WITH_TARGET_WORD_BITSIZE - 1));

  switch (op->match)
    {
    case MATCH_DIV:
      TRACE_INSN (cpu, "div %s, %s, %s;  // %s = %s / %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      if (riscv_cpu->regs[rs1] == dividend_max && riscv_cpu->regs[rs2] == -1)
	tmp = dividend_max;
      else if (riscv_cpu->regs[rs2])
	tmp = (signed_word) riscv_cpu->regs[rs1] /
	  (signed_word) riscv_cpu->regs[rs2];
      else
	tmp = -1;
      store_rd (cpu, rd, tmp);
      break;
    case MATCH_DIVW:
      TRACE_INSN (cpu, "divw %s, %s, %s;  // %s = %s / %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      if (EXTEND32 (riscv_cpu->regs[rs2]) == -1)
	tmp = 1 << 31;
      else if (EXTEND32 (riscv_cpu->regs[rs2]))
	tmp = EXTEND32 (riscv_cpu->regs[rs1]) / EXTEND32 (riscv_cpu->regs[rs2]);
      else
	tmp = -1;
      store_rd (cpu, rd, EXTEND32 (tmp));
      break;
    case MATCH_DIVU:
      TRACE_INSN (cpu, "divu %s, %s, %s;  // %s = %s / %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      if (riscv_cpu->regs[rs2])
	store_rd (cpu, rd, (unsigned_word) riscv_cpu->regs[rs1]
			   / (unsigned_word) riscv_cpu->regs[rs2]);
      else
	store_rd (cpu, rd, -1);
      break;
    case MATCH_DIVUW:
      TRACE_INSN (cpu, "divuw %s, %s, %s;  // %s = %s / %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      if ((uint32_t) riscv_cpu->regs[rs2])
	tmp = (uint32_t) riscv_cpu->regs[rs1] / (uint32_t) riscv_cpu->regs[rs2];
      else
	tmp = -1;
      store_rd (cpu, rd, EXTEND32 (tmp));
      break;
    case MATCH_MUL:
      TRACE_INSN (cpu, "mul %s, %s, %s;  // %s = %s * %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      store_rd (cpu, rd, riscv_cpu->regs[rs1] * riscv_cpu->regs[rs2]);
      break;
    case MATCH_MULW:
      TRACE_INSN (cpu, "mulw %s, %s, %s;  // %s = %s * %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      store_rd (cpu, rd, EXTEND32 ((int32_t) riscv_cpu->regs[rs1]
				   * (int32_t) riscv_cpu->regs[rs2]));
      break;
    case MATCH_MULH:
      TRACE_INSN (cpu, "mulh %s, %s, %s;  // %s = %s * %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      if (RISCV_XLEN (cpu) == 32)
	store_rd (cpu, rd,
		  ((int64_t)(signed_word) riscv_cpu->regs[rs1]
		   * (int64_t)(signed_word) riscv_cpu->regs[rs2]) >> 32);
      else
	store_rd (cpu, rd, mulh (riscv_cpu->regs[rs1], riscv_cpu->regs[rs2]));
      break;
    case MATCH_MULHU:
      TRACE_INSN (cpu, "mulhu %s, %s, %s;  // %s = %s * %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      if (RISCV_XLEN (cpu) == 32)
	store_rd (cpu, rd, ((uint64_t)riscv_cpu->regs[rs1]
			    * (uint64_t)riscv_cpu->regs[rs2]) >> 32);
      else
	store_rd (cpu, rd, mulhu (riscv_cpu->regs[rs1], riscv_cpu->regs[rs2]));
      break;
    case MATCH_MULHSU:
      TRACE_INSN (cpu, "mulhsu %s, %s, %s;  // %s = %s * %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      if (RISCV_XLEN (cpu) == 32)
	store_rd (cpu, rd, ((int64_t)(signed_word) riscv_cpu->regs[rs1]
			    * (uint64_t)riscv_cpu->regs[rs2]) >> 32);
      else
	store_rd (cpu, rd, mulhsu (riscv_cpu->regs[rs1], riscv_cpu->regs[rs2]));
      break;
    case MATCH_REM:
      TRACE_INSN (cpu, "rem %s, %s, %s;  // %s = %s %% %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      if (riscv_cpu->regs[rs1] == dividend_max && riscv_cpu->regs[rs2] == -1)
	tmp = 0;
      else if (riscv_cpu->regs[rs2])
	tmp = (signed_word) riscv_cpu->regs[rs1]
	  % (signed_word) riscv_cpu->regs[rs2];
      else
	tmp = riscv_cpu->regs[rs1];
      store_rd (cpu, rd, tmp);
      break;
    case MATCH_REMW:
      TRACE_INSN (cpu, "remw %s, %s, %s;  // %s = %s %% %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      if (EXTEND32 (riscv_cpu->regs[rs2]) == -1)
	tmp = 0;
      else if (EXTEND32 (riscv_cpu->regs[rs2]))
	tmp = EXTEND32 (riscv_cpu->regs[rs1]) % EXTEND32 (riscv_cpu->regs[rs2]);
      else
	tmp = riscv_cpu->regs[rs1];
      store_rd (cpu, rd, EXTEND32 (tmp));
      break;
    case MATCH_REMU:
      TRACE_INSN (cpu, "remu %s, %s, %s;  // %s = %s %% %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      if (riscv_cpu->regs[rs2])
	store_rd (cpu, rd, riscv_cpu->regs[rs1] % riscv_cpu->regs[rs2]);
      else
	store_rd (cpu, rd, riscv_cpu->regs[rs1]);
      break;
    case MATCH_REMUW:
      TRACE_INSN (cpu, "remuw %s, %s, %s;  // %s = %s %% %s",
		  rd_name, rs1_name, rs2_name, rd_name, rs1_name, rs2_name);
      RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);
      if ((uint32_t) riscv_cpu->regs[rs2])
	tmp = (uint32_t) riscv_cpu->regs[rs1] % (uint32_t) riscv_cpu->regs[rs2];
      else
	tmp = riscv_cpu->regs[rs1];
      store_rd (cpu, rd, EXTEND32 (tmp));
      break;
    default:
      TRACE_INSN (cpu, "UNHANDLED INSN: %s", op->name);
      sim_engine_halt (sd, cpu, NULL, riscv_cpu->pc, sim_signalled, SIM_SIGILL);
    }

  return pc;
}

static sim_cia
execute_a (SIM_CPU *cpu, unsigned_word iw, const struct riscv_opcode *op)
{
  struct riscv_sim_cpu *riscv_cpu = RISCV_SIM_CPU (cpu);
  SIM_DESC sd = CPU_STATE (cpu);
  struct riscv_sim_state *state = RISCV_SIM_STATE (sd);
  int rd = (iw >> OP_SH_RD) & OP_MASK_RD;
  int rs1 = (iw >> OP_SH_RS1) & OP_MASK_RS1;
  int rs2 = (iw >> OP_SH_RS2) & OP_MASK_RS2;
  const char *rd_name = riscv_gpr_names_abi[rd];
  const char *rs1_name = riscv_gpr_names_abi[rs1];
  const char *rs2_name = riscv_gpr_names_abi[rs2];
  struct atomic_mem_reserved_list *amo_prev, *amo_curr;
  unsigned_word tmp;
  sim_cia pc = riscv_cpu->pc + 4;

  /* Handle these two load/store operations specifically.  */
  switch (op->match)
    {
    case MATCH_LR_W:
      TRACE_INSN (cpu, "%s %s, (%s);", op->name, rd_name, rs1_name);
      store_rd (cpu, rd,
	sim_core_read_unaligned_4 (cpu, riscv_cpu->pc, read_map,
				   riscv_cpu->regs[rs1]));

      /* Walk the reservation list to find an existing match.  */
      amo_curr = state->amo_reserved_list;
      while (amo_curr)
	{
	  if (amo_curr->addr == riscv_cpu->regs[rs1])
	    goto done;
	  amo_curr = amo_curr->next;
	}

      /* No reservation exists, so add one.  */
      amo_curr = xmalloc (sizeof (*amo_curr));
      amo_curr->addr = riscv_cpu->regs[rs1];
      amo_curr->next = state->amo_reserved_list;
      state->amo_reserved_list = amo_curr;
      goto done;
    case MATCH_SC_W:
      TRACE_INSN (cpu, "%s %s, %s, (%s);", op->name, rd_name, rs2_name,
		  rs1_name);

      /* Walk the reservation list to find a match.  */
      amo_curr = amo_prev = state->amo_reserved_list;
      while (amo_curr)
	{
	  if (amo_curr->addr == riscv_cpu->regs[rs1])
	    {
	      /* We found a reservation, so operate it.  */
	      sim_core_write_unaligned_4 (cpu, riscv_cpu->pc, write_map,
					  riscv_cpu->regs[rs1],
					  riscv_cpu->regs[rs2]);
	      store_rd (cpu, rd, 0);
	      if (amo_curr == state->amo_reserved_list)
		state->amo_reserved_list = amo_curr->next;
	      else
		amo_prev->next = amo_curr->next;
	      free (amo_curr);
	      goto done;
	    }
	  amo_prev = amo_curr;
	  amo_curr = amo_curr->next;
	}

      /* If we're still here, then no reservation exists, so mark as failed.  */
      store_rd (cpu, rd, 1);
      goto done;
    }

  /* Handle the rest of the atomic insns with common code paths.  */
  TRACE_INSN (cpu, "%s %s, %s, (%s);",
	      op->name, rd_name, rs2_name, rs1_name);
  if (op->xlen_requirement == 64)
    tmp = sim_core_read_unaligned_8 (cpu, riscv_cpu->pc, read_map,
				     riscv_cpu->regs[rs1]);
  else
    tmp = EXTEND32 (sim_core_read_unaligned_4 (cpu, riscv_cpu->pc, read_map,
					       riscv_cpu->regs[rs1]));
  store_rd (cpu, rd, tmp);

  switch (op->match)
    {
    case MATCH_AMOADD_D:
    case MATCH_AMOADD_W:
      tmp = riscv_cpu->regs[rd] + riscv_cpu->regs[rs2];
      break;
    case MATCH_AMOAND_D:
    case MATCH_AMOAND_W:
      tmp = riscv_cpu->regs[rd] & riscv_cpu->regs[rs2];
      break;
    case MATCH_AMOMAX_D:
    case MATCH_AMOMAX_W:
      tmp = max ((signed_word) riscv_cpu->regs[rd],
		 (signed_word) riscv_cpu->regs[rs2]);
      break;
    case MATCH_AMOMAXU_D:
    case MATCH_AMOMAXU_W:
      tmp = max ((unsigned_word) riscv_cpu->regs[rd],
		 (unsigned_word) riscv_cpu->regs[rs2]);
      break;
    case MATCH_AMOMIN_D:
    case MATCH_AMOMIN_W:
      tmp = min ((signed_word) riscv_cpu->regs[rd],
		 (signed_word) riscv_cpu->regs[rs2]);
      break;
    case MATCH_AMOMINU_D:
    case MATCH_AMOMINU_W:
      tmp = min ((unsigned_word) riscv_cpu->regs[rd],
		 (unsigned_word) riscv_cpu->regs[rs2]);
      break;
    case MATCH_AMOOR_D:
    case MATCH_AMOOR_W:
      tmp = riscv_cpu->regs[rd] | riscv_cpu->regs[rs2];
      break;
    case MATCH_AMOSWAP_D:
    case MATCH_AMOSWAP_W:
      tmp = riscv_cpu->regs[rs2];
      break;
    case MATCH_AMOXOR_D:
    case MATCH_AMOXOR_W:
      tmp = riscv_cpu->regs[rd] ^ riscv_cpu->regs[rs2];
      break;
    default:
      TRACE_INSN (cpu, "UNHANDLED INSN: %s", op->name);
      sim_engine_halt (sd, cpu, NULL, riscv_cpu->pc, sim_signalled, SIM_SIGILL);
    }

  if (op->xlen_requirement == 64)
    sim_core_write_unaligned_8 (cpu, riscv_cpu->pc, write_map,
				riscv_cpu->regs[rs1], tmp);
  else
    sim_core_write_unaligned_4 (cpu, riscv_cpu->pc, write_map,
				riscv_cpu->regs[rs1], tmp);

 done:
  return pc;
}

static sim_cia
execute_one (SIM_CPU *cpu, unsigned_word iw, const struct riscv_opcode *op)
{
  struct riscv_sim_cpu *riscv_cpu = RISCV_SIM_CPU (cpu);
  SIM_DESC sd = CPU_STATE (cpu);

  if (op->xlen_requirement == 32)
    RISCV_ASSERT_RV32 (cpu, "insn: %s", op->name);
  else if (op->xlen_requirement == 64)
    RISCV_ASSERT_RV64 (cpu, "insn: %s", op->name);

  switch (op->insn_class)
    {
    case INSN_CLASS_A:
      return execute_a (cpu, iw, op);
    case INSN_CLASS_I:
      return execute_i (cpu, iw, op);
    case INSN_CLASS_M:
    case INSN_CLASS_ZMMUL:
      return execute_m (cpu, iw, op);
    default:
      TRACE_INSN (cpu, "UNHANDLED EXTENSION: %d", op->insn_class);
      sim_engine_halt (sd, cpu, NULL, riscv_cpu->pc, sim_signalled, SIM_SIGILL);
    }

  return riscv_cpu->pc + riscv_insn_length (iw);
}

/* Decode & execute a single instruction.  */
void step_once (SIM_CPU *cpu)
{
  struct riscv_sim_cpu *riscv_cpu = RISCV_SIM_CPU (cpu);
  SIM_DESC sd = CPU_STATE (cpu);
  unsigned_word iw;
  unsigned int len;
  sim_cia pc = riscv_cpu->pc;
  const struct riscv_opcode *op;
  int xlen = RISCV_XLEN (cpu);

  if (TRACE_ANY_P (cpu))
    trace_prefix (sd, cpu, NULL_CIA, pc, TRACE_LINENUM_P (cpu),
		  NULL, 0, " "); /* Use a space for gcc warnings.  */

  iw = sim_core_read_aligned_2 (cpu, pc, exec_map, pc);

  /* Reject non-32-bit opcodes first.  */
  len = riscv_insn_length (iw);
  if (len != 4)
    {
      sim_io_printf (sd, "sim: bad insn len %#x @ %#" PRIxTA ": %#" PRIxTW "\n",
		     len, pc, iw);
      sim_engine_halt (sd, cpu, NULL, pc, sim_signalled, SIM_SIGILL);
    }

  iw |= ((unsigned_word) sim_core_read_aligned_2 (
    cpu, pc, exec_map, pc + 2) << 16);

  TRACE_CORE (cpu, "0x%08" PRIxTW, iw);

  op = riscv_hash[OP_HASH_IDX (iw)];
  if (!op)
    sim_engine_halt (sd, cpu, NULL, pc, sim_signalled, SIM_SIGILL);

  /* NB: Same loop logic as riscv_disassemble_insn.  */
  for (; op->name; op++)
    {
      /* Does the opcode match?  */
      if (! op->match_func (op, iw))
	continue;
      /* Is this a pseudo-instruction and may we print it as such?  */
      if (op->pinfo & INSN_ALIAS)
	continue;
      /* Is this instruction restricted to a certain value of XLEN?  */
      if (op->xlen_requirement != 0 && op->xlen_requirement != xlen)
	continue;

      /* It's a match.  */
      pc = execute_one (cpu, iw, op);
      break;
    }

  /* TODO: Handle overflow into high 32 bits.  */
  /* TODO: Try to use a common counter and only update on demand (reads).  */
  ++riscv_cpu->csr.cycle;
  ++riscv_cpu->csr.instret;

  riscv_cpu->pc = pc;
}

/* Return the program counter for this cpu. */
static sim_cia
pc_get (sim_cpu *cpu)
{
  struct riscv_sim_cpu *riscv_cpu = RISCV_SIM_CPU (cpu);

  return riscv_cpu->pc;
}

/* Set the program counter for this cpu to the new pc value. */
static void
pc_set (sim_cpu *cpu, sim_cia pc)
{
  struct riscv_sim_cpu *riscv_cpu = RISCV_SIM_CPU (cpu);

  riscv_cpu->pc = pc;
}

static int
reg_fetch (sim_cpu *cpu, int rn, void *buf, int len)
{
  struct riscv_sim_cpu *riscv_cpu = RISCV_SIM_CPU (cpu);

  if (len <= 0 || len > sizeof (unsigned_word))
    return -1;

  switch (rn)
    {
    case SIM_RISCV_ZERO_REGNUM:
      memset (buf, 0, len);
      return len;
    case SIM_RISCV_RA_REGNUM ... SIM_RISCV_T6_REGNUM:
      memcpy (buf, &riscv_cpu->regs[rn], len);
      return len;
    case SIM_RISCV_FIRST_FP_REGNUM ... SIM_RISCV_LAST_FP_REGNUM:
      memcpy (buf, &riscv_cpu->fpregs[rn - SIM_RISCV_FIRST_FP_REGNUM], len);
      return len;
    case SIM_RISCV_PC_REGNUM:
      memcpy (buf, &riscv_cpu->pc, len);
      return len;

#define DECLARE_CSR(name, num, ...) \
    case SIM_RISCV_ ## num ## _REGNUM: \
      memcpy (buf, &riscv_cpu->csr.name, len); \
      return len;
#include "opcode/riscv-opc.h"
#undef DECLARE_CSR

    default:
      return -1;
    }
}

static int
reg_store (sim_cpu *cpu, int rn, const void *buf, int len)
{
  struct riscv_sim_cpu *riscv_cpu = RISCV_SIM_CPU (cpu);

  if (len <= 0 || len > sizeof (unsigned_word))
    return -1;

  switch (rn)
    {
    case SIM_RISCV_ZERO_REGNUM:
      /* Ignore writes.  */
      return len;
    case SIM_RISCV_RA_REGNUM ... SIM_RISCV_T6_REGNUM:
      memcpy (&riscv_cpu->regs[rn], buf, len);
      return len;
    case SIM_RISCV_FIRST_FP_REGNUM ... SIM_RISCV_LAST_FP_REGNUM:
      memcpy (&riscv_cpu->fpregs[rn - SIM_RISCV_FIRST_FP_REGNUM], buf, len);
      return len;
    case SIM_RISCV_PC_REGNUM:
      memcpy (&riscv_cpu->pc, buf, len);
      return len;

#define DECLARE_CSR(name, num, ...) \
    case SIM_RISCV_ ## num ## _REGNUM: \
      memcpy (&riscv_cpu->csr.name, buf, len); \
      return len;
#include "opcode/riscv-opc.h"
#undef DECLARE_CSR

    default:
      return -1;
    }
}

/* Initialize the state for a single cpu.  Usuaully this involves clearing all
   registers back to their reset state.  Should also hook up the fetch/store
   helper functions too.  */
void
initialize_cpu (SIM_DESC sd, SIM_CPU *cpu, int mhartid)
{
  struct riscv_sim_cpu *riscv_cpu = RISCV_SIM_CPU (cpu);
  const char *extensions;
  int i;

  memset (riscv_cpu->regs, 0, sizeof (riscv_cpu->regs));

  CPU_PC_FETCH (cpu) = pc_get;
  CPU_PC_STORE (cpu) = pc_set;
  CPU_REG_FETCH (cpu) = reg_fetch;
  CPU_REG_STORE (cpu) = reg_store;

  if (!riscv_hash[0])
    {
      const struct riscv_opcode *op;

      for (op = riscv_opcodes; op->name; op++)
	if (!riscv_hash[OP_HASH_IDX (op->match)])
	  riscv_hash[OP_HASH_IDX (op->match)] = op;
    }

  riscv_cpu->csr.misa = 0;
  /* RV32 sets this field to 0, and we don't really support RV128 yet.  */
  if (RISCV_XLEN (cpu) == 64)
    riscv_cpu->csr.misa |= (uint64_t)2 << 62;

  /* Skip the leading "rv" prefix and the two numbers.  */
  extensions = MODEL_NAME (CPU_MODEL (cpu)) + 4;
  for (i = 0; i < 26; ++i)
    {
      char ext = 'A' + i;

      if (ext == 'X')
	continue;
      else if (strchr (extensions, ext) != NULL)
	{
	  if (ext == 'G')
	    riscv_cpu->csr.misa |= 0x1129;  /* G = IMAFD.  */
	  else
	    riscv_cpu->csr.misa |= (1 << i);
	}
    }

  riscv_cpu->csr.mimpid = 0x8000;
  riscv_cpu->csr.mhartid = mhartid;
}

/* Some utils don't like having a NULL environ.  */
static const char * const simple_env[] = { "HOME=/", "PATH=/bin", NULL };

/* Count the number of arguments in an argv.  */
static int
count_argv (const char * const *argv)
{
  int i;

  if (!argv)
    return -1;

  for (i = 0; argv[i] != NULL; ++i)
    continue;
  return i;
}

void
initialize_env (SIM_DESC sd, const char * const *argv, const char * const *env)
{
  SIM_CPU *cpu = STATE_CPU (sd, 0);
  struct riscv_sim_cpu *riscv_cpu = RISCV_SIM_CPU (cpu);
  int i;
  int argc, argv_flat;
  int envc, env_flat;
  address_word sp, sp_flat;
  unsigned char null[8] = { 0, 0, 0, 0, 0, 0, 0, 0, };

  /* Figure out how many bytes the argv strings take up.  */
  argc = count_argv (argv);
  if (argc == -1)
    argc = 0;
  argv_flat = argc; /* NUL bytes.  */
  for (i = 0; i < argc; ++i)
    argv_flat += strlen (argv[i]);

  /* Figure out how many bytes the environ strings take up.  */
  if (!env)
    env = simple_env;
  envc = count_argv (env);
  env_flat = envc; /* NUL bytes.  */
  for (i = 0; i < envc; ++i)
    env_flat += strlen (env[i]);

  /* Make space for the strings themselves.  */
  sp_flat = (DEFAULT_MEM_SIZE - argv_flat - env_flat) & -sizeof (address_word);
  /* Then the pointers to the strings.  */
  sp = sp_flat - ((argc + 1 + envc + 1) * sizeof (address_word));
  /* Then the argc.  */
  sp -= sizeof (unsigned_word);

  /* Set up the regs the libgloss crt0 expects.  */
  riscv_cpu->a0 = argc;
  riscv_cpu->sp = sp;

  /* First push the argc value.  */
  sim_write (sd, sp, &argc, sizeof (unsigned_word));
  sp += sizeof (unsigned_word);

  /* Then the actual argv strings so we know where to point argv[].  */
  for (i = 0; i < argc; ++i)
    {
      unsigned len = strlen (argv[i]) + 1;
      sim_write (sd, sp_flat, argv[i], len);
      sim_write (sd, sp, &sp_flat, sizeof (address_word));
      sp_flat += len;
      sp += sizeof (address_word);
    }
  sim_write (sd, sp, null, sizeof (address_word));
  sp += sizeof (address_word);

  /* Then the actual env strings so we know where to point env[].  */
  for (i = 0; i < envc; ++i)
    {
      unsigned len = strlen (env[i]) + 1;
      sim_write (sd, sp_flat, env[i], len);
      sim_write (sd, sp, &sp_flat, sizeof (address_word));
      sp_flat += len;
      sp += sizeof (address_word);
    }
}
