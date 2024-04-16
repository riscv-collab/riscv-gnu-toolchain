/* Common target dependent code for GDB on AArch64 systems.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.
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

#include "defs.h"

#include "frame.h"
#include "language.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "dis-asm.h"
#include "regcache.h"
#include "reggroups.h"
#include "value.h"
#include "arch-utils.h"
#include "osabi.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "trad-frame.h"
#include "objfiles.h"
#include "dwarf2.h"
#include "dwarf2/frame.h"
#include "gdbtypes.h"
#include "prologue-value.h"
#include "target-descriptions.h"
#include "user-regs.h"
#include "ax-gdb.h"
#include "gdbsupport/selftest.h"

#include "aarch64-tdep.h"
#include "aarch64-ravenscar-thread.h"

#include "record.h"
#include "record-full.h"
#include "arch/aarch64-insn.h"
#include "gdbarch.h"

#include "opcode/aarch64.h"
#include <algorithm>
#include <unordered_map>

/* For inferior_ptid and current_inferior ().  */
#include "inferior.h"
/* For std::sqrt and std::pow.  */
#include <cmath>

/* A Homogeneous Floating-Point or Short-Vector Aggregate may have at most
   four members.  */
#define HA_MAX_NUM_FLDS		4

/* All possible aarch64 target descriptors.  */
static std::unordered_map <aarch64_features, target_desc *> tdesc_aarch64_map;

/* The standard register names, and all the valid aliases for them.
   We're not adding fp here, that name is already taken, see
   _initialize_frame_reg.  */
static const struct
{
  const char *const name;
  int regnum;
} aarch64_register_aliases[] =
{
  /* Link register alias for x30.  */
  {"lr", AARCH64_LR_REGNUM},
  /* SP is the canonical name for x31 according to aarch64_r_register_names,
     so we're adding an x31 alias for sp.  */
  {"x31", AARCH64_SP_REGNUM},
  /*  specials */
  {"ip0", AARCH64_X0_REGNUM + 16},
  {"ip1", AARCH64_X0_REGNUM + 17}
};

/* The required core 'R' registers.  */
static const char *const aarch64_r_register_names[] =
{
  /* These registers must appear in consecutive RAW register number
     order and they must begin with AARCH64_X0_REGNUM! */
  "x0", "x1", "x2", "x3",
  "x4", "x5", "x6", "x7",
  "x8", "x9", "x10", "x11",
  "x12", "x13", "x14", "x15",
  "x16", "x17", "x18", "x19",
  "x20", "x21", "x22", "x23",
  "x24", "x25", "x26", "x27",
  "x28", "x29", "x30", "sp",
  "pc", "cpsr"
};

/* The FP/SIMD 'V' registers.  */
static const char *const aarch64_v_register_names[] =
{
  /* These registers must appear in consecutive RAW register number
     order and they must begin with AARCH64_V0_REGNUM! */
  "v0", "v1", "v2", "v3",
  "v4", "v5", "v6", "v7",
  "v8", "v9", "v10", "v11",
  "v12", "v13", "v14", "v15",
  "v16", "v17", "v18", "v19",
  "v20", "v21", "v22", "v23",
  "v24", "v25", "v26", "v27",
  "v28", "v29", "v30", "v31",
  "fpsr",
  "fpcr"
};

/* The SVE 'Z' and 'P' registers.  */
static const char *const aarch64_sve_register_names[] =
{
  /* These registers must appear in consecutive RAW register number
     order and they must begin with AARCH64_SVE_Z0_REGNUM! */
  "z0", "z1", "z2", "z3",
  "z4", "z5", "z6", "z7",
  "z8", "z9", "z10", "z11",
  "z12", "z13", "z14", "z15",
  "z16", "z17", "z18", "z19",
  "z20", "z21", "z22", "z23",
  "z24", "z25", "z26", "z27",
  "z28", "z29", "z30", "z31",
  "fpsr", "fpcr",
  "p0", "p1", "p2", "p3",
  "p4", "p5", "p6", "p7",
  "p8", "p9", "p10", "p11",
  "p12", "p13", "p14", "p15",
  "ffr", "vg"
};

static const char *const aarch64_pauth_register_names[] =
{
  /* Authentication mask for data pointer, low half/user pointers.  */
  "pauth_dmask",
  /* Authentication mask for code pointer, low half/user pointers.  */
  "pauth_cmask",
  /* Authentication mask for data pointer, high half / kernel pointers.  */
  "pauth_dmask_high",
  /* Authentication mask for code pointer, high half / kernel pointers.  */
  "pauth_cmask_high"
};

static const char *const aarch64_mte_register_names[] =
{
  /* Tag Control Register.  */
  "tag_ctl"
};

static int aarch64_stack_frame_destroyed_p (struct gdbarch *, CORE_ADDR);

/* AArch64 prologue cache structure.  */
struct aarch64_prologue_cache
{
  /* The program counter at the start of the function.  It is used to
     identify this frame as a prologue frame.  */
  CORE_ADDR func;

  /* The program counter at the time this frame was created; i.e. where
     this function was called from.  It is used to identify this frame as a
     stub frame.  */
  CORE_ADDR prev_pc;

  /* The stack pointer at the time this frame was created; i.e. the
     caller's stack pointer when this function was called.  It is used
     to identify this frame.  */
  CORE_ADDR prev_sp;

  /* Is the target available to read from?  */
  int available_p;

  /* The frame base for this frame is just prev_sp - frame size.
     FRAMESIZE is the distance from the frame pointer to the
     initial stack pointer.  */
  int framesize;

  /* The register used to hold the frame pointer for this frame.  */
  int framereg;

  /* Saved register offsets.  */
  trad_frame_saved_reg *saved_regs;
};

/* Holds information used to read/write from/to ZA
   pseudo-registers.

   With this information, the read/write code can be simplified so it
   deals only with the required information to map a ZA pseudo-register
   to the exact bytes into the ZA contents buffer.  Otherwise we'd need
   to use a lot of conditionals.  */

struct za_offsets
{
  /* Offset, into ZA, of the starting byte of the pseudo-register.  */
  size_t starting_offset;
  /* The size of the contiguous chunks of the pseudo-register.  */
  size_t chunk_size;
  /* The number of pseudo-register chunks contained in ZA.  */
  size_t chunks;
  /* The offset between each contiguous chunk.  */
  size_t stride_size;
};

/* Holds data that is helpful to determine the individual fields that make
   up the names of the ZA pseudo-registers.  It is also very helpful to
   determine offsets, stride and sizes for reading ZA tiles and tile
   slices.  */

struct za_pseudo_encoding
{
  /* The slice index (0 ~ svl).  Only used for tile slices.  */
  uint8_t slice_index;
  /* The tile number (0 ~ 15).  */
  uint8_t tile_index;
  /* Direction (horizontal/vertical).  Only used for tile slices.  */
  bool horizontal;
  /* Qualifier index (0 ~ 4).  These map to B, H, S, D and Q.  */
  uint8_t qualifier_index;
};

static void
show_aarch64_debug (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("AArch64 debugging is %s.\n"), value);
}

namespace {

/* Abstract instruction reader.  */

class abstract_instruction_reader
{
public:
  /* Read in one instruction.  */
  virtual ULONGEST read (CORE_ADDR memaddr, int len,
			 enum bfd_endian byte_order) = 0;
};

/* Instruction reader from real target.  */

class instruction_reader : public abstract_instruction_reader
{
 public:
  ULONGEST read (CORE_ADDR memaddr, int len, enum bfd_endian byte_order)
    override
  {
    return read_code_unsigned_integer (memaddr, len, byte_order);
  }
};

} // namespace

/* If address signing is enabled, mask off the signature bits from the link
   register, which is passed by value in ADDR, using the register values in
   THIS_FRAME.  */

static CORE_ADDR
aarch64_frame_unmask_lr (aarch64_gdbarch_tdep *tdep,
			 frame_info_ptr this_frame, CORE_ADDR addr)
{
  if (tdep->has_pauth ()
      && frame_unwind_register_unsigned (this_frame,
					 tdep->ra_sign_state_regnum))
    {
      /* VA range select (bit 55) tells us whether to use the low half masks
	 or the high half masks.  */
      int cmask_num;
      if (tdep->pauth_reg_count > 2 && addr & VA_RANGE_SELECT_BIT_MASK)
	cmask_num = AARCH64_PAUTH_CMASK_HIGH_REGNUM (tdep->pauth_reg_base);
      else
	cmask_num = AARCH64_PAUTH_CMASK_REGNUM (tdep->pauth_reg_base);

      /* By default, we assume TBI and discard the top 8 bits plus the VA range
	 select bit (55).  */
      CORE_ADDR mask = AARCH64_TOP_BITS_MASK;
      mask |= frame_unwind_register_unsigned (this_frame, cmask_num);
      addr = aarch64_remove_top_bits (addr, mask);

      /* Record in the frame that the link register required unmasking.  */
      set_frame_previous_pc_masked (this_frame);
    }

  return addr;
}

/* Implement the "get_pc_address_flags" gdbarch method.  */

static std::string
aarch64_get_pc_address_flags (frame_info_ptr frame, CORE_ADDR pc)
{
  if (pc != 0 && get_frame_pc_masked (frame))
    return "PAC";

  return "";
}

/* Analyze a prologue, looking for a recognizable stack frame
   and frame pointer.  Scan until we encounter a store that could
   clobber the stack frame unexpectedly, or an unknown instruction.  */

static CORE_ADDR
aarch64_analyze_prologue (struct gdbarch *gdbarch,
			  CORE_ADDR start, CORE_ADDR limit,
			  struct aarch64_prologue_cache *cache,
			  abstract_instruction_reader& reader)
{
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  int i;

  /* Whether the stack has been set.  This should be true when we notice a SP
     to FP move or if we are using the SP as the base register for storing
     data, in case the FP is omitted.  */
  bool seen_stack_set = false;

  /* Track X registers and D registers in prologue.  */
  pv_t regs[AARCH64_X_REGISTER_COUNT + AARCH64_D_REGISTER_COUNT];

  for (i = 0; i < AARCH64_X_REGISTER_COUNT + AARCH64_D_REGISTER_COUNT; i++)
    regs[i] = pv_register (i, 0);
  pv_area stack (AARCH64_SP_REGNUM, gdbarch_addr_bit (gdbarch));

  for (; start < limit; start += 4)
    {
      uint32_t insn;
      aarch64_inst inst;

      insn = reader.read (start, 4, byte_order_for_code);

      if (aarch64_decode_insn (insn, &inst, 1, NULL) != 0)
	break;

      if (inst.opcode->iclass == addsub_imm
	  && (inst.opcode->op == OP_ADD
	      || strcmp ("sub", inst.opcode->name) == 0))
	{
	  unsigned rd = inst.operands[0].reg.regno;
	  unsigned rn = inst.operands[1].reg.regno;

	  gdb_assert (aarch64_num_of_operands (inst.opcode) == 3);
	  gdb_assert (inst.operands[0].type == AARCH64_OPND_Rd_SP);
	  gdb_assert (inst.operands[1].type == AARCH64_OPND_Rn_SP);
	  gdb_assert (inst.operands[2].type == AARCH64_OPND_AIMM);

	  if (inst.opcode->op == OP_ADD)
	    {
	      regs[rd] = pv_add_constant (regs[rn],
					  inst.operands[2].imm.value);
	    }
	  else
	    {
	      regs[rd] = pv_add_constant (regs[rn],
					  -inst.operands[2].imm.value);
	    }

	  /* Did we move SP to FP?  */
	  if (rn == AARCH64_SP_REGNUM && rd == AARCH64_FP_REGNUM)
	    seen_stack_set = true;
	}
      else if (inst.opcode->iclass == addsub_ext
	       && strcmp ("sub", inst.opcode->name) == 0)
	{
	  unsigned rd = inst.operands[0].reg.regno;
	  unsigned rn = inst.operands[1].reg.regno;
	  unsigned rm = inst.operands[2].reg.regno;

	  gdb_assert (aarch64_num_of_operands (inst.opcode) == 3);
	  gdb_assert (inst.operands[0].type == AARCH64_OPND_Rd_SP);
	  gdb_assert (inst.operands[1].type == AARCH64_OPND_Rn_SP);
	  gdb_assert (inst.operands[2].type == AARCH64_OPND_Rm_EXT);

	  regs[rd] = pv_subtract (regs[rn], regs[rm]);
	}
      else if (inst.opcode->iclass == branch_imm)
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else if (inst.opcode->iclass == condbranch)
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else if (inst.opcode->iclass == branch_reg)
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else if (inst.opcode->iclass == compbranch)
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else if (inst.opcode->op == OP_MOVZ)
	{
	  unsigned rd = inst.operands[0].reg.regno;

	  gdb_assert (aarch64_num_of_operands (inst.opcode) == 2);
	  gdb_assert (inst.operands[0].type == AARCH64_OPND_Rd);
	  gdb_assert (inst.operands[1].type == AARCH64_OPND_HALF);
	  gdb_assert (inst.operands[1].shifter.kind == AARCH64_MOD_LSL);

	  /* If this shows up before we set the stack, keep going.  Otherwise
	     stop the analysis.  */
	  if (seen_stack_set)
	    break;

	  regs[rd] = pv_constant (inst.operands[1].imm.value
				  << inst.operands[1].shifter.amount);
	}
      else if (inst.opcode->iclass == log_shift
	       && strcmp (inst.opcode->name, "orr") == 0)
	{
	  unsigned rd = inst.operands[0].reg.regno;
	  unsigned rn = inst.operands[1].reg.regno;
	  unsigned rm = inst.operands[2].reg.regno;

	  gdb_assert (inst.operands[0].type == AARCH64_OPND_Rd);
	  gdb_assert (inst.operands[1].type == AARCH64_OPND_Rn);
	  gdb_assert (inst.operands[2].type == AARCH64_OPND_Rm_SFT);

	  if (inst.operands[2].shifter.amount == 0
	      && rn == AARCH64_SP_REGNUM)
	    regs[rd] = regs[rm];
	  else
	    {
	      aarch64_debug_printf ("prologue analysis gave up "
				    "addr=%s opcode=0x%x (orr x register)",
				    core_addr_to_string_nz (start), insn);

	      break;
	    }
	}
      else if (inst.opcode->op == OP_STUR)
	{
	  unsigned rt = inst.operands[0].reg.regno;
	  unsigned rn = inst.operands[1].addr.base_regno;
	  int size = aarch64_get_qualifier_esize (inst.operands[0].qualifier);

	  gdb_assert (aarch64_num_of_operands (inst.opcode) == 2);
	  gdb_assert (inst.operands[0].type == AARCH64_OPND_Rt);
	  gdb_assert (inst.operands[1].type == AARCH64_OPND_ADDR_SIMM9);
	  gdb_assert (!inst.operands[1].addr.offset.is_reg);

	  stack.store
	    (pv_add_constant (regs[rn], inst.operands[1].addr.offset.imm),
	     size, regs[rt]);

	  /* Are we storing with SP as a base?  */
	  if (rn == AARCH64_SP_REGNUM)
	    seen_stack_set = true;
	}
      else if ((inst.opcode->iclass == ldstpair_off
		|| (inst.opcode->iclass == ldstpair_indexed
		    && inst.operands[2].addr.preind))
	       && strcmp ("stp", inst.opcode->name) == 0)
	{
	  /* STP with addressing mode Pre-indexed and Base register.  */
	  unsigned rt1;
	  unsigned rt2;
	  unsigned rn = inst.operands[2].addr.base_regno;
	  int32_t imm = inst.operands[2].addr.offset.imm;
	  int size = aarch64_get_qualifier_esize (inst.operands[0].qualifier);

	  gdb_assert (inst.operands[0].type == AARCH64_OPND_Rt
		      || inst.operands[0].type == AARCH64_OPND_Ft);
	  gdb_assert (inst.operands[1].type == AARCH64_OPND_Rt2
		      || inst.operands[1].type == AARCH64_OPND_Ft2);
	  gdb_assert (inst.operands[2].type == AARCH64_OPND_ADDR_SIMM7);
	  gdb_assert (!inst.operands[2].addr.offset.is_reg);

	  /* If recording this store would invalidate the store area
	     (perhaps because rn is not known) then we should abandon
	     further prologue analysis.  */
	  if (stack.store_would_trash (pv_add_constant (regs[rn], imm)))
	    break;

	  if (stack.store_would_trash (pv_add_constant (regs[rn], imm + 8)))
	    break;

	  rt1 = inst.operands[0].reg.regno;
	  rt2 = inst.operands[1].reg.regno;
	  if (inst.operands[0].type == AARCH64_OPND_Ft)
	    {
	      rt1 += AARCH64_X_REGISTER_COUNT;
	      rt2 += AARCH64_X_REGISTER_COUNT;
	    }

	  stack.store (pv_add_constant (regs[rn], imm), size, regs[rt1]);
	  stack.store (pv_add_constant (regs[rn], imm + size), size, regs[rt2]);

	  if (inst.operands[2].addr.writeback)
	    regs[rn] = pv_add_constant (regs[rn], imm);

	  /* Ignore the instruction that allocates stack space and sets
	     the SP.  */
	  if (rn == AARCH64_SP_REGNUM && !inst.operands[2].addr.writeback)
	    seen_stack_set = true;
	}
      else if ((inst.opcode->iclass == ldst_imm9 /* Signed immediate.  */
		|| (inst.opcode->iclass == ldst_pos /* Unsigned immediate.  */
		    && (inst.opcode->op == OP_STR_POS
			|| inst.opcode->op == OP_STRF_POS)))
	       && inst.operands[1].addr.base_regno == AARCH64_SP_REGNUM
	       && strcmp ("str", inst.opcode->name) == 0)
	{
	  /* STR (immediate) */
	  unsigned int rt = inst.operands[0].reg.regno;
	  int32_t imm = inst.operands[1].addr.offset.imm;
	  unsigned int rn = inst.operands[1].addr.base_regno;
	  int size = aarch64_get_qualifier_esize (inst.operands[0].qualifier);
	  gdb_assert (inst.operands[0].type == AARCH64_OPND_Rt
		      || inst.operands[0].type == AARCH64_OPND_Ft);

	  if (inst.operands[0].type == AARCH64_OPND_Ft)
	    rt += AARCH64_X_REGISTER_COUNT;

	  stack.store (pv_add_constant (regs[rn], imm), size, regs[rt]);
	  if (inst.operands[1].addr.writeback)
	    regs[rn] = pv_add_constant (regs[rn], imm);

	  /* Are we storing with SP as a base?  */
	  if (rn == AARCH64_SP_REGNUM)
	    seen_stack_set = true;
	}
      else if (inst.opcode->iclass == testbranch)
	{
	  /* Stop analysis on branch.  */
	  break;
	}
      else if (inst.opcode->iclass == ic_system)
	{
	  aarch64_gdbarch_tdep *tdep
	    = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);
	  int ra_state_val = 0;

	  if (insn == 0xd503233f /* paciasp.  */
	      || insn == 0xd503237f  /* pacibsp.  */)
	    {
	      /* Return addresses are mangled.  */
	      ra_state_val = 1;
	    }
	  else if (insn == 0xd50323bf /* autiasp.  */
		   || insn == 0xd50323ff /* autibsp.  */)
	    {
	      /* Return addresses are not mangled.  */
	      ra_state_val = 0;
	    }
	  else if (IS_BTI (insn))
	    /* We don't need to do anything special for a BTI instruction.  */
	    continue;
	  else
	    {
	      aarch64_debug_printf ("prologue analysis gave up addr=%s"
				    " opcode=0x%x (iclass)",
				    core_addr_to_string_nz (start), insn);
	      break;
	    }

	  if (tdep->has_pauth () && cache != nullptr)
	    {
	      int regnum = tdep->ra_sign_state_regnum;
	      cache->saved_regs[regnum].set_value (ra_state_val);
	    }
	}
      else
	{
	  aarch64_debug_printf ("prologue analysis gave up addr=%s"
				" opcode=0x%x",
				core_addr_to_string_nz (start), insn);

	  break;
	}
    }

  if (cache == NULL)
    return start;

  if (pv_is_register (regs[AARCH64_FP_REGNUM], AARCH64_SP_REGNUM))
    {
      /* Frame pointer is fp.  Frame size is constant.  */
      cache->framereg = AARCH64_FP_REGNUM;
      cache->framesize = -regs[AARCH64_FP_REGNUM].k;
    }
  else if (pv_is_register (regs[AARCH64_SP_REGNUM], AARCH64_SP_REGNUM))
    {
      /* Try the stack pointer.  */
      cache->framesize = -regs[AARCH64_SP_REGNUM].k;
      cache->framereg = AARCH64_SP_REGNUM;
    }
  else
    {
      /* We're just out of luck.  We don't know where the frame is.  */
      cache->framereg = -1;
      cache->framesize = 0;
    }

  for (i = 0; i < AARCH64_X_REGISTER_COUNT; i++)
    {
      CORE_ADDR offset;

      if (stack.find_reg (gdbarch, i, &offset))
	cache->saved_regs[i].set_addr (offset);
    }

  for (i = 0; i < AARCH64_D_REGISTER_COUNT; i++)
    {
      int regnum = gdbarch_num_regs (gdbarch);
      CORE_ADDR offset;

      if (stack.find_reg (gdbarch, i + AARCH64_X_REGISTER_COUNT,
			  &offset))
	cache->saved_regs[i + regnum + AARCH64_D0_REGNUM].set_addr (offset);
    }

  return start;
}

static CORE_ADDR
aarch64_analyze_prologue (struct gdbarch *gdbarch,
			  CORE_ADDR start, CORE_ADDR limit,
			  struct aarch64_prologue_cache *cache)
{
  instruction_reader reader;

  return aarch64_analyze_prologue (gdbarch, start, limit, cache,
				   reader);
}

#if GDB_SELF_TEST

namespace selftests {

/* Instruction reader from manually cooked instruction sequences.  */

class instruction_reader_test : public abstract_instruction_reader
{
public:
  template<size_t SIZE>
  explicit instruction_reader_test (const uint32_t (&insns)[SIZE])
  : m_insns (insns), m_insns_size (SIZE)
  {}

  ULONGEST read (CORE_ADDR memaddr, int len, enum bfd_endian byte_order)
    override
  {
    SELF_CHECK (len == 4);
    SELF_CHECK (memaddr % 4 == 0);
    SELF_CHECK (memaddr / 4 < m_insns_size);

    return m_insns[memaddr / 4];
  }

private:
  const uint32_t *m_insns;
  size_t m_insns_size;
};

static void
aarch64_analyze_prologue_test (void)
{
  struct gdbarch_info info;

  info.bfd_arch_info = bfd_scan_arch ("aarch64");

  struct gdbarch *gdbarch = gdbarch_find_by_info (info);
  SELF_CHECK (gdbarch != NULL);

  struct aarch64_prologue_cache cache;
  cache.saved_regs = trad_frame_alloc_saved_regs (gdbarch);

  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  /* Test the simple prologue in which frame pointer is used.  */
  {
    static const uint32_t insns[] = {
      0xa9af7bfd, /* stp     x29, x30, [sp,#-272]! */
      0x910003fd, /* mov     x29, sp */
      0x97ffffe6, /* bl      0x400580 */
    };
    instruction_reader_test reader (insns);

    CORE_ADDR end = aarch64_analyze_prologue (gdbarch, 0, 128, &cache, reader);
    SELF_CHECK (end == 4 * 2);

    SELF_CHECK (cache.framereg == AARCH64_FP_REGNUM);
    SELF_CHECK (cache.framesize == 272);

    for (int i = 0; i < AARCH64_X_REGISTER_COUNT; i++)
      {
	if (i == AARCH64_FP_REGNUM)
	  SELF_CHECK (cache.saved_regs[i].addr () == -272);
	else if (i == AARCH64_LR_REGNUM)
	  SELF_CHECK (cache.saved_regs[i].addr () == -264);
	else
	  SELF_CHECK (cache.saved_regs[i].is_realreg ()
		      && cache.saved_regs[i].realreg () == i);
      }

    for (int i = 0; i < AARCH64_D_REGISTER_COUNT; i++)
      {
	int num_regs = gdbarch_num_regs (gdbarch);
	int regnum = i + num_regs + AARCH64_D0_REGNUM;

	SELF_CHECK (cache.saved_regs[regnum].is_realreg ()
		    && cache.saved_regs[regnum].realreg () == regnum);
      }
  }

  /* Test a prologue in which STR is used and frame pointer is not
     used.  */
  {
    static const uint32_t insns[] = {
      0xf81d0ff3, /* str	x19, [sp, #-48]! */
      0xb9002fe0, /* str	w0, [sp, #44] */
      0xf90013e1, /* str	x1, [sp, #32]*/
      0xfd000fe0, /* str	d0, [sp, #24] */
      0xaa0203f3, /* mov	x19, x2 */
      0xf94013e0, /* ldr	x0, [sp, #32] */
    };
    instruction_reader_test reader (insns);

    trad_frame_reset_saved_regs (gdbarch, cache.saved_regs);
    CORE_ADDR end = aarch64_analyze_prologue (gdbarch, 0, 128, &cache, reader);

    SELF_CHECK (end == 4 * 5);

    SELF_CHECK (cache.framereg == AARCH64_SP_REGNUM);
    SELF_CHECK (cache.framesize == 48);

    for (int i = 0; i < AARCH64_X_REGISTER_COUNT; i++)
      {
	if (i == 1)
	  SELF_CHECK (cache.saved_regs[i].addr () == -16);
	else if (i == 19)
	  SELF_CHECK (cache.saved_regs[i].addr () == -48);
	else
	  SELF_CHECK (cache.saved_regs[i].is_realreg ()
		      && cache.saved_regs[i].realreg () == i);
      }

    for (int i = 0; i < AARCH64_D_REGISTER_COUNT; i++)
      {
	int num_regs = gdbarch_num_regs (gdbarch);
	int regnum = i + num_regs + AARCH64_D0_REGNUM;


	if (i == 0)
	  SELF_CHECK (cache.saved_regs[regnum].addr () == -24);
	else
	  SELF_CHECK (cache.saved_regs[regnum].is_realreg ()
		      && cache.saved_regs[regnum].realreg () == regnum);
      }
  }

  /* Test handling of movz before setting the frame pointer.  */
  {
    static const uint32_t insns[] = {
      0xa9bf7bfd, /* stp     x29, x30, [sp, #-16]! */
      0x52800020, /* mov     w0, #0x1 */
      0x910003fd, /* mov     x29, sp */
      0x528000a2, /* mov     w2, #0x5 */
      0x97fffff8, /* bl      6e4 */
    };

    instruction_reader_test reader (insns);

    trad_frame_reset_saved_regs (gdbarch, cache.saved_regs);
    CORE_ADDR end = aarch64_analyze_prologue (gdbarch, 0, 128, &cache, reader);

    /* We should stop at the 4th instruction.  */
    SELF_CHECK (end == (4 - 1) * 4);
    SELF_CHECK (cache.framereg == AARCH64_FP_REGNUM);
    SELF_CHECK (cache.framesize == 16);
  }

  /* Test handling of movz/stp when using the stack pointer as frame
     pointer.  */
  {
    static const uint32_t insns[] = {
      0xa9bc7bfd, /* stp     x29, x30, [sp, #-64]! */
      0x52800020, /* mov     w0, #0x1 */
      0x290207e0, /* stp     w0, w1, [sp, #16] */
      0xa9018fe2, /* stp     x2, x3, [sp, #24] */
      0x528000a2, /* mov     w2, #0x5 */
      0x97fffff8, /* bl      6e4 */
    };

    instruction_reader_test reader (insns);

    trad_frame_reset_saved_regs (gdbarch, cache.saved_regs);
    CORE_ADDR end = aarch64_analyze_prologue (gdbarch, 0, 128, &cache, reader);

    /* We should stop at the 5th instruction.  */
    SELF_CHECK (end == (5 - 1) * 4);
    SELF_CHECK (cache.framereg == AARCH64_SP_REGNUM);
    SELF_CHECK (cache.framesize == 64);
  }

  /* Test handling of movz/str when using the stack pointer as frame
     pointer  */
  {
    static const uint32_t insns[] = {
      0xa9bc7bfd, /* stp     x29, x30, [sp, #-64]! */
      0x52800020, /* mov     w0, #0x1 */
      0xb9002be4, /* str     w4, [sp, #40] */
      0xf9001be5, /* str     x5, [sp, #48] */
      0x528000a2, /* mov     w2, #0x5 */
      0x97fffff8, /* bl      6e4 */
    };

    instruction_reader_test reader (insns);

    trad_frame_reset_saved_regs (gdbarch, cache.saved_regs);
    CORE_ADDR end = aarch64_analyze_prologue (gdbarch, 0, 128, &cache, reader);

    /* We should stop at the 5th instruction.  */
    SELF_CHECK (end == (5 - 1) * 4);
    SELF_CHECK (cache.framereg == AARCH64_SP_REGNUM);
    SELF_CHECK (cache.framesize == 64);
  }

  /* Test handling of movz/stur when using the stack pointer as frame
     pointer.  */
  {
    static const uint32_t insns[] = {
      0xa9bc7bfd, /* stp     x29, x30, [sp, #-64]! */
      0x52800020, /* mov     w0, #0x1 */
      0xb80343e6, /* stur    w6, [sp, #52] */
      0xf80383e7, /* stur    x7, [sp, #56] */
      0x528000a2, /* mov     w2, #0x5 */
      0x97fffff8, /* bl      6e4 */
    };

    instruction_reader_test reader (insns);

    trad_frame_reset_saved_regs (gdbarch, cache.saved_regs);
    CORE_ADDR end = aarch64_analyze_prologue (gdbarch, 0, 128, &cache, reader);

    /* We should stop at the 5th instruction.  */
    SELF_CHECK (end == (5 - 1) * 4);
    SELF_CHECK (cache.framereg == AARCH64_SP_REGNUM);
    SELF_CHECK (cache.framesize == 64);
  }

  /* Test handling of movz when there is no frame pointer set or no stack
     pointer used.  */
  {
    static const uint32_t insns[] = {
      0xa9bf7bfd, /* stp     x29, x30, [sp, #-16]! */
      0x52800020, /* mov     w0, #0x1 */
      0x528000a2, /* mov     w2, #0x5 */
      0x97fffff8, /* bl      6e4 */
    };

    instruction_reader_test reader (insns);

    trad_frame_reset_saved_regs (gdbarch, cache.saved_regs);
    CORE_ADDR end = aarch64_analyze_prologue (gdbarch, 0, 128, &cache, reader);

    /* We should stop at the 4th instruction.  */
    SELF_CHECK (end == (4 - 1) * 4);
    SELF_CHECK (cache.framereg == AARCH64_SP_REGNUM);
    SELF_CHECK (cache.framesize == 16);
  }

  /* Test a prologue in which there is a return address signing instruction.  */
  if (tdep->has_pauth ())
    {
      static const uint32_t insns[] = {
	0xd503233f, /* paciasp */
	0xa9bd7bfd, /* stp	x29, x30, [sp, #-48]! */
	0x910003fd, /* mov	x29, sp */
	0xf801c3f3, /* str	x19, [sp, #28] */
	0xb9401fa0, /* ldr	x19, [x29, #28] */
      };
      instruction_reader_test reader (insns);

      trad_frame_reset_saved_regs (gdbarch, cache.saved_regs);
      CORE_ADDR end = aarch64_analyze_prologue (gdbarch, 0, 128, &cache,
						reader);

      SELF_CHECK (end == 4 * 4);
      SELF_CHECK (cache.framereg == AARCH64_FP_REGNUM);
      SELF_CHECK (cache.framesize == 48);

      for (int i = 0; i < AARCH64_X_REGISTER_COUNT; i++)
	{
	  if (i == 19)
	    SELF_CHECK (cache.saved_regs[i].addr () == -20);
	  else if (i == AARCH64_FP_REGNUM)
	    SELF_CHECK (cache.saved_regs[i].addr () == -48);
	  else if (i == AARCH64_LR_REGNUM)
	    SELF_CHECK (cache.saved_regs[i].addr () == -40);
	  else
	    SELF_CHECK (cache.saved_regs[i].is_realreg ()
			&& cache.saved_regs[i].realreg () == i);
	}

      if (tdep->has_pauth ())
	{
	  int regnum = tdep->ra_sign_state_regnum;
	  SELF_CHECK (cache.saved_regs[regnum].is_value ());
	}
    }

  /* Test a prologue with a BTI instruction.  */
    {
      static const uint32_t insns[] = {
	0xd503245f, /* bti */
	0xa9bd7bfd, /* stp	x29, x30, [sp, #-48]! */
	0x910003fd, /* mov	x29, sp */
	0xf801c3f3, /* str	x19, [sp, #28] */
	0xb9401fa0, /* ldr	x19, [x29, #28] */
      };
      instruction_reader_test reader (insns);

      trad_frame_reset_saved_regs (gdbarch, cache.saved_regs);
      CORE_ADDR end = aarch64_analyze_prologue (gdbarch, 0, 128, &cache,
						reader);

      SELF_CHECK (end == 4 * 4);
      SELF_CHECK (cache.framereg == AARCH64_FP_REGNUM);
      SELF_CHECK (cache.framesize == 48);

      for (int i = 0; i < AARCH64_X_REGISTER_COUNT; i++)
	{
	  if (i == 19)
	    SELF_CHECK (cache.saved_regs[i].addr () == -20);
	  else if (i == AARCH64_FP_REGNUM)
	    SELF_CHECK (cache.saved_regs[i].addr () == -48);
	  else if (i == AARCH64_LR_REGNUM)
	    SELF_CHECK (cache.saved_regs[i].addr () == -40);
	  else
	    SELF_CHECK (cache.saved_regs[i].is_realreg ()
			&& cache.saved_regs[i].realreg () == i);
	}
    }
}
} // namespace selftests
#endif /* GDB_SELF_TEST */

/* Implement the "skip_prologue" gdbarch method.  */

static CORE_ADDR
aarch64_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr, func_end_addr, limit_pc;

  /* See if we can determine the end of the prologue via the symbol
     table.  If so, then return either PC, or the PC after the
     prologue, whichever is greater.  */
  bool func_addr_found
    = find_pc_partial_function (pc, NULL, &func_addr, &func_end_addr);

  if (func_addr_found)
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);

      if (post_prologue_pc != 0)
	return std::max (pc, post_prologue_pc);
    }

  /* Can't determine prologue from the symbol table, need to examine
     instructions.  */

  /* Find an upper limit on the function prologue using the debug
     information.  If the debug information could not be used to
     provide that bound, then use an arbitrary large number as the
     upper bound.  */
  limit_pc = skip_prologue_using_sal (gdbarch, pc);
  if (limit_pc == 0)
    limit_pc = pc + 128;	/* Magic.  */

  limit_pc
    = func_end_addr == 0 ? limit_pc : std::min (limit_pc, func_end_addr - 4);

  /* Try disassembling prologue.  */
  return aarch64_analyze_prologue (gdbarch, pc, limit_pc, NULL);
}

/* Scan the function prologue for THIS_FRAME and populate the prologue
   cache CACHE.  */

static void
aarch64_scan_prologue (frame_info_ptr this_frame,
		       struct aarch64_prologue_cache *cache)
{
  CORE_ADDR block_addr = get_frame_address_in_block (this_frame);
  CORE_ADDR prologue_start;
  CORE_ADDR prologue_end;
  CORE_ADDR prev_pc = get_frame_pc (this_frame);
  struct gdbarch *gdbarch = get_frame_arch (this_frame);

  cache->prev_pc = prev_pc;

  /* Assume we do not find a frame.  */
  cache->framereg = -1;
  cache->framesize = 0;

  if (find_pc_partial_function (block_addr, NULL, &prologue_start,
				&prologue_end))
    {
      struct symtab_and_line sal = find_pc_line (prologue_start, 0);

      if (sal.line == 0)
	{
	  /* No line info so use the current PC.  */
	  prologue_end = prev_pc;
	}
      else if (sal.end < prologue_end)
	{
	  /* The next line begins after the function end.  */
	  prologue_end = sal.end;
	}

      prologue_end = std::min (prologue_end, prev_pc);
      aarch64_analyze_prologue (gdbarch, prologue_start, prologue_end, cache);
    }
  else
    {
      CORE_ADDR frame_loc;

      frame_loc = get_frame_register_unsigned (this_frame, AARCH64_FP_REGNUM);
      if (frame_loc == 0)
	return;

      cache->framereg = AARCH64_FP_REGNUM;
      cache->framesize = 16;
      cache->saved_regs[29].set_addr (0);
      cache->saved_regs[30].set_addr (8);
    }
}

/* Fill in *CACHE with information about the prologue of *THIS_FRAME.  This
   function may throw an exception if the inferior's registers or memory is
   not available.  */

static void
aarch64_make_prologue_cache_1 (frame_info_ptr this_frame,
			       struct aarch64_prologue_cache *cache)
{
  CORE_ADDR unwound_fp;
  int reg;

  aarch64_scan_prologue (this_frame, cache);

  if (cache->framereg == -1)
    return;

  unwound_fp = get_frame_register_unsigned (this_frame, cache->framereg);
  if (unwound_fp == 0)
    return;

  cache->prev_sp = unwound_fp;
  if (!aarch64_stack_frame_destroyed_p (get_frame_arch (this_frame),
					cache->prev_pc))
    cache->prev_sp += cache->framesize;

  /* Calculate actual addresses of saved registers using offsets
     determined by aarch64_analyze_prologue.  */
  for (reg = 0; reg < gdbarch_num_regs (get_frame_arch (this_frame)); reg++)
    if (cache->saved_regs[reg].is_addr ())
      cache->saved_regs[reg].set_addr (cache->saved_regs[reg].addr ()
				       + cache->prev_sp);

  cache->func = get_frame_func (this_frame);

  cache->available_p = 1;
}

/* Allocate and fill in *THIS_CACHE with information about the prologue of
   *THIS_FRAME.  Do not do this is if *THIS_CACHE was already allocated.
   Return a pointer to the current aarch64_prologue_cache in
   *THIS_CACHE.  */

static struct aarch64_prologue_cache *
aarch64_make_prologue_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct aarch64_prologue_cache *cache;

  if (*this_cache != NULL)
    return (struct aarch64_prologue_cache *) *this_cache;

  cache = FRAME_OBSTACK_ZALLOC (struct aarch64_prologue_cache);
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);
  *this_cache = cache;

  try
    {
      aarch64_make_prologue_cache_1 (this_frame, cache);
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error != NOT_AVAILABLE_ERROR)
	throw;
    }

  return cache;
}

/* Implement the "stop_reason" frame_unwind method.  */

static enum unwind_stop_reason
aarch64_prologue_frame_unwind_stop_reason (frame_info_ptr this_frame,
					   void **this_cache)
{
  struct aarch64_prologue_cache *cache
    = aarch64_make_prologue_cache (this_frame, this_cache);

  if (!cache->available_p)
    return UNWIND_UNAVAILABLE;

  /* Halt the backtrace at "_start".  */
  gdbarch *arch = get_frame_arch (this_frame);
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (arch);
  if (cache->prev_pc <= tdep->lowest_pc)
    return UNWIND_OUTERMOST;

  /* We've hit a wall, stop.  */
  if (cache->prev_sp == 0)
    return UNWIND_OUTERMOST;

  return UNWIND_NO_REASON;
}

/* Our frame ID for a normal frame is the current function's starting
   PC and the caller's SP when we were called.  */

static void
aarch64_prologue_this_id (frame_info_ptr this_frame,
			  void **this_cache, struct frame_id *this_id)
{
  struct aarch64_prologue_cache *cache
    = aarch64_make_prologue_cache (this_frame, this_cache);

  if (!cache->available_p)
    *this_id = frame_id_build_unavailable_stack (cache->func);
  else
    *this_id = frame_id_build (cache->prev_sp, cache->func);
}

/* Implement the "prev_register" frame_unwind method.  */

static struct value *
aarch64_prologue_prev_register (frame_info_ptr this_frame,
				void **this_cache, int prev_regnum)
{
  struct aarch64_prologue_cache *cache
    = aarch64_make_prologue_cache (this_frame, this_cache);

  /* If we are asked to unwind the PC, then we need to return the LR
     instead.  The prologue may save PC, but it will point into this
     frame's prologue, not the next frame's resume location.  */
  if (prev_regnum == AARCH64_PC_REGNUM)
    {
      CORE_ADDR lr;
      struct gdbarch *gdbarch = get_frame_arch (this_frame);
      aarch64_gdbarch_tdep *tdep
	= gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

      lr = frame_unwind_register_unsigned (this_frame, AARCH64_LR_REGNUM);

      if (tdep->has_pauth ()
	  && cache->saved_regs[tdep->ra_sign_state_regnum].is_value ())
	lr = aarch64_frame_unmask_lr (tdep, this_frame, lr);

      return frame_unwind_got_constant (this_frame, prev_regnum, lr);
    }

  /* SP is generally not saved to the stack, but this frame is
     identified by the next frame's stack pointer at the time of the
     call.  The value was already reconstructed into PREV_SP.  */
  /*
	 +----------+  ^
	 | saved lr |  |
      +->| saved fp |--+
      |  |          |
      |  |          |     <- Previous SP
      |  +----------+
      |  | saved lr |
      +--| saved fp |<- FP
	 |          |
	 |          |<- SP
	 +----------+  */
  if (prev_regnum == AARCH64_SP_REGNUM)
    return frame_unwind_got_constant (this_frame, prev_regnum,
				      cache->prev_sp);

  return trad_frame_get_prev_register (this_frame, cache->saved_regs,
				       prev_regnum);
}

/* AArch64 prologue unwinder.  */
static frame_unwind aarch64_prologue_unwind =
{
  "aarch64 prologue",
  NORMAL_FRAME,
  aarch64_prologue_frame_unwind_stop_reason,
  aarch64_prologue_this_id,
  aarch64_prologue_prev_register,
  NULL,
  default_frame_sniffer
};

/* Allocate and fill in *THIS_CACHE with information about the prologue of
   *THIS_FRAME.  Do not do this is if *THIS_CACHE was already allocated.
   Return a pointer to the current aarch64_prologue_cache in
   *THIS_CACHE.  */

static struct aarch64_prologue_cache *
aarch64_make_stub_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct aarch64_prologue_cache *cache;

  if (*this_cache != NULL)
    return (struct aarch64_prologue_cache *) *this_cache;

  cache = FRAME_OBSTACK_ZALLOC (struct aarch64_prologue_cache);
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);
  *this_cache = cache;

  try
    {
      cache->prev_sp = get_frame_register_unsigned (this_frame,
						    AARCH64_SP_REGNUM);
      cache->prev_pc = get_frame_pc (this_frame);
      cache->available_p = 1;
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error != NOT_AVAILABLE_ERROR)
	throw;
    }

  return cache;
}

/* Implement the "stop_reason" frame_unwind method.  */

static enum unwind_stop_reason
aarch64_stub_frame_unwind_stop_reason (frame_info_ptr this_frame,
				       void **this_cache)
{
  struct aarch64_prologue_cache *cache
    = aarch64_make_stub_cache (this_frame, this_cache);

  if (!cache->available_p)
    return UNWIND_UNAVAILABLE;

  return UNWIND_NO_REASON;
}

/* Our frame ID for a stub frame is the current SP and LR.  */

static void
aarch64_stub_this_id (frame_info_ptr this_frame,
		      void **this_cache, struct frame_id *this_id)
{
  struct aarch64_prologue_cache *cache
    = aarch64_make_stub_cache (this_frame, this_cache);

  if (cache->available_p)
    *this_id = frame_id_build (cache->prev_sp, cache->prev_pc);
  else
    *this_id = frame_id_build_unavailable_stack (cache->prev_pc);
}

/* Implement the "sniffer" frame_unwind method.  */

static int
aarch64_stub_unwind_sniffer (const struct frame_unwind *self,
			     frame_info_ptr this_frame,
			     void **this_prologue_cache)
{
  CORE_ADDR addr_in_block;
  gdb_byte dummy[4];

  addr_in_block = get_frame_address_in_block (this_frame);
  if (in_plt_section (addr_in_block)
      /* We also use the stub winder if the target memory is unreadable
	 to avoid having the prologue unwinder trying to read it.  */
      || target_read_memory (get_frame_pc (this_frame), dummy, 4) != 0)
    return 1;

  return 0;
}

/* AArch64 stub unwinder.  */
static frame_unwind aarch64_stub_unwind =
{
  "aarch64 stub",
  NORMAL_FRAME,
  aarch64_stub_frame_unwind_stop_reason,
  aarch64_stub_this_id,
  aarch64_prologue_prev_register,
  NULL,
  aarch64_stub_unwind_sniffer
};

/* Return the frame base address of *THIS_FRAME.  */

static CORE_ADDR
aarch64_normal_frame_base (frame_info_ptr this_frame, void **this_cache)
{
  struct aarch64_prologue_cache *cache
    = aarch64_make_prologue_cache (this_frame, this_cache);

  return cache->prev_sp - cache->framesize;
}

/* AArch64 default frame base information.  */
static frame_base aarch64_normal_base =
{
  &aarch64_prologue_unwind,
  aarch64_normal_frame_base,
  aarch64_normal_frame_base,
  aarch64_normal_frame_base
};

/* Return the value of the REGNUM register in the previous frame of
   *THIS_FRAME.  */

static struct value *
aarch64_dwarf2_prev_register (frame_info_ptr this_frame,
			      void **this_cache, int regnum)
{
  gdbarch *arch = get_frame_arch (this_frame);
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (arch);
  CORE_ADDR lr;

  switch (regnum)
    {
    case AARCH64_PC_REGNUM:
      lr = frame_unwind_register_unsigned (this_frame, AARCH64_LR_REGNUM);
      lr = aarch64_frame_unmask_lr (tdep, this_frame, lr);
      return frame_unwind_got_constant (this_frame, regnum, lr);

    default:
      internal_error (_("Unexpected register %d"), regnum);
    }
}

static const unsigned char op_lit0 = DW_OP_lit0;
static const unsigned char op_lit1 = DW_OP_lit1;

/* Implement the "init_reg" dwarf2_frame_ops method.  */

static void
aarch64_dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
			       struct dwarf2_frame_state_reg *reg,
			       frame_info_ptr this_frame)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  switch (regnum)
    {
    case AARCH64_PC_REGNUM:
      reg->how = DWARF2_FRAME_REG_FN;
      reg->loc.fn = aarch64_dwarf2_prev_register;
      return;

    case AARCH64_SP_REGNUM:
      reg->how = DWARF2_FRAME_REG_CFA;
      return;
    }

  /* Init pauth registers.  */
  if (tdep->has_pauth ())
    {
      if (regnum == tdep->ra_sign_state_regnum)
	{
	  /* Initialize RA_STATE to zero.  */
	  reg->how = DWARF2_FRAME_REG_SAVED_VAL_EXP;
	  reg->loc.exp.start = &op_lit0;
	  reg->loc.exp.len = 1;
	  return;
	}
      else if (regnum >= tdep->pauth_reg_base
	       && regnum < tdep->pauth_reg_base + tdep->pauth_reg_count)
	{
	  reg->how = DWARF2_FRAME_REG_SAME_VALUE;
	  return;
	}
    }
}

/* Implement the execute_dwarf_cfa_vendor_op method.  */

static bool
aarch64_execute_dwarf_cfa_vendor_op (struct gdbarch *gdbarch, gdb_byte op,
				     struct dwarf2_frame_state *fs)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);
  struct dwarf2_frame_state_reg *ra_state;

  if (op == DW_CFA_AARCH64_negate_ra_state)
    {
      /* On systems without pauth, treat as a nop.  */
      if (!tdep->has_pauth ())
	return true;

      /* Allocate RA_STATE column if it's not allocated yet.  */
      fs->regs.alloc_regs (AARCH64_DWARF_RA_SIGN_STATE + 1);

      /* Toggle the status of RA_STATE between 0 and 1.  */
      ra_state = &(fs->regs.reg[AARCH64_DWARF_RA_SIGN_STATE]);
      ra_state->how = DWARF2_FRAME_REG_SAVED_VAL_EXP;

      if (ra_state->loc.exp.start == nullptr
	  || ra_state->loc.exp.start == &op_lit0)
	ra_state->loc.exp.start = &op_lit1;
      else
	ra_state->loc.exp.start = &op_lit0;

      ra_state->loc.exp.len = 1;

      return true;
    }

  return false;
}

/* Used for matching BRK instructions for AArch64.  */
static constexpr uint32_t BRK_INSN_MASK = 0xffe0001f;
static constexpr uint32_t BRK_INSN_BASE = 0xd4200000;

/* Implementation of gdbarch_program_breakpoint_here_p for aarch64.  */

static bool
aarch64_program_breakpoint_here_p (gdbarch *gdbarch, CORE_ADDR address)
{
  const uint32_t insn_len = 4;
  gdb_byte target_mem[4];

  /* Enable the automatic memory restoration from breakpoints while
     we read the memory.  Otherwise we may find temporary breakpoints, ones
     inserted by GDB, and flag them as permanent breakpoints.  */
  scoped_restore restore_memory
    = make_scoped_restore_show_memory_breakpoints (0);

  if (target_read_memory (address, target_mem, insn_len) == 0)
    {
      uint32_t insn =
	(uint32_t) extract_unsigned_integer (target_mem, insn_len,
					     gdbarch_byte_order_for_code (gdbarch));

      /* Check if INSN is a BRK instruction pattern.  There are multiple choices
	 of such instructions with different immediate values.  Different OS'
	 may use a different variation, but they have the same outcome.  */
	return ((insn & BRK_INSN_MASK) == BRK_INSN_BASE);
    }

  return false;
}

/* When arguments must be pushed onto the stack, they go on in reverse
   order.  The code below implements a FILO (stack) to do this.  */

struct stack_item_t
{
  /* Value to pass on stack.  It can be NULL if this item is for stack
     padding.  */
  const gdb_byte *data;

  /* Size in bytes of value to pass on stack.  */
  int len;
};

/* Implement the gdbarch type alignment method, overrides the generic
   alignment algorithm for anything that is aarch64 specific.  */

static ULONGEST
aarch64_type_align (gdbarch *gdbarch, struct type *t)
{
  t = check_typedef (t);
  if (t->code () == TYPE_CODE_ARRAY && t->is_vector ())
    {
      /* Use the natural alignment for vector types (the same for
	 scalar type), but the maximum alignment is 128-bit.  */
      if (t->length () > 16)
	return 16;
      else
	return t->length ();
    }

  /* Allow the common code to calculate the alignment.  */
  return 0;
}

/* Worker function for aapcs_is_vfp_call_or_return_candidate.

   Return the number of register required, or -1 on failure.

   When encountering a base element, if FUNDAMENTAL_TYPE is not set then set it
   to the element, else fail if the type of this element does not match the
   existing value.  */

static int
aapcs_is_vfp_call_or_return_candidate_1 (struct type *type,
					 struct type **fundamental_type)
{
  if (type == nullptr)
    return -1;

  switch (type->code ())
    {
    case TYPE_CODE_FLT:
    case TYPE_CODE_DECFLOAT:
      if (type->length () > 16)
	return -1;

      if (*fundamental_type == nullptr)
	*fundamental_type = type;
      else if (type->length () != (*fundamental_type)->length ()
	       || type->code () != (*fundamental_type)->code ())
	return -1;

      return 1;

    case TYPE_CODE_COMPLEX:
      {
	struct type *target_type = check_typedef (type->target_type ());
	if (target_type->length () > 16)
	  return -1;

	if (*fundamental_type == nullptr)
	  *fundamental_type = target_type;
	else if (target_type->length () != (*fundamental_type)->length ()
		 || target_type->code () != (*fundamental_type)->code ())
	  return -1;

	return 2;
      }

    case TYPE_CODE_ARRAY:
      {
	if (type->is_vector ())
	  {
	    if (type->length () != 8 && type->length () != 16)
	      return -1;

	    if (*fundamental_type == nullptr)
	      *fundamental_type = type;
	    else if (type->length () != (*fundamental_type)->length ()
		     || type->code () != (*fundamental_type)->code ())
	      return -1;

	    return 1;
	  }
	else
	  {
	    struct type *target_type = type->target_type ();
	    int count = aapcs_is_vfp_call_or_return_candidate_1
			  (target_type, fundamental_type);

	    if (count == -1)
	      return count;

	    count *= (type->length () / target_type->length ());
	      return count;
	  }
      }

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      {
	int count = 0;

	for (int i = 0; i < type->num_fields (); i++)
	  {
	    /* Ignore any static fields.  */
	    if (type->field (i).is_static ())
	      continue;

	    struct type *member = check_typedef (type->field (i).type ());

	    int sub_count = aapcs_is_vfp_call_or_return_candidate_1
			      (member, fundamental_type);
	    if (sub_count == -1)
	      return -1;
	    count += sub_count;
	  }

	/* Ensure there is no padding between the fields (allowing for empty
	   zero length structs)  */
	int ftype_length = (*fundamental_type == nullptr)
			   ? 0 : (*fundamental_type)->length ();
	if (count * ftype_length != type->length ())
	  return -1;

	return count;
      }

    default:
      break;
    }

  return -1;
}

/* Return true if an argument, whose type is described by TYPE, can be passed or
   returned in simd/fp registers, providing enough parameter passing registers
   are available.  This is as described in the AAPCS64.

   Upon successful return, *COUNT returns the number of needed registers,
   *FUNDAMENTAL_TYPE contains the type of those registers.

   Candidate as per the AAPCS64 5.4.2.C is either a:
   - float.
   - short-vector.
   - HFA (Homogeneous Floating-point Aggregate, 4.3.5.1). A Composite type where
     all the members are floats and has at most 4 members.
   - HVA (Homogeneous Short-vector Aggregate, 4.3.5.2). A Composite type where
     all the members are short vectors and has at most 4 members.
   - Complex (7.1.1)

   Note that HFAs and HVAs can include nested structures and arrays.  */

static bool
aapcs_is_vfp_call_or_return_candidate (struct type *type, int *count,
				       struct type **fundamental_type)
{
  if (type == nullptr)
    return false;

  *fundamental_type = nullptr;

  int ag_count = aapcs_is_vfp_call_or_return_candidate_1 (type,
							  fundamental_type);

  if (ag_count > 0 && ag_count <= HA_MAX_NUM_FLDS)
    {
      *count = ag_count;
      return true;
    }
  else
    return false;
}

/* AArch64 function call information structure.  */
struct aarch64_call_info
{
  /* the current argument number.  */
  unsigned argnum = 0;

  /* The next general purpose register number, equivalent to NGRN as
     described in the AArch64 Procedure Call Standard.  */
  unsigned ngrn = 0;

  /* The next SIMD and floating point register number, equivalent to
     NSRN as described in the AArch64 Procedure Call Standard.  */
  unsigned nsrn = 0;

  /* The next stacked argument address, equivalent to NSAA as
     described in the AArch64 Procedure Call Standard.  */
  unsigned nsaa = 0;

  /* Stack item vector.  */
  std::vector<stack_item_t> si;
};

/* Pass a value in a sequence of consecutive X registers.  The caller
   is responsible for ensuring sufficient registers are available.  */

static void
pass_in_x (struct gdbarch *gdbarch, struct regcache *regcache,
	   struct aarch64_call_info *info, struct type *type,
	   struct value *arg)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int len = type->length ();
  enum type_code typecode = type->code ();
  int regnum = AARCH64_X0_REGNUM + info->ngrn;
  const bfd_byte *buf = arg->contents ().data ();

  info->argnum++;

  while (len > 0)
    {
      int partial_len = len < X_REGISTER_SIZE ? len : X_REGISTER_SIZE;
      CORE_ADDR regval = extract_unsigned_integer (buf, partial_len,
						   byte_order);


      /* Adjust sub-word struct/union args when big-endian.  */
      if (byte_order == BFD_ENDIAN_BIG
	  && partial_len < X_REGISTER_SIZE
	  && (typecode == TYPE_CODE_STRUCT || typecode == TYPE_CODE_UNION))
	regval <<= ((X_REGISTER_SIZE - partial_len) * TARGET_CHAR_BIT);

      aarch64_debug_printf ("arg %d in %s = 0x%s", info->argnum,
			    gdbarch_register_name (gdbarch, regnum),
			    phex (regval, X_REGISTER_SIZE));

      regcache_cooked_write_unsigned (regcache, regnum, regval);
      len -= partial_len;
      buf += partial_len;
      regnum++;
    }
}

/* Attempt to marshall a value in a V register.  Return 1 if
   successful, or 0 if insufficient registers are available.  This
   function, unlike the equivalent pass_in_x() function does not
   handle arguments spread across multiple registers.  */

static int
pass_in_v (struct gdbarch *gdbarch,
	   struct regcache *regcache,
	   struct aarch64_call_info *info,
	   int len, const bfd_byte *buf)
{
  if (info->nsrn < 8)
    {
      int regnum = AARCH64_V0_REGNUM + info->nsrn;
      /* Enough space for a full vector register.  */
      gdb_byte reg[register_size (gdbarch, regnum)];
      gdb_assert (len <= sizeof (reg));

      info->argnum++;
      info->nsrn++;

      memset (reg, 0, sizeof (reg));
      /* PCS C.1, the argument is allocated to the least significant
	 bits of V register.  */
      memcpy (reg, buf, len);
      regcache->cooked_write (regnum, reg);

      aarch64_debug_printf ("arg %d in %s", info->argnum,
			    gdbarch_register_name (gdbarch, regnum));

      return 1;
    }
  info->nsrn = 8;
  return 0;
}

/* Marshall an argument onto the stack.  */

static void
pass_on_stack (struct aarch64_call_info *info, struct type *type,
	       struct value *arg)
{
  const bfd_byte *buf = arg->contents ().data ();
  int len = type->length ();
  int align;
  stack_item_t item;

  info->argnum++;

  align = type_align (type);

  /* PCS C.17 Stack should be aligned to the larger of 8 bytes or the
     Natural alignment of the argument's type.  */
  align = align_up (align, 8);

  /* The AArch64 PCS requires at most doubleword alignment.  */
  if (align > 16)
    align = 16;

  aarch64_debug_printf ("arg %d len=%d @ sp + %d\n", info->argnum, len,
			info->nsaa);

  item.len = len;
  item.data = buf;
  info->si.push_back (item);

  info->nsaa += len;
  if (info->nsaa & (align - 1))
    {
      /* Push stack alignment padding.  */
      int pad = align - (info->nsaa & (align - 1));

      item.len = pad;
      item.data = NULL;

      info->si.push_back (item);
      info->nsaa += pad;
    }
}

/* Marshall an argument into a sequence of one or more consecutive X
   registers or, if insufficient X registers are available then onto
   the stack.  */

static void
pass_in_x_or_stack (struct gdbarch *gdbarch, struct regcache *regcache,
		    struct aarch64_call_info *info, struct type *type,
		    struct value *arg)
{
  int len = type->length ();
  int nregs = (len + X_REGISTER_SIZE - 1) / X_REGISTER_SIZE;

  /* PCS C.13 - Pass in registers if we have enough spare */
  if (info->ngrn + nregs <= 8)
    {
      pass_in_x (gdbarch, regcache, info, type, arg);
      info->ngrn += nregs;
    }
  else
    {
      info->ngrn = 8;
      pass_on_stack (info, type, arg);
    }
}

/* Pass a value, which is of type arg_type, in a V register.  Assumes value is a
   aapcs_is_vfp_call_or_return_candidate and there are enough spare V
   registers.  A return value of false is an error state as the value will have
   been partially passed to the stack.  */
static bool
pass_in_v_vfp_candidate (struct gdbarch *gdbarch, struct regcache *regcache,
			 struct aarch64_call_info *info, struct type *arg_type,
			 struct value *arg)
{
  switch (arg_type->code ())
    {
    case TYPE_CODE_FLT:
    case TYPE_CODE_DECFLOAT:
      return pass_in_v (gdbarch, regcache, info, arg_type->length (),
			arg->contents ().data ());
      break;

    case TYPE_CODE_COMPLEX:
      {
	const bfd_byte *buf = arg->contents ().data ();
	struct type *target_type = check_typedef (arg_type->target_type ());

	if (!pass_in_v (gdbarch, regcache, info, target_type->length (),
			buf))
	  return false;

	return pass_in_v (gdbarch, regcache, info, target_type->length (),
			  buf + target_type->length ());
      }

    case TYPE_CODE_ARRAY:
      if (arg_type->is_vector ())
	return pass_in_v (gdbarch, regcache, info, arg_type->length (),
			  arg->contents ().data ());
      [[fallthrough]];

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      for (int i = 0; i < arg_type->num_fields (); i++)
	{
	  /* Don't include static fields.  */
	  if (arg_type->field (i).is_static ())
	    continue;

	  struct value *field = arg->primitive_field (0, i, arg_type);
	  struct type *field_type = check_typedef (field->type ());

	  if (!pass_in_v_vfp_candidate (gdbarch, regcache, info, field_type,
					field))
	    return false;
	}
      return true;

    default:
      return false;
    }
}

/* Implement the "push_dummy_call" gdbarch method.  */

static CORE_ADDR
aarch64_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
			 struct regcache *regcache, CORE_ADDR bp_addr,
			 int nargs,
			 struct value **args, CORE_ADDR sp,
			 function_call_return_method return_method,
			 CORE_ADDR struct_addr)
{
  int argnum;
  struct aarch64_call_info info;

  /* We need to know what the type of the called function is in order
     to determine the number of named/anonymous arguments for the
     actual argument placement, and the return type in order to handle
     return value correctly.

     The generic code above us views the decision of return in memory
     or return in registers as a two stage processes.  The language
     handler is consulted first and may decide to return in memory (eg
     class with copy constructor returned by value), this will cause
     the generic code to allocate space AND insert an initial leading
     argument.

     If the language code does not decide to pass in memory then the
     target code is consulted.

     If the language code decides to pass in memory we want to move
     the pointer inserted as the initial argument from the argument
     list and into X8, the conventional AArch64 struct return pointer
     register.  */

  /* Set the return address.  For the AArch64, the return breakpoint
     is always at BP_ADDR.  */
  regcache_cooked_write_unsigned (regcache, AARCH64_LR_REGNUM, bp_addr);

  /* If we were given an initial argument for the return slot, lose it.  */
  if (return_method == return_method_hidden_param)
    {
      args++;
      nargs--;
    }

  /* The struct_return pointer occupies X8.  */
  if (return_method != return_method_normal)
    {
      aarch64_debug_printf ("struct return in %s = 0x%s",
			    gdbarch_register_name
			      (gdbarch, AARCH64_STRUCT_RETURN_REGNUM),
			    paddress (gdbarch, struct_addr));

      regcache_cooked_write_unsigned (regcache, AARCH64_STRUCT_RETURN_REGNUM,
				      struct_addr);
    }

  for (argnum = 0; argnum < nargs; argnum++)
    {
      struct value *arg = args[argnum];
      struct type *arg_type, *fundamental_type;
      int len, elements;

      arg_type = check_typedef (arg->type ());
      len = arg_type->length ();

      /* If arg can be passed in v registers as per the AAPCS64, then do so if
	 if there are enough spare registers.  */
      if (aapcs_is_vfp_call_or_return_candidate (arg_type, &elements,
						 &fundamental_type))
	{
	  if (info.nsrn + elements <= 8)
	    {
	      /* We know that we have sufficient registers available therefore
		 this will never need to fallback to the stack.  */
	      if (!pass_in_v_vfp_candidate (gdbarch, regcache, &info, arg_type,
					    arg))
		gdb_assert_not_reached ("Failed to push args");
	    }
	  else
	    {
	      info.nsrn = 8;
	      pass_on_stack (&info, arg_type, arg);
	    }
	  continue;
	}

      switch (arg_type->code ())
	{
	case TYPE_CODE_INT:
	case TYPE_CODE_BOOL:
	case TYPE_CODE_CHAR:
	case TYPE_CODE_RANGE:
	case TYPE_CODE_ENUM:
	  if (len < 4 && !is_fixed_point_type (arg_type))
	    {
	      /* Promote to 32 bit integer.  */
	      if (arg_type->is_unsigned ())
		arg_type = builtin_type (gdbarch)->builtin_uint32;
	      else
		arg_type = builtin_type (gdbarch)->builtin_int32;
	      arg = value_cast (arg_type, arg);
	    }
	  pass_in_x_or_stack (gdbarch, regcache, &info, arg_type, arg);
	  break;

	case TYPE_CODE_STRUCT:
	case TYPE_CODE_ARRAY:
	case TYPE_CODE_UNION:
	  if (len > 16)
	    {
	      /* PCS B.7 Aggregates larger than 16 bytes are passed by
		 invisible reference.  */

	      /* Allocate aligned storage.  */
	      sp = align_down (sp - len, 16);

	      /* Write the real data into the stack.  */
	      write_memory (sp, arg->contents ().data (), len);

	      /* Construct the indirection.  */
	      arg_type = lookup_pointer_type (arg_type);
	      arg = value_from_pointer (arg_type, sp);
	      pass_in_x_or_stack (gdbarch, regcache, &info, arg_type, arg);
	    }
	  else
	    /* PCS C.15 / C.18 multiple values pass.  */
	    pass_in_x_or_stack (gdbarch, regcache, &info, arg_type, arg);
	  break;

	default:
	  pass_in_x_or_stack (gdbarch, regcache, &info, arg_type, arg);
	  break;
	}
    }

  /* Make sure stack retains 16 byte alignment.  */
  if (info.nsaa & 15)
    sp -= 16 - (info.nsaa & 15);

  while (!info.si.empty ())
    {
      const stack_item_t &si = info.si.back ();

      sp -= si.len;
      if (si.data != NULL)
	write_memory (sp, si.data, si.len);
      info.si.pop_back ();
    }

  /* Finally, update the SP register.  */
  regcache_cooked_write_unsigned (regcache, AARCH64_SP_REGNUM, sp);

  return sp;
}

/* Implement the "frame_align" gdbarch method.  */

static CORE_ADDR
aarch64_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  /* Align the stack to sixteen bytes.  */
  return sp & ~(CORE_ADDR) 15;
}

/* Return the type for an AdvSISD Q register.  */

static struct type *
aarch64_vnq_type (struct gdbarch *gdbarch)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  if (tdep->vnq_type == NULL)
    {
      struct type *t;
      struct type *elem;

      t = arch_composite_type (gdbarch, "__gdb_builtin_type_vnq",
			       TYPE_CODE_UNION);

      elem = builtin_type (gdbarch)->builtin_uint128;
      append_composite_type_field (t, "u", elem);

      elem = builtin_type (gdbarch)->builtin_int128;
      append_composite_type_field (t, "s", elem);

      tdep->vnq_type = t;
    }

  return tdep->vnq_type;
}

/* Return the type for an AdvSISD D register.  */

static struct type *
aarch64_vnd_type (struct gdbarch *gdbarch)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  if (tdep->vnd_type == NULL)
    {
      struct type *t;
      struct type *elem;

      t = arch_composite_type (gdbarch, "__gdb_builtin_type_vnd",
			       TYPE_CODE_UNION);

      elem = builtin_type (gdbarch)->builtin_double;
      append_composite_type_field (t, "f", elem);

      elem = builtin_type (gdbarch)->builtin_uint64;
      append_composite_type_field (t, "u", elem);

      elem = builtin_type (gdbarch)->builtin_int64;
      append_composite_type_field (t, "s", elem);

      tdep->vnd_type = t;
    }

  return tdep->vnd_type;
}

/* Return the type for an AdvSISD S register.  */

static struct type *
aarch64_vns_type (struct gdbarch *gdbarch)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  if (tdep->vns_type == NULL)
    {
      struct type *t;
      struct type *elem;

      t = arch_composite_type (gdbarch, "__gdb_builtin_type_vns",
			       TYPE_CODE_UNION);

      elem = builtin_type (gdbarch)->builtin_float;
      append_composite_type_field (t, "f", elem);

      elem = builtin_type (gdbarch)->builtin_uint32;
      append_composite_type_field (t, "u", elem);

      elem = builtin_type (gdbarch)->builtin_int32;
      append_composite_type_field (t, "s", elem);

      tdep->vns_type = t;
    }

  return tdep->vns_type;
}

/* Return the type for an AdvSISD H register.  */

static struct type *
aarch64_vnh_type (struct gdbarch *gdbarch)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  if (tdep->vnh_type == NULL)
    {
      struct type *t;
      struct type *elem;

      t = arch_composite_type (gdbarch, "__gdb_builtin_type_vnh",
			       TYPE_CODE_UNION);

      elem = builtin_type (gdbarch)->builtin_bfloat16;
      append_composite_type_field (t, "bf", elem);

      elem = builtin_type (gdbarch)->builtin_half;
      append_composite_type_field (t, "f", elem);

      elem = builtin_type (gdbarch)->builtin_uint16;
      append_composite_type_field (t, "u", elem);

      elem = builtin_type (gdbarch)->builtin_int16;
      append_composite_type_field (t, "s", elem);

      tdep->vnh_type = t;
    }

  return tdep->vnh_type;
}

/* Return the type for an AdvSISD B register.  */

static struct type *
aarch64_vnb_type (struct gdbarch *gdbarch)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  if (tdep->vnb_type == NULL)
    {
      struct type *t;
      struct type *elem;

      t = arch_composite_type (gdbarch, "__gdb_builtin_type_vnb",
			       TYPE_CODE_UNION);

      elem = builtin_type (gdbarch)->builtin_uint8;
      append_composite_type_field (t, "u", elem);

      elem = builtin_type (gdbarch)->builtin_int8;
      append_composite_type_field (t, "s", elem);

      tdep->vnb_type = t;
    }

  return tdep->vnb_type;
}

/* Return TRUE if REGNUM is a ZA tile slice pseudo-register number.  Return
   FALSE otherwise.  */

static bool
is_sme_tile_slice_pseudo_register (struct gdbarch *gdbarch, int regnum)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  gdb_assert (tdep->has_sme ());
  gdb_assert (tdep->sme_svq > 0);
  gdb_assert (tdep->sme_pseudo_base <= regnum);
  gdb_assert (regnum < tdep->sme_pseudo_base + tdep->sme_pseudo_count);

  if (tdep->sme_tile_slice_pseudo_base <= regnum
      && regnum < tdep->sme_tile_slice_pseudo_base
		  + tdep->sme_tile_slice_pseudo_count)
    return true;

  return false;
}

/* Given REGNUM, a ZA pseudo-register number, return, in ENCODING, the
   decoded fields that make up its name.  */

static void
aarch64_za_decode_pseudos (struct gdbarch *gdbarch, int regnum,
			   struct za_pseudo_encoding &encoding)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  gdb_assert (tdep->has_sme ());
  gdb_assert (tdep->sme_svq > 0);
  gdb_assert (tdep->sme_pseudo_base <= regnum);
  gdb_assert (regnum < tdep->sme_pseudo_base + tdep->sme_pseudo_count);

  if (is_sme_tile_slice_pseudo_register (gdbarch, regnum))
    {
      /* Calculate the tile slice pseudo-register offset relative to the other
	 tile slice pseudo-registers.  */
      int offset = regnum - tdep->sme_tile_slice_pseudo_base;

      /* Fetch the qualifier.  We can have 160 to 2560 possible tile slice
	 pseudo-registers.  Each qualifier (we have 5 of them: B, H, S, D
	 and Q) covers 32 * svq pseudo-registers, so we divide the offset by
	 that constant.  */
      size_t qualifier = offset / (tdep->sme_svq * 32);
      encoding.qualifier_index = qualifier;

      /* Prepare to fetch the direction (d), tile number (t) and slice
	 number (s).  */
      int dts = offset % (tdep->sme_svq * 32);

      /* The direction is represented by the even/odd numbers.  Even-numbered
	 pseudo-registers are horizontal tile slices and odd-numbered
	 pseudo-registers are vertical tile slices.  */
      encoding.horizontal = !(dts & 1);

      /* Fetch the tile number.  The tile number is closely related to the
	 qualifier.  B has 1 tile, H has 2 tiles, S has 4 tiles, D has 8 tiles
	 and Q has 16 tiles.  */
      encoding.tile_index = (dts >> 1) & ((1 << qualifier) - 1);

      /* Fetch the slice number.  The slice number is closely related to the
	 qualifier and the svl.  */
      encoding.slice_index = dts >> (qualifier + 1);
    }
  else
    {
      /* Calculate the tile pseudo-register offset relative to the other
	 tile pseudo-registers.  */
      int offset = regnum - tdep->sme_tile_pseudo_base;

      encoding.qualifier_index = std::floor (std::log2 (offset + 1));
      /* Calculate the tile number.  */
      encoding.tile_index = (offset + 1) - (1 << encoding.qualifier_index);
      /* Direction and slice index don't get used for tiles.  Set them to
	 0/false values.  */
      encoding.slice_index = 0;
      encoding.horizontal = false;
    }
}

/* Return the type for a ZA tile slice pseudo-register based on ENCODING.  */

static struct type *
aarch64_za_tile_slice_type (struct gdbarch *gdbarch,
			    const struct za_pseudo_encoding &encoding)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  gdb_assert (tdep->has_sme ());
  gdb_assert (tdep->sme_svq > 0);

  if (tdep->sme_tile_slice_type_q == nullptr)
    {
      /* Q tile slice type.  */
      tdep->sme_tile_slice_type_q
	= init_vector_type (builtin_type (gdbarch)->builtin_uint128,
			    tdep->sme_svq);
      /* D tile slice type.  */
      tdep->sme_tile_slice_type_d
	= init_vector_type (builtin_type (gdbarch)->builtin_uint64,
			    tdep->sme_svq * 2);
      /* S tile slice type.  */
      tdep->sme_tile_slice_type_s
	= init_vector_type (builtin_type (gdbarch)->builtin_uint32,
			    tdep->sme_svq * 4);
      /* H tile slice type.  */
      tdep->sme_tile_slice_type_h
	= init_vector_type (builtin_type (gdbarch)->builtin_uint16,
			    tdep->sme_svq * 8);
      /* B tile slice type.  */
      tdep->sme_tile_slice_type_b
	= init_vector_type (builtin_type (gdbarch)->builtin_uint8,
			    tdep->sme_svq * 16);
  }

  switch (encoding.qualifier_index)
    {
      case 4:
	return tdep->sme_tile_slice_type_q;
      case 3:
	return tdep->sme_tile_slice_type_d;
      case 2:
	return tdep->sme_tile_slice_type_s;
      case 1:
	return tdep->sme_tile_slice_type_h;
      case 0:
	return tdep->sme_tile_slice_type_b;
      default:
	error (_("Invalid qualifier index %s for tile slice pseudo register."),
	       pulongest (encoding.qualifier_index));
    }

  gdb_assert_not_reached ("Unknown qualifier for ZA tile slice register");
}

/* Return the type for a ZA tile pseudo-register based on ENCODING.  */

static struct type *
aarch64_za_tile_type (struct gdbarch *gdbarch,
		      const struct za_pseudo_encoding &encoding)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  gdb_assert (tdep->has_sme ());
  gdb_assert (tdep->sme_svq > 0);

  if (tdep->sme_tile_type_q == nullptr)
    {
      struct type *inner_vectors_type;

      /* Q tile type.  */
      inner_vectors_type
	= init_vector_type (builtin_type (gdbarch)->builtin_uint128,
			    tdep->sme_svq);
      tdep->sme_tile_type_q
	= init_vector_type (inner_vectors_type, tdep->sme_svq);

      /* D tile type.  */
      inner_vectors_type
	= init_vector_type (builtin_type (gdbarch)->builtin_uint64,
			    tdep->sme_svq * 2);
      tdep->sme_tile_type_d
	= init_vector_type (inner_vectors_type, tdep->sme_svq * 2);

      /* S tile type.  */
      inner_vectors_type
	= init_vector_type (builtin_type (gdbarch)->builtin_uint32,
			    tdep->sme_svq * 4);
      tdep->sme_tile_type_s
	= init_vector_type (inner_vectors_type, tdep->sme_svq * 4);

      /* H tile type.  */
      inner_vectors_type
	= init_vector_type (builtin_type (gdbarch)->builtin_uint16,
			    tdep->sme_svq * 8);
      tdep->sme_tile_type_h
	= init_vector_type (inner_vectors_type, tdep->sme_svq * 8);

      /* B tile type.  */
      inner_vectors_type
	= init_vector_type (builtin_type (gdbarch)->builtin_uint8,
			    tdep->sme_svq * 16);
      tdep->sme_tile_type_b
	= init_vector_type (inner_vectors_type, tdep->sme_svq * 16);
  }

  switch (encoding.qualifier_index)
    {
      case 4:
	return tdep->sme_tile_type_q;
      case 3:
	return tdep->sme_tile_type_d;
      case 2:
	return tdep->sme_tile_type_s;
      case 1:
	return tdep->sme_tile_type_h;
      case 0:
	return tdep->sme_tile_type_b;
      default:
	error (_("Invalid qualifier index %s for ZA tile pseudo register."),
	       pulongest (encoding.qualifier_index));
    }

  gdb_assert_not_reached ("unknown qualifier for tile pseudo-register");
}

/* Return the type for an AdvSISD V register.  */

static struct type *
aarch64_vnv_type (struct gdbarch *gdbarch)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  if (tdep->vnv_type == NULL)
    {
      /* The other AArch64 pseudo registers (Q,D,H,S,B) refer to a single value
	 slice from the non-pseudo vector registers.  However NEON V registers
	 are always vector registers, and need constructing as such.  */
      const struct builtin_type *bt = builtin_type (gdbarch);

      struct type *t = arch_composite_type (gdbarch, "__gdb_builtin_type_vnv",
					    TYPE_CODE_UNION);

      struct type *sub = arch_composite_type (gdbarch, "__gdb_builtin_type_vnd",
				 TYPE_CODE_UNION);
      append_composite_type_field (sub, "f",
				   init_vector_type (bt->builtin_double, 2));
      append_composite_type_field (sub, "u",
				   init_vector_type (bt->builtin_uint64, 2));
      append_composite_type_field (sub, "s",
				   init_vector_type (bt->builtin_int64, 2));
      append_composite_type_field (t, "d", sub);

      sub = arch_composite_type (gdbarch, "__gdb_builtin_type_vns",
				 TYPE_CODE_UNION);
      append_composite_type_field (sub, "f",
				   init_vector_type (bt->builtin_float, 4));
      append_composite_type_field (sub, "u",
				   init_vector_type (bt->builtin_uint32, 4));
      append_composite_type_field (sub, "s",
				   init_vector_type (bt->builtin_int32, 4));
      append_composite_type_field (t, "s", sub);

      sub = arch_composite_type (gdbarch, "__gdb_builtin_type_vnh",
				 TYPE_CODE_UNION);
      append_composite_type_field (sub, "bf",
				   init_vector_type (bt->builtin_bfloat16, 8));
      append_composite_type_field (sub, "f",
				   init_vector_type (bt->builtin_half, 8));
      append_composite_type_field (sub, "u",
				   init_vector_type (bt->builtin_uint16, 8));
      append_composite_type_field (sub, "s",
				   init_vector_type (bt->builtin_int16, 8));
      append_composite_type_field (t, "h", sub);

      sub = arch_composite_type (gdbarch, "__gdb_builtin_type_vnb",
				 TYPE_CODE_UNION);
      append_composite_type_field (sub, "u",
				   init_vector_type (bt->builtin_uint8, 16));
      append_composite_type_field (sub, "s",
				   init_vector_type (bt->builtin_int8, 16));
      append_composite_type_field (t, "b", sub);

      sub = arch_composite_type (gdbarch, "__gdb_builtin_type_vnq",
				 TYPE_CODE_UNION);
      append_composite_type_field (sub, "u",
				   init_vector_type (bt->builtin_uint128, 1));
      append_composite_type_field (sub, "s",
				   init_vector_type (bt->builtin_int128, 1));
      append_composite_type_field (t, "q", sub);

      tdep->vnv_type = t;
    }

  return tdep->vnv_type;
}

/* Implement the "dwarf2_reg_to_regnum" gdbarch method.  */

static int
aarch64_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  if (reg >= AARCH64_DWARF_X0 && reg <= AARCH64_DWARF_X0 + 30)
    return AARCH64_X0_REGNUM + reg - AARCH64_DWARF_X0;

  if (reg == AARCH64_DWARF_SP)
    return AARCH64_SP_REGNUM;

  if (reg == AARCH64_DWARF_PC)
    return AARCH64_PC_REGNUM;

  if (reg >= AARCH64_DWARF_V0 && reg <= AARCH64_DWARF_V0 + 31)
    return AARCH64_V0_REGNUM + reg - AARCH64_DWARF_V0;

  if (reg == AARCH64_DWARF_SVE_VG)
    return AARCH64_SVE_VG_REGNUM;

  if (reg == AARCH64_DWARF_SVE_FFR)
    return AARCH64_SVE_FFR_REGNUM;

  if (reg >= AARCH64_DWARF_SVE_P0 && reg <= AARCH64_DWARF_SVE_P0 + 15)
    return AARCH64_SVE_P0_REGNUM + reg - AARCH64_DWARF_SVE_P0;

  if (reg >= AARCH64_DWARF_SVE_Z0 && reg <= AARCH64_DWARF_SVE_Z0 + 15)
    return AARCH64_SVE_Z0_REGNUM + reg - AARCH64_DWARF_SVE_Z0;

  if (tdep->has_pauth ())
    {
      if (reg == AARCH64_DWARF_RA_SIGN_STATE)
	return tdep->ra_sign_state_regnum;
    }

  return -1;
}

/* Implement the "print_insn" gdbarch method.  */

static int
aarch64_gdb_print_insn (bfd_vma memaddr, disassemble_info *info)
{
  info->symbols = NULL;
  return default_print_insn (memaddr, info);
}

/* AArch64 BRK software debug mode instruction.
   Note that AArch64 code is always little-endian.
   1101.0100.0010.0000.0000.0000.0000.0000 = 0xd4200000.  */
constexpr gdb_byte aarch64_default_breakpoint[] = {0x00, 0x00, 0x20, 0xd4};

typedef BP_MANIPULATION (aarch64_default_breakpoint) aarch64_breakpoint;

/* Extract from an array REGS containing the (raw) register state a
   function return value of type TYPE, and copy that, in virtual
   format, into VALBUF.  */

static void
aarch64_extract_return_value (struct type *type, struct regcache *regs,
			      gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regs->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int elements;
  struct type *fundamental_type;

  if (aapcs_is_vfp_call_or_return_candidate (type, &elements,
					     &fundamental_type))
    {
      int len = fundamental_type->length ();

      for (int i = 0; i < elements; i++)
	{
	  int regno = AARCH64_V0_REGNUM + i;
	  /* Enough space for a full vector register.  */
	  gdb_byte buf[register_size (gdbarch, regno)];
	  gdb_assert (len <= sizeof (buf));

	  aarch64_debug_printf
	    ("read HFA or HVA return value element %d from %s",
	     i + 1, gdbarch_register_name (gdbarch, regno));

	  regs->cooked_read (regno, buf);

	  memcpy (valbuf, buf, len);
	  valbuf += len;
	}
    }
  else if (type->code () == TYPE_CODE_INT
	   || type->code () == TYPE_CODE_CHAR
	   || type->code () == TYPE_CODE_BOOL
	   || type->code () == TYPE_CODE_PTR
	   || TYPE_IS_REFERENCE (type)
	   || type->code () == TYPE_CODE_ENUM)
    {
      /* If the type is a plain integer, then the access is
	 straight-forward.  Otherwise we have to play around a bit
	 more.  */
      int len = type->length ();
      int regno = AARCH64_X0_REGNUM;
      ULONGEST tmp;

      while (len > 0)
	{
	  /* By using store_unsigned_integer we avoid having to do
	     anything special for small big-endian values.  */
	  regcache_cooked_read_unsigned (regs, regno++, &tmp);
	  store_unsigned_integer (valbuf,
				  (len > X_REGISTER_SIZE
				   ? X_REGISTER_SIZE : len), byte_order, tmp);
	  len -= X_REGISTER_SIZE;
	  valbuf += X_REGISTER_SIZE;
	}
    }
  else
    {
      /* For a structure or union the behaviour is as if the value had
	 been stored to word-aligned memory and then loaded into
	 registers with 64-bit load instruction(s).  */
      int len = type->length ();
      int regno = AARCH64_X0_REGNUM;
      bfd_byte buf[X_REGISTER_SIZE];

      while (len > 0)
	{
	  regs->cooked_read (regno++, buf);
	  memcpy (valbuf, buf, len > X_REGISTER_SIZE ? X_REGISTER_SIZE : len);
	  len -= X_REGISTER_SIZE;
	  valbuf += X_REGISTER_SIZE;
	}
    }
}


/* Will a function return an aggregate type in memory or in a
   register?  Return 0 if an aggregate type can be returned in a
   register, 1 if it must be returned in memory.  */

static int
aarch64_return_in_memory (struct gdbarch *gdbarch, struct type *type)
{
  type = check_typedef (type);
  int elements;
  struct type *fundamental_type;

  if (TYPE_HAS_DYNAMIC_LENGTH (type))
    return 1;

  if (aapcs_is_vfp_call_or_return_candidate (type, &elements,
					     &fundamental_type))
    {
      /* v0-v7 are used to return values and one register is allocated
	 for one member.  However, HFA or HVA has at most four members.  */
      return 0;
    }

  if (type->length () > 16
      || !language_pass_by_reference (type).trivially_copyable)
    {
      /* PCS B.6 Aggregates larger than 16 bytes are passed by
	 invisible reference.  */

      return 1;
    }

  return 0;
}

/* Write into appropriate registers a function return value of type
   TYPE, given in virtual format.  */

static void
aarch64_store_return_value (struct type *type, struct regcache *regs,
			    const gdb_byte *valbuf)
{
  struct gdbarch *gdbarch = regs->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int elements;
  struct type *fundamental_type;

  if (aapcs_is_vfp_call_or_return_candidate (type, &elements,
					     &fundamental_type))
    {
      int len = fundamental_type->length ();

      for (int i = 0; i < elements; i++)
	{
	  int regno = AARCH64_V0_REGNUM + i;
	  /* Enough space for a full vector register.  */
	  gdb_byte tmpbuf[register_size (gdbarch, regno)];
	  gdb_assert (len <= sizeof (tmpbuf));

	  aarch64_debug_printf
	    ("write HFA or HVA return value element %d to %s",
	     i + 1, gdbarch_register_name (gdbarch, regno));

	  /* Depending on whether the target supports SVE or not, the V
	     registers may report a size > 16 bytes.  In that case, read the
	     original contents of the register before overriding it with a new
	     value that has a potential size <= 16 bytes.  */
	  regs->cooked_read (regno, tmpbuf);
	  memcpy (tmpbuf, valbuf,
		  len > V_REGISTER_SIZE ? V_REGISTER_SIZE : len);
	  regs->cooked_write (regno, tmpbuf);
	  valbuf += len;
	}
    }
  else if (type->code () == TYPE_CODE_INT
	   || type->code () == TYPE_CODE_CHAR
	   || type->code () == TYPE_CODE_BOOL
	   || type->code () == TYPE_CODE_PTR
	   || TYPE_IS_REFERENCE (type)
	   || type->code () == TYPE_CODE_ENUM)
    {
      if (type->length () <= X_REGISTER_SIZE)
	{
	  /* Values of one word or less are zero/sign-extended and
	     returned in r0.  */
	  bfd_byte tmpbuf[X_REGISTER_SIZE];
	  LONGEST val = unpack_long (type, valbuf);

	  store_signed_integer (tmpbuf, X_REGISTER_SIZE, byte_order, val);
	  regs->cooked_write (AARCH64_X0_REGNUM, tmpbuf);
	}
      else
	{
	  /* Integral values greater than one word are stored in
	     consecutive registers starting with r0.  This will always
	     be a multiple of the regiser size.  */
	  int len = type->length ();
	  int regno = AARCH64_X0_REGNUM;

	  while (len > 0)
	    {
	      regs->cooked_write (regno++, valbuf);
	      len -= X_REGISTER_SIZE;
	      valbuf += X_REGISTER_SIZE;
	    }
	}
    }
  else
    {
      /* For a structure or union the behaviour is as if the value had
	 been stored to word-aligned memory and then loaded into
	 registers with 64-bit load instruction(s).  */
      int len = type->length ();
      int regno = AARCH64_X0_REGNUM;
      bfd_byte tmpbuf[X_REGISTER_SIZE];

      while (len > 0)
	{
	  memcpy (tmpbuf, valbuf,
		  len > X_REGISTER_SIZE ? X_REGISTER_SIZE : len);
	  regs->cooked_write (regno++, tmpbuf);
	  len -= X_REGISTER_SIZE;
	  valbuf += X_REGISTER_SIZE;
	}
    }
}

/* Implement the "return_value" gdbarch method.  */

static enum return_value_convention
aarch64_return_value (struct gdbarch *gdbarch, struct value *func_value,
		      struct type *valtype, struct regcache *regcache,
		      struct value **read_value, const gdb_byte *writebuf)
{
  if (valtype->code () == TYPE_CODE_STRUCT
      || valtype->code () == TYPE_CODE_UNION
      || valtype->code () == TYPE_CODE_ARRAY)
    {
      if (aarch64_return_in_memory (gdbarch, valtype))
	{
	  /* From the AAPCS64's Result Return section:

	     "Otherwise, the caller shall reserve a block of memory of
	      sufficient size and alignment to hold the result.  The address
	      of the memory block shall be passed as an additional argument to
	      the function in x8.  */

	  aarch64_debug_printf ("return value in memory");

	  if (read_value != nullptr)
	    {
	      CORE_ADDR addr;

	      regcache->cooked_read (AARCH64_STRUCT_RETURN_REGNUM, &addr);
	      *read_value = value_at_non_lval (valtype, addr);
	    }

	  return RETURN_VALUE_ABI_RETURNS_ADDRESS;
	}
    }

  if (writebuf)
    aarch64_store_return_value (valtype, regcache, writebuf);

  if (read_value)
    {
      *read_value = value::allocate (valtype);
      aarch64_extract_return_value (valtype, regcache,
				    (*read_value)->contents_raw ().data ());
    }

  aarch64_debug_printf ("return value in registers");

  return RETURN_VALUE_REGISTER_CONVENTION;
}

/* Implement the "get_longjmp_target" gdbarch method.  */

static int
aarch64_get_longjmp_target (frame_info_ptr frame, CORE_ADDR *pc)
{
  CORE_ADDR jb_addr;
  gdb_byte buf[X_REGISTER_SIZE];
  struct gdbarch *gdbarch = get_frame_arch (frame);
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  jb_addr = get_frame_register_unsigned (frame, AARCH64_X0_REGNUM);

  if (target_read_memory (jb_addr + tdep->jb_pc * tdep->jb_elt_size, buf,
			  X_REGISTER_SIZE))
    return 0;

  *pc = extract_unsigned_integer (buf, X_REGISTER_SIZE, byte_order);
  return 1;
}

/* Implement the "gen_return_address" gdbarch method.  */

static void
aarch64_gen_return_address (struct gdbarch *gdbarch,
			    struct agent_expr *ax, struct axs_value *value,
			    CORE_ADDR scope)
{
  value->type = register_type (gdbarch, AARCH64_LR_REGNUM);
  value->kind = axs_lvalue_register;
  value->u.reg = AARCH64_LR_REGNUM;
}


/* Return TRUE if REGNUM is a W pseudo-register number.  Return FALSE
   otherwise.  */

static bool
is_w_pseudo_register (struct gdbarch *gdbarch, int regnum)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  if (tdep->w_pseudo_base <= regnum
      && regnum < tdep->w_pseudo_base + tdep->w_pseudo_count)
    return true;

  return false;
}

/* Return TRUE if REGNUM is a SME pseudo-register number.  Return FALSE
   otherwise.  */

static bool
is_sme_pseudo_register (struct gdbarch *gdbarch, int regnum)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  if (tdep->has_sme () && tdep->sme_pseudo_base <= regnum
      && regnum < tdep->sme_pseudo_base + tdep->sme_pseudo_count)
    return true;

  return false;
}

/* Convert ENCODING into a ZA tile slice name.  */

static const std::string
aarch64_za_tile_slice_name (const struct za_pseudo_encoding &encoding)
{
  gdb_assert (encoding.qualifier_index >= 0);
  gdb_assert (encoding.qualifier_index <= 4);
  gdb_assert (encoding.tile_index >= 0);
  gdb_assert (encoding.tile_index <= 15);
  gdb_assert (encoding.slice_index >= 0);
  gdb_assert (encoding.slice_index <= 255);

  const char orientation = encoding.horizontal ? 'h' : 'v';

  const char qualifiers[6] = "bhsdq";
  const char qualifier = qualifiers [encoding.qualifier_index];
  return string_printf ("za%d%c%c%d", encoding.tile_index, orientation,
			qualifier, encoding.slice_index);
}

/* Convert ENCODING into a ZA tile name.  */

static const std::string
aarch64_za_tile_name (const struct za_pseudo_encoding &encoding)
{
  /* Tiles don't use the slice number and the direction fields.  */
  gdb_assert (encoding.qualifier_index >= 0);
  gdb_assert (encoding.qualifier_index <= 4);
  gdb_assert (encoding.tile_index >= 0);
  gdb_assert (encoding.tile_index <= 15);

  const char qualifiers[6] = "bhsdq";
  const char qualifier = qualifiers [encoding.qualifier_index];
  return (string_printf ("za%d%c", encoding.tile_index, qualifier));
}

/* Given a SME pseudo-register REGNUM, return its type.  */

static struct type *
aarch64_sme_pseudo_register_type (struct gdbarch *gdbarch, int regnum)
{
  struct za_pseudo_encoding encoding;

  /* Decode the SME pseudo-register number.  */
  aarch64_za_decode_pseudos (gdbarch, regnum, encoding);

  if (is_sme_tile_slice_pseudo_register (gdbarch, regnum))
    return aarch64_za_tile_slice_type (gdbarch, encoding);
  else
    return aarch64_za_tile_type (gdbarch, encoding);
}

/* Return the pseudo register name corresponding to register regnum.  */

static const char *
aarch64_pseudo_register_name (struct gdbarch *gdbarch, int regnum)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  /* W pseudo-registers.  Bottom halves of the X registers.  */
  static const char *const w_name[] =
    {
      "w0", "w1", "w2", "w3",
      "w4", "w5", "w6", "w7",
      "w8", "w9", "w10", "w11",
      "w12", "w13", "w14", "w15",
      "w16", "w17", "w18", "w19",
      "w20", "w21", "w22", "w23",
      "w24", "w25", "w26", "w27",
      "w28", "w29", "w30",
    };

  static const char *const q_name[] =
    {
      "q0", "q1", "q2", "q3",
      "q4", "q5", "q6", "q7",
      "q8", "q9", "q10", "q11",
      "q12", "q13", "q14", "q15",
      "q16", "q17", "q18", "q19",
      "q20", "q21", "q22", "q23",
      "q24", "q25", "q26", "q27",
      "q28", "q29", "q30", "q31",
    };

  static const char *const d_name[] =
    {
      "d0", "d1", "d2", "d3",
      "d4", "d5", "d6", "d7",
      "d8", "d9", "d10", "d11",
      "d12", "d13", "d14", "d15",
      "d16", "d17", "d18", "d19",
      "d20", "d21", "d22", "d23",
      "d24", "d25", "d26", "d27",
      "d28", "d29", "d30", "d31",
    };

  static const char *const s_name[] =
    {
      "s0", "s1", "s2", "s3",
      "s4", "s5", "s6", "s7",
      "s8", "s9", "s10", "s11",
      "s12", "s13", "s14", "s15",
      "s16", "s17", "s18", "s19",
      "s20", "s21", "s22", "s23",
      "s24", "s25", "s26", "s27",
      "s28", "s29", "s30", "s31",
    };

  static const char *const h_name[] =
    {
      "h0", "h1", "h2", "h3",
      "h4", "h5", "h6", "h7",
      "h8", "h9", "h10", "h11",
      "h12", "h13", "h14", "h15",
      "h16", "h17", "h18", "h19",
      "h20", "h21", "h22", "h23",
      "h24", "h25", "h26", "h27",
      "h28", "h29", "h30", "h31",
    };

  static const char *const b_name[] =
    {
      "b0", "b1", "b2", "b3",
      "b4", "b5", "b6", "b7",
      "b8", "b9", "b10", "b11",
      "b12", "b13", "b14", "b15",
      "b16", "b17", "b18", "b19",
      "b20", "b21", "b22", "b23",
      "b24", "b25", "b26", "b27",
      "b28", "b29", "b30", "b31",
    };

  int p_regnum = regnum - gdbarch_num_regs (gdbarch);

  if (p_regnum >= AARCH64_Q0_REGNUM && p_regnum < AARCH64_Q0_REGNUM + 32)
    return q_name[p_regnum - AARCH64_Q0_REGNUM];

  if (p_regnum >= AARCH64_D0_REGNUM && p_regnum < AARCH64_D0_REGNUM + 32)
    return d_name[p_regnum - AARCH64_D0_REGNUM];

  if (p_regnum >= AARCH64_S0_REGNUM && p_regnum < AARCH64_S0_REGNUM + 32)
    return s_name[p_regnum - AARCH64_S0_REGNUM];

  if (p_regnum >= AARCH64_H0_REGNUM && p_regnum < AARCH64_H0_REGNUM + 32)
    return h_name[p_regnum - AARCH64_H0_REGNUM];

  if (p_regnum >= AARCH64_B0_REGNUM && p_regnum < AARCH64_B0_REGNUM + 32)
    return b_name[p_regnum - AARCH64_B0_REGNUM];

  /* W pseudo-registers? */
  if (is_w_pseudo_register (gdbarch, regnum))
    return w_name[regnum - tdep->w_pseudo_base];

  if (tdep->has_sve ())
    {
      static const char *const sve_v_name[] =
	{
	  "v0", "v1", "v2", "v3",
	  "v4", "v5", "v6", "v7",
	  "v8", "v9", "v10", "v11",
	  "v12", "v13", "v14", "v15",
	  "v16", "v17", "v18", "v19",
	  "v20", "v21", "v22", "v23",
	  "v24", "v25", "v26", "v27",
	  "v28", "v29", "v30", "v31",
	};

      if (p_regnum >= AARCH64_SVE_V0_REGNUM
	  && p_regnum < AARCH64_SVE_V0_REGNUM + AARCH64_V_REGS_NUM)
	return sve_v_name[p_regnum - AARCH64_SVE_V0_REGNUM];
    }

  if (is_sme_pseudo_register (gdbarch, regnum))
    return tdep->sme_pseudo_names[regnum - tdep->sme_pseudo_base].c_str ();

  /* RA_STATE is used for unwinding only.  Do not assign it a name - this
     prevents it from being read by methods such as
     mi_cmd_trace_frame_collected.  */
  if (tdep->has_pauth () && regnum == tdep->ra_sign_state_regnum)
    return "";

  internal_error (_("aarch64_pseudo_register_name: bad register number %d"),
		  p_regnum);
}

/* Implement the "pseudo_register_type" tdesc_arch_data method.  */

static struct type *
aarch64_pseudo_register_type (struct gdbarch *gdbarch, int regnum)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  int p_regnum = regnum - gdbarch_num_regs (gdbarch);

  if (p_regnum >= AARCH64_Q0_REGNUM && p_regnum < AARCH64_Q0_REGNUM + 32)
    return aarch64_vnq_type (gdbarch);

  if (p_regnum >= AARCH64_D0_REGNUM && p_regnum < AARCH64_D0_REGNUM + 32)
    return aarch64_vnd_type (gdbarch);

  if (p_regnum >= AARCH64_S0_REGNUM && p_regnum < AARCH64_S0_REGNUM + 32)
    return aarch64_vns_type (gdbarch);

  if (p_regnum >= AARCH64_H0_REGNUM && p_regnum < AARCH64_H0_REGNUM + 32)
    return aarch64_vnh_type (gdbarch);

  if (p_regnum >= AARCH64_B0_REGNUM && p_regnum < AARCH64_B0_REGNUM + 32)
    return aarch64_vnb_type (gdbarch);

  if (tdep->has_sve () && p_regnum >= AARCH64_SVE_V0_REGNUM
      && p_regnum < AARCH64_SVE_V0_REGNUM + AARCH64_V_REGS_NUM)
    return aarch64_vnv_type (gdbarch);

  /* W pseudo-registers are 32-bit.  */
  if (is_w_pseudo_register (gdbarch, regnum))
    return builtin_type (gdbarch)->builtin_uint32;

  if (is_sme_pseudo_register (gdbarch, regnum))
    return aarch64_sme_pseudo_register_type (gdbarch, regnum);

  if (tdep->has_pauth () && regnum == tdep->ra_sign_state_regnum)
    return builtin_type (gdbarch)->builtin_uint64;

  internal_error (_("aarch64_pseudo_register_type: bad register number %d"),
		  p_regnum);
}

/* Implement the "pseudo_register_reggroup_p" tdesc_arch_data method.  */

static int
aarch64_pseudo_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
				    const struct reggroup *group)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  int p_regnum = regnum - gdbarch_num_regs (gdbarch);

  if (p_regnum >= AARCH64_Q0_REGNUM && p_regnum < AARCH64_Q0_REGNUM + 32)
    return group == all_reggroup || group == vector_reggroup;
  else if (p_regnum >= AARCH64_D0_REGNUM && p_regnum < AARCH64_D0_REGNUM + 32)
    return (group == all_reggroup || group == vector_reggroup
	    || group == float_reggroup);
  else if (p_regnum >= AARCH64_S0_REGNUM && p_regnum < AARCH64_S0_REGNUM + 32)
    return (group == all_reggroup || group == vector_reggroup
	    || group == float_reggroup);
  else if (p_regnum >= AARCH64_H0_REGNUM && p_regnum < AARCH64_H0_REGNUM + 32)
    return group == all_reggroup || group == vector_reggroup;
  else if (p_regnum >= AARCH64_B0_REGNUM && p_regnum < AARCH64_B0_REGNUM + 32)
    return group == all_reggroup || group == vector_reggroup;
  else if (tdep->has_sve () && p_regnum >= AARCH64_SVE_V0_REGNUM
	   && p_regnum < AARCH64_SVE_V0_REGNUM + AARCH64_V_REGS_NUM)
    return group == all_reggroup || group == vector_reggroup;
  else if (is_sme_pseudo_register (gdbarch, regnum))
    return group == all_reggroup || group == vector_reggroup;
  /* RA_STATE is used for unwinding only.  Do not assign it to any groups.  */
  if (tdep->has_pauth () && regnum == tdep->ra_sign_state_regnum)
    return 0;

  return group == all_reggroup;
}

/* Helper for aarch64_pseudo_read_value.  */

static value *
aarch64_pseudo_read_value_1 (frame_info_ptr next_frame,
			     const int pseudo_reg_num, int raw_regnum_offset)
{
  unsigned v_regnum = AARCH64_V0_REGNUM + raw_regnum_offset;

  return pseudo_from_raw_part (next_frame, pseudo_reg_num, v_regnum, 0);
}

/* Helper function for reading/writing ZA pseudo-registers.  Given REGNUM,
   a ZA pseudo-register number, return the information on positioning of the
   bytes that must be read from/written to.  */

static za_offsets
aarch64_za_offsets_from_regnum (struct gdbarch *gdbarch, int regnum)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  gdb_assert (tdep->has_sme ());
  gdb_assert (tdep->sme_svq > 0);
  gdb_assert (tdep->sme_pseudo_base <= regnum);
  gdb_assert (regnum < tdep->sme_pseudo_base + tdep->sme_pseudo_count);

  struct za_pseudo_encoding encoding;

  /* Decode the ZA pseudo-register number.  */
  aarch64_za_decode_pseudos (gdbarch, regnum, encoding);

  /* Fetch the streaming vector length.  */
  size_t svl = sve_vl_from_vq (tdep->sme_svq);
  za_offsets offsets;

  if (is_sme_tile_slice_pseudo_register (gdbarch, regnum))
    {
      if (encoding.horizontal)
	{
	  /* Horizontal tile slices are contiguous ranges of svl bytes.  */

	  /* The starting offset depends on the tile index (to locate the tile
	     in the ZA buffer), the slice index (to locate the slice within the
	     tile) and the qualifier.  */
	  offsets.starting_offset
	    = encoding.tile_index * svl + encoding.slice_index
					  * (svl >> encoding.qualifier_index);
	  /* Horizontal tile slice data is contiguous and thus doesn't have
	     a stride.  */
	  offsets.stride_size = 0;
	  /* Horizontal tile slice data is contiguous and thus only has 1
	     chunk.  */
	  offsets.chunks = 1;
	  /* The chunk size is always svl bytes.  */
	  offsets.chunk_size = svl;
	}
      else
	{
	  /* Vertical tile slices are non-contiguous ranges of
	     (1 << qualifier_index) bytes.  */

	  /* The starting offset depends on the tile number (to locate the
	     tile in the ZA buffer), the slice index (to locate the element
	     within the tile slice) and the qualifier.  */
	  offsets.starting_offset
	    = encoding.tile_index * svl + encoding.slice_index
					  * (1 << encoding.qualifier_index);
	  /* The offset between vertical tile slices depends on the qualifier
	     and svl.  */
	  offsets.stride_size = svl << encoding.qualifier_index;
	  /* The number of chunks depends on svl and the qualifier size.  */
	  offsets.chunks = svl >> encoding.qualifier_index;
	  /* The chunk size depends on the qualifier.  */
	  offsets.chunk_size = 1 << encoding.qualifier_index;
	}
    }
  else
    {
      /* ZA tile pseudo-register.  */

      /* Starting offset depends on the tile index and qualifier.  */
      offsets.starting_offset = encoding.tile_index * svl;
      /* The offset between tile slices depends on the qualifier and svl.  */
      offsets.stride_size = svl << encoding.qualifier_index;
      /* The number of chunks depends on the qualifier and svl.  */
      offsets.chunks = svl >> encoding.qualifier_index;
      /* The chunk size is always svl bytes.  */
      offsets.chunk_size = svl;
    }

  return offsets;
}

/* Given REGNUM, a SME pseudo-register number, return its value in RESULT.  */

static value *
aarch64_sme_pseudo_register_read (gdbarch *gdbarch, frame_info_ptr next_frame,
				  const int pseudo_reg_num)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  gdb_assert (tdep->has_sme ());
  gdb_assert (tdep->sme_svq > 0);
  gdb_assert (tdep->sme_pseudo_base <= pseudo_reg_num);
  gdb_assert (pseudo_reg_num < tdep->sme_pseudo_base + tdep->sme_pseudo_count);

  /* Fetch the offsets that we need in order to read from the correct blocks
     of ZA.  */
  za_offsets offsets
    = aarch64_za_offsets_from_regnum (gdbarch, pseudo_reg_num);

  /* Fetch the contents of ZA.  */
  value *za_value = value_of_register (tdep->sme_za_regnum, next_frame);
  value *result = value::allocate_register (next_frame, pseudo_reg_num);

  /* Copy the requested data.  */
  for (int chunks = 0; chunks < offsets.chunks; chunks++)
    {
      int src_offset = offsets.starting_offset + chunks * offsets.stride_size;
      int dst_offset = chunks * offsets.chunk_size;
      za_value->contents_copy (result, dst_offset, src_offset,
			       offsets.chunk_size);
    }

  return result;
}

/* Implement the "pseudo_register_read_value" gdbarch method.  */

static value *
aarch64_pseudo_read_value (gdbarch *gdbarch, frame_info_ptr next_frame,
			   const int pseudo_reg_num)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  if (is_w_pseudo_register (gdbarch, pseudo_reg_num))
    {
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      /* Default offset for little endian.  */
      int offset = 0;

      if (byte_order == BFD_ENDIAN_BIG)
	offset = 4;

      /* Find the correct X register to extract the data from.  */
      int x_regnum
	= AARCH64_X0_REGNUM + (pseudo_reg_num - tdep->w_pseudo_base);

      /* Read the bottom 4 bytes of X.  */
      return pseudo_from_raw_part (next_frame, pseudo_reg_num, x_regnum,
				   offset);
    }
  else if (is_sme_pseudo_register (gdbarch, pseudo_reg_num))
    return aarch64_sme_pseudo_register_read (gdbarch, next_frame,
					     pseudo_reg_num);

  /* Offset in the "pseudo-register space".  */
  int pseudo_offset = pseudo_reg_num - gdbarch_num_regs (gdbarch);

  if (pseudo_offset >= AARCH64_Q0_REGNUM
      && pseudo_offset < AARCH64_Q0_REGNUM + 32)
    return aarch64_pseudo_read_value_1 (next_frame, pseudo_reg_num,
					pseudo_offset - AARCH64_Q0_REGNUM);

  if (pseudo_offset >= AARCH64_D0_REGNUM
      && pseudo_offset < AARCH64_D0_REGNUM + 32)
    return aarch64_pseudo_read_value_1 (next_frame, pseudo_reg_num,
					pseudo_offset - AARCH64_D0_REGNUM);

  if (pseudo_offset >= AARCH64_S0_REGNUM
      && pseudo_offset < AARCH64_S0_REGNUM + 32)
    return aarch64_pseudo_read_value_1 (next_frame, pseudo_reg_num,
					pseudo_offset - AARCH64_S0_REGNUM);

  if (pseudo_offset >= AARCH64_H0_REGNUM
      && pseudo_offset < AARCH64_H0_REGNUM + 32)
    return aarch64_pseudo_read_value_1 (next_frame, pseudo_reg_num,
					pseudo_offset - AARCH64_H0_REGNUM);

  if (pseudo_offset >= AARCH64_B0_REGNUM
      && pseudo_offset < AARCH64_B0_REGNUM + 32)
    return aarch64_pseudo_read_value_1 (next_frame, pseudo_reg_num,
					pseudo_offset - AARCH64_B0_REGNUM);

  if (tdep->has_sve () && pseudo_offset >= AARCH64_SVE_V0_REGNUM
      && pseudo_offset < AARCH64_SVE_V0_REGNUM + 32)
    return aarch64_pseudo_read_value_1 (next_frame, pseudo_reg_num,
					pseudo_offset - AARCH64_SVE_V0_REGNUM);

  gdb_assert_not_reached ("regnum out of bound");
}

/* Helper for aarch64_pseudo_write.  */

static void
aarch64_pseudo_write_1 (gdbarch *gdbarch, frame_info_ptr next_frame,
			int regnum_offset,
			gdb::array_view<const gdb_byte> buf)
{
  unsigned raw_regnum = AARCH64_V0_REGNUM + regnum_offset;

  /* Enough space for a full vector register.  */
  int raw_reg_size = register_size (gdbarch, raw_regnum);
  gdb_byte raw_buf[raw_reg_size];
  static_assert (AARCH64_V0_REGNUM == AARCH64_SVE_Z0_REGNUM);

  /* Ensure the register buffer is zero, we want gdb writes of the
     various 'scalar' pseudo registers to behavior like architectural
     writes, register width bytes are written the remainder are set to
     zero.  */
  memset (raw_buf, 0, register_size (gdbarch, AARCH64_V0_REGNUM));

  gdb::array_view<gdb_byte> raw_view (raw_buf, raw_reg_size);
  copy (buf, raw_view.slice (0, buf.size ()));
  put_frame_register (next_frame, raw_regnum, raw_view);
}

/* Given REGNUM, a SME pseudo-register number, store the bytes from DATA to the
   pseudo-register.  */

static void
aarch64_sme_pseudo_register_write (gdbarch *gdbarch, frame_info_ptr next_frame,
				   const int regnum,
				   gdb::array_view<const gdb_byte> data)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  gdb_assert (tdep->has_sme ());
  gdb_assert (tdep->sme_svq > 0);
  gdb_assert (tdep->sme_pseudo_base <= regnum);
  gdb_assert (regnum < tdep->sme_pseudo_base + tdep->sme_pseudo_count);

  /* Fetch the offsets that we need in order to write to the correct blocks
     of ZA.  */
  za_offsets offsets = aarch64_za_offsets_from_regnum (gdbarch, regnum);

  /* Fetch the contents of ZA.  */
  value *za_value = value_of_register (tdep->sme_za_regnum, next_frame);

  {
    /* Create a view only on the portion of za we want to write.  */
    gdb::array_view<gdb_byte> za_view
      = za_value->contents_writeable ().slice (offsets.starting_offset);

    /* Copy the requested data.  */
    for (int chunks = 0; chunks < offsets.chunks; chunks++)
      {
	gdb::array_view<const gdb_byte> src
	  = data.slice (chunks * offsets.chunk_size, offsets.chunk_size);
	gdb::array_view<gdb_byte> dst
	  = za_view.slice (chunks * offsets.stride_size, offsets.chunk_size);
	copy (src, dst);
      }
  }

  /* Write back to ZA.  */
  put_frame_register (next_frame, tdep->sme_za_regnum,
		      za_value->contents_raw ());
}

/* Implement the "pseudo_register_write" gdbarch method.  */

static void
aarch64_pseudo_write (gdbarch *gdbarch, frame_info_ptr next_frame,
		      const int pseudo_reg_num,
		      gdb::array_view<const gdb_byte> buf)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  if (is_w_pseudo_register (gdbarch, pseudo_reg_num))
    {
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      /* Default offset for little endian.  */
      int offset = 0;

      if (byte_order == BFD_ENDIAN_BIG)
	offset = 4;

      /* Find the correct X register to extract the data from.  */
      int x_regnum = AARCH64_X0_REGNUM + (pseudo_reg_num - tdep->w_pseudo_base);

      /* First zero-out the contents of X.  */
      gdb_byte bytes[8] {};
      gdb::array_view<gdb_byte> bytes_view (bytes);
      copy (buf, bytes_view.slice (offset, 4));

      /* Write to the bottom 4 bytes of X.  */
      put_frame_register (next_frame, x_regnum, bytes_view);
      return;
    }
  else if (is_sme_pseudo_register (gdbarch, pseudo_reg_num))
    {
      aarch64_sme_pseudo_register_write (gdbarch, next_frame, pseudo_reg_num,
					 buf);
      return;
    }

  /* Offset in the "pseudo-register space".  */
  int pseudo_offset = pseudo_reg_num - gdbarch_num_regs (gdbarch);

  if (pseudo_offset >= AARCH64_Q0_REGNUM
      && pseudo_offset < AARCH64_Q0_REGNUM + 32)
    return aarch64_pseudo_write_1 (gdbarch, next_frame,
				   pseudo_offset - AARCH64_Q0_REGNUM, buf);

  if (pseudo_offset >= AARCH64_D0_REGNUM
      && pseudo_offset < AARCH64_D0_REGNUM + 32)
    return aarch64_pseudo_write_1 (gdbarch, next_frame,
				   pseudo_offset - AARCH64_D0_REGNUM, buf);

  if (pseudo_offset >= AARCH64_S0_REGNUM
      && pseudo_offset < AARCH64_S0_REGNUM + 32)
    return aarch64_pseudo_write_1 (gdbarch, next_frame,
				   pseudo_offset - AARCH64_S0_REGNUM, buf);

  if (pseudo_offset >= AARCH64_H0_REGNUM
      && pseudo_offset < AARCH64_H0_REGNUM + 32)
    return aarch64_pseudo_write_1 (gdbarch, next_frame,
				   pseudo_offset - AARCH64_H0_REGNUM, buf);

  if (pseudo_offset >= AARCH64_B0_REGNUM
      && pseudo_offset < AARCH64_B0_REGNUM + 32)
    return aarch64_pseudo_write_1 (gdbarch, next_frame,
				   pseudo_offset - AARCH64_B0_REGNUM, buf);

  if (tdep->has_sve () && pseudo_offset >= AARCH64_SVE_V0_REGNUM
      && pseudo_offset < AARCH64_SVE_V0_REGNUM + 32)
    return aarch64_pseudo_write_1 (gdbarch, next_frame,
				   pseudo_offset - AARCH64_SVE_V0_REGNUM, buf);

  gdb_assert_not_reached ("regnum out of bound");
}

/* Callback function for user_reg_add.  */

static struct value *
value_of_aarch64_user_reg (frame_info_ptr frame, const void *baton)
{
  const int *reg_p = (const int *) baton;

  return value_of_register (*reg_p, get_next_frame_sentinel_okay (frame));
}

/* Implement the "software_single_step" gdbarch method, needed to
   single step through atomic sequences on AArch64.  */

static std::vector<CORE_ADDR>
aarch64_software_single_step (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  const int insn_size = 4;
  const int atomic_sequence_length = 16; /* Instruction sequence length.  */
  CORE_ADDR pc = regcache_read_pc (regcache);
  CORE_ADDR breaks[2] = { CORE_ADDR_MAX, CORE_ADDR_MAX };
  CORE_ADDR loc = pc;
  CORE_ADDR closing_insn = 0;

  ULONGEST insn_from_memory;
  if (!safe_read_memory_unsigned_integer (loc, insn_size,
					  byte_order_for_code,
					  &insn_from_memory))
  {
    /* Assume we don't have a atomic sequence, as we couldn't read the
       instruction in this location.  */
    return {};
  }

  uint32_t insn = insn_from_memory;
  int index;
  int insn_count;
  int bc_insn_count = 0; /* Conditional branch instruction count.  */
  int last_breakpoint = 0; /* Defaults to 0 (no breakpoints placed).  */
  aarch64_inst inst;

  if (aarch64_decode_insn (insn, &inst, 1, NULL) != 0)
    return {};

  /* Look for a Load Exclusive instruction which begins the sequence.  */
  if (inst.opcode->iclass != ldstexcl || bit (insn, 22) == 0)
    return {};

  for (insn_count = 0; insn_count < atomic_sequence_length; ++insn_count)
    {
      loc += insn_size;

      if (!safe_read_memory_unsigned_integer (loc, insn_size,
					      byte_order_for_code,
					      &insn_from_memory))
      {
	/* Assume we don't have a atomic sequence, as we couldn't read the
	   instruction in this location.  */
	return {};
      }

      insn = insn_from_memory;
      if (aarch64_decode_insn (insn, &inst, 1, NULL) != 0)
	return {};
      /* Check if the instruction is a conditional branch.  */
      if (inst.opcode->iclass == condbranch)
	{
	  gdb_assert (inst.operands[0].type == AARCH64_OPND_ADDR_PCREL19);

	  if (bc_insn_count >= 1)
	    return {};

	  /* It is, so we'll try to set a breakpoint at the destination.  */
	  breaks[1] = loc + inst.operands[0].imm.value;

	  bc_insn_count++;
	  last_breakpoint++;
	}

      /* Look for the Store Exclusive which closes the atomic sequence.  */
      if (inst.opcode->iclass == ldstexcl && bit (insn, 22) == 0)
	{
	  closing_insn = loc;
	  break;
	}
    }

  /* We didn't find a closing Store Exclusive instruction, fall back.  */
  if (!closing_insn)
    return {};

  /* Insert breakpoint after the end of the atomic sequence.  */
  breaks[0] = loc + insn_size;

  /* Check for duplicated breakpoints, and also check that the second
     breakpoint is not within the atomic sequence.  */
  if (last_breakpoint
      && (breaks[1] == breaks[0]
	  || (breaks[1] >= pc && breaks[1] <= closing_insn)))
    last_breakpoint = 0;

  std::vector<CORE_ADDR> next_pcs;

  /* Insert the breakpoint at the end of the sequence, and one at the
     destination of the conditional branch, if it exists.  */
  for (index = 0; index <= last_breakpoint; index++)
    next_pcs.push_back (breaks[index]);

  return next_pcs;
}

struct aarch64_displaced_step_copy_insn_closure
  : public displaced_step_copy_insn_closure
{
  /* It is true when condition instruction, such as B.CON, TBZ, etc,
     is being displaced stepping.  */
  bool cond = false;

  /* PC adjustment offset after displaced stepping.  If 0, then we don't
     write the PC back, assuming the PC is already the right address.  */
  int32_t pc_adjust = 0;
};

/* Data when visiting instructions for displaced stepping.  */

struct aarch64_displaced_step_data
{
  struct aarch64_insn_data base;

  /* The address where the instruction will be executed at.  */
  CORE_ADDR new_addr;
  /* Buffer of instructions to be copied to NEW_ADDR to execute.  */
  uint32_t insn_buf[AARCH64_DISPLACED_MODIFIED_INSNS];
  /* Number of instructions in INSN_BUF.  */
  unsigned insn_count;
  /* Registers when doing displaced stepping.  */
  struct regcache *regs;

  aarch64_displaced_step_copy_insn_closure *dsc;
};

/* Implementation of aarch64_insn_visitor method "b".  */

static void
aarch64_displaced_step_b (const int is_bl, const int32_t offset,
			  struct aarch64_insn_data *data)
{
  struct aarch64_displaced_step_data *dsd
    = (struct aarch64_displaced_step_data *) data;
  int64_t new_offset = data->insn_addr - dsd->new_addr + offset;

  if (can_encode_int32 (new_offset, 28))
    {
      /* Emit B rather than BL, because executing BL on a new address
	 will get the wrong address into LR.  In order to avoid this,
	 we emit B, and update LR if the instruction is BL.  */
      emit_b (dsd->insn_buf, 0, new_offset);
      dsd->insn_count++;
    }
  else
    {
      /* Write NOP.  */
      emit_nop (dsd->insn_buf);
      dsd->insn_count++;
      dsd->dsc->pc_adjust = offset;
    }

  if (is_bl)
    {
      /* Update LR.  */
      regcache_cooked_write_unsigned (dsd->regs, AARCH64_LR_REGNUM,
				      data->insn_addr + 4);
    }
}

/* Implementation of aarch64_insn_visitor method "b_cond".  */

static void
aarch64_displaced_step_b_cond (const unsigned cond, const int32_t offset,
			       struct aarch64_insn_data *data)
{
  struct aarch64_displaced_step_data *dsd
    = (struct aarch64_displaced_step_data *) data;

  /* GDB has to fix up PC after displaced step this instruction
     differently according to the condition is true or false.  Instead
     of checking COND against conditional flags, we can use
     the following instructions, and GDB can tell how to fix up PC
     according to the PC value.

     B.COND TAKEN    ; If cond is true, then jump to TAKEN.
     INSN1     ;
     TAKEN:
     INSN2
  */

  emit_bcond (dsd->insn_buf, cond, 8);
  dsd->dsc->cond = true;
  dsd->dsc->pc_adjust = offset;
  dsd->insn_count = 1;
}

/* Dynamically allocate a new register.  If we know the register
   statically, we should make it a global as above instead of using this
   helper function.  */

static struct aarch64_register
aarch64_register (unsigned num, int is64)
{
  return (struct aarch64_register) { num, is64 };
}

/* Implementation of aarch64_insn_visitor method "cb".  */

static void
aarch64_displaced_step_cb (const int32_t offset, const int is_cbnz,
			   const unsigned rn, int is64,
			   struct aarch64_insn_data *data)
{
  struct aarch64_displaced_step_data *dsd
    = (struct aarch64_displaced_step_data *) data;

  /* The offset is out of range for a compare and branch
     instruction.  We can use the following instructions instead:

	 CBZ xn, TAKEN   ; xn == 0, then jump to TAKEN.
	 INSN1     ;
	 TAKEN:
	 INSN2
  */
  emit_cb (dsd->insn_buf, is_cbnz, aarch64_register (rn, is64), 8);
  dsd->insn_count = 1;
  dsd->dsc->cond = true;
  dsd->dsc->pc_adjust = offset;
}

/* Implementation of aarch64_insn_visitor method "tb".  */

static void
aarch64_displaced_step_tb (const int32_t offset, int is_tbnz,
			   const unsigned rt, unsigned bit,
			   struct aarch64_insn_data *data)
{
  struct aarch64_displaced_step_data *dsd
    = (struct aarch64_displaced_step_data *) data;

  /* The offset is out of range for a test bit and branch
     instruction We can use the following instructions instead:

     TBZ xn, #bit, TAKEN ; xn[bit] == 0, then jump to TAKEN.
     INSN1         ;
     TAKEN:
     INSN2

  */
  emit_tb (dsd->insn_buf, is_tbnz, bit, aarch64_register (rt, 1), 8);
  dsd->insn_count = 1;
  dsd->dsc->cond = true;
  dsd->dsc->pc_adjust = offset;
}

/* Implementation of aarch64_insn_visitor method "adr".  */

static void
aarch64_displaced_step_adr (const int32_t offset, const unsigned rd,
			    const int is_adrp, struct aarch64_insn_data *data)
{
  struct aarch64_displaced_step_data *dsd
    = (struct aarch64_displaced_step_data *) data;
  /* We know exactly the address the ADR{P,} instruction will compute.
     We can just write it to the destination register.  */
  CORE_ADDR address = data->insn_addr + offset;

  if (is_adrp)
    {
      /* Clear the lower 12 bits of the offset to get the 4K page.  */
      regcache_cooked_write_unsigned (dsd->regs, AARCH64_X0_REGNUM + rd,
				      address & ~0xfff);
    }
  else
      regcache_cooked_write_unsigned (dsd->regs, AARCH64_X0_REGNUM + rd,
				      address);

  dsd->dsc->pc_adjust = 4;
  emit_nop (dsd->insn_buf);
  dsd->insn_count = 1;
}

/* Implementation of aarch64_insn_visitor method "ldr_literal".  */

static void
aarch64_displaced_step_ldr_literal (const int32_t offset, const int is_sw,
				    const unsigned rt, const int is64,
				    struct aarch64_insn_data *data)
{
  struct aarch64_displaced_step_data *dsd
    = (struct aarch64_displaced_step_data *) data;
  CORE_ADDR address = data->insn_addr + offset;
  struct aarch64_memory_operand zero = { MEMORY_OPERAND_OFFSET, 0 };

  regcache_cooked_write_unsigned (dsd->regs, AARCH64_X0_REGNUM + rt,
				  address);

  if (is_sw)
    dsd->insn_count = emit_ldrsw (dsd->insn_buf, aarch64_register (rt, 1),
				  aarch64_register (rt, 1), zero);
  else
    dsd->insn_count = emit_ldr (dsd->insn_buf, aarch64_register (rt, is64),
				aarch64_register (rt, 1), zero);

  dsd->dsc->pc_adjust = 4;
}

/* Implementation of aarch64_insn_visitor method "others".  */

static void
aarch64_displaced_step_others (const uint32_t insn,
			       struct aarch64_insn_data *data)
{
  struct aarch64_displaced_step_data *dsd
    = (struct aarch64_displaced_step_data *) data;

  uint32_t masked_insn = (insn & CLEAR_Rn_MASK);
  if (masked_insn == BLR)
    {
      /* Emit a BR to the same register and then update LR to the original
	 address (similar to aarch64_displaced_step_b).  */
      aarch64_emit_insn (dsd->insn_buf, insn & 0xffdfffff);
      regcache_cooked_write_unsigned (dsd->regs, AARCH64_LR_REGNUM,
				      data->insn_addr + 4);
    }
  else
    aarch64_emit_insn (dsd->insn_buf, insn);
  dsd->insn_count = 1;

  if (masked_insn == RET || masked_insn == BR || masked_insn == BLR)
    dsd->dsc->pc_adjust = 0;
  else
    dsd->dsc->pc_adjust = 4;
}

static const struct aarch64_insn_visitor visitor =
{
  aarch64_displaced_step_b,
  aarch64_displaced_step_b_cond,
  aarch64_displaced_step_cb,
  aarch64_displaced_step_tb,
  aarch64_displaced_step_adr,
  aarch64_displaced_step_ldr_literal,
  aarch64_displaced_step_others,
};

/* Implement the "displaced_step_copy_insn" gdbarch method.  */

displaced_step_copy_insn_closure_up
aarch64_displaced_step_copy_insn (struct gdbarch *gdbarch,
				  CORE_ADDR from, CORE_ADDR to,
				  struct regcache *regs)
{
  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);
  struct aarch64_displaced_step_data dsd;
  aarch64_inst inst;
  ULONGEST insn_from_memory;

  if (!safe_read_memory_unsigned_integer (from, 4, byte_order_for_code,
					  &insn_from_memory))
    return nullptr;

  uint32_t insn = insn_from_memory;

  if (aarch64_decode_insn (insn, &inst, 1, NULL) != 0)
    return NULL;

  /* Look for a Load Exclusive instruction which begins the sequence.  */
  if (inst.opcode->iclass == ldstexcl && bit (insn, 22))
    {
      /* We can't displaced step atomic sequences.  */
      return NULL;
    }

  std::unique_ptr<aarch64_displaced_step_copy_insn_closure> dsc
    (new aarch64_displaced_step_copy_insn_closure);
  dsd.base.insn_addr = from;
  dsd.new_addr = to;
  dsd.regs = regs;
  dsd.dsc = dsc.get ();
  dsd.insn_count = 0;
  aarch64_relocate_instruction (insn, &visitor,
				(struct aarch64_insn_data *) &dsd);
  gdb_assert (dsd.insn_count <= AARCH64_DISPLACED_MODIFIED_INSNS);

  if (dsd.insn_count != 0)
    {
      int i;

      /* Instruction can be relocated to scratch pad.  Copy
	 relocated instruction(s) there.  */
      for (i = 0; i < dsd.insn_count; i++)
	{
	  displaced_debug_printf ("writing insn %.8x at %s",
				  dsd.insn_buf[i],
				  paddress (gdbarch, to + i * 4));

	  write_memory_unsigned_integer (to + i * 4, 4, byte_order_for_code,
					 (ULONGEST) dsd.insn_buf[i]);
	}
    }
  else
    {
      dsc = NULL;
    }

  /* This is a work around for a problem with g++ 4.8.  */
  return displaced_step_copy_insn_closure_up (dsc.release ());
}

/* Implement the "displaced_step_fixup" gdbarch method.  */

void
aarch64_displaced_step_fixup (struct gdbarch *gdbarch,
			      struct displaced_step_copy_insn_closure *dsc_,
			      CORE_ADDR from, CORE_ADDR to,
			      struct regcache *regs, bool completed_p)
{
  CORE_ADDR pc = regcache_read_pc (regs);

  /* If the displaced instruction didn't complete successfully then all we
     need to do is restore the program counter.  */
  if (!completed_p)
    {
      pc = from + (pc - to);
      regcache_write_pc (regs, pc);
      return;
    }

  aarch64_displaced_step_copy_insn_closure *dsc
    = (aarch64_displaced_step_copy_insn_closure *) dsc_;

  displaced_debug_printf ("PC after stepping: %s (was %s).",
			  paddress (gdbarch, pc), paddress (gdbarch, to));

  if (dsc->cond)
    {
      displaced_debug_printf ("[Conditional] pc_adjust before: %d",
			      dsc->pc_adjust);

      if (pc - to == 8)
	{
	  /* Condition is true.  */
	}
      else if (pc - to == 4)
	{
	  /* Condition is false.  */
	  dsc->pc_adjust = 4;
	}
      else
	gdb_assert_not_reached ("Unexpected PC value after displaced stepping");

      displaced_debug_printf ("[Conditional] pc_adjust after: %d",
			      dsc->pc_adjust);
    }

  displaced_debug_printf ("%s PC by %d",
			  dsc->pc_adjust ? "adjusting" : "not adjusting",
			  dsc->pc_adjust);

  if (dsc->pc_adjust != 0)
    {
      /* Make sure the previous instruction was executed (that is, the PC
	 has changed).  If the PC didn't change, then discard the adjustment
	 offset.  Otherwise we may skip an instruction before its execution
	 took place.  */
      if ((pc - to) == 0)
	{
	  displaced_debug_printf ("PC did not move. Discarding PC adjustment.");
	  dsc->pc_adjust = 0;
	}

      displaced_debug_printf ("fixup: set PC to %s:%d",
			      paddress (gdbarch, from), dsc->pc_adjust);

      regcache_cooked_write_unsigned (regs, AARCH64_PC_REGNUM,
				      from + dsc->pc_adjust);
    }
}

/* Implement the "displaced_step_hw_singlestep" gdbarch method.  */

bool
aarch64_displaced_step_hw_singlestep (struct gdbarch *gdbarch)
{
  return true;
}

/* Get the correct target description for the given VQ value.
   If VQ is zero then it is assumed SVE is not supported.
   (It is not possible to set VQ to zero on an SVE system).

   MTE_P indicates the presence of the Memory Tagging Extension feature.

   TLS_P indicates the presence of the Thread Local Storage feature.  */

const target_desc *
aarch64_read_description (const aarch64_features &features)
{
  if (features.vq > AARCH64_MAX_SVE_VQ)
    error (_("VQ is %" PRIu64 ", maximum supported value is %d"), features.vq,
	   AARCH64_MAX_SVE_VQ);

  struct target_desc *tdesc = tdesc_aarch64_map[features];

  if (tdesc == NULL)
    {
      tdesc = aarch64_create_target_description (features);
      tdesc_aarch64_map[features] = tdesc;
    }

  return tdesc;
}

/* Return the VQ used when creating the target description TDESC.  */

static uint64_t
aarch64_get_tdesc_vq (const struct target_desc *tdesc)
{
  const struct tdesc_feature *feature_sve;

  if (!tdesc_has_registers (tdesc))
    return 0;

  feature_sve = tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.sve");

  if (feature_sve == nullptr)
    return 0;

  uint64_t vl = tdesc_register_bitsize (feature_sve,
					aarch64_sve_register_names[0]) / 8;
  return sve_vq_from_vl (vl);
}


/* Return the svq (streaming vector quotient) used when creating the target
   description TDESC.  */

static uint64_t
aarch64_get_tdesc_svq (const struct target_desc *tdesc)
{
  const struct tdesc_feature *feature_sme;

  if (!tdesc_has_registers (tdesc))
    return 0;

  feature_sme = tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.sme");

  if (feature_sme == nullptr)
    return 0;

  size_t svl_squared = tdesc_register_bitsize (feature_sme, "za");

  /* We have the total size of the ZA matrix, in bits.  Figure out the svl
     value.  */
  size_t svl = std::sqrt (svl_squared / 8);

  /* Now extract svq.  */
  return sve_vq_from_vl (svl);
}

/* Get the AArch64 features present in the given target description. */

aarch64_features
aarch64_features_from_target_desc (const struct target_desc *tdesc)
{
  aarch64_features features;

  if (tdesc == nullptr)
    return features;

  features.vq = aarch64_get_tdesc_vq (tdesc);

  /* We need to look for a couple pauth feature name variations.  */
  features.pauth
      = (tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.pauth") != nullptr);

  if (!features.pauth)
    features.pauth = (tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.pauth_v2")
		      != nullptr);

  features.mte
      = (tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.mte") != nullptr);

  const struct tdesc_feature *tls_feature
    = tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.tls");

  if (tls_feature != nullptr)
    {
      /* We have TLS registers.  Find out how many.  */
      if (tdesc_unnumbered_register (tls_feature, "tpidr2"))
	features.tls = 2;
      else
	features.tls = 1;
    }

  features.svq = aarch64_get_tdesc_svq (tdesc);

  /* Check for the SME2 feature.  */
  features.sme2 = (tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.sme2")
		   != nullptr);

  return features;
}

/* Implement the "cannot_store_register" gdbarch method.  */

static int
aarch64_cannot_store_register (struct gdbarch *gdbarch, int regnum)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  if (!tdep->has_pauth ())
    return 0;

  /* Pointer authentication registers are read-only.  */
  return (regnum >= tdep->pauth_reg_base
	  && regnum < tdep->pauth_reg_base + tdep->pauth_reg_count);
}

/* Implement the stack_frame_destroyed_p gdbarch method.  */

static int
aarch64_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_start, func_end;
  if (!find_pc_partial_function (pc, NULL, &func_start, &func_end))
    return 0;

  enum bfd_endian byte_order_for_code = gdbarch_byte_order_for_code (gdbarch);

  ULONGEST insn_from_memory;
  if (!safe_read_memory_unsigned_integer (pc, 4, byte_order_for_code,
					  &insn_from_memory))
    return 0;

  uint32_t insn = insn_from_memory;

  aarch64_inst inst;
  if (aarch64_decode_insn (insn, &inst, 1, nullptr) != 0)
    return 0;

  return streq (inst.opcode->name, "ret");
}

/* AArch64 implementation of the remove_non_address_bits gdbarch hook.  Remove
   non address bits from a pointer value.  */

static CORE_ADDR
aarch64_remove_non_address_bits (struct gdbarch *gdbarch, CORE_ADDR pointer)
{
  /* By default, we assume TBI and discard the top 8 bits plus the VA range
     select bit (55).  Below we try to fetch information about pointer
     authentication masks in order to make non-address removal more
     precise.  */
  CORE_ADDR mask = AARCH64_TOP_BITS_MASK;

  /* Check if we have an inferior first.  If not, just use the default
     mask.

     We use the inferior_ptid here because the pointer authentication masks
     should be the same across threads of a process.  Since we may not have
     access to the current thread (gdb may have switched to no inferiors
     momentarily), we use the inferior ptid.  */
  if (inferior_ptid != null_ptid)
    {
      /* If we do have an inferior, attempt to fetch its thread's thread_info
	 struct.  */
      thread_info *thread = current_inferior ()->find_thread (inferior_ptid);

      /* If the thread is running, we will not be able to fetch the mask
	 registers.  */
      if (thread != nullptr && thread->state != THREAD_RUNNING)
	{
	  /* Otherwise, fetch the register cache and the masks.  */
	  struct regcache *regs
	    = get_thread_regcache (current_inferior ()->process_target (),
				   inferior_ptid);

	  /* Use the gdbarch from the register cache to check for pointer
	     authentication support, as it matches the features found in
	     that particular thread.  */
	  aarch64_gdbarch_tdep *tdep
	    = gdbarch_tdep<aarch64_gdbarch_tdep> (regs->arch ());

	  /* Is there pointer authentication support?  */
	  if (tdep->has_pauth ())
	    {
	      CORE_ADDR cmask, dmask;
	      int dmask_regnum
		= AARCH64_PAUTH_DMASK_REGNUM (tdep->pauth_reg_base);
	      int cmask_regnum
		= AARCH64_PAUTH_CMASK_REGNUM (tdep->pauth_reg_base);

	      /* If we have a kernel address and we have kernel-mode address
		 mask registers, use those instead.  */
	      if (tdep->pauth_reg_count > 2
		  && pointer & VA_RANGE_SELECT_BIT_MASK)
		{
		  dmask_regnum
		    = AARCH64_PAUTH_DMASK_HIGH_REGNUM (tdep->pauth_reg_base);
		  cmask_regnum
		    = AARCH64_PAUTH_CMASK_HIGH_REGNUM (tdep->pauth_reg_base);
		}

	      /* We have both a code mask and a data mask.  For now they are
		 the same, but this may change in the future.  */
	      if (regs->cooked_read (dmask_regnum, &dmask) != REG_VALID)
		dmask = mask;

	      if (regs->cooked_read (cmask_regnum, &cmask) != REG_VALID)
		cmask = mask;

	      mask |= aarch64_mask_from_pac_registers (cmask, dmask);
	    }
	}
    }

  return aarch64_remove_top_bits (pointer, mask);
}

/* Given NAMES, a vector of strings, initialize it with all the SME
   pseudo-register names for the current streaming vector length.  */

static void
aarch64_initialize_sme_pseudo_names (struct gdbarch *gdbarch,
				     std::vector<std::string> &names)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  gdb_assert (tdep->has_sme ());
  gdb_assert (tdep->sme_tile_slice_pseudo_base > 0);
  gdb_assert (tdep->sme_tile_pseudo_base > 0);

  for (int i = 0; i < tdep->sme_tile_slice_pseudo_count; i++)
    {
      int regnum = tdep->sme_tile_slice_pseudo_base + i;
      struct za_pseudo_encoding encoding;
      aarch64_za_decode_pseudos (gdbarch, regnum, encoding);
      names.push_back (aarch64_za_tile_slice_name (encoding));
    }
  for (int i = 0; i < AARCH64_ZA_TILES_NUM; i++)
    {
      int regnum = tdep->sme_tile_pseudo_base + i;
      struct za_pseudo_encoding encoding;
      aarch64_za_decode_pseudos (gdbarch, regnum, encoding);
      names.push_back (aarch64_za_tile_name (encoding));
    }
}

/* Initialize the current architecture based on INFO.  If possible,
   re-use an architecture from ARCHES, which is a list of
   architectures already created during this debugging session.

   Called e.g. at program startup, when reading a core file, and when
   reading a binary file.  */

static struct gdbarch *
aarch64_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  const struct tdesc_feature *feature_core, *feature_fpu, *feature_sve;
  const struct tdesc_feature *feature_pauth;
  bool valid_p = true;
  int i, num_regs = 0, num_pseudo_regs = 0;
  int first_pauth_regnum = -1, ra_sign_state_offset = -1;
  int first_mte_regnum = -1, first_tls_regnum = -1;
  uint64_t vq = aarch64_get_tdesc_vq (info.target_desc);
  uint64_t svq = aarch64_get_tdesc_svq (info.target_desc);

  if (vq > AARCH64_MAX_SVE_VQ)
    internal_error (_("VQ out of bounds: %s (max %d)"),
		    pulongest (vq), AARCH64_MAX_SVE_VQ);

  if (svq > AARCH64_MAX_SVE_VQ)
    internal_error (_("Streaming vector quotient (svq) out of bounds: %s"
		      " (max %d)"),
		    pulongest (svq), AARCH64_MAX_SVE_VQ);

  /* If there is already a candidate, use it.  */
  for (gdbarch_list *best_arch = gdbarch_list_lookup_by_info (arches, &info);
       best_arch != nullptr;
       best_arch = gdbarch_list_lookup_by_info (best_arch->next, &info))
    {
      aarch64_gdbarch_tdep *tdep
	= gdbarch_tdep<aarch64_gdbarch_tdep> (best_arch->gdbarch);
      if (tdep && tdep->vq == vq && tdep->sme_svq == svq)
	return best_arch->gdbarch;
    }

  /* Ensure we always have a target descriptor, and that it is for the given VQ
     value.  */
  const struct target_desc *tdesc = info.target_desc;
  if (!tdesc_has_registers (tdesc) || vq != aarch64_get_tdesc_vq (tdesc)
      || svq != aarch64_get_tdesc_svq (tdesc))
    {
      aarch64_features features;
      features.vq = vq;
      features.svq = svq;
      tdesc = aarch64_read_description (features);
    }
  gdb_assert (tdesc);

  feature_core = tdesc_find_feature (tdesc,"org.gnu.gdb.aarch64.core");
  feature_fpu = tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.fpu");
  feature_sve = tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.sve");
  const struct tdesc_feature *feature_mte
    = tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.mte");
  const struct tdesc_feature *feature_tls
    = tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.tls");

  if (feature_core == nullptr)
    return nullptr;

  tdesc_arch_data_up tdesc_data = tdesc_data_alloc ();

  /* Validate the description provides the mandatory core R registers
     and allocate their numbers.  */
  for (i = 0; i < ARRAY_SIZE (aarch64_r_register_names); i++)
    valid_p &= tdesc_numbered_register (feature_core, tdesc_data.get (),
					AARCH64_X0_REGNUM + i,
					aarch64_r_register_names[i]);

  num_regs = AARCH64_X0_REGNUM + i;

  /* Add the V registers.  */
  if (feature_fpu != nullptr)
    {
      if (feature_sve != nullptr)
	error (_("Program contains both fpu and SVE features."));

      /* Validate the description provides the mandatory V registers
	 and allocate their numbers.  */
      for (i = 0; i < ARRAY_SIZE (aarch64_v_register_names); i++)
	valid_p &= tdesc_numbered_register (feature_fpu, tdesc_data.get (),
					    AARCH64_V0_REGNUM + i,
					    aarch64_v_register_names[i]);

      num_regs = AARCH64_V0_REGNUM + i;
    }

  /* Add the SVE registers.  */
  if (feature_sve != nullptr)
    {
      /* Validate the description provides the mandatory SVE registers
	 and allocate their numbers.  */
      for (i = 0; i < ARRAY_SIZE (aarch64_sve_register_names); i++)
	valid_p &= tdesc_numbered_register (feature_sve, tdesc_data.get (),
					    AARCH64_SVE_Z0_REGNUM + i,
					    aarch64_sve_register_names[i]);

      num_regs = AARCH64_SVE_Z0_REGNUM + i;
      num_pseudo_regs += 32;	/* add the Vn register pseudos.  */
    }

  if (feature_fpu != nullptr || feature_sve != nullptr)
    {
      num_pseudo_regs += 32;	/* add the Qn scalar register pseudos */
      num_pseudo_regs += 32;	/* add the Dn scalar register pseudos */
      num_pseudo_regs += 32;	/* add the Sn scalar register pseudos */
      num_pseudo_regs += 32;	/* add the Hn scalar register pseudos */
      num_pseudo_regs += 32;	/* add the Bn scalar register pseudos */
    }

  int first_sme_regnum = -1;
  int first_sme2_regnum = -1;
  int first_sme_pseudo_regnum = -1;
  const struct tdesc_feature *feature_sme
    = tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.sme");
  if (feature_sme != nullptr)
    {
      /* Record the first SME register.  */
      first_sme_regnum = num_regs;

      valid_p &= tdesc_numbered_register (feature_sme, tdesc_data.get (),
					  num_regs++, "svg");

      valid_p &= tdesc_numbered_register (feature_sme, tdesc_data.get (),
					  num_regs++, "svcr");

      valid_p &= tdesc_numbered_register (feature_sme, tdesc_data.get (),
					  num_regs++, "za");

      /* Record the first SME pseudo register.  */
      first_sme_pseudo_regnum = num_pseudo_regs;

      /* Add the ZA tile slice pseudo registers.  The number of tile slice
	 pseudo-registers depend on the svl, and is always a multiple of 5.  */
      num_pseudo_regs += (svq << 5) * 5;

      /* Add the ZA tile pseudo registers.  */
      num_pseudo_regs += AARCH64_ZA_TILES_NUM;

      /* Now check for the SME2 feature.  SME2 is only available if SME is
	 available.  */
      const struct tdesc_feature *feature_sme2
	= tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.sme2");
      if (feature_sme2 != nullptr)
	{
	  /* Record the first SME2 register.  */
	  first_sme2_regnum = num_regs;

	  valid_p &= tdesc_numbered_register (feature_sme2, tdesc_data.get (),
					      num_regs++, "zt0");
	}
    }

  /* Add the TLS register.  */
  int tls_register_count = 0;
  if (feature_tls != nullptr)
    {
      first_tls_regnum = num_regs;

      /* Look for the TLS registers.  tpidr is required, but tpidr2 is
	 optional.  */
      valid_p
	= tdesc_numbered_register (feature_tls, tdesc_data.get (),
				   first_tls_regnum, "tpidr");

      if (valid_p)
	{
	  tls_register_count++;

	  bool has_tpidr2
	    = tdesc_numbered_register (feature_tls, tdesc_data.get (),
				       first_tls_regnum + tls_register_count,
				       "tpidr2");

	  /* Figure out how many TLS registers we have.  */
	  if (has_tpidr2)
	    tls_register_count++;

	  num_regs += tls_register_count;
	}
      else
	{
	  warning (_("Provided TLS register feature doesn't contain "
		     "required tpidr register."));
	  return nullptr;
	}
    }

  /* We have two versions of the pauth target description due to a past bug
     where GDB would crash when seeing the first version of the pauth target
     description.  */
  feature_pauth = tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.pauth");
  if (feature_pauth == nullptr)
    feature_pauth = tdesc_find_feature (tdesc, "org.gnu.gdb.aarch64.pauth_v2");

  /* Add the pauth registers.  */
  int pauth_masks = 0;
  if (feature_pauth != NULL)
    {
      first_pauth_regnum = num_regs;
      ra_sign_state_offset = num_pseudo_regs;

      /* Size of the expected register set with all 4 masks.  */
      int set_size = ARRAY_SIZE (aarch64_pauth_register_names);

      /* QEMU exposes a couple additional masks for the high half of the
	 address.  We should either have 2 registers or 4 registers.  */
      if (tdesc_unnumbered_register (feature_pauth,
				     "pauth_dmask_high") == 0)
	{
	  /* We did not find pauth_dmask_high, assume we only have
	     2 masks.  We are not dealing with QEMU/Emulators then.  */
	  set_size -= 2;
	}

      /* Validate the descriptor provides the mandatory PAUTH registers and
	 allocate their numbers.  */
      for (i = 0; i < set_size; i++)
	valid_p &= tdesc_numbered_register (feature_pauth, tdesc_data.get (),
					    first_pauth_regnum + i,
					    aarch64_pauth_register_names[i]);

      num_regs += i;
      num_pseudo_regs += 1;	/* Count RA_STATE pseudo register.  */
      pauth_masks = set_size;
    }

  /* Add the MTE registers.  */
  if (feature_mte != NULL)
    {
      first_mte_regnum = num_regs;
      /* Validate the descriptor provides the mandatory MTE registers and
	 allocate their numbers.  */
      for (i = 0; i < ARRAY_SIZE (aarch64_mte_register_names); i++)
	valid_p &= tdesc_numbered_register (feature_mte, tdesc_data.get (),
					    first_mte_regnum + i,
					    aarch64_mte_register_names[i]);

      num_regs += i;
    }
    /* W pseudo-registers */
    int first_w_regnum = num_pseudo_regs;
    num_pseudo_regs += 31;

  if (!valid_p)
    return nullptr;

  /* AArch64 code is always little-endian.  */
  info.byte_order_for_code = BFD_ENDIAN_LITTLE;

  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new aarch64_gdbarch_tdep));
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  /* This should be low enough for everything.  */
  tdep->lowest_pc = 0x20;
  tdep->jb_pc = -1;		/* Longjump support not enabled by default.  */
  tdep->jb_elt_size = 8;
  tdep->vq = vq;
  tdep->pauth_reg_base = first_pauth_regnum;
  tdep->pauth_reg_count = pauth_masks;
  tdep->ra_sign_state_regnum = -1;
  tdep->mte_reg_base = first_mte_regnum;
  tdep->tls_regnum_base = first_tls_regnum;
  tdep->tls_register_count = tls_register_count;

  /* Set the SME register set details.  The pseudo-registers will be adjusted
     later.  */
  tdep->sme_reg_base = first_sme_regnum;
  tdep->sme_svg_regnum = first_sme_regnum;
  tdep->sme_svcr_regnum = first_sme_regnum + 1;
  tdep->sme_za_regnum = first_sme_regnum + 2;
  tdep->sme_svq = svq;

  /* Set the SME2 register set details.  */
  tdep->sme2_zt0_regnum = first_sme2_regnum;

  set_gdbarch_push_dummy_call (gdbarch, aarch64_push_dummy_call);
  set_gdbarch_frame_align (gdbarch, aarch64_frame_align);

  /* Advance PC across function entry code.  */
  set_gdbarch_skip_prologue (gdbarch, aarch64_skip_prologue);

  /* The stack grows downward.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  /* Breakpoint manipulation.  */
  set_gdbarch_breakpoint_kind_from_pc (gdbarch,
				       aarch64_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch,
				       aarch64_breakpoint::bp_from_kind);
  set_gdbarch_have_nonsteppable_watchpoint (gdbarch, 1);
  set_gdbarch_software_single_step (gdbarch, aarch64_software_single_step);

  /* Information about registers, etc.  */
  set_gdbarch_sp_regnum (gdbarch, AARCH64_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, AARCH64_PC_REGNUM);
  set_gdbarch_num_regs (gdbarch, num_regs);

  set_gdbarch_num_pseudo_regs (gdbarch, num_pseudo_regs);
  set_gdbarch_pseudo_register_read_value (gdbarch, aarch64_pseudo_read_value);
  set_gdbarch_pseudo_register_write (gdbarch, aarch64_pseudo_write);
  set_tdesc_pseudo_register_name (gdbarch, aarch64_pseudo_register_name);
  set_tdesc_pseudo_register_type (gdbarch, aarch64_pseudo_register_type);
  set_tdesc_pseudo_register_reggroup_p (gdbarch,
					aarch64_pseudo_register_reggroup_p);
  set_gdbarch_cannot_store_register (gdbarch, aarch64_cannot_store_register);

  /* ABI */
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 128);
  set_gdbarch_long_bit (gdbarch, 64);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_ptr_bit (gdbarch, 64);
  set_gdbarch_char_signed (gdbarch, 0);
  set_gdbarch_wchar_signed (gdbarch, 0);
  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_double_format (gdbarch, floatformats_ieee_double);
  set_gdbarch_long_double_format (gdbarch, floatformats_ieee_quad);
  set_gdbarch_type_align (gdbarch, aarch64_type_align);

  /* Detect whether PC is at a point where the stack has been destroyed.  */
  set_gdbarch_stack_frame_destroyed_p (gdbarch, aarch64_stack_frame_destroyed_p);

  /* Internal <-> external register number maps.  */
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, aarch64_dwarf_reg_to_regnum);

  /* Returning results.  */
  set_gdbarch_return_value_as_value (gdbarch, aarch64_return_value);

  /* Disassembly.  */
  set_gdbarch_print_insn (gdbarch, aarch64_gdb_print_insn);

  /* Virtual tables.  */
  set_gdbarch_vbit_in_delta (gdbarch, 1);

  /* Hook in the ABI-specific overrides, if they have been registered.  */
  info.target_desc = tdesc;
  info.tdesc_data = tdesc_data.get ();
  gdbarch_init_osabi (info, gdbarch);

  dwarf2_frame_set_init_reg (gdbarch, aarch64_dwarf2_frame_init_reg);
  /* Register DWARF CFA vendor handler.  */
  set_gdbarch_execute_dwarf_cfa_vendor_op (gdbarch,
					   aarch64_execute_dwarf_cfa_vendor_op);

  /* Permanent/Program breakpoint handling.  */
  set_gdbarch_program_breakpoint_here_p (gdbarch,
					 aarch64_program_breakpoint_here_p);

  /* Add some default predicates.  */
  frame_unwind_append_unwinder (gdbarch, &aarch64_stub_unwind);
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &aarch64_prologue_unwind);

  frame_base_set_default (gdbarch, &aarch64_normal_base);

  /* Now we have tuned the configuration, set a few final things,
     based on what the OS ABI has told us.  */

  if (tdep->jb_pc >= 0)
    set_gdbarch_get_longjmp_target (gdbarch, aarch64_get_longjmp_target);

  set_gdbarch_gen_return_address (gdbarch, aarch64_gen_return_address);

  set_gdbarch_get_pc_address_flags (gdbarch, aarch64_get_pc_address_flags);

  tdesc_use_registers (gdbarch, tdesc, std::move (tdesc_data));

  /* Fetch the updated number of registers after we're done adding all
     entries from features we don't explicitly care about.  This is the case
     for bare metal debugging stubs that include a lot of system registers.  */
  num_regs = gdbarch_num_regs (gdbarch);

  /* With the number of real registers updated, setup the pseudo-registers and
     record their numbers.  */

  /* Setup W pseudo-register numbers.  */
  tdep->w_pseudo_base = first_w_regnum + num_regs;
  tdep->w_pseudo_count = 31;

  /* Pointer authentication pseudo-registers.  */
  if (tdep->has_pauth ())
    tdep->ra_sign_state_regnum = ra_sign_state_offset + num_regs;

  /* Architecture hook to remove bits of a pointer that are not part of the
     address, like memory tags (MTE) and pointer authentication signatures.  */
  set_gdbarch_remove_non_address_bits (gdbarch,
				       aarch64_remove_non_address_bits);

  /* SME pseudo-registers.  */
  if (tdep->has_sme ())
    {
      tdep->sme_pseudo_base = num_regs + first_sme_pseudo_regnum;
      tdep->sme_tile_slice_pseudo_base = tdep->sme_pseudo_base;
      tdep->sme_tile_slice_pseudo_count = (svq * 32) * 5;
      tdep->sme_tile_pseudo_base
	= tdep->sme_pseudo_base + tdep->sme_tile_slice_pseudo_count;
      tdep->sme_pseudo_count
	= tdep->sme_tile_slice_pseudo_count + AARCH64_ZA_TILES_NUM;

      /* The SME ZA pseudo-registers are a set of 160 to 2560 pseudo-registers
	 depending on the value of svl.

	 The tile pseudo-registers are organized around their qualifiers
	 (b, h, s, d and q).  Their numbers are distributed as follows:

	 b 0
	 h 1~2
	 s 3~6
	 d 7~14
	 q 15~30

	 The naming of the tile pseudo-registers follows the pattern za<t><q>,
	 where:

	 <t> is the tile number, with the following possible values based on
	 the qualifiers:

	 Qualifier - Allocated indexes

	 b - 0
	 h - 0~1
	 s - 0~3
	 d - 0~7
	 q - 0~15

	 <q> is the qualifier: b, h, s, d and q.

	 The tile slice pseudo-registers are organized around their
	 qualifiers as well (b, h, s, d and q), but also around their
	 direction (h - horizontal and v - vertical).

	 Even-numbered tile slice pseudo-registers are horizontally-oriented
	 and odd-numbered tile slice pseudo-registers are vertically-oriented.

	 Their numbers are distributed as follows:

	 Qualifier - Allocated indexes

	 b tile slices - 0~511
	 h tile slices - 512~1023
	 s tile slices - 1024~1535
	 d tile slices - 1536~2047
	 q tile slices - 2048~2559

	 The naming of the tile slice pseudo-registers follows the pattern
	 za<t><d><q><s>, where:

	 <t> is the tile number as described for the tile pseudo-registers.
	 <d> is the direction of the tile slice (h or v)
	 <q> is the qualifier of the tile slice (b, h, s, d or q)
	 <s> is the slice number, defined as follows:

	 Qualifier - Allocated indexes

	 b - 0~15
	 h - 0~7
	 s - 0~3
	 d - 0~1
	 q - 0

	 We have helper functions to translate to/from register index from/to
	 the set of fields that make the pseudo-register names.  */

      /* Build the array of pseudo-register names available for this
	 particular gdbarch configuration.  */
      aarch64_initialize_sme_pseudo_names (gdbarch, tdep->sme_pseudo_names);
    }

  /* Add standard register aliases.  */
  for (i = 0; i < ARRAY_SIZE (aarch64_register_aliases); i++)
    user_reg_add (gdbarch, aarch64_register_aliases[i].name,
		  value_of_aarch64_user_reg,
		  &aarch64_register_aliases[i].regnum);

  register_aarch64_ravenscar_ops (gdbarch);

  return gdbarch;
}

static void
aarch64_dump_tdep (struct gdbarch *gdbarch, struct ui_file *file)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  if (tdep == NULL)
    return;

  gdb_printf (file, _("aarch64_dump_tdep: Lowest pc = 0x%s\n"),
	      paddress (gdbarch, tdep->lowest_pc));

  /* SME fields.  */
  gdb_printf (file, _("aarch64_dump_tdep: sme_tile_type_q = %s\n"),
	      host_address_to_string (tdep->sme_tile_type_q));
  gdb_printf (file, _("aarch64_dump_tdep: sme_tile_type_d = %s\n"),
	      host_address_to_string (tdep->sme_tile_type_d));
  gdb_printf (file, _("aarch64_dump_tdep: sme_tile_type_s = %s\n"),
	      host_address_to_string (tdep->sme_tile_type_s));
  gdb_printf (file, _("aarch64_dump_tdep: sme_tile_type_h = %s\n"),
	      host_address_to_string (tdep->sme_tile_type_h));
  gdb_printf (file, _("aarch64_dump_tdep: sme_tile_type_n = %s\n"),
	      host_address_to_string (tdep->sme_tile_type_b));
  gdb_printf (file, _("aarch64_dump_tdep: sme_tile_slice_type_q = %s\n"),
	      host_address_to_string (tdep->sme_tile_slice_type_q));
  gdb_printf (file, _("aarch64_dump_tdep: sme_tile_slice_type_d = %s\n"),
	      host_address_to_string (tdep->sme_tile_slice_type_d));
  gdb_printf (file, _("aarch64_dump_tdep: sme_tile_slice_type_s = %s\n"),
	      host_address_to_string (tdep->sme_tile_slice_type_s));
  gdb_printf (file, _("aarch64_dump_tdep: sme_tile_slice_type_h = %s\n"),
	      host_address_to_string (tdep->sme_tile_slice_type_h));
  gdb_printf (file, _("aarch64_dump_tdep: sme_tile_slice_type_b = %s\n"),
	      host_address_to_string (tdep->sme_tile_slice_type_b));
  gdb_printf (file, _("aarch64_dump_tdep: sme_reg_base = %s\n"),
	      pulongest (tdep->sme_reg_base));
  gdb_printf (file, _("aarch64_dump_tdep: sme_svg_regnum = %s\n"),
	      pulongest (tdep->sme_svg_regnum));
  gdb_printf (file, _("aarch64_dump_tdep: sme_svcr_regnum = %s\n"),
	      pulongest (tdep->sme_svcr_regnum));
  gdb_printf (file, _("aarch64_dump_tdep: sme_za_regnum = %s\n"),
	      pulongest (tdep->sme_za_regnum));
  gdb_printf (file, _("aarch64_dump_tdep: sme_pseudo_base = %s\n"),
	      pulongest (tdep->sme_pseudo_base));
  gdb_printf (file, _("aarch64_dump_tdep: sme_pseudo_count = %s\n"),
	      pulongest (tdep->sme_pseudo_count));
  gdb_printf (file, _("aarch64_dump_tdep: sme_tile_slice_pseudo_base = %s\n"),
	      pulongest (tdep->sme_tile_slice_pseudo_base));
  gdb_printf (file, _("aarch64_dump_tdep: sme_tile_slice_pseudo_count = %s\n"),
	      pulongest (tdep->sme_tile_slice_pseudo_count));
  gdb_printf (file, _("aarch64_dump_tdep: sme_tile_pseudo_base = %s\n"),
	      pulongest (tdep->sme_tile_pseudo_base));
  gdb_printf (file, _("aarch64_dump_tdep: sme_svq = %s\n"),
	      pulongest (tdep->sme_svq));
}

#if GDB_SELF_TEST
namespace selftests
{
static void aarch64_process_record_test (void);
}
#endif

void _initialize_aarch64_tdep ();
void
_initialize_aarch64_tdep ()
{
  gdbarch_register (bfd_arch_aarch64, aarch64_gdbarch_init,
		    aarch64_dump_tdep);

  /* Debug this file's internals.  */
  add_setshow_boolean_cmd ("aarch64", class_maintenance, &aarch64_debug, _("\
Set AArch64 debugging."), _("\
Show AArch64 debugging."), _("\
When on, AArch64 specific debugging is enabled."),
			    NULL,
			    show_aarch64_debug,
			    &setdebuglist, &showdebuglist);

#if GDB_SELF_TEST
  selftests::register_test ("aarch64-analyze-prologue",
			    selftests::aarch64_analyze_prologue_test);
  selftests::register_test ("aarch64-process-record",
			    selftests::aarch64_process_record_test);
#endif
}

/* AArch64 process record-replay related structures, defines etc.  */

#define REG_ALLOC(REGS, LENGTH, RECORD_BUF) \
	do  \
	  { \
	    unsigned int reg_len = LENGTH; \
	    if (reg_len) \
	      { \
		REGS = XNEWVEC (uint32_t, reg_len); \
		memcpy(&REGS[0], &RECORD_BUF[0], sizeof(uint32_t)*LENGTH); \
	      } \
	  } \
	while (0)

#define MEM_ALLOC(MEMS, LENGTH, RECORD_BUF) \
	do  \
	  { \
	    unsigned int mem_len = LENGTH; \
	    if (mem_len) \
	      { \
		MEMS =  XNEWVEC (struct aarch64_mem_r, mem_len);  \
		memcpy(MEMS, &RECORD_BUF[0], \
		       sizeof(struct aarch64_mem_r) * LENGTH); \
	      } \
	  } \
	  while (0)

/* AArch64 record/replay structures and enumerations.  */

struct aarch64_mem_r
{
  uint64_t len;    /* Record length.  */
  uint64_t addr;   /* Memory address.  */
};

enum aarch64_record_result
{
  AARCH64_RECORD_SUCCESS,
  AARCH64_RECORD_UNSUPPORTED,
  AARCH64_RECORD_UNKNOWN
};

struct aarch64_insn_decode_record
{
  struct gdbarch *gdbarch;
  struct regcache *regcache;
  CORE_ADDR this_addr;                 /* Address of insn to be recorded.  */
  uint32_t aarch64_insn;               /* Insn to be recorded.  */
  uint32_t mem_rec_count;              /* Count of memory records.  */
  uint32_t reg_rec_count;              /* Count of register records.  */
  uint32_t *aarch64_regs;              /* Registers to be recorded.  */
  struct aarch64_mem_r *aarch64_mems;  /* Memory locations to be recorded.  */
};

/* Record handler for data processing - register instructions.  */

static unsigned int
aarch64_record_data_proc_reg (aarch64_insn_decode_record *aarch64_insn_r)
{
  uint8_t reg_rd, insn_bits24_27, insn_bits21_23;
  uint32_t record_buf[4];

  reg_rd = bits (aarch64_insn_r->aarch64_insn, 0, 4);
  insn_bits24_27 = bits (aarch64_insn_r->aarch64_insn, 24, 27);
  insn_bits21_23 = bits (aarch64_insn_r->aarch64_insn, 21, 23);

  if (!bit (aarch64_insn_r->aarch64_insn, 28))
    {
      uint8_t setflags;

      /* Logical (shifted register).  */
      if (insn_bits24_27 == 0x0a)
	setflags = (bits (aarch64_insn_r->aarch64_insn, 29, 30) == 0x03);
      /* Add/subtract.  */
      else if (insn_bits24_27 == 0x0b)
	setflags = bit (aarch64_insn_r->aarch64_insn, 29);
      else
	return AARCH64_RECORD_UNKNOWN;

      record_buf[0] = reg_rd;
      aarch64_insn_r->reg_rec_count = 1;
      if (setflags)
	record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_CPSR_REGNUM;
    }
  else
    {
      if (insn_bits24_27 == 0x0b)
	{
	  /* Data-processing (3 source).  */
	  record_buf[0] = reg_rd;
	  aarch64_insn_r->reg_rec_count = 1;
	}
      else if (insn_bits24_27 == 0x0a)
	{
	  if (insn_bits21_23 == 0x00)
	    {
	      /* Add/subtract (with carry).  */
	      record_buf[0] = reg_rd;
	      aarch64_insn_r->reg_rec_count = 1;
	      if (bit (aarch64_insn_r->aarch64_insn, 29))
		{
		  record_buf[1] = AARCH64_CPSR_REGNUM;
		  aarch64_insn_r->reg_rec_count = 2;
		}
	    }
	  else if (insn_bits21_23 == 0x02)
	    {
	      /* Conditional compare (register) and conditional compare
		 (immediate) instructions.  */
	      record_buf[0] = AARCH64_CPSR_REGNUM;
	      aarch64_insn_r->reg_rec_count = 1;
	    }
	  else if (insn_bits21_23 == 0x04 || insn_bits21_23 == 0x06)
	    {
	      /* Conditional select.  */
	      /* Data-processing (2 source).  */
	      /* Data-processing (1 source).  */
	      record_buf[0] = reg_rd;
	      aarch64_insn_r->reg_rec_count = 1;
	    }
	  else
	    return AARCH64_RECORD_UNKNOWN;
	}
    }

  REG_ALLOC (aarch64_insn_r->aarch64_regs, aarch64_insn_r->reg_rec_count,
	     record_buf);
  return AARCH64_RECORD_SUCCESS;
}

/* Record handler for data processing - immediate instructions.  */

static unsigned int
aarch64_record_data_proc_imm (aarch64_insn_decode_record *aarch64_insn_r)
{
  uint8_t reg_rd, insn_bit23, insn_bits24_27, setflags;
  uint32_t record_buf[4];

  reg_rd = bits (aarch64_insn_r->aarch64_insn, 0, 4);
  insn_bit23 = bit (aarch64_insn_r->aarch64_insn, 23);
  insn_bits24_27 = bits (aarch64_insn_r->aarch64_insn, 24, 27);

  if (insn_bits24_27 == 0x00                     /* PC rel addressing.  */
     || insn_bits24_27 == 0x03                   /* Bitfield and Extract.  */
     || (insn_bits24_27 == 0x02 && insn_bit23))  /* Move wide (immediate).  */
    {
      record_buf[0] = reg_rd;
      aarch64_insn_r->reg_rec_count = 1;
    }
  else if (insn_bits24_27 == 0x01)
    {
      /* Add/Subtract (immediate).  */
      setflags = bit (aarch64_insn_r->aarch64_insn, 29);
      record_buf[0] = reg_rd;
      aarch64_insn_r->reg_rec_count = 1;
      if (setflags)
	record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_CPSR_REGNUM;
    }
  else if (insn_bits24_27 == 0x02 && !insn_bit23)
    {
      /* Logical (immediate).  */
      setflags = bits (aarch64_insn_r->aarch64_insn, 29, 30) == 0x03;
      record_buf[0] = reg_rd;
      aarch64_insn_r->reg_rec_count = 1;
      if (setflags)
	record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_CPSR_REGNUM;
    }
  else
    return AARCH64_RECORD_UNKNOWN;

  REG_ALLOC (aarch64_insn_r->aarch64_regs, aarch64_insn_r->reg_rec_count,
	     record_buf);
  return AARCH64_RECORD_SUCCESS;
}

/* Record handler for branch, exception generation and system instructions.  */

static unsigned int
aarch64_record_branch_except_sys (aarch64_insn_decode_record *aarch64_insn_r)
{

  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (aarch64_insn_r->gdbarch);
  uint8_t insn_bits24_27, insn_bits28_31, insn_bits22_23;
  uint32_t record_buf[4];

  insn_bits24_27 = bits (aarch64_insn_r->aarch64_insn, 24, 27);
  insn_bits28_31 = bits (aarch64_insn_r->aarch64_insn, 28, 31);
  insn_bits22_23 = bits (aarch64_insn_r->aarch64_insn, 22, 23);

  if (insn_bits28_31 == 0x0d)
    {
      /* Exception generation instructions. */
      if (insn_bits24_27 == 0x04)
	{
	  if (!bits (aarch64_insn_r->aarch64_insn, 2, 4)
	      && !bits (aarch64_insn_r->aarch64_insn, 21, 23)
	      && bits (aarch64_insn_r->aarch64_insn, 0, 1) == 0x01)
	    {
	      ULONGEST svc_number;

	      regcache_raw_read_unsigned (aarch64_insn_r->regcache, 8,
					  &svc_number);
	      return tdep->aarch64_syscall_record (aarch64_insn_r->regcache,
						   svc_number);
	    }
	  else
	    return AARCH64_RECORD_UNSUPPORTED;
	}
      /* System instructions. */
      else if (insn_bits24_27 == 0x05 && insn_bits22_23 == 0x00)
	{
	  uint32_t reg_rt, reg_crn;

	  reg_rt = bits (aarch64_insn_r->aarch64_insn, 0, 4);
	  reg_crn = bits (aarch64_insn_r->aarch64_insn, 12, 15);

	  /* Record rt in case of sysl and mrs instructions.  */
	  if (bit (aarch64_insn_r->aarch64_insn, 21))
	    {
	      record_buf[0] = reg_rt;
	      aarch64_insn_r->reg_rec_count = 1;
	    }
	  /* Record cpsr for hint and msr(immediate) instructions.  */
	  else if (reg_crn == 0x02 || reg_crn == 0x04)
	    {
	      record_buf[0] = AARCH64_CPSR_REGNUM;
	      aarch64_insn_r->reg_rec_count = 1;
	    }
	}
      /* Unconditional branch (register).  */
      else if((insn_bits24_27 & 0x0e) == 0x06)
	{
	  record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_PC_REGNUM;
	  if (bits (aarch64_insn_r->aarch64_insn, 21, 22) == 0x01)
	    record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_LR_REGNUM;
	}
      else
	return AARCH64_RECORD_UNKNOWN;
    }
  /* Unconditional branch (immediate).  */
  else if ((insn_bits28_31 & 0x07) == 0x01 && (insn_bits24_27 & 0x0c) == 0x04)
    {
      record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_PC_REGNUM;
      if (bit (aarch64_insn_r->aarch64_insn, 31))
	record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_LR_REGNUM;
    }
  else
    /* Compare & branch (immediate), Test & branch (immediate) and
       Conditional branch (immediate).  */
    record_buf[aarch64_insn_r->reg_rec_count++] = AARCH64_PC_REGNUM;

  REG_ALLOC (aarch64_insn_r->aarch64_regs, aarch64_insn_r->reg_rec_count,
	     record_buf);
  return AARCH64_RECORD_SUCCESS;
}

/* Record handler for advanced SIMD load and store instructions.  */

static unsigned int
aarch64_record_asimd_load_store (aarch64_insn_decode_record *aarch64_insn_r)
{
  CORE_ADDR address;
  uint64_t addr_offset = 0;
  uint32_t record_buf[24];
  uint64_t record_buf_mem[24];
  uint32_t reg_rn, reg_rt;
  uint32_t reg_index = 0, mem_index = 0;
  uint8_t opcode_bits, size_bits;

  reg_rt = bits (aarch64_insn_r->aarch64_insn, 0, 4);
  reg_rn = bits (aarch64_insn_r->aarch64_insn, 5, 9);
  size_bits = bits (aarch64_insn_r->aarch64_insn, 10, 11);
  opcode_bits = bits (aarch64_insn_r->aarch64_insn, 12, 15);
  regcache_raw_read_unsigned (aarch64_insn_r->regcache, reg_rn, &address);

  if (record_debug)
    debug_printf ("Process record: Advanced SIMD load/store\n");

  /* Load/store single structure.  */
  if (bit (aarch64_insn_r->aarch64_insn, 24))
    {
      uint8_t sindex, scale, selem, esize, replicate = 0;
      scale = opcode_bits >> 2;
      selem = ((opcode_bits & 0x02) |
	      bit (aarch64_insn_r->aarch64_insn, 21)) + 1;
      switch (scale)
	{
	case 1:
	  if (size_bits & 0x01)
	    return AARCH64_RECORD_UNKNOWN;
	  break;
	case 2:
	  if ((size_bits >> 1) & 0x01)
	    return AARCH64_RECORD_UNKNOWN;
	  if (size_bits & 0x01)
	    {
	      if (!((opcode_bits >> 1) & 0x01))
		scale = 3;
	      else
		return AARCH64_RECORD_UNKNOWN;
	    }
	  break;
	case 3:
	  if (bit (aarch64_insn_r->aarch64_insn, 22) && !(opcode_bits & 0x01))
	    {
	      scale = size_bits;
	      replicate = 1;
	      break;
	    }
	  else
	    return AARCH64_RECORD_UNKNOWN;
	default:
	  break;
	}
      esize = 8 << scale;
      if (replicate)
	for (sindex = 0; sindex < selem; sindex++)
	  {
	    record_buf[reg_index++] = reg_rt + AARCH64_V0_REGNUM;
	    reg_rt = (reg_rt + 1) % 32;
	  }
      else
	{
	  for (sindex = 0; sindex < selem; sindex++)
	    {
	      if (bit (aarch64_insn_r->aarch64_insn, 22))
		record_buf[reg_index++] = reg_rt + AARCH64_V0_REGNUM;
	      else
		{
		  record_buf_mem[mem_index++] = esize / 8;
		  record_buf_mem[mem_index++] = address + addr_offset;
		}
	      addr_offset = addr_offset + (esize / 8);
	      reg_rt = (reg_rt + 1) % 32;
	    }
	}
    }
  /* Load/store multiple structure.  */
  else
    {
      uint8_t selem, esize, rpt, elements;
      uint8_t eindex, rindex;

      esize = 8 << size_bits;
      if (bit (aarch64_insn_r->aarch64_insn, 30))
	elements = 128 / esize;
      else
	elements = 64 / esize;

      switch (opcode_bits)
	{
	/*LD/ST4 (4 Registers).  */
	case 0:
	  rpt = 1;
	  selem = 4;
	  break;
	/*LD/ST1 (4 Registers).  */
	case 2:
	  rpt = 4;
	  selem = 1;
	  break;
	/*LD/ST3 (3 Registers).  */
	case 4:
	  rpt = 1;
	  selem = 3;
	  break;
	/*LD/ST1 (3 Registers).  */
	case 6:
	  rpt = 3;
	  selem = 1;
	  break;
	/*LD/ST1 (1 Register).  */
	case 7:
	  rpt = 1;
	  selem = 1;
	  break;
	/*LD/ST2 (2 Registers).  */
	case 8:
	  rpt = 1;
	  selem = 2;
	  break;
	/*LD/ST1 (2 Registers).  */
	case 10:
	  rpt = 2;
	  selem = 1;
	  break;
	default:
	  return AARCH64_RECORD_UNSUPPORTED;
	  break;
	}
      for (rindex = 0; rindex < rpt; rindex++)
	for (eindex = 0; eindex < elements; eindex++)
	  {
	    uint8_t reg_tt, sindex;
	    reg_tt = (reg_rt + rindex) % 32;
	    for (sindex = 0; sindex < selem; sindex++)
	      {
		if (bit (aarch64_insn_r->aarch64_insn, 22))
		  record_buf[reg_index++] = reg_tt + AARCH64_V0_REGNUM;
		else
		  {
		    record_buf_mem[mem_index++] = esize / 8;
		    record_buf_mem[mem_index++] = address + addr_offset;
		  }
		addr_offset = addr_offset + (esize / 8);
		reg_tt = (reg_tt + 1) % 32;
	      }
	  }
    }

  if (bit (aarch64_insn_r->aarch64_insn, 23))
    record_buf[reg_index++] = reg_rn;

  aarch64_insn_r->reg_rec_count = reg_index;
  aarch64_insn_r->mem_rec_count = mem_index / 2;
  MEM_ALLOC (aarch64_insn_r->aarch64_mems, aarch64_insn_r->mem_rec_count,
	     record_buf_mem);
  REG_ALLOC (aarch64_insn_r->aarch64_regs, aarch64_insn_r->reg_rec_count,
	     record_buf);
  return AARCH64_RECORD_SUCCESS;
}

/* Record handler for load and store instructions.  */

static unsigned int
aarch64_record_load_store (aarch64_insn_decode_record *aarch64_insn_r)
{
  uint8_t insn_bits24_27, insn_bits28_29, insn_bits10_11;
  uint8_t insn_bit23, insn_bit21;
  uint8_t opc, size_bits, ld_flag, vector_flag;
  uint32_t reg_rn, reg_rt, reg_rt2;
  uint64_t datasize, offset;
  uint32_t record_buf[8];
  uint64_t record_buf_mem[8];
  CORE_ADDR address;

  insn_bits10_11 = bits (aarch64_insn_r->aarch64_insn, 10, 11);
  insn_bits24_27 = bits (aarch64_insn_r->aarch64_insn, 24, 27);
  insn_bits28_29 = bits (aarch64_insn_r->aarch64_insn, 28, 29);
  insn_bit21 = bit (aarch64_insn_r->aarch64_insn, 21);
  insn_bit23 = bit (aarch64_insn_r->aarch64_insn, 23);
  ld_flag = bit (aarch64_insn_r->aarch64_insn, 22);
  vector_flag = bit (aarch64_insn_r->aarch64_insn, 26);
  reg_rt = bits (aarch64_insn_r->aarch64_insn, 0, 4);
  reg_rn = bits (aarch64_insn_r->aarch64_insn, 5, 9);
  reg_rt2 = bits (aarch64_insn_r->aarch64_insn, 10, 14);
  size_bits = bits (aarch64_insn_r->aarch64_insn, 30, 31);

  /* Load/store exclusive.  */
  if (insn_bits24_27 == 0x08 && insn_bits28_29 == 0x00)
    {
      if (record_debug)
	debug_printf ("Process record: load/store exclusive\n");

      if (ld_flag)
	{
	  record_buf[0] = reg_rt;
	  aarch64_insn_r->reg_rec_count = 1;
	  if (insn_bit21)
	    {
	      record_buf[1] = reg_rt2;
	      aarch64_insn_r->reg_rec_count = 2;
	    }
	}
      else
	{
	  if (insn_bit21)
	    datasize = (8 << size_bits) * 2;
	  else
	    datasize = (8 << size_bits);
	  regcache_raw_read_unsigned (aarch64_insn_r->regcache, reg_rn,
				      &address);
	  record_buf_mem[0] = datasize / 8;
	  record_buf_mem[1] = address;
	  aarch64_insn_r->mem_rec_count = 1;
	  if (!insn_bit23)
	    {
	      /* Save register rs.  */
	      record_buf[0] = bits (aarch64_insn_r->aarch64_insn, 16, 20);
	      aarch64_insn_r->reg_rec_count = 1;
	    }
	}
    }
  /* Load register (literal) instructions decoding.  */
  else if ((insn_bits24_27 & 0x0b) == 0x08 && insn_bits28_29 == 0x01)
    {
      if (record_debug)
	debug_printf ("Process record: load register (literal)\n");
      if (vector_flag)
	record_buf[0] = reg_rt + AARCH64_V0_REGNUM;
      else
	record_buf[0] = reg_rt;
      aarch64_insn_r->reg_rec_count = 1;
    }
  /* All types of load/store pair instructions decoding.  */
  else if ((insn_bits24_27 & 0x0a) == 0x08 && insn_bits28_29 == 0x02)
    {
      if (record_debug)
	debug_printf ("Process record: load/store pair\n");

      if (ld_flag)
	{
	  if (vector_flag)
	    {
	      record_buf[0] = reg_rt + AARCH64_V0_REGNUM;
	      record_buf[1] = reg_rt2 + AARCH64_V0_REGNUM;
	    }
	  else
	    {
	      record_buf[0] = reg_rt;
	      record_buf[1] = reg_rt2;
	    }
	  aarch64_insn_r->reg_rec_count = 2;
	}
      else
	{
	  uint16_t imm7_off;
	  imm7_off = bits (aarch64_insn_r->aarch64_insn, 15, 21);
	  if (!vector_flag)
	    size_bits = size_bits >> 1;
	  datasize = 8 << (2 + size_bits);
	  offset = (imm7_off & 0x40) ? (~imm7_off & 0x007f) + 1 : imm7_off;
	  offset = offset << (2 + size_bits);
	  regcache_raw_read_unsigned (aarch64_insn_r->regcache, reg_rn,
				      &address);
	  if (!((insn_bits24_27 & 0x0b) == 0x08 && insn_bit23))
	    {
	      if (imm7_off & 0x40)
		address = address - offset;
	      else
		address = address + offset;
	    }

	  record_buf_mem[0] = datasize / 8;
	  record_buf_mem[1] = address;
	  record_buf_mem[2] = datasize / 8;
	  record_buf_mem[3] = address + (datasize / 8);
	  aarch64_insn_r->mem_rec_count = 2;
	}
      if (bit (aarch64_insn_r->aarch64_insn, 23))
	record_buf[aarch64_insn_r->reg_rec_count++] = reg_rn;
    }
  /* Load/store register (unsigned immediate) instructions.  */
  else if ((insn_bits24_27 & 0x0b) == 0x09 && insn_bits28_29 == 0x03)
    {
      opc = bits (aarch64_insn_r->aarch64_insn, 22, 23);
      if (!(opc >> 1))
	{
	  if (opc & 0x01)
	    ld_flag = 0x01;
	  else
	    ld_flag = 0x0;
	}
      else
	{
	  if (size_bits == 0x3 && vector_flag == 0x0 && opc == 0x2)
	    {
	      /* PRFM (immediate) */
	      return AARCH64_RECORD_SUCCESS;
	    }
	  else if (size_bits == 0x2 && vector_flag == 0x0 && opc == 0x2)
	    {
	      /* LDRSW (immediate) */
	      ld_flag = 0x1;
	    }
	  else
	    {
	      if (opc & 0x01)
		ld_flag = 0x01;
	      else
		ld_flag = 0x0;
	    }
	}

      if (record_debug)
	{
	  debug_printf ("Process record: load/store (unsigned immediate):"
			" size %x V %d opc %x\n", size_bits, vector_flag,
			opc);
	}

      if (!ld_flag)
	{
	  offset = bits (aarch64_insn_r->aarch64_insn, 10, 21);
	  datasize = 8 << size_bits;
	  regcache_raw_read_unsigned (aarch64_insn_r->regcache, reg_rn,
				      &address);
	  offset = offset << size_bits;
	  address = address + offset;

	  record_buf_mem[0] = datasize >> 3;
	  record_buf_mem[1] = address;
	  aarch64_insn_r->mem_rec_count = 1;
	}
      else
	{
	  if (vector_flag)
	    record_buf[0] = reg_rt + AARCH64_V0_REGNUM;
	  else
	    record_buf[0] = reg_rt;
	  aarch64_insn_r->reg_rec_count = 1;
	}
    }
  /* Load/store register (register offset) instructions.  */
  else if ((insn_bits24_27 & 0x0b) == 0x08 && insn_bits28_29 == 0x03
	   && insn_bits10_11 == 0x02 && insn_bit21)
    {
      if (record_debug)
	debug_printf ("Process record: load/store (register offset)\n");
      opc = bits (aarch64_insn_r->aarch64_insn, 22, 23);
      if (!(opc >> 1))
	if (opc & 0x01)
	  ld_flag = 0x01;
	else
	  ld_flag = 0x0;
      else
	if (size_bits != 0x03)
	  ld_flag = 0x01;
	else
	  return AARCH64_RECORD_UNKNOWN;

      if (!ld_flag)
	{
	  ULONGEST reg_rm_val;

	  regcache_raw_read_unsigned (aarch64_insn_r->regcache,
		     bits (aarch64_insn_r->aarch64_insn, 16, 20), &reg_rm_val);
	  if (bit (aarch64_insn_r->aarch64_insn, 12))
	    offset = reg_rm_val << size_bits;
	  else
	    offset = reg_rm_val;
	  datasize = 8 << size_bits;
	  regcache_raw_read_unsigned (aarch64_insn_r->regcache, reg_rn,
				      &address);
	  address = address + offset;
	  record_buf_mem[0] = datasize >> 3;
	  record_buf_mem[1] = address;
	  aarch64_insn_r->mem_rec_count = 1;
	}
      else
	{
	  if (vector_flag)
	    record_buf[0] = reg_rt + AARCH64_V0_REGNUM;
	  else
	    record_buf[0] = reg_rt;
	  aarch64_insn_r->reg_rec_count = 1;
	}
    }
  /* Load/store register (immediate and unprivileged) instructions.  */
  else if ((insn_bits24_27 & 0x0b) == 0x08 && insn_bits28_29 == 0x03
	   && !insn_bit21)
    {
      if (record_debug)
	{
	  debug_printf ("Process record: load/store "
			"(immediate and unprivileged)\n");
	}
      opc = bits (aarch64_insn_r->aarch64_insn, 22, 23);
      if (!(opc >> 1))
	if (opc & 0x01)
	  ld_flag = 0x01;
	else
	  ld_flag = 0x0;
      else
	if (size_bits != 0x03)
	  ld_flag = 0x01;
	else
	  return AARCH64_RECORD_UNKNOWN;

      if (!ld_flag)
	{
	  uint16_t imm9_off;
	  imm9_off = bits (aarch64_insn_r->aarch64_insn, 12, 20);
	  offset = (imm9_off & 0x0100) ? (((~imm9_off) & 0x01ff) + 1) : imm9_off;
	  datasize = 8 << size_bits;
	  regcache_raw_read_unsigned (aarch64_insn_r->regcache, reg_rn,
				      &address);
	  if (insn_bits10_11 != 0x01)
	    {
	      if (imm9_off & 0x0100)
		address = address - offset;
	      else
		address = address + offset;
	    }
	  record_buf_mem[0] = datasize >> 3;
	  record_buf_mem[1] = address;
	  aarch64_insn_r->mem_rec_count = 1;
	}
      else
	{
	  if (vector_flag)
	    record_buf[0] = reg_rt + AARCH64_V0_REGNUM;
	  else
	    record_buf[0] = reg_rt;
	  aarch64_insn_r->reg_rec_count = 1;
	}
      if (insn_bits10_11 == 0x01 || insn_bits10_11 == 0x03)
	record_buf[aarch64_insn_r->reg_rec_count++] = reg_rn;
    }
  /* Advanced SIMD load/store instructions.  */
  else
    return aarch64_record_asimd_load_store (aarch64_insn_r);

  MEM_ALLOC (aarch64_insn_r->aarch64_mems, aarch64_insn_r->mem_rec_count,
	     record_buf_mem);
  REG_ALLOC (aarch64_insn_r->aarch64_regs, aarch64_insn_r->reg_rec_count,
	     record_buf);
  return AARCH64_RECORD_SUCCESS;
}

/* Record handler for data processing SIMD and floating point instructions.  */

static unsigned int
aarch64_record_data_proc_simd_fp (aarch64_insn_decode_record *aarch64_insn_r)
{
  uint8_t insn_bit21, opcode, rmode, reg_rd;
  uint8_t insn_bits24_27, insn_bits28_31, insn_bits10_11, insn_bits12_15;
  uint8_t insn_bits11_14;
  uint32_t record_buf[2];

  insn_bits24_27 = bits (aarch64_insn_r->aarch64_insn, 24, 27);
  insn_bits28_31 = bits (aarch64_insn_r->aarch64_insn, 28, 31);
  insn_bits10_11 = bits (aarch64_insn_r->aarch64_insn, 10, 11);
  insn_bits12_15 = bits (aarch64_insn_r->aarch64_insn, 12, 15);
  insn_bits11_14 = bits (aarch64_insn_r->aarch64_insn, 11, 14);
  opcode = bits (aarch64_insn_r->aarch64_insn, 16, 18);
  rmode = bits (aarch64_insn_r->aarch64_insn, 19, 20);
  reg_rd = bits (aarch64_insn_r->aarch64_insn, 0, 4);
  insn_bit21 = bit (aarch64_insn_r->aarch64_insn, 21);

  if (record_debug)
    debug_printf ("Process record: data processing SIMD/FP: ");

  if ((insn_bits28_31 & 0x05) == 0x01 && insn_bits24_27 == 0x0e)
    {
      /* Floating point - fixed point conversion instructions.  */
      if (!insn_bit21)
	{
	  if (record_debug)
	    debug_printf ("FP - fixed point conversion");

	  if ((opcode >> 1) == 0x0 && rmode == 0x03)
	    record_buf[0] = reg_rd;
	  else
	    record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
	}
      /* Floating point - conditional compare instructions.  */
      else if (insn_bits10_11 == 0x01)
	{
	  if (record_debug)
	    debug_printf ("FP - conditional compare");

	  record_buf[0] = AARCH64_CPSR_REGNUM;
	}
      /* Floating point - data processing (2-source) and
	 conditional select instructions.  */
      else if (insn_bits10_11 == 0x02 || insn_bits10_11 == 0x03)
	{
	  if (record_debug)
	    debug_printf ("FP - DP (2-source)");

	  record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
	}
      else if (insn_bits10_11 == 0x00)
	{
	  /* Floating point - immediate instructions.  */
	  if ((insn_bits12_15 & 0x01) == 0x01
	      || (insn_bits12_15 & 0x07) == 0x04)
	    {
	      if (record_debug)
		debug_printf ("FP - immediate");
	      record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
	    }
	  /* Floating point - compare instructions.  */
	  else if ((insn_bits12_15 & 0x03) == 0x02)
	    {
	      if (record_debug)
		debug_printf ("FP - immediate");
	      record_buf[0] = AARCH64_CPSR_REGNUM;
	    }
	  /* Floating point - integer conversions instructions.  */
	  else if (insn_bits12_15 == 0x00)
	    {
	      /* Convert float to integer instruction.  */
	      if (!(opcode >> 1) || ((opcode >> 1) == 0x02 && !rmode))
		{
		  if (record_debug)
		    debug_printf ("float to int conversion");

		  record_buf[0] = reg_rd + AARCH64_X0_REGNUM;
		}
	      /* Convert integer to float instruction.  */
	      else if ((opcode >> 1) == 0x01 && !rmode)
		{
		  if (record_debug)
		    debug_printf ("int to float conversion");

		  record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
		}
	      /* Move float to integer instruction.  */
	      else if ((opcode >> 1) == 0x03)
		{
		  if (record_debug)
		    debug_printf ("move float to int");

		  if (!(opcode & 0x01))
		    record_buf[0] = reg_rd + AARCH64_X0_REGNUM;
		  else
		    record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
		}
	      else
		return AARCH64_RECORD_UNKNOWN;
	    }
	  else
	    return AARCH64_RECORD_UNKNOWN;
	}
      else
	return AARCH64_RECORD_UNKNOWN;
    }
  else if ((insn_bits28_31 & 0x09) == 0x00 && insn_bits24_27 == 0x0e)
    {
      if (record_debug)
	debug_printf ("SIMD copy");

      /* Advanced SIMD copy instructions.  */
      if (!bits (aarch64_insn_r->aarch64_insn, 21, 23)
	  && !bit (aarch64_insn_r->aarch64_insn, 15)
	  && bit (aarch64_insn_r->aarch64_insn, 10))
	{
	  if (insn_bits11_14 == 0x05 || insn_bits11_14 == 0x07)
	    record_buf[0] = reg_rd + AARCH64_X0_REGNUM;
	  else
	    record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
	}
      else
	record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
    }
  /* All remaining floating point or advanced SIMD instructions.  */
  else
    {
      if (record_debug)
	debug_printf ("all remain");

      record_buf[0] = reg_rd + AARCH64_V0_REGNUM;
    }

  if (record_debug)
    debug_printf ("\n");

  /* Record the V/X register.  */
  aarch64_insn_r->reg_rec_count++;

  /* Some of these instructions may set bits in the FPSR, so record it
     too.  */
  record_buf[1] = AARCH64_FPSR_REGNUM;
  aarch64_insn_r->reg_rec_count++;

  gdb_assert (aarch64_insn_r->reg_rec_count == 2);
  REG_ALLOC (aarch64_insn_r->aarch64_regs, aarch64_insn_r->reg_rec_count,
	     record_buf);
  return AARCH64_RECORD_SUCCESS;
}

/* Decodes insns type and invokes its record handler.  */

static unsigned int
aarch64_record_decode_insn_handler (aarch64_insn_decode_record *aarch64_insn_r)
{
  uint32_t ins_bit25, ins_bit26, ins_bit27, ins_bit28;

  ins_bit25 = bit (aarch64_insn_r->aarch64_insn, 25);
  ins_bit26 = bit (aarch64_insn_r->aarch64_insn, 26);
  ins_bit27 = bit (aarch64_insn_r->aarch64_insn, 27);
  ins_bit28 = bit (aarch64_insn_r->aarch64_insn, 28);

  /* Data processing - immediate instructions.  */
  if (!ins_bit26 && !ins_bit27 && ins_bit28)
    return aarch64_record_data_proc_imm (aarch64_insn_r);

  /* Branch, exception generation and system instructions.  */
  if (ins_bit26 && !ins_bit27 && ins_bit28)
    return aarch64_record_branch_except_sys (aarch64_insn_r);

  /* Load and store instructions.  */
  if (!ins_bit25 && ins_bit27)
    return aarch64_record_load_store (aarch64_insn_r);

  /* Data processing - register instructions.  */
  if (ins_bit25 && !ins_bit26 && ins_bit27)
    return aarch64_record_data_proc_reg (aarch64_insn_r);

  /* Data processing - SIMD and floating point instructions.  */
  if (ins_bit25 && ins_bit26 && ins_bit27)
    return aarch64_record_data_proc_simd_fp (aarch64_insn_r);

  return AARCH64_RECORD_UNSUPPORTED;
}

/* Cleans up local record registers and memory allocations.  */

static void
deallocate_reg_mem (aarch64_insn_decode_record *record)
{
  xfree (record->aarch64_regs);
  xfree (record->aarch64_mems);
}

#if GDB_SELF_TEST
namespace selftests {

static void
aarch64_process_record_test (void)
{
  struct gdbarch_info info;
  uint32_t ret;

  info.bfd_arch_info = bfd_scan_arch ("aarch64");

  struct gdbarch *gdbarch = gdbarch_find_by_info (info);
  SELF_CHECK (gdbarch != NULL);

  aarch64_insn_decode_record aarch64_record;

  memset (&aarch64_record, 0, sizeof (aarch64_insn_decode_record));
  aarch64_record.regcache = NULL;
  aarch64_record.this_addr = 0;
  aarch64_record.gdbarch = gdbarch;

  /* 20 00 80 f9	prfm	pldl1keep, [x1] */
  aarch64_record.aarch64_insn = 0xf9800020;
  ret = aarch64_record_decode_insn_handler (&aarch64_record);
  SELF_CHECK (ret == AARCH64_RECORD_SUCCESS);
  SELF_CHECK (aarch64_record.reg_rec_count == 0);
  SELF_CHECK (aarch64_record.mem_rec_count == 0);

  deallocate_reg_mem (&aarch64_record);
}

} // namespace selftests
#endif /* GDB_SELF_TEST */

/* Parse the current instruction and record the values of the registers and
   memory that will be changed in current instruction to record_arch_list
   return -1 if something is wrong.  */

int
aarch64_process_record (struct gdbarch *gdbarch, struct regcache *regcache,
			CORE_ADDR insn_addr)
{
  uint32_t rec_no = 0;
  uint8_t insn_size = 4;
  uint32_t ret = 0;
  gdb_byte buf[insn_size];
  aarch64_insn_decode_record aarch64_record;

  memset (&buf[0], 0, insn_size);
  memset (&aarch64_record, 0, sizeof (aarch64_insn_decode_record));
  target_read_memory (insn_addr, &buf[0], insn_size);
  aarch64_record.aarch64_insn
    = (uint32_t) extract_unsigned_integer (&buf[0],
					   insn_size,
					   gdbarch_byte_order (gdbarch));
  aarch64_record.regcache = regcache;
  aarch64_record.this_addr = insn_addr;
  aarch64_record.gdbarch = gdbarch;

  ret = aarch64_record_decode_insn_handler (&aarch64_record);
  if (ret == AARCH64_RECORD_UNSUPPORTED)
    {
      gdb_printf (gdb_stderr,
		  _("Process record does not support instruction "
		    "0x%0x at address %s.\n"),
		  aarch64_record.aarch64_insn,
		  paddress (gdbarch, insn_addr));
      ret = -1;
    }

  if (0 == ret)
    {
      /* Record registers.  */
      record_full_arch_list_add_reg (aarch64_record.regcache,
				     AARCH64_PC_REGNUM);
      /* Always record register CPSR.  */
      record_full_arch_list_add_reg (aarch64_record.regcache,
				     AARCH64_CPSR_REGNUM);
      if (aarch64_record.aarch64_regs)
	for (rec_no = 0; rec_no < aarch64_record.reg_rec_count; rec_no++)
	  if (record_full_arch_list_add_reg (aarch64_record.regcache,
					     aarch64_record.aarch64_regs[rec_no]))
	    ret = -1;

      /* Record memories.  */
      if (aarch64_record.aarch64_mems)
	for (rec_no = 0; rec_no < aarch64_record.mem_rec_count; rec_no++)
	  if (record_full_arch_list_add_mem
	      ((CORE_ADDR)aarch64_record.aarch64_mems[rec_no].addr,
	       aarch64_record.aarch64_mems[rec_no].len))
	    ret = -1;

      if (record_full_arch_list_add_end ())
	ret = -1;
    }

  deallocate_reg_mem (&aarch64_record);
  return ret;
}
