/* Target dependent code for ARC architecture, for GDB.

   Copyright 2005-2024 Free Software Foundation, Inc.
   Contributed by Synopsys Inc.

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

/* GDB header files.  */
#include "defs.h"
#include "arch-utils.h"
#include "elf-bfd.h"
#include "disasm.h"
#include "dwarf2/frame.h"
#include "frame-base.h"
#include "frame-unwind.h"
#include "gdbcore.h"
#include "inferior.h"
#include "reggroups.h"
#include "gdbcmd.h"
#include "objfiles.h"
#include "osabi.h"
#include "prologue-value.h"
#include "target-descriptions.h"
#include "trad-frame.h"

/* ARC header files.  */
#include "opcode/arc.h"
#include "opcodes/arc-dis.h"
#include "arc-tdep.h"
#include "arch/arc.h"

/* Standard headers.  */
#include <algorithm>
#include <sstream>

/* The frame unwind cache for ARC.  */

struct arc_frame_cache
{
  /* The stack pointer at the time this frame was created; i.e. the caller's
     stack pointer when this function was called.  It is used to identify this
     frame.  */
  CORE_ADDR prev_sp;

  /* Register that is a base for this frame - FP for normal frame, SP for
     non-FP frames.  */
  int frame_base_reg;

  /* Offset from the previous SP to the current frame base.  If GCC uses
     `SUB SP,SP,offset` to allocate space for local variables, then it will be
     done after setting up a frame pointer, but it still will be considered
     part of prologue, therefore SP will be lesser than FP at the end of the
     prologue analysis.  In this case that would be an offset from old SP to a
     new FP.  But in case of non-FP frames, frame base is an SP and thus that
     would be an offset from old SP to new SP.  What is important is that this
     is an offset from old SP to a known register, so it can be used to find
     old SP.

     Using FP is preferable, when possible, because SP can change in function
     body after prologue due to alloca, variadic arguments or other shenanigans.
     If that is the case in the caller frame, then PREV_SP will point to SP at
     the moment of function call, but it will be different from SP value at the
     end of the caller prologue.  As a result it will not be possible to
     reconstruct caller's frame and go past it in the backtrace.  Those things
     are unlikely to happen to FP - FP value at the moment of function call (as
     stored on stack in callee prologue) is also an FP value at the end of the
     caller's prologue.  */

  LONGEST frame_base_offset;

  /* Store addresses for registers saved in prologue.  During prologue analysis
     GDB stores offsets relatively to "old SP", then after old SP is evaluated,
     offsets are replaced with absolute addresses.  */
  trad_frame_saved_reg *saved_regs;
};

/* Global debug flag.  */

bool arc_debug;

/* List of "maintenance print arc" commands.  */

static struct cmd_list_element *maintenance_print_arc_list = NULL;

/* A set of registers that we expect to find in a tdesc_feature.  These
   are used in ARC_TDESC_INIT when processing the target description.  */

struct arc_register_feature
{
  /* Information for a single register.  */
  struct register_info
  {
    /* The GDB register number for this register.  */
    int regnum;

    /* List of names for this register.  The first name in this list is the
       preferred name, the name GDB will use when describing this register.  */
    std::vector<const char *> names;

    /* When true, this register must be present in this feature set.  */
    bool required_p;
  };

  /* The name for this feature.  This is the name used to find this feature
     within the target description.  */
  const char *name;

  /* List of all the registers that we expect to encounter in this register
     set.  */
  std::vector<struct register_info> registers;
};

/* Obsolete feature names for backward compatibility.  */
static const char *ARC_CORE_V1_OBSOLETE_FEATURE_NAME
  = "org.gnu.gdb.arc.core.arcompact";
static const char *ARC_CORE_V2_OBSOLETE_FEATURE_NAME
  = "org.gnu.gdb.arc.core.v2";
static const char *ARC_CORE_V2_REDUCED_OBSOLETE_FEATURE_NAME
  = "org.gnu.gdb.arc.core-reduced.v2";
static const char *ARC_AUX_OBSOLETE_FEATURE_NAME
  = "org.gnu.gdb.arc.aux-minimal";
/* Modern feature names.  */
static const char *ARC_CORE_FEATURE_NAME = "org.gnu.gdb.arc.core";
static const char *ARC_AUX_FEATURE_NAME = "org.gnu.gdb.arc.aux";

/* ARCv1 (ARC600, ARC601, ARC700) general core registers feature set.
   See also arc_update_acc_reg_names() for "accl/acch" names.  */

static struct arc_register_feature arc_v1_core_reg_feature =
{
  ARC_CORE_FEATURE_NAME,
  {
    { ARC_R0_REGNUM + 0, { "r0" }, true },
    { ARC_R0_REGNUM + 1, { "r1" }, true },
    { ARC_R0_REGNUM + 2, { "r2" }, true },
    { ARC_R0_REGNUM + 3, { "r3" }, true },
    { ARC_R0_REGNUM + 4, { "r4" }, false },
    { ARC_R0_REGNUM + 5, { "r5" }, false },
    { ARC_R0_REGNUM + 6, { "r6" }, false },
    { ARC_R0_REGNUM + 7, { "r7" }, false },
    { ARC_R0_REGNUM + 8, { "r8" }, false },
    { ARC_R0_REGNUM + 9, { "r9" }, false },
    { ARC_R0_REGNUM + 10, { "r10" }, true },
    { ARC_R0_REGNUM + 11, { "r11" }, true },
    { ARC_R0_REGNUM + 12, { "r12" }, true },
    { ARC_R0_REGNUM + 13, { "r13" }, true },
    { ARC_R0_REGNUM + 14, { "r14" }, true },
    { ARC_R0_REGNUM + 15, { "r15" }, true },
    { ARC_R0_REGNUM + 16, { "r16" }, false },
    { ARC_R0_REGNUM + 17, { "r17" }, false },
    { ARC_R0_REGNUM + 18, { "r18" }, false },
    { ARC_R0_REGNUM + 19, { "r19" }, false },
    { ARC_R0_REGNUM + 20, { "r20" }, false },
    { ARC_R0_REGNUM + 21, { "r21" }, false },
    { ARC_R0_REGNUM + 22, { "r22" }, false },
    { ARC_R0_REGNUM + 23, { "r23" }, false },
    { ARC_R0_REGNUM + 24, { "r24" }, false },
    { ARC_R0_REGNUM + 25, { "r25" }, false },
    { ARC_R0_REGNUM + 26, { "gp" }, true },
    { ARC_R0_REGNUM + 27, { "fp" }, true },
    { ARC_R0_REGNUM + 28, { "sp" }, true },
    { ARC_R0_REGNUM + 29, { "ilink1" }, false },
    { ARC_R0_REGNUM + 30, { "ilink2" }, false },
    { ARC_R0_REGNUM + 31, { "blink" }, true },
    { ARC_R0_REGNUM + 32, { "r32" }, false },
    { ARC_R0_REGNUM + 33, { "r33" }, false },
    { ARC_R0_REGNUM + 34, { "r34" }, false },
    { ARC_R0_REGNUM + 35, { "r35" }, false },
    { ARC_R0_REGNUM + 36, { "r36" }, false },
    { ARC_R0_REGNUM + 37, { "r37" }, false },
    { ARC_R0_REGNUM + 38, { "r38" }, false },
    { ARC_R0_REGNUM + 39, { "r39" }, false },
    { ARC_R0_REGNUM + 40, { "r40" }, false },
    { ARC_R0_REGNUM + 41, { "r41" }, false },
    { ARC_R0_REGNUM + 42, { "r42" }, false },
    { ARC_R0_REGNUM + 43, { "r43" }, false },
    { ARC_R0_REGNUM + 44, { "r44" }, false },
    { ARC_R0_REGNUM + 45, { "r45" }, false },
    { ARC_R0_REGNUM + 46, { "r46" }, false },
    { ARC_R0_REGNUM + 47, { "r47" }, false },
    { ARC_R0_REGNUM + 48, { "r48" }, false },
    { ARC_R0_REGNUM + 49, { "r49" }, false },
    { ARC_R0_REGNUM + 50, { "r50" }, false },
    { ARC_R0_REGNUM + 51, { "r51" }, false },
    { ARC_R0_REGNUM + 52, { "r52" }, false },
    { ARC_R0_REGNUM + 53, { "r53" }, false },
    { ARC_R0_REGNUM + 54, { "r54" }, false },
    { ARC_R0_REGNUM + 55, { "r55" }, false },
    { ARC_R0_REGNUM + 56, { "r56" }, false },
    { ARC_R0_REGNUM + 57, { "r57" }, false },
    { ARC_R0_REGNUM + 58, { "r58", "accl" }, false },
    { ARC_R0_REGNUM + 59, { "r59", "acch" }, false },
    { ARC_R0_REGNUM + 60, { "lp_count" }, false },
    { ARC_R0_REGNUM + 61, { "reserved" }, false },
    { ARC_R0_REGNUM + 62, { "limm" }, false },
    { ARC_R0_REGNUM + 63, { "pcl" }, true }
  }
};

/* ARCv2 (ARCHS) general core registers feature set.  See also
   arc_update_acc_reg_names() for "accl/acch" names.  */

static struct arc_register_feature arc_v2_core_reg_feature =
{
  ARC_CORE_FEATURE_NAME,
  {
    { ARC_R0_REGNUM + 0, { "r0" }, true },
    { ARC_R0_REGNUM + 1, { "r1" }, true },
    { ARC_R0_REGNUM + 2, { "r2" }, true },
    { ARC_R0_REGNUM + 3, { "r3" }, true },
    { ARC_R0_REGNUM + 4, { "r4" }, false },
    { ARC_R0_REGNUM + 5, { "r5" }, false },
    { ARC_R0_REGNUM + 6, { "r6" }, false },
    { ARC_R0_REGNUM + 7, { "r7" }, false },
    { ARC_R0_REGNUM + 8, { "r8" }, false },
    { ARC_R0_REGNUM + 9, { "r9" }, false },
    { ARC_R0_REGNUM + 10, { "r10" }, true },
    { ARC_R0_REGNUM + 11, { "r11" }, true },
    { ARC_R0_REGNUM + 12, { "r12" }, true },
    { ARC_R0_REGNUM + 13, { "r13" }, true },
    { ARC_R0_REGNUM + 14, { "r14" }, true },
    { ARC_R0_REGNUM + 15, { "r15" }, true },
    { ARC_R0_REGNUM + 16, { "r16" }, false },
    { ARC_R0_REGNUM + 17, { "r17" }, false },
    { ARC_R0_REGNUM + 18, { "r18" }, false },
    { ARC_R0_REGNUM + 19, { "r19" }, false },
    { ARC_R0_REGNUM + 20, { "r20" }, false },
    { ARC_R0_REGNUM + 21, { "r21" }, false },
    { ARC_R0_REGNUM + 22, { "r22" }, false },
    { ARC_R0_REGNUM + 23, { "r23" }, false },
    { ARC_R0_REGNUM + 24, { "r24" }, false },
    { ARC_R0_REGNUM + 25, { "r25" }, false },
    { ARC_R0_REGNUM + 26, { "gp" }, true },
    { ARC_R0_REGNUM + 27, { "fp" }, true },
    { ARC_R0_REGNUM + 28, { "sp" }, true },
    { ARC_R0_REGNUM + 29, { "ilink" }, false },
    { ARC_R0_REGNUM + 30, { "r30" }, true },
    { ARC_R0_REGNUM + 31, { "blink" }, true },
    { ARC_R0_REGNUM + 32, { "r32" }, false },
    { ARC_R0_REGNUM + 33, { "r33" }, false },
    { ARC_R0_REGNUM + 34, { "r34" }, false },
    { ARC_R0_REGNUM + 35, { "r35" }, false },
    { ARC_R0_REGNUM + 36, { "r36" }, false },
    { ARC_R0_REGNUM + 37, { "r37" }, false },
    { ARC_R0_REGNUM + 38, { "r38" }, false },
    { ARC_R0_REGNUM + 39, { "r39" }, false },
    { ARC_R0_REGNUM + 40, { "r40" }, false },
    { ARC_R0_REGNUM + 41, { "r41" }, false },
    { ARC_R0_REGNUM + 42, { "r42" }, false },
    { ARC_R0_REGNUM + 43, { "r43" }, false },
    { ARC_R0_REGNUM + 44, { "r44" }, false },
    { ARC_R0_REGNUM + 45, { "r45" }, false },
    { ARC_R0_REGNUM + 46, { "r46" }, false },
    { ARC_R0_REGNUM + 47, { "r47" }, false },
    { ARC_R0_REGNUM + 48, { "r48" }, false },
    { ARC_R0_REGNUM + 49, { "r49" }, false },
    { ARC_R0_REGNUM + 50, { "r50" }, false },
    { ARC_R0_REGNUM + 51, { "r51" }, false },
    { ARC_R0_REGNUM + 52, { "r52" }, false },
    { ARC_R0_REGNUM + 53, { "r53" }, false },
    { ARC_R0_REGNUM + 54, { "r54" }, false },
    { ARC_R0_REGNUM + 55, { "r55" }, false },
    { ARC_R0_REGNUM + 56, { "r56" }, false },
    { ARC_R0_REGNUM + 57, { "r57" }, false },
    { ARC_R0_REGNUM + 58, { "r58", "accl" }, false },
    { ARC_R0_REGNUM + 59, { "r59", "acch" }, false },
    { ARC_R0_REGNUM + 60, { "lp_count" }, false },
    { ARC_R0_REGNUM + 61, { "reserved" }, false },
    { ARC_R0_REGNUM + 62, { "limm" }, false },
    { ARC_R0_REGNUM + 63, { "pcl" }, true }
  }
};

/* The common auxiliary registers feature set.  The REGNUM field
   must match the ARC_REGNUM enum in arc-tdep.h.  */

static const struct arc_register_feature arc_common_aux_reg_feature =
{
  ARC_AUX_FEATURE_NAME,
  {
    { ARC_FIRST_AUX_REGNUM + 0, { "pc" }, true },
    { ARC_FIRST_AUX_REGNUM + 1, { "status32" }, true },
    { ARC_FIRST_AUX_REGNUM + 2, { "lp_start" }, false },
    { ARC_FIRST_AUX_REGNUM + 3, { "lp_end" }, false },
    { ARC_FIRST_AUX_REGNUM + 4, { "bta" }, false }
  }
};

static char *arc_disassembler_options = NULL;

/* Functions are sorted in the order as they are used in the
   _initialize_arc_tdep (), which uses the same order as gdbarch.h.  Static
   functions are defined before the first invocation.  */

/* Returns an unsigned value of OPERAND_NUM in instruction INSN.
   For relative branch instructions returned value is an offset, not an actual
   branch target.  */

static ULONGEST
arc_insn_get_operand_value (const struct arc_instruction &insn,
			    unsigned int operand_num)
{
  switch (insn.operands[operand_num].kind)
    {
    case ARC_OPERAND_KIND_LIMM:
      gdb_assert (insn.limm_p);
      return insn.limm_value;
    case ARC_OPERAND_KIND_SHIMM:
      return insn.operands[operand_num].value;
    default:
      /* Value in instruction is a register number.  */
      regcache *regcache = get_thread_regcache (inferior_thread ());
      ULONGEST value;
      regcache_cooked_read_unsigned (regcache,
				     insn.operands[operand_num].value,
				     &value);
      return value;
    }
}

/* Like arc_insn_get_operand_value, but returns a signed value.  */

static LONGEST
arc_insn_get_operand_value_signed (const struct arc_instruction &insn,
				   unsigned int operand_num)
{
  switch (insn.operands[operand_num].kind)
    {
    case ARC_OPERAND_KIND_LIMM:
      gdb_assert (insn.limm_p);
      /* Convert unsigned raw value to signed one.  This assumes 2's
	 complement arithmetic, but so is the LONG_MIN value from generic
	 defs.h and that assumption is true for ARC.  */
      static_assert (sizeof (insn.limm_value) == sizeof (int));
      return (((LONGEST) insn.limm_value) ^ INT_MIN) - INT_MIN;
    case ARC_OPERAND_KIND_SHIMM:
      /* Sign conversion has been done by binutils.  */
      return insn.operands[operand_num].value;
    default:
      /* Value in instruction is a register number.  */
      regcache *regcache = get_thread_regcache (inferior_thread ());
      LONGEST value;
      regcache_cooked_read_signed (regcache,
				   insn.operands[operand_num].value,
				   &value);
      return value;
    }
}

/* Get register with base address of memory operation.  */

static int
arc_insn_get_memory_base_reg (const struct arc_instruction &insn)
{
  /* POP_S and PUSH_S have SP as an implicit argument in a disassembler.  */
  if (insn.insn_class == PUSH || insn.insn_class == POP)
    return ARC_SP_REGNUM;

  gdb_assert (insn.insn_class == LOAD || insn.insn_class == STORE);

  /* Other instructions all have at least two operands: operand 0 is data,
     operand 1 is address.  Operand 2 is offset from address.  However, see
     comment to arc_instruction.operands - in some cases, third operand may be
     missing, namely if it is 0.  */
  gdb_assert (insn.operands_count >= 2);
  return insn.operands[1].value;
}

/* Get offset of a memory operation INSN.  */

static CORE_ADDR
arc_insn_get_memory_offset (const struct arc_instruction &insn)
{
  /* POP_S and PUSH_S have offset as an implicit argument in a
     disassembler.  */
  if (insn.insn_class == POP)
    return 4;
  else if (insn.insn_class == PUSH)
    return -4;

  gdb_assert (insn.insn_class == LOAD || insn.insn_class == STORE);

  /* Other instructions all have at least two operands: operand 0 is data,
     operand 1 is address.  Operand 2 is offset from address.  However, see
     comment to arc_instruction.operands - in some cases, third operand may be
     missing, namely if it is 0.  */
  if (insn.operands_count < 3)
    return 0;

  CORE_ADDR value = arc_insn_get_operand_value (insn, 2);
  /* Handle scaling.  */
  if (insn.writeback_mode == ARC_WRITEBACK_AS)
    {
      /* Byte data size is not valid for AS.  Halfword means shift by 1 bit.
	 Word and double word means shift by 2 bits.  */
      gdb_assert (insn.data_size_mode != ARC_SCALING_B);
      if (insn.data_size_mode == ARC_SCALING_H)
	value <<= 1;
      else
	value <<= 2;
    }
  return value;
}

CORE_ADDR
arc_insn_get_branch_target (const struct arc_instruction &insn)
{
  gdb_assert (insn.is_control_flow);

  /* BI [c]: PC = nextPC + (c << 2).  */
  if (insn.insn_class == BI)
    {
      ULONGEST reg_value = arc_insn_get_operand_value (insn, 0);
      return arc_insn_get_linear_next_pc (insn) + (reg_value << 2);
    }
  /* BIH [c]: PC = nextPC + (c << 1).  */
  else if (insn.insn_class == BIH)
    {
      ULONGEST reg_value = arc_insn_get_operand_value (insn, 0);
      return arc_insn_get_linear_next_pc (insn) + (reg_value << 1);
    }
  /* JLI and EI.  */
  /* JLI and EI depend on optional AUX registers.  Not supported right now.  */
  else if (insn.insn_class == JLI)
    {
      gdb_printf (gdb_stderr,
		  "JLI_S instruction is not supported by the GDB.");
      return 0;
    }
  else if (insn.insn_class == EI)
    {
      gdb_printf (gdb_stderr,
		  "EI_S instruction is not supported by the GDB.");
      return 0;
    }
  /* LEAVE_S: PC = BLINK.  */
  else if (insn.insn_class == LEAVE)
    {
      regcache *regcache = get_thread_regcache (inferior_thread ());
      ULONGEST value;
      regcache_cooked_read_unsigned (regcache, ARC_BLINK_REGNUM, &value);
      return value;
    }
  /* BBIT0/1, BRcc: PC = currentPC + operand.  */
  else if (insn.insn_class == BBIT0 || insn.insn_class == BBIT1
	   || insn.insn_class == BRCC)
    {
      /* Most instructions has branch target as their sole argument.  However
	 conditional brcc/bbit has it as a third operand.  */
      CORE_ADDR pcrel_addr = arc_insn_get_operand_value (insn, 2);

      /* Offset is relative to the 4-byte aligned address of the current
	 instruction, hence last two bits should be truncated.  */
      return pcrel_addr + align_down (insn.address, 4);
    }
  /* B, Bcc, BL, BLcc, LP, LPcc: PC = currentPC + operand.  */
  else if (insn.insn_class == BRANCH || insn.insn_class == LOOP)
    {
      CORE_ADDR pcrel_addr = arc_insn_get_operand_value (insn, 0);

      /* Offset is relative to the 4-byte aligned address of the current
	 instruction, hence last two bits should be truncated.  */
      return pcrel_addr + align_down (insn.address, 4);
    }
  /* J, Jcc, JL, JLcc: PC = operand.  */
  else if (insn.insn_class == JUMP)
    {
      /* All jumps are single-operand.  */
      return arc_insn_get_operand_value (insn, 0);
    }

  /* This is some new and unknown instruction.  */
  gdb_assert_not_reached ("Unknown branch instruction.");
}

/* Dump INSN into gdb_stdlog.  */

static void
arc_insn_dump (const struct arc_instruction &insn)
{
  struct gdbarch *gdbarch = current_inferior ()->arch ();

  arc_print ("Dumping arc_instruction at %s\n",
	     paddress (gdbarch, insn.address));
  arc_print ("\tlength = %u\n", insn.length);

  if (!insn.valid)
    {
      arc_print ("\tThis is not a valid ARC instruction.\n");
      return;
    }

  arc_print ("\tlength_with_limm = %u\n", insn.length + (insn.limm_p ? 4 : 0));
  arc_print ("\tcc = 0x%x\n", insn.condition_code);
  arc_print ("\tinsn_class = %u\n", insn.insn_class);
  arc_print ("\tis_control_flow = %i\n", insn.is_control_flow);
  arc_print ("\thas_delay_slot = %i\n", insn.has_delay_slot);

  CORE_ADDR next_pc = arc_insn_get_linear_next_pc (insn);
  arc_print ("\tlinear_next_pc = %s\n", paddress (gdbarch, next_pc));

  if (insn.is_control_flow)
    {
      CORE_ADDR t = arc_insn_get_branch_target (insn);
      arc_print ("\tbranch_target = %s\n", paddress (gdbarch, t));
    }

  arc_print ("\tlimm_p = %i\n", insn.limm_p);
  if (insn.limm_p)
    arc_print ("\tlimm_value = 0x%08x\n", insn.limm_value);

  if (insn.insn_class == STORE || insn.insn_class == LOAD
      || insn.insn_class == PUSH || insn.insn_class == POP)
    {
      arc_print ("\twriteback_mode = %u\n", insn.writeback_mode);
      arc_print ("\tdata_size_mode = %u\n", insn.data_size_mode);
      arc_print ("\tmemory_base_register = %s\n",
		 gdbarch_register_name (gdbarch,
					arc_insn_get_memory_base_reg (insn)));
      /* get_memory_offset returns an unsigned CORE_ADDR, but treat it as a
	 LONGEST for a nicer representation.  */
      arc_print ("\taddr_offset = %s\n",
		 plongest (arc_insn_get_memory_offset (insn)));
    }

  arc_print ("\toperands_count = %u\n", insn.operands_count);
  for (unsigned int i = 0; i < insn.operands_count; ++i)
    {
      int is_reg = (insn.operands[i].kind == ARC_OPERAND_KIND_REG);

      arc_print ("\toperand[%u] = {\n", i);
      arc_print ("\t\tis_reg = %i\n", is_reg);
      if (is_reg)
	arc_print ("\t\tregister = %s\n",
		   gdbarch_register_name (gdbarch, insn.operands[i].value));
      /* Don't know if this value is signed or not, so print both
	 representations.  This tends to look quite ugly, especially for big
	 numbers.  */
      arc_print ("\t\tunsigned value = %s\n",
		 pulongest (arc_insn_get_operand_value (insn, i)));
      arc_print ("\t\tsigned value = %s\n",
		 plongest (arc_insn_get_operand_value_signed (insn, i)));
      arc_print ("\t}\n");
    }
}

CORE_ADDR
arc_insn_get_linear_next_pc (const struct arc_instruction &insn)
{
  /* In ARC long immediate is always 4 bytes.  */
  return (insn.address + insn.length + (insn.limm_p ? 4 : 0));
}

/* Implement the "write_pc" gdbarch method.

   In ARC PC register is a normal register so in most cases setting PC value
   is a straightforward process: debugger just writes PC value.  However it
   gets trickier in case when current instruction is an instruction in delay
   slot.  In this case CPU will execute instruction at current PC value, then
   will set PC to the current value of BTA register; also current instruction
   cannot be branch/jump and some of the other instruction types.  Thus if
   debugger would try to just change PC value in this case, this instruction
   will get executed, but then core will "jump" to the original branch target.

   Whether current instruction is a delay-slot instruction or not is indicated
   by DE bit in STATUS32 register indicates if current instruction is a delay
   slot instruction.  This bit is writable by debug host, which allows debug
   host to prevent core from jumping after the delay slot instruction.  It
   also works in another direction: setting this bit will make core to treat
   any current instructions as a delay slot instruction and to set PC to the
   current value of BTA register.

   To workaround issues with changing PC register while in delay slot
   instruction, debugger should check for the STATUS32.DE bit and reset it if
   it is set.  No other change is required in this function.  Most common
   case, where this function might be required is calling inferior functions
   from debugger.  Generic GDB logic handles this pretty well: current values
   of registers are stored, value of PC is changed (that is the job of this
   function), and after inferior function is executed, GDB restores all
   registers, include BTA and STATUS32, which also means that core is returned
   to its original state of being halted on delay slot instructions.

   This method is useless for ARC 600, because it doesn't have externally
   exposed BTA register.  In the case of ARC 600 it is impossible to restore
   core to its state in all occasions thus core should never be halted (from
   the perspective of debugger host) in the delay slot.  */

static void
arc_write_pc (struct regcache *regcache, CORE_ADDR new_pc)
{
  struct gdbarch *gdbarch = regcache->arch ();

  arc_debug_printf ("Writing PC, new value=%s",
		    paddress (gdbarch, new_pc));

  regcache_cooked_write_unsigned (regcache, gdbarch_pc_regnum (gdbarch),
				  new_pc);

  ULONGEST status32;
  regcache_cooked_read_unsigned (regcache, gdbarch_ps_regnum (gdbarch),
				 &status32);

  if ((status32 & ARC_STATUS32_DE_MASK) != 0)
    {
      arc_debug_printf ("Changing PC while in delay slot.  Will "
			"reset STATUS32.DE bit to zero.  Value of STATUS32 "
			"register is 0x%s",
			phex (status32, ARC_REGISTER_SIZE));

      /* Reset bit and write to the cache.  */
      status32 &= ~0x40;
      regcache_cooked_write_unsigned (regcache, gdbarch_ps_regnum (gdbarch),
				      status32);
    }
}

/* Implement the "virtual_frame_pointer" gdbarch method.

   According to ABI the FP (r27) is used to point to the middle of the current
   stack frame, just below the saved FP and before local variables, register
   spill area and outgoing args.  However for optimization levels above O2 and
   in any case in leaf functions, the frame pointer is usually not set at all.
   The exception being when handling nested functions.

   We use this function to return a "virtual" frame pointer, marking the start
   of the current stack frame as a register-offset pair.  If the FP is not
   being used, then it should return SP, with an offset of the frame size.

   The current implementation doesn't actually know the frame size, nor
   whether the FP is actually being used, so for now we just return SP and an
   offset of zero.  This is no worse than other architectures, but is needed
   to avoid assertion failures.

   TODO: Can we determine the frame size to get a correct offset?

   PC is a program counter where we need the virtual FP.  REG_PTR is the base
   register used for the virtual FP.  OFFSET_PTR is the offset used for the
   virtual FP.  */

static void
arc_virtual_frame_pointer (struct gdbarch *gdbarch, CORE_ADDR pc,
			   int *reg_ptr, LONGEST *offset_ptr)
{
  *reg_ptr = gdbarch_sp_regnum (gdbarch);
  *offset_ptr = 0;
}

/* Implement the "push_dummy_call" gdbarch method.

   Stack Frame Layout

   This shows the layout of the stack frame for the general case of a
   function call; a given function might not have a variable number of
   arguments or local variables, or might not save any registers, so it would
   not have the corresponding frame areas.  Additionally, a leaf function
   (i.e. one which calls no other functions) does not need to save the
   contents of the BLINK register (which holds its return address), and a
   function might not have a frame pointer.

   The stack grows downward, so SP points below FP in memory; SP always
   points to the last used word on the stack, not the first one.

		      |                       |   |
		      |      arg word N       |   | caller's
		      |           :           |   | frame
		      |      arg word 10      |   |
		      |      arg word 9       |   |
	  old SP ---> +-----------------------+ --+
		      |                       |   |
		      |      callee-saved     |   |
		      |       registers       |   |
		      |  including fp, blink  |   |
		      |                       |   | callee's
	  new FP ---> +-----------------------+   | frame
		      |                       |   |
		      |         local         |   |
		      |       variables       |   |
		      |                       |   |
		      |       register        |   |
		      |      spill area       |   |
		      |                       |   |
		      |     outgoing args     |   |
		      |                       |   |
	  new SP ---> +-----------------------+ --+
		      |                       |
		      |         unused        |
		      |                       |
				  |
				  |
				  V
			      downwards

   The list of arguments to be passed to a function is considered to be a
   sequence of _N_ words (as though all the parameters were stored in order in
   memory with each parameter occupying an integral number of words).  Words
   1..8 are passed in registers 0..7; if the function has more than 8 words of
   arguments then words 9..@em N are passed on the stack in the caller's frame.

   If the function has a variable number of arguments, e.g. it has a form such
   as `function (p1, p2, ...);' and _P_ words are required to hold the values
   of the named parameters (which are passed in registers 0..@em P -1), then
   the remaining 8 - _P_ words passed in registers _P_..7 are spilled into the
   top of the frame so that the anonymous parameter words occupy a continuous
   region.

   Any arguments are already in target byte order.  We just need to store
   them!

   BP_ADDR is the return address where breakpoint must be placed.  NARGS is
   the number of arguments to the function.  ARGS is the arguments values (in
   target byte order).  SP is the Current value of SP register.  STRUCT_RETURN
   is TRUE if structures are returned by the function.  STRUCT_ADDR is the
   hidden address for returning a struct.  Returns SP of a new frame.  */

static CORE_ADDR
arc_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		     struct regcache *regcache, CORE_ADDR bp_addr, int nargs,
		     struct value **args, CORE_ADDR sp,
		     function_call_return_method return_method,
		     CORE_ADDR struct_addr)
{
  arc_debug_printf ("nargs = %d", nargs);

  int arg_reg = ARC_FIRST_ARG_REGNUM;

  /* Push the return address.  */
  regcache_cooked_write_unsigned (regcache, ARC_BLINK_REGNUM, bp_addr);

  /* Are we returning a value using a structure return instead of a normal
     value return?  If so, struct_addr is the address of the reserved space for
     the return structure to be written on the stack, and that address is
     passed to that function as a hidden first argument.  */
  if (return_method == return_method_struct)
    {
      /* Pass the return address in the first argument register.  */
      regcache_cooked_write_unsigned (regcache, arg_reg, struct_addr);

      arc_debug_printf ("struct return address %s passed in R%d",
			print_core_address (gdbarch, struct_addr), arg_reg);

      arg_reg++;
    }

  if (nargs > 0)
    {
      unsigned int total_space = 0;

      /* How much space do the arguments occupy in total?  Must round each
	 argument's size up to an integral number of words.  */
      for (int i = 0; i < nargs; i++)
	{
	  unsigned int len = args[i]->type ()->length ();
	  unsigned int space = align_up (len, 4);

	  total_space += space;

	  arc_debug_printf ("arg %d: %u bytes -> %u", i, len, space);
	}

      /* Allocate a buffer to hold a memory image of the arguments.  */
      gdb_byte *memory_image = XCNEWVEC (gdb_byte, total_space);

      /* Now copy all of the arguments into the buffer, correctly aligned.  */
      gdb_byte *data = memory_image;
      for (int i = 0; i < nargs; i++)
	{
	  unsigned int len = args[i]->type ()->length ();
	  unsigned int space = align_up (len, 4);

	  memcpy (data, args[i]->contents ().data (), (size_t) len);
	  arc_debug_printf ("copying arg %d, val 0x%08x, len %d to mem",
			    i, *((int *) args[i]->contents ().data ()),
			    len);

	  data += space;
	}

      /* Now load as much as possible of the memory image into registers.  */
      data = memory_image;
      while (arg_reg <= ARC_LAST_ARG_REGNUM)
	{
	  arc_debug_printf ("passing 0x%02x%02x%02x%02x in register R%d",
			    data[0], data[1], data[2], data[3], arg_reg);

	  /* Note we don't use write_unsigned here, since that would convert
	     the byte order, but we are already in the correct byte order.  */
	  regcache->cooked_write (arg_reg, data);

	  data += ARC_REGISTER_SIZE;
	  total_space -= ARC_REGISTER_SIZE;

	  /* All the data is now in registers.  */
	  if (total_space == 0)
	    break;

	  arg_reg++;
	}

      /* If there is any data left, push it onto the stack (in a single write
	 operation).  */
      if (total_space > 0)
	{
	  arc_debug_printf ("passing %d bytes on stack\n", total_space);

	  sp -= total_space;
	  write_memory (sp, data, (int) total_space);
	}

      xfree (memory_image);
    }

  /* Finally, update the SP register.  */
  regcache_cooked_write_unsigned (regcache, gdbarch_sp_regnum (gdbarch), sp);

  return sp;
}

/* Implement the "push_dummy_code" gdbarch method.

   We don't actually push any code.  We just identify where a breakpoint can
   be inserted to which we are can return and the resume address where we
   should be called.

   ARC does not necessarily have an executable stack, so we can't put the
   return breakpoint there.  Instead we put it at the entry point of the
   function.  This means the SP is unchanged.

   SP is a current stack pointer FUNADDR is an address of the function to be
   called.  ARGS is arguments to pass.  NARGS is a number of args to pass.
   VALUE_TYPE is a type of value returned.  REAL_PC is a resume address when
   the function is called.  BP_ADDR is an address where breakpoint should be
   set.  Returns the updated stack pointer.  */

static CORE_ADDR
arc_push_dummy_code (struct gdbarch *gdbarch, CORE_ADDR sp, CORE_ADDR funaddr,
		     struct value **args, int nargs, struct type *value_type,
		     CORE_ADDR *real_pc, CORE_ADDR *bp_addr,
		     struct regcache *regcache)
{
  *real_pc = funaddr;
  *bp_addr = entry_point_address ();
  return sp;
}

/* Implement the "cannot_fetch_register" gdbarch method.  */

static int
arc_cannot_fetch_register (struct gdbarch *gdbarch, int regnum)
{
  /* Assume that register is readable if it is unknown.  LIMM and RESERVED are
     not real registers, but specific register numbers.  They are available as
     regnums to align architectural register numbers with GDB internal regnums,
     but they shouldn't appear in target descriptions generated by
     GDB-servers.  */
  switch (regnum)
    {
    case ARC_RESERVED_REGNUM:
    case ARC_LIMM_REGNUM:
      return true;
    default:
      return false;
    }
}

/* Implement the "cannot_store_register" gdbarch method.  */

static int
arc_cannot_store_register (struct gdbarch *gdbarch, int regnum)
{
  /* Assume that register is writable if it is unknown.  See comment in
     arc_cannot_fetch_register about LIMM and RESERVED.  */
  switch (regnum)
    {
    case ARC_RESERVED_REGNUM:
    case ARC_LIMM_REGNUM:
    case ARC_PCL_REGNUM:
      return true;
    default:
      return false;
    }
}

/* Get the return value of a function from the registers/memory used to
   return it, according to the convention used by the ABI - 4-bytes values are
   in the R0, while 8-byte values are in the R0-R1.

   TODO: This implementation ignores the case of "complex double", where
   according to ABI, value is returned in the R0-R3 registers.

   TYPE is a returned value's type.  VALBUF is a buffer for the returned
   value.  */

static void
arc_extract_return_value (struct gdbarch *gdbarch, struct type *type,
			  struct regcache *regcache, gdb_byte *valbuf)
{
  unsigned int len = type->length ();

  arc_debug_printf ("called");

  if (len <= ARC_REGISTER_SIZE)
    {
      ULONGEST val;

      /* Get the return value from one register.  */
      regcache_cooked_read_unsigned (regcache, ARC_R0_REGNUM, &val);
      store_unsigned_integer (valbuf, (int) len,
			      gdbarch_byte_order (gdbarch), val);

      arc_debug_printf ("returning 0x%s", phex (val, ARC_REGISTER_SIZE));
    }
  else if (len <= ARC_REGISTER_SIZE * 2)
    {
      ULONGEST low, high;

      /* Get the return value from two registers.  */
      regcache_cooked_read_unsigned (regcache, ARC_R0_REGNUM, &low);
      regcache_cooked_read_unsigned (regcache, ARC_R1_REGNUM, &high);

      store_unsigned_integer (valbuf, ARC_REGISTER_SIZE,
			      gdbarch_byte_order (gdbarch), low);
      store_unsigned_integer (valbuf + ARC_REGISTER_SIZE,
			      (int) len - ARC_REGISTER_SIZE,
			      gdbarch_byte_order (gdbarch), high);

      arc_debug_printf ("returning 0x%s%s",
			phex (high, ARC_REGISTER_SIZE),
			phex (low, ARC_REGISTER_SIZE));
    }
  else
    error (_("arc: extract_return_value: type length %u too large"), len);
}


/* Store the return value of a function into the registers/memory used to
   return it, according to the convention used by the ABI.

   TODO: This implementation ignores the case of "complex double", where
   according to ABI, value is returned in the R0-R3 registers.

   TYPE is a returned value's type.  VALBUF is a buffer with the value to
   return.  */

static void
arc_store_return_value (struct gdbarch *gdbarch, struct type *type,
			struct regcache *regcache, const gdb_byte *valbuf)
{
  unsigned int len = type->length ();

  arc_debug_printf ("called");

  if (len <= ARC_REGISTER_SIZE)
    {
      ULONGEST val;

      /* Put the return value into one register.  */
      val = extract_unsigned_integer (valbuf, (int) len,
				      gdbarch_byte_order (gdbarch));
      regcache_cooked_write_unsigned (regcache, ARC_R0_REGNUM, val);

      arc_debug_printf ("storing 0x%s", phex (val, ARC_REGISTER_SIZE));
    }
  else if (len <= ARC_REGISTER_SIZE * 2)
    {
      ULONGEST low, high;

      /* Put the return value into  two registers.  */
      low = extract_unsigned_integer (valbuf, ARC_REGISTER_SIZE,
				      gdbarch_byte_order (gdbarch));
      high = extract_unsigned_integer (valbuf + ARC_REGISTER_SIZE,
				       (int) len - ARC_REGISTER_SIZE,
				       gdbarch_byte_order (gdbarch));

      regcache_cooked_write_unsigned (regcache, ARC_R0_REGNUM, low);
      regcache_cooked_write_unsigned (regcache, ARC_R1_REGNUM, high);

      arc_debug_printf ("storing 0x%s%s",
			phex (high, ARC_REGISTER_SIZE),
			phex (low, ARC_REGISTER_SIZE));
    }
  else
    error (_("arc_store_return_value: type length too large."));
}

/* Implement the "get_longjmp_target" gdbarch method.  */

static int
arc_get_longjmp_target (frame_info_ptr frame, CORE_ADDR *pc)
{
  arc_debug_printf ("called");

  struct gdbarch *gdbarch = get_frame_arch (frame);
  arc_gdbarch_tdep *tdep = gdbarch_tdep<arc_gdbarch_tdep> (gdbarch);
  int pc_offset = tdep->jb_pc * ARC_REGISTER_SIZE;
  gdb_byte buf[ARC_REGISTER_SIZE];
  CORE_ADDR jb_addr = get_frame_register_unsigned (frame, ARC_FIRST_ARG_REGNUM);

  if (target_read_memory (jb_addr + pc_offset, buf, ARC_REGISTER_SIZE))
    return 0; /* Failed to read from memory.  */

  *pc = extract_unsigned_integer (buf, ARC_REGISTER_SIZE,
				  gdbarch_byte_order (gdbarch));
  return 1;
}

/* Implement the "return_value" gdbarch method.  */

static enum return_value_convention
arc_return_value (struct gdbarch *gdbarch, struct value *function,
		  struct type *valtype, struct regcache *regcache,
		  gdb_byte *readbuf, const gdb_byte *writebuf)
{
  /* If the return type is a struct, or a union, or would occupy more than two
     registers, the ABI uses the "struct return convention": the calling
     function passes a hidden first parameter to the callee (in R0).  That
     parameter is the address at which the value being returned should be
     stored.  Otherwise, the result is returned in registers.  */
  int is_struct_return = (valtype->code () == TYPE_CODE_STRUCT
			  || valtype->code () == TYPE_CODE_UNION
			  || valtype->length () > 2 * ARC_REGISTER_SIZE);

  arc_debug_printf ("readbuf = %s, writebuf = %s",
		    host_address_to_string (readbuf),
		    host_address_to_string (writebuf));

  if (writebuf != NULL)
    {
      /* Case 1.  GDB should not ask us to set a struct return value: it
	 should know the struct return location and write the value there
	 itself.  */
      gdb_assert (!is_struct_return);
      arc_store_return_value (gdbarch, valtype, regcache, writebuf);
    }
  else if (readbuf != NULL)
    {
      /* Case 2.  GDB should not ask us to get a struct return value: it
	 should know the struct return location and read the value from there
	 itself.  */
      gdb_assert (!is_struct_return);
      arc_extract_return_value (gdbarch, valtype, regcache, readbuf);
    }

  return (is_struct_return
	  ? RETURN_VALUE_STRUCT_CONVENTION
	  : RETURN_VALUE_REGISTER_CONVENTION);
}

/* Return the base address of the frame.  For ARC, the base address is the
   frame pointer.  */

static CORE_ADDR
arc_frame_base_address (frame_info_ptr this_frame, void **prologue_cache)
{
  return (CORE_ADDR) get_frame_register_unsigned (this_frame, ARC_FP_REGNUM);
}

/* Helper function that returns valid pv_t for an instruction operand:
   either a register or a constant.  */

static pv_t
arc_pv_get_operand (pv_t *regs, const struct arc_instruction &insn, int operand)
{
  if (insn.operands[operand].kind == ARC_OPERAND_KIND_REG)
    return regs[insn.operands[operand].value];
  else
    return pv_constant (arc_insn_get_operand_value (insn, operand));
}

/* Determine whether the given disassembled instruction may be part of a
   function prologue.  If it is, the information in the frame unwind cache will
   be updated.  */

static bool
arc_is_in_prologue (struct gdbarch *gdbarch, const struct arc_instruction &insn,
		    pv_t *regs, struct pv_area *stack)
{
  /* It might be that currently analyzed address doesn't contain an
     instruction, hence INSN is not valid.  It likely means that address points
     to a data, non-initialized memory, or middle of a 32-bit instruction.  In
     practice this may happen if GDB connects to a remote target that has
     non-zeroed memory.  GDB would read PC value and would try to analyze
     prologue, but there is no guarantee that memory contents at the address
     specified in PC is address is a valid instruction.  There is not much that
     that can be done about that.  */
  if (!insn.valid)
    return false;

  /* Branch/jump or a predicated instruction.  */
  if (insn.is_control_flow || insn.condition_code != ARC_CC_AL)
    return false;

  /* Store of some register.  May or may not update base address register.  */
  if (insn.insn_class == STORE || insn.insn_class == PUSH)
    {
      /* There is definitely at least one operand - register/value being
	 stored.  */
      gdb_assert (insn.operands_count > 0);

      /* Store at some constant address.  */
      if (insn.operands_count > 1
	  && insn.operands[1].kind != ARC_OPERAND_KIND_REG)
	return false;

      /* Writeback modes:
	 Mode	Address used		    Writeback value
	 --------------------------------------------------
	 No	reg + offset		    no
	 A/AW	reg + offset		    reg + offset
	 AB	reg			    reg + offset
	 AS	reg + (offset << scaling)   no

	 "PUSH reg" is an alias to "ST.AW reg, [SP, -4]" encoding.  However
	 16-bit PUSH_S is a distinct instruction encoding, where offset and
	 base register are implied through opcode.  */

      /* Register with base memory address.  */
      int base_reg = arc_insn_get_memory_base_reg (insn);

      /* Address where to write.  arc_insn_get_memory_offset returns scaled
	 value for ARC_WRITEBACK_AS.  */
      pv_t addr;
      if (insn.writeback_mode == ARC_WRITEBACK_AB)
	addr = regs[base_reg];
      else
	addr = pv_add_constant (regs[base_reg],
				arc_insn_get_memory_offset (insn));

      if (stack->store_would_trash (addr))
	return false;

      if (insn.data_size_mode != ARC_SCALING_D)
	{
	  /* Find the value being stored.  */
	  pv_t store_value = arc_pv_get_operand (regs, insn, 0);

	  /* What is the size of a the stored value?  */
	  CORE_ADDR size;
	  if (insn.data_size_mode == ARC_SCALING_B)
	    size = 1;
	  else if (insn.data_size_mode == ARC_SCALING_H)
	    size = 2;
	  else
	    size = ARC_REGISTER_SIZE;

	  stack->store (addr, size, store_value);
	}
      else
	{
	  if (insn.operands[0].kind == ARC_OPERAND_KIND_REG)
	    {
	      /* If this is a double store, than write N+1 register as well.  */
	      pv_t store_value1 = regs[insn.operands[0].value];
	      pv_t store_value2 = regs[insn.operands[0].value + 1];
	      stack->store (addr, ARC_REGISTER_SIZE, store_value1);
	      stack->store (pv_add_constant (addr, ARC_REGISTER_SIZE),
			    ARC_REGISTER_SIZE, store_value2);
	    }
	  else
	    {
	      pv_t store_value
		= pv_constant (arc_insn_get_operand_value (insn, 0));
	      stack->store (addr, ARC_REGISTER_SIZE * 2, store_value);
	    }
	}

      /* Is base register updated?  */
      if (insn.writeback_mode == ARC_WRITEBACK_A
	  || insn.writeback_mode == ARC_WRITEBACK_AB)
	regs[base_reg] = pv_add_constant (regs[base_reg],
					  arc_insn_get_memory_offset (insn));

      return true;
    }
  else if (insn.insn_class == MOVE)
    {
      gdb_assert (insn.operands_count == 2);

      /* Destination argument can be "0", so nothing will happen.  */
      if (insn.operands[0].kind == ARC_OPERAND_KIND_REG)
	{
	  int dst_regnum = insn.operands[0].value;
	  regs[dst_regnum] = arc_pv_get_operand (regs, insn, 1);
	}
      return true;
    }
  else if (insn.insn_class == SUB)
    {
      gdb_assert (insn.operands_count == 3);

      /* SUB 0,b,c.  */
      if (insn.operands[0].kind != ARC_OPERAND_KIND_REG)
	return true;

      int dst_regnum = insn.operands[0].value;
      regs[dst_regnum] = pv_subtract (arc_pv_get_operand (regs, insn, 1),
				      arc_pv_get_operand (regs, insn, 2));
      return true;
    }
  else if (insn.insn_class == ENTER)
    {
      /* ENTER_S is a prologue-in-instruction - it saves all callee-saved
	 registers according to given arguments thus greatly reducing code
	 size.  Which registers will be actually saved depends on arguments.

	 ENTER_S {R13-...,FP,BLINK} stores registers in following order:

	 new SP ->
		   BLINK
		   R13
		   R14
		   R15
		   ...
		   FP
	 old SP ->

	 There are up to three arguments for this opcode, as presented by ARC
	 disassembler:
	 1) amount of general-purpose registers to be saved - this argument is
	    always present even when it is 0;
	 2) FP register number (27) if FP has to be stored, otherwise argument
	    is not present;
	 3) BLINK register number (31) if BLINK has to be stored, otherwise
	    argument is not present.  If both FP and BLINK are stored, then FP
	    is present before BLINK in argument list.  */
      gdb_assert (insn.operands_count > 0);

      int regs_saved = arc_insn_get_operand_value (insn, 0);

      bool is_fp_saved;
      if (insn.operands_count > 1)
	is_fp_saved = (insn.operands[1].value  == ARC_FP_REGNUM);
      else
	is_fp_saved = false;

      bool is_blink_saved;
      if (insn.operands_count > 1)
	is_blink_saved = (insn.operands[insn.operands_count - 1].value
			  == ARC_BLINK_REGNUM);
      else
	is_blink_saved = false;

      /* Amount of bytes to be allocated to store specified registers.  */
      CORE_ADDR st_size = ((regs_saved + is_fp_saved + is_blink_saved)
			   * ARC_REGISTER_SIZE);
      pv_t new_sp = pv_add_constant (regs[ARC_SP_REGNUM], -st_size);

      /* Assume that if the last register (closest to new SP) can be written,
	 then it is possible to write all of them.  */
      if (stack->store_would_trash (new_sp))
	return false;

      /* Current store address.  */
      pv_t addr = regs[ARC_SP_REGNUM];

      if (is_fp_saved)
	{
	  addr = pv_add_constant (addr, -ARC_REGISTER_SIZE);
	  stack->store (addr, ARC_REGISTER_SIZE, regs[ARC_FP_REGNUM]);
	}

      /* Registers are stored in backward order: from GP (R26) to R13.  */
      for (int i = ARC_R13_REGNUM + regs_saved - 1; i >= ARC_R13_REGNUM; i--)
	{
	  addr = pv_add_constant (addr, -ARC_REGISTER_SIZE);
	  stack->store (addr, ARC_REGISTER_SIZE, regs[i]);
	}

      if (is_blink_saved)
	{
	  addr = pv_add_constant (addr, -ARC_REGISTER_SIZE);
	  stack->store (addr, ARC_REGISTER_SIZE,
			regs[ARC_BLINK_REGNUM]);
	}

      gdb_assert (pv_is_identical (addr, new_sp));

      regs[ARC_SP_REGNUM] = new_sp;

      if (is_fp_saved)
	regs[ARC_FP_REGNUM] = regs[ARC_SP_REGNUM];

      return true;
    }

  /* Some other architectures, like nds32 or arm, try to continue as far as
     possible when building a prologue cache (as opposed to when skipping
     prologue), so that cache will be as full as possible.  However current
     code for ARC doesn't recognize some instructions that may modify SP, like
     ADD, AND, OR, etc, hence there is no way to guarantee that SP wasn't
     clobbered by the skipped instruction.  Potential existence of extension
     instruction, which may do anything they want makes this even more complex,
     so it is just better to halt on a first unrecognized instruction.  */

  return false;
}

/* Analyze the prologue and update the corresponding frame cache for the frame
   unwinder for unwinding frames that doesn't have debug info.  In such
   situation GDB attempts to parse instructions in the prologue to understand
   where each register is saved.

   If CACHE is not NULL, then it will be filled with information about saved
   registers.

   There are several variations of prologue which GDB may encounter.  "Full"
   prologue looks like this:

	sub	sp,sp,<imm>   ; Space for variadic arguments.
	push	blink	      ; Store return address.
	push	r13	      ; Store callee saved registers (up to R26/GP).
	push	r14
	push	fp	      ; Store frame pointer.
	mov	fp,sp	      ; Update frame pointer.
	sub	sp,sp,<imm>   ; Create space for local vars on the stack.

   Depending on compiler options lots of things may change:

    1) BLINK is not saved in leaf functions.
    2) Frame pointer is not saved and updated if -fomit-frame-pointer is used.
    3) 16-bit versions of those instructions may be used.
    4) Instead of a sequence of several push'es, compiler may instead prefer to
    do one subtract on stack pointer and then store registers using normal
    store, that doesn't update SP.  Like this:


	sub	sp,sp,8		; Create space for callee-saved registers.
	st	r13,[sp,4]      ; Store callee saved registers (up to R26/GP).
	st	r14,[sp,0]

    5) ENTER_S instruction can encode most of prologue sequence in one
    instruction (except for those subtracts for variadic arguments and local
    variables).
    6) GCC may use "millicode" functions from libgcc to store callee-saved
    registers with minimal code-size requirements.  This function currently
    doesn't support this.

   ENTRYPOINT is a function entry point where prologue starts.

   LIMIT_PC is a maximum possible end address of prologue (meaning address
   of first instruction after the prologue).  It might also point to the middle
   of prologue if execution has been stopped by the breakpoint at this address
   - in this case debugger should analyze prologue only up to this address,
   because further instructions haven't been executed yet.

   Returns address of the first instruction after the prologue.  */

static CORE_ADDR
arc_analyze_prologue (struct gdbarch *gdbarch, const CORE_ADDR entrypoint,
		      const CORE_ADDR limit_pc, struct arc_frame_cache *cache)
{
  arc_debug_printf ("entrypoint=%s, limit_pc=%s",
		    paddress (gdbarch, entrypoint),
		    paddress (gdbarch, limit_pc));

  /* Prologue values.  Only core registers can be stored.  */
  pv_t regs[ARC_LAST_CORE_REGNUM + 1];
  for (int i = 0; i <= ARC_LAST_CORE_REGNUM; i++)
    regs[i] = pv_register (i, 0);
  pv_area stack (ARC_SP_REGNUM, gdbarch_addr_bit (gdbarch));

  CORE_ADDR current_prologue_end = entrypoint;

  /* Look at each instruction in the prologue.  */
  while (current_prologue_end < limit_pc)
    {
      struct arc_instruction insn;

      struct gdb_non_printing_memory_disassembler dis (gdbarch);
      arc_insn_decode (current_prologue_end, dis.disasm_info (),
		       arc_delayed_print_insn, &insn);

      if (arc_debug)
	arc_insn_dump (insn);

      /* If this instruction is in the prologue, fields in the cache will be
	 updated, and the saved registers mask may be updated.  */
      if (!arc_is_in_prologue (gdbarch, insn, regs, &stack))
	{
	  /* Found an instruction that is not in the prologue.  */
	  arc_debug_printf ("End of prologue reached at address %s",
			    paddress (gdbarch, insn.address));
	  break;
	}

      current_prologue_end = arc_insn_get_linear_next_pc (insn);
    }

  if (cache != NULL)
    {
      /* Figure out if it is a frame pointer or just a stack pointer.  */
      if (pv_is_register (regs[ARC_FP_REGNUM], ARC_SP_REGNUM))
	{
	  cache->frame_base_reg = ARC_FP_REGNUM;
	  cache->frame_base_offset = -regs[ARC_FP_REGNUM].k;
	}
      else
	{
	  cache->frame_base_reg = ARC_SP_REGNUM;
	  cache->frame_base_offset = -regs[ARC_SP_REGNUM].k;
	}

      /* Assign offset from old SP to all saved registers.  */
      for (int i = 0; i <= ARC_LAST_CORE_REGNUM; i++)
	{
	  CORE_ADDR offset;
	  if (stack.find_reg (gdbarch, i, &offset))
	    cache->saved_regs[i].set_addr (offset);
	}
    }

  return current_prologue_end;
}

/* Estimated maximum prologue length in bytes.  This should include:
   1) Store instruction for each callee-saved register (R25 - R13 + 1)
   2) Two instructions for FP
   3) One for BLINK
   4) Three substract instructions for SP (for variadic args, for
   callee saved regs and for local vars) and assuming that those SUB use
   long-immediate (hence double length).
   5) Stores of arguments registers are considered part of prologue too
      (R7 - R1 + 1).
   This is quite an extreme case, because even with -O0 GCC will collapse first
   two SUBs into one and long immediate values are quite unlikely to appear in
   this case, but still better to overshoot a bit - prologue analysis will
   anyway stop at the first instruction that doesn't fit prologue, so this
   limit will be rarely reached.  */

const static int MAX_PROLOGUE_LENGTH
  = 4 * (ARC_R25_REGNUM - ARC_R13_REGNUM + 1 + 2 + 1 + 6
	 + ARC_LAST_ARG_REGNUM - ARC_FIRST_ARG_REGNUM + 1);

/* Implement the "skip_prologue" gdbarch method.

   Skip the prologue for the function at PC.  This is done by checking from
   the line information read from the DWARF, if possible; otherwise, we scan
   the function prologue to find its end.  */

static CORE_ADDR
arc_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  arc_debug_printf ("pc = %s", paddress (gdbarch, pc));

  CORE_ADDR func_addr;
  const char *func_name;

  /* See what the symbol table says.  */
  if (find_pc_partial_function (pc, &func_name, &func_addr, NULL))
    {
      /* Found a function.  */
      CORE_ADDR postprologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);

      if (postprologue_pc != 0)
	return std::max (pc, postprologue_pc);
    }

  /* No prologue info in symbol table, have to analyze prologue.  */

  /* Find an upper limit on the function prologue using the debug
     information.  If there is no debug information about prologue end, then
     skip_prologue_using_sal will return 0.  */
  CORE_ADDR limit_pc = skip_prologue_using_sal (gdbarch, pc);

  /* If there is no debug information at all, it is required to give some
     semi-arbitrary hard limit on amount of bytes to scan during prologue
     analysis.  */
  if (limit_pc == 0)
    limit_pc = pc + MAX_PROLOGUE_LENGTH;

  /* Find the address of the first instruction after the prologue by scanning
     through it - no other information is needed, so pass NULL as a cache.  */
  return arc_analyze_prologue (gdbarch, pc, limit_pc, NULL);
}

/* Implement the "print_insn" gdbarch method.

   arc_get_disassembler () may return different functions depending on bfd
   type, so it is not possible to pass print_insn directly to
   set_gdbarch_print_insn ().  Instead this wrapper function is used.  It also
   may be used by other functions to get disassemble_info for address.  It is
   important to note, that those print_insn from opcodes always print
   instruction to the stream specified in the INFO.  If this is not desired,
   then either `print_insn` function in INFO should be set to some function
   that will not print, or `stream` should be different from standard
   gdb_stdlog.  */

int
arc_delayed_print_insn (bfd_vma addr, struct disassemble_info *info)
{
  /* Standard BFD "machine number" field allows libopcodes disassembler to
     distinguish ARC 600, 700 and v2 cores, however v2 encompasses both ARC EM
     and HS, which have some difference between.  There are two ways to specify
     what is the target core:
     1) via the disassemble_info->disassembler_options;
     2) otherwise libopcodes will use private (architecture-specific) ELF
     header.

     Using disassembler_options is preferable, because it comes directly from
     GDBserver which scanned an actual ARC core identification info.  However,
     not all GDBservers report core architecture, so as a fallback GDB still
     should support analysis of ELF header.  The libopcodes disassembly code
     uses the section to find the BFD and the BFD to find the ELF header,
     therefore this function should set disassemble_info->section properly.

     disassembler_options was already set by non-target specific code with
     proper options obtained via gdbarch_disassembler_options ().

     This function might be called multiple times in a sequence, reusing same
     disassemble_info.  */
  if ((info->disassembler_options == NULL) && (info->section == NULL))
    {
      struct obj_section *s = find_pc_section (addr);
      if (s != NULL)
	info->section = s->the_bfd_section;
    }

  return default_print_insn (addr, info);
}

/* Baremetal breakpoint instructions.

   ARC supports both big- and little-endian.  However, instructions for
   little-endian processors are encoded in the middle-endian: half-words are
   in big-endian, while bytes inside the half-words are in little-endian; data
   is represented in the "normal" little-endian.  Big-endian processors treat
   data and code identically.

   Assuming the number 0x01020304, it will be presented this way:

   Address            :  N   N+1  N+2  N+3
   little-endian      : 0x04 0x03 0x02 0x01
   big-endian         : 0x01 0x02 0x03 0x04
   ARC middle-endian  : 0x02 0x01 0x04 0x03
  */

static const gdb_byte arc_brk_s_be[] = { 0x7f, 0xff };
static const gdb_byte arc_brk_s_le[] = { 0xff, 0x7f };
static const gdb_byte arc_brk_be[] = { 0x25, 0x6f, 0x00, 0x3f };
static const gdb_byte arc_brk_le[] = { 0x6f, 0x25, 0x3f, 0x00 };

/* For ARC ELF, breakpoint uses the 16-bit BRK_S instruction, which is 0x7fff
   (little endian) or 0xff7f (big endian).  We used to insert BRK_S even
   instead of 32-bit instructions, which works mostly ok, unless breakpoint is
   inserted into delay slot instruction.  In this case if branch is taken
   BLINK value will be set to address of instruction after delay slot, however
   if we replaced 32-bit instruction in delay slot with 16-bit long BRK_S,
   then BLINK value will have an invalid value - it will point to the address
   after the BRK_S (which was there at the moment of branch execution) while
   it should point to the address after the 32-bit long instruction.  To avoid
   such issues this function disassembles instruction at target location and
   evaluates it value.

   ARC 600 supports only 16-bit BRK_S.

   NB: Baremetal GDB uses BRK[_S], while user-space GDB uses TRAP_S.  BRK[_S]
   is much better because it doesn't commit unlike TRAP_S, so it can be set in
   delay slots; however it cannot be used in user-mode, hence usage of TRAP_S
   in GDB for user-space.  */

/* Implement the "breakpoint_kind_from_pc" gdbarch method.  */

static int
arc_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  size_t length_with_limm = gdb_insn_length (gdbarch, *pcptr);

  /* Replace 16-bit instruction with BRK_S, replace 32-bit instructions with
     BRK.  LIMM is part of instruction length, so it can be either 4 or 8
     bytes for 32-bit instructions.  */
  if ((length_with_limm == 4 || length_with_limm == 8)
      && !arc_mach_is_arc600 (gdbarch))
    return sizeof (arc_brk_le);
  else
    return sizeof (arc_brk_s_le);
}

/* Implement the "sw_breakpoint_from_kind" gdbarch method.  */

static const gdb_byte *
arc_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
  gdb_assert (kind == 2 || kind == 4);
  *size = kind;

  if (kind == sizeof (arc_brk_le))
    {
      return ((gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	      ? arc_brk_be
	      : arc_brk_le);
    }
  else
    {
      return ((gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	      ? arc_brk_s_be
	      : arc_brk_s_le);
    }
}

/* Implement the "frame_align" gdbarch method.  */

static CORE_ADDR
arc_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  return align_down (sp, 4);
}

/* Dump the frame info.  Used for internal debugging only.  */

static void
arc_print_frame_cache (struct gdbarch *gdbarch, const char *message,
		       struct arc_frame_cache *cache, int addresses_known)
{
  arc_debug_printf ("frame_info %s", message);
  arc_debug_printf ("prev_sp = %s", paddress (gdbarch, cache->prev_sp));
  arc_debug_printf ("frame_base_reg = %i", cache->frame_base_reg);
  arc_debug_printf ("frame_base_offset = %s",
		    plongest (cache->frame_base_offset));

  for (int i = 0; i <= ARC_BLINK_REGNUM; i++)
    {
      if (cache->saved_regs[i].is_addr ())
	arc_debug_printf ("saved register %s at %s %s",
			  gdbarch_register_name (gdbarch, i),
			  (addresses_known) ? "address" : "offset",
			      paddress (gdbarch, cache->saved_regs[i].addr ()));
    }
}

/* Frame unwinder for normal frames.  */

static struct arc_frame_cache *
arc_make_frame_cache (frame_info_ptr this_frame)
{
  arc_debug_printf ("called");

  struct gdbarch *gdbarch = get_frame_arch (this_frame);

  CORE_ADDR block_addr = get_frame_address_in_block (this_frame);
  CORE_ADDR entrypoint, prologue_end;
  if (find_pc_partial_function (block_addr, NULL, &entrypoint, &prologue_end))
    {
      struct symtab_and_line sal = find_pc_line (entrypoint, 0);
      CORE_ADDR prev_pc = get_frame_pc (this_frame);
      if (sal.line == 0)
	/* No line info so use current PC.  */
	prologue_end = prev_pc;
      else if (sal.end < prologue_end)
	/* The next line begins after the function end.  */
	prologue_end = sal.end;

      prologue_end = std::min (prologue_end, prev_pc);
    }
  else
    {
      /* If find_pc_partial_function returned nothing then there is no symbol
	 information at all for this PC.  Currently it is assumed in this case
	 that current PC is entrypoint to function and try to construct the
	 frame from that.  This is, probably, suboptimal, for example ARM
	 assumes in this case that program is inside the normal frame (with
	 frame pointer).  ARC, perhaps, should try to do the same.  */
      entrypoint = get_frame_register_unsigned (this_frame,
						gdbarch_pc_regnum (gdbarch));
      prologue_end = entrypoint + MAX_PROLOGUE_LENGTH;
    }

  /* Allocate new frame cache instance and space for saved register info.
     FRAME_OBSTACK_ZALLOC will initialize fields to zeroes.  */
  struct arc_frame_cache *cache
    = FRAME_OBSTACK_ZALLOC (struct arc_frame_cache);
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  arc_analyze_prologue (gdbarch, entrypoint, prologue_end, cache);

  if (arc_debug)
    arc_print_frame_cache (gdbarch, "after prologue", cache, false);

  CORE_ADDR unwound_fb = get_frame_register_unsigned (this_frame,
						      cache->frame_base_reg);
  if (unwound_fb == 0)
    return cache;
  cache->prev_sp = unwound_fb + cache->frame_base_offset;

  for (int i = 0; i <= ARC_LAST_CORE_REGNUM; i++)
    {
      if (cache->saved_regs[i].is_addr ())
	cache->saved_regs[i].set_addr (cache->saved_regs[i].addr ()
				       + cache->prev_sp);
    }

  if (arc_debug)
    arc_print_frame_cache (gdbarch, "after previous SP found", cache, true);

  return cache;
}

/* Implement the "this_id" frame_unwind method.  */

static void
arc_frame_this_id (frame_info_ptr this_frame, void **this_cache,
		   struct frame_id *this_id)
{
  arc_debug_printf ("called");

  struct gdbarch *gdbarch = get_frame_arch (this_frame);

  if (*this_cache == NULL)
    *this_cache = arc_make_frame_cache (this_frame);
  struct arc_frame_cache *cache = (struct arc_frame_cache *) (*this_cache);

  CORE_ADDR stack_addr = cache->prev_sp;

  /* There are 4 possible situation which decide how frame_id->code_addr is
     evaluated:

     1) Function is compiled with option -g.  Then frame_id will be created
     in dwarf_* function and not in this function.  NB: even if target
     binary is compiled with -g, some std functions like __start and _init
     are not, so they still will follow one of the following choices.

     2) Function is compiled without -g and binary hasn't been stripped in
     any way.  In this case GDB still has enough information to evaluate
     frame code_addr properly.  This case is covered by call to
     get_frame_func ().

     3) Binary has been striped with option -g (strip debug symbols).  In
     this case there is still enough symbols for get_frame_func () to work
     properly, so this case is also covered by it.

     4) Binary has been striped with option -s (strip all symbols).  In this
     case GDB cannot get function start address properly, so we return current
     PC value instead.
   */
  CORE_ADDR code_addr = get_frame_func (this_frame);
  if (code_addr == 0)
    code_addr = get_frame_register_unsigned (this_frame,
					     gdbarch_pc_regnum (gdbarch));

  *this_id = frame_id_build (stack_addr, code_addr);
}

/* Implement the "prev_register" frame_unwind method.  */

static struct value *
arc_frame_prev_register (frame_info_ptr this_frame,
			 void **this_cache, int regnum)
{
  if (*this_cache == NULL)
    *this_cache = arc_make_frame_cache (this_frame);
  struct arc_frame_cache *cache = (struct arc_frame_cache *) (*this_cache);

  struct gdbarch *gdbarch = get_frame_arch (this_frame);

  /* If we are asked to unwind the PC, then we need to return BLINK instead:
     the saved value of PC points into this frame's function's prologue, not
     the next frame's function's resume location.  */
  if (regnum == gdbarch_pc_regnum (gdbarch))
    regnum = ARC_BLINK_REGNUM;

  /* SP is a special case - we should return prev_sp, because
     trad_frame_get_prev_register will return _current_ SP value.
     Alternatively we could have stored cache->prev_sp in the cache->saved
     regs, but here we follow the lead of AArch64, ARM and Xtensa and will
     leave that logic in this function, instead of prologue analyzers.  That I
     think is a bit more clear as `saved_regs` should contain saved regs, not
     computable.

     Because value has been computed, "got_constant" should be used, so that
     returned value will be a "not_lval" - immutable.  */

  if (regnum == gdbarch_sp_regnum (gdbarch))
    return frame_unwind_got_constant (this_frame, regnum, cache->prev_sp);

  return trad_frame_get_prev_register (this_frame, cache->saved_regs, regnum);
}

/* Implement the "init_reg" dwarf2_frame method.  */

static void
arc_dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
			   struct dwarf2_frame_state_reg *reg,
			   frame_info_ptr info)
{
  if (regnum == gdbarch_pc_regnum (gdbarch))
    /* The return address column.  */
    reg->how = DWARF2_FRAME_REG_RA;
  else if (regnum == gdbarch_sp_regnum (gdbarch))
    /* The call frame address.  */
    reg->how = DWARF2_FRAME_REG_CFA;
}

/*  Signal trampoline frame unwinder.  Allows frame unwinding to happen
    from within signal handlers.  */

static struct arc_frame_cache *
arc_make_sigtramp_frame_cache (frame_info_ptr this_frame)
{
  arc_debug_printf ("called");

  gdbarch *arch = get_frame_arch (this_frame);
  arc_gdbarch_tdep *tdep = gdbarch_tdep<arc_gdbarch_tdep> (arch);

  /* Allocate new frame cache instance and space for saved register info.  */
  struct arc_frame_cache *cache = FRAME_OBSTACK_ZALLOC (struct arc_frame_cache);
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* Get the stack pointer and use it as the frame base.  */
  cache->prev_sp = arc_frame_base_address (this_frame, NULL);

  /* If the ARC-private target-dependent info doesn't have a table of
     offsets of saved register contents within an OS signal context
     structure, then there is nothing to analyze.  */
  if (tdep->sc_reg_offset == NULL)
    return cache;

  /* Find the address of the sigcontext structure.  */
  CORE_ADDR addr = tdep->sigcontext_addr (this_frame);

  /* For each register, if its contents have been saved within the
     sigcontext structure, determine the address of those contents.  */
  gdb_assert (tdep->sc_num_regs <= (ARC_LAST_REGNUM + 1));
  for (int i = 0; i < tdep->sc_num_regs; i++)
    {
      if (tdep->sc_reg_offset[i] != ARC_OFFSET_NO_REGISTER)
	cache->saved_regs[i].set_addr (addr + tdep->sc_reg_offset[i]);
    }

  return cache;
}

/* Implement the "this_id" frame_unwind method for signal trampoline
   frames.  */

static void
arc_sigtramp_frame_this_id (frame_info_ptr this_frame,
			    void **this_cache, struct frame_id *this_id)
{
  arc_debug_printf ("called");

  if (*this_cache == NULL)
    *this_cache = arc_make_sigtramp_frame_cache (this_frame);

  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct arc_frame_cache *cache = (struct arc_frame_cache *) *this_cache;
  CORE_ADDR stack_addr = cache->prev_sp;
  CORE_ADDR code_addr
    = get_frame_register_unsigned (this_frame, gdbarch_pc_regnum (gdbarch));
  *this_id = frame_id_build (stack_addr, code_addr);
}

/* Get a register from a signal handler frame.  */

static struct value *
arc_sigtramp_frame_prev_register (frame_info_ptr this_frame,
				  void **this_cache, int regnum)
{
  arc_debug_printf ("regnum = %d", regnum);

  /* Make sure we've initialized the cache.  */
  if (*this_cache == NULL)
    *this_cache = arc_make_sigtramp_frame_cache (this_frame);

  struct arc_frame_cache *cache = (struct arc_frame_cache *) *this_cache;
  return trad_frame_get_prev_register (this_frame, cache->saved_regs, regnum);
}

/* Frame sniffer for signal handler frame.  Only recognize a frame if we
   have a sigcontext_addr handler in the target dependency.  */

static int
arc_sigtramp_frame_sniffer (const struct frame_unwind *self,
			    frame_info_ptr this_frame,
			    void **this_cache)
{
  arc_debug_printf ("called");

  gdbarch *arch = get_frame_arch (this_frame);
  arc_gdbarch_tdep *tdep = gdbarch_tdep<arc_gdbarch_tdep> (arch);

  /* If we have a sigcontext_addr handler, then just return 1 (same as the
     "default_frame_sniffer ()").  */
  return (tdep->sigcontext_addr != NULL && tdep->is_sigtramp != NULL
	  && tdep->is_sigtramp (this_frame));
}

/* Structure defining the ARC ordinary frame unwind functions.  Since we are
   the fallback unwinder, we use the default frame sniffer, which always
   accepts the frame.  */

static const struct frame_unwind arc_frame_unwind = {
  "arc prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  arc_frame_this_id,
  arc_frame_prev_register,
  NULL,
  default_frame_sniffer,
  NULL,
  NULL
};

/* Structure defining the ARC signal frame unwind functions.  Custom
   sniffer is used, because this frame must be accepted only in the right
   context.  */

static const struct frame_unwind arc_sigtramp_frame_unwind = {
  "arc sigtramp",
  SIGTRAMP_FRAME,
  default_frame_unwind_stop_reason,
  arc_sigtramp_frame_this_id,
  arc_sigtramp_frame_prev_register,
  NULL,
  arc_sigtramp_frame_sniffer,
  NULL,
  NULL
};


static const struct frame_base arc_normal_base = {
  &arc_frame_unwind,
  arc_frame_base_address,
  arc_frame_base_address,
  arc_frame_base_address
};

static enum arc_isa
mach_type_to_arc_isa (const unsigned long mach)
{
  switch (mach)
    {
    case bfd_mach_arc_arc600:
    case bfd_mach_arc_arc601:
    case bfd_mach_arc_arc700:
      return ARC_ISA_ARCV1;
    case bfd_mach_arc_arcv2:
      return ARC_ISA_ARCV2;
    default:
	internal_error (_("unknown machine id %lu"), mach);
    }
}

/* See arc-tdep.h.  */

arc_arch_features
arc_arch_features_create (const bfd *abfd, const unsigned long mach)
{
  /* Use 4 as a fallback value.  */
  int reg_size = 4;

  /* Try to guess the features parameters by looking at the binary to be
     executed.  If the user is providing a binary that does not match the
     target, then tough luck.  This is the last effort to makes sense of
     what's going on.  */
  if (abfd != nullptr && bfd_get_flavour (abfd) == bfd_target_elf_flavour)
    {
      unsigned char eclass = elf_elfheader (abfd)->e_ident[EI_CLASS];

      if (eclass == ELFCLASS32)
	reg_size = 4;
      else if (eclass == ELFCLASS64)
	reg_size = 8;
      else
	internal_error (_("unknown ELF header class %d"), eclass);
    }

  /* MACH from a bfd_arch_info struct is used here.  It should be a safe
     bet, as it looks like the struct is always initialized even when we
     don't pass any elf file to GDB at all (it uses default arch in that
     case).  */
  arc_isa isa = mach_type_to_arc_isa (mach);

  return arc_arch_features (reg_size, isa);
}

/* Look for obsolete core feature names in TDESC.  */

static const struct tdesc_feature *
find_obsolete_core_names (const struct target_desc *tdesc)
{
  const struct tdesc_feature *feat = nullptr;

  feat = tdesc_find_feature (tdesc, ARC_CORE_V1_OBSOLETE_FEATURE_NAME);

  if (feat == nullptr)
    feat = tdesc_find_feature (tdesc, ARC_CORE_V2_OBSOLETE_FEATURE_NAME);

  if (feat == nullptr)
    feat = tdesc_find_feature
      (tdesc, ARC_CORE_V2_REDUCED_OBSOLETE_FEATURE_NAME);

  return feat;
}

/* Look for obsolete aux feature names in TDESC.  */

static const struct tdesc_feature *
find_obsolete_aux_names (const struct target_desc *tdesc)
{
  return tdesc_find_feature (tdesc, ARC_AUX_OBSOLETE_FEATURE_NAME);
}

/* Based on the MACH value, determines which core register features set
   must be used.  */

static arc_register_feature *
determine_core_reg_feature_set (const unsigned long mach)
{
  switch (mach_type_to_arc_isa (mach))
    {
    case ARC_ISA_ARCV1:
      return &arc_v1_core_reg_feature;
    case ARC_ISA_ARCV2:
      return &arc_v2_core_reg_feature;
    default:
      gdb_assert_not_reached
	("Unknown machine type to determine the core feature set.");
    }
}

/* At the moment, there is only 1 auxiliary register features set.
   This is a place holder for future extendability.  */

static const arc_register_feature *
determine_aux_reg_feature_set ()
{
  return &arc_common_aux_reg_feature;
}

/* Update accumulator register names (ACCH/ACCL) for r58 and r59 in the
   register sets.  The endianness determines the assignment:

	,------.------.
	| acch | accl |
   ,----|------+------|
   | LE | r59  | r58  |
   | BE | r58  | r59  |
   `----^------^------'  */

static void
arc_update_acc_reg_names (const int byte_order)
{
  const char *r58_alias
    = byte_order == BFD_ENDIAN_LITTLE ? "accl" : "acch";
  const char *r59_alias
    = byte_order == BFD_ENDIAN_LITTLE ? "acch" : "accl";

  /* Subscript 1 must be OK because those registers have 2 names.  */
  arc_v1_core_reg_feature.registers[ARC_R58_REGNUM].names[1] = r58_alias;
  arc_v1_core_reg_feature.registers[ARC_R59_REGNUM].names[1] = r59_alias;
  arc_v2_core_reg_feature.registers[ARC_R58_REGNUM].names[1] = r58_alias;
  arc_v2_core_reg_feature.registers[ARC_R59_REGNUM].names[1] = r59_alias;
}

/* Go through all the registers in REG_SET and check if they exist
   in FEATURE.  The TDESC_DATA is updated with the register number
   in REG_SET if it is found in the feature.  If a required register
   is not found, this function returns false.  */

static bool
arc_check_tdesc_feature (struct tdesc_arch_data *tdesc_data,
			 const struct tdesc_feature *feature,
			 const struct arc_register_feature *reg_set)
{
  for (const auto &reg : reg_set->registers)
    {
      bool found = false;

      for (const char *name : reg.names)
	{
	  found
	    = tdesc_numbered_register (feature, tdesc_data, reg.regnum, name);

	  if (found)
	    break;
	}

      if (!found && reg.required_p)
	{
	  std::ostringstream reg_names;
	  for (std::size_t i = 0; i < reg.names.size(); ++i)
	    {
	      if (i == 0)
		reg_names << "'" << reg.names[0] << "'";
	      else
		reg_names << " or '" << reg.names[0] << "'";
	    }
	  arc_print (_("Error: Cannot find required register(s) %s "
		       "in feature '%s'.\n"), reg_names.str ().c_str (),
		       feature->name.c_str ());
	  return false;
	}
    }

  return true;
}

/* Check for the existance of "lp_start" and "lp_end" in target description.
   If both are present, assume there is hardware loop support in the target.
   This can be improved by looking into "lpc_size" field of "isa_config"
   auxiliary register.  */

static bool
arc_check_for_hw_loops (const struct target_desc *tdesc,
			struct tdesc_arch_data *data)
{
  const auto feature_aux = tdesc_find_feature (tdesc, ARC_AUX_FEATURE_NAME);
  const auto aux_regset = determine_aux_reg_feature_set ();

  if (feature_aux == nullptr)
    return false;

  bool hw_loop_p = false;
  const auto lp_start_name =
    aux_regset->registers[ARC_LP_START_REGNUM - ARC_FIRST_AUX_REGNUM].names[0];
  const auto lp_end_name =
    aux_regset->registers[ARC_LP_END_REGNUM - ARC_FIRST_AUX_REGNUM].names[0];

  hw_loop_p = tdesc_numbered_register (feature_aux, data,
				       ARC_LP_START_REGNUM, lp_start_name);
  hw_loop_p &= tdesc_numbered_register (feature_aux, data,
				       ARC_LP_END_REGNUM, lp_end_name);

  return hw_loop_p;
}

/* Initialize target description for the ARC.

   Returns true if input TDESC was valid and in this case it will assign TDESC
   and TDESC_DATA output parameters.  */

static bool
arc_tdesc_init (struct gdbarch_info info, const struct target_desc **tdesc,
		tdesc_arch_data_up *tdesc_data)
{
  const struct target_desc *tdesc_loc = info.target_desc;
  arc_debug_printf ("Target description initialization.");

  /* If target doesn't provide a description, use the default ones.  */
  if (!tdesc_has_registers (tdesc_loc))
    {
      arc_arch_features features
	= arc_arch_features_create (info.abfd,
				    info.bfd_arch_info->mach);
      tdesc_loc = arc_lookup_target_description (features);
    }
  gdb_assert (tdesc_loc != nullptr);

  arc_debug_printf ("Have got a target description");

  const struct tdesc_feature *feature_core
    = tdesc_find_feature (tdesc_loc, ARC_CORE_FEATURE_NAME);
  const struct tdesc_feature *feature_aux
    = tdesc_find_feature (tdesc_loc, ARC_AUX_FEATURE_NAME);

  /* Maybe there still is a chance to salvage the input.  */
  if (feature_core == nullptr)
    feature_core = find_obsolete_core_names (tdesc_loc);
  if (feature_aux == nullptr)
    feature_aux = find_obsolete_aux_names (tdesc_loc);

  if (feature_core == nullptr)
    {
      arc_print (_("Error: Cannot find required feature '%s' in supplied "
		   "target description.\n"), ARC_CORE_FEATURE_NAME);
      return false;
    }

  if (feature_aux == nullptr)
    {
      arc_print (_("Error: Cannot find required feature '%s' in supplied "
		   "target description.\n"), ARC_AUX_FEATURE_NAME);
      return false;
    }

  const arc_register_feature *arc_core_reg_feature
    = determine_core_reg_feature_set (info.bfd_arch_info->mach);
  const arc_register_feature *arc_aux_reg_feature
    = determine_aux_reg_feature_set ();

  tdesc_arch_data_up tdesc_data_loc = tdesc_data_alloc ();

  arc_update_acc_reg_names (info.byte_order);

  bool valid_p = arc_check_tdesc_feature (tdesc_data_loc.get (),
					  feature_core,
					  arc_core_reg_feature);

  valid_p &= arc_check_tdesc_feature (tdesc_data_loc.get (),
				      feature_aux,
				      arc_aux_reg_feature);

  if (!valid_p)
    {
      arc_debug_printf ("Target description is not valid");
      return false;
    }

  *tdesc = tdesc_loc;
  *tdesc_data = std::move (tdesc_data_loc);

  return true;
}

/* Implement the type_align gdbarch function.  */

static ULONGEST
arc_type_align (struct gdbarch *gdbarch, struct type *type)
{
  switch (type->code ())
    {
    case TYPE_CODE_PTR:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_INT:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_FLT:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_REF:
    case TYPE_CODE_RVALUE_REF:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_DECFLOAT:
    case TYPE_CODE_METHODPTR:
    case TYPE_CODE_MEMBERPTR:
      type = check_typedef (type);
      return std::min<ULONGEST> (4, type->length ());
    default:
      return 0;
    }
}

/* Implement the "init" gdbarch method.  */

static struct gdbarch *
arc_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  const struct target_desc *tdesc;
  tdesc_arch_data_up tdesc_data;

  arc_debug_printf ("Architecture initialization.");

  if (!arc_tdesc_init (info, &tdesc, &tdesc_data))
    return nullptr;

  /* Allocate the ARC-private target-dependent information structure, and the
     GDB target-independent information structure.  */
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new arc_gdbarch_tdep));
  arc_gdbarch_tdep *tdep = gdbarch_tdep<arc_gdbarch_tdep> (gdbarch);
  tdep->jb_pc = -1; /* No longjmp support by default.  */
  tdep->has_hw_loops = arc_check_for_hw_loops (tdesc, tdesc_data.get ());

  /* Data types.  */
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_type_align (gdbarch, arc_type_align);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_double_format (gdbarch, floatformats_ieee_double);
  set_gdbarch_ptr_bit (gdbarch, 32);
  set_gdbarch_addr_bit (gdbarch, 32);
  set_gdbarch_char_signed (gdbarch, 0);

  set_gdbarch_write_pc (gdbarch, arc_write_pc);

  set_gdbarch_virtual_frame_pointer (gdbarch, arc_virtual_frame_pointer);

  /* tdesc_use_registers expects gdbarch_num_regs to return number of registers
     parsed by gdbarch_init, and then it will add all of the remaining
     registers and will increase number of registers.  */
  set_gdbarch_num_regs (gdbarch, ARC_LAST_REGNUM + 1);
  set_gdbarch_num_pseudo_regs (gdbarch, 0);
  set_gdbarch_sp_regnum (gdbarch, ARC_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, ARC_PC_REGNUM);
  set_gdbarch_ps_regnum (gdbarch, ARC_STATUS32_REGNUM);
  set_gdbarch_fp0_regnum (gdbarch, -1);	/* No FPU registers.  */

  set_gdbarch_push_dummy_call (gdbarch, arc_push_dummy_call);
  set_gdbarch_push_dummy_code (gdbarch, arc_push_dummy_code);

  set_gdbarch_cannot_fetch_register (gdbarch, arc_cannot_fetch_register);
  set_gdbarch_cannot_store_register (gdbarch, arc_cannot_store_register);

  set_gdbarch_believe_pcc_promotion (gdbarch, 1);

  set_gdbarch_return_value (gdbarch, arc_return_value);

  set_gdbarch_skip_prologue (gdbarch, arc_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  set_gdbarch_breakpoint_kind_from_pc (gdbarch, arc_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, arc_sw_breakpoint_from_kind);

  /* On ARC 600 BRK_S instruction advances PC, unlike other ARC cores.  */
  if (!arc_mach_is_arc600 (gdbarch))
    set_gdbarch_decr_pc_after_break (gdbarch, 0);
  else
    set_gdbarch_decr_pc_after_break (gdbarch, 2);

  set_gdbarch_frame_align (gdbarch, arc_frame_align);

  set_gdbarch_print_insn (gdbarch, arc_delayed_print_insn);

  set_gdbarch_cannot_step_breakpoint (gdbarch, 1);

  /* "nonsteppable" watchpoint means that watchpoint triggers before
     instruction is committed, therefore it is required to remove watchpoint
     to step though instruction that triggers it.  ARC watchpoints trigger
     only after instruction is committed, thus there is no need to remove
     them.  In fact on ARC watchpoint for memory writes may trigger with more
     significant delay, like one or two instructions, depending on type of
     memory where write is performed (CCM or external) and next instruction
     after the memory write.  */
  set_gdbarch_have_nonsteppable_watchpoint (gdbarch, 0);

  /* This doesn't include possible long-immediate value.  */
  set_gdbarch_max_insn_length (gdbarch, 4);

  /* Frame unwinders and sniffers.  */
  dwarf2_frame_set_init_reg (gdbarch, arc_dwarf2_frame_init_reg);
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &arc_sigtramp_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &arc_frame_unwind);
  frame_base_set_default (gdbarch, &arc_normal_base);

  /* Setup stuff specific to a particular environment (baremetal or Linux).
     It can override functions set earlier.  */
  gdbarch_init_osabi (info, gdbarch);

  if (tdep->jb_pc >= 0)
    set_gdbarch_get_longjmp_target (gdbarch, arc_get_longjmp_target);

  /* Disassembler options.  Enforce CPU if it was specified in XML target
     description, otherwise use default method of determining CPU (ELF private
     header).  */
  if (info.target_desc != NULL)
    {
      const struct bfd_arch_info *tdesc_arch
	= tdesc_architecture (info.target_desc);
      if (tdesc_arch != NULL)
	{
	  xfree (arc_disassembler_options);
	  /* FIXME: It is not really good to change disassembler options
	     behind the scene, because that might override options
	     specified by the user.  However as of now ARC doesn't support
	     `set disassembler-options' hence this code is the only place
	     where options are changed.  It also changes options for all
	     existing gdbarches, which also can be problematic, if
	     arc_gdbarch_init will start reusing existing gdbarch
	     instances.  */
	  /* Target description specifies a BFD architecture, which is
	     different from ARC cpu, as accepted by disassembler (and most
	     other ARC tools), because cpu values are much more fine grained -
	     there can be multiple cpu values per single BFD architecture.  As
	     a result this code should translate architecture to some cpu
	     value.  Since there is no info on exact cpu configuration, it is
	     best to use the most feature-rich CPU, so that disassembler will
	     recognize all instructions available to the specified
	     architecture.  */
	  switch (tdesc_arch->mach)
	    {
	    case bfd_mach_arc_arc601:
	      arc_disassembler_options = xstrdup ("cpu=arc601");
	      break;
	    case bfd_mach_arc_arc600:
	      arc_disassembler_options = xstrdup ("cpu=arc600");
	      break;
	    case bfd_mach_arc_arc700:
	      arc_disassembler_options = xstrdup ("cpu=arc700");
	      break;
	    case bfd_mach_arc_arcv2:
	      /* Machine arcv2 has three arches: ARCv2, EM and HS; where ARCv2
		 is treated as EM.  */
	      if (arc_arch_is_hs (tdesc_arch))
		arc_disassembler_options = xstrdup ("cpu=hs38_linux");
	      else
		arc_disassembler_options = xstrdup ("cpu=em4_fpuda");
	      break;
	    default:
	      arc_disassembler_options = NULL;
	      break;
	    }
	}
    }

  set_gdbarch_disassembler_options (gdbarch, &arc_disassembler_options);
  set_gdbarch_valid_disassembler_options (gdbarch,
					  disassembler_options_arc ());

  tdesc_use_registers (gdbarch, tdesc, std::move (tdesc_data));

  return gdbarch;
}

/* Implement the "dump_tdep" gdbarch method.  */

static void
arc_dump_tdep (struct gdbarch *gdbarch, struct ui_file *file)
{
  arc_gdbarch_tdep *tdep = gdbarch_tdep<arc_gdbarch_tdep> (gdbarch);

  gdb_printf (file, "arc_dump_tdep: jb_pc = %i\n", tdep->jb_pc);

  gdb_printf (file, "arc_dump_tdep: is_sigtramp = <%s>\n",
	      host_address_to_string (tdep->is_sigtramp));
  gdb_printf (file, "arc_dump_tdep: sigcontext_addr = <%s>\n",
	      host_address_to_string (tdep->sigcontext_addr));
  gdb_printf (file, "arc_dump_tdep: sc_reg_offset = <%s>\n",
	      host_address_to_string (tdep->sc_reg_offset));
  gdb_printf (file, "arc_dump_tdep: sc_num_regs = %d\n",
	      tdep->sc_num_regs);
}

/* This command accepts single argument - address of instruction to
   disassemble.  */

static void
dump_arc_instruction_command (const char *args, int from_tty)
{
  struct value *val;
  if (args != NULL && strlen (args) > 0)
    val = parse_expression (args)->evaluate ();
  else
    val = access_value_history (0);
  val->record_latest ();

  CORE_ADDR address = value_as_address (val);
  struct arc_instruction insn;
  gdb_non_printing_memory_disassembler dis (current_inferior ()->arch ());
  arc_insn_decode (address, dis.disasm_info (), arc_delayed_print_insn, &insn);
  arc_insn_dump (insn);
}

void _initialize_arc_tdep ();
void
_initialize_arc_tdep ()
{
  gdbarch_register (bfd_arch_arc, arc_gdbarch_init, arc_dump_tdep);

  /* Register ARC-specific commands with gdb.  */

  /* Add root prefix command for "maintenance print arc" commands.  */
  add_basic_prefix_cmd ("arc", class_maintenance,
			_("ARC-specific maintenance commands for printing GDB "
			  "internal state."),
			&maintenance_print_arc_list,
			0, &maintenanceprintlist);

  add_cmd ("arc-instruction", class_maintenance,
	   dump_arc_instruction_command,
	   _("Dump arc_instruction structure for specified address."),
	   &maintenance_print_arc_list);

  /* Debug internals for ARC GDB.  */
  add_setshow_boolean_cmd ("arc", class_maintenance,
			   &arc_debug,
			   _("Set ARC specific debugging."),
			   _("Show ARC specific debugging."),
			   _("When set, ARC specific debugging is enabled."),
			   NULL, NULL, &setdebuglist, &showdebuglist);
}
