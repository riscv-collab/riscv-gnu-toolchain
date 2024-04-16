/* simulator.c -- Interface for the AArch64 simulator.

   Copyright (C) 2015-2024 Free Software Foundation, Inc.

   Contributed by Red Hat.

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

/* This must come before any other includes.  */
#include "defs.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#include "aarch64-sim.h"
#include "simulator.h"
#include "cpustate.h"
#include "memory.h"

#include "sim-signal.h"

#define NO_SP 0
#define SP_OK 1

#define TST(_flag)   (aarch64_test_CPSR_bit (cpu, _flag))
#define IS_SET(_X)   (TST (( _X )) ? 1 : 0)
#define IS_CLEAR(_X) (TST (( _X )) ? 0 : 1)

/* Space saver macro.  */
#define INSTR(HIGH, LOW) uimm (aarch64_get_instr (cpu), (HIGH), (LOW))

#define HALT_UNALLOC							\
  do									\
    {									\
      TRACE_DISASM (cpu, aarch64_get_PC (cpu));				\
      TRACE_INSN (cpu,							\
		  "Unallocated instruction detected at sim line %d,"	\
		  " exe addr %" PRIx64,					\
		  __LINE__, aarch64_get_PC (cpu));			\
      sim_engine_halt (CPU_STATE (cpu), cpu, NULL, aarch64_get_PC (cpu),\
		       sim_stopped, SIM_SIGILL);			\
    }									\
  while (0)

#define HALT_NYI							\
  do									\
    {									\
      TRACE_DISASM (cpu, aarch64_get_PC (cpu));				\
      TRACE_INSN (cpu,							\
		  "Unimplemented instruction detected at sim line %d,"	\
		  " exe addr %" PRIx64,					\
		  __LINE__, aarch64_get_PC (cpu));			\
      if (! TRACE_ANY_P (cpu))						\
        sim_io_eprintf (CPU_STATE (cpu), "SIM Error: Unimplemented instruction: %#08x\n", \
                        aarch64_get_instr (cpu));			\
      sim_engine_halt (CPU_STATE (cpu), cpu, NULL, aarch64_get_PC (cpu),\
		       sim_stopped, SIM_SIGABRT);			\
    }									\
  while (0)

#define NYI_assert(HI, LO, EXPECTED)					\
  do									\
    {									\
      if (INSTR ((HI), (LO)) != (EXPECTED))				\
	HALT_NYI;							\
    }									\
  while (0)

static uint64_t
expand_logical_immediate (uint32_t S, uint32_t R, uint32_t N)
{
  uint64_t mask;
  uint64_t imm;
  unsigned simd_size;

  /* The immediate value is S+1 bits to 1, left rotated by SIMDsize - R
     (in other words, right rotated by R), then replicated. */
  if (N != 0)
    {
      simd_size = 64;
      mask = 0xffffffffffffffffull;
    }
  else
    {
      switch (S)
	{
	case 0x00 ... 0x1f: /* 0xxxxx */ simd_size = 32;           break;
	case 0x20 ... 0x2f: /* 10xxxx */ simd_size = 16; S &= 0xf; break;
	case 0x30 ... 0x37: /* 110xxx */ simd_size =  8; S &= 0x7; break;
	case 0x38 ... 0x3b: /* 1110xx */ simd_size =  4; S &= 0x3; break;
	case 0x3c ... 0x3d: /* 11110x */ simd_size =  2; S &= 0x1; break;
	default: return 0;
	}
      mask = (1ull << simd_size) - 1;
      /* Top bits are IGNORED.  */
      R &= simd_size - 1;
    }

  /* NOTE: if S = simd_size - 1 we get 0xf..f which is rejected.  */
  if (S == simd_size - 1)
    return 0;

  /* S+1 consecutive bits to 1.  */
  /* NOTE: S can't be 63 due to detection above.  */
  imm = (1ull << (S + 1)) - 1;

  /* Rotate to the left by simd_size - R.  */
  if (R != 0)
    imm = ((imm << (simd_size - R)) & mask) | (imm >> R);

  /* Replicate the value according to SIMD size.  */
  switch (simd_size)
    {
    case  2: imm = (imm <<  2) | imm; ATTRIBUTE_FALLTHROUGH;
    case  4: imm = (imm <<  4) | imm; ATTRIBUTE_FALLTHROUGH;
    case  8: imm = (imm <<  8) | imm; ATTRIBUTE_FALLTHROUGH;
    case 16: imm = (imm << 16) | imm; ATTRIBUTE_FALLTHROUGH;
    case 32: imm = (imm << 32) | imm; ATTRIBUTE_FALLTHROUGH;
    case 64: break;
    default: return 0;
    }

  return imm;
}

/* Instr[22,10] encodes N immr and imms. we want a lookup table
   for each possible combination i.e. 13 bits worth of int entries.  */
#define  LI_TABLE_SIZE  (1 << 13)
static uint64_t LITable[LI_TABLE_SIZE];

void
aarch64_init_LIT_table (void)
{
  unsigned index;

  for (index = 0; index < LI_TABLE_SIZE; index++)
    {
      uint32_t N    = uimm (index, 12, 12);
      uint32_t immr = uimm (index, 11, 6);
      uint32_t imms = uimm (index, 5, 0);

      LITable [index] = expand_logical_immediate (imms, immr, N);
    }
}

static void
dexNotify (sim_cpu *cpu)
{
  /* instr[14,0] == type : 0 ==> method entry, 1 ==> method reentry
                           2 ==> exit Java, 3 ==> start next bytecode.  */
  uint32_t type = INSTR (14, 0);

  TRACE_EVENTS (cpu, "Notify Insn encountered, type = 0x%x", type);

  switch (type)
    {
    case 0:
      /* aarch64_notifyMethodEntry (aarch64_get_reg_u64 (cpu, R23, 0),
	 aarch64_get_reg_u64 (cpu, R22, 0));  */
      break;
    case 1:
      /* aarch64_notifyMethodReentry (aarch64_get_reg_u64 (cpu, R23, 0),
	 aarch64_get_reg_u64 (cpu, R22, 0));  */
      break;
    case 2:
      /* aarch64_notifyMethodExit ();  */
      break;
    case 3:
      /* aarch64_notifyBCStart (aarch64_get_reg_u64 (cpu, R23, 0),
	 aarch64_get_reg_u64 (cpu, R22, 0));  */
      break;
    }
}

/* secondary decode within top level groups  */

static void
dexPseudo (sim_cpu *cpu)
{
  /* assert instr[28,27] = 00

     We provide 2 pseudo instructions:

     HALT stops execution of the simulator causing an immediate
     return to the x86 code which entered it.

     CALLOUT initiates recursive entry into x86 code.  A register
     argument holds the address of the x86 routine.  Immediate
     values in the instruction identify the number of general
     purpose and floating point register arguments to be passed
     and the type of any value to be returned.  */

  uint32_t PSEUDO_HALT      =  0xE0000000U;
  uint32_t PSEUDO_CALLOUT   =  0x00018000U;
  uint32_t PSEUDO_CALLOUTR  =  0x00018001U;
  uint32_t PSEUDO_NOTIFY    =  0x00014000U;
  uint32_t dispatch;

  if (aarch64_get_instr (cpu) == PSEUDO_HALT)
    {
      TRACE_EVENTS (cpu, " Pseudo Halt Instruction");
      sim_engine_halt (CPU_STATE (cpu), cpu, NULL, aarch64_get_PC (cpu),
		       sim_stopped, SIM_SIGTRAP);
    }

  dispatch = INSTR (31, 15);

  /* We do not handle callouts at the moment.  */
  if (dispatch == PSEUDO_CALLOUT || dispatch == PSEUDO_CALLOUTR)
    {
      TRACE_EVENTS (cpu, " Callout");
      sim_engine_halt (CPU_STATE (cpu), cpu, NULL, aarch64_get_PC (cpu),
		       sim_stopped, SIM_SIGABRT);
    }

  else if (dispatch == PSEUDO_NOTIFY)
    dexNotify (cpu);

  else
    HALT_UNALLOC;
}

/* Load-store single register (unscaled offset)
   These instructions employ a base register plus an unscaled signed
   9 bit offset.

   N.B. the base register (source) can be Xn or SP. all other
   registers may not be SP.  */

/* 32 bit load 32 bit unscaled signed 9 bit.  */
static void
ldur32 (sim_cpu *cpu, int32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rt, NO_SP, aarch64_get_mem_u32
		       (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
			+ offset));
}

/* 64 bit load 64 bit unscaled signed 9 bit.  */
static void
ldur64 (sim_cpu *cpu, int32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rt, NO_SP, aarch64_get_mem_u64
		       (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
			+ offset));
}

/* 32 bit load zero-extended byte unscaled signed 9 bit.  */
static void
ldurb32 (sim_cpu *cpu, int32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rt, NO_SP, aarch64_get_mem_u8
		       (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
			+ offset));
}

/* 32 bit load sign-extended byte unscaled signed 9 bit.  */
static void
ldursb32 (sim_cpu *cpu, int32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rt, NO_SP, (uint32_t) aarch64_get_mem_s8
		       (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
			+ offset));
}

/* 64 bit load sign-extended byte unscaled signed 9 bit.  */
static void
ldursb64 (sim_cpu *cpu, int32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_s64 (cpu, rt, NO_SP, aarch64_get_mem_s8
		       (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
			+ offset));
}

/* 32 bit load zero-extended short unscaled signed 9 bit  */
static void
ldurh32 (sim_cpu *cpu, int32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, aarch64_get_mem_u16
		       (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
			+ offset));
}

/* 32 bit load sign-extended short unscaled signed 9 bit  */
static void
ldursh32 (sim_cpu *cpu, int32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, (uint32_t) aarch64_get_mem_s16
		       (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
			+ offset));
}

/* 64 bit load sign-extended short unscaled signed 9 bit  */
static void
ldursh64 (sim_cpu *cpu, int32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_s64 (cpu, rt, NO_SP, aarch64_get_mem_s16
		       (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
			+ offset));
}

/* 64 bit load sign-extended word unscaled signed 9 bit  */
static void
ldursw (sim_cpu *cpu, int32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, (uint32_t) aarch64_get_mem_s32
		       (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
			+ offset));
}

/* N.B. with stores the value in source is written to the address
   identified by source2 modified by offset.  */

/* 32 bit store 32 bit unscaled signed 9 bit.  */
static void
stur32 (sim_cpu *cpu, int32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_mem_u32 (cpu,
		       aarch64_get_reg_u64 (cpu, rn, SP_OK) + offset,
		       aarch64_get_reg_u32 (cpu, rd, NO_SP));
}

/* 64 bit store 64 bit unscaled signed 9 bit  */
static void
stur64 (sim_cpu *cpu, int32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_mem_u64 (cpu,
		       aarch64_get_reg_u64 (cpu, rn, SP_OK) + offset,
		       aarch64_get_reg_u64 (cpu, rd, NO_SP));
}

/* 32 bit store byte unscaled signed 9 bit  */
static void
sturb (sim_cpu *cpu, int32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_mem_u8 (cpu,
		      aarch64_get_reg_u64 (cpu, rn, SP_OK) + offset,
		      aarch64_get_reg_u8 (cpu, rd, NO_SP));
}

/* 32 bit store short unscaled signed 9 bit  */
static void
sturh (sim_cpu *cpu, int32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_mem_u16 (cpu,
		       aarch64_get_reg_u64 (cpu, rn, SP_OK) + offset,
		       aarch64_get_reg_u16 (cpu, rd, NO_SP));
}

/* Load single register pc-relative label
   Offset is a signed 19 bit immediate count in words
   rt may not be SP.  */

/* 32 bit pc-relative load  */
static void
ldr32_pcrel (sim_cpu *cpu, int32_t offset)
{
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       aarch64_get_mem_u32
		       (cpu, aarch64_get_PC (cpu) + offset * 4));
}

/* 64 bit pc-relative load  */
static void
ldr_pcrel (sim_cpu *cpu, int32_t offset)
{
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       aarch64_get_mem_u64
		       (cpu, aarch64_get_PC (cpu) + offset * 4));
}

/* sign extended 32 bit pc-relative load  */
static void
ldrsw_pcrel (sim_cpu *cpu, int32_t offset)
{
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       aarch64_get_mem_s32
		       (cpu, aarch64_get_PC (cpu) + offset * 4));
}

/* float pc-relative load  */
static void
fldrs_pcrel (sim_cpu *cpu, int32_t offset)
{
  unsigned int rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u32 (cpu, rd, 0,
		       aarch64_get_mem_u32
		       (cpu, aarch64_get_PC (cpu) + offset * 4));
}

/* double pc-relative load  */
static void
fldrd_pcrel (sim_cpu *cpu, int32_t offset)
{
  unsigned int st = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u64 (cpu, st, 0,
		       aarch64_get_mem_u64
		       (cpu, aarch64_get_PC (cpu) + offset * 4));
}

/* long double pc-relative load.  */
static void
fldrq_pcrel (sim_cpu *cpu, int32_t offset)
{
  unsigned int st = INSTR (4, 0);
  uint64_t addr = aarch64_get_PC (cpu) + offset * 4;
  FRegister a;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_get_mem_long_double (cpu, addr, & a);
  aarch64_set_FP_long_double (cpu, st, a);
}

/* This can be used to scale an offset by applying
   the requisite shift. the second argument is either
   16, 32 or 64.  */

#define SCALE(_offset, _elementSize) \
    ((_offset) << ScaleShift ## _elementSize)

/* This can be used to optionally scale a register derived offset
   by applying the requisite shift as indicated by the Scaling
   argument.  The second argument is either Byte, Short, Word
   or Long. The third argument is either Scaled or Unscaled.
   N.B. when _Scaling is Scaled the shift gets ANDed with
   all 1s while when it is Unscaled it gets ANDed with 0.  */

#define OPT_SCALE(_offset, _elementType, _Scaling) \
  ((_offset) << (_Scaling ? ScaleShift ## _elementType : 0))

/* This can be used to zero or sign extend a 32 bit register derived
   value to a 64 bit value.  the first argument must be the value as
   a uint32_t and the second must be either UXTW or SXTW. The result
   is returned as an int64_t.  */

static inline int64_t
extend (uint32_t value, Extension extension)
{
  union
  {
    uint32_t u;
    int32_t   n;
  } x;

  /* A branchless variant of this ought to be possible.  */
  if (extension == UXTW || extension == NoExtension)
    return value;

  x.u = value;
  return x.n;
}

/* Scalar Floating Point

   FP load/store single register (4 addressing modes)

   N.B. the base register (source) can be the stack pointer.
   The secondary source register (source2) can only be an Xn register.  */

/* Load 32 bit unscaled signed 9 bit with pre- or post-writeback.  */
static void
fldrs_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned st = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u32 (cpu, st, 0, aarch64_get_mem_u32 (cpu, address));
  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* Load 8 bit with unsigned 12 bit offset.  */
static void
fldrb_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rd = INSTR (4, 0);
  unsigned rn = INSTR (9, 5);
  uint64_t addr = aarch64_get_reg_u64 (cpu, rn, SP_OK) + offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u8 (cpu, rd, 0, aarch64_get_mem_u32 (cpu, addr));
}

/* Load 16 bit scaled unsigned 12 bit.  */
static void
fldrh_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rd = INSTR (4, 0);
  unsigned rn = INSTR (9, 5);
  uint64_t addr = aarch64_get_reg_u64 (cpu, rn, SP_OK) + SCALE (offset, 16);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u16 (cpu, rd, 0, aarch64_get_mem_u16 (cpu, addr));
}

/* Load 32 bit scaled unsigned 12 bit.  */
static void
fldrs_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rd = INSTR (4, 0);
  unsigned rn = INSTR (9, 5);
  uint64_t addr = aarch64_get_reg_u64 (cpu, rn, SP_OK) + SCALE (offset, 32);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u32 (cpu, rd, 0, aarch64_get_mem_u32 (cpu, addr));
}

/* Load 64 bit scaled unsigned 12 bit.  */
static void
fldrd_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rd = INSTR (4, 0);
  unsigned rn = INSTR (9, 5);
  uint64_t addr = aarch64_get_reg_u64 (cpu, rn, SP_OK) + SCALE (offset, 64);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u64 (cpu, rd, 0, aarch64_get_mem_u64 (cpu, addr));
}

/* Load 128 bit scaled unsigned 12 bit.  */
static void
fldrq_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rd = INSTR (4, 0);
  unsigned rn = INSTR (9, 5);
  uint64_t addr = aarch64_get_reg_u64 (cpu, rn, SP_OK) + SCALE (offset, 128);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u64 (cpu, rd, 0, aarch64_get_mem_u64 (cpu, addr));
  aarch64_set_vec_u64 (cpu, rd, 1, aarch64_get_mem_u64 (cpu, addr + 8));
}

/* Load 32 bit scaled or unscaled zero- or sign-extended
   32-bit register offset.  */
static void
fldrs_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned st = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP), extension);
  uint64_t displacement = OPT_SCALE (extended, 32, scaling);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u32 (cpu, st, 0, aarch64_get_mem_u32
		       (cpu, address + displacement));
}

/* Load 64 bit unscaled signed 9 bit with pre- or post-writeback.  */
static void
fldrd_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned st = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u64 (cpu, st, 0, aarch64_get_mem_u64 (cpu, address));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* Load 64 bit scaled or unscaled zero- or sign-extended 32-bit register offset.  */
static void
fldrd_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  int64_t extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP), extension);
  uint64_t displacement = OPT_SCALE (extended, 64, scaling);

  fldrd_wb (cpu, displacement, NoWriteBack);
}

/* Load 128 bit unscaled signed 9 bit with pre- or post-writeback.  */
static void
fldrq_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  FRegister a;
  unsigned rn = INSTR (9, 5);
  unsigned st = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_get_mem_long_double (cpu, address, & a);
  aarch64_set_FP_long_double (cpu, st, a);

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* Load 128 bit scaled or unscaled zero- or sign-extended 32-bit register offset  */
static void
fldrq_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  int64_t extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP), extension);
  uint64_t displacement = OPT_SCALE (extended, 128, scaling);

  fldrq_wb (cpu, displacement, NoWriteBack);
}

/* Memory Access

   load-store single register
   There are four addressing modes available here which all employ a
   64 bit source (base) register.

   N.B. the base register (source) can be the stack pointer.
   The secondary source register (source2)can only be an Xn register.

   Scaled, 12-bit, unsigned immediate offset, without pre- and
   post-index options.
   Unscaled, 9-bit, signed immediate offset with pre- or post-index
   writeback.
   scaled or unscaled 64-bit register offset.
   scaled or unscaled 32-bit extended register offset.

   All offsets are assumed to be raw from the decode i.e. the
   simulator is expected to adjust scaled offsets based on the
   accessed data size with register or extended register offset
   versions the same applies except that in the latter case the
   operation may also require a sign extend.

   A separate method is provided for each possible addressing mode.  */

/* 32 bit load 32 bit scaled unsigned 12 bit  */
static void
ldr32_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* The target register may not be SP but the source may be.  */
  aarch64_set_reg_u64 (cpu, rt, NO_SP, aarch64_get_mem_u32
		       (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
			+ SCALE (offset, 32)));
}

/* 32 bit load 32 bit unscaled signed 9 bit with pre- or post-writeback.  */
static void
ldr32_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  uint64_t address;

  if (rn == rt && wb != NoWriteBack)
    HALT_UNALLOC;

  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rt, NO_SP, aarch64_get_mem_u32 (cpu, address));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* 32 bit load 32 bit scaled or unscaled
   zero- or sign-extended 32-bit register offset  */
static void
ldr32_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  /* rn may reference SP, rm and rt must reference ZR  */

  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP), extension);
  uint64_t displacement =  OPT_SCALE (extended, 32, scaling);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rt, NO_SP,
		       aarch64_get_mem_u32 (cpu, address + displacement));
}

/* 64 bit load 64 bit scaled unsigned 12 bit  */
static void
ldr_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* The target register may not be SP but the source may be.  */
  aarch64_set_reg_u64 (cpu, rt, NO_SP, aarch64_get_mem_u64
		       (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
			+ SCALE (offset, 64)));
}

/* 64 bit load 64 bit unscaled signed 9 bit with pre- or post-writeback.  */
static void
ldr_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  uint64_t address;

  if (rn == rt && wb != NoWriteBack)
    HALT_UNALLOC;

  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rt, NO_SP, aarch64_get_mem_u64 (cpu, address));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* 64 bit load 64 bit scaled or unscaled zero-
   or sign-extended 32-bit register offset.  */
static void
ldr_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  /* rn may reference SP, rm and rt must reference ZR  */

  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP), extension);
  uint64_t displacement =  OPT_SCALE (extended, 64, scaling);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rt, NO_SP,
		       aarch64_get_mem_u64 (cpu, address + displacement));
}

/* 32 bit load zero-extended byte scaled unsigned 12 bit.  */
static void
ldrb32_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* The target register may not be SP but the source may be
     there is no scaling required for a byte load.  */
  aarch64_set_reg_u64 (cpu, rt, NO_SP,
		       aarch64_get_mem_u8
		       (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK) + offset));
}

/* 32 bit load zero-extended byte unscaled signed 9 bit with pre- or post-writeback.  */
static void
ldrb32_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  uint64_t address;

  if (rn == rt && wb != NoWriteBack)
    HALT_UNALLOC;

  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rt, NO_SP, aarch64_get_mem_u8 (cpu, address));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* 32 bit load zero-extended byte scaled or unscaled zero-
   or sign-extended 32-bit register offset.  */
static void
ldrb32_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  /* rn may reference SP, rm and rt must reference ZR  */

  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t displacement = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP),
				 extension);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* There is no scaling required for a byte load.  */
  aarch64_set_reg_u64 (cpu, rt, NO_SP,
		       aarch64_get_mem_u8 (cpu, address + displacement));
}

/* 64 bit load sign-extended byte unscaled signed 9 bit
   with pre- or post-writeback.  */
static void
ldrsb_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  uint64_t address;
  int64_t val;

  if (rn == rt && wb != NoWriteBack)
    HALT_UNALLOC;

  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  val = aarch64_get_mem_s8 (cpu, address);
  aarch64_set_reg_s64 (cpu, rt, NO_SP, val);

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* 64 bit load sign-extended byte scaled unsigned 12 bit.  */
static void
ldrsb_abs (sim_cpu *cpu, uint32_t offset)
{
  ldrsb_wb (cpu, offset, NoWriteBack);
}

/* 64 bit load sign-extended byte scaled or unscaled zero-
   or sign-extended 32-bit register offset.  */
static void
ldrsb_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  /* rn may reference SP, rm and rt must reference ZR  */

  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t displacement = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP),
				 extension);
  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* There is no scaling required for a byte load.  */
  aarch64_set_reg_s64 (cpu, rt, NO_SP,
		       aarch64_get_mem_s8 (cpu, address + displacement));
}

/* 32 bit load zero-extended short scaled unsigned 12 bit.  */
static void
ldrh32_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  uint32_t val;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* The target register may not be SP but the source may be.  */
  val = aarch64_get_mem_u16 (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
			     + SCALE (offset, 16));
  aarch64_set_reg_u32 (cpu, rt, NO_SP, val);
}

/* 32 bit load zero-extended short unscaled signed 9 bit
   with pre- or post-writeback.  */
static void
ldrh32_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  uint64_t address;

  if (rn == rt && wb != NoWriteBack)
    HALT_UNALLOC;

  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u32 (cpu, rt, NO_SP, aarch64_get_mem_u16 (cpu, address));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* 32 bit load zero-extended short scaled or unscaled zero-
   or sign-extended 32-bit register offset.  */
static void
ldrh32_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  /* rn may reference SP, rm and rt must reference ZR  */

  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP), extension);
  uint64_t displacement =  OPT_SCALE (extended, 16, scaling);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u32 (cpu, rt, NO_SP,
		       aarch64_get_mem_u16 (cpu, address + displacement));
}

/* 32 bit load sign-extended short scaled unsigned 12 bit.  */
static void
ldrsh32_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  int32_t val;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* The target register may not be SP but the source may be.  */
  val = aarch64_get_mem_s16 (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
			     + SCALE (offset, 16));
  aarch64_set_reg_s32 (cpu, rt, NO_SP, val);
}

/* 32 bit load sign-extended short unscaled signed 9 bit
   with pre- or post-writeback.  */
static void
ldrsh32_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  uint64_t address;

  if (rn == rt && wb != NoWriteBack)
    HALT_UNALLOC;

  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_s32 (cpu, rt, NO_SP,
		       (int32_t) aarch64_get_mem_s16 (cpu, address));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* 32 bit load sign-extended short scaled or unscaled zero-
   or sign-extended 32-bit register offset.  */
static void
ldrsh32_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  /* rn may reference SP, rm and rt must reference ZR  */

  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP), extension);
  uint64_t displacement =  OPT_SCALE (extended, 16, scaling);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_s32 (cpu, rt, NO_SP,
		       (int32_t) aarch64_get_mem_s16
		       (cpu, address + displacement));
}

/* 64 bit load sign-extended short scaled unsigned 12 bit.  */
static void
ldrsh_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  int64_t val;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* The target register may not be SP but the source may be.  */
  val = aarch64_get_mem_s16  (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
			      + SCALE (offset, 16));
  aarch64_set_reg_s64 (cpu, rt, NO_SP, val);
}

/* 64 bit load sign-extended short unscaled signed 9 bit
   with pre- or post-writeback.  */
static void
ldrsh64_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  uint64_t address;
  int64_t val;

  if (rn == rt && wb != NoWriteBack)
    HALT_UNALLOC;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  val = aarch64_get_mem_s16 (cpu, address);
  aarch64_set_reg_s64 (cpu, rt, NO_SP, val);

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* 64 bit load sign-extended short scaled or unscaled zero-
   or sign-extended 32-bit register offset.  */
static void
ldrsh_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);

  /* rn may reference SP, rm and rt must reference ZR  */

  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP), extension);
  uint64_t displacement = OPT_SCALE (extended, 16, scaling);
  int64_t val;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  val = aarch64_get_mem_s16 (cpu, address + displacement);
  aarch64_set_reg_s64 (cpu, rt, NO_SP, val);
}

/* 64 bit load sign-extended 32 bit scaled unsigned 12 bit.  */
static void
ldrsw_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  int64_t val;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  val = aarch64_get_mem_s32 (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
			     + SCALE (offset, 32));
  /* The target register may not be SP but the source may be.  */
  return aarch64_set_reg_s64 (cpu, rt, NO_SP, val);
}

/* 64 bit load sign-extended 32 bit unscaled signed 9 bit
   with pre- or post-writeback.  */
static void
ldrsw_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  uint64_t address;

  if (rn == rt && wb != NoWriteBack)
    HALT_UNALLOC;

  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_s64 (cpu, rt, NO_SP, aarch64_get_mem_s32 (cpu, address));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* 64 bit load sign-extended 32 bit scaled or unscaled zero-
   or sign-extended 32-bit register offset.  */
static void
ldrsw_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  /* rn may reference SP, rm and rt must reference ZR  */

  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP), extension);
  uint64_t displacement =  OPT_SCALE (extended, 32, scaling);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_s64 (cpu, rt, NO_SP,
		       aarch64_get_mem_s32 (cpu, address + displacement));
}

/* N.B. with stores the value in source is written to the
   address identified by source2 modified by source3/offset.  */

/* 32 bit store scaled unsigned 12 bit.  */
static void
str32_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* The target register may not be SP but the source may be.  */
  aarch64_set_mem_u32 (cpu, (aarch64_get_reg_u64 (cpu, rn, SP_OK)
			     + SCALE (offset, 32)),
		       aarch64_get_reg_u32 (cpu, rt, NO_SP));
}

/* 32 bit store unscaled signed 9 bit with pre- or post-writeback.  */
static void
str32_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  uint64_t address;

  if (rn == rt && wb != NoWriteBack)
    HALT_UNALLOC;

  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  if (wb != Post)
    address += offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_mem_u32 (cpu, address, aarch64_get_reg_u32 (cpu, rt, NO_SP));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* 32 bit store scaled or unscaled zero- or
   sign-extended 32-bit register offset.  */
static void
str32_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);

  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t  extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP), extension);
  uint64_t displacement = OPT_SCALE (extended, 32, scaling);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_mem_u32 (cpu, address + displacement,
		       aarch64_get_reg_u64 (cpu, rt, NO_SP));
}

/* 64 bit store scaled unsigned 12 bit.  */
static void
str_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_mem_u64 (cpu,
		       aarch64_get_reg_u64 (cpu, rn, SP_OK)
		       + SCALE (offset, 64),
		       aarch64_get_reg_u64 (cpu, rt, NO_SP));
}

/* 64 bit store unscaled signed 9 bit with pre- or post-writeback.  */
static void
str_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  uint64_t address;

  if (rn == rt && wb != NoWriteBack)
    HALT_UNALLOC;

  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_mem_u64 (cpu, address, aarch64_get_reg_u64 (cpu, rt, NO_SP));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* 64 bit store scaled or unscaled zero-
   or sign-extended 32-bit register offset.  */
static void
str_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  /* rn may reference SP, rm and rt must reference ZR  */

  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t   extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP),
			       extension);
  uint64_t displacement = OPT_SCALE (extended, 64, scaling);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_mem_u64 (cpu, address + displacement,
		       aarch64_get_reg_u64 (cpu, rt, NO_SP));
}

/* 32 bit store byte scaled unsigned 12 bit.  */
static void
strb_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* The target register may not be SP but the source may be.
     There is no scaling required for a byte load.  */
  aarch64_set_mem_u8 (cpu,
		      aarch64_get_reg_u64 (cpu, rn, SP_OK) + offset,
		      aarch64_get_reg_u8 (cpu, rt, NO_SP));
}

/* 32 bit store byte unscaled signed 9 bit with pre- or post-writeback.  */
static void
strb_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  uint64_t address;

  if (rn == rt && wb != NoWriteBack)
    HALT_UNALLOC;

  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_mem_u8 (cpu, address, aarch64_get_reg_u8 (cpu, rt, NO_SP));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* 32 bit store byte scaled or unscaled zero-
   or sign-extended 32-bit register offset.  */
static void
strb_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  /* rn may reference SP, rm and rt must reference ZR  */

  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t displacement = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP),
				 extension);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* There is no scaling required for a byte load.  */
  aarch64_set_mem_u8 (cpu, address + displacement,
		      aarch64_get_reg_u8 (cpu, rt, NO_SP));
}

/* 32 bit store short scaled unsigned 12 bit.  */
static void
strh_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* The target register may not be SP but the source may be.  */
  aarch64_set_mem_u16 (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK)
		       + SCALE (offset, 16),
		       aarch64_get_reg_u16 (cpu, rt, NO_SP));
}

/* 32 bit store short unscaled signed 9 bit with pre- or post-writeback.  */
static void
strh_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  uint64_t address;

  if (rn == rt && wb != NoWriteBack)
    HALT_UNALLOC;

  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_mem_u16 (cpu, address, aarch64_get_reg_u16 (cpu, rt, NO_SP));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* 32 bit store short scaled or unscaled zero-
   or sign-extended 32-bit register offset.  */
static void
strh_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  /* rn may reference SP, rm and rt must reference ZR  */

  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP), extension);
  uint64_t displacement =  OPT_SCALE (extended, 16, scaling);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_mem_u16 (cpu, address + displacement,
		       aarch64_get_reg_u16 (cpu, rt, NO_SP));
}

/* Prefetch unsigned 12 bit.  */
static void
prfm_abs (sim_cpu *cpu, uint32_t offset)
{
  /* instr[4,0] = prfop : 00000 ==> PLDL1KEEP, 00001 ==> PLDL1STRM,
                          00010 ==> PLDL2KEEP, 00001 ==> PLDL2STRM,
                          00100 ==> PLDL3KEEP, 00101 ==> PLDL3STRM,
                          10000 ==> PSTL1KEEP, 10001 ==> PSTL1STRM,
                          10010 ==> PSTL2KEEP, 10001 ==> PSTL2STRM,
                          10100 ==> PSTL3KEEP, 10101 ==> PSTL3STRM,
                          ow ==> UNALLOC
     PrfOp prfop = prfop (instr, 4, 0);
     uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK)
     + SCALE (offset, 64).  */

  /* TODO : implement prefetch of address.  */
}

/* Prefetch scaled or unscaled zero- or sign-extended 32-bit register offset.  */
static void
prfm_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  /* instr[4,0] = prfop : 00000 ==> PLDL1KEEP, 00001 ==> PLDL1STRM,
                          00010 ==> PLDL2KEEP, 00001 ==> PLDL2STRM,
                          00100 ==> PLDL3KEEP, 00101 ==> PLDL3STRM,
                          10000 ==> PSTL1KEEP, 10001 ==> PSTL1STRM,
                          10010 ==> PSTL2KEEP, 10001 ==> PSTL2STRM,
                          10100 ==> PSTL3KEEP, 10101 ==> PSTL3STRM,
                          ow ==> UNALLOC
     rn may reference SP, rm may only reference ZR
     PrfOp prfop = prfop (instr, 4, 0);
     uint64_t base = aarch64_get_reg_u64 (cpu, rn, SP_OK);
     int64_t extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP),
                                extension);
     uint64_t displacement =  OPT_SCALE (extended, 64, scaling);
     uint64_t address = base + displacement.  */

  /* TODO : implement prefetch of address  */
}

/* 64 bit pc-relative prefetch.  */
static void
prfm_pcrel (sim_cpu *cpu, int32_t offset)
{
  /* instr[4,0] = prfop : 00000 ==> PLDL1KEEP, 00001 ==> PLDL1STRM,
                          00010 ==> PLDL2KEEP, 00001 ==> PLDL2STRM,
                          00100 ==> PLDL3KEEP, 00101 ==> PLDL3STRM,
                          10000 ==> PSTL1KEEP, 10001 ==> PSTL1STRM,
                          10010 ==> PSTL2KEEP, 10001 ==> PSTL2STRM,
                          10100 ==> PSTL3KEEP, 10101 ==> PSTL3STRM,
                          ow ==> UNALLOC
     PrfOp prfop = prfop (instr, 4, 0);
     uint64_t address = aarch64_get_PC (cpu) + offset.  */

  /* TODO : implement this  */
}

/* Load-store exclusive.  */

static void
ldxr (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int size = INSTR (31, 30);
  /* int ordered = INSTR (15, 15);  */
  /* int exclusive = ! INSTR (23, 23);  */

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (size)
    {
    case 0:
      aarch64_set_reg_u64 (cpu, rt, NO_SP, aarch64_get_mem_u8 (cpu, address));
      break;
    case 1:
      aarch64_set_reg_u64 (cpu, rt, NO_SP, aarch64_get_mem_u16 (cpu, address));
      break;
    case 2:
      aarch64_set_reg_u64 (cpu, rt, NO_SP, aarch64_get_mem_u32 (cpu, address));
      break;
    case 3:
      aarch64_set_reg_u64 (cpu, rt, NO_SP, aarch64_get_mem_u64 (cpu, address));
      break;
    }
}

static void
stxr (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rt = INSTR (4, 0);
  unsigned rs = INSTR (20, 16);
  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int      size = INSTR (31, 30);
  uint64_t data = aarch64_get_reg_u64 (cpu, rt, NO_SP);

  switch (size)
    {
    case 0: aarch64_set_mem_u8 (cpu, address, data); break;
    case 1: aarch64_set_mem_u16 (cpu, address, data); break;
    case 2: aarch64_set_mem_u32 (cpu, address, data); break;
    case 3: aarch64_set_mem_u64 (cpu, address, data); break;
    }

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rs, NO_SP, 0); /* Always exclusive...  */
}

static void
dexLoadLiteral (sim_cpu *cpu)
{
  /* instr[29,27] == 011
     instr[25,24] == 00
     instr[31,30:26] = opc: 000 ==> LDRW,  001 ==> FLDRS
                            010 ==> LDRX,  011 ==> FLDRD
                            100 ==> LDRSW, 101 ==> FLDRQ
                            110 ==> PRFM, 111 ==> UNALLOC
     instr[26] ==> V : 0 ==> GReg, 1 ==> FReg
     instr[23, 5] == simm19  */

  /* unsigned rt = INSTR (4, 0);  */
  uint32_t dispatch = (INSTR (31, 30) << 1) | INSTR (26, 26);
  int32_t imm = simm32 (aarch64_get_instr (cpu), 23, 5);

  switch (dispatch)
    {
    case 0: ldr32_pcrel (cpu, imm); break;
    case 1: fldrs_pcrel (cpu, imm); break;
    case 2: ldr_pcrel   (cpu, imm); break;
    case 3: fldrd_pcrel (cpu, imm); break;
    case 4: ldrsw_pcrel (cpu, imm); break;
    case 5: fldrq_pcrel (cpu, imm); break;
    case 6: prfm_pcrel  (cpu, imm); break;
    case 7:
    default:
      HALT_UNALLOC;
    }
}

/* Immediate arithmetic
   The aimm argument is a 12 bit unsigned value or a 12 bit unsigned
   value left shifted by 12 bits (done at decode).

   N.B. the register args (dest, source) can normally be Xn or SP.
   the exception occurs for flag setting instructions which may
   only use Xn for the output (dest).  */

/* 32 bit add immediate.  */
static void
add32 (sim_cpu *cpu, uint32_t aimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, SP_OK,
		       aarch64_get_reg_u32 (cpu, rn, SP_OK) + aimm);
}

/* 64 bit add immediate.  */
static void
add64 (sim_cpu *cpu, uint32_t aimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, SP_OK,
		       aarch64_get_reg_u64 (cpu, rn, SP_OK) + aimm);
}

static void
set_flags_for_add32 (sim_cpu *cpu, int32_t value1, int32_t value2)
{
  int32_t   result = value1 + value2;
  int64_t   sresult = (int64_t) value1 + (int64_t) value2;
  uint64_t  uresult = (uint64_t)(uint32_t) value1
    + (uint64_t)(uint32_t) value2;
  uint32_t  flags = 0;

  if (result == 0)
    flags |= Z;

  if (result & (1 << 31))
    flags |= N;

  if (uresult != (uint32_t)uresult)
    flags |= C;

  if (sresult != (int32_t)sresult)
    flags |= V;

  aarch64_set_CPSR (cpu, flags);
}

#define NEG(a) (((a) & signbit) == signbit)
#define POS(a) (((a) & signbit) == 0)

static void
set_flags_for_add64 (sim_cpu *cpu, uint64_t value1, uint64_t value2)
{
  uint64_t result = value1 + value2;
  uint32_t flags = 0;
  uint64_t signbit = 1ULL << 63;

  if (result == 0)
    flags |= Z;

  if (NEG (result))
    flags |= N;

  if (   (NEG (value1) && NEG (value2))
      || (NEG (value1) && POS (result))
      || (NEG (value2) && POS (result)))
    flags |= C;

  if (   (NEG (value1) && NEG (value2) && POS (result))
      || (POS (value1) && POS (value2) && NEG (result)))
    flags |= V;

  aarch64_set_CPSR (cpu, flags);
}

static void
set_flags_for_sub32 (sim_cpu *cpu, uint32_t value1, uint32_t value2)
{
  uint32_t result = value1 - value2;
  uint32_t flags = 0;
  uint32_t signbit = 1U << 31;

  if (result == 0)
    flags |= Z;

  if (NEG (result))
    flags |= N;

  if (   (NEG (value1) && POS (value2))
      || (NEG (value1) && POS (result))
      || (POS (value2) && POS (result)))
    flags |= C;

  if (   (NEG (value1) && POS (value2) && POS (result))
      || (POS (value1) && NEG (value2) && NEG (result)))
    flags |= V;

  aarch64_set_CPSR (cpu, flags);
}

static void
set_flags_for_sub64 (sim_cpu *cpu, uint64_t value1, uint64_t value2)
{
  uint64_t result = value1 - value2;
  uint32_t flags = 0;
  uint64_t signbit = 1ULL << 63;

  if (result == 0)
    flags |= Z;

  if (NEG (result))
    flags |= N;

  if (   (NEG (value1) && POS (value2))
      || (NEG (value1) && POS (result))
      || (POS (value2) && POS (result)))
    flags |= C;

  if (   (NEG (value1) && POS (value2) && POS (result))
      || (POS (value1) && NEG (value2) && NEG (result)))
    flags |= V;

  aarch64_set_CPSR (cpu, flags);
}

static void
set_flags_for_binop32 (sim_cpu *cpu, uint32_t result)
{
  uint32_t flags = 0;

  if (result == 0)
    flags |= Z;
  else
    flags &= ~ Z;

  if (result & (1 << 31))
    flags |= N;
  else
    flags &= ~ N;

  aarch64_set_CPSR (cpu, flags);
}

static void
set_flags_for_binop64 (sim_cpu *cpu, uint64_t result)
{
  uint32_t flags = 0;

  if (result == 0)
    flags |= Z;
  else
    flags &= ~ Z;

  if (result & (1ULL << 63))
    flags |= N;
  else
    flags &= ~ N;

  aarch64_set_CPSR (cpu, flags);
}

/* 32 bit add immediate set flags.  */
static void
adds32 (sim_cpu *cpu, uint32_t aimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  /* TODO : do we need to worry about signs here?  */
  int32_t value1 = aarch64_get_reg_s32 (cpu, rn, SP_OK);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 + aimm);
  set_flags_for_add32 (cpu, value1, aimm);
}

/* 64 bit add immediate set flags.  */
static void
adds64 (sim_cpu *cpu, uint32_t aimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  uint64_t value1 = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  uint64_t value2 = aimm;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 + value2);
  set_flags_for_add64 (cpu, value1, value2);
}

/* 32 bit sub immediate.  */
static void
sub32 (sim_cpu *cpu, uint32_t aimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, SP_OK,
		       aarch64_get_reg_u32 (cpu, rn, SP_OK) - aimm);
}

/* 64 bit sub immediate.  */
static void
sub64 (sim_cpu *cpu, uint32_t aimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, SP_OK,
		       aarch64_get_reg_u64 (cpu, rn, SP_OK) - aimm);
}

/* 32 bit sub immediate set flags.  */
static void
subs32 (sim_cpu *cpu, uint32_t aimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  uint32_t value1 = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  uint32_t value2 = aimm;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 - value2);
  set_flags_for_sub32 (cpu, value1, value2);
}

/* 64 bit sub immediate set flags.  */
static void
subs64 (sim_cpu *cpu, uint32_t aimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  uint64_t value1 = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  uint32_t value2 = aimm;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 - value2);
  set_flags_for_sub64 (cpu, value1, value2);
}

/* Data Processing Register.  */

/* First two helpers to perform the shift operations.  */

static inline uint32_t
shifted32 (uint32_t value, Shift shift, uint32_t count)
{
  switch (shift)
    {
    default:
    case LSL:
      return (value << count);
    case LSR:
      return (value >> count);
    case ASR:
      {
	int32_t svalue = value;
	return (svalue >> count);
      }
    case ROR:
      {
	uint32_t top = value >> count;
	uint32_t bottom = value << (32 - count);
	return (bottom | top);
      }
    }
}

static inline uint64_t
shifted64 (uint64_t value, Shift shift, uint32_t count)
{
  switch (shift)
    {
    default:
    case LSL:
      return (value << count);
    case LSR:
      return (value >> count);
    case ASR:
      {
	int64_t svalue = value;
	return (svalue >> count);
      }
    case ROR:
      {
	uint64_t top = value >> count;
	uint64_t bottom = value << (64 - count);
	return (bottom | top);
      }
    }
}

/* Arithmetic shifted register.
   These allow an optional LSL, ASR or LSR to the second source
   register with a count up to the register bit count.

   N.B register args may not be SP.  */

/* 32 bit ADD shifted register.  */
static void
add32_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       aarch64_get_reg_u32 (cpu, rn, NO_SP)
		       + shifted32 (aarch64_get_reg_u32 (cpu, rm, NO_SP),
				    shift, count));
}

/* 64 bit ADD shifted register.  */
static void
add64_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       aarch64_get_reg_u64 (cpu, rn, NO_SP)
		       + shifted64 (aarch64_get_reg_u64 (cpu, rm, NO_SP),
				    shift, count));
}

/* 32 bit ADD shifted register setting flags.  */
static void
adds32_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint32_t value1 = aarch64_get_reg_u32 (cpu, rn, NO_SP);
  uint32_t value2 = shifted32 (aarch64_get_reg_u32 (cpu, rm, NO_SP),
			       shift, count);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 + value2);
  set_flags_for_add32 (cpu, value1, value2);
}

/* 64 bit ADD shifted register setting flags.  */
static void
adds64_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint64_t value1 = aarch64_get_reg_u64 (cpu, rn, NO_SP);
  uint64_t value2 = shifted64 (aarch64_get_reg_u64 (cpu, rm, NO_SP),
			       shift, count);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 + value2);
  set_flags_for_add64 (cpu, value1, value2);
}

/* 32 bit SUB shifted register.  */
static void
sub32_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       aarch64_get_reg_u32 (cpu, rn, NO_SP)
		       - shifted32 (aarch64_get_reg_u32 (cpu, rm, NO_SP),
				    shift, count));
}

/* 64 bit SUB shifted register.  */
static void
sub64_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       aarch64_get_reg_u64 (cpu, rn, NO_SP)
		       - shifted64 (aarch64_get_reg_u64 (cpu, rm, NO_SP),
				    shift, count));
}

/* 32 bit SUB shifted register setting flags.  */
static void
subs32_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint32_t value1 = aarch64_get_reg_u32 (cpu, rn, NO_SP);
  uint32_t value2 = shifted32 (aarch64_get_reg_u32 (cpu, rm, NO_SP),
			      shift, count);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 - value2);
  set_flags_for_sub32 (cpu, value1, value2);
}

/* 64 bit SUB shifted register setting flags.  */
static void
subs64_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint64_t value1 = aarch64_get_reg_u64 (cpu, rn, NO_SP);
  uint64_t value2 = shifted64 (aarch64_get_reg_u64 (cpu, rm, NO_SP),
			       shift, count);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 - value2);
  set_flags_for_sub64 (cpu, value1, value2);
}

/* First a couple more helpers to fetch the
   relevant source register element either
   sign or zero extended as required by the
   extension value.  */

static uint32_t
extreg32 (sim_cpu *cpu, unsigned int lo, Extension extension)
{
  switch (extension)
    {
    case UXTB: return aarch64_get_reg_u8  (cpu, lo, NO_SP);
    case UXTH: return aarch64_get_reg_u16 (cpu, lo, NO_SP);
    case UXTW: ATTRIBUTE_FALLTHROUGH;
    case UXTX: return aarch64_get_reg_u32 (cpu, lo, NO_SP);
    case SXTB: return aarch64_get_reg_s8  (cpu, lo, NO_SP);
    case SXTH: return aarch64_get_reg_s16 (cpu, lo, NO_SP);
    case SXTW: ATTRIBUTE_FALLTHROUGH;
    case SXTX: ATTRIBUTE_FALLTHROUGH;
    default:   return aarch64_get_reg_s32 (cpu, lo, NO_SP);
  }
}

static uint64_t
extreg64 (sim_cpu *cpu, unsigned int lo, Extension extension)
{
  switch (extension)
    {
    case UXTB: return aarch64_get_reg_u8  (cpu, lo, NO_SP);
    case UXTH: return aarch64_get_reg_u16 (cpu, lo, NO_SP);
    case UXTW: return aarch64_get_reg_u32 (cpu, lo, NO_SP);
    case UXTX: return aarch64_get_reg_u64 (cpu, lo, NO_SP);
    case SXTB: return aarch64_get_reg_s8  (cpu, lo, NO_SP);
    case SXTH: return aarch64_get_reg_s16 (cpu, lo, NO_SP);
    case SXTW: return aarch64_get_reg_s32 (cpu, lo, NO_SP);
    case SXTX:
    default:   return aarch64_get_reg_s64 (cpu, lo, NO_SP);
    }
}

/* Arithmetic extending register
   These allow an optional sign extension of some portion of the
   second source register followed by an optional left shift of
   between 1 and 4 bits (i.e. a shift of 0-4 bits???)

   N.B output (dest) and first input arg (source) may normally be Xn
   or SP. However, for flag setting operations dest can only be
   Xn. Second input registers are always Xn.  */

/* 32 bit ADD extending register.  */
static void
add32_ext (sim_cpu *cpu, Extension extension, uint32_t shift)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, SP_OK,
		       aarch64_get_reg_u32 (cpu, rn, SP_OK)
		       + (extreg32 (cpu, rm, extension) << shift));
}

/* 64 bit ADD extending register.
   N.B. This subsumes the case with 64 bit source2 and UXTX #n or LSL #0.  */
static void
add64_ext (sim_cpu *cpu, Extension extension, uint32_t shift)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, SP_OK,
		       aarch64_get_reg_u64 (cpu, rn, SP_OK)
		       + (extreg64 (cpu, rm, extension) << shift));
}

/* 32 bit ADD extending register setting flags.  */
static void
adds32_ext (sim_cpu *cpu, Extension extension, uint32_t shift)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint32_t value1 = aarch64_get_reg_u32 (cpu, rn, SP_OK);
  uint32_t value2 = extreg32 (cpu, rm, extension) << shift;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 + value2);
  set_flags_for_add32 (cpu, value1, value2);
}

/* 64 bit ADD extending register setting flags  */
/* N.B. this subsumes the case with 64 bit source2 and UXTX #n or LSL #0  */
static void
adds64_ext (sim_cpu *cpu, Extension extension, uint32_t shift)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint64_t value1 = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  uint64_t value2 = extreg64 (cpu, rm, extension) << shift;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 + value2);
  set_flags_for_add64 (cpu, value1, value2);
}

/* 32 bit SUB extending register.  */
static void
sub32_ext (sim_cpu *cpu, Extension extension, uint32_t shift)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, SP_OK,
		       aarch64_get_reg_u32 (cpu, rn, SP_OK)
		       - (extreg32 (cpu, rm, extension) << shift));
}

/* 64 bit SUB extending register.  */
/* N.B. this subsumes the case with 64 bit source2 and UXTX #n or LSL #0.  */
static void
sub64_ext (sim_cpu *cpu, Extension extension, uint32_t shift)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, SP_OK,
		       aarch64_get_reg_u64 (cpu, rn, SP_OK)
		       - (extreg64 (cpu, rm, extension) << shift));
}

/* 32 bit SUB extending register setting flags.  */
static void
subs32_ext (sim_cpu *cpu, Extension extension, uint32_t shift)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint32_t value1 = aarch64_get_reg_u32 (cpu, rn, SP_OK);
  uint32_t value2 = extreg32 (cpu, rm, extension) << shift;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 - value2);
  set_flags_for_sub32 (cpu, value1, value2);
}

/* 64 bit SUB extending register setting flags  */
/* N.B. this subsumes the case with 64 bit source2 and UXTX #n or LSL #0  */
static void
subs64_ext (sim_cpu *cpu, Extension extension, uint32_t shift)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint64_t value1 = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  uint64_t value2 = extreg64 (cpu, rm, extension) << shift;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 - value2);
  set_flags_for_sub64 (cpu, value1, value2);
}

static void
dexAddSubtractImmediate (sim_cpu *cpu)
{
  /* instr[31]    = size : 0 ==> 32 bit, 1 ==> 64 bit
     instr[30]    = op : 0 ==> ADD, 1 ==> SUB
     instr[29]    = set : 0 ==> no flags, 1 ==> set flags
     instr[28,24] = 10001
     instr[23,22] = shift : 00 == LSL#0, 01 = LSL#12 1x = UNALLOC
     instr[21,10] = uimm12
     instr[9,5]   = Rn
     instr[4,0]   = Rd  */

  /* N.B. the shift is applied at decode before calling the add/sub routine.  */
  uint32_t shift = INSTR (23, 22);
  uint32_t imm = INSTR (21, 10);
  uint32_t dispatch = INSTR (31, 29);

  NYI_assert (28, 24, 0x11);

  if (shift > 1)
    HALT_UNALLOC;

  if (shift)
    imm <<= 12;

  switch (dispatch)
    {
    case 0: add32 (cpu, imm); break;
    case 1: adds32 (cpu, imm); break;
    case 2: sub32 (cpu, imm); break;
    case 3: subs32 (cpu, imm); break;
    case 4: add64 (cpu, imm); break;
    case 5: adds64 (cpu, imm); break;
    case 6: sub64 (cpu, imm); break;
    case 7: subs64 (cpu, imm); break;
    }
}

static void
dexAddSubtractShiftedRegister (sim_cpu *cpu)
{
  /* instr[31]    = size : 0 ==> 32 bit, 1 ==> 64 bit
     instr[30,29] = op : 00 ==> ADD, 01 ==> ADDS, 10 ==> SUB, 11 ==> SUBS
     instr[28,24] = 01011
     instr[23,22] = shift : 0 ==> LSL, 1 ==> LSR, 2 ==> ASR, 3 ==> UNALLOC
     instr[21]    = 0
     instr[20,16] = Rm
     instr[15,10] = count : must be 0xxxxx for 32 bit
     instr[9,5]   = Rn
     instr[4,0]   = Rd  */

  uint32_t size = INSTR (31, 31);
  uint32_t count = INSTR (15, 10);
  Shift shiftType = INSTR (23, 22);

  NYI_assert (28, 24, 0x0B);
  NYI_assert (21, 21, 0);

  /* Shift encoded as ROR is unallocated.  */
  if (shiftType == ROR)
    HALT_UNALLOC;

  /* 32 bit operations must have count[5] = 0
     or else we have an UNALLOC.  */
  if (size == 0 && uimm (count, 5, 5))
    HALT_UNALLOC;

  /* Dispatch on size:op i.e instr [31,29].  */
  switch (INSTR (31, 29))
    {
    case 0: add32_shift  (cpu, shiftType, count); break;
    case 1: adds32_shift (cpu, shiftType, count); break;
    case 2: sub32_shift  (cpu, shiftType, count); break;
    case 3: subs32_shift (cpu, shiftType, count); break;
    case 4: add64_shift  (cpu, shiftType, count); break;
    case 5: adds64_shift (cpu, shiftType, count); break;
    case 6: sub64_shift  (cpu, shiftType, count); break;
    case 7: subs64_shift (cpu, shiftType, count); break;
    }
}

static void
dexAddSubtractExtendedRegister (sim_cpu *cpu)
{
  /* instr[31]    = size : 0 ==> 32 bit, 1 ==> 64 bit
     instr[30]    = op : 0 ==> ADD, 1 ==> SUB
     instr[29]    = set? : 0 ==> no flags, 1 ==> set flags
     instr[28,24] = 01011
     instr[23,22] = opt : 0 ==> ok, 1,2,3 ==> UNALLOC
     instr[21]    = 1
     instr[20,16] = Rm
     instr[15,13] = option : 000 ==> UXTB, 001 ==> UXTH,
                             000 ==> LSL|UXTW, 001 ==> UXTZ,
                             000 ==> SXTB, 001 ==> SXTH,
                             000 ==> SXTW, 001 ==> SXTX,
     instr[12,10] = shift : 0,1,2,3,4 ==> ok, 5,6,7 ==> UNALLOC
     instr[9,5]   = Rn
     instr[4,0]   = Rd  */

  Extension extensionType = INSTR (15, 13);
  uint32_t shift = INSTR (12, 10);

  NYI_assert (28, 24, 0x0B);
  NYI_assert (21, 21, 1);

  /* Shift may not exceed 4.  */
  if (shift > 4)
    HALT_UNALLOC;

  /* Dispatch on size:op:set?.  */
  switch (INSTR (31, 29))
    {
    case 0: add32_ext  (cpu, extensionType, shift); break;
    case 1: adds32_ext (cpu, extensionType, shift); break;
    case 2: sub32_ext  (cpu, extensionType, shift); break;
    case 3: subs32_ext (cpu, extensionType, shift); break;
    case 4: add64_ext  (cpu, extensionType, shift); break;
    case 5: adds64_ext (cpu, extensionType, shift); break;
    case 6: sub64_ext  (cpu, extensionType, shift); break;
    case 7: subs64_ext (cpu, extensionType, shift); break;
    }
}

/* Conditional data processing
   Condition register is implicit 3rd source.  */

/* 32 bit add with carry.  */
/* N.B register args may not be SP.  */

static void
adc32 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       aarch64_get_reg_u32 (cpu, rn, NO_SP)
		       + aarch64_get_reg_u32 (cpu, rm, NO_SP)
		       + IS_SET (C));
}

/* 64 bit add with carry  */
static void
adc64 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       aarch64_get_reg_u64 (cpu, rn, NO_SP)
		       + aarch64_get_reg_u64 (cpu, rm, NO_SP)
		       + IS_SET (C));
}

/* 32 bit add with carry setting flags.  */
static void
adcs32 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint32_t value1 = aarch64_get_reg_u32 (cpu, rn, NO_SP);
  uint32_t value2 = aarch64_get_reg_u32 (cpu, rm, NO_SP);
  uint32_t carry = IS_SET (C);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 + value2 + carry);
  set_flags_for_add32 (cpu, value1, value2 + carry);
}

/* 64 bit add with carry setting flags.  */
static void
adcs64 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint64_t value1 = aarch64_get_reg_u64 (cpu, rn, NO_SP);
  uint64_t value2 = aarch64_get_reg_u64 (cpu, rm, NO_SP);
  uint64_t carry = IS_SET (C);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 + value2 + carry);
  set_flags_for_add64 (cpu, value1, value2 + carry);
}

/* 32 bit sub with carry.  */
static void
sbc32 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5); /* ngc iff rn == 31.  */
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       aarch64_get_reg_u32 (cpu, rn, NO_SP)
		       - aarch64_get_reg_u32 (cpu, rm, NO_SP)
		       - 1 + IS_SET (C));
}

/* 64 bit sub with carry  */
static void
sbc64 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       aarch64_get_reg_u64 (cpu, rn, NO_SP)
		       - aarch64_get_reg_u64 (cpu, rm, NO_SP)
		       - 1 + IS_SET (C));
}

/* 32 bit sub with carry setting flags  */
static void
sbcs32 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint32_t value1 = aarch64_get_reg_u32 (cpu, rn, NO_SP);
  uint32_t value2 = aarch64_get_reg_u32 (cpu, rm, NO_SP);
  uint32_t carry  = IS_SET (C);
  uint32_t result = value1 - value2 + 1 - carry;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, result);
  set_flags_for_sub32 (cpu, value1, value2 + 1 - carry);
}

/* 64 bit sub with carry setting flags  */
static void
sbcs64 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint64_t value1 = aarch64_get_reg_u64 (cpu, rn, NO_SP);
  uint64_t value2 = aarch64_get_reg_u64 (cpu, rm, NO_SP);
  uint64_t carry  = IS_SET (C);
  uint64_t result = value1 - value2 + 1 - carry;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, result);
  set_flags_for_sub64 (cpu, value1, value2 + 1 - carry);
}

static void
dexAddSubtractWithCarry (sim_cpu *cpu)
{
  /* instr[31]    = size : 0 ==> 32 bit, 1 ==> 64 bit
     instr[30]    = op : 0 ==> ADC, 1 ==> SBC
     instr[29]    = set? : 0 ==> no flags, 1 ==> set flags
     instr[28,21] = 1 1010 000
     instr[20,16] = Rm
     instr[15,10] = op2 : 00000 ==> ok, ow ==> UNALLOC
     instr[9,5]   = Rn
     instr[4,0]   = Rd  */

  uint32_t op2 = INSTR (15, 10);

  NYI_assert (28, 21, 0xD0);

  if (op2 != 0)
    HALT_UNALLOC;

  /* Dispatch on size:op:set?.  */
  switch (INSTR (31, 29))
    {
    case 0: adc32 (cpu); break;
    case 1: adcs32 (cpu); break;
    case 2: sbc32 (cpu); break;
    case 3: sbcs32 (cpu); break;
    case 4: adc64 (cpu); break;
    case 5: adcs64 (cpu); break;
    case 6: sbc64 (cpu); break;
    case 7: sbcs64 (cpu); break;
    }
}

static uint32_t
testConditionCode (sim_cpu *cpu, CondCode cc)
{
  /* This should be reduceable to branchless logic
     by some careful testing of bits in CC followed
     by the requisite masking and combining of bits
     from the flag register.

     For now we do it with a switch.  */
  int res;

  switch (cc)
    {
    case EQ:  res = IS_SET (Z);    break;
    case NE:  res = IS_CLEAR (Z);  break;
    case CS:  res = IS_SET (C);    break;
    case CC:  res = IS_CLEAR (C);  break;
    case MI:  res = IS_SET (N);    break;
    case PL:  res = IS_CLEAR (N);  break;
    case VS:  res = IS_SET (V);    break;
    case VC:  res = IS_CLEAR (V);  break;
    case HI:  res = IS_SET (C) && IS_CLEAR (Z);  break;
    case LS:  res = IS_CLEAR (C) || IS_SET (Z);  break;
    case GE:  res = IS_SET (N) == IS_SET (V);    break;
    case LT:  res = IS_SET (N) != IS_SET (V);    break;
    case GT:  res = IS_CLEAR (Z) && (IS_SET (N) == IS_SET (V));  break;
    case LE:  res = IS_SET (Z) || (IS_SET (N) != IS_SET (V));    break;
    case AL:
    case NV:
    default:
      res = 1;
      break;
    }
  return res;
}

static void
CondCompare (sim_cpu *cpu) /* aka: ccmp and ccmn  */
{
  /* instr[31]    = size : 0 ==> 32 bit, 1 ==> 64 bit
     instr[30]    = compare with positive (1) or negative value (0)
     instr[29,21] = 1 1101 0010
     instr[20,16] = Rm or const
     instr[15,12] = cond
     instr[11]    = compare reg (0) or const (1)
     instr[10]    = 0
     instr[9,5]   = Rn
     instr[4]     = 0
     instr[3,0]   = value for CPSR bits if the comparison does not take place.  */
  signed int negate;
  unsigned rm;
  unsigned rn;

  NYI_assert (29, 21, 0x1d2);
  NYI_assert (10, 10, 0);
  NYI_assert (4, 4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (! testConditionCode (cpu, INSTR (15, 12)))
    {
      aarch64_set_CPSR (cpu, INSTR (3, 0));
      return;
    }

  negate = INSTR (30, 30) ? 1 : -1;
  rm = INSTR (20, 16);
  rn = INSTR ( 9,  5);

  if (INSTR (31, 31))
    {
      if (INSTR (11, 11))
	set_flags_for_sub64 (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK),
			     negate * (uint64_t) rm);
      else
	set_flags_for_sub64 (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK),
			     negate * aarch64_get_reg_u64 (cpu, rm, SP_OK));
    }
  else
    {
      if (INSTR (11, 11))
	set_flags_for_sub32 (cpu, aarch64_get_reg_u32 (cpu, rn, SP_OK),
			     negate * rm);
      else
	set_flags_for_sub32 (cpu, aarch64_get_reg_u32 (cpu, rn, SP_OK),
			     negate * aarch64_get_reg_u32 (cpu, rm, SP_OK));
    }
}

static void
do_vec_MOV_whole_vector (sim_cpu *cpu)
{
  /* MOV Vd.T, Vs.T  (alias for ORR Vd.T, Vn.T, Vm.T where Vn == Vm)

     instr[31]    = 0
     instr[30]    = half(0)/full(1)
     instr[29,21] = 001110101
     instr[20,16] = Vs
     instr[15,10] = 000111
     instr[9,5]   = Vs
     instr[4,0]   = Vd  */

  unsigned vs = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);

  NYI_assert (29, 21, 0x075);
  NYI_assert (15, 10, 0x07);

  if (INSTR (20, 16) != vs)
    HALT_NYI;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (30, 30))
    aarch64_set_vec_u64 (cpu, vd, 1, aarch64_get_vec_u64 (cpu, vs, 1));

  aarch64_set_vec_u64 (cpu, vd, 0, aarch64_get_vec_u64 (cpu, vs, 0));
}

static void
do_vec_SMOV_into_scalar (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = word(0)/long(1)
     instr[29,21] = 00 1110 000
     instr[20,16] = element size and index
     instr[15,10] = 00 0010 11
     instr[9,5]   = V source
     instr[4,0]   = R dest  */

  unsigned vs = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  unsigned imm5 = INSTR (20, 16);
  unsigned full = INSTR (30, 30);
  int size, index;

  NYI_assert (29, 21, 0x070);
  NYI_assert (15, 10, 0x0B);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);

  if (imm5 & 0x1)
    {
      size = 0;
      index = (imm5 >> 1) & 0xF;
    }
  else if (imm5 & 0x2)
    {
      size = 1;
      index = (imm5 >> 2) & 0x7;
    }
  else if (full && (imm5 & 0x4))
    {
      size = 2;
      index = (imm5 >> 3) & 0x3;
    }
  else
    HALT_UNALLOC;

  switch (size)
    {
    case 0:
      if (full)
	aarch64_set_reg_s64 (cpu, rd, NO_SP,
			     aarch64_get_vec_s8 (cpu, vs, index));
      else
	aarch64_set_reg_s32 (cpu, rd, NO_SP,
			     aarch64_get_vec_s8 (cpu, vs, index));
      break;

    case 1:
      if (full)
	aarch64_set_reg_s64 (cpu, rd, NO_SP,
			     aarch64_get_vec_s16 (cpu, vs, index));
      else
	aarch64_set_reg_s32 (cpu, rd, NO_SP,
			     aarch64_get_vec_s16 (cpu, vs, index));
      break;

    case 2:
      aarch64_set_reg_s64 (cpu, rd, NO_SP,
			   aarch64_get_vec_s32 (cpu, vs, index));
      break;

    default:
      HALT_UNALLOC;
    }
}

static void
do_vec_UMOV_into_scalar (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = word(0)/long(1)
     instr[29,21] = 00 1110 000
     instr[20,16] = element size and index
     instr[15,10] = 00 0011 11
     instr[9,5]   = V source
     instr[4,0]   = R dest  */

  unsigned vs = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  unsigned imm5 = INSTR (20, 16);
  unsigned full = INSTR (30, 30);
  int size, index;

  NYI_assert (29, 21, 0x070);
  NYI_assert (15, 10, 0x0F);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);

  if (!full)
    {
      if (imm5 & 0x1)
	{
	  size = 0;
	  index = (imm5 >> 1) & 0xF;
	}
      else if (imm5 & 0x2)
	{
	  size = 1;
	  index = (imm5 >> 2) & 0x7;
	}
      else if (imm5 & 0x4)
	{
	  size = 2;
	  index = (imm5 >> 3) & 0x3;
	}
      else
	HALT_UNALLOC;
    }
  else if (imm5 & 0x8)
    {
      size = 3;
      index = (imm5 >> 4) & 0x1;
    }
  else
    HALT_UNALLOC;

  switch (size)
    {
    case 0:
      aarch64_set_reg_u32 (cpu, rd, NO_SP,
			   aarch64_get_vec_u8 (cpu, vs, index));
      break;

    case 1:
      aarch64_set_reg_u32 (cpu, rd, NO_SP,
			   aarch64_get_vec_u16 (cpu, vs, index));
      break;

    case 2:
      aarch64_set_reg_u32 (cpu, rd, NO_SP,
			   aarch64_get_vec_u32 (cpu, vs, index));
      break;

    case 3:
      aarch64_set_reg_u64 (cpu, rd, NO_SP,
			   aarch64_get_vec_u64 (cpu, vs, index));
      break;

    default:
      HALT_UNALLOC;
    }
}

static void
do_vec_INS (sim_cpu *cpu)
{
  /* instr[31,21] = 01001110000
     instr[20,16] = element size and index
     instr[15,10] = 000111
     instr[9,5]   = W source
     instr[4,0]   = V dest  */

  int index;
  unsigned rs = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);

  NYI_assert (31, 21, 0x270);
  NYI_assert (15, 10, 0x07);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (16, 16))
    {
      index = INSTR (20, 17);
      aarch64_set_vec_u8 (cpu, vd, index,
			  aarch64_get_reg_u8 (cpu, rs, NO_SP));
    }
  else if (INSTR (17, 17))
    {
      index = INSTR (20, 18);
      aarch64_set_vec_u16 (cpu, vd, index,
			   aarch64_get_reg_u16 (cpu, rs, NO_SP));
    }
  else if (INSTR (18, 18))
    {
      index = INSTR (20, 19);
      aarch64_set_vec_u32 (cpu, vd, index,
			   aarch64_get_reg_u32 (cpu, rs, NO_SP));
    }
  else if (INSTR (19, 19))
    {
      index = INSTR (20, 20);
      aarch64_set_vec_u64 (cpu, vd, index,
			   aarch64_get_reg_u64 (cpu, rs, NO_SP));
    }
  else
    HALT_NYI;
}

static void
do_vec_DUP_vector_into_vector (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half(0)/full(1)
     instr[29,21] = 00 1110 000
     instr[20,16] = element size and index
     instr[15,10] = 0000 01
     instr[9,5]   = V source
     instr[4,0]   = V dest.  */

  unsigned full = INSTR (30, 30);
  unsigned vs = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  int i, index;

  NYI_assert (29, 21, 0x070);
  NYI_assert (15, 10, 0x01);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (16, 16))
    {
      index = INSTR (20, 17);

      for (i = 0; i < (full ? 16 : 8); i++)
	aarch64_set_vec_u8 (cpu, vd, i, aarch64_get_vec_u8 (cpu, vs, index));
    }
  else if (INSTR (17, 17))
    {
      index = INSTR (20, 18);

      for (i = 0; i < (full ? 8 : 4); i++)
	aarch64_set_vec_u16 (cpu, vd, i, aarch64_get_vec_u16 (cpu, vs, index));
    }
  else if (INSTR (18, 18))
    {
      index = INSTR (20, 19);

      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_u32 (cpu, vd, i, aarch64_get_vec_u32 (cpu, vs, index));
    }
  else
    {
      if (INSTR (19, 19) == 0)
	HALT_UNALLOC;

      if (! full)
	HALT_UNALLOC;

      index = INSTR (20, 20);

      for (i = 0; i < 2; i++)
	aarch64_set_vec_u64 (cpu, vd, i, aarch64_get_vec_u64 (cpu, vs, index));
    }
}

static void
do_vec_TBL (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half(0)/full(1)
     instr[29,21] = 00 1110 000
     instr[20,16] = Vm
     instr[15]    = 0
     instr[14,13] = vec length
     instr[12,10] = 000
     instr[9,5]   = V start
     instr[4,0]   = V dest  */

  int full    = INSTR (30, 30);
  int len     = INSTR (14, 13) + 1;
  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;

  NYI_assert (29, 21, 0x070);
  NYI_assert (12, 10, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  for (i = 0; i < (full ? 16 : 8); i++)
    {
      unsigned int selector = aarch64_get_vec_u8 (cpu, vm, i);
      uint8_t val;

      if (selector < 16)
	val = aarch64_get_vec_u8 (cpu, vn, selector);
      else if (selector < 32)
	val = len < 2 ? 0 : aarch64_get_vec_u8 (cpu, vn + 1, selector - 16);
      else if (selector < 48)
	val = len < 3 ? 0 : aarch64_get_vec_u8 (cpu, vn + 2, selector - 32);
      else if (selector < 64)
	val = len < 4 ? 0 : aarch64_get_vec_u8 (cpu, vn + 3, selector - 48);
      else
	val = 0;

      aarch64_set_vec_u8 (cpu, vd, i, val);
    }
}

static void
do_vec_TRN (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half(0)/full(1)
     instr[29,24] = 00 1110
     instr[23,22] = size
     instr[21]    = 0
     instr[20,16] = Vm
     instr[15]    = 0
     instr[14]    = TRN1 (0) / TRN2 (1)
     instr[13,10] = 1010
     instr[9,5]   = V source
     instr[4,0]   = V dest.  */

  int full    = INSTR (30, 30);
  int second  = INSTR (14, 14);
  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;

  NYI_assert (29, 24, 0x0E);
  NYI_assert (13, 10, 0xA);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 22))
    {
    case 0:
      for (i = 0; i < (full ? 8 : 4); i++)
	{
	  aarch64_set_vec_u8
	    (cpu, vd, i * 2,
	     aarch64_get_vec_u8 (cpu, second ? vm : vn, i * 2));
	  aarch64_set_vec_u8
	    (cpu, vd, 1 * 2 + 1,
	     aarch64_get_vec_u8 (cpu, second ? vn : vm, i * 2 + 1));
	}
      break;

    case 1:
      for (i = 0; i < (full ? 4 : 2); i++)
	{
	  aarch64_set_vec_u16
	    (cpu, vd, i * 2,
	     aarch64_get_vec_u16 (cpu, second ? vm : vn, i * 2));
	  aarch64_set_vec_u16
	    (cpu, vd, 1 * 2 + 1,
	     aarch64_get_vec_u16 (cpu, second ? vn : vm, i * 2 + 1));
	}
      break;

    case 2:
      aarch64_set_vec_u32
	(cpu, vd, 0, aarch64_get_vec_u32 (cpu, second ? vm : vn, 0));
      aarch64_set_vec_u32
	(cpu, vd, 1, aarch64_get_vec_u32 (cpu, second ? vn : vm, 1));
      aarch64_set_vec_u32
	(cpu, vd, 2, aarch64_get_vec_u32 (cpu, second ? vm : vn, 2));
      aarch64_set_vec_u32
	(cpu, vd, 3, aarch64_get_vec_u32 (cpu, second ? vn : vm, 3));
      break;

    case 3:
      if (! full)
	HALT_UNALLOC;

      aarch64_set_vec_u64 (cpu, vd, 0,
			   aarch64_get_vec_u64 (cpu, second ? vm : vn, 0));
      aarch64_set_vec_u64 (cpu, vd, 1,
			   aarch64_get_vec_u64 (cpu, second ? vn : vm, 1));
      break;
    }
}

static void
do_vec_DUP_scalar_into_vector (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = 0=> zero top 64-bits, 1=> duplicate into top 64-bits
                    [must be 1 for 64-bit xfer]
     instr[29,20] = 00 1110 0000
     instr[19,16] = element size: 0001=> 8-bits, 0010=> 16-bits,
                                  0100=> 32-bits. 1000=>64-bits
     instr[15,10] = 0000 11
     instr[9,5]   = W source
     instr[4,0]   = V dest.  */

  unsigned i;
  unsigned Vd = INSTR (4, 0);
  unsigned Rs = INSTR (9, 5);
  int both    = INSTR (30, 30);

  NYI_assert (29, 20, 0x0E0);
  NYI_assert (15, 10, 0x03);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (19, 16))
    {
    case 1:
      for (i = 0; i < (both ? 16 : 8); i++)
	aarch64_set_vec_u8 (cpu, Vd, i, aarch64_get_reg_u8 (cpu, Rs, NO_SP));
      break;

    case 2:
      for (i = 0; i < (both ? 8 : 4); i++)
	aarch64_set_vec_u16 (cpu, Vd, i, aarch64_get_reg_u16 (cpu, Rs, NO_SP));
      break;

    case 4:
      for (i = 0; i < (both ? 4 : 2); i++)
	aarch64_set_vec_u32 (cpu, Vd, i, aarch64_get_reg_u32 (cpu, Rs, NO_SP));
      break;

    case 8:
      if (!both)
	HALT_NYI;
      aarch64_set_vec_u64 (cpu, Vd, 0, aarch64_get_reg_u64 (cpu, Rs, NO_SP));
      aarch64_set_vec_u64 (cpu, Vd, 1, aarch64_get_reg_u64 (cpu, Rs, NO_SP));
      break;

    default:
      HALT_NYI;
    }
}

static void
do_vec_UZP (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half(0)/full(1)
     instr[29,24] = 00 1110
     instr[23,22] = size: byte(00), half(01), word (10), long (11)
     instr[21]    = 0
     instr[20,16] = Vm
     instr[15]    = 0
     instr[14]    = lower (0) / upper (1)
     instr[13,10] = 0110
     instr[9,5]   = Vn
     instr[4,0]   = Vd.  */

  int full = INSTR (30, 30);
  int upper = INSTR (14, 14);

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);

  uint64_t val_m1 = aarch64_get_vec_u64 (cpu, vm, 0);
  uint64_t val_m2 = aarch64_get_vec_u64 (cpu, vm, 1);
  uint64_t val_n1 = aarch64_get_vec_u64 (cpu, vn, 0);
  uint64_t val_n2 = aarch64_get_vec_u64 (cpu, vn, 1);

  uint64_t val1;
  uint64_t val2;

  uint64_t input2 = full ? val_n2 : val_m1;

  NYI_assert (29, 24, 0x0E);
  NYI_assert (21, 21, 0);
  NYI_assert (15, 15, 0);
  NYI_assert (13, 10, 6);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 22))
    {
    case 0:
      val1 = (val_n1 >> (upper * 8)) & 0xFFULL;
      val1 |= (val_n1 >> ((upper * 8) + 8)) & 0xFF00ULL;
      val1 |= (val_n1 >> ((upper * 8) + 16)) & 0xFF0000ULL;
      val1 |= (val_n1 >> ((upper * 8) + 24)) & 0xFF000000ULL;

      val1 |= (input2 << (32 - (upper * 8))) & 0xFF00000000ULL;
      val1 |= (input2 << (24 - (upper * 8))) & 0xFF0000000000ULL;
      val1 |= (input2 << (16 - (upper * 8))) & 0xFF000000000000ULL;
      val1 |= (input2 << (8 - (upper * 8))) & 0xFF00000000000000ULL;

      if (full)
	{
	  val2 = (val_m1 >> (upper * 8)) & 0xFFULL;
	  val2 |= (val_m1 >> ((upper * 8) + 8)) & 0xFF00ULL;
	  val2 |= (val_m1 >> ((upper * 8) + 16)) & 0xFF0000ULL;
	  val2 |= (val_m1 >> ((upper * 8) + 24)) & 0xFF000000ULL;

	  val2 |= (val_m2 << (32 - (upper * 8))) & 0xFF00000000ULL;
	  val2 |= (val_m2 << (24 - (upper * 8))) & 0xFF0000000000ULL;
	  val2 |= (val_m2 << (16 - (upper * 8))) & 0xFF000000000000ULL;
	  val2 |= (val_m2 << (8 - (upper * 8))) & 0xFF00000000000000ULL;
	}
      break;

    case 1:
      val1 = (val_n1 >> (upper * 16)) & 0xFFFFULL;
      val1 |= (val_n1 >> ((upper * 16) + 16)) & 0xFFFF0000ULL;

      val1 |= (input2 << (32 - (upper * 16))) & 0xFFFF00000000ULL;;
      val1 |= (input2 << (16 - (upper * 16))) & 0xFFFF000000000000ULL;

      if (full)
	{
	  val2 = (val_m1 >> (upper * 16)) & 0xFFFFULL;
	  val2 |= (val_m1 >> ((upper * 16) + 16)) & 0xFFFF0000ULL;

	  val2 |= (val_m2 << (32 - (upper * 16))) & 0xFFFF00000000ULL;
	  val2 |= (val_m2 << (16 - (upper * 16))) & 0xFFFF000000000000ULL;
	}
      break;

    case 2:
      val1 = (val_n1 >> (upper * 32)) & 0xFFFFFFFF;
      val1 |= (input2 << (32 - (upper * 32))) & 0xFFFFFFFF00000000ULL;

      if (full)
	{
	  val2 = (val_m1 >> (upper * 32)) & 0xFFFFFFFF;
	  val2 |= (val_m2 << (32 - (upper * 32))) & 0xFFFFFFFF00000000ULL;
	}
      break;

    case 3:
      if (! full)
	HALT_UNALLOC;

      val1 = upper ? val_n2 : val_n1;
      val2 = upper ? val_m2 : val_m1;
      break;
    }

  aarch64_set_vec_u64 (cpu, vd, 0, val1);
  if (full)
    aarch64_set_vec_u64 (cpu, vd, 1, val2);
}

static void
do_vec_ZIP (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half(0)/full(1)
     instr[29,24] = 00 1110
     instr[23,22] = size: byte(00), hald(01), word (10), long (11)
     instr[21]    = 0
     instr[20,16] = Vm
     instr[15]    = 0
     instr[14]    = lower (0) / upper (1)
     instr[13,10] = 1110
     instr[9,5]   = Vn
     instr[4,0]   = Vd.  */

  int full = INSTR (30, 30);
  int upper = INSTR (14, 14);

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);

  uint64_t val_m1 = aarch64_get_vec_u64 (cpu, vm, 0);
  uint64_t val_m2 = aarch64_get_vec_u64 (cpu, vm, 1);
  uint64_t val_n1 = aarch64_get_vec_u64 (cpu, vn, 0);
  uint64_t val_n2 = aarch64_get_vec_u64 (cpu, vn, 1);

  uint64_t val1 = 0;
  uint64_t val2 = 0;

  uint64_t input1 = upper ? val_n1 : val_m1;
  uint64_t input2 = upper ? val_n2 : val_m2;

  NYI_assert (29, 24, 0x0E);
  NYI_assert (21, 21, 0);
  NYI_assert (15, 15, 0);
  NYI_assert (13, 10, 0xE);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 23))
    {
    case 0:
      val1 =
	  ((input1 <<  0) & (0xFF    <<  0))
	| ((input2 <<  8) & (0xFF    <<  8))
	| ((input1 <<  8) & (0xFF    << 16))
	| ((input2 << 16) & (0xFF    << 24))
	| ((input1 << 16) & (0xFFULL << 32))
	| ((input2 << 24) & (0xFFULL << 40))
	| ((input1 << 24) & (0xFFULL << 48))
	| ((input2 << 32) & (0xFFULL << 56));

      val2 =
	  ((input1 >> 32) & (0xFF    <<  0))
	| ((input2 >> 24) & (0xFF    <<  8))
	| ((input1 >> 24) & (0xFF    << 16))
	| ((input2 >> 16) & (0xFF    << 24))
	| ((input1 >> 16) & (0xFFULL << 32))
	| ((input2 >>  8) & (0xFFULL << 40))
	| ((input1 >>  8) & (0xFFULL << 48))
	| ((input2 >>  0) & (0xFFULL << 56));
      break;

    case 1:
      val1 =
	  ((input1 <<  0) & (0xFFFF    <<  0))
	| ((input2 << 16) & (0xFFFF    << 16))
	| ((input1 << 16) & (0xFFFFULL << 32))
	| ((input2 << 32) & (0xFFFFULL << 48));

      val2 =
	  ((input1 >> 32) & (0xFFFF    <<  0))
	| ((input2 >> 16) & (0xFFFF    << 16))
	| ((input1 >> 16) & (0xFFFFULL << 32))
	| ((input2 >>  0) & (0xFFFFULL << 48));
      break;

    case 2:
      val1 = (input1 & 0xFFFFFFFFULL) | (input2 << 32);
      val2 = (input2 & 0xFFFFFFFFULL) | (input1 << 32);
      break;

    case 3:
      val1 = input1;
      val2 = input2;
      break;
    }

  aarch64_set_vec_u64 (cpu, vd, 0, val1);
  if (full)
    aarch64_set_vec_u64 (cpu, vd, 1, val2);
}

/* Floating point immediates are encoded in 8 bits.
   fpimm[7] = sign bit.
   fpimm[6:4] = signed exponent.
   fpimm[3:0] = fraction (assuming leading 1).
   i.e. F = s * 1.f * 2^(e - b).  */

static float
fp_immediate_for_encoding_32 (uint32_t imm8)
{
  float u;
  uint32_t s, e, f, i;

  s = (imm8 >> 7) & 0x1;
  e = (imm8 >> 4) & 0x7;
  f = imm8 & 0xf;

  /* The fp value is s * n/16 * 2r where n is 16+e.  */
  u = (16.0 + f) / 16.0;

  /* N.B. exponent is signed.  */
  if (e < 4)
    {
      int epos = e;

      for (i = 0; i <= epos; i++)
	u *= 2.0;
    }
  else
    {
      int eneg = 7 - e;

      for (i = 0; i < eneg; i++)
	u /= 2.0;
    }

  if (s)
    u = - u;

  return u;
}

static double
fp_immediate_for_encoding_64 (uint32_t imm8)
{
  double u;
  uint32_t s, e, f, i;

  s = (imm8 >> 7) & 0x1;
  e = (imm8 >> 4) & 0x7;
  f = imm8 & 0xf;

  /* The fp value is s * n/16 * 2r where n is 16+e.  */
  u = (16.0 + f) / 16.0;

  /* N.B. exponent is signed.  */
  if (e < 4)
    {
      int epos = e;

      for (i = 0; i <= epos; i++)
	u *= 2.0;
    }
  else
    {
      int eneg = 7 - e;

      for (i = 0; i < eneg; i++)
	u /= 2.0;
    }

  if (s)
    u = - u;

  return u;
}

static void
do_vec_MOV_immediate (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = full/half selector
     instr[29,19] = 00111100000
     instr[18,16] = high 3 bits of uimm8
     instr[15,12] = size & shift:
                                  0000 => 32-bit
                                  0010 => 32-bit + LSL#8
                                  0100 => 32-bit + LSL#16
                                  0110 => 32-bit + LSL#24
                                  1010 => 16-bit + LSL#8
                                  1000 => 16-bit
                                  1101 => 32-bit + MSL#16
                                  1100 => 32-bit + MSL#8
                                  1110 => 8-bit
                                  1111 => double
     instr[11,10] = 01
     instr[9,5]   = low 5-bits of uimm8
     instr[4,0]   = Vd.  */

  int full     = INSTR (30, 30);
  unsigned vd  = INSTR (4, 0);
  unsigned val = (INSTR (18, 16) << 5) | INSTR (9, 5);
  unsigned i;

  NYI_assert (29, 19, 0x1E0);
  NYI_assert (11, 10, 1);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (15, 12))
    {
    case 0x0: /* 32-bit, no shift.  */
    case 0x2: /* 32-bit, shift by 8.  */
    case 0x4: /* 32-bit, shift by 16.  */
    case 0x6: /* 32-bit, shift by 24.  */
      val <<= (8 * INSTR (14, 13));
      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_u32 (cpu, vd, i, val);
      break;

    case 0xa: /* 16-bit, shift by 8.  */
      val <<= 8;
      ATTRIBUTE_FALLTHROUGH;
    case 0x8: /* 16-bit, no shift.  */
      for (i = 0; i < (full ? 8 : 4); i++)
	aarch64_set_vec_u16 (cpu, vd, i, val);
      break;

    case 0xd: /* 32-bit, mask shift by 16.  */
      val <<= 8;
      val |= 0xFF;
      ATTRIBUTE_FALLTHROUGH;
    case 0xc: /* 32-bit, mask shift by 8. */
      val <<= 8;
      val |= 0xFF;
      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_u32 (cpu, vd, i, val);
      break;

    case 0xe: /* 8-bit, no shift.  */
      for (i = 0; i < (full ? 16 : 8); i++)
	aarch64_set_vec_u8 (cpu, vd, i, val);
      break;

    case 0xf: /* FMOV Vs.{2|4}S, #fpimm.  */
      {
	float u = fp_immediate_for_encoding_32 (val);
	for (i = 0; i < (full ? 4 : 2); i++)
	  aarch64_set_vec_float (cpu, vd, i, u);
	break;
      }

    default:
      HALT_NYI;
    }
}

static void
do_vec_MVNI (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = full/half selector
     instr[29,19] = 10111100000
     instr[18,16] = high 3 bits of uimm8
     instr[15,12] = selector
     instr[11,10] = 01
     instr[9,5]   = low 5-bits of uimm8
     instr[4,0]   = Vd.  */

  int full     = INSTR (30, 30);
  unsigned vd  = INSTR (4, 0);
  unsigned val = (INSTR (18, 16) << 5) | INSTR (9, 5);
  unsigned i;

  NYI_assert (29, 19, 0x5E0);
  NYI_assert (11, 10, 1);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (15, 12))
    {
    case 0x0: /* 32-bit, no shift.  */
    case 0x2: /* 32-bit, shift by 8.  */
    case 0x4: /* 32-bit, shift by 16.  */
    case 0x6: /* 32-bit, shift by 24.  */
      val <<= (8 * INSTR (14, 13));
      val = ~ val;
      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_u32 (cpu, vd, i, val);
      return;

    case 0xa: /* 16-bit, 8 bit shift. */
      val <<= 8;
      ATTRIBUTE_FALLTHROUGH;
    case 0x8: /* 16-bit, no shift. */
      val = ~ val;
      for (i = 0; i < (full ? 8 : 4); i++)
	aarch64_set_vec_u16 (cpu, vd, i, val);
      return;

    case 0xd: /* 32-bit, mask shift by 16.  */
      val <<= 8;
      val |= 0xFF;
      ATTRIBUTE_FALLTHROUGH;
    case 0xc: /* 32-bit, mask shift by 8. */
      val <<= 8;
      val |= 0xFF;
      val = ~ val;
      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_u32 (cpu, vd, i, val);
      return;

    case 0xE: /* MOVI Dn, #mask64 */
      {
	uint64_t mask = 0;

	for (i = 0; i < 8; i++)
	  if (val & (1 << i))
	    mask |= (0xFFUL << (i * 8));
	aarch64_set_vec_u64 (cpu, vd, 0, mask);
	aarch64_set_vec_u64 (cpu, vd, 1, mask);
	return;
      }

    case 0xf: /* FMOV Vd.2D, #fpimm.  */
      {
	double u = fp_immediate_for_encoding_64 (val);

	if (! full)
	  HALT_UNALLOC;

	aarch64_set_vec_double (cpu, vd, 0, u);
	aarch64_set_vec_double (cpu, vd, 1, u);
	return;
      }

    default:
      HALT_NYI;
    }
}

#define ABS(A) ((A) < 0 ? - (A) : (A))

static void
do_vec_ABS (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half(0)/full(1)
     instr[29,24] = 00 1110
     instr[23,22] = size: 00=> 8-bit, 01=> 16-bit, 10=> 32-bit, 11=> 64-bit
     instr[21,10] = 10 0000 1011 10
     instr[9,5]   = Vn
     instr[4.0]   = Vd.  */

  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned full = INSTR (30, 30);
  unsigned i;

  NYI_assert (29, 24, 0x0E);
  NYI_assert (21, 10, 0x82E);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 22))
    {
    case 0:
      for (i = 0; i < (full ? 16 : 8); i++)
	aarch64_set_vec_s8 (cpu, vd, i,
			    ABS (aarch64_get_vec_s8 (cpu, vn, i)));
      break;

    case 1:
      for (i = 0; i < (full ? 8 : 4); i++)
	aarch64_set_vec_s16 (cpu, vd, i,
			     ABS (aarch64_get_vec_s16 (cpu, vn, i)));
      break;

    case 2:
      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_s32 (cpu, vd, i,
			     ABS (aarch64_get_vec_s32 (cpu, vn, i)));
      break;

    case 3:
      if (! full)
	HALT_NYI;
      for (i = 0; i < 2; i++)
	aarch64_set_vec_s64 (cpu, vd, i,
			     ABS (aarch64_get_vec_s64 (cpu, vn, i)));
      break;
    }
}

static void
do_vec_ADDV (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = full/half selector
     instr[29,24] = 00 1110
     instr[23,22] = size: 00=> 8-bit, 01=> 16-bit, 10=> 32-bit, 11=> 64-bit
     instr[21,10] = 11 0001 1011 10
     instr[9,5]   = Vm
     instr[4.0]   = Rd.  */

  unsigned vm = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30, 30);

  NYI_assert (29, 24, 0x0E);
  NYI_assert (21, 10, 0xC6E);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 22))
    {
    case 0:
      {
	uint8_t val = 0;
	for (i = 0; i < (full ? 16 : 8); i++)
	  val += aarch64_get_vec_u8 (cpu, vm, i);
	aarch64_set_vec_u64 (cpu, rd, 0, val);
	return;
      }

    case 1:
      {
	uint16_t val = 0;
	for (i = 0; i < (full ? 8 : 4); i++)
	  val += aarch64_get_vec_u16 (cpu, vm, i);
	aarch64_set_vec_u64 (cpu, rd, 0, val);
	return;
      }

    case 2:
      {
	uint32_t val = 0;
	if (! full)
	  HALT_UNALLOC;
	for (i = 0; i < 4; i++)
	  val += aarch64_get_vec_u32 (cpu, vm, i);
	aarch64_set_vec_u64 (cpu, rd, 0, val);
	return;
      }

    case 3:
      HALT_UNALLOC;
    }
}

static void
do_vec_ins_2 (sim_cpu *cpu)
{
  /* instr[31,21] = 01001110000
     instr[20,18] = size & element selector
     instr[17,14] = 0000
     instr[13]    = direction: to vec(0), from vec (1)
     instr[12,10] = 111
     instr[9,5]   = Vm
     instr[4,0]   = Vd.  */

  unsigned elem;
  unsigned vm = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);

  NYI_assert (31, 21, 0x270);
  NYI_assert (17, 14, 0);
  NYI_assert (12, 10, 7);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (13, 13) == 1)
    {
      if (INSTR (18, 18) == 1)
	{
	  /* 32-bit moves.  */
	  elem = INSTR (20, 19);
	  aarch64_set_reg_u64 (cpu, vd, NO_SP,
			       aarch64_get_vec_u32 (cpu, vm, elem));
	}
      else
	{
	  /* 64-bit moves.  */
	  if (INSTR (19, 19) != 1)
	    HALT_NYI;

	  elem = INSTR (20, 20);
	  aarch64_set_reg_u64 (cpu, vd, NO_SP,
			       aarch64_get_vec_u64 (cpu, vm, elem));
	}
    }
  else
    {
      if (INSTR (18, 18) == 1)
	{
	  /* 32-bit moves.  */
	  elem = INSTR (20, 19);
	  aarch64_set_vec_u32 (cpu, vd, elem,
			       aarch64_get_reg_u32 (cpu, vm, NO_SP));
	}
      else
	{
	  /* 64-bit moves.  */
	  if (INSTR (19, 19) != 1)
	    HALT_NYI;

	  elem = INSTR (20, 20);
	  aarch64_set_vec_u64 (cpu, vd, elem,
			       aarch64_get_reg_u64 (cpu, vm, NO_SP));
	}
    }
}

#define DO_VEC_WIDENING_MUL(N, DST_TYPE, READ_TYPE, WRITE_TYPE)	  \
  do								  \
    {								  \
      DST_TYPE a[N], b[N];					  \
								  \
      for (i = 0; i < (N); i++)					  \
	{							  \
	  a[i] = aarch64_get_vec_##READ_TYPE (cpu, vn, i + bias); \
	  b[i] = aarch64_get_vec_##READ_TYPE (cpu, vm, i + bias); \
	}							  \
      for (i = 0; i < (N); i++)					  \
	aarch64_set_vec_##WRITE_TYPE (cpu, vd, i, a[i] * b[i]);	  \
    }								  \
  while (0)

static void
do_vec_mull (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = lower(0)/upper(1) selector
     instr[29]    = signed(0)/unsigned(1)
     instr[28,24] = 0 1110
     instr[23,22] = size: 8-bit (00), 16-bit (01), 32-bit (10)
     instr[21]    = 1
     instr[20,16] = Vm
     instr[15,10] = 11 0000
     instr[9,5]   = Vn
     instr[4.0]   = Vd.  */

  int    unsign = INSTR (29, 29);
  int    bias = INSTR (30, 30);
  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR ( 9,  5);
  unsigned vd = INSTR ( 4,  0);
  unsigned i;

  NYI_assert (28, 24, 0x0E);
  NYI_assert (15, 10, 0x30);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* NB: Read source values before writing results, in case
     the source and destination vectors are the same.  */
  switch (INSTR (23, 22))
    {
    case 0:
      if (bias)
	bias = 8;
      if (unsign)
	DO_VEC_WIDENING_MUL (8, uint16_t, u8, u16);
      else
	DO_VEC_WIDENING_MUL (8, int16_t, s8, s16);
      return;

    case 1:
      if (bias)
	bias = 4;
      if (unsign)
	DO_VEC_WIDENING_MUL (4, uint32_t, u16, u32);
      else
	DO_VEC_WIDENING_MUL (4, int32_t, s16, s32);
      return;

    case 2:
      if (bias)
	bias = 2;
      if (unsign)
	DO_VEC_WIDENING_MUL (2, uint64_t, u32, u64);
      else
	DO_VEC_WIDENING_MUL (2, int64_t, s32, s64);
      return;

    case 3:
      HALT_NYI;
    }
}

static void
do_vec_fadd (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half(0)/full(1)
     instr[29,24] = 001110
     instr[23]    = FADD(0)/FSUB(1)
     instr[22]    = float (0)/double(1)
     instr[21]    = 1
     instr[20,16] = Vm
     instr[15,10] = 110101
     instr[9,5]   = Vn
     instr[4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30, 30);

  NYI_assert (29, 24, 0x0E);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x35);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (23, 23))
    {
      if (INSTR (22, 22))
	{
	  if (! full)
	    HALT_NYI;

	  for (i = 0; i < 2; i++)
	    aarch64_set_vec_double (cpu, vd, i,
				    aarch64_get_vec_double (cpu, vn, i)
				    - aarch64_get_vec_double (cpu, vm, i));
	}
      else
	{
	  for (i = 0; i < (full ? 4 : 2); i++)
	    aarch64_set_vec_float (cpu, vd, i,
				   aarch64_get_vec_float (cpu, vn, i)
				   - aarch64_get_vec_float (cpu, vm, i));
	}
    }
  else
    {
      if (INSTR (22, 22))
	{
	  if (! full)
	    HALT_NYI;

	  for (i = 0; i < 2; i++)
	    aarch64_set_vec_double (cpu, vd, i,
				    aarch64_get_vec_double (cpu, vm, i)
				    + aarch64_get_vec_double (cpu, vn, i));
	}
      else
	{
	  for (i = 0; i < (full ? 4 : 2); i++)
	    aarch64_set_vec_float (cpu, vd, i,
				   aarch64_get_vec_float (cpu, vm, i)
				   + aarch64_get_vec_float (cpu, vn, i));
	}
    }
}

static void
do_vec_add (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = full/half selector
     instr[29,24] = 001110
     instr[23,22] = size: 00=> 8-bit, 01=> 16-bit, 10=> 32-bit, 11=> 64-bit
     instr[21]    = 1
     instr[20,16] = Vn
     instr[15,10] = 100001
     instr[9,5]   = Vm
     instr[4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30, 30);

  NYI_assert (29, 24, 0x0E);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x21);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 22))
    {
    case 0:
      for (i = 0; i < (full ? 16 : 8); i++)
	aarch64_set_vec_u8 (cpu, vd, i, aarch64_get_vec_u8 (cpu, vn, i)
			    + aarch64_get_vec_u8 (cpu, vm, i));
      return;

    case 1:
      for (i = 0; i < (full ? 8 : 4); i++)
	aarch64_set_vec_u16 (cpu, vd, i, aarch64_get_vec_u16 (cpu, vn, i)
			     + aarch64_get_vec_u16 (cpu, vm, i));
      return;

    case 2:
      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_u32 (cpu, vd, i, aarch64_get_vec_u32 (cpu, vn, i)
			     + aarch64_get_vec_u32 (cpu, vm, i));
      return;

    case 3:
      if (! full)
	HALT_UNALLOC;
      aarch64_set_vec_u64 (cpu, vd, 0, aarch64_get_vec_u64 (cpu, vn, 0)
			   + aarch64_get_vec_u64 (cpu, vm, 0));
      aarch64_set_vec_u64 (cpu, vd, 1,
			   aarch64_get_vec_u64 (cpu, vn, 1)
			   + aarch64_get_vec_u64 (cpu, vm, 1));
      return;
    }
}

static void
do_vec_mul (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = full/half selector
     instr[29,24] = 00 1110
     instr[23,22] = size: 00=> 8-bit, 01=> 16-bit, 10=> 32-bit
     instr[21]    = 1
     instr[20,16] = Vn
     instr[15,10] = 10 0111
     instr[9,5]   = Vm
     instr[4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30, 30);
  int      bias = 0;

  NYI_assert (29, 24, 0x0E);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x27);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 22))
    {
    case 0:
      DO_VEC_WIDENING_MUL (full ? 16 : 8, uint8_t, u8, u8);
      return;

    case 1:
      DO_VEC_WIDENING_MUL (full ? 8 : 4, uint16_t, u16, u16);
      return;

    case 2:
      DO_VEC_WIDENING_MUL (full ? 4 : 2, uint32_t, u32, u32);
      return;

    case 3:
      HALT_UNALLOC;
    }
}

static void
do_vec_MLA (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = full/half selector
     instr[29,24] = 00 1110
     instr[23,22] = size: 00=> 8-bit, 01=> 16-bit, 10=> 32-bit
     instr[21]    = 1
     instr[20,16] = Vn
     instr[15,10] = 1001 01
     instr[9,5]   = Vm
     instr[4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30, 30);

  NYI_assert (29, 24, 0x0E);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x25);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 22))
    {
    case 0:
      for (i = 0; i < (full ? 16 : 8); i++)
	aarch64_set_vec_u8 (cpu, vd, i,
			    aarch64_get_vec_u8 (cpu, vd, i)
			    + (aarch64_get_vec_u8 (cpu, vn, i)
			       * aarch64_get_vec_u8 (cpu, vm, i)));
      return;

    case 1:
      for (i = 0; i < (full ? 8 : 4); i++)
	aarch64_set_vec_u16 (cpu, vd, i,
			     aarch64_get_vec_u16 (cpu, vd, i)
			     + (aarch64_get_vec_u16 (cpu, vn, i)
				* aarch64_get_vec_u16 (cpu, vm, i)));
      return;

    case 2:
      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_u32 (cpu, vd, i,
			     aarch64_get_vec_u32 (cpu, vd, i)
			     + (aarch64_get_vec_u32 (cpu, vn, i)
				* aarch64_get_vec_u32 (cpu, vm, i)));
      return;

    default:
      HALT_UNALLOC;
    }
}

static float
fmaxnm (float a, float b)
{
  if (! isnan (a))
    {
      if (! isnan (b))
	return a > b ? a : b;
      return a;
    }
  else if (! isnan (b))
    return b;
  return a;
}

static float
fminnm (float a, float b)
{
  if (! isnan (a))
    {
      if (! isnan (b))
	return a < b ? a : b;
      return a;
    }
  else if (! isnan (b))
    return b;
  return a;
}

static double
dmaxnm (double a, double b)
{
  if (! isnan (a))
    {
      if (! isnan (b))
	return a > b ? a : b;
      return a;
    }
  else if (! isnan (b))
    return b;
  return a;
}

static double
dminnm (double a, double b)
{
  if (! isnan (a))
    {
      if (! isnan (b))
	return a < b ? a : b;
      return a;
    }
  else if (! isnan (b))
    return b;
  return a;
}

static void
do_vec_FminmaxNMP (sim_cpu *cpu)
{
  /* instr [31]    = 0
     instr [30]    = half (0)/full (1)
     instr [29,24] = 10 1110
     instr [23]    = max(0)/min(1)
     instr [22]    = float (0)/double (1)
     instr [21]    = 1
     instr [20,16] = Vn
     instr [15,10] = 1100 01
     instr [9,5]   = Vm
     instr [4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  int      full = INSTR (30, 30);

  NYI_assert (29, 24, 0x2E);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x31);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      double (* fn)(double, double) = INSTR (23, 23)
	? dminnm : dmaxnm;

      if (! full)
	HALT_NYI;
      aarch64_set_vec_double (cpu, vd, 0,
			      fn (aarch64_get_vec_double (cpu, vn, 0),
				  aarch64_get_vec_double (cpu, vn, 1)));
      aarch64_set_vec_double (cpu, vd, 0,
			      fn (aarch64_get_vec_double (cpu, vm, 0),
				  aarch64_get_vec_double (cpu, vm, 1)));
    }
  else
    {
      float (* fn)(float, float) = INSTR (23, 23)
	? fminnm : fmaxnm;

      aarch64_set_vec_float (cpu, vd, 0,
			     fn (aarch64_get_vec_float (cpu, vn, 0),
				 aarch64_get_vec_float (cpu, vn, 1)));
      if (full)
	aarch64_set_vec_float (cpu, vd, 1,
			       fn (aarch64_get_vec_float (cpu, vn, 2),
				   aarch64_get_vec_float (cpu, vn, 3)));

      aarch64_set_vec_float (cpu, vd, (full ? 2 : 1),
			     fn (aarch64_get_vec_float (cpu, vm, 0),
				 aarch64_get_vec_float (cpu, vm, 1)));
      if (full)
	aarch64_set_vec_float (cpu, vd, 3,
			       fn (aarch64_get_vec_float (cpu, vm, 2),
				   aarch64_get_vec_float (cpu, vm, 3)));
    }
}

static void
do_vec_AND (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half (0)/full (1)
     instr[29,21] = 001110001
     instr[20,16] = Vm
     instr[15,10] = 000111
     instr[9,5]   = Vn
     instr[4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30, 30);

  NYI_assert (29, 21, 0x071);
  NYI_assert (15, 10, 0x07);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  for (i = 0; i < (full ? 4 : 2); i++)
    aarch64_set_vec_u32 (cpu, vd, i,
			 aarch64_get_vec_u32 (cpu, vn, i)
			 & aarch64_get_vec_u32 (cpu, vm, i));
}

static void
do_vec_BSL (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half (0)/full (1)
     instr[29,21] = 101110011
     instr[20,16] = Vm
     instr[15,10] = 000111
     instr[9,5]   = Vn
     instr[4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30, 30);

  NYI_assert (29, 21, 0x173);
  NYI_assert (15, 10, 0x07);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  for (i = 0; i < (full ? 16 : 8); i++)
    aarch64_set_vec_u8 (cpu, vd, i,
			(    aarch64_get_vec_u8 (cpu, vd, i)
			   & aarch64_get_vec_u8 (cpu, vn, i))
			| ((~ aarch64_get_vec_u8 (cpu, vd, i))
			   & aarch64_get_vec_u8 (cpu, vm, i)));
}

static void
do_vec_EOR (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half (0)/full (1)
     instr[29,21] = 10 1110 001
     instr[20,16] = Vm
     instr[15,10] = 000111
     instr[9,5]   = Vn
     instr[4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30, 30);

  NYI_assert (29, 21, 0x171);
  NYI_assert (15, 10, 0x07);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  for (i = 0; i < (full ? 4 : 2); i++)
    aarch64_set_vec_u32 (cpu, vd, i,
			 aarch64_get_vec_u32 (cpu, vn, i)
			 ^ aarch64_get_vec_u32 (cpu, vm, i));
}

static void
do_vec_bit (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half (0)/full (1)
     instr[29,23] = 10 1110 1
     instr[22]    = BIT (0) / BIF (1)
     instr[21]    = 1
     instr[20,16] = Vm
     instr[15,10] = 0001 11
     instr[9,5]   = Vn
     instr[4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned full = INSTR (30, 30);
  unsigned test_false = INSTR (22, 22);
  unsigned i;

  NYI_assert (29, 23, 0x5D);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x07);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  for (i = 0; i < (full ? 4 : 2); i++)
    {
      uint32_t vd_val = aarch64_get_vec_u32 (cpu, vd, i);
      uint32_t vn_val = aarch64_get_vec_u32 (cpu, vn, i);
      uint32_t vm_val = aarch64_get_vec_u32 (cpu, vm, i);
      if (test_false)
	aarch64_set_vec_u32 (cpu, vd, i,
			     (vd_val & vm_val) | (vn_val & ~vm_val));
      else
	aarch64_set_vec_u32 (cpu, vd, i,
			     (vd_val & ~vm_val) | (vn_val & vm_val));
    }
}

static void
do_vec_ORN (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half (0)/full (1)
     instr[29,21] = 00 1110 111
     instr[20,16] = Vm
     instr[15,10] = 00 0111
     instr[9,5]   = Vn
     instr[4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30, 30);

  NYI_assert (29, 21, 0x077);
  NYI_assert (15, 10, 0x07);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  for (i = 0; i < (full ? 16 : 8); i++)
    aarch64_set_vec_u8 (cpu, vd, i,
			aarch64_get_vec_u8 (cpu, vn, i)
			| ~ aarch64_get_vec_u8 (cpu, vm, i));
}

static void
do_vec_ORR (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half (0)/full (1)
     instr[29,21] = 00 1110 101
     instr[20,16] = Vm
     instr[15,10] = 0001 11
     instr[9,5]   = Vn
     instr[4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30, 30);

  NYI_assert (29, 21, 0x075);
  NYI_assert (15, 10, 0x07);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  for (i = 0; i < (full ? 16 : 8); i++)
    aarch64_set_vec_u8 (cpu, vd, i,
			aarch64_get_vec_u8 (cpu, vn, i)
			| aarch64_get_vec_u8 (cpu, vm, i));
}

static void
do_vec_BIC (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half (0)/full (1)
     instr[29,21] = 00 1110 011
     instr[20,16] = Vm
     instr[15,10] = 00 0111
     instr[9,5]   = Vn
     instr[4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30, 30);

  NYI_assert (29, 21, 0x073);
  NYI_assert (15, 10, 0x07);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  for (i = 0; i < (full ? 16 : 8); i++)
    aarch64_set_vec_u8 (cpu, vd, i,
			aarch64_get_vec_u8 (cpu, vn, i)
			& ~ aarch64_get_vec_u8 (cpu, vm, i));
}

static void
do_vec_XTN (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = first part (0)/ second part (1)
     instr[29,24] = 00 1110
     instr[23,22] = size: byte(00), half(01), word (10)
     instr[21,10] = 1000 0100 1010
     instr[9,5]   = Vs
     instr[4,0]   = Vd.  */

  unsigned vs = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned bias = INSTR (30, 30);
  unsigned i;

  NYI_assert (29, 24, 0x0E);
  NYI_assert (21, 10, 0x84A);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 22))
    {
    case 0:
      for (i = 0; i < 8; i++)
	aarch64_set_vec_u8 (cpu, vd, i + (bias * 8),
			    aarch64_get_vec_u16 (cpu, vs, i));
      return;

    case 1:
      for (i = 0; i < 4; i++)
	aarch64_set_vec_u16 (cpu, vd, i + (bias * 4),
			     aarch64_get_vec_u32 (cpu, vs, i));
      return;

    case 2:
      for (i = 0; i < 2; i++)
	aarch64_set_vec_u32 (cpu, vd, i + (bias * 2),
			     aarch64_get_vec_u64 (cpu, vs, i));
      return;
    }
}

/* Return the number of bits set in the input value.  */
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
# define popcount __builtin_popcount
#else
static int
popcount (unsigned char x)
{
  static const unsigned char popcnt[16] =
    {
      0, 1, 1, 2,
      1, 2, 2, 3,
      1, 2, 2, 3,
      2, 3, 3, 4
    };

  /* Only counts the low 8 bits of the input as that is all we need.  */
  return popcnt[x % 16] + popcnt[x / 16];
}
#endif

static void
do_vec_CNT (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half (0)/ full (1)
     instr[29,24] = 00 1110
     instr[23,22] = size: byte(00)
     instr[21,10] = 1000 0001 0110
     instr[9,5]   = Vs
     instr[4,0]   = Vd.  */

  unsigned vs = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  int full = INSTR (30, 30);
  int size = INSTR (23, 22);
  int i;

  NYI_assert (29, 24, 0x0E);
  NYI_assert (21, 10, 0x816);

  if (size != 0)
    HALT_UNALLOC;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);

  for (i = 0; i < (full ? 16 : 8); i++)
    aarch64_set_vec_u8 (cpu, vd, i,
			popcount (aarch64_get_vec_u8 (cpu, vs, i)));
}

static void
do_vec_maxv (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half(0)/full(1)
     instr[29]    = signed (0)/unsigned(1)
     instr[28,24] = 0 1110
     instr[23,22] = size: byte(00), half(01), word (10)
     instr[21]    = 1
     instr[20,17] = 1 000
     instr[16]    = max(0)/min(1)
     instr[15,10] = 1010 10
     instr[9,5]   = V source
     instr[4.0]   = R dest.  */

  unsigned vs = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  unsigned full = INSTR (30, 30);
  unsigned i;

  NYI_assert (28, 24, 0x0E);
  NYI_assert (21, 21, 1);
  NYI_assert (20, 17, 8);
  NYI_assert (15, 10, 0x2A);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch ((INSTR (29, 29) << 1) | INSTR (16, 16))
    {
    case 0: /* SMAXV.  */
       {
	int64_t smax;
	switch (INSTR (23, 22))
	  {
	  case 0:
	    smax = aarch64_get_vec_s8 (cpu, vs, 0);
	    for (i = 1; i < (full ? 16 : 8); i++)
	      smax = max (smax, aarch64_get_vec_s8 (cpu, vs, i));
	    break;
	  case 1:
	    smax = aarch64_get_vec_s16 (cpu, vs, 0);
	    for (i = 1; i < (full ? 8 : 4); i++)
	      smax = max (smax, aarch64_get_vec_s16 (cpu, vs, i));
	    break;
	  case 2:
	    smax = aarch64_get_vec_s32 (cpu, vs, 0);
	    for (i = 1; i < (full ? 4 : 2); i++)
	      smax = max (smax, aarch64_get_vec_s32 (cpu, vs, i));
	    break;
	  case 3:
	    HALT_UNALLOC;
	  }
	aarch64_set_reg_s64 (cpu, rd, NO_SP, smax);
	return;
      }

    case 1: /* SMINV.  */
      {
	int64_t smin;
	switch (INSTR (23, 22))
	  {
	  case 0:
	    smin = aarch64_get_vec_s8 (cpu, vs, 0);
	    for (i = 1; i < (full ? 16 : 8); i++)
	      smin = min (smin, aarch64_get_vec_s8 (cpu, vs, i));
	    break;
	  case 1:
	    smin = aarch64_get_vec_s16 (cpu, vs, 0);
	    for (i = 1; i < (full ? 8 : 4); i++)
	      smin = min (smin, aarch64_get_vec_s16 (cpu, vs, i));
	    break;
	  case 2:
	    smin = aarch64_get_vec_s32 (cpu, vs, 0);
	    for (i = 1; i < (full ? 4 : 2); i++)
	      smin = min (smin, aarch64_get_vec_s32 (cpu, vs, i));
	    break;

	  case 3:
	    HALT_UNALLOC;
	  }
	aarch64_set_reg_s64 (cpu, rd, NO_SP, smin);
	return;
      }

    case 2: /* UMAXV.  */
      {
	uint64_t umax;
	switch (INSTR (23, 22))
	  {
	  case 0:
	    umax = aarch64_get_vec_u8 (cpu, vs, 0);
	    for (i = 1; i < (full ? 16 : 8); i++)
	      umax = max (umax, aarch64_get_vec_u8 (cpu, vs, i));
	    break;
	  case 1:
	    umax = aarch64_get_vec_u16 (cpu, vs, 0);
	    for (i = 1; i < (full ? 8 : 4); i++)
	      umax = max (umax, aarch64_get_vec_u16 (cpu, vs, i));
	    break;
	  case 2:
	    umax = aarch64_get_vec_u32 (cpu, vs, 0);
	    for (i = 1; i < (full ? 4 : 2); i++)
	      umax = max (umax, aarch64_get_vec_u32 (cpu, vs, i));
	    break;

	  case 3:
	    HALT_UNALLOC;
	  }
	aarch64_set_reg_u64 (cpu, rd, NO_SP, umax);
	return;
      }

    case 3: /* UMINV.  */
      {
	uint64_t umin;
	switch (INSTR (23, 22))
	  {
	  case 0:
	    umin = aarch64_get_vec_u8 (cpu, vs, 0);
	    for (i = 1; i < (full ? 16 : 8); i++)
	      umin = min (umin, aarch64_get_vec_u8 (cpu, vs, i));
	    break;
	  case 1:
	    umin = aarch64_get_vec_u16 (cpu, vs, 0);
	    for (i = 1; i < (full ? 8 : 4); i++)
	      umin = min (umin, aarch64_get_vec_u16 (cpu, vs, i));
	    break;
	  case 2:
	    umin = aarch64_get_vec_u32 (cpu, vs, 0);
	    for (i = 1; i < (full ? 4 : 2); i++)
	      umin = min (umin, aarch64_get_vec_u32 (cpu, vs, i));
	    break;

	  case 3:
	    HALT_UNALLOC;
	  }
	aarch64_set_reg_u64 (cpu, rd, NO_SP, umin);
	return;
      }
    }
}

static void
do_vec_fminmaxV (sim_cpu *cpu)
{
  /* instr[31,24] = 0110 1110
     instr[23]    = max(0)/min(1)
     instr[22,14] = 011 0000 11
     instr[13,12] = nm(00)/normal(11)
     instr[11,10] = 10
     instr[9,5]   = V source
     instr[4.0]   = R dest.  */

  unsigned vs = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  unsigned i;
  float res   = aarch64_get_vec_float (cpu, vs, 0);

  NYI_assert (31, 24, 0x6E);
  NYI_assert (22, 14, 0x0C3);
  NYI_assert (11, 10, 2);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (23, 23))
    {
      switch (INSTR (13, 12))
	{
	case 0: /* FMNINNMV.  */
	  for (i = 1; i < 4; i++)
	    res = fminnm (res, aarch64_get_vec_float (cpu, vs, i));
	  break;

	case 3: /* FMINV.  */
	  for (i = 1; i < 4; i++)
	    res = min (res, aarch64_get_vec_float (cpu, vs, i));
	  break;

	default:
	  HALT_NYI;
	}
    }
  else
    {
      switch (INSTR (13, 12))
	{
	case 0: /* FMNAXNMV.  */
	  for (i = 1; i < 4; i++)
	    res = fmaxnm (res, aarch64_get_vec_float (cpu, vs, i));
	  break;

	case 3: /* FMAXV.  */
	  for (i = 1; i < 4; i++)
	    res = max (res, aarch64_get_vec_float (cpu, vs, i));
	  break;

	default:
	  HALT_NYI;
	}
    }

  aarch64_set_FP_float (cpu, rd, res);
}

static void
do_vec_Fminmax (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half(0)/full(1)
     instr[29,24] = 00 1110
     instr[23]    = max(0)/min(1)
     instr[22]    = float(0)/double(1)
     instr[21]    = 1
     instr[20,16] = Vm
     instr[15,14] = 11
     instr[13,12] = nm(00)/normal(11)
     instr[11,10] = 01
     instr[9,5]   = Vn
     instr[4,0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned full = INSTR (30, 30);
  unsigned min = INSTR (23, 23);
  unsigned i;

  NYI_assert (29, 24, 0x0E);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 14, 3);
  NYI_assert (11, 10, 1);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      double (* func)(double, double);

      if (! full)
	HALT_NYI;

      if (INSTR (13, 12) == 0)
	func = min ? dminnm : dmaxnm;
      else if (INSTR (13, 12) == 3)
	func = min ? fmin : fmax;
      else
	HALT_NYI;

      for (i = 0; i < 2; i++)
	aarch64_set_vec_double (cpu, vd, i,
				func (aarch64_get_vec_double (cpu, vn, i),
				      aarch64_get_vec_double (cpu, vm, i)));
    }
  else
    {
      float (* func)(float, float);

      if (INSTR (13, 12) == 0)
	func = min ? fminnm : fmaxnm;
      else if (INSTR (13, 12) == 3)
	func = min ? fminf : fmaxf;
      else
	HALT_NYI;

      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_float (cpu, vd, i,
			       func (aarch64_get_vec_float (cpu, vn, i),
				     aarch64_get_vec_float (cpu, vm, i)));
    }
}

static void
do_vec_SCVTF (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = Q
     instr[29,23] = 00 1110 0
     instr[22]    = float(0)/double(1)
     instr[21,10] = 10 0001 1101 10
     instr[9,5]   = Vn
     instr[4,0]   = Vd.  */

  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned full = INSTR (30, 30);
  unsigned size = INSTR (22, 22);
  unsigned i;

  NYI_assert (29, 23, 0x1C);
  NYI_assert (21, 10, 0x876);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (size)
    {
      if (! full)
	HALT_UNALLOC;

      for (i = 0; i < 2; i++)
	{
	  double val = (double) aarch64_get_vec_u64 (cpu, vn, i);
	  aarch64_set_vec_double (cpu, vd, i, val);
	}
    }
  else
    {
      for (i = 0; i < (full ? 4 : 2); i++)
	{
	  float val = (float) aarch64_get_vec_u32 (cpu, vn, i);
	  aarch64_set_vec_float (cpu, vd, i, val);
	}
    }
}

#define VEC_CMP(SOURCE, CMP)						\
  do									\
    {									\
      switch (size)							\
	{								\
	case 0:								\
	  for (i = 0; i < (full ? 16 : 8); i++)				\
	    aarch64_set_vec_u8 (cpu, vd, i,				\
				aarch64_get_vec_##SOURCE##8 (cpu, vn, i) \
				CMP					\
				aarch64_get_vec_##SOURCE##8 (cpu, vm, i) \
				? -1 : 0);				\
	  return;							\
	case 1:								\
	  for (i = 0; i < (full ? 8 : 4); i++)				\
	    aarch64_set_vec_u16 (cpu, vd, i,				\
				 aarch64_get_vec_##SOURCE##16 (cpu, vn, i) \
				 CMP					\
				 aarch64_get_vec_##SOURCE##16 (cpu, vm, i) \
				 ? -1 : 0);				\
	  return;							\
	case 2:								\
	  for (i = 0; i < (full ? 4 : 2); i++)				\
	    aarch64_set_vec_u32 (cpu, vd, i, \
				 aarch64_get_vec_##SOURCE##32 (cpu, vn, i) \
				 CMP					\
				 aarch64_get_vec_##SOURCE##32 (cpu, vm, i) \
				 ? -1 : 0);				\
	  return;							\
	case 3:								\
	  if (! full)							\
	    HALT_UNALLOC;						\
	  for (i = 0; i < 2; i++)					\
	    aarch64_set_vec_u64 (cpu, vd, i, \
				 aarch64_get_vec_##SOURCE##64 (cpu, vn, i) \
				 CMP					\
				 aarch64_get_vec_##SOURCE##64 (cpu, vm, i) \
				 ? -1ULL : 0);				\
	  return;							\
	default:							\
	  HALT_UNALLOC;							\
	}								\
    }									\
  while (0)

#define VEC_CMP0(SOURCE, CMP)						\
  do									\
    {									\
      switch (size)							\
	{								\
	case 0:								\
	  for (i = 0; i < (full ? 16 : 8); i++)				\
	    aarch64_set_vec_u8 (cpu, vd, i,				\
				aarch64_get_vec_##SOURCE##8 (cpu, vn, i) \
				CMP 0 ? -1 : 0);			\
	  return;							\
	case 1:								\
	  for (i = 0; i < (full ? 8 : 4); i++)				\
	    aarch64_set_vec_u16 (cpu, vd, i,				\
				 aarch64_get_vec_##SOURCE##16 (cpu, vn, i) \
				 CMP 0 ? -1 : 0);			\
	  return;							\
	case 2:								\
	  for (i = 0; i < (full ? 4 : 2); i++)				\
	    aarch64_set_vec_u32 (cpu, vd, i,				\
				 aarch64_get_vec_##SOURCE##32 (cpu, vn, i) \
				 CMP 0 ? -1 : 0);			\
	  return;							\
	case 3:								\
	  if (! full)							\
	    HALT_UNALLOC;						\
	  for (i = 0; i < 2; i++)					\
	    aarch64_set_vec_u64 (cpu, vd, i,				\
				 aarch64_get_vec_##SOURCE##64 (cpu, vn, i) \
				 CMP 0 ? -1ULL : 0);			\
	  return;							\
	default:							\
	  HALT_UNALLOC;							\
	}								\
    }									\
  while (0)

#define VEC_FCMP0(CMP)							\
  do									\
    {									\
      if (vm != 0)							\
	HALT_NYI;							\
      if (INSTR (22, 22))						\
	{								\
	  if (! full)							\
	    HALT_NYI;							\
	  for (i = 0; i < 2; i++)					\
	    aarch64_set_vec_u64 (cpu, vd, i,				\
				 aarch64_get_vec_double (cpu, vn, i)	\
				 CMP 0.0 ? -1 : 0);			\
	}								\
      else								\
	{								\
	  for (i = 0; i < (full ? 4 : 2); i++)				\
	    aarch64_set_vec_u32 (cpu, vd, i,				\
				 aarch64_get_vec_float (cpu, vn, i)	\
				 CMP 0.0 ? -1 : 0);			\
	}								\
      return;								\
    }									\
  while (0)

#define VEC_FCMP(CMP)							\
  do									\
    {									\
      if (INSTR (22, 22))						\
	{								\
	  if (! full)							\
	    HALT_NYI;							\
	  for (i = 0; i < 2; i++)					\
	    aarch64_set_vec_u64 (cpu, vd, i,				\
				 aarch64_get_vec_double (cpu, vn, i)	\
				 CMP					\
				 aarch64_get_vec_double (cpu, vm, i)	\
				 ? -1 : 0);				\
	}								\
      else								\
	{								\
	  for (i = 0; i < (full ? 4 : 2); i++)				\
	    aarch64_set_vec_u32 (cpu, vd, i,				\
				 aarch64_get_vec_float (cpu, vn, i)	\
				 CMP					\
				 aarch64_get_vec_float (cpu, vm, i)	\
				 ? -1 : 0);				\
	}								\
      return;								\
    }									\
  while (0)

static void
do_vec_compare (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half(0)/full(1)
     instr[29]    = part-of-comparison-type
     instr[28,24] = 0 1110
     instr[23,22] = size of integer compares: byte(00), half(01), word (10), long (11)
                    type of float compares: single (-0) / double (-1)
     instr[21]    = 1
     instr[20,16] = Vm or 00000 (compare vs 0)
     instr[15,10] = part-of-comparison-type
     instr[9,5]   = Vn
     instr[4.0]   = Vd.  */

  int full = INSTR (30, 30);
  int size = INSTR (23, 22);
  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;

  NYI_assert (28, 24, 0x0E);
  NYI_assert (21, 21, 1);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if ((INSTR (11, 11)
       && INSTR (14, 14))
      || ((INSTR (11, 11) == 0
	   && INSTR (10, 10) == 0)))
    {
      /* A compare vs 0.  */
      if (vm != 0)
	{
	  if (INSTR (15, 10) == 0x2A)
	    do_vec_maxv (cpu);
	  else if (INSTR (15, 10) == 0x32
		   || INSTR (15, 10) == 0x3E)
	    do_vec_fminmaxV (cpu);
	  else if (INSTR (29, 23) == 0x1C
		   && INSTR (21, 10) == 0x876)
	    do_vec_SCVTF (cpu);
	  else
	    HALT_NYI;
	  return;
	}
    }

  if (INSTR (14, 14))
    {
      /* A floating point compare.  */
      unsigned decode = (INSTR (29, 29) << 5) | (INSTR (23, 23) << 4)
	| INSTR (13, 10);

      NYI_assert (15, 15, 1);

      switch (decode)
	{
	case /* 0b010010: GT#0 */ 0x12: VEC_FCMP0 (>);
	case /* 0b110010: GE#0 */ 0x32: VEC_FCMP0 (>=);
	case /* 0b010110: EQ#0 */ 0x16: VEC_FCMP0 (==);
	case /* 0b110110: LE#0 */ 0x36: VEC_FCMP0 (<=);
	case /* 0b011010: LT#0 */ 0x1A: VEC_FCMP0 (<);
	case /* 0b111001: GT */   0x39: VEC_FCMP  (>);
	case /* 0b101001: GE */   0x29: VEC_FCMP  (>=);
	case /* 0b001001: EQ */   0x09: VEC_FCMP  (==);

	default:
	  HALT_NYI;
	}
    }
  else
    {
      unsigned decode = (INSTR (29, 29) << 6) | INSTR (15, 10);

      switch (decode)
	{
	case 0x0D: /* 0001101 GT */     VEC_CMP  (s, > );
	case 0x0F: /* 0001111 GE */     VEC_CMP  (s, >= );
	case 0x22: /* 0100010 GT #0 */  VEC_CMP0 (s, > );
	case 0x23: /* 0100011 TST */	VEC_CMP  (u, & );
	case 0x26: /* 0100110 EQ #0 */  VEC_CMP0 (s, == );
	case 0x2A: /* 0101010 LT #0 */  VEC_CMP0 (s, < );
	case 0x4D: /* 1001101 HI */     VEC_CMP  (u, > );
	case 0x4F: /* 1001111 HS */     VEC_CMP  (u, >= );
	case 0x62: /* 1100010 GE #0 */  VEC_CMP0 (s, >= );
	case 0x63: /* 1100011 EQ */     VEC_CMP  (u, == );
	case 0x66: /* 1100110 LE #0 */  VEC_CMP0 (s, <= );
	default:
	  if (vm == 0)
	    HALT_NYI;
	  do_vec_maxv (cpu);
	}
    }
}

static void
do_vec_SSHL (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = first part (0)/ second part (1)
     instr[29,24] = 00 1110
     instr[23,22] = size: byte(00), half(01), word (10), long (11)
     instr[21]    = 1
     instr[20,16] = Vm
     instr[15,10] = 0100 01
     instr[9,5]   = Vn
     instr[4,0]   = Vd.  */

  unsigned full = INSTR (30, 30);
  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  signed int shift;

  NYI_assert (29, 24, 0x0E);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x11);

  /* FIXME: What is a signed shift left in this context ?.  */

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 22))
    {
    case 0:
      for (i = 0; i < (full ? 16 : 8); i++)
	{
	  shift = aarch64_get_vec_s8 (cpu, vm, i);
	  if (shift >= 0)
	    aarch64_set_vec_s8 (cpu, vd, i, aarch64_get_vec_s8 (cpu, vn, i)
				<< shift);
	  else
	    aarch64_set_vec_s8 (cpu, vd, i, aarch64_get_vec_s8 (cpu, vn, i)
				>> - shift);
	}
      return;

    case 1:
      for (i = 0; i < (full ? 8 : 4); i++)
	{
	  shift = aarch64_get_vec_s8 (cpu, vm, i * 2);
	  if (shift >= 0)
	    aarch64_set_vec_s16 (cpu, vd, i, aarch64_get_vec_s16 (cpu, vn, i)
				 << shift);
	  else
	    aarch64_set_vec_s16 (cpu, vd, i, aarch64_get_vec_s16 (cpu, vn, i)
				 >> - shift);
	}
      return;

    case 2:
      for (i = 0; i < (full ? 4 : 2); i++)
	{
	  shift = aarch64_get_vec_s8 (cpu, vm, i * 4);
	  if (shift >= 0)
	    aarch64_set_vec_s32 (cpu, vd, i, aarch64_get_vec_s32 (cpu, vn, i)
				 << shift);
	  else
	    aarch64_set_vec_s32 (cpu, vd, i, aarch64_get_vec_s32 (cpu, vn, i)
				 >> - shift);
	}
      return;

    case 3:
      if (! full)
	HALT_UNALLOC;
      for (i = 0; i < 2; i++)
	{
	  shift = aarch64_get_vec_s8 (cpu, vm, i * 8);
	  if (shift >= 0)
	    aarch64_set_vec_s64 (cpu, vd, i, aarch64_get_vec_s64 (cpu, vn, i)
				 << shift);
	  else
	    aarch64_set_vec_s64 (cpu, vd, i, aarch64_get_vec_s64 (cpu, vn, i)
				 >> - shift);
	}
      return;
    }
}

static void
do_vec_USHL (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = first part (0)/ second part (1)
     instr[29,24] = 10 1110
     instr[23,22] = size: byte(00), half(01), word (10), long (11)
     instr[21]    = 1
     instr[20,16] = Vm
     instr[15,10] = 0100 01
     instr[9,5]   = Vn
     instr[4,0]   = Vd  */

  unsigned full = INSTR (30, 30);
  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  signed int shift;

  NYI_assert (29, 24, 0x2E);
  NYI_assert (15, 10, 0x11);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 22))
    {
    case 0:
	for (i = 0; i < (full ? 16 : 8); i++)
	  {
	    shift = aarch64_get_vec_s8 (cpu, vm, i);
	    if (shift >= 0)
	      aarch64_set_vec_u8 (cpu, vd, i, aarch64_get_vec_u8 (cpu, vn, i)
				  << shift);
	    else
	      aarch64_set_vec_u8 (cpu, vd, i, aarch64_get_vec_u8 (cpu, vn, i)
				  >> - shift);
	  }
      return;

    case 1:
      for (i = 0; i < (full ? 8 : 4); i++)
	{
	  shift = aarch64_get_vec_s8 (cpu, vm, i * 2);
	  if (shift >= 0)
	    aarch64_set_vec_u16 (cpu, vd, i, aarch64_get_vec_u16 (cpu, vn, i)
				 << shift);
	  else
	    aarch64_set_vec_u16 (cpu, vd, i, aarch64_get_vec_u16 (cpu, vn, i)
				 >> - shift);
	}
      return;

    case 2:
      for (i = 0; i < (full ? 4 : 2); i++)
	{
	  shift = aarch64_get_vec_s8 (cpu, vm, i * 4);
	  if (shift >= 0)
	    aarch64_set_vec_u32 (cpu, vd, i, aarch64_get_vec_u32 (cpu, vn, i)
				 << shift);
	  else
	    aarch64_set_vec_u32 (cpu, vd, i, aarch64_get_vec_u32 (cpu, vn, i)
				 >> - shift);
	}
      return;

    case 3:
      if (! full)
	HALT_UNALLOC;
      for (i = 0; i < 2; i++)
	{
	  shift = aarch64_get_vec_s8 (cpu, vm, i * 8);
	  if (shift >= 0)
	    aarch64_set_vec_u64 (cpu, vd, i, aarch64_get_vec_u64 (cpu, vn, i)
				 << shift);
	  else
	    aarch64_set_vec_u64 (cpu, vd, i, aarch64_get_vec_u64 (cpu, vn, i)
				 >> - shift);
	}
      return;
    }
}

static void
do_vec_FMLA (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = full/half selector
     instr[29,23] = 0011100
     instr[22]    = size: 0=>float, 1=>double
     instr[21]    = 1
     instr[20,16] = Vn
     instr[15,10] = 1100 11
     instr[9,5]   = Vm
     instr[4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30, 30);

  NYI_assert (29, 23, 0x1C);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x33);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      if (! full)
	HALT_UNALLOC;
      for (i = 0; i < 2; i++)
	aarch64_set_vec_double (cpu, vd, i,
				aarch64_get_vec_double (cpu, vn, i) *
				aarch64_get_vec_double (cpu, vm, i) +
				aarch64_get_vec_double (cpu, vd, i));
    }
  else
    {
      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_float (cpu, vd, i,
			       aarch64_get_vec_float (cpu, vn, i) *
			       aarch64_get_vec_float (cpu, vm, i) +
			       aarch64_get_vec_float (cpu, vd, i));
    }
}

static void
do_vec_max (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = full/half selector
     instr[29]    = SMAX (0) / UMAX (1)
     instr[28,24] = 0 1110
     instr[23,22] = size: 00=> 8-bit, 01=> 16-bit, 10=> 32-bit
     instr[21]    = 1
     instr[20,16] = Vn
     instr[15,10] = 0110 01
     instr[9,5]   = Vm
     instr[4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30, 30);

  NYI_assert (28, 24, 0x0E);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x19);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (29, 29))
    {
      switch (INSTR (23, 22))
	{
	case 0:
	  for (i = 0; i < (full ? 16 : 8); i++)
	    aarch64_set_vec_u8 (cpu, vd, i,
				aarch64_get_vec_u8 (cpu, vn, i)
				> aarch64_get_vec_u8 (cpu, vm, i)
				? aarch64_get_vec_u8 (cpu, vn, i)
				: aarch64_get_vec_u8 (cpu, vm, i));
	  return;

	case 1:
	  for (i = 0; i < (full ? 8 : 4); i++)
	    aarch64_set_vec_u16 (cpu, vd, i,
				 aarch64_get_vec_u16 (cpu, vn, i)
				 > aarch64_get_vec_u16 (cpu, vm, i)
				 ? aarch64_get_vec_u16 (cpu, vn, i)
				 : aarch64_get_vec_u16 (cpu, vm, i));
	  return;

	case 2:
	  for (i = 0; i < (full ? 4 : 2); i++)
	    aarch64_set_vec_u32 (cpu, vd, i,
				 aarch64_get_vec_u32 (cpu, vn, i)
				 > aarch64_get_vec_u32 (cpu, vm, i)
				 ? aarch64_get_vec_u32 (cpu, vn, i)
				 : aarch64_get_vec_u32 (cpu, vm, i));
	  return;

	case 3:
	  HALT_UNALLOC;
	}
    }
  else
    {
      switch (INSTR (23, 22))
	{
	case 0:
	  for (i = 0; i < (full ? 16 : 8); i++)
	    aarch64_set_vec_s8 (cpu, vd, i,
				aarch64_get_vec_s8 (cpu, vn, i)
				> aarch64_get_vec_s8 (cpu, vm, i)
				? aarch64_get_vec_s8 (cpu, vn, i)
				: aarch64_get_vec_s8 (cpu, vm, i));
	  return;

	case 1:
	  for (i = 0; i < (full ? 8 : 4); i++)
	    aarch64_set_vec_s16 (cpu, vd, i,
				 aarch64_get_vec_s16 (cpu, vn, i)
				 > aarch64_get_vec_s16 (cpu, vm, i)
				 ? aarch64_get_vec_s16 (cpu, vn, i)
				 : aarch64_get_vec_s16 (cpu, vm, i));
	  return;

	case 2:
	  for (i = 0; i < (full ? 4 : 2); i++)
	    aarch64_set_vec_s32 (cpu, vd, i,
				 aarch64_get_vec_s32 (cpu, vn, i)
				 > aarch64_get_vec_s32 (cpu, vm, i)
				 ? aarch64_get_vec_s32 (cpu, vn, i)
				 : aarch64_get_vec_s32 (cpu, vm, i));
	  return;

	case 3:
	  HALT_UNALLOC;
	}
    }
}

static void
do_vec_min (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = full/half selector
     instr[29]    = SMIN (0) / UMIN (1)
     instr[28,24] = 0 1110
     instr[23,22] = size: 00=> 8-bit, 01=> 16-bit, 10=> 32-bit
     instr[21]    = 1
     instr[20,16] = Vn
     instr[15,10] = 0110 11
     instr[9,5]   = Vm
     instr[4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30, 30);

  NYI_assert (28, 24, 0x0E);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x1B);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (29, 29))
    {
      switch (INSTR (23, 22))
	{
	case 0:
	  for (i = 0; i < (full ? 16 : 8); i++)
	    aarch64_set_vec_u8 (cpu, vd, i,
				aarch64_get_vec_u8 (cpu, vn, i)
				< aarch64_get_vec_u8 (cpu, vm, i)
				? aarch64_get_vec_u8 (cpu, vn, i)
				: aarch64_get_vec_u8 (cpu, vm, i));
	  return;

	case 1:
	  for (i = 0; i < (full ? 8 : 4); i++)
	    aarch64_set_vec_u16 (cpu, vd, i,
				 aarch64_get_vec_u16 (cpu, vn, i)
				 < aarch64_get_vec_u16 (cpu, vm, i)
				 ? aarch64_get_vec_u16 (cpu, vn, i)
				 : aarch64_get_vec_u16 (cpu, vm, i));
	  return;

	case 2:
	  for (i = 0; i < (full ? 4 : 2); i++)
	    aarch64_set_vec_u32 (cpu, vd, i,
				 aarch64_get_vec_u32 (cpu, vn, i)
				 < aarch64_get_vec_u32 (cpu, vm, i)
				 ? aarch64_get_vec_u32 (cpu, vn, i)
				 : aarch64_get_vec_u32 (cpu, vm, i));
	  return;

	case 3:
	  HALT_UNALLOC;
	}
    }
  else
    {
      switch (INSTR (23, 22))
	{
	case 0:
	  for (i = 0; i < (full ? 16 : 8); i++)
	    aarch64_set_vec_s8 (cpu, vd, i,
				aarch64_get_vec_s8 (cpu, vn, i)
				< aarch64_get_vec_s8 (cpu, vm, i)
				? aarch64_get_vec_s8 (cpu, vn, i)
				: aarch64_get_vec_s8 (cpu, vm, i));
	  return;

	case 1:
	  for (i = 0; i < (full ? 8 : 4); i++)
	    aarch64_set_vec_s16 (cpu, vd, i,
				 aarch64_get_vec_s16 (cpu, vn, i)
				 < aarch64_get_vec_s16 (cpu, vm, i)
				 ? aarch64_get_vec_s16 (cpu, vn, i)
				 : aarch64_get_vec_s16 (cpu, vm, i));
	  return;

	case 2:
	  for (i = 0; i < (full ? 4 : 2); i++)
	    aarch64_set_vec_s32 (cpu, vd, i,
				 aarch64_get_vec_s32 (cpu, vn, i)
				 < aarch64_get_vec_s32 (cpu, vm, i)
				 ? aarch64_get_vec_s32 (cpu, vn, i)
				 : aarch64_get_vec_s32 (cpu, vm, i));
	  return;

	case 3:
	  HALT_UNALLOC;
	}
    }
}

static void
do_vec_sub_long (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = lower (0) / upper (1)
     instr[29]    = signed (0) / unsigned (1)
     instr[28,24] = 0 1110
     instr[23,22] = size: bytes (00), half (01), word (10)
     instr[21]    = 1
     insrt[20,16] = Vm
     instr[15,10] = 0010 00
     instr[9,5]   = Vn
     instr[4,0]   = V dest.  */

  unsigned size = INSTR (23, 22);
  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned bias = 0;
  unsigned i;

  NYI_assert (28, 24, 0x0E);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x08);

  if (size == 3)
    HALT_UNALLOC;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (30, 29))
    {
    case 2: /* SSUBL2.  */
      bias = 2;
      ATTRIBUTE_FALLTHROUGH;
    case 0: /* SSUBL.  */
      switch (size)
	{
	case 0:
	  bias *= 3;
	  for (i = 0; i < 8; i++)
	    aarch64_set_vec_s16 (cpu, vd, i,
				 aarch64_get_vec_s8 (cpu, vn, i + bias)
				 - aarch64_get_vec_s8 (cpu, vm, i + bias));
	  break;

	case 1:
	  bias *= 2;
	  for (i = 0; i < 4; i++)
	    aarch64_set_vec_s32 (cpu, vd, i,
				 aarch64_get_vec_s16 (cpu, vn, i + bias)
				 - aarch64_get_vec_s16 (cpu, vm, i + bias));
	  break;

	case 2:
	  for (i = 0; i < 2; i++)
	    aarch64_set_vec_s64 (cpu, vd, i,
				 aarch64_get_vec_s32 (cpu, vn, i + bias)
				 - aarch64_get_vec_s32 (cpu, vm, i + bias));
	  break;

	default:
	  HALT_UNALLOC;
	}
      break;

    case 3: /* USUBL2.  */
      bias = 2;
      ATTRIBUTE_FALLTHROUGH;
    case 1: /* USUBL.  */
      switch (size)
	{
	case 0:
	  bias *= 3;
	  for (i = 0; i < 8; i++)
	    aarch64_set_vec_u16 (cpu, vd, i,
				 aarch64_get_vec_u8 (cpu, vn, i + bias)
				 - aarch64_get_vec_u8 (cpu, vm, i + bias));
	  break;

	case 1:
	  bias *= 2;
	  for (i = 0; i < 4; i++)
	    aarch64_set_vec_u32 (cpu, vd, i,
				 aarch64_get_vec_u16 (cpu, vn, i + bias)
				 - aarch64_get_vec_u16 (cpu, vm, i + bias));
	  break;

	case 2:
	  for (i = 0; i < 2; i++)
	    aarch64_set_vec_u64 (cpu, vd, i,
				 aarch64_get_vec_u32 (cpu, vn, i + bias)
				 - aarch64_get_vec_u32 (cpu, vm, i + bias));
	  break;

	default:
	  HALT_UNALLOC;
	}
      break;
    }
}

static void
do_vec_ADDP (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half(0)/full(1)
     instr[29,24] = 00 1110
     instr[23,22] = size: bytes (00), half (01), word (10), long (11)
     instr[21]    = 1
     insrt[20,16] = Vm
     instr[15,10] = 1011 11
     instr[9,5]   = Vn
     instr[4,0]   = V dest.  */

  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);
  FRegister copy_vn;
  FRegister copy_vm;
  unsigned full = INSTR (30, 30);
  unsigned size = INSTR (23, 22);
  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i, range;

  NYI_assert (29, 24, 0x0E);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x2F);

  /* Make copies of the source registers in case vd == vn/vm.  */
  copy_vn = aarch64_cpu->fr[vn];
  copy_vm = aarch64_cpu->fr[vm];

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (size)
    {
    case 0:
      range = full ? 8 : 4;
      for (i = 0; i < range; i++)
	{
	  aarch64_set_vec_u8 (cpu, vd, i,
			      copy_vn.b[i * 2] + copy_vn.b[i * 2 + 1]);
	  aarch64_set_vec_u8 (cpu, vd, i + range,
			      copy_vm.b[i * 2] + copy_vm.b[i * 2 + 1]);
	}
      return;

    case 1:
      range = full ? 4 : 2;
      for (i = 0; i < range; i++)
	{
	  aarch64_set_vec_u16 (cpu, vd, i,
			       copy_vn.h[i * 2] + copy_vn.h[i * 2 + 1]);
	  aarch64_set_vec_u16 (cpu, vd, i + range,
			       copy_vm.h[i * 2] + copy_vm.h[i * 2 + 1]);
	}
      return;

    case 2:
      range = full ? 2 : 1;
      for (i = 0; i < range; i++)
	{
	  aarch64_set_vec_u32 (cpu, vd, i,
			       copy_vn.w[i * 2] + copy_vn.w[i * 2 + 1]);
	  aarch64_set_vec_u32 (cpu, vd, i + range,
			       copy_vm.w[i * 2] + copy_vm.w[i * 2 + 1]);
	}
      return;

    case 3:
      if (! full)
	HALT_UNALLOC;
      aarch64_set_vec_u64 (cpu, vd, 0, copy_vn.v[0] + copy_vn.v[1]);
      aarch64_set_vec_u64 (cpu, vd, 1, copy_vm.v[0] + copy_vm.v[1]);
      return;
    }
}

/* Float point vector convert to longer (precision).  */
static void
do_vec_FCVTL (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half (0) / all (1)
     instr[29,23] = 00 1110 0
     instr[22]    = single (0) / double (1)
     instr[21,10] = 10 0001 0111 10
     instr[9,5]   = Rn
     instr[4,0]   = Rd.  */

  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  unsigned full = INSTR (30, 30);
  unsigned i;

  NYI_assert (31, 31, 0);
  NYI_assert (29, 23, 0x1C);
  NYI_assert (21, 10, 0x85E);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      for (i = 0; i < 2; i++)
	aarch64_set_vec_double (cpu, rd, i,
				aarch64_get_vec_float (cpu, rn, i + 2*full));
    }
  else
    {
      HALT_NYI;

#if 0
      /* TODO: Implement missing half-float support.  */
      for (i = 0; i < 4; i++)
	aarch64_set_vec_float (cpu, rd, i,
			     aarch64_get_vec_halffloat (cpu, rn, i + 4*full));
#endif
    }
}

static void
do_vec_FABS (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half(0)/full(1)
     instr[29,23] = 00 1110 1
     instr[22]    = float(0)/double(1)
     instr[21,16] = 10 0000
     instr[15,10] = 1111 10
     instr[9,5]   = Vn
     instr[4,0]   = Vd.  */

  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned full = INSTR (30, 30);
  unsigned i;

  NYI_assert (29, 23, 0x1D);
  NYI_assert (21, 10, 0x83E);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      if (! full)
	HALT_NYI;

      for (i = 0; i < 2; i++)
	aarch64_set_vec_double (cpu, vd, i,
				fabs (aarch64_get_vec_double (cpu, vn, i)));
    }
  else
    {
      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_float (cpu, vd, i,
			       fabsf (aarch64_get_vec_float (cpu, vn, i)));
    }
}

static void
do_vec_FCVTZS (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half (0) / all (1)
     instr[29,23] = 00 1110 1
     instr[22]    = single (0) / double (1)
     instr[21,10] = 10 0001 1011 10
     instr[9,5]   = Rn
     instr[4,0]   = Rd.  */

  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  unsigned full = INSTR (30, 30);
  unsigned i;

  NYI_assert (31, 31, 0);
  NYI_assert (29, 23, 0x1D);
  NYI_assert (21, 10, 0x86E);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      if (! full)
	HALT_UNALLOC;

      for (i = 0; i < 2; i++)
	aarch64_set_vec_s64 (cpu, rd, i,
			     (int64_t) aarch64_get_vec_double (cpu, rn, i));
    }
  else
    for (i = 0; i < (full ? 4 : 2); i++)
      aarch64_set_vec_s32 (cpu, rd, i,
			   (int32_t) aarch64_get_vec_float (cpu, rn, i));
}

static void
do_vec_REV64 (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = full/half
     instr[29,24] = 00 1110 
     instr[23,22] = size
     instr[21,10] = 10 0000 0000 10
     instr[9,5]   = Rn
     instr[4,0]   = Rd.  */

  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  unsigned size = INSTR (23, 22);
  unsigned full = INSTR (30, 30);
  unsigned i;
  FRegister val;

  NYI_assert (29, 24, 0x0E);
  NYI_assert (21, 10, 0x802);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (size)
    {
    case 0:
      for (i = 0; i < (full ? 16 : 8); i++)
	val.b[i ^ 0x7] = aarch64_get_vec_u8 (cpu, rn, i);
      break;

    case 1:
      for (i = 0; i < (full ? 8 : 4); i++)
	val.h[i ^ 0x3] = aarch64_get_vec_u16 (cpu, rn, i);
      break;

    case 2:
      for (i = 0; i < (full ? 4 : 2); i++)
	val.w[i ^ 0x1] = aarch64_get_vec_u32 (cpu, rn, i);
      break;
      
    case 3:
      HALT_UNALLOC;
    }

  aarch64_set_vec_u64 (cpu, rd, 0, val.v[0]);
  if (full)
    aarch64_set_vec_u64 (cpu, rd, 1, val.v[1]);
}

static void
do_vec_REV16 (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = full/half
     instr[29,24] = 00 1110 
     instr[23,22] = size
     instr[21,10] = 10 0000 0001 10
     instr[9,5]   = Rn
     instr[4,0]   = Rd.  */

  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  unsigned size = INSTR (23, 22);
  unsigned full = INSTR (30, 30);
  unsigned i;
  FRegister val;

  NYI_assert (29, 24, 0x0E);
  NYI_assert (21, 10, 0x806);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (size)
    {
    case 0:
      for (i = 0; i < (full ? 16 : 8); i++)
	val.b[i ^ 0x1] = aarch64_get_vec_u8 (cpu, rn, i);
      break;

    default:
      HALT_UNALLOC;
    }

  aarch64_set_vec_u64 (cpu, rd, 0, val.v[0]);
  if (full)
    aarch64_set_vec_u64 (cpu, rd, 1, val.v[1]);
}

static void
do_vec_op1 (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half/full
     instr[29,24] = 00 1110
     instr[23,21] = ???
     instr[20,16] = Vm
     instr[15,10] = sub-opcode
     instr[9,5]   = Vn
     instr[4,0]   = Vd  */
  NYI_assert (29, 24, 0x0E);

  if (INSTR (21, 21) == 0)
    {
      if (INSTR (23, 22) == 0)
	{
	  if (INSTR (30, 30) == 1
	      && INSTR (17, 14) == 0
	      && INSTR (12, 10) == 7)
	    return do_vec_ins_2 (cpu);

	  switch (INSTR (15, 10))
	    {
	    case 0x01: do_vec_DUP_vector_into_vector (cpu); return;
	    case 0x03: do_vec_DUP_scalar_into_vector (cpu); return;
	    case 0x07: do_vec_INS (cpu); return;
	    case 0x0B: do_vec_SMOV_into_scalar (cpu); return;
	    case 0x0F: do_vec_UMOV_into_scalar (cpu); return;

	    case 0x00:
	    case 0x08:
	    case 0x10:
	    case 0x18:
	      do_vec_TBL (cpu); return;

	    case 0x06:
	    case 0x16:
	      do_vec_UZP (cpu); return;

	    case 0x0A: do_vec_TRN (cpu); return;

	    case 0x0E:
	    case 0x1E:
	      do_vec_ZIP (cpu); return;

	    default:
	      HALT_NYI;
	    }
	}

      switch (INSTR (13, 10))
	{
	case 0x6: do_vec_UZP (cpu); return;
	case 0xE: do_vec_ZIP (cpu); return;
	case 0xA: do_vec_TRN (cpu); return;
	default:  HALT_NYI;
	}
    }

  switch (INSTR (15, 10))
    {
    case 0x02: do_vec_REV64 (cpu); return;
    case 0x06: do_vec_REV16 (cpu); return;

    case 0x07:
      switch (INSTR (23, 21))
	{
	case 1: do_vec_AND (cpu); return;
	case 3: do_vec_BIC (cpu); return;
	case 5: do_vec_ORR (cpu); return;
	case 7: do_vec_ORN (cpu); return;
	default: HALT_NYI;
	}

    case 0x08: do_vec_sub_long (cpu); return;
    case 0x0a: do_vec_XTN (cpu); return;
    case 0x11: do_vec_SSHL (cpu); return;
    case 0x16: do_vec_CNT (cpu); return;
    case 0x19: do_vec_max (cpu); return;
    case 0x1B: do_vec_min (cpu); return;
    case 0x21: do_vec_add (cpu); return;
    case 0x25: do_vec_MLA (cpu); return;
    case 0x27: do_vec_mul (cpu); return;
    case 0x2F: do_vec_ADDP (cpu); return;
    case 0x30: do_vec_mull (cpu); return;
    case 0x33: do_vec_FMLA (cpu); return;
    case 0x35: do_vec_fadd (cpu); return;

    case 0x1E:
      switch (INSTR (20, 16))
	{
	case 0x01: do_vec_FCVTL (cpu); return;
	default: HALT_NYI;
	}

    case 0x2E:
      switch (INSTR (20, 16))
	{
	case 0x00: do_vec_ABS (cpu); return;
	case 0x01: do_vec_FCVTZS (cpu); return;
	case 0x11: do_vec_ADDV (cpu); return;
	default: HALT_NYI;
	}

    case 0x31:
    case 0x3B:
      do_vec_Fminmax (cpu); return;

    case 0x0D:
    case 0x0F:
    case 0x22:
    case 0x23:
    case 0x26:
    case 0x2A:
    case 0x32:
    case 0x36:
    case 0x39:
    case 0x3A:
      do_vec_compare (cpu); return;

    case 0x3E:
      do_vec_FABS (cpu); return;

    default:
      HALT_NYI;
    }
}

static void
do_vec_xtl (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30,29] = SXTL (00), UXTL (01), SXTL2 (10), UXTL2 (11)
     instr[28,22] = 0 1111 00
     instr[21,16] = size & shift (USHLL, SSHLL, USHLL2, SSHLL2)
     instr[15,10] = 1010 01
     instr[9,5]   = V source
     instr[4,0]   = V dest.  */

  unsigned vs = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i, shift, bias = 0;

  NYI_assert (28, 22, 0x3C);
  NYI_assert (15, 10, 0x29);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (30, 29))
    {
    case 2: /* SXTL2, SSHLL2.  */
      bias = 2;
      ATTRIBUTE_FALLTHROUGH;
    case 0: /* SXTL, SSHLL.  */
      if (INSTR (21, 21))
	{
	  int64_t val1, val2;

	  shift = INSTR (20, 16);
	  /* Get the source values before setting the destination values
	     in case the source and destination are the same.  */
	  val1 = aarch64_get_vec_s32 (cpu, vs, bias) << shift;
	  val2 = aarch64_get_vec_s32 (cpu, vs, bias + 1) << shift;
	  aarch64_set_vec_s64 (cpu, vd, 0, val1);
	  aarch64_set_vec_s64 (cpu, vd, 1, val2);
	}
      else if (INSTR (20, 20))
	{
	  int32_t v[4];

	  shift = INSTR (19, 16);
	  bias *= 2;
	  for (i = 0; i < 4; i++)
	    v[i] = aarch64_get_vec_s16 (cpu, vs, bias + i) << shift;
	  for (i = 0; i < 4; i++)
	    aarch64_set_vec_s32 (cpu, vd, i, v[i]);
	}
      else
	{
	  int16_t v[8];
	  NYI_assert (19, 19, 1);

	  shift = INSTR (18, 16);
	  bias *= 4;
	  for (i = 0; i < 8; i++)
	    v[i] = aarch64_get_vec_s8 (cpu, vs, i + bias) << shift;
	  for (i = 0; i < 8; i++)
	    aarch64_set_vec_s16 (cpu, vd, i, v[i]);
	}
      return;

    case 3: /* UXTL2, USHLL2.  */
      bias = 2;
      ATTRIBUTE_FALLTHROUGH;
    case 1: /* UXTL, USHLL.  */
      if (INSTR (21, 21))
	{
	  uint64_t v1, v2;
	  shift = INSTR (20, 16);
	  v1 = aarch64_get_vec_u32 (cpu, vs, bias) << shift;
	  v2 = aarch64_get_vec_u32 (cpu, vs, bias + 1) << shift;
	  aarch64_set_vec_u64 (cpu, vd, 0, v1);
	  aarch64_set_vec_u64 (cpu, vd, 1, v2);
	}
      else if (INSTR (20, 20))
	{
	  uint32_t v[4];
	  shift = INSTR (19, 16);
	  bias *= 2;
	  for (i = 0; i < 4; i++)
	    v[i] = aarch64_get_vec_u16 (cpu, vs, i + bias) << shift;
	  for (i = 0; i < 4; i++)
	    aarch64_set_vec_u32 (cpu, vd, i, v[i]);
	}
      else
	{
	  uint16_t v[8];
	  NYI_assert (19, 19, 1);

	  shift = INSTR (18, 16);
	  bias *= 4;
	  for (i = 0; i < 8; i++)
	    v[i] = aarch64_get_vec_u8 (cpu, vs, i + bias) << shift;
	  for (i = 0; i < 8; i++)
	    aarch64_set_vec_u16 (cpu, vd, i, v[i]);
	}
      return;
    }
}

static void
do_vec_SHL (sim_cpu *cpu)
{
  /* instr [31]    = 0
     instr [30]    = half(0)/full(1)
     instr [29,23] = 001 1110
     instr [22,16] = size and shift amount
     instr [15,10] = 01 0101
     instr [9, 5]  = Vs
     instr [4, 0]  = Vd.  */

  int shift;
  int full    = INSTR (30, 30);
  unsigned vs = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;

  NYI_assert (29, 23, 0x1E);
  NYI_assert (15, 10, 0x15);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      shift = INSTR (21, 16);

      if (full == 0)
	HALT_UNALLOC;

      for (i = 0; i < 2; i++)
	{
	  uint64_t val = aarch64_get_vec_u64 (cpu, vs, i);
	  aarch64_set_vec_u64 (cpu, vd, i, val << shift);
	}

      return;
    }

  if (INSTR (21, 21))
    {
      shift = INSTR (20, 16);

      for (i = 0; i < (full ? 4 : 2); i++)
	{
	  uint32_t val = aarch64_get_vec_u32 (cpu, vs, i);
	  aarch64_set_vec_u32 (cpu, vd, i, val << shift);
	}

      return;
    }

  if (INSTR (20, 20))
    {
      shift = INSTR (19, 16);

      for (i = 0; i < (full ? 8 : 4); i++)
	{
	  uint16_t val = aarch64_get_vec_u16 (cpu, vs, i);
	  aarch64_set_vec_u16 (cpu, vd, i, val << shift);
	}

      return;
    }

  if (INSTR (19, 19) == 0)
    HALT_UNALLOC;

  shift = INSTR (18, 16);

  for (i = 0; i < (full ? 16 : 8); i++)
    {
      uint8_t val = aarch64_get_vec_u8 (cpu, vs, i);
      aarch64_set_vec_u8 (cpu, vd, i, val << shift);
    }
}

static void
do_vec_SSHR_USHR (sim_cpu *cpu)
{
  /* instr [31]    = 0
     instr [30]    = half(0)/full(1)
     instr [29]    = signed(0)/unsigned(1)
     instr [28,23] = 0 1111 0
     instr [22,16] = size and shift amount
     instr [15,10] = 0000 01
     instr [9, 5]  = Vs
     instr [4, 0]  = Vd.  */

  int full       = INSTR (30, 30);
  int sign       = ! INSTR (29, 29);
  unsigned shift = INSTR (22, 16);
  unsigned vs    = INSTR (9, 5);
  unsigned vd    = INSTR (4, 0);
  unsigned i;

  NYI_assert (28, 23, 0x1E);
  NYI_assert (15, 10, 0x01);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      shift = 128 - shift;

      if (full == 0)
	HALT_UNALLOC;

      if (sign)
	for (i = 0; i < 2; i++)
	  {
	    int64_t val = aarch64_get_vec_s64 (cpu, vs, i);
	    aarch64_set_vec_s64 (cpu, vd, i, val >> shift);
	  }
      else
	for (i = 0; i < 2; i++)
	  {
	    uint64_t val = aarch64_get_vec_u64 (cpu, vs, i);
	    aarch64_set_vec_u64 (cpu, vd, i, val >> shift);
	  }

      return;
    }

  if (INSTR (21, 21))
    {
      shift = 64 - shift;

      if (sign)
	for (i = 0; i < (full ? 4 : 2); i++)
	  {
	    int32_t val = aarch64_get_vec_s32 (cpu, vs, i);
	    aarch64_set_vec_s32 (cpu, vd, i, val >> shift);
	  }
      else
	for (i = 0; i < (full ? 4 : 2); i++)
	  {
	    uint32_t val = aarch64_get_vec_u32 (cpu, vs, i);
	    aarch64_set_vec_u32 (cpu, vd, i, val >> shift);
	  }

      return;
    }

  if (INSTR (20, 20))
    {
      shift = 32 - shift;

      if (sign)
	for (i = 0; i < (full ? 8 : 4); i++)
	  {
	    int16_t val = aarch64_get_vec_s16 (cpu, vs, i);
	    aarch64_set_vec_s16 (cpu, vd, i, val >> shift);
	  }
      else
	for (i = 0; i < (full ? 8 : 4); i++)
	  {
	    uint16_t val = aarch64_get_vec_u16 (cpu, vs, i);
	    aarch64_set_vec_u16 (cpu, vd, i, val >> shift);
	  }

      return;
    }

  if (INSTR (19, 19) == 0)
    HALT_UNALLOC;

  shift = 16 - shift;

  if (sign)
    for (i = 0; i < (full ? 16 : 8); i++)
      {
	int8_t val = aarch64_get_vec_s8 (cpu, vs, i);
	aarch64_set_vec_s8 (cpu, vd, i, val >> shift);
      }
  else
    for (i = 0; i < (full ? 16 : 8); i++)
      {
	uint8_t val = aarch64_get_vec_u8 (cpu, vs, i);
	aarch64_set_vec_u8 (cpu, vd, i, val >> shift);
      }
}

static void
do_vec_MUL_by_element (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half/full
     instr[29,24] = 00 1111
     instr[23,22] = size
     instr[21]    = L
     instr[20]    = M
     instr[19,16] = m
     instr[15,12] = 1000
     instr[11]    = H
     instr[10]    = 0
     instr[9,5]   = Vn
     instr[4,0]   = Vd  */

  unsigned full     = INSTR (30, 30);
  unsigned L        = INSTR (21, 21);
  unsigned H        = INSTR (11, 11);
  unsigned vn       = INSTR (9, 5);
  unsigned vd       = INSTR (4, 0);
  unsigned size     = INSTR (23, 22);
  unsigned index;
  unsigned vm;
  unsigned e;

  NYI_assert (29, 24, 0x0F);
  NYI_assert (15, 12, 0x8);
  NYI_assert (10, 10, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (size)
    {
    case 1:
      {
	/* 16 bit products.  */
	uint16_t product;
	uint16_t element1;
	uint16_t element2;

	index = (H << 2) | (L << 1) | INSTR (20, 20);
	vm = INSTR (19, 16);
	element2 = aarch64_get_vec_u16 (cpu, vm, index);

	for (e = 0; e < (full ? 8 : 4); e ++)
	  {
	    element1 = aarch64_get_vec_u16 (cpu, vn, e);
	    product  = element1 * element2;
	    aarch64_set_vec_u16 (cpu, vd, e, product);
	  }
      }
      break;

    case 2:
      {
	/* 32 bit products.  */
	uint32_t product;
	uint32_t element1;
	uint32_t element2;

	index = (H << 1) | L;
	vm = INSTR (20, 16);
	element2 = aarch64_get_vec_u32 (cpu, vm, index);

	for (e = 0; e < (full ? 4 : 2); e ++)
	  {
	    element1 = aarch64_get_vec_u32 (cpu, vn, e);
	    product  = element1 * element2;
	    aarch64_set_vec_u32 (cpu, vd, e, product);
	  }
      }
      break;

    default:
      HALT_UNALLOC;
    }
}

static void
do_FMLA_by_element (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half/full
     instr[29,23] = 00 1111 1
     instr[22]    = size
     instr[21]    = L
     instr[20,16] = m
     instr[15,12] = 0001
     instr[11]    = H
     instr[10]    = 0
     instr[9,5]   = Vn
     instr[4,0]   = Vd  */

  unsigned full     = INSTR (30, 30);
  unsigned size     = INSTR (22, 22);
  unsigned L        = INSTR (21, 21);
  unsigned vm       = INSTR (20, 16);
  unsigned H        = INSTR (11, 11);
  unsigned vn       = INSTR (9, 5);
  unsigned vd       = INSTR (4, 0);
  unsigned e;

  NYI_assert (29, 23, 0x1F);
  NYI_assert (15, 12, 0x1);
  NYI_assert (10, 10, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (size)
    {
      double element1, element2;

      if (! full || L)
	HALT_UNALLOC;

      element2 = aarch64_get_vec_double (cpu, vm, H);

      for (e = 0; e < 2; e++)
	{
	  element1 = aarch64_get_vec_double (cpu, vn, e);
	  element1 *= element2;
	  element1 += aarch64_get_vec_double (cpu, vd, e);
	  aarch64_set_vec_double (cpu, vd, e, element1);
	}
    }
  else
    {
      float element1;
      float element2 = aarch64_get_vec_float (cpu, vm, (H << 1) | L);

      for (e = 0; e < (full ? 4 : 2); e++)
	{
	  element1 = aarch64_get_vec_float (cpu, vn, e);
	  element1 *= element2;
	  element1 += aarch64_get_vec_float (cpu, vd, e);
	  aarch64_set_vec_float (cpu, vd, e, element1);
	}
    }
}

static void
do_vec_op2 (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half/full
     instr[29,24] = 00 1111
     instr[23]    = ?
     instr[22,16] = element size & index
     instr[15,10] = sub-opcode
     instr[9,5]   = Vm
     instr[4,0]   = Vd  */

  NYI_assert (29, 24, 0x0F);

  if (INSTR (23, 23) != 0)
    {
      switch (INSTR (15, 10))
	{
	case 0x04:
	case 0x06:
	  do_FMLA_by_element (cpu);
	  return;

	case 0x20:
	case 0x22:
	  do_vec_MUL_by_element (cpu);
	  return;

	default:
	  HALT_NYI;
	}
    }
  else
    {
      switch (INSTR (15, 10))
	{
	case 0x01: do_vec_SSHR_USHR (cpu); return;
	case 0x15: do_vec_SHL (cpu); return;
	case 0x20:
	case 0x22: do_vec_MUL_by_element (cpu); return;
	case 0x29: do_vec_xtl (cpu); return;
	default:   HALT_NYI;
	}
    }
}

static void
do_vec_neg (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = full(1)/half(0)
     instr[29,24] = 10 1110
     instr[23,22] = size: byte(00), half (01), word (10), long (11)
     instr[21,10] = 1000 0010 1110
     instr[9,5]   = Vs
     instr[4,0]   = Vd  */

  int    full = INSTR (30, 30);
  unsigned vs = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;

  NYI_assert (29, 24, 0x2E);
  NYI_assert (21, 10, 0x82E);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 22))
    {
    case 0:
      for (i = 0; i < (full ? 16 : 8); i++)
	aarch64_set_vec_s8 (cpu, vd, i, - aarch64_get_vec_s8 (cpu, vs, i));
      return;

    case 1:
      for (i = 0; i < (full ? 8 : 4); i++)
	aarch64_set_vec_s16 (cpu, vd, i, - aarch64_get_vec_s16 (cpu, vs, i));
      return;

    case 2:
      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_s32 (cpu, vd, i, - aarch64_get_vec_s32 (cpu, vs, i));
      return;

    case 3:
      if (! full)
	HALT_NYI;
      for (i = 0; i < 2; i++)
	aarch64_set_vec_s64 (cpu, vd, i, - aarch64_get_vec_s64 (cpu, vs, i));
      return;
    }
}

static void
do_vec_sqrt (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = full(1)/half(0)
     instr[29,23] = 101 1101
     instr[22]    = single(0)/double(1)
     instr[21,10] = 1000 0111 1110
     instr[9,5]   = Vs
     instr[4,0]   = Vd.  */

  int    full = INSTR (30, 30);
  unsigned vs = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;

  NYI_assert (29, 23, 0x5B);
  NYI_assert (21, 10, 0x87E);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22) == 0)
    for (i = 0; i < (full ? 4 : 2); i++)
      aarch64_set_vec_float (cpu, vd, i,
			     sqrtf (aarch64_get_vec_float (cpu, vs, i)));
  else
    for (i = 0; i < 2; i++)
      aarch64_set_vec_double (cpu, vd, i,
			      sqrt (aarch64_get_vec_double (cpu, vs, i)));
}

static void
do_vec_mls_indexed (sim_cpu *cpu)
{
  /* instr[31]       = 0
     instr[30]       = half(0)/full(1)
     instr[29,24]    = 10 1111
     instr[23,22]    = 16-bit(01)/32-bit(10)
     instr[21,20+11] = index (if 16-bit)
     instr[21+11]    = index (if 32-bit)
     instr[20,16]    = Vm
     instr[15,12]    = 0100
     instr[11]       = part of index
     instr[10]       = 0
     instr[9,5]      = Vs
     instr[4,0]      = Vd.  */

  int    full = INSTR (30, 30);
  unsigned vs = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned vm = INSTR (20, 16);
  unsigned i;

  NYI_assert (15, 12, 4);
  NYI_assert (10, 10, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 22))
    {
    case 1:
      {
	unsigned elem;
	uint32_t val;

	if (vm > 15)
	  HALT_NYI;

	elem = (INSTR (21, 20) << 1) | INSTR (11, 11);
	val = aarch64_get_vec_u16 (cpu, vm, elem);

	for (i = 0; i < (full ? 8 : 4); i++)
	  aarch64_set_vec_u32 (cpu, vd, i,
			       aarch64_get_vec_u32 (cpu, vd, i) -
			       (aarch64_get_vec_u32 (cpu, vs, i) * val));
	return;
      }

    case 2:
      {
	unsigned elem = (INSTR (21, 21) << 1) | INSTR (11, 11);
	uint64_t val = aarch64_get_vec_u32 (cpu, vm, elem);

	for (i = 0; i < (full ? 4 : 2); i++)
	  aarch64_set_vec_u64 (cpu, vd, i,
			       aarch64_get_vec_u64 (cpu, vd, i) -
			       (aarch64_get_vec_u64 (cpu, vs, i) * val));
	return;
      }

    case 0:
    case 3:
    default:
      HALT_NYI;
    }
}

static void
do_vec_SUB (sim_cpu *cpu)
{
  /* instr [31]    = 0
     instr [30]    = half(0)/full(1)
     instr [29,24] = 10 1110
     instr [23,22] = size: byte(00, half(01), word (10), long (11)
     instr [21]    = 1
     instr [20,16] = Vm
     instr [15,10] = 10 0001
     instr [9, 5]  = Vn
     instr [4, 0]  = Vd.  */

  unsigned full = INSTR (30, 30);
  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;

  NYI_assert (29, 24, 0x2E);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x21);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 22))
    {
    case 0:
      for (i = 0; i < (full ? 16 : 8); i++)
	aarch64_set_vec_s8 (cpu, vd, i,
			    aarch64_get_vec_s8 (cpu, vn, i)
			    - aarch64_get_vec_s8 (cpu, vm, i));
      return;

    case 1:
      for (i = 0; i < (full ? 8 : 4); i++)
	aarch64_set_vec_s16 (cpu, vd, i,
			     aarch64_get_vec_s16 (cpu, vn, i)
			     - aarch64_get_vec_s16 (cpu, vm, i));
      return;

    case 2:
      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_s32 (cpu, vd, i,
			     aarch64_get_vec_s32 (cpu, vn, i)
			     - aarch64_get_vec_s32 (cpu, vm, i));
      return;

    case 3:
      if (full == 0)
	HALT_UNALLOC;

      for (i = 0; i < 2; i++)
	aarch64_set_vec_s64 (cpu, vd, i,
			     aarch64_get_vec_s64 (cpu, vn, i)
			     - aarch64_get_vec_s64 (cpu, vm, i));
      return;
    }
}

static void
do_vec_MLS (sim_cpu *cpu)
{
  /* instr [31]    = 0
     instr [30]    = half(0)/full(1)
     instr [29,24] = 10 1110
     instr [23,22] = size: byte(00, half(01), word (10)
     instr [21]    = 1
     instr [20,16] = Vm
     instr [15,10] = 10 0101
     instr [9, 5]  = Vn
     instr [4, 0]  = Vd.  */

  unsigned full = INSTR (30, 30);
  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;

  NYI_assert (29, 24, 0x2E);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x25);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 22))
    {
    case 0:
      for (i = 0; i < (full ? 16 : 8); i++)
	aarch64_set_vec_u8 (cpu, vd, i,
			    aarch64_get_vec_u8 (cpu, vd, i)
			    - (aarch64_get_vec_u8 (cpu, vn, i)
			       * aarch64_get_vec_u8 (cpu, vm, i)));
      return;

    case 1:
      for (i = 0; i < (full ? 8 : 4); i++)
	aarch64_set_vec_u16 (cpu, vd, i,
			     aarch64_get_vec_u16 (cpu, vd, i)
			     - (aarch64_get_vec_u16 (cpu, vn, i)
				* aarch64_get_vec_u16 (cpu, vm, i)));
      return;

    case 2:
      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_u32 (cpu, vd, i,
			     aarch64_get_vec_u32 (cpu, vd, i)
			     - (aarch64_get_vec_u32 (cpu, vn, i)
				* aarch64_get_vec_u32 (cpu, vm, i)));
      return;

    default:
      HALT_UNALLOC;
    }
}

static void
do_vec_FDIV (sim_cpu *cpu)
{
  /* instr [31]    = 0
     instr [30]    = half(0)/full(1)
     instr [29,23] = 10 1110 0
     instr [22]    = float()/double(1)
     instr [21]    = 1
     instr [20,16] = Vm
     instr [15,10] = 1111 11
     instr [9, 5]  = Vn
     instr [4, 0]  = Vd.  */

  unsigned full = INSTR (30, 30);
  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;

  NYI_assert (29, 23, 0x5C);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x3F);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      if (! full)
	HALT_UNALLOC;

      for (i = 0; i < 2; i++)
	aarch64_set_vec_double (cpu, vd, i,
				aarch64_get_vec_double (cpu, vn, i)
				/ aarch64_get_vec_double (cpu, vm, i));
    }
  else
    for (i = 0; i < (full ? 4 : 2); i++)
      aarch64_set_vec_float (cpu, vd, i,
			     aarch64_get_vec_float (cpu, vn, i)
			     / aarch64_get_vec_float (cpu, vm, i));
}

static void
do_vec_FMUL (sim_cpu *cpu)
{
  /* instr [31]    = 0
     instr [30]    = half(0)/full(1)
     instr [29,23] = 10 1110 0
     instr [22]    = float(0)/double(1)
     instr [21]    = 1
     instr [20,16] = Vm
     instr [15,10] = 1101 11
     instr [9, 5]  = Vn
     instr [4, 0]  = Vd.  */

  unsigned full = INSTR (30, 30);
  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;

  NYI_assert (29, 23, 0x5C);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x37);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      if (! full)
	HALT_UNALLOC;

      for (i = 0; i < 2; i++)
	aarch64_set_vec_double (cpu, vd, i,
				aarch64_get_vec_double (cpu, vn, i)
				* aarch64_get_vec_double (cpu, vm, i));
    }
  else
    for (i = 0; i < (full ? 4 : 2); i++)
      aarch64_set_vec_float (cpu, vd, i,
			     aarch64_get_vec_float (cpu, vn, i)
			     * aarch64_get_vec_float (cpu, vm, i));
}

static void
do_vec_FADDP (sim_cpu *cpu)
{
  /* instr [31]    = 0
     instr [30]    = half(0)/full(1)
     instr [29,23] = 10 1110 0
     instr [22]    = float(0)/double(1)
     instr [21]    = 1
     instr [20,16] = Vm
     instr [15,10] = 1101 01
     instr [9, 5]  = Vn
     instr [4, 0]  = Vd.  */

  unsigned full = INSTR (30, 30);
  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);

  NYI_assert (29, 23, 0x5C);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x35);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      /* Extract values before adding them incase vd == vn/vm.  */
      double tmp1 = aarch64_get_vec_double (cpu, vn, 0);
      double tmp2 = aarch64_get_vec_double (cpu, vn, 1);
      double tmp3 = aarch64_get_vec_double (cpu, vm, 0);
      double tmp4 = aarch64_get_vec_double (cpu, vm, 1);

      if (! full)
	HALT_UNALLOC;

      aarch64_set_vec_double (cpu, vd, 0, tmp1 + tmp2);
      aarch64_set_vec_double (cpu, vd, 1, tmp3 + tmp4);
    }
  else
    {
      /* Extract values before adding them incase vd == vn/vm.  */
      float tmp1 = aarch64_get_vec_float (cpu, vn, 0);
      float tmp2 = aarch64_get_vec_float (cpu, vn, 1);
      float tmp5 = aarch64_get_vec_float (cpu, vm, 0);
      float tmp6 = aarch64_get_vec_float (cpu, vm, 1);

      if (full)
	{
	  float tmp3 = aarch64_get_vec_float (cpu, vn, 2);
	  float tmp4 = aarch64_get_vec_float (cpu, vn, 3);
	  float tmp7 = aarch64_get_vec_float (cpu, vm, 2);
	  float tmp8 = aarch64_get_vec_float (cpu, vm, 3);

	  aarch64_set_vec_float (cpu, vd, 0, tmp1 + tmp2);
	  aarch64_set_vec_float (cpu, vd, 1, tmp3 + tmp4);
	  aarch64_set_vec_float (cpu, vd, 2, tmp5 + tmp6);
	  aarch64_set_vec_float (cpu, vd, 3, tmp7 + tmp8);
	}
      else
	{
	  aarch64_set_vec_float (cpu, vd, 0, tmp1 + tmp2);
	  aarch64_set_vec_float (cpu, vd, 1, tmp5 + tmp6);
	}
    }
}

static void
do_vec_FSQRT (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half(0)/full(1)
     instr[29,23] = 10 1110 1
     instr[22]    = single(0)/double(1)
     instr[21,10] = 10 0001 1111 10
     instr[9,5]   = Vsrc
     instr[4,0]   = Vdest.  */

  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned full = INSTR (30, 30);
  int i;

  NYI_assert (29, 23, 0x5D);
  NYI_assert (21, 10, 0x87E);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      if (! full)
	HALT_UNALLOC;

      for (i = 0; i < 2; i++)
	aarch64_set_vec_double (cpu, vd, i,
				sqrt (aarch64_get_vec_double (cpu, vn, i)));
    }
  else
    {
      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_float (cpu, vd, i,
			       sqrtf (aarch64_get_vec_float (cpu, vn, i)));
    }
}

static void
do_vec_FNEG (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half (0)/full (1)
     instr[29,23] = 10 1110 1
     instr[22]    = single (0)/double (1)
     instr[21,10] = 10 0000 1111 10
     instr[9,5]   = Vsrc
     instr[4,0]   = Vdest.  */

  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned full = INSTR (30, 30);
  int i;

  NYI_assert (29, 23, 0x5D);
  NYI_assert (21, 10, 0x83E);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      if (! full)
	HALT_UNALLOC;

      for (i = 0; i < 2; i++)
	aarch64_set_vec_double (cpu, vd, i,
				- aarch64_get_vec_double (cpu, vn, i));
    }
  else
    {
      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_float (cpu, vd, i,
			       - aarch64_get_vec_float (cpu, vn, i));
    }
}

static void
do_vec_NOT (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half (0)/full (1)
     instr[29,10] = 10 1110 0010 0000 0101 10
     instr[9,5]   = Vn
     instr[4.0]   = Vd.  */

  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30, 30);

  NYI_assert (29, 10, 0xB8816);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  for (i = 0; i < (full ? 16 : 8); i++)
    aarch64_set_vec_u8 (cpu, vd, i, ~ aarch64_get_vec_u8 (cpu, vn, i));
}

static unsigned int
clz (uint64_t val, unsigned size)
{
  uint64_t mask = 1;
  int      count;

  mask <<= (size - 1);
  count = 0;
  do
    {
      if (val & mask)
	break;
      mask >>= 1;
      count ++;
    }
  while (mask);

  return count;
}

static void
do_vec_CLZ (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = half (0)/full (1)
     instr[29,24] = 10 1110
     instr[23,22] = size
     instr[21,10] = 10 0000 0100 10
     instr[9,5]   = Vn
     instr[4.0]   = Vd.  */

  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned i;
  int      full = INSTR (30,30);

  NYI_assert (29, 24, 0x2E);
  NYI_assert (21, 10, 0x812);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (23, 22))
    {
    case 0:
      for (i = 0; i < (full ? 16 : 8); i++)
	aarch64_set_vec_u8 (cpu, vd, i, clz (aarch64_get_vec_u8 (cpu, vn, i), 8));
      break;
    case 1:
      for (i = 0; i < (full ? 8 : 4); i++)
	aarch64_set_vec_u16 (cpu, vd, i, clz (aarch64_get_vec_u16 (cpu, vn, i), 16));
      break;
    case 2:
      for (i = 0; i < (full ? 4 : 2); i++)
	aarch64_set_vec_u32 (cpu, vd, i, clz (aarch64_get_vec_u32 (cpu, vn, i), 32));
      break;
    case 3:
      if (! full)
	HALT_UNALLOC;
      aarch64_set_vec_u64 (cpu, vd, 0, clz (aarch64_get_vec_u64 (cpu, vn, 0), 64));
      aarch64_set_vec_u64 (cpu, vd, 1, clz (aarch64_get_vec_u64 (cpu, vn, 1), 64));
      break;
    }
}

static void
do_vec_MOV_element (sim_cpu *cpu)
{
  /* instr[31,21] = 0110 1110 000
     instr[20,16] = size & dest index
     instr[15]    = 0
     instr[14,11] = source index
     instr[10]    = 1
     instr[9,5]   = Vs
     instr[4.0]   = Vd.  */

  unsigned vs = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned src_index;
  unsigned dst_index;

  NYI_assert (31, 21, 0x370);
  NYI_assert (15, 15, 0);
  NYI_assert (10, 10, 1);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (16, 16))
    {
      /* Move a byte.  */
      src_index = INSTR (14, 11);
      dst_index = INSTR (20, 17);
      aarch64_set_vec_u8 (cpu, vd, dst_index,
			  aarch64_get_vec_u8 (cpu, vs, src_index));
    }
  else if (INSTR (17, 17))
    {
      /* Move 16-bits.  */
      NYI_assert (11, 11, 0);
      src_index = INSTR (14, 12);
      dst_index = INSTR (20, 18);
      aarch64_set_vec_u16 (cpu, vd, dst_index,
			   aarch64_get_vec_u16 (cpu, vs, src_index));
    }
  else if (INSTR (18, 18))
    {
      /* Move 32-bits.  */
      NYI_assert (12, 11, 0);
      src_index = INSTR (14, 13);
      dst_index = INSTR (20, 19);
      aarch64_set_vec_u32 (cpu, vd, dst_index,
			   aarch64_get_vec_u32 (cpu, vs, src_index));
    }
  else
    {
      NYI_assert (19, 19, 1);
      NYI_assert (13, 11, 0);
      src_index = INSTR (14, 14);
      dst_index = INSTR (20, 20);
      aarch64_set_vec_u64 (cpu, vd, dst_index,
			   aarch64_get_vec_u64 (cpu, vs, src_index));
    }
}

static void
do_vec_REV32 (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = full/half
     instr[29,24] = 10 1110 
     instr[23,22] = size
     instr[21,10] = 10 0000 0000 10
     instr[9,5]   = Rn
     instr[4,0]   = Rd.  */

  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  unsigned size = INSTR (23, 22);
  unsigned full = INSTR (30, 30);
  unsigned i;
  FRegister val;

  NYI_assert (29, 24, 0x2E);
  NYI_assert (21, 10, 0x802);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (size)
    {
    case 0:
      for (i = 0; i < (full ? 16 : 8); i++)
	val.b[i ^ 0x3] = aarch64_get_vec_u8 (cpu, rn, i);
      break;

    case 1:
      for (i = 0; i < (full ? 8 : 4); i++)
	val.h[i ^ 0x1] = aarch64_get_vec_u16 (cpu, rn, i);
      break;

    default:
      HALT_UNALLOC;
    }

  aarch64_set_vec_u64 (cpu, rd, 0, val.v[0]);
  if (full)
    aarch64_set_vec_u64 (cpu, rd, 1, val.v[1]);
}

static void
do_vec_EXT (sim_cpu *cpu)
{
  /* instr[31]    = 0
     instr[30]    = full/half
     instr[29,21] = 10 1110 000
     instr[20,16] = Vm
     instr[15]    = 0
     instr[14,11] = source index
     instr[10]    = 0
     instr[9,5]   = Vn
     instr[4.0]   = Vd.  */

  unsigned vm = INSTR (20, 16);
  unsigned vn = INSTR (9, 5);
  unsigned vd = INSTR (4, 0);
  unsigned src_index = INSTR (14, 11);
  unsigned full = INSTR (30, 30);
  unsigned i;
  unsigned j;
  FRegister val;

  NYI_assert (31, 21, 0x370);
  NYI_assert (15, 15, 0);
  NYI_assert (10, 10, 0);

  if (!full && (src_index & 0x8))
    HALT_UNALLOC;

  j = 0;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  for (i = src_index; i < (full ? 16 : 8); i++)
    val.b[j ++] = aarch64_get_vec_u8 (cpu, vn, i);
  for (i = 0; i < src_index; i++)
    val.b[j ++] = aarch64_get_vec_u8 (cpu, vm, i);

  aarch64_set_vec_u64 (cpu, vd, 0, val.v[0]);
  if (full)
    aarch64_set_vec_u64 (cpu, vd, 1, val.v[1]);
}

static void
dexAdvSIMD0 (sim_cpu *cpu)
{
  /* instr [28,25] = 0 111.  */
  if (    INSTR (15, 10) == 0x07
      && (INSTR (9, 5) ==
	  INSTR (20, 16)))
    {
      if (INSTR (31, 21) == 0x075
	  || INSTR (31, 21) == 0x275)
	{
	  do_vec_MOV_whole_vector (cpu);
	  return;
	}
    }

  if (INSTR (29, 19) == 0x1E0)
    {
      do_vec_MOV_immediate (cpu);
      return;
    }

  if (INSTR (29, 19) == 0x5E0)
    {
      do_vec_MVNI (cpu);
      return;
    }

  if (INSTR (29, 19) == 0x1C0
      || INSTR (29, 19) == 0x1C1)
    {
      if (INSTR (15, 10) == 0x03)
	{
	  do_vec_DUP_scalar_into_vector (cpu);
	  return;
	}
    }

  switch (INSTR (29, 24))
    {
    case 0x0E: do_vec_op1 (cpu); return;
    case 0x0F: do_vec_op2 (cpu); return;

    case 0x2E:
      if (INSTR (21, 21) == 1)
	{
	  switch (INSTR (15, 10))
	    {
	    case 0x02:
	      do_vec_REV32 (cpu);
	      return;

	    case 0x07:
	      switch (INSTR (23, 22))
		{
		case 0: do_vec_EOR (cpu); return;
		case 1: do_vec_BSL (cpu); return;
		case 2:
		case 3: do_vec_bit (cpu); return;
		}
	      break;

	    case 0x08: do_vec_sub_long (cpu); return;
	    case 0x11: do_vec_USHL (cpu); return;
	    case 0x12: do_vec_CLZ (cpu); return;
	    case 0x16: do_vec_NOT (cpu); return;
	    case 0x19: do_vec_max (cpu); return;
	    case 0x1B: do_vec_min (cpu); return;
	    case 0x21: do_vec_SUB (cpu); return;
	    case 0x25: do_vec_MLS (cpu); return;
	    case 0x31: do_vec_FminmaxNMP (cpu); return;
	    case 0x35: do_vec_FADDP (cpu); return;
	    case 0x37: do_vec_FMUL (cpu); return;
	    case 0x3F: do_vec_FDIV (cpu); return;

	    case 0x3E:
	      switch (INSTR (20, 16))
		{
		case 0x00: do_vec_FNEG (cpu); return;
		case 0x01: do_vec_FSQRT (cpu); return;
		default:   HALT_NYI;
		}

	    case 0x0D:
	    case 0x0F:
	    case 0x22:
	    case 0x23:
	    case 0x26:
	    case 0x2A:
	    case 0x32:
	    case 0x36:
	    case 0x39:
	    case 0x3A:
	      do_vec_compare (cpu); return;

	    default:
	      break;
	    }
	}

      if (INSTR (31, 21) == 0x370)
	{
	  if (INSTR (10, 10))
	    do_vec_MOV_element (cpu);
	  else
	    do_vec_EXT (cpu);
	  return;
	}

      switch (INSTR (21, 10))
	{
	case 0x82E: do_vec_neg (cpu); return;
	case 0x87E: do_vec_sqrt (cpu); return;
	default:
	  if (INSTR (15, 10) == 0x30)
	    {
	      do_vec_mull (cpu);
	      return;
	    }
	  break;
	}
      break;

    case 0x2f:
      switch (INSTR (15, 10))
	{
	case 0x01: do_vec_SSHR_USHR (cpu); return;
	case 0x10:
	case 0x12: do_vec_mls_indexed (cpu); return;
	case 0x29: do_vec_xtl (cpu); return;
	default:
	  HALT_NYI;
	}

    default:
      break;
    }

  HALT_NYI;
}

/* 3 sources.  */

/* Float multiply add.  */
static void
fmadds (sim_cpu *cpu)
{
  unsigned sa = INSTR (14, 10);
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, sd, aarch64_get_FP_float (cpu, sa)
			+ aarch64_get_FP_float (cpu, sn)
			* aarch64_get_FP_float (cpu, sm));
}

/* Double multiply add.  */
static void
fmaddd (sim_cpu *cpu)
{
  unsigned sa = INSTR (14, 10);
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, sd, aarch64_get_FP_double (cpu, sa)
			 + aarch64_get_FP_double (cpu, sn)
			 * aarch64_get_FP_double (cpu, sm));
}

/* Float multiply subtract.  */
static void
fmsubs (sim_cpu *cpu)
{
  unsigned sa = INSTR (14, 10);
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, sd, aarch64_get_FP_float (cpu, sa)
			- aarch64_get_FP_float (cpu, sn)
			* aarch64_get_FP_float (cpu, sm));
}

/* Double multiply subtract.  */
static void
fmsubd (sim_cpu *cpu)
{
  unsigned sa = INSTR (14, 10);
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, sd, aarch64_get_FP_double (cpu, sa)
			 - aarch64_get_FP_double (cpu, sn)
			 * aarch64_get_FP_double (cpu, sm));
}

/* Float negative multiply add.  */
static void
fnmadds (sim_cpu *cpu)
{
  unsigned sa = INSTR (14, 10);
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, sd, - aarch64_get_FP_float (cpu, sa)
			+ (- aarch64_get_FP_float (cpu, sn))
			* aarch64_get_FP_float (cpu, sm));
}

/* Double negative multiply add.  */
static void
fnmaddd (sim_cpu *cpu)
{
  unsigned sa = INSTR (14, 10);
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, sd, - aarch64_get_FP_double (cpu, sa)
			 + (- aarch64_get_FP_double (cpu, sn))
			 * aarch64_get_FP_double (cpu, sm));
}

/* Float negative multiply subtract.  */
static void
fnmsubs (sim_cpu *cpu)
{
  unsigned sa = INSTR (14, 10);
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, sd, - aarch64_get_FP_float (cpu, sa)
			+ aarch64_get_FP_float (cpu, sn)
			* aarch64_get_FP_float (cpu, sm));
}

/* Double negative multiply subtract.  */
static void
fnmsubd (sim_cpu *cpu)
{
  unsigned sa = INSTR (14, 10);
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, sd, - aarch64_get_FP_double (cpu, sa)
			 + aarch64_get_FP_double (cpu, sn)
			 * aarch64_get_FP_double (cpu, sm));
}

static void
dexSimpleFPDataProc3Source (sim_cpu *cpu)
{
  /* instr[31]    ==> M : 0 ==> OK, 1 ==> UNALLOC
     instr[30]    = 0
     instr[29]    ==> S :  0 ==> OK, 1 ==> UNALLOC
     instr[28,25] = 1111
     instr[24]    = 1
     instr[23,22] ==> type : 0 ==> single, 01 ==> double, 1x ==> UNALLOC
     instr[21]    ==> o1 : 0 ==> unnegated, 1 ==> negated
     instr[15]    ==> o2 : 0 ==> ADD, 1 ==> SUB  */

  uint32_t M_S = (INSTR (31, 31) << 1) | INSTR (29, 29);
  /* dispatch on combined type:o1:o2.  */
  uint32_t dispatch = (INSTR (23, 21) << 1) | INSTR (15, 15);

  if (M_S != 0)
    HALT_UNALLOC;

  switch (dispatch)
    {
    case 0: fmadds (cpu); return;
    case 1: fmsubs (cpu); return;
    case 2: fnmadds (cpu); return;
    case 3: fnmsubs (cpu); return;
    case 4: fmaddd (cpu); return;
    case 5: fmsubd (cpu); return;
    case 6: fnmaddd (cpu); return;
    case 7: fnmsubd (cpu); return;
    default:
      /* type > 1 is currently unallocated.  */
      HALT_UNALLOC;
    }
}

static void
dexSimpleFPFixedConvert (sim_cpu *cpu)
{
  HALT_NYI;
}

static void
dexSimpleFPCondCompare (sim_cpu *cpu)
{
  /* instr [31,23] = 0001 1110 0
     instr [22]    = type
     instr [21]    = 1
     instr [20,16] = Rm
     instr [15,12] = condition
     instr [11,10] = 01
     instr [9,5]   = Rn
     instr [4]     = 0
     instr [3,0]   = nzcv  */

  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);

  NYI_assert (31, 23, 0x3C);
  NYI_assert (11, 10, 0x1);
  NYI_assert (4,  4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (! testConditionCode (cpu, INSTR (15, 12)))
    {
      aarch64_set_CPSR (cpu, INSTR (3, 0));
      return;
    }

  if (INSTR (22, 22))
    {
      /* Double precision.  */
      double val1 = aarch64_get_vec_double (cpu, rn, 0);
      double val2 = aarch64_get_vec_double (cpu, rm, 0);

      /* FIXME: Check for NaNs.  */
      if (val1 == val2)
	aarch64_set_CPSR (cpu, (Z | C));
      else if (val1 < val2)
	aarch64_set_CPSR (cpu, N);
      else /* val1 > val2 */
	aarch64_set_CPSR (cpu, C);
    }
  else
    {
      /* Single precision.  */
      float val1 = aarch64_get_vec_float (cpu, rn, 0);
      float val2 = aarch64_get_vec_float (cpu, rm, 0);

      /* FIXME: Check for NaNs.  */
      if (val1 == val2)
	aarch64_set_CPSR (cpu, (Z | C));
      else if (val1 < val2)
	aarch64_set_CPSR (cpu, N);
      else /* val1 > val2 */
	aarch64_set_CPSR (cpu, C);
    }
}

/* 2 sources.  */

/* Float add.  */
static void
fadds (sim_cpu *cpu)
{
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, sd, aarch64_get_FP_float (cpu, sn)
			+ aarch64_get_FP_float (cpu, sm));
}

/* Double add.  */
static void
faddd (sim_cpu *cpu)
{
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, sd, aarch64_get_FP_double (cpu, sn)
			 + aarch64_get_FP_double (cpu, sm));
}

/* Float divide.  */
static void
fdivs (sim_cpu *cpu)
{
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, sd, aarch64_get_FP_float (cpu, sn)
			/ aarch64_get_FP_float (cpu, sm));
}

/* Double divide.  */
static void
fdivd (sim_cpu *cpu)
{
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, sd, aarch64_get_FP_double (cpu, sn)
			 / aarch64_get_FP_double (cpu, sm));
}

/* Float multiply.  */
static void
fmuls (sim_cpu *cpu)
{
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, sd, aarch64_get_FP_float (cpu, sn)
			* aarch64_get_FP_float (cpu, sm));
}

/* Double multiply.  */
static void
fmuld (sim_cpu *cpu)
{
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, sd, aarch64_get_FP_double (cpu, sn)
			 * aarch64_get_FP_double (cpu, sm));
}

/* Float negate and multiply.  */
static void
fnmuls (sim_cpu *cpu)
{
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, sd, - (aarch64_get_FP_float (cpu, sn)
				    * aarch64_get_FP_float (cpu, sm)));
}

/* Double negate and multiply.  */
static void
fnmuld (sim_cpu *cpu)
{
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, sd, - (aarch64_get_FP_double (cpu, sn)
				     * aarch64_get_FP_double (cpu, sm)));
}

/* Float subtract.  */
static void
fsubs (sim_cpu *cpu)
{
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, sd, aarch64_get_FP_float (cpu, sn)
			- aarch64_get_FP_float (cpu, sm));
}

/* Double subtract.  */
static void
fsubd (sim_cpu *cpu)
{
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, sd, aarch64_get_FP_double (cpu, sn)
			 - aarch64_get_FP_double (cpu, sm));
}

static void
do_FMINNM (sim_cpu *cpu)
{
  /* instr[31,23] = 0 0011 1100
     instr[22]    = float(0)/double(1)
     instr[21]    = 1
     instr[20,16] = Sm
     instr[15,10] = 01 1110
     instr[9,5]   = Sn
     instr[4,0]   = Cpu  */

  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  NYI_assert (31, 23, 0x03C);
  NYI_assert (15, 10, 0x1E);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    aarch64_set_FP_double (cpu, sd,
			   dminnm (aarch64_get_FP_double (cpu, sn),
				   aarch64_get_FP_double (cpu, sm)));
  else
    aarch64_set_FP_float (cpu, sd,
			  fminnm (aarch64_get_FP_float (cpu, sn),
				  aarch64_get_FP_float (cpu, sm)));
}

static void
do_FMAXNM (sim_cpu *cpu)
{
  /* instr[31,23] = 0 0011 1100
     instr[22]    = float(0)/double(1)
     instr[21]    = 1
     instr[20,16] = Sm
     instr[15,10] = 01 1010
     instr[9,5]   = Sn
     instr[4,0]   = Cpu  */

  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);
  unsigned sd = INSTR ( 4,  0);

  NYI_assert (31, 23, 0x03C);
  NYI_assert (15, 10, 0x1A);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    aarch64_set_FP_double (cpu, sd,
			   dmaxnm (aarch64_get_FP_double (cpu, sn),
				   aarch64_get_FP_double (cpu, sm)));
  else
    aarch64_set_FP_float (cpu, sd,
			  fmaxnm (aarch64_get_FP_float (cpu, sn),
				  aarch64_get_FP_float (cpu, sm)));
}

static void
dexSimpleFPDataProc2Source (sim_cpu *cpu)
{
  /* instr[31]    ==> M : 0 ==> OK, 1 ==> UNALLOC
     instr[30]    = 0
     instr[29]    ==> S :  0 ==> OK, 1 ==> UNALLOC
     instr[28,25] = 1111
     instr[24]    = 0
     instr[23,22] ==> type : 0 ==> single, 01 ==> double, 1x ==> UNALLOC
     instr[21]    = 1
     instr[20,16] = Vm
     instr[15,12] ==> opcode : 0000 ==> FMUL, 0001 ==> FDIV
                               0010 ==> FADD, 0011 ==> FSUB,
                               0100 ==> FMAX, 0101 ==> FMIN
                               0110 ==> FMAXNM, 0111 ==> FMINNM
                               1000 ==> FNMUL, ow ==> UNALLOC
     instr[11,10] = 10
     instr[9,5]   = Vn
     instr[4,0]   = Vd  */

  uint32_t M_S = (INSTR (31, 31) << 1) | INSTR (29, 29);
  uint32_t type = INSTR (23, 22);
  /* Dispatch on opcode.  */
  uint32_t dispatch = INSTR (15, 12);

  if (type > 1)
    HALT_UNALLOC;

  if (M_S != 0)
    HALT_UNALLOC;

  if (type)
    switch (dispatch)
      {
      case 0: fmuld (cpu); return;
      case 1: fdivd (cpu); return;
      case 2: faddd (cpu); return;
      case 3: fsubd (cpu); return;
      case 6: do_FMAXNM (cpu); return;
      case 7: do_FMINNM (cpu); return;
      case 8: fnmuld (cpu); return;

	/* Have not yet implemented fmax and fmin.  */
      case 4:
      case 5:
	HALT_NYI;

      default:
	HALT_UNALLOC;
      }
  else /* type == 0 => floats.  */
    switch (dispatch)
      {
      case 0: fmuls (cpu); return;
      case 1: fdivs (cpu); return;
      case 2: fadds (cpu); return;
      case 3: fsubs (cpu); return;
      case 6: do_FMAXNM (cpu); return;
      case 7: do_FMINNM (cpu); return;
      case 8: fnmuls (cpu); return;

      case 4:
      case 5:
	HALT_NYI;

      default:
	HALT_UNALLOC;
      }
}

static void
dexSimpleFPCondSelect (sim_cpu *cpu)
{
  /* FCSEL
     instr[31,23] = 0 0011 1100
     instr[22]    = 0=>single 1=>double
     instr[21]    = 1
     instr[20,16] = Sm
     instr[15,12] = cond
     instr[11,10] = 11
     instr[9,5]   = Sn
     instr[4,0]   = Cpu  */
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9, 5);
  unsigned sd = INSTR ( 4, 0);
  uint32_t set = testConditionCode (cpu, INSTR (15, 12));

  NYI_assert (31, 23, 0x03C);
  NYI_assert (11, 10, 0x3);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    aarch64_set_FP_double (cpu, sd, (set ? aarch64_get_FP_double (cpu, sn)
				     : aarch64_get_FP_double (cpu, sm)));
  else
    aarch64_set_FP_float (cpu, sd, (set ? aarch64_get_FP_float (cpu, sn)
				    : aarch64_get_FP_float (cpu, sm)));
}

/* Store 32 bit unscaled signed 9 bit.  */
static void
fsturs (sim_cpu *cpu, int32_t offset)
{
  unsigned int rn = INSTR (9, 5);
  unsigned int st = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_mem_u32 (cpu, aarch64_get_reg_u64 (cpu, rn, 1) + offset,
		       aarch64_get_vec_u32 (cpu, st, 0));
}

/* Store 64 bit unscaled signed 9 bit.  */
static void
fsturd (sim_cpu *cpu, int32_t offset)
{
  unsigned int rn = INSTR (9, 5);
  unsigned int st = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_mem_u64 (cpu, aarch64_get_reg_u64 (cpu, rn, 1) + offset,
		       aarch64_get_vec_u64 (cpu, st, 0));
}

/* Store 128 bit unscaled signed 9 bit.  */
static void
fsturq (sim_cpu *cpu, int32_t offset)
{
  unsigned int rn = INSTR (9, 5);
  unsigned int st = INSTR (4, 0);
  FRegister a;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_get_FP_long_double (cpu, st, & a);
  aarch64_set_mem_long_double (cpu,
			       aarch64_get_reg_u64 (cpu, rn, 1)
			       + offset, a);
}

/* TODO FP move register.  */

/* 32 bit fp to fp move register.  */
static void
ffmovs (sim_cpu *cpu)
{
  unsigned int rn = INSTR (9, 5);
  unsigned int st = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, st, aarch64_get_FP_float (cpu, rn));
}

/* 64 bit fp to fp move register.  */
static void
ffmovd (sim_cpu *cpu)
{
  unsigned int rn = INSTR (9, 5);
  unsigned int st = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, st, aarch64_get_FP_double (cpu, rn));
}

/* 32 bit GReg to Vec move register.  */
static void
fgmovs (sim_cpu *cpu)
{
  unsigned int rn = INSTR (9, 5);
  unsigned int st = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u32 (cpu, st, 0, aarch64_get_reg_u32 (cpu, rn, NO_SP));
}

/* 64 bit g to fp move register.  */
static void
fgmovd (sim_cpu *cpu)
{
  unsigned int rn = INSTR (9, 5);
  unsigned int st = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u64 (cpu, st, 0, aarch64_get_reg_u64 (cpu, rn, NO_SP));
}

/* 32 bit fp to g move register.  */
static void
gfmovs (sim_cpu *cpu)
{
  unsigned int rn = INSTR (9, 5);
  unsigned int st = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, st, NO_SP, aarch64_get_vec_u32 (cpu, rn, 0));
}

/* 64 bit fp to g move register.  */
static void
gfmovd (sim_cpu *cpu)
{
  unsigned int rn = INSTR (9, 5);
  unsigned int st = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, st, NO_SP, aarch64_get_vec_u64 (cpu, rn, 0));
}

/* FP move immediate

   These install an immediate 8 bit value in the target register
   where the 8 bits comprise 1 sign bit, 4 bits of fraction and a 3
   bit exponent.  */

static void
fmovs (sim_cpu *cpu)
{
  unsigned int sd = INSTR (4, 0);
  uint32_t imm = INSTR (20, 13);
  float f = fp_immediate_for_encoding_32 (imm);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, sd, f);
}

static void
fmovd (sim_cpu *cpu)
{
  unsigned int sd = INSTR (4, 0);
  uint32_t imm = INSTR (20, 13);
  double d = fp_immediate_for_encoding_64 (imm);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, sd, d);
}

static void
dexSimpleFPImmediate (sim_cpu *cpu)
{
  /* instr[31,23] == 00111100
     instr[22]    == type : single(0)/double(1)
     instr[21]    == 1
     instr[20,13] == imm8
     instr[12,10] == 100
     instr[9,5]   == imm5 : 00000 ==> PK, ow ==> UNALLOC
     instr[4,0]   == Rd  */
  uint32_t imm5 = INSTR (9, 5);

  NYI_assert (31, 23, 0x3C);

  if (imm5 != 0)
    HALT_UNALLOC;

  if (INSTR (22, 22))
    fmovd (cpu);
  else
    fmovs (cpu);
}

/* TODO specific decode and execute for group Load Store.  */

/* TODO FP load/store single register (unscaled offset).  */

/* TODO load 8 bit unscaled signed 9 bit.  */
/* TODO load 16 bit unscaled signed 9 bit.  */

/* Load 32 bit unscaled signed 9 bit.  */
static void
fldurs (sim_cpu *cpu, int32_t offset)
{
  unsigned int rn = INSTR (9, 5);
  unsigned int st = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u32 (cpu, st, 0, aarch64_get_mem_u32
		       (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK) + offset));
}

/* Load 64 bit unscaled signed 9 bit.  */
static void
fldurd (sim_cpu *cpu, int32_t offset)
{
  unsigned int rn = INSTR (9, 5);
  unsigned int st = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u64 (cpu, st, 0, aarch64_get_mem_u64
		       (cpu, aarch64_get_reg_u64 (cpu, rn, SP_OK) + offset));
}

/* Load 128 bit unscaled signed 9 bit.  */
static void
fldurq (sim_cpu *cpu, int32_t offset)
{
  unsigned int rn = INSTR (9, 5);
  unsigned int st = INSTR (4, 0);
  FRegister a;
  uint64_t addr = aarch64_get_reg_u64 (cpu, rn, SP_OK) + offset;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_get_mem_long_double (cpu, addr, & a);
  aarch64_set_FP_long_double (cpu, st, a);
}

/* TODO store 8 bit unscaled signed 9 bit.  */
/* TODO store 16 bit unscaled signed 9 bit.  */


/* 1 source.  */

/* Float absolute value.  */
static void
fabss (sim_cpu *cpu)
{
  unsigned sn = INSTR (9, 5);
  unsigned sd = INSTR (4, 0);
  float value = aarch64_get_FP_float (cpu, sn);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, sd, fabsf (value));
}

/* Double absolute value.  */
static void
fabcpu (sim_cpu *cpu)
{
  unsigned sn = INSTR (9, 5);
  unsigned sd = INSTR (4, 0);
  double value = aarch64_get_FP_double (cpu, sn);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, sd, fabs (value));
}

/* Float negative value.  */
static void
fnegs (sim_cpu *cpu)
{
  unsigned sn = INSTR (9, 5);
  unsigned sd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, sd, - aarch64_get_FP_float (cpu, sn));
}

/* Double negative value.  */
static void
fnegd (sim_cpu *cpu)
{
  unsigned sn = INSTR (9, 5);
  unsigned sd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, sd, - aarch64_get_FP_double (cpu, sn));
}

/* Float square root.  */
static void
fsqrts (sim_cpu *cpu)
{
  unsigned sn = INSTR (9, 5);
  unsigned sd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, sd, sqrtf (aarch64_get_FP_float (cpu, sn)));
}

/* Double square root.  */
static void
fsqrtd (sim_cpu *cpu)
{
  unsigned sn = INSTR (9, 5);
  unsigned sd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, sd,
			 sqrt (aarch64_get_FP_double (cpu, sn)));
}

/* Convert double to float.  */
static void
fcvtds (sim_cpu *cpu)
{
  unsigned sn = INSTR (9, 5);
  unsigned sd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, sd, (float) aarch64_get_FP_double (cpu, sn));
}

/* Convert float to double.  */
static void
fcvtcpu (sim_cpu *cpu)
{
  unsigned sn = INSTR (9, 5);
  unsigned sd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, sd, (double) aarch64_get_FP_float (cpu, sn));
}

static void
do_FRINT (sim_cpu *cpu)
{
  /* instr[31,23] = 0001 1110 0
     instr[22]    = single(0)/double(1)
     instr[21,18] = 1001
     instr[17,15] = rounding mode
     instr[14,10] = 10000
     instr[9,5]   = source
     instr[4,0]   = dest  */

  float val;
  unsigned rs = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  unsigned int rmode = INSTR (17, 15);

  NYI_assert (31, 23, 0x03C);
  NYI_assert (21, 18, 0x9);
  NYI_assert (14, 10, 0x10);

  if (rmode == 6 || rmode == 7)
    /* FIXME: Add support for rmode == 6 exactness check.  */
    rmode = uimm (aarch64_get_FPSR (cpu), 23, 22);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      double dval = aarch64_get_FP_double (cpu, rs);

      switch (rmode)
	{
	case 0: /* mode N: nearest or even.  */
	  {
	    double rval = round (dval);

	    if (dval - rval == 0.5)
	      {
		if (((rval / 2.0) * 2.0) != rval)
		  rval += 1.0;
	      }

	    aarch64_set_FP_double (cpu, rd, round (dval));
	    return;
	  }

	case 1: /* mode P: towards +inf.  */
	  if (dval < 0.0)
	    aarch64_set_FP_double (cpu, rd, trunc (dval));
	  else
	    aarch64_set_FP_double (cpu, rd, round (dval));
	  return;

	case 2: /* mode M: towards -inf.  */
	  if (dval < 0.0)
	    aarch64_set_FP_double (cpu, rd, round (dval));
	  else
	    aarch64_set_FP_double (cpu, rd, trunc (dval));
	  return;

	case 3: /* mode Z: towards 0.  */
	  aarch64_set_FP_double (cpu, rd, trunc (dval));
	  return;

	case 4: /* mode A: away from 0.  */
	  aarch64_set_FP_double (cpu, rd, round (dval));
	  return;

	case 6: /* mode X: use FPCR with exactness check.  */
	case 7: /* mode I: use FPCR mode.  */
	  HALT_NYI;

	default:
	  HALT_UNALLOC;
	}
    }

  val = aarch64_get_FP_float (cpu, rs);

  switch (rmode)
    {
    case 0: /* mode N: nearest or even.  */
      {
	float rval = roundf (val);

	if (val - rval == 0.5)
	  {
	    if (((rval / 2.0) * 2.0) != rval)
	      rval += 1.0;
	  }

	aarch64_set_FP_float (cpu, rd, rval);
	return;
      }

    case 1: /* mode P: towards +inf.  */
      if (val < 0.0)
	aarch64_set_FP_float (cpu, rd, truncf (val));
      else
	aarch64_set_FP_float (cpu, rd, roundf (val));
      return;

    case 2: /* mode M: towards -inf.  */
      if (val < 0.0)
	aarch64_set_FP_float (cpu, rd, truncf (val));
      else
	aarch64_set_FP_float (cpu, rd, roundf (val));
      return;

    case 3: /* mode Z: towards 0.  */
      aarch64_set_FP_float (cpu, rd, truncf (val));
      return;

    case 4: /* mode A: away from 0.  */
      aarch64_set_FP_float (cpu, rd, roundf (val));
      return;

    case 6: /* mode X: use FPCR with exactness check.  */
    case 7: /* mode I: use FPCR mode.  */
      HALT_NYI;

    default:
      HALT_UNALLOC;
    }
}

/* Convert half to float.  */
static void
do_FCVT_half_to_single (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (31, 10, 0x7B890);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float (cpu, rd, (float) aarch64_get_FP_half  (cpu, rn));
}

/* Convert half to double.  */
static void
do_FCVT_half_to_double (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (31, 10, 0x7B8B0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double (cpu, rd, (double) aarch64_get_FP_half  (cpu, rn));
}

static void
do_FCVT_single_to_half (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (31, 10, 0x788F0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_half (cpu, rd, aarch64_get_FP_float  (cpu, rn));
}

/* Convert double to half.  */
static void
do_FCVT_double_to_half (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (31, 10, 0x798F0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_half (cpu, rd, (float) aarch64_get_FP_double  (cpu, rn));
}

static void
dexSimpleFPDataProc1Source (sim_cpu *cpu)
{
  /* instr[31]    ==> M : 0 ==> OK, 1 ==> UNALLOC
     instr[30]    = 0
     instr[29]    ==> S :  0 ==> OK, 1 ==> UNALLOC
     instr[28,25] = 1111
     instr[24]    = 0
     instr[23,22] ==> type : 00 ==> source is single,
                             01 ==> source is double
                             10 ==> UNALLOC
                             11 ==> UNALLOC or source is half
     instr[21]    = 1
     instr[20,15] ==> opcode : with type 00 or 01
                               000000 ==> FMOV, 000001 ==> FABS,
                               000010 ==> FNEG, 000011 ==> FSQRT,
                               000100 ==> UNALLOC, 000101 ==> FCVT,(to single/double)
                               000110 ==> UNALLOC, 000111 ==> FCVT (to half)
                               001000 ==> FRINTN, 001001 ==> FRINTP,
                               001010 ==> FRINTM, 001011 ==> FRINTZ,
                               001100 ==> FRINTA, 001101 ==> UNALLOC
                               001110 ==> FRINTX, 001111 ==> FRINTI
                               with type 11
                               000100 ==> FCVT (half-to-single)
                               000101 ==> FCVT (half-to-double)
			       instr[14,10] = 10000.  */

  uint32_t M_S = (INSTR (31, 31) << 1) | INSTR (29, 29);
  uint32_t type   = INSTR (23, 22);
  uint32_t opcode = INSTR (20, 15);

  if (M_S != 0)
    HALT_UNALLOC;

  if (type == 3)
    {
      if (opcode == 4)
	do_FCVT_half_to_single (cpu);
      else if (opcode == 5)
	do_FCVT_half_to_double (cpu);
      else
	HALT_UNALLOC;
      return;
    }

  if (type == 2)
    HALT_UNALLOC;

  switch (opcode)
    {
    case 0:
      if (type)
	ffmovd (cpu);
      else
	ffmovs (cpu);
      return;

    case 1:
      if (type)
	fabcpu (cpu);
      else
	fabss (cpu);
      return;

    case 2:
      if (type)
	fnegd (cpu);
      else
	fnegs (cpu);
      return;

    case 3:
      if (type)
	fsqrtd (cpu);
      else
	fsqrts (cpu);
      return;

    case 4:
      if (type)
	fcvtds (cpu);
      else
	HALT_UNALLOC;
      return;

    case 5:
      if (type)
	HALT_UNALLOC;
      fcvtcpu (cpu);
      return;

    case 8:		/* FRINTN etc.  */
    case 9:
    case 10:
    case 11:
    case 12:
    case 14:
    case 15:
       do_FRINT (cpu);
       return;

    case 7:
      if (INSTR (22, 22))
	do_FCVT_double_to_half (cpu);
      else
	do_FCVT_single_to_half (cpu);
      return;

    case 13:
      HALT_NYI;

    default:
      HALT_UNALLOC;
    }
}

/* 32 bit signed int to float.  */
static void
scvtf32 (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned sd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float
    (cpu, sd, (float) aarch64_get_reg_s32 (cpu, rn, NO_SP));
}

/* signed int to float.  */
static void
scvtf (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned sd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_float
    (cpu, sd, (float) aarch64_get_reg_s64 (cpu, rn, NO_SP));
}

/* 32 bit signed int to double.  */
static void
scvtd32 (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned sd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double
    (cpu, sd, (double) aarch64_get_reg_s32 (cpu, rn, NO_SP));
}

/* signed int to double.  */
static void
scvtd (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned sd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_FP_double
    (cpu, sd, (double) aarch64_get_reg_s64 (cpu, rn, NO_SP));
}

static const float  FLOAT_INT_MAX   = (float)  INT_MAX;
static const float  FLOAT_INT_MIN   = (float)  INT_MIN;
static const double DOUBLE_INT_MAX  = (double) INT_MAX;
static const double DOUBLE_INT_MIN  = (double) INT_MIN;
static const float  FLOAT_LONG_MAX  = (float)  LONG_MAX;
static const float  FLOAT_LONG_MIN  = (float)  LONG_MIN;
static const double DOUBLE_LONG_MAX = (double) LONG_MAX;
static const double DOUBLE_LONG_MIN = (double) LONG_MIN;

#define UINT_MIN 0
#define ULONG_MIN 0
static const float  FLOAT_UINT_MAX   = (float)  UINT_MAX;
static const float  FLOAT_UINT_MIN   = (float)  UINT_MIN;
static const double DOUBLE_UINT_MAX  = (double) UINT_MAX;
static const double DOUBLE_UINT_MIN  = (double) UINT_MIN;
static const float  FLOAT_ULONG_MAX  = (float)  ULONG_MAX;
static const float  FLOAT_ULONG_MIN  = (float)  ULONG_MIN;
static const double DOUBLE_ULONG_MAX = (double) ULONG_MAX;
static const double DOUBLE_ULONG_MIN = (double) ULONG_MIN;

/* Check for FP exception conditions:
     NaN raises IO
     Infinity raises IO
     Out of Range raises IO and IX and saturates value
     Denormal raises ID and IX and sets to zero.  */
#define RAISE_EXCEPTIONS(F, VALUE, FTYPE, ITYPE)	\
  do							\
    {							\
      switch (fpclassify (F))				\
	{						\
	case FP_INFINITE:				\
	case FP_NAN:					\
	  aarch64_set_FPSR (cpu, IO);			\
	  if (signbit (F))				\
	    VALUE = ITYPE##_MAX;			\
	  else						\
	    VALUE = ITYPE##_MIN;			\
	  break;					\
							\
	case FP_NORMAL:					\
	  if (F >= FTYPE##_##ITYPE##_MAX)		\
	    {						\
	      aarch64_set_FPSR_bits (cpu, IO | IX, IO | IX);	\
	      VALUE = ITYPE##_MAX;			\
	    }						\
	  else if (F <= FTYPE##_##ITYPE##_MIN)		\
	    {						\
	      aarch64_set_FPSR_bits (cpu, IO | IX, IO | IX);	\
	      VALUE = ITYPE##_MIN;			\
	    }						\
	  break;					\
							\
	case FP_SUBNORMAL:				\
	  aarch64_set_FPSR_bits (cpu, IO | IX | ID, IX | ID);	\
	  VALUE = 0;					\
	  break;					\
							\
	default:					\
	case FP_ZERO:					\
	  VALUE = 0;					\
	  break;					\
	}						\
    }							\
  while (0)

/* 32 bit convert float to signed int truncate towards zero.  */
static void
fcvtszs32 (sim_cpu *cpu)
{
  unsigned sn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  /* TODO : check that this rounds toward zero.  */
  float   f = aarch64_get_FP_float (cpu, sn);
  int32_t value = (int32_t) f;

  RAISE_EXCEPTIONS (f, value, FLOAT, INT);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* Avoid sign extension to 64 bit.  */
  aarch64_set_reg_u64 (cpu, rd, NO_SP, (uint32_t) value);
}

/* 64 bit convert float to signed int truncate towards zero.  */
static void
fcvtszs (sim_cpu *cpu)
{
  unsigned sn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  float f = aarch64_get_FP_float (cpu, sn);
  int64_t value = (int64_t) f;

  RAISE_EXCEPTIONS (f, value, FLOAT, LONG);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_s64 (cpu, rd, NO_SP, value);
}

/* 32 bit convert double to signed int truncate towards zero.  */
static void
fcvtszd32 (sim_cpu *cpu)
{
  unsigned sn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  /* TODO : check that this rounds toward zero.  */
  double   d = aarch64_get_FP_double (cpu, sn);
  int32_t  value = (int32_t) d;

  RAISE_EXCEPTIONS (d, value, DOUBLE, INT);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* Avoid sign extension to 64 bit.  */
  aarch64_set_reg_u64 (cpu, rd, NO_SP, (uint32_t) value);
}

/* 64 bit convert double to signed int truncate towards zero.  */
static void
fcvtszd (sim_cpu *cpu)
{
  unsigned sn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  /* TODO : check that this rounds toward zero.  */
  double  d = aarch64_get_FP_double (cpu, sn);
  int64_t value;

  value = (int64_t) d;

  RAISE_EXCEPTIONS (d, value, DOUBLE, LONG);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_s64 (cpu, rd, NO_SP, value);
}

static void
do_fcvtzu (sim_cpu *cpu)
{
  /* instr[31]    = size: 32-bit (0), 64-bit (1)
     instr[30,23] = 00111100
     instr[22]    = type: single (0)/ double (1)
     instr[21]    = enable (0)/disable(1) precision
     instr[20,16] = 11001
     instr[15,10] = precision
     instr[9,5]   = Rs
     instr[4,0]   = Rd.  */

  unsigned rs = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (30, 23, 0x3C);
  NYI_assert (20, 16, 0x19);

  if (INSTR (21, 21) != 1)
    /* Convert to fixed point.  */
    HALT_NYI;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (31, 31))
    {
      /* Convert to unsigned 64-bit integer.  */
      if (INSTR (22, 22))
	{
	  double  d = aarch64_get_FP_double (cpu, rs);
	  uint64_t value = (uint64_t) d;

	  /* Do not raise an exception if we have reached ULONG_MAX.  */
	  if (value != (1ULL << 63))
	    RAISE_EXCEPTIONS (d, value, DOUBLE, ULONG);

	  aarch64_set_reg_u64 (cpu, rd, NO_SP, value);
	}
      else
	{
	  float  f = aarch64_get_FP_float (cpu, rs);
	  uint64_t value = (uint64_t) f;

	  /* Do not raise an exception if we have reached ULONG_MAX.  */
	  if (value != (1ULL << 63))
	    RAISE_EXCEPTIONS (f, value, FLOAT, ULONG);

	  aarch64_set_reg_u64 (cpu, rd, NO_SP, value);
	}
    }
  else
    {
      uint32_t value;

      /* Convert to unsigned 32-bit integer.  */
      if (INSTR (22, 22))
	{
	  double  d = aarch64_get_FP_double (cpu, rs);

	  value = (uint32_t) d;
	  /* Do not raise an exception if we have reached UINT_MAX.  */
	  if (value != (1UL << 31))
	    RAISE_EXCEPTIONS (d, value, DOUBLE, UINT);
	}
      else
	{
	  float  f = aarch64_get_FP_float (cpu, rs);

	  value = (uint32_t) f;
	  /* Do not raise an exception if we have reached UINT_MAX.  */
	  if (value != (1UL << 31))
	    RAISE_EXCEPTIONS (f, value, FLOAT, UINT);
	}

      aarch64_set_reg_u64 (cpu, rd, NO_SP, value);
    }
}

static void
do_UCVTF (sim_cpu *cpu)
{
  /* instr[31]    = size: 32-bit (0), 64-bit (1)
     instr[30,23] = 001 1110 0
     instr[22]    = type: single (0)/ double (1)
     instr[21]    = enable (0)/disable(1) precision
     instr[20,16] = 0 0011
     instr[15,10] = precision
     instr[9,5]   = Rs
     instr[4,0]   = Rd.  */

  unsigned rs = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (30, 23, 0x3C);
  NYI_assert (20, 16, 0x03);

  if (INSTR (21, 21) != 1)
    HALT_NYI;

  /* FIXME: Add exception raising.  */
  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (31, 31))
    {
      uint64_t value = aarch64_get_reg_u64 (cpu, rs, NO_SP);

      if (INSTR (22, 22))
	aarch64_set_FP_double (cpu, rd, (double) value);
      else
	aarch64_set_FP_float (cpu, rd, (float) value);
    }
  else
    {
      uint32_t value =  aarch64_get_reg_u32 (cpu, rs, NO_SP);

      if (INSTR (22, 22))
	aarch64_set_FP_double (cpu, rd, (double) value);
      else
	aarch64_set_FP_float (cpu, rd, (float) value);
    }
}

static void
float_vector_move (sim_cpu *cpu)
{
  /* instr[31,17] == 100 1111 0101 0111
     instr[16]    ==> direction 0=> to GR, 1=> from GR
     instr[15,10] => ???
     instr[9,5]   ==> source
     instr[4,0]   ==> dest.  */

  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (31, 17, 0x4F57);

  if (INSTR (15, 10) != 0)
    HALT_UNALLOC;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (16, 16))
    aarch64_set_vec_u64 (cpu, rd, 1, aarch64_get_reg_u64 (cpu, rn, NO_SP));
  else
    aarch64_set_reg_u64 (cpu, rd, NO_SP, aarch64_get_vec_u64 (cpu, rn, 1));
}

static void
dexSimpleFPIntegerConvert (sim_cpu *cpu)
{
  /* instr[31]    = size : 0 ==> 32 bit, 1 ==> 64 bit
     instr[30     = 0
     instr[29]    = S :  0 ==> OK, 1 ==> UNALLOC
     instr[28,25] = 1111
     instr[24]    = 0
     instr[23,22] = type : 00 ==> single, 01 ==> double, 1x ==> UNALLOC
     instr[21]    = 1
     instr[20,19] = rmode
     instr[18,16] = opcode
     instr[15,10] = 10 0000  */

  uint32_t rmode_opcode;
  uint32_t size_type;
  uint32_t type;
  uint32_t size;
  uint32_t S;

  if (INSTR (31, 17) == 0x4F57)
    {
      float_vector_move (cpu);
      return;
    }

  size = INSTR (31, 31);
  S = INSTR (29, 29);
  if (S != 0)
    HALT_UNALLOC;

  type = INSTR (23, 22);
  if (type > 1)
    HALT_UNALLOC;

  rmode_opcode = INSTR (20, 16);
  size_type = (size << 1) | type; /* 0==32f, 1==32d, 2==64f, 3==64d.  */

  switch (rmode_opcode)
    {
    case 2:			/* SCVTF.  */
      switch (size_type)
	{
	case 0: scvtf32 (cpu); return;
	case 1: scvtd32 (cpu); return;
	case 2: scvtf (cpu); return;
	case 3: scvtd (cpu); return;
	default: HALT_UNALLOC;
	}

    case 6:			/* FMOV GR, Vec.  */
      switch (size_type)
	{
	case 0:  gfmovs (cpu); return;
	case 3:  gfmovd (cpu); return;
	default: HALT_UNALLOC;
	}

    case 7:			/* FMOV vec, GR.  */
      switch (size_type)
	{
	case 0:  fgmovs (cpu); return;
	case 3:  fgmovd (cpu); return;
	default: HALT_UNALLOC;
	}

    case 24:			/* FCVTZS.  */
      switch (size_type)
	{
	case 0: fcvtszs32 (cpu); return;
	case 1: fcvtszd32 (cpu); return;
	case 2: fcvtszs (cpu); return;
	case 3: fcvtszd (cpu); return;
	default: HALT_UNALLOC;
	}

    case 25: do_fcvtzu (cpu); return;
    case 3:  do_UCVTF (cpu); return;

    case 0:	/* FCVTNS.  */
    case 1:	/* FCVTNU.  */
    case 4:	/* FCVTAS.  */
    case 5:	/* FCVTAU.  */
    case 8:	/* FCVPTS.  */
    case 9:	/* FCVTPU.  */
    case 16:	/* FCVTMS.  */
    case 17:	/* FCVTMU.  */
    default:
      HALT_NYI;
    }
}

static void
set_flags_for_float_compare (sim_cpu *cpu, float fvalue1, float fvalue2)
{
  uint32_t flags;

  /* FIXME: Add exception raising.  */
  if (isnan (fvalue1) || isnan (fvalue2))
    flags = C|V;
  else if (isinf (fvalue1) && isinf (fvalue2))
    {
      /* Subtracting two infinities may give a NaN.  We only need to compare
	 the signs, which we can get from isinf.  */
      int result = isinf (fvalue1) - isinf (fvalue2);

      if (result == 0)
	flags = Z|C;
      else if (result < 0)
	flags = N;
      else /* (result > 0).  */
	flags = C;
    }
  else
    {
      float result = fvalue1 - fvalue2;

      if (result == 0.0)
	flags = Z|C;
      else if (result < 0)
	flags = N;
      else /* (result > 0).  */
	flags = C;
    }

  aarch64_set_CPSR (cpu, flags);
}

static void
fcmps (sim_cpu *cpu)
{
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);

  float fvalue1 = aarch64_get_FP_float (cpu, sn);
  float fvalue2 = aarch64_get_FP_float (cpu, sm);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  set_flags_for_float_compare (cpu, fvalue1, fvalue2);
}

/* Float compare to zero -- Invalid Operation exception
   only on signaling NaNs.  */
static void
fcmpzs (sim_cpu *cpu)
{
  unsigned sn = INSTR ( 9,  5);
  float fvalue1 = aarch64_get_FP_float (cpu, sn);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  set_flags_for_float_compare (cpu, fvalue1, 0.0f);
}

/* Float compare -- Invalid Operation exception on all NaNs.  */
static void
fcmpes (sim_cpu *cpu)
{
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);

  float fvalue1 = aarch64_get_FP_float (cpu, sn);
  float fvalue2 = aarch64_get_FP_float (cpu, sm);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  set_flags_for_float_compare (cpu, fvalue1, fvalue2);
}

/* Float compare to zero -- Invalid Operation exception on all NaNs.  */
static void
fcmpzes (sim_cpu *cpu)
{
  unsigned sn = INSTR ( 9,  5);
  float fvalue1 = aarch64_get_FP_float (cpu, sn);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  set_flags_for_float_compare (cpu, fvalue1, 0.0f);
}

static void
set_flags_for_double_compare (sim_cpu *cpu, double dval1, double dval2)
{
  uint32_t flags;

  /* FIXME: Add exception raising.  */
  if (isnan (dval1) || isnan (dval2))
    flags = C|V;
  else if (isinf (dval1) && isinf (dval2))
    {
      /* Subtracting two infinities may give a NaN.  We only need to compare
	 the signs, which we can get from isinf.  */
      int result = isinf (dval1) - isinf (dval2);

      if (result == 0)
	flags = Z|C;
      else if (result < 0)
	flags = N;
      else /* (result > 0).  */
	flags = C;
    }
  else
    {
      double result = dval1 - dval2;

      if (result == 0.0)
	flags = Z|C;
      else if (result < 0)
	flags = N;
      else /* (result > 0).  */
	flags = C;
    }

  aarch64_set_CPSR (cpu, flags);
}

/* Double compare -- Invalid Operation exception only on signaling NaNs.  */
static void
fcmpd (sim_cpu *cpu)
{
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);

  double dvalue1 = aarch64_get_FP_double (cpu, sn);
  double dvalue2 = aarch64_get_FP_double (cpu, sm);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  set_flags_for_double_compare (cpu, dvalue1, dvalue2);
}

/* Double compare to zero -- Invalid Operation exception
   only on signaling NaNs.  */
static void
fcmpzd (sim_cpu *cpu)
{
  unsigned sn = INSTR ( 9,  5);
  double dvalue1 = aarch64_get_FP_double (cpu, sn);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  set_flags_for_double_compare (cpu, dvalue1, 0.0);
}

/* Double compare -- Invalid Operation exception on all NaNs.  */
static void
fcmped (sim_cpu *cpu)
{
  unsigned sm = INSTR (20, 16);
  unsigned sn = INSTR ( 9,  5);

  double dvalue1 = aarch64_get_FP_double (cpu, sn);
  double dvalue2 = aarch64_get_FP_double (cpu, sm);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  set_flags_for_double_compare (cpu, dvalue1, dvalue2);
}

/* Double compare to zero -- Invalid Operation exception on all NaNs.  */
static void
fcmpzed (sim_cpu *cpu)
{
  unsigned sn = INSTR ( 9,  5);
  double dvalue1 = aarch64_get_FP_double (cpu, sn);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  set_flags_for_double_compare (cpu, dvalue1, 0.0);
}

static void
dexSimpleFPCompare (sim_cpu *cpu)
{
  /* assert instr[28,25] == 1111
     instr[30:24:21:13,10] = 0011000
     instr[31] = M : 0 ==> OK, 1 ==> UNALLOC
     instr[29] ==> S :  0 ==> OK, 1 ==> UNALLOC
     instr[23,22] ==> type : 0 ==> single, 01 ==> double, 1x ==> UNALLOC
     instr[15,14] ==> op : 00 ==> OK, ow ==> UNALLOC
     instr[4,0] ==> opcode2 : 00000 ==> FCMP, 10000 ==> FCMPE,
                              01000 ==> FCMPZ, 11000 ==> FCMPEZ,
                              ow ==> UNALLOC  */
  uint32_t dispatch;
  uint32_t M_S = (INSTR (31, 31) << 1) | INSTR (29, 29);
  uint32_t type = INSTR (23, 22);
  uint32_t op = INSTR (15, 14);
  uint32_t op2_2_0 = INSTR (2, 0);

  if (op2_2_0 != 0)
    HALT_UNALLOC;

  if (M_S != 0)
    HALT_UNALLOC;

  if (type > 1)
    HALT_UNALLOC;

  if (op != 0)
    HALT_UNALLOC;

  /* dispatch on type and top 2 bits of opcode.  */
  dispatch = (type << 2) | INSTR (4, 3);

  switch (dispatch)
    {
    case 0: fcmps (cpu); return;
    case 1: fcmpzs (cpu); return;
    case 2: fcmpes (cpu); return;
    case 3: fcmpzes (cpu); return;
    case 4: fcmpd (cpu); return;
    case 5: fcmpzd (cpu); return;
    case 6: fcmped (cpu); return;
    case 7: fcmpzed (cpu); return;
    }
}

static void
do_scalar_FADDP (sim_cpu *cpu)
{
  /* instr [31,23] = 0111 1110 0
     instr [22]    = single(0)/double(1)
     instr [21,10] = 11 0000 1101 10
     instr [9,5]   = Fn
     instr [4,0]   = Fd.  */

  unsigned Fn = INSTR (9, 5);
  unsigned Fd = INSTR (4, 0);

  NYI_assert (31, 23, 0x0FC);
  NYI_assert (21, 10, 0xC36);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      double val1 = aarch64_get_vec_double (cpu, Fn, 0);
      double val2 = aarch64_get_vec_double (cpu, Fn, 1);

      aarch64_set_FP_double (cpu, Fd, val1 + val2);
    }
  else
    {
      float val1 = aarch64_get_vec_float (cpu, Fn, 0);
      float val2 = aarch64_get_vec_float (cpu, Fn, 1);

      aarch64_set_FP_float (cpu, Fd, val1 + val2);
    }
}

/* Floating point absolute difference.  */

static void
do_scalar_FABD (sim_cpu *cpu)
{
  /* instr [31,23] = 0111 1110 1
     instr [22]    = float(0)/double(1)
     instr [21]    = 1
     instr [20,16] = Rm
     instr [15,10] = 1101 01
     instr [9, 5]  = Rn
     instr [4, 0]  = Rd.  */

  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (31, 23, 0x0FD);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 10, 0x35);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    aarch64_set_FP_double (cpu, rd,
			   fabs (aarch64_get_FP_double (cpu, rn)
				 - aarch64_get_FP_double (cpu, rm)));
  else
    aarch64_set_FP_float (cpu, rd,
			  fabsf (aarch64_get_FP_float (cpu, rn)
				 - aarch64_get_FP_float (cpu, rm)));
}

static void
do_scalar_CMGT (sim_cpu *cpu)
{
  /* instr [31,21] = 0101 1110 111
     instr [20,16] = Rm
     instr [15,10] = 00 1101
     instr [9, 5]  = Rn
     instr [4, 0]  = Rd.  */

  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (31, 21, 0x2F7);
  NYI_assert (15, 10, 0x0D);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u64 (cpu, rd, 0,
		       aarch64_get_vec_u64 (cpu, rn, 0) >
		       aarch64_get_vec_u64 (cpu, rm, 0) ? -1L : 0L);
}

static void
do_scalar_USHR (sim_cpu *cpu)
{
  /* instr [31,23] = 0111 1111 0
     instr [22,16] = shift amount
     instr [15,10] = 0000 01
     instr [9, 5]  = Rn
     instr [4, 0]  = Rd.  */

  unsigned amount = 128 - INSTR (22, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (31, 23, 0x0FE);
  NYI_assert (15, 10, 0x01);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u64 (cpu, rd, 0,
		       aarch64_get_vec_u64 (cpu, rn, 0) >> amount);
}

static void
do_scalar_SSHL (sim_cpu *cpu)
{
  /* instr [31,21] = 0101 1110 111
     instr [20,16] = Rm
     instr [15,10] = 0100 01
     instr [9, 5]  = Rn
     instr [4, 0]  = Rd.  */

  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  signed int shift = aarch64_get_vec_s8 (cpu, rm, 0);

  NYI_assert (31, 21, 0x2F7);
  NYI_assert (15, 10, 0x11);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (shift >= 0)
    aarch64_set_vec_s64 (cpu, rd, 0,
			 aarch64_get_vec_s64 (cpu, rn, 0) << shift);
  else
    aarch64_set_vec_s64 (cpu, rd, 0,
			 aarch64_get_vec_s64 (cpu, rn, 0) >> - shift);
}

/* Floating point scalar compare greater than or equal to 0.  */
static void
do_scalar_FCMGE_zero (sim_cpu *cpu)
{
  /* instr [31,23] = 0111 1110 1
     instr [22,22] = size
     instr [21,16] = 1000 00
     instr [15,10] = 1100 10
     instr [9, 5]  = Rn
     instr [4, 0]  = Rd.  */

  unsigned size = INSTR (22, 22);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (31, 23, 0x0FD);
  NYI_assert (21, 16, 0x20);
  NYI_assert (15, 10, 0x32);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (size)
    aarch64_set_vec_u64 (cpu, rd, 0,
			 aarch64_get_vec_double (cpu, rn, 0) >= 0.0 ? -1 : 0);
  else
    aarch64_set_vec_u32 (cpu, rd, 0,
			 aarch64_get_vec_float (cpu, rn, 0) >= 0.0 ? -1 : 0);
}

/* Floating point scalar compare less than or equal to 0.  */
static void
do_scalar_FCMLE_zero (sim_cpu *cpu)
{
  /* instr [31,23] = 0111 1110 1
     instr [22,22] = size
     instr [21,16] = 1000 00
     instr [15,10] = 1101 10
     instr [9, 5]  = Rn
     instr [4, 0]  = Rd.  */

  unsigned size = INSTR (22, 22);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (31, 23, 0x0FD);
  NYI_assert (21, 16, 0x20);
  NYI_assert (15, 10, 0x36);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (size)
    aarch64_set_vec_u64 (cpu, rd, 0,
			 aarch64_get_vec_double (cpu, rn, 0) <= 0.0 ? -1 : 0);
  else
    aarch64_set_vec_u32 (cpu, rd, 0,
			 aarch64_get_vec_float (cpu, rn, 0) <= 0.0 ? -1 : 0);
}

/* Floating point scalar compare greater than 0.  */
static void
do_scalar_FCMGT_zero (sim_cpu *cpu)
{
  /* instr [31,23] = 0101 1110 1
     instr [22,22] = size
     instr [21,16] = 1000 00
     instr [15,10] = 1100 10
     instr [9, 5]  = Rn
     instr [4, 0]  = Rd.  */

  unsigned size = INSTR (22, 22);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (31, 23, 0x0BD);
  NYI_assert (21, 16, 0x20);
  NYI_assert (15, 10, 0x32);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (size)
    aarch64_set_vec_u64 (cpu, rd, 0,
			 aarch64_get_vec_double (cpu, rn, 0) > 0.0 ? -1 : 0);
  else
    aarch64_set_vec_u32 (cpu, rd, 0,
			 aarch64_get_vec_float (cpu, rn, 0) > 0.0 ? -1 : 0);
}

/* Floating point scalar compare equal to 0.  */
static void
do_scalar_FCMEQ_zero (sim_cpu *cpu)
{
  /* instr [31,23] = 0101 1110 1
     instr [22,22] = size
     instr [21,16] = 1000 00
     instr [15,10] = 1101 10
     instr [9, 5]  = Rn
     instr [4, 0]  = Rd.  */

  unsigned size = INSTR (22, 22);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (31, 23, 0x0BD);
  NYI_assert (21, 16, 0x20);
  NYI_assert (15, 10, 0x36);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (size)
    aarch64_set_vec_u64 (cpu, rd, 0,
			 aarch64_get_vec_double (cpu, rn, 0) == 0.0 ? -1 : 0);
  else
    aarch64_set_vec_u32 (cpu, rd, 0,
			 aarch64_get_vec_float (cpu, rn, 0) == 0.0 ? -1 : 0);
}

/* Floating point scalar compare less than 0.  */
static void
do_scalar_FCMLT_zero (sim_cpu *cpu)
{
  /* instr [31,23] = 0101 1110 1
     instr [22,22] = size
     instr [21,16] = 1000 00
     instr [15,10] = 1110 10
     instr [9, 5]  = Rn
     instr [4, 0]  = Rd.  */

  unsigned size = INSTR (22, 22);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (31, 23, 0x0BD);
  NYI_assert (21, 16, 0x20);
  NYI_assert (15, 10, 0x3A);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (size)
    aarch64_set_vec_u64 (cpu, rd, 0,
			 aarch64_get_vec_double (cpu, rn, 0) < 0.0 ? -1 : 0);
  else
    aarch64_set_vec_u32 (cpu, rd, 0,
			 aarch64_get_vec_float (cpu, rn, 0) < 0.0 ? -1 : 0);
}

static void
do_scalar_shift (sim_cpu *cpu)
{
  /* instr [31,23] = 0101 1111 0
     instr [22,16] = shift amount
     instr [15,10] = 0101 01   [SHL]
     instr [15,10] = 0000 01   [SSHR]
     instr [9, 5]  = Rn
     instr [4, 0]  = Rd.  */

  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  unsigned amount;

  NYI_assert (31, 23, 0x0BE);

  if (INSTR (22, 22) == 0)
    HALT_UNALLOC;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  switch (INSTR (15, 10))
    {
    case 0x01: /* SSHR */
      amount = 128 - INSTR (22, 16);
      aarch64_set_vec_s64 (cpu, rd, 0,
			   aarch64_get_vec_s64 (cpu, rn, 0) >> amount);
      return;
    case 0x15: /* SHL */
      amount = INSTR (22, 16) - 64;
      aarch64_set_vec_u64 (cpu, rd, 0,
			   aarch64_get_vec_u64 (cpu, rn, 0) << amount);
      return;
    default:
      HALT_NYI;
    }
}

/* FCMEQ FCMGT FCMGE.  */
static void
do_scalar_FCM (sim_cpu *cpu)
{
  /* instr [31,30] = 01
     instr [29]    = U
     instr [28,24] = 1 1110
     instr [23]    = E
     instr [22]    = size
     instr [21]    = 1
     instr [20,16] = Rm
     instr [15,12] = 1110
     instr [11]    = AC
     instr [10]    = 1
     instr [9, 5]  = Rn
     instr [4, 0]  = Rd.  */

  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  unsigned EUac = (INSTR (23, 23) << 2) | (INSTR (29, 29) << 1) | INSTR (11, 11);
  unsigned result;
  float val1;
  float val2;

  NYI_assert (31, 30, 1);
  NYI_assert (28, 24, 0x1E);
  NYI_assert (21, 21, 1);
  NYI_assert (15, 12, 0xE);
  NYI_assert (10, 10, 1);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      double dval1 = aarch64_get_FP_double (cpu, rn);
      double dval2 = aarch64_get_FP_double (cpu, rm);

      switch (EUac)
	{
	case 0: /* 000 */
	  result = dval1 == dval2;
	  break;

	case 3: /* 011 */
	  dval1 = fabs (dval1);
	  dval2 = fabs (dval2);
	  ATTRIBUTE_FALLTHROUGH;
	case 2: /* 010 */
	  result = dval1 >= dval2;
	  break;

	case 7: /* 111 */
	  dval1 = fabs (dval1);
	  dval2 = fabs (dval2);
	  ATTRIBUTE_FALLTHROUGH;
	case 6: /* 110 */
	  result = dval1 > dval2;
	  break;

	default:
	  HALT_UNALLOC;
	}

      aarch64_set_vec_u32 (cpu, rd, 0, result ? -1 : 0);
      return;
    }

  val1 = aarch64_get_FP_float (cpu, rn);
  val2 = aarch64_get_FP_float (cpu, rm);

  switch (EUac)
    {
    case 0: /* 000 */
      result = val1 == val2;
      break;

    case 3: /* 011 */
      val1 = fabsf (val1);
      val2 = fabsf (val2);
      ATTRIBUTE_FALLTHROUGH;
    case 2: /* 010 */
      result = val1 >= val2;
      break;

    case 7: /* 111 */
      val1 = fabsf (val1);
      val2 = fabsf (val2);
      ATTRIBUTE_FALLTHROUGH;
    case 6: /* 110 */
      result = val1 > val2;
      break;

    default:
      HALT_UNALLOC;
    }

  aarch64_set_vec_u32 (cpu, rd, 0, result ? -1 : 0);
}

/* An alias of DUP.  */
static void
do_scalar_MOV (sim_cpu *cpu)
{
  /* instr [31,21] = 0101 1110 000
     instr [20,16] = imm5
     instr [15,10] = 0000 01
     instr [9, 5]  = Rn
     instr [4, 0]  = Rd.  */

  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  unsigned index;

  NYI_assert (31, 21, 0x2F0);
  NYI_assert (15, 10, 0x01);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (16, 16))
    {
      /* 8-bit.  */
      index = INSTR (20, 17);
      aarch64_set_vec_u8
	(cpu, rd, 0, aarch64_get_vec_u8 (cpu, rn, index));
    }
  else if (INSTR (17, 17))
    {
      /* 16-bit.  */
      index = INSTR (20, 18);
      aarch64_set_vec_u16
	(cpu, rd, 0, aarch64_get_vec_u16 (cpu, rn, index));
    }
  else if (INSTR (18, 18))
    {
      /* 32-bit.  */
      index = INSTR (20, 19);
      aarch64_set_vec_u32
	(cpu, rd, 0, aarch64_get_vec_u32 (cpu, rn, index));
    }
  else if (INSTR (19, 19))
    {
      /* 64-bit.  */
      index = INSTR (20, 20);
      aarch64_set_vec_u64
	(cpu, rd, 0, aarch64_get_vec_u64 (cpu, rn, index));
    }
  else
    HALT_UNALLOC;
}

static void
do_scalar_NEG (sim_cpu *cpu)
{
  /* instr [31,10] = 0111 1110 1110 0000 1011 10
     instr [9, 5]  = Rn
     instr [4, 0]  = Rd.  */

  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (31, 10, 0x1FB82E);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_vec_u64 (cpu, rd, 0, - aarch64_get_vec_u64 (cpu, rn, 0));
}

static void
do_scalar_USHL (sim_cpu *cpu)
{
  /* instr [31,21] = 0111 1110 111
     instr [20,16] = Rm
     instr [15,10] = 0100 01
     instr [9, 5]  = Rn
     instr [4, 0]  = Rd.  */

  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  signed int shift = aarch64_get_vec_s8 (cpu, rm, 0);

  NYI_assert (31, 21, 0x3F7);
  NYI_assert (15, 10, 0x11);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (shift >= 0)
    aarch64_set_vec_u64 (cpu, rd, 0, aarch64_get_vec_u64 (cpu, rn, 0) << shift);
  else
    aarch64_set_vec_u64 (cpu, rd, 0, aarch64_get_vec_u64 (cpu, rn, 0) >> - shift);
}

static void
do_double_add (sim_cpu *cpu)
{
  /* instr [31,21] = 0101 1110 111
     instr [20,16] = Fn
     instr [15,10] = 1000 01
     instr [9,5]   = Fm
     instr [4,0]   = Fd.  */
  unsigned Fd;
  unsigned Fm;
  unsigned Fn;
  double val1;
  double val2;

  NYI_assert (31, 21, 0x2F7);
  NYI_assert (15, 10, 0x21);

  Fd = INSTR (4, 0);
  Fm = INSTR (9, 5);
  Fn = INSTR (20, 16);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  val1 = aarch64_get_FP_double (cpu, Fm);
  val2 = aarch64_get_FP_double (cpu, Fn);

  aarch64_set_FP_double (cpu, Fd, val1 + val2);
}

static void
do_scalar_UCVTF (sim_cpu *cpu)
{
  /* instr [31,23] = 0111 1110 0
     instr [22]    = single(0)/double(1)
     instr [21,10] = 10 0001 1101 10
     instr [9,5]   = rn
     instr [4,0]   = rd.  */

  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  NYI_assert (31, 23, 0x0FC);
  NYI_assert (21, 10, 0x876);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (INSTR (22, 22))
    {
      uint64_t val = aarch64_get_vec_u64 (cpu, rn, 0);

      aarch64_set_vec_double (cpu, rd, 0, (double) val);
    }
  else
    {
      uint32_t val = aarch64_get_vec_u32 (cpu, rn, 0);

      aarch64_set_vec_float (cpu, rd, 0, (float) val);
    }
}

static void
do_scalar_vec (sim_cpu *cpu)
{
  /* instr [30] = 1.  */
  /* instr [28,25] = 1111.  */
  switch (INSTR (31, 23))
    {
    case 0xBC:
      switch (INSTR (15, 10))
	{
	case 0x01: do_scalar_MOV (cpu); return;
	case 0x39: do_scalar_FCM (cpu); return;
	case 0x3B: do_scalar_FCM (cpu); return;
	}
      break;

    case 0xBE: do_scalar_shift (cpu); return;

    case 0xFC:
      switch (INSTR (15, 10))
	{
	case 0x36:
	  switch (INSTR (21, 16))
	    {
	    case 0x30: do_scalar_FADDP (cpu); return;
	    case 0x21: do_scalar_UCVTF (cpu); return;
	    }
	  HALT_NYI;
	case 0x39: do_scalar_FCM (cpu); return;
	case 0x3B: do_scalar_FCM (cpu); return;
	}
      break;

    case 0xFD:
      switch (INSTR (15, 10))
	{
	case 0x0D: do_scalar_CMGT (cpu); return;
	case 0x11: do_scalar_USHL (cpu); return;
	case 0x2E: do_scalar_NEG (cpu); return;
	case 0x32: do_scalar_FCMGE_zero (cpu); return;
	case 0x35: do_scalar_FABD (cpu); return;
	case 0x36: do_scalar_FCMLE_zero (cpu); return;
	case 0x39: do_scalar_FCM (cpu); return;
	case 0x3B: do_scalar_FCM (cpu); return;
	default:
	  HALT_NYI;
	}

    case 0xFE: do_scalar_USHR (cpu); return;

    case 0xBD:
      switch (INSTR (15, 10))
	{
	case 0x21: do_double_add (cpu); return;
	case 0x11: do_scalar_SSHL (cpu); return;
	case 0x32: do_scalar_FCMGT_zero (cpu); return;
	case 0x36: do_scalar_FCMEQ_zero (cpu); return;
	case 0x3A: do_scalar_FCMLT_zero (cpu); return;
	default:
	  HALT_NYI;
	}

    default:
      HALT_NYI;
    }
}

static void
dexAdvSIMD1 (sim_cpu *cpu)
{
  /* instr [28,25] = 1 111.  */

  /* We are currently only interested in the basic
     scalar fp routines which all have bit 30 = 0.  */
  if (INSTR (30, 30))
    do_scalar_vec (cpu);

  /* instr[24] is set for FP data processing 3-source and clear for
     all other basic scalar fp instruction groups.  */
  else if (INSTR (24, 24))
    dexSimpleFPDataProc3Source (cpu);

  /* instr[21] is clear for floating <-> fixed conversions and set for
     all other basic scalar fp instruction groups.  */
  else if (!INSTR (21, 21))
    dexSimpleFPFixedConvert (cpu);

  /* instr[11,10] : 01 ==> cond compare, 10 ==> Data Proc 2 Source
     11 ==> cond select,  00 ==> other.  */
  else
    switch (INSTR (11, 10))
      {
      case 1: dexSimpleFPCondCompare (cpu); return;
      case 2: dexSimpleFPDataProc2Source (cpu); return;
      case 3: dexSimpleFPCondSelect (cpu); return;

      default:
	/* Now an ordered cascade of tests.
	   FP immediate has instr [12] == 1.
	   FP compare has   instr [13] == 1.
	   FP Data Proc 1 Source has instr [14] == 1.
	   FP floating <--> integer conversions has instr [15] == 0.  */
	if (INSTR (12, 12))
	  dexSimpleFPImmediate (cpu);

	else if (INSTR (13, 13))
	  dexSimpleFPCompare (cpu);

	else if (INSTR (14, 14))
	  dexSimpleFPDataProc1Source (cpu);

	else if (!INSTR (15, 15))
	  dexSimpleFPIntegerConvert (cpu);

	else
	  /* If we get here then instr[15] == 1 which means UNALLOC.  */
	  HALT_UNALLOC;
      }
}

/* PC relative addressing.  */

static void
pcadr (sim_cpu *cpu)
{
  /* instr[31] = op : 0 ==> ADR, 1 ==> ADRP
     instr[30,29] = immlo
     instr[23,5] = immhi.  */
  uint64_t address;
  unsigned rd = INSTR (4, 0);
  uint32_t isPage = INSTR (31, 31);
  union { int64_t u64; uint64_t s64; } imm;
  uint64_t offset;

  imm.s64 = simm64 (aarch64_get_instr (cpu), 23, 5);
  offset = imm.u64;
  offset = (offset << 2) | INSTR (30, 29);

  address = aarch64_get_PC (cpu);

  if (isPage)
    {
      offset <<= 12;
      address &= ~0xfff;
    }

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, address + offset);
}

/* Specific decode and execute for group Data Processing Immediate.  */

static void
dexPCRelAddressing (sim_cpu *cpu)
{
  /* assert instr[28,24] = 10000.  */
  pcadr (cpu);
}

/* Immediate logical.
   The bimm32/64 argument is constructed by replicating a 2, 4, 8,
   16, 32 or 64 bit sequence pulled out at decode and possibly
   inverting it..

   N.B. the output register (dest) can normally be Xn or SP
   the exception occurs for flag setting instructions which may
   only use Xn for the output (dest).  The input register can
   never be SP.  */

/* 32 bit and immediate.  */
static void
and32 (sim_cpu *cpu, uint32_t bimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, SP_OK,
		       aarch64_get_reg_u32 (cpu, rn, NO_SP) & bimm);
}

/* 64 bit and immediate.  */
static void
and64 (sim_cpu *cpu, uint64_t bimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, SP_OK,
		       aarch64_get_reg_u64 (cpu, rn, NO_SP) & bimm);
}

/* 32 bit and immediate set flags.  */
static void
ands32 (sim_cpu *cpu, uint32_t bimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint32_t value1 = aarch64_get_reg_u32 (cpu, rn, NO_SP);
  uint32_t value2 = bimm;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 & value2);
  set_flags_for_binop32 (cpu, value1 & value2);
}

/* 64 bit and immediate set flags.  */
static void
ands64 (sim_cpu *cpu, uint64_t bimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint64_t value1 = aarch64_get_reg_u64 (cpu, rn, NO_SP);
  uint64_t value2 = bimm;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 & value2);
  set_flags_for_binop64 (cpu, value1 & value2);
}

/* 32 bit exclusive or immediate.  */
static void
eor32 (sim_cpu *cpu, uint32_t bimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, SP_OK,
		       aarch64_get_reg_u32 (cpu, rn, NO_SP) ^ bimm);
}

/* 64 bit exclusive or immediate.  */
static void
eor64 (sim_cpu *cpu, uint64_t bimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, SP_OK,
		       aarch64_get_reg_u64 (cpu, rn, NO_SP) ^ bimm);
}

/* 32 bit or immediate.  */
static void
orr32 (sim_cpu *cpu, uint32_t bimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, SP_OK,
		       aarch64_get_reg_u32 (cpu, rn, NO_SP) | bimm);
}

/* 64 bit or immediate.  */
static void
orr64 (sim_cpu *cpu, uint64_t bimm)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, SP_OK,
		       aarch64_get_reg_u64 (cpu, rn, NO_SP) | bimm);
}

/* Logical shifted register.
   These allow an optional LSL, ASR, LSR or ROR to the second source
   register with a count up to the register bit count.
   N.B register args may not be SP.  */

/* 32 bit AND shifted register.  */
static void
and32_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, aarch64_get_reg_u32 (cpu, rn, NO_SP)
     & shifted32 (aarch64_get_reg_u32 (cpu, rm, NO_SP), shift, count));
}

/* 64 bit AND shifted register.  */
static void
and64_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, aarch64_get_reg_u64 (cpu, rn, NO_SP)
     & shifted64 (aarch64_get_reg_u64 (cpu, rm, NO_SP), shift, count));
}

/* 32 bit AND shifted register setting flags.  */
static void
ands32_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint32_t value1 = aarch64_get_reg_u32 (cpu, rn, NO_SP);
  uint32_t value2 = shifted32 (aarch64_get_reg_u32 (cpu, rm, NO_SP),
			       shift, count);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 & value2);
  set_flags_for_binop32 (cpu, value1 & value2);
}

/* 64 bit AND shifted register setting flags.  */
static void
ands64_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint64_t value1 = aarch64_get_reg_u64 (cpu, rn, NO_SP);
  uint64_t value2 = shifted64 (aarch64_get_reg_u64 (cpu, rm, NO_SP),
			       shift, count);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 & value2);
  set_flags_for_binop64 (cpu, value1 & value2);
}

/* 32 bit BIC shifted register.  */
static void
bic32_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, aarch64_get_reg_u32 (cpu, rn, NO_SP)
     & ~ shifted32 (aarch64_get_reg_u32 (cpu, rm, NO_SP), shift, count));
}

/* 64 bit BIC shifted register.  */
static void
bic64_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, aarch64_get_reg_u64 (cpu, rn, NO_SP)
     & ~ shifted64 (aarch64_get_reg_u64 (cpu, rm, NO_SP), shift, count));
}

/* 32 bit BIC shifted register setting flags.  */
static void
bics32_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint32_t value1 = aarch64_get_reg_u32 (cpu, rn, NO_SP);
  uint32_t value2 = ~ shifted32 (aarch64_get_reg_u32 (cpu, rm, NO_SP),
				 shift, count);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 & value2);
  set_flags_for_binop32 (cpu, value1 & value2);
}

/* 64 bit BIC shifted register setting flags.  */
static void
bics64_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  uint64_t value1 = aarch64_get_reg_u64 (cpu, rn, NO_SP);
  uint64_t value2 = ~ shifted64 (aarch64_get_reg_u64 (cpu, rm, NO_SP),
				 shift, count);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value1 & value2);
  set_flags_for_binop64 (cpu, value1 & value2);
}

/* 32 bit EON shifted register.  */
static void
eon32_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, aarch64_get_reg_u32 (cpu, rn, NO_SP)
     ^ ~ shifted32 (aarch64_get_reg_u32 (cpu, rm, NO_SP), shift, count));
}

/* 64 bit EON shifted register.  */
static void
eon64_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, aarch64_get_reg_u64 (cpu, rn, NO_SP)
     ^ ~ shifted64 (aarch64_get_reg_u64 (cpu, rm, NO_SP), shift, count));
}

/* 32 bit EOR shifted register.  */
static void
eor32_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, aarch64_get_reg_u32 (cpu, rn, NO_SP)
     ^ shifted32 (aarch64_get_reg_u32 (cpu, rm, NO_SP), shift, count));
}

/* 64 bit EOR shifted register.  */
static void
eor64_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, aarch64_get_reg_u64 (cpu, rn, NO_SP)
     ^ shifted64 (aarch64_get_reg_u64 (cpu, rm, NO_SP), shift, count));
}

/* 32 bit ORR shifted register.  */
static void
orr32_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, aarch64_get_reg_u32 (cpu, rn, NO_SP)
     | shifted32 (aarch64_get_reg_u32 (cpu, rm, NO_SP), shift, count));
}

/* 64 bit ORR shifted register.  */
static void
orr64_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, aarch64_get_reg_u64 (cpu, rn, NO_SP)
     | shifted64 (aarch64_get_reg_u64 (cpu, rm, NO_SP), shift, count));
}

/* 32 bit ORN shifted register.  */
static void
orn32_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, aarch64_get_reg_u32 (cpu, rn, NO_SP)
     | ~ shifted32 (aarch64_get_reg_u32 (cpu, rm, NO_SP), shift, count));
}

/* 64 bit ORN shifted register.  */
static void
orn64_shift (sim_cpu *cpu, Shift shift, uint32_t count)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, aarch64_get_reg_u64 (cpu, rn, NO_SP)
     | ~ shifted64 (aarch64_get_reg_u64 (cpu, rm, NO_SP), shift, count));
}

static void
dexLogicalImmediate (sim_cpu *cpu)
{
  /* assert instr[28,23] = 1001000
     instr[31] = size : 0 ==> 32 bit, 1 ==> 64 bit
     instr[30,29] = op : 0 ==> AND, 1 ==> ORR, 2 ==> EOR, 3 ==> ANDS
     instr[22] = N : used to construct immediate mask
     instr[21,16] = immr
     instr[15,10] = imms
     instr[9,5] = Rn
     instr[4,0] = Rd  */

  /* 32 bit operations must have N = 0 or else we have an UNALLOC.  */
  uint32_t size = INSTR (31, 31);
  uint32_t N = INSTR (22, 22);
  /* uint32_t immr = INSTR (21, 16);.  */
  /* uint32_t imms = INSTR (15, 10);.  */
  uint32_t index = INSTR (22, 10);
  uint64_t bimm64 = LITable [index];
  uint32_t dispatch = INSTR (30, 29);

  if (~size & N)
    HALT_UNALLOC;

  if (!bimm64)
    HALT_UNALLOC;

  if (size == 0)
    {
      uint32_t bimm = (uint32_t) bimm64;

      switch (dispatch)
	{
	case 0: and32 (cpu, bimm); return;
	case 1: orr32 (cpu, bimm); return;
	case 2: eor32 (cpu, bimm); return;
	case 3: ands32 (cpu, bimm); return;
	}
    }
  else
    {
      switch (dispatch)
	{
	case 0: and64 (cpu, bimm64); return;
	case 1: orr64 (cpu, bimm64); return;
	case 2: eor64 (cpu, bimm64); return;
	case 3: ands64 (cpu, bimm64); return;
	}
    }
  HALT_UNALLOC;
}

/* Immediate move.
   The uimm argument is a 16 bit value to be inserted into the
   target register the pos argument locates the 16 bit word in the
   dest register i.e. it is in {0, 1} for 32 bit and {0, 1, 2,
   3} for 64 bit.
   N.B register arg may not be SP so it should be.
   accessed using the setGZRegisterXXX accessors.  */

/* 32 bit move 16 bit immediate zero remaining shorts.  */
static void
movz32 (sim_cpu *cpu, uint32_t val, uint32_t pos)
{
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, val << (pos * 16));
}

/* 64 bit move 16 bit immediate zero remaining shorts.  */
static void
movz64 (sim_cpu *cpu, uint32_t val, uint32_t pos)
{
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, ((uint64_t) val) << (pos * 16));
}

/* 32 bit move 16 bit immediate negated.  */
static void
movn32 (sim_cpu *cpu, uint32_t val, uint32_t pos)
{
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, ((val << (pos * 16)) ^ 0xffffffffU));
}

/* 64 bit move 16 bit immediate negated.  */
static void
movn64 (sim_cpu *cpu, uint32_t val, uint32_t pos)
{
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, ((((uint64_t) val) << (pos * 16))
		      ^ 0xffffffffffffffffULL));
}

/* 32 bit move 16 bit immediate keep remaining shorts.  */
static void
movk32 (sim_cpu *cpu, uint32_t val, uint32_t pos)
{
  unsigned rd = INSTR (4, 0);
  uint32_t current = aarch64_get_reg_u32 (cpu, rd, NO_SP);
  uint32_t value = val << (pos * 16);
  uint32_t mask = ~(0xffffU << (pos * 16));

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, (value | (current & mask)));
}

/* 64 bit move 16 it immediate keep remaining shorts.  */
static void
movk64 (sim_cpu *cpu, uint32_t val, uint32_t pos)
{
  unsigned rd = INSTR (4, 0);
  uint64_t current = aarch64_get_reg_u64 (cpu, rd, NO_SP);
  uint64_t value = (uint64_t) val << (pos * 16);
  uint64_t mask = ~(0xffffULL << (pos * 16));

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, (value | (current & mask)));
}

static void
dexMoveWideImmediate (sim_cpu *cpu)
{
  /* assert instr[28:23] = 100101
     instr[31] = size : 0 ==> 32 bit, 1 ==> 64 bit
     instr[30,29] = op : 0 ==> MOVN, 1 ==> UNALLOC, 2 ==> MOVZ, 3 ==> MOVK
     instr[22,21] = shift : 00 == LSL#0, 01 = LSL#16, 10 = LSL#32, 11 = LSL#48
     instr[20,5] = uimm16
     instr[4,0] = Rd  */

  /* N.B. the (multiple of 16) shift is applied by the called routine,
     we just pass the multiplier.  */

  uint32_t imm;
  uint32_t size = INSTR (31, 31);
  uint32_t op = INSTR (30, 29);
  uint32_t shift = INSTR (22, 21);

  /* 32 bit can only shift 0 or 1 lot of 16.
     anything else is an unallocated instruction.  */
  if (size == 0 && (shift > 1))
    HALT_UNALLOC;

  if (op == 1)
    HALT_UNALLOC;

  imm = INSTR (20, 5);

  if (size == 0)
    {
      if (op == 0)
	movn32 (cpu, imm, shift);
      else if (op == 2)
	movz32 (cpu, imm, shift);
      else
	movk32 (cpu, imm, shift);
    }
  else
    {
      if (op == 0)
	movn64 (cpu, imm, shift);
      else if (op == 2)
	movz64 (cpu, imm, shift);
      else
	movk64 (cpu, imm, shift);
    }
}

/* Bitfield operations.
   These take a pair of bit positions r and s which are in {0..31}
   or {0..63} depending on the instruction word size.
   N.B register args may not be SP.  */

/* OK, we start with ubfm which just needs to pick
   some bits out of source zero the rest and write
   the result to dest.  Just need two logical shifts.  */

/* 32 bit bitfield move, left and right of affected zeroed
   if r <= s Wd<s-r:0> = Wn<s:r> else Wd<32+s-r,32-r> = Wn<s:0>.  */
static void
ubfm32 (sim_cpu *cpu, uint32_t r, uint32_t s)
{
  unsigned rd;
  unsigned rn = INSTR (9, 5);
  uint32_t value = aarch64_get_reg_u32 (cpu, rn, NO_SP);

  /* Pick either s+1-r or s+1 consecutive bits out of the original word.  */
  if (r <= s)
    {
      /* 31:...:s:xxx:r:...:0 ==> 31:...:s-r:xxx:0.
         We want only bits s:xxx:r at the bottom of the word
         so we LSL bit s up to bit 31 i.e. by 31 - s
         and then we LSR to bring bit 31 down to bit s - r
	 i.e. by 31 + r - s.  */
      value <<= 31 - s;
      value >>= 31 + r - s;
    }
  else
    {
      /* 31:...:s:xxx:0 ==> 31:...:31-(r-1)+s:xxx:31-(r-1):...:0
         We want only bits s:xxx:0 starting at it 31-(r-1)
         so we LSL bit s up to bit 31 i.e. by 31 - s
         and then we LSL to bring bit 31 down to 31-(r-1)+s
	 i.e. by r - (s + 1).  */
      value <<= 31 - s;
      value >>= r - (s + 1);
    }

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  rd = INSTR (4, 0);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value);
}

/* 64 bit bitfield move, left and right of affected zeroed
   if r <= s Wd<s-r:0> = Wn<s:r> else Wd<64+s-r,64-r> = Wn<s:0>.  */
static void
ubfm (sim_cpu *cpu, uint32_t r, uint32_t s)
{
  unsigned rd;
  unsigned rn = INSTR (9, 5);
  uint64_t value = aarch64_get_reg_u64 (cpu, rn, NO_SP);

  if (r <= s)
    {
      /* 63:...:s:xxx:r:...:0 ==> 63:...:s-r:xxx:0.
         We want only bits s:xxx:r at the bottom of the word.
         So we LSL bit s up to bit 63 i.e. by 63 - s
         and then we LSR to bring bit 63 down to bit s - r
	 i.e. by 63 + r - s.  */
      value <<= 63 - s;
      value >>= 63 + r - s;
    }
  else
    {
      /* 63:...:s:xxx:0 ==> 63:...:63-(r-1)+s:xxx:63-(r-1):...:0.
         We want only bits s:xxx:0 starting at it 63-(r-1).
         So we LSL bit s up to bit 63 i.e. by 63 - s
         and then we LSL to bring bit 63 down to 63-(r-1)+s
	 i.e. by r - (s + 1).  */
      value <<= 63 - s;
      value >>= r - (s + 1);
    }

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  rd = INSTR (4, 0);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value);
}

/* The signed versions need to insert sign bits
   on the left of the inserted bit field. so we do
   much the same as the unsigned version except we
   use an arithmetic shift right -- this just means
   we need to operate on signed values.  */

/* 32 bit bitfield move, left of affected sign-extended, right zeroed.  */
/* If r <= s Wd<s-r:0> = Wn<s:r> else Wd<32+s-r,32-r> = Wn<s:0>.  */
static void
sbfm32 (sim_cpu *cpu, uint32_t r, uint32_t s)
{
  unsigned rd;
  unsigned rn = INSTR (9, 5);
  /* as per ubfm32 but use an ASR instead of an LSR.  */
  int32_t value = aarch64_get_reg_s32 (cpu, rn, NO_SP);

  if (r <= s)
    {
      value <<= 31 - s;
      value >>= 31 + r - s;
    }
  else
    {
      value <<= 31 - s;
      value >>= r - (s + 1);
    }

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  rd = INSTR (4, 0);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, (uint32_t) value);
}

/* 64 bit bitfield move, left of affected sign-extended, right zeroed.  */
/* If r <= s Wd<s-r:0> = Wn<s:r> else Wd<64+s-r,64-r> = Wn<s:0>.  */
static void
sbfm (sim_cpu *cpu, uint32_t r, uint32_t s)
{
  unsigned rd;
  unsigned rn = INSTR (9, 5);
  /* acpu per ubfm but use an ASR instead of an LSR.  */
  int64_t value = aarch64_get_reg_s64 (cpu, rn, NO_SP);

  if (r <= s)
    {
      value <<= 63 - s;
      value >>= 63 + r - s;
    }
  else
    {
      value <<= 63 - s;
      value >>= r - (s + 1);
    }

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  rd = INSTR (4, 0);
  aarch64_set_reg_s64 (cpu, rd, NO_SP, value);
}

/* Finally, these versions leave non-affected bits
   as is. so we need to generate the bits as per
   ubfm and also generate a mask to pick the
   bits from the original and computed values.  */

/* 32 bit bitfield move, non-affected bits left as is.
   If r <= s Wd<s-r:0> = Wn<s:r> else Wd<32+s-r,32-r> = Wn<s:0>.  */
static void
bfm32 (sim_cpu *cpu, uint32_t r, uint32_t s)
{
  unsigned rn = INSTR (9, 5);
  uint32_t value = aarch64_get_reg_u32 (cpu, rn, NO_SP);
  uint32_t mask = -1;
  unsigned rd;
  uint32_t value2;

  /* Pick either s+1-r or s+1 consecutive bits out of the original word.  */
  if (r <= s)
    {
      /* 31:...:s:xxx:r:...:0 ==> 31:...:s-r:xxx:0.
         We want only bits s:xxx:r at the bottom of the word
         so we LSL bit s up to bit 31 i.e. by 31 - s
         and then we LSR to bring bit 31 down to bit s - r
	 i.e. by 31 + r - s.  */
      value <<= 31 - s;
      value >>= 31 + r - s;
      /* the mask must include the same bits.  */
      mask <<= 31 - s;
      mask >>= 31 + r - s;
    }
  else
    {
      /* 31:...:s:xxx:0 ==> 31:...:31-(r-1)+s:xxx:31-(r-1):...:0.
         We want only bits s:xxx:0 starting at it 31-(r-1)
         so we LSL bit s up to bit 31 i.e. by 31 - s
         and then we LSL to bring bit 31 down to 31-(r-1)+s
	 i.e. by r - (s + 1).  */
      value <<= 31 - s;
      value >>= r - (s + 1);
      /* The mask must include the same bits.  */
      mask <<= 31 - s;
      mask >>= r - (s + 1);
    }

  rd = INSTR (4, 0);
  value2 = aarch64_get_reg_u32 (cpu, rd, NO_SP);

  value2 &= ~mask;
  value2 |= value;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, value2);
}

/* 64 bit bitfield move, non-affected bits left as is.
   If r <= s Wd<s-r:0> = Wn<s:r> else Wd<64+s-r,64-r> = Wn<s:0>.  */
static void
bfm (sim_cpu *cpu, uint32_t r, uint32_t s)
{
  unsigned rd;
  unsigned rn = INSTR (9, 5);
  uint64_t value = aarch64_get_reg_u64 (cpu, rn, NO_SP);
  uint64_t mask = 0xffffffffffffffffULL;

  if (r <= s)
    {
      /* 63:...:s:xxx:r:...:0 ==> 63:...:s-r:xxx:0.
         We want only bits s:xxx:r at the bottom of the word
         so we LSL bit s up to bit 63 i.e. by 63 - s
         and then we LSR to bring bit 63 down to bit s - r
	 i.e. by 63 + r - s.  */
      value <<= 63 - s;
      value >>= 63 + r - s;
      /* The mask must include the same bits.  */
      mask <<= 63 - s;
      mask >>= 63 + r - s;
    }
  else
    {
      /* 63:...:s:xxx:0 ==> 63:...:63-(r-1)+s:xxx:63-(r-1):...:0
         We want only bits s:xxx:0 starting at it 63-(r-1)
         so we LSL bit s up to bit 63 i.e. by 63 - s
         and then we LSL to bring bit 63 down to 63-(r-1)+s
	 i.e. by r - (s + 1).  */
      value <<= 63 - s;
      value >>= r - (s + 1);
      /* The mask must include the same bits.  */
      mask <<= 63 - s;
      mask >>= r - (s + 1);
    }

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  rd = INSTR (4, 0);
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, (aarch64_get_reg_u64 (cpu, rd, NO_SP) & ~mask) | value);
}

static void
dexBitfieldImmediate (sim_cpu *cpu)
{
  /* assert instr[28:23] = 100110
     instr[31] = size : 0 ==> 32 bit, 1 ==> 64 bit
     instr[30,29] = op : 0 ==> SBFM, 1 ==> BFM, 2 ==> UBFM, 3 ==> UNALLOC
     instr[22] = N : must be 0 for 32 bit, 1 for 64 bit ow UNALLOC
     instr[21,16] = immr : 0xxxxx for 32 bit, xxxxxx for 64 bit
     instr[15,10] = imms :  0xxxxx for 32 bit, xxxxxx for 64 bit
     instr[9,5] = Rn
     instr[4,0] = Rd  */

  /* 32 bit operations must have N = 0 or else we have an UNALLOC.  */
  uint32_t dispatch;
  uint32_t imms;
  uint32_t size = INSTR (31, 31);
  uint32_t N = INSTR (22, 22);
  /* 32 bit operations must have immr[5] = 0 and imms[5] = 0.  */
  /* or else we have an UNALLOC.  */
  uint32_t immr = INSTR (21, 16);

  if (~size & N)
    HALT_UNALLOC;

  if (!size && uimm (immr, 5, 5))
    HALT_UNALLOC;

  imms = INSTR (15, 10);
  if (!size && uimm (imms, 5, 5))
    HALT_UNALLOC;

  /* Switch on combined size and op.  */
  dispatch = INSTR (31, 29);
  switch (dispatch)
    {
    case 0: sbfm32 (cpu, immr, imms); return;
    case 1: bfm32 (cpu, immr, imms); return;
    case 2: ubfm32 (cpu, immr, imms); return;
    case 4: sbfm (cpu, immr, imms); return;
    case 5: bfm (cpu, immr, imms); return;
    case 6: ubfm (cpu, immr, imms); return;
    default: HALT_UNALLOC;
    }
}

static void
do_EXTR_32 (sim_cpu *cpu)
{
  /* instr[31:21] = 00010011100
     instr[20,16] = Rm
     instr[15,10] = imms :  0xxxxx for 32 bit
     instr[9,5]   = Rn
     instr[4,0]   = Rd  */
  unsigned rm   = INSTR (20, 16);
  unsigned imms = INSTR (15, 10) & 31;
  unsigned rn   = INSTR ( 9,  5);
  unsigned rd   = INSTR ( 4,  0);
  uint64_t val1;
  uint64_t val2;

  val1 = aarch64_get_reg_u32 (cpu, rm, NO_SP);
  val1 >>= imms;
  val2 = aarch64_get_reg_u32 (cpu, rn, NO_SP);
  val2 <<= (32 - imms);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP, val1 | val2);
}

static void
do_EXTR_64 (sim_cpu *cpu)
{
  /* instr[31:21] = 10010011100
     instr[20,16] = Rm
     instr[15,10] = imms
     instr[9,5]   = Rn
     instr[4,0]   = Rd  */
  unsigned rm   = INSTR (20, 16);
  unsigned imms = INSTR (15, 10) & 63;
  unsigned rn   = INSTR ( 9,  5);
  unsigned rd   = INSTR ( 4,  0);
  uint64_t val;

  val = aarch64_get_reg_u64 (cpu, rm, NO_SP);
  val >>= imms;
  val |= (aarch64_get_reg_u64 (cpu, rn, NO_SP) << (64 - imms));

  aarch64_set_reg_u64 (cpu, rd, NO_SP, val);
}

static void
dexExtractImmediate (sim_cpu *cpu)
{
  /* assert instr[28:23] = 100111
     instr[31]    = size : 0 ==> 32 bit, 1 ==> 64 bit
     instr[30,29] = op21 : 0 ==> EXTR, 1,2,3 ==> UNALLOC
     instr[22]    = N : must be 0 for 32 bit, 1 for 64 bit or UNALLOC
     instr[21]    = op0 : must be 0 or UNALLOC
     instr[20,16] = Rm
     instr[15,10] = imms :  0xxxxx for 32 bit, xxxxxx for 64 bit
     instr[9,5]   = Rn
     instr[4,0]   = Rd  */

  /* 32 bit operations must have N = 0 or else we have an UNALLOC.  */
  /* 64 bit operations must have N = 1 or else we have an UNALLOC.  */
  uint32_t dispatch;
  uint32_t size = INSTR (31, 31);
  uint32_t N = INSTR (22, 22);
  /* 32 bit operations must have imms[5] = 0
     or else we have an UNALLOC.  */
  uint32_t imms = INSTR (15, 10);

  if (size ^ N)
    HALT_UNALLOC;

  if (!size && uimm (imms, 5, 5))
    HALT_UNALLOC;

  /* Switch on combined size and op.  */
  dispatch = INSTR (31, 29);

  if (dispatch == 0)
    do_EXTR_32 (cpu);

  else if (dispatch == 4)
    do_EXTR_64 (cpu);

  else if (dispatch == 1)
    HALT_NYI;
  else
    HALT_UNALLOC;
}

static void
dexDPImm (sim_cpu *cpu)
{
  /* uint32_t group = dispatchGroup (aarch64_get_instr (cpu));
     assert  group == GROUP_DPIMM_1000 || grpoup == GROUP_DPIMM_1001
     bits [25,23] of a DPImm are the secondary dispatch vector.  */
  uint32_t group2 = dispatchDPImm (aarch64_get_instr (cpu));

  switch (group2)
    {
    case DPIMM_PCADR_000:
    case DPIMM_PCADR_001:
      dexPCRelAddressing (cpu);
      return;

    case DPIMM_ADDSUB_010:
    case DPIMM_ADDSUB_011:
      dexAddSubtractImmediate (cpu);
      return;

    case DPIMM_LOG_100:
      dexLogicalImmediate (cpu);
      return;

    case DPIMM_MOV_101:
      dexMoveWideImmediate (cpu);
      return;

    case DPIMM_BITF_110:
      dexBitfieldImmediate (cpu);
      return;

    case DPIMM_EXTR_111:
      dexExtractImmediate (cpu);
      return;

    default:
      /* Should never reach here.  */
      HALT_NYI;
    }
}

static void
dexLoadUnscaledImmediate (sim_cpu *cpu)
{
  /* instr[29,24] == 111_00
     instr[21] == 0
     instr[11,10] == 00
     instr[31,30] = size
     instr[26] = V
     instr[23,22] = opc
     instr[20,12] = simm9
     instr[9,5] = rn may be SP.  */
  /* unsigned rt = INSTR (4, 0);  */
  uint32_t V = INSTR (26, 26);
  uint32_t dispatch = ((INSTR (31, 30) << 2) | INSTR (23, 22));
  int32_t imm = simm32 (aarch64_get_instr (cpu), 20, 12);

  if (!V)
    {
      /* GReg operations.  */
      switch (dispatch)
	{
	case 0:	 sturb (cpu, imm); return;
	case 1:	 ldurb32 (cpu, imm); return;
	case 2:	 ldursb64 (cpu, imm); return;
	case 3:	 ldursb32 (cpu, imm); return;
	case 4:	 sturh (cpu, imm); return;
	case 5:	 ldurh32 (cpu, imm); return;
	case 6:	 ldursh64 (cpu, imm); return;
	case 7:	 ldursh32 (cpu, imm); return;
	case 8:	 stur32 (cpu, imm); return;
	case 9:	 ldur32 (cpu, imm); return;
	case 10: ldursw (cpu, imm); return;
	case 12: stur64 (cpu, imm); return;
	case 13: ldur64 (cpu, imm); return;

	case 14:
	  /* PRFUM NYI.  */
	  HALT_NYI;

	default:
	case 11:
	case 15:
	  HALT_UNALLOC;
	}
    }

  /* FReg operations.  */
  switch (dispatch)
    {
    case 2:  fsturq (cpu, imm); return;
    case 3:  fldurq (cpu, imm); return;
    case 8:  fsturs (cpu, imm); return;
    case 9:  fldurs (cpu, imm); return;
    case 12: fsturd (cpu, imm); return;
    case 13: fldurd (cpu, imm); return;

    case 0: /* STUR 8 bit FP.  */
    case 1: /* LDUR 8 bit FP.  */
    case 4: /* STUR 16 bit FP.  */
    case 5: /* LDUR 8 bit FP.  */
      HALT_NYI;

    default:
    case 6:
    case 7:
    case 10:
    case 11:
    case 14:
    case 15:
      HALT_UNALLOC;
    }
}

/*  N.B. A preliminary note regarding all the ldrs<x>32
    instructions

   The signed value loaded by these instructions is cast to unsigned
   before being assigned to aarch64_get_reg_u64 (cpu, N) i.e. to the
   64 bit element of the GReg union. this performs a 32 bit sign extension
   (as required) but avoids 64 bit sign extension, thus ensuring that the
   top half of the register word is zero. this is what the spec demands
   when a 32 bit load occurs.  */

/* 32 bit load sign-extended byte scaled unsigned 12 bit.  */
static void
ldrsb32_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned int rn = INSTR (9, 5);
  unsigned int rt = INSTR (4, 0);

  /* The target register may not be SP but the source may be
     there is no scaling required for a byte load.  */
  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK) + offset;
  aarch64_set_reg_u64 (cpu, rt, NO_SP,
		       (int64_t) aarch64_get_mem_s8 (cpu, address));
}

/* 32 bit load sign-extended byte scaled or unscaled zero-
   or sign-extended 32-bit register offset.  */
static void
ldrsb32_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned int rm = INSTR (20, 16);
  unsigned int rn = INSTR (9, 5);
  unsigned int rt = INSTR (4, 0);

  /* rn may reference SP, rm and rt must reference ZR.  */

  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t displacement = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP),
				 extension);

  /* There is no scaling required for a byte load.  */
  aarch64_set_reg_u64
    (cpu, rt, NO_SP, (int64_t) aarch64_get_mem_s8 (cpu, address
						   + displacement));
}

/* 32 bit load sign-extended byte unscaled signed 9 bit with
   pre- or post-writeback.  */
static void
ldrsb32_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  uint64_t address;
  unsigned int rn = INSTR (9, 5);
  unsigned int rt = INSTR (4, 0);

  if (rn == rt && wb != NoWriteBack)
    HALT_UNALLOC;

  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb == Pre)
      address += offset;

  aarch64_set_reg_u64 (cpu, rt, NO_SP,
		       (int64_t) aarch64_get_mem_s8 (cpu, address));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, NO_SP, address);
}

/* 8 bit store scaled.  */
static void
fstrb_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned st = INSTR (4, 0);
  unsigned rn = INSTR (9, 5);

  aarch64_set_mem_u8 (cpu,
		      aarch64_get_reg_u64 (cpu, rn, SP_OK) + offset,
		      aarch64_get_vec_u8 (cpu, st, 0));
}

/* 8 bit store scaled or unscaled zero- or
   sign-extended 8-bit register offset.  */
static void
fstrb_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned st = INSTR (4, 0);

  uint64_t  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t   extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP),
			       extension);
  uint64_t  displacement = scaling == Scaled ? extended : 0;

  aarch64_set_mem_u8
    (cpu, address + displacement, aarch64_get_vec_u8 (cpu, st, 0));
}

/* 16 bit store scaled.  */
static void
fstrh_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned st = INSTR (4, 0);
  unsigned rn = INSTR (9, 5);

  aarch64_set_mem_u16
    (cpu,
     aarch64_get_reg_u64 (cpu, rn, SP_OK) + SCALE (offset, 16),
     aarch64_get_vec_u16 (cpu, st, 0));
}

/* 16 bit store scaled or unscaled zero-
   or sign-extended 16-bit register offset.  */
static void
fstrh_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned st = INSTR (4, 0);

  uint64_t  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t   extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP),
			       extension);
  uint64_t  displacement = OPT_SCALE (extended, 16, scaling);

  aarch64_set_mem_u16
    (cpu, address + displacement, aarch64_get_vec_u16 (cpu, st, 0));
}

/* 32 bit store scaled unsigned 12 bit.  */
static void
fstrs_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned st = INSTR (4, 0);
  unsigned rn = INSTR (9, 5);

  aarch64_set_mem_u32
    (cpu,
     aarch64_get_reg_u64 (cpu, rn, SP_OK) + SCALE (offset, 32),
     aarch64_get_vec_u32 (cpu, st, 0));
}

/* 32 bit store unscaled signed 9 bit with pre- or post-writeback.  */
static void
fstrs_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned st = INSTR (4, 0);

  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  aarch64_set_mem_u32 (cpu, address, aarch64_get_vec_u32 (cpu, st, 0));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* 32 bit store scaled or unscaled zero-
   or sign-extended 32-bit register offset.  */
static void
fstrs_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned st = INSTR (4, 0);

  uint64_t  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t   extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP),
			       extension);
  uint64_t  displacement = OPT_SCALE (extended, 32, scaling);

  aarch64_set_mem_u32
    (cpu, address + displacement, aarch64_get_vec_u32 (cpu, st, 0));
}

/* 64 bit store scaled unsigned 12 bit.  */
static void
fstrd_abs (sim_cpu *cpu, uint32_t offset)
{
  unsigned st = INSTR (4, 0);
  unsigned rn = INSTR (9, 5);

  aarch64_set_mem_u64
    (cpu,
     aarch64_get_reg_u64 (cpu, rn, SP_OK) + SCALE (offset, 64),
     aarch64_get_vec_u64 (cpu, st, 0));
}

/* 64 bit store unscaled signed 9 bit with pre- or post-writeback.  */
static void
fstrd_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (9, 5);
  unsigned st = INSTR (4, 0);

  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  aarch64_set_mem_u64 (cpu, address, aarch64_get_vec_u64 (cpu, st, 0));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* 64 bit store scaled or unscaled zero-
   or sign-extended 32-bit register offset.  */
static void
fstrd_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned st = INSTR (4, 0);

  uint64_t  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t   extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP),
			       extension);
  uint64_t  displacement = OPT_SCALE (extended, 64, scaling);

  aarch64_set_mem_u64
    (cpu, address + displacement, aarch64_get_vec_u64 (cpu, st, 0));
}

/* 128 bit store scaled unsigned 12 bit.  */
static void
fstrq_abs (sim_cpu *cpu, uint32_t offset)
{
  FRegister a;
  unsigned st = INSTR (4, 0);
  unsigned rn = INSTR (9, 5);
  uint64_t addr;

  aarch64_get_FP_long_double (cpu, st, & a);

  addr = aarch64_get_reg_u64 (cpu, rn, SP_OK) + SCALE (offset, 128);
  aarch64_set_mem_long_double (cpu, addr, a);
}

/* 128 bit store unscaled signed 9 bit with pre- or post-writeback.  */
static void
fstrq_wb (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  FRegister a;
  unsigned rn = INSTR (9, 5);
  unsigned st = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rn, SP_OK);

  if (wb != Post)
    address += offset;

  aarch64_get_FP_long_double (cpu, st, & a);
  aarch64_set_mem_long_double (cpu, address, a);

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rn, SP_OK, address);
}

/* 128 bit store scaled or unscaled zero-
   or sign-extended 32-bit register offset.  */
static void
fstrq_scale_ext (sim_cpu *cpu, Scaling scaling, Extension extension)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned st = INSTR (4, 0);

  uint64_t  address = aarch64_get_reg_u64 (cpu, rn, SP_OK);
  int64_t   extended = extend (aarch64_get_reg_u32 (cpu, rm, NO_SP),
			       extension);
  uint64_t  displacement = OPT_SCALE (extended, 128, scaling);

  FRegister a;

  aarch64_get_FP_long_double (cpu, st, & a);
  aarch64_set_mem_long_double (cpu, address + displacement, a);
}

static void
dexLoadImmediatePrePost (sim_cpu *cpu)
{
  /* instr[31,30] = size
     instr[29,27] = 111
     instr[26]    = V
     instr[25,24] = 00
     instr[23,22] = opc
     instr[21]    = 0
     instr[20,12] = simm9
     instr[11]    = wb : 0 ==> Post, 1 ==> Pre
     instr[10]    = 0
     instr[9,5]   = Rn may be SP.
     instr[4,0]   = Rt */

  uint32_t  V        = INSTR (26, 26);
  uint32_t  dispatch = ((INSTR (31, 30) << 2) | INSTR (23, 22));
  int32_t   imm      = simm32 (aarch64_get_instr (cpu), 20, 12);
  WriteBack wb       = INSTR (11, 11);

  if (!V)
    {
      /* GReg operations.  */
      switch (dispatch)
	{
	case 0:	 strb_wb (cpu, imm, wb); return;
	case 1:	 ldrb32_wb (cpu, imm, wb); return;
	case 2:	 ldrsb_wb (cpu, imm, wb); return;
	case 3:	 ldrsb32_wb (cpu, imm, wb); return;
	case 4:	 strh_wb (cpu, imm, wb); return;
	case 5:	 ldrh32_wb (cpu, imm, wb); return;
	case 6:	 ldrsh64_wb (cpu, imm, wb); return;
	case 7:	 ldrsh32_wb (cpu, imm, wb); return;
	case 8:	 str32_wb (cpu, imm, wb); return;
	case 9:	 ldr32_wb (cpu, imm, wb); return;
	case 10: ldrsw_wb (cpu, imm, wb); return;
	case 12: str_wb (cpu, imm, wb); return;
	case 13: ldr_wb (cpu, imm, wb); return;

	default:
	case 11:
	case 14:
	case 15:
	  HALT_UNALLOC;
	}
    }

  /* FReg operations.  */
  switch (dispatch)
    {
    case 2:  fstrq_wb (cpu, imm, wb); return;
    case 3:  fldrq_wb (cpu, imm, wb); return;
    case 8:  fstrs_wb (cpu, imm, wb); return;
    case 9:  fldrs_wb (cpu, imm, wb); return;
    case 12: fstrd_wb (cpu, imm, wb); return;
    case 13: fldrd_wb (cpu, imm, wb); return;

    case 0:	  /* STUR 8 bit FP.  */
    case 1:	  /* LDUR 8 bit FP.  */
    case 4:	  /* STUR 16 bit FP.  */
    case 5:	  /* LDUR 8 bit FP.  */
      HALT_NYI;

    default:
    case 6:
    case 7:
    case 10:
    case 11:
    case 14:
    case 15:
      HALT_UNALLOC;
    }
}

static void
dexLoadRegisterOffset (sim_cpu *cpu)
{
  /* instr[31,30] = size
     instr[29,27] = 111
     instr[26]    = V
     instr[25,24] = 00
     instr[23,22] = opc
     instr[21]    = 1
     instr[20,16] = rm
     instr[15,13] = option : 010 ==> UXTW, 011 ==> UXTX/LSL,
                             110 ==> SXTW, 111 ==> SXTX,
                             ow ==> RESERVED
     instr[12]    = scaled
     instr[11,10] = 10
     instr[9,5]   = rn
     instr[4,0]   = rt.  */

  uint32_t  V = INSTR (26, 26);
  uint32_t  dispatch = ((INSTR (31, 30) << 2) | INSTR (23, 22));
  Scaling   scale = INSTR (12, 12);
  Extension extensionType = INSTR (15, 13);

  /* Check for illegal extension types.  */
  if (uimm (extensionType, 1, 1) == 0)
    HALT_UNALLOC;

  if (extensionType == UXTX || extensionType == SXTX)
    extensionType = NoExtension;

  if (!V)
    {
      /* GReg operations.  */
      switch (dispatch)
	{
	case 0:	 strb_scale_ext (cpu, scale, extensionType); return;
	case 1:	 ldrb32_scale_ext (cpu, scale, extensionType); return;
	case 2:	 ldrsb_scale_ext (cpu, scale, extensionType); return;
	case 3:	 ldrsb32_scale_ext (cpu, scale, extensionType); return;
	case 4:	 strh_scale_ext (cpu, scale, extensionType); return;
	case 5:	 ldrh32_scale_ext (cpu, scale, extensionType); return;
	case 6:	 ldrsh_scale_ext (cpu, scale, extensionType); return;
	case 7:	 ldrsh32_scale_ext (cpu, scale, extensionType); return;
	case 8:	 str32_scale_ext (cpu, scale, extensionType); return;
	case 9:	 ldr32_scale_ext (cpu, scale, extensionType); return;
	case 10: ldrsw_scale_ext (cpu, scale, extensionType); return;
	case 12: str_scale_ext (cpu, scale, extensionType); return;
	case 13: ldr_scale_ext (cpu, scale, extensionType); return;
	case 14: prfm_scale_ext (cpu, scale, extensionType); return;

	default:
	case 11:
	case 15:
	  HALT_UNALLOC;
	}
    }

  /* FReg operations.  */
  switch (dispatch)
    {
    case 1: /* LDUR 8 bit FP.  */
      HALT_NYI;
    case 3:  fldrq_scale_ext (cpu, scale, extensionType); return;
    case 5: /* LDUR 8 bit FP.  */
      HALT_NYI;
    case 9:  fldrs_scale_ext (cpu, scale, extensionType); return;
    case 13: fldrd_scale_ext (cpu, scale, extensionType); return;

    case 0:  fstrb_scale_ext (cpu, scale, extensionType); return;
    case 2:  fstrq_scale_ext (cpu, scale, extensionType); return;
    case 4:  fstrh_scale_ext (cpu, scale, extensionType); return;
    case 8:  fstrs_scale_ext (cpu, scale, extensionType); return;
    case 12: fstrd_scale_ext (cpu, scale, extensionType); return;

    default:
    case 6:
    case 7:
    case 10:
    case 11:
    case 14:
    case 15:
      HALT_UNALLOC;
    }
}

static void
dexLoadUnsignedImmediate (sim_cpu *cpu)
{
  /* instr[29,24] == 111_01
     instr[31,30] = size
     instr[26]    = V
     instr[23,22] = opc
     instr[21,10] = uimm12 : unsigned immediate offset
     instr[9,5]   = rn may be SP.
     instr[4,0]   = rt.  */

  uint32_t V = INSTR (26,26);
  uint32_t dispatch = ((INSTR (31, 30) << 2) | INSTR (23, 22));
  uint32_t imm = INSTR (21, 10);

  if (!V)
    {
      /* GReg operations.  */
      switch (dispatch)
	{
	case 0:  strb_abs (cpu, imm); return;
	case 1:  ldrb32_abs (cpu, imm); return;
	case 2:  ldrsb_abs (cpu, imm); return;
	case 3:  ldrsb32_abs (cpu, imm); return;
	case 4:  strh_abs (cpu, imm); return;
	case 5:  ldrh32_abs (cpu, imm); return;
	case 6:  ldrsh_abs (cpu, imm); return;
	case 7:  ldrsh32_abs (cpu, imm); return;
	case 8:  str32_abs (cpu, imm); return;
	case 9:  ldr32_abs (cpu, imm); return;
	case 10: ldrsw_abs (cpu, imm); return;
	case 12: str_abs (cpu, imm); return;
	case 13: ldr_abs (cpu, imm); return;
	case 14: prfm_abs (cpu, imm); return;

	default:
	case 11:
	case 15:
	  HALT_UNALLOC;
	}
    }

  /* FReg operations.  */
  switch (dispatch)
    {
    case 0:  fstrb_abs (cpu, imm); return;
    case 4:  fstrh_abs (cpu, imm); return;
    case 8:  fstrs_abs (cpu, imm); return;
    case 12: fstrd_abs (cpu, imm); return;
    case 2:  fstrq_abs (cpu, imm); return;

    case 1:  fldrb_abs (cpu, imm); return;
    case 5:  fldrh_abs (cpu, imm); return;
    case 9:  fldrs_abs (cpu, imm); return;
    case 13: fldrd_abs (cpu, imm); return;
    case 3:  fldrq_abs (cpu, imm); return;

    default:
    case 6:
    case 7:
    case 10:
    case 11:
    case 14:
    case 15:
      HALT_UNALLOC;
    }
}

static void
dexLoadExclusive (sim_cpu *cpu)
{
  /* assert instr[29:24] = 001000;
     instr[31,30] = size
     instr[23] = 0 if exclusive
     instr[22] = L : 1 if load, 0 if store
     instr[21] = 1 if pair
     instr[20,16] = Rs
     instr[15] = o0 : 1 if ordered
     instr[14,10] = Rt2
     instr[9,5] = Rn
     instr[4.0] = Rt.  */

  switch (INSTR (22, 21))
    {
    case 2:   ldxr (cpu); return;
    case 0:   stxr (cpu); return;
    default:  HALT_NYI;
    }
}

static void
dexLoadOther (sim_cpu *cpu)
{
  uint32_t dispatch;

  /* instr[29,25] = 111_0
     instr[24] == 0 ==> dispatch, 1 ==> ldst reg unsigned immediate
     instr[21:11,10] is the secondary dispatch.  */
  if (INSTR (24, 24))
    {
      dexLoadUnsignedImmediate (cpu);
      return;
    }

  dispatch = ((INSTR (21, 21) << 2) | INSTR (11, 10));
  switch (dispatch)
    {
    case 0: dexLoadUnscaledImmediate (cpu); return;
    case 1: dexLoadImmediatePrePost (cpu); return;
    case 3: dexLoadImmediatePrePost (cpu); return;
    case 6: dexLoadRegisterOffset (cpu); return;

    default:
    case 2:
    case 4:
    case 5:
    case 7:
      HALT_NYI;
    }
}

static void
store_pair_u32 (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (14, 10);
  unsigned rd = INSTR (9, 5);
  unsigned rm = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rd, SP_OK);

  if ((rn == rd || rm == rd) && wb != NoWriteBack)
    HALT_UNALLOC; /* ??? */

  offset <<= 2;

  if (wb != Post)
    address += offset;

  aarch64_set_mem_u32 (cpu, address,
		       aarch64_get_reg_u32 (cpu, rm, NO_SP));
  aarch64_set_mem_u32 (cpu, address + 4,
		       aarch64_get_reg_u32 (cpu, rn, NO_SP));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rd, SP_OK, address);
}

static void
store_pair_u64 (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (14, 10);
  unsigned rd = INSTR (9, 5);
  unsigned rm = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rd, SP_OK);

  if ((rn == rd || rm == rd) && wb != NoWriteBack)
    HALT_UNALLOC; /* ??? */

  offset <<= 3;

  if (wb != Post)
    address += offset;

  aarch64_set_mem_u64 (cpu, address,
		       aarch64_get_reg_u64 (cpu, rm, NO_SP));
  aarch64_set_mem_u64 (cpu, address + 8,
		       aarch64_get_reg_u64 (cpu, rn, NO_SP));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rd, SP_OK, address);
}

static void
load_pair_u32 (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (14, 10);
  unsigned rd = INSTR (9, 5);
  unsigned rm = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rd, SP_OK);

  /* Treat this as unalloc to make sure we don't do it.  */
  if (rn == rm)
    HALT_UNALLOC;

  offset <<= 2;

  if (wb != Post)
    address += offset;

  aarch64_set_reg_u64 (cpu, rm, SP_OK, aarch64_get_mem_u32 (cpu, address));
  aarch64_set_reg_u64 (cpu, rn, SP_OK, aarch64_get_mem_u32 (cpu, address + 4));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rd, SP_OK, address);
}

static void
load_pair_s32 (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (14, 10);
  unsigned rd = INSTR (9, 5);
  unsigned rm = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rd, SP_OK);

  /* Treat this as unalloc to make sure we don't do it.  */
  if (rn == rm)
    HALT_UNALLOC;

  offset <<= 2;

  if (wb != Post)
    address += offset;

  aarch64_set_reg_s64 (cpu, rm, SP_OK, aarch64_get_mem_s32 (cpu, address));
  aarch64_set_reg_s64 (cpu, rn, SP_OK, aarch64_get_mem_s32 (cpu, address + 4));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rd, SP_OK, address);
}

static void
load_pair_u64 (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (14, 10);
  unsigned rd = INSTR (9, 5);
  unsigned rm = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rd, SP_OK);

  /* Treat this as unalloc to make sure we don't do it.  */
  if (rn == rm)
    HALT_UNALLOC;

  offset <<= 3;

  if (wb != Post)
    address += offset;

  aarch64_set_reg_u64 (cpu, rm, SP_OK, aarch64_get_mem_u64 (cpu, address));
  aarch64_set_reg_u64 (cpu, rn, SP_OK, aarch64_get_mem_u64 (cpu, address + 8));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rd, SP_OK, address);
}

static void
dex_load_store_pair_gr (sim_cpu *cpu)
{
  /* instr[31,30] = size (10=> 64-bit, 01=> signed 32-bit, 00=> 32-bit)
     instr[29,25] = instruction encoding: 101_0
     instr[26]    = V : 1 if fp 0 if gp
     instr[24,23] = addressing mode (10=> offset, 01=> post, 11=> pre)
     instr[22]    = load/store (1=> load)
     instr[21,15] = signed, scaled, offset
     instr[14,10] = Rn
     instr[ 9, 5] = Rd
     instr[ 4, 0] = Rm.  */

  uint32_t dispatch = ((INSTR (31, 30) << 3) | INSTR (24, 22));
  int32_t offset = simm32 (aarch64_get_instr (cpu), 21, 15);

  switch (dispatch)
    {
    case 2: store_pair_u32 (cpu, offset, Post); return;
    case 3: load_pair_u32  (cpu, offset, Post); return;
    case 4: store_pair_u32 (cpu, offset, NoWriteBack); return;
    case 5: load_pair_u32  (cpu, offset, NoWriteBack); return;
    case 6: store_pair_u32 (cpu, offset, Pre); return;
    case 7: load_pair_u32  (cpu, offset, Pre); return;

    case 11: load_pair_s32  (cpu, offset, Post); return;
    case 13: load_pair_s32  (cpu, offset, NoWriteBack); return;
    case 15: load_pair_s32  (cpu, offset, Pre); return;

    case 18: store_pair_u64 (cpu, offset, Post); return;
    case 19: load_pair_u64  (cpu, offset, Post); return;
    case 20: store_pair_u64 (cpu, offset, NoWriteBack); return;
    case 21: load_pair_u64  (cpu, offset, NoWriteBack); return;
    case 22: store_pair_u64 (cpu, offset, Pre); return;
    case 23: load_pair_u64  (cpu, offset, Pre); return;

    default:
      HALT_UNALLOC;
    }
}

static void
store_pair_float (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (14, 10);
  unsigned rd = INSTR (9, 5);
  unsigned rm = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rd, SP_OK);

  offset <<= 2;

  if (wb != Post)
    address += offset;

  aarch64_set_mem_u32 (cpu, address,     aarch64_get_vec_u32 (cpu, rm, 0));
  aarch64_set_mem_u32 (cpu, address + 4, aarch64_get_vec_u32 (cpu, rn, 0));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rd, SP_OK, address);
}

static void
store_pair_double (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (14, 10);
  unsigned rd = INSTR (9, 5);
  unsigned rm = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rd, SP_OK);

  offset <<= 3;

  if (wb != Post)
    address += offset;

  aarch64_set_mem_u64 (cpu, address,     aarch64_get_vec_u64 (cpu, rm, 0));
  aarch64_set_mem_u64 (cpu, address + 8, aarch64_get_vec_u64 (cpu, rn, 0));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rd, SP_OK, address);
}

static void
store_pair_long_double (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  FRegister a;
  unsigned rn = INSTR (14, 10);
  unsigned rd = INSTR (9, 5);
  unsigned rm = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rd, SP_OK);

  offset <<= 4;

  if (wb != Post)
    address += offset;

  aarch64_get_FP_long_double (cpu, rm, & a);
  aarch64_set_mem_long_double (cpu, address, a);
  aarch64_get_FP_long_double (cpu, rn, & a);
  aarch64_set_mem_long_double (cpu, address + 16, a);

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rd, SP_OK, address);
}

static void
load_pair_float (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (14, 10);
  unsigned rd = INSTR (9, 5);
  unsigned rm = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rd, SP_OK);

  if (rm == rn)
    HALT_UNALLOC;

  offset <<= 2;

  if (wb != Post)
    address += offset;

  aarch64_set_vec_u32 (cpu, rm, 0, aarch64_get_mem_u32 (cpu, address));
  aarch64_set_vec_u32 (cpu, rn, 0, aarch64_get_mem_u32 (cpu, address + 4));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rd, SP_OK, address);
}

static void
load_pair_double (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  unsigned rn = INSTR (14, 10);
  unsigned rd = INSTR (9, 5);
  unsigned rm = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rd, SP_OK);

  if (rm == rn)
    HALT_UNALLOC;

  offset <<= 3;

  if (wb != Post)
    address += offset;

  aarch64_set_vec_u64 (cpu, rm, 0, aarch64_get_mem_u64 (cpu, address));
  aarch64_set_vec_u64 (cpu, rn, 0, aarch64_get_mem_u64 (cpu, address + 8));

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rd, SP_OK, address);
}

static void
load_pair_long_double (sim_cpu *cpu, int32_t offset, WriteBack wb)
{
  FRegister a;
  unsigned rn = INSTR (14, 10);
  unsigned rd = INSTR (9, 5);
  unsigned rm = INSTR (4, 0);
  uint64_t address = aarch64_get_reg_u64 (cpu, rd, SP_OK);

  if (rm == rn)
    HALT_UNALLOC;

  offset <<= 4;

  if (wb != Post)
    address += offset;

  aarch64_get_mem_long_double (cpu, address, & a);
  aarch64_set_FP_long_double (cpu, rm, a);
  aarch64_get_mem_long_double (cpu, address + 16, & a);
  aarch64_set_FP_long_double (cpu, rn, a);

  if (wb == Post)
    address += offset;

  if (wb != NoWriteBack)
    aarch64_set_reg_u64 (cpu, rd, SP_OK, address);
}

static void
dex_load_store_pair_fp (sim_cpu *cpu)
{
  /* instr[31,30] = size (10=> 128-bit, 01=> 64-bit, 00=> 32-bit)
     instr[29,25] = instruction encoding
     instr[24,23] = addressing mode (10=> offset, 01=> post, 11=> pre)
     instr[22]    = load/store (1=> load)
     instr[21,15] = signed, scaled, offset
     instr[14,10] = Rn
     instr[ 9, 5] = Rd
     instr[ 4, 0] = Rm  */

  uint32_t dispatch = ((INSTR (31, 30) << 3) | INSTR (24, 22));
  int32_t offset = simm32 (aarch64_get_instr (cpu), 21, 15);

  switch (dispatch)
    {
    case 2: store_pair_float (cpu, offset, Post); return;
    case 3: load_pair_float  (cpu, offset, Post); return;
    case 4: store_pair_float (cpu, offset, NoWriteBack); return;
    case 5: load_pair_float  (cpu, offset, NoWriteBack); return;
    case 6: store_pair_float (cpu, offset, Pre); return;
    case 7: load_pair_float  (cpu, offset, Pre); return;

    case 10: store_pair_double (cpu, offset, Post); return;
    case 11: load_pair_double  (cpu, offset, Post); return;
    case 12: store_pair_double (cpu, offset, NoWriteBack); return;
    case 13: load_pair_double  (cpu, offset, NoWriteBack); return;
    case 14: store_pair_double (cpu, offset, Pre); return;
    case 15: load_pair_double  (cpu, offset, Pre); return;

    case 18: store_pair_long_double (cpu, offset, Post); return;
    case 19: load_pair_long_double  (cpu, offset, Post); return;
    case 20: store_pair_long_double (cpu, offset, NoWriteBack); return;
    case 21: load_pair_long_double  (cpu, offset, NoWriteBack); return;
    case 22: store_pair_long_double (cpu, offset, Pre); return;
    case 23: load_pair_long_double  (cpu, offset, Pre); return;

    default:
      HALT_UNALLOC;
    }
}

static inline unsigned
vec_reg (unsigned v, unsigned o)
{
  return (v + o) & 0x3F;
}

/* Load multiple N-element structures to M consecutive registers.  */
static void
vec_load (sim_cpu *cpu, uint64_t address, unsigned N, unsigned M)
{
  int      all  = INSTR (30, 30);
  unsigned size = INSTR (11, 10);
  unsigned vd   = INSTR (4, 0);
  unsigned rpt = (N == M) ? 1 : M;
  unsigned selem = N;
  unsigned i, j, k;

  switch (size)
    {
    case 0: /* 8-bit operations.  */
      for (i = 0; i < rpt; i++)
	for (j = 0; j < (8 + (8 * all)); j++)
	  for (k = 0; k < selem; k++)
	    {
	      aarch64_set_vec_u8 (cpu, vec_reg (vd, i + k), j,
				  aarch64_get_mem_u8 (cpu, address));
	      address += 1;
	    }
      return;

    case 1: /* 16-bit operations.  */
      for (i = 0; i < rpt; i++)
	for (j = 0; j < (4 + (4 * all)); j++)
	  for (k = 0; k < selem; k++)
	    {
	      aarch64_set_vec_u16 (cpu, vec_reg (vd, i + k), j,
				   aarch64_get_mem_u16 (cpu, address));
	      address += 2;
	    }
      return;

    case 2: /* 32-bit operations.  */
      for (i = 0; i < rpt; i++)
	for (j = 0; j < (2 + (2 * all)); j++)
	  for (k = 0; k < selem; k++)
	    {
	      aarch64_set_vec_u32 (cpu, vec_reg (vd, i + k), j,
				   aarch64_get_mem_u32 (cpu, address));
	      address += 4;
	    }
      return;

    case 3: /* 64-bit operations.  */
      for (i = 0; i < rpt; i++)
	for (j = 0; j < (1 + all); j++)
	  for (k = 0; k < selem; k++)
	    {
	      aarch64_set_vec_u64 (cpu, vec_reg (vd, i + k), j,
				   aarch64_get_mem_u64 (cpu, address));
	      address += 8;
	    }
      return;
    }
}

/* Load multiple 4-element structures into four consecutive registers.  */
static void
LD4 (sim_cpu *cpu, uint64_t address)
{
  vec_load (cpu, address, 4, 4);
}

/* Load multiple 3-element structures into three consecutive registers.  */
static void
LD3 (sim_cpu *cpu, uint64_t address)
{
  vec_load (cpu, address, 3, 3);
}

/* Load multiple 2-element structures into two consecutive registers.  */
static void
LD2 (sim_cpu *cpu, uint64_t address)
{
  vec_load (cpu, address, 2, 2);
}

/* Load multiple 1-element structures into one register.  */
static void
LD1_1 (sim_cpu *cpu, uint64_t address)
{
  vec_load (cpu, address, 1, 1);
}

/* Load multiple 1-element structures into two registers.  */
static void
LD1_2 (sim_cpu *cpu, uint64_t address)
{
  vec_load (cpu, address, 1, 2);
}

/* Load multiple 1-element structures into three registers.  */
static void
LD1_3 (sim_cpu *cpu, uint64_t address)
{
  vec_load (cpu, address, 1, 3);
}

/* Load multiple 1-element structures into four registers.  */
static void
LD1_4 (sim_cpu *cpu, uint64_t address)
{
  vec_load (cpu, address, 1, 4);
}

/* Store multiple N-element structures from M consecutive registers.  */
static void
vec_store (sim_cpu *cpu, uint64_t address, unsigned N, unsigned M)
{
  int      all  = INSTR (30, 30);
  unsigned size = INSTR (11, 10);
  unsigned vd   = INSTR (4, 0);
  unsigned rpt = (N == M) ? 1 : M;
  unsigned selem = N;
  unsigned i, j, k;

  switch (size)
    {
    case 0: /* 8-bit operations.  */
      for (i = 0; i < rpt; i++)
	for (j = 0; j < (8 + (8 * all)); j++)
	  for (k = 0; k < selem; k++)
	    {
	      aarch64_set_mem_u8
		(cpu, address,
		 aarch64_get_vec_u8 (cpu, vec_reg (vd, i + k), j));
	      address += 1;
	    }
      return;

    case 1: /* 16-bit operations.  */
      for (i = 0; i < rpt; i++)
	for (j = 0; j < (4 + (4 * all)); j++)
	  for (k = 0; k < selem; k++)
	    {
	      aarch64_set_mem_u16
		(cpu, address,
		 aarch64_get_vec_u16 (cpu, vec_reg (vd, i + k), j));
	      address += 2;
	    }
      return;

    case 2: /* 32-bit operations.  */
      for (i = 0; i < rpt; i++)
	for (j = 0; j < (2 + (2 * all)); j++)
	  for (k = 0; k < selem; k++)
	    {
	      aarch64_set_mem_u32
		(cpu, address,
		 aarch64_get_vec_u32 (cpu, vec_reg (vd, i + k), j));
	      address += 4;
	    }
      return;

    case 3: /* 64-bit operations.  */
      for (i = 0; i < rpt; i++)
	for (j = 0; j < (1 + all); j++)
	  for (k = 0; k < selem; k++)
	    {
	      aarch64_set_mem_u64
		(cpu, address,
		 aarch64_get_vec_u64 (cpu, vec_reg (vd, i + k), j));
	      address += 8;
	    }
      return;
    }
}

/* Store multiple 4-element structure from four consecutive registers.  */
static void
ST4 (sim_cpu *cpu, uint64_t address)
{
  vec_store (cpu, address, 4, 4);
}

/* Store multiple 3-element structures from three consecutive registers.  */
static void
ST3 (sim_cpu *cpu, uint64_t address)
{
  vec_store (cpu, address, 3, 3);
}

/* Store multiple 2-element structures from two consecutive registers.  */
static void
ST2 (sim_cpu *cpu, uint64_t address)
{
  vec_store (cpu, address, 2, 2);
}

/* Store multiple 1-element structures from one register.  */
static void
ST1_1 (sim_cpu *cpu, uint64_t address)
{
  vec_store (cpu, address, 1, 1);
}

/* Store multiple 1-element structures from two registers.  */
static void
ST1_2 (sim_cpu *cpu, uint64_t address)
{
  vec_store (cpu, address, 1, 2);
}

/* Store multiple 1-element structures from three registers.  */
static void
ST1_3 (sim_cpu *cpu, uint64_t address)
{
  vec_store (cpu, address, 1, 3);
}

/* Store multiple 1-element structures from four registers.  */
static void
ST1_4 (sim_cpu *cpu, uint64_t address)
{
  vec_store (cpu, address, 1, 4);
}

#define LDn_STn_SINGLE_LANE_AND_SIZE()				\
  do								\
    {								\
      switch (INSTR (15, 14))					\
	{							\
	case 0:							\
	  lane = (full << 3) | (s << 2) | size;			\
	  size = 0;						\
	  break;						\
								\
	case 1:							\
	  if ((size & 1) == 1)					\
	    HALT_UNALLOC;					\
	  lane = (full << 2) | (s << 1) | (size >> 1);		\
	  size = 1;						\
	  break;						\
								\
	case 2:							\
	  if ((size & 2) == 2)					\
	    HALT_UNALLOC;					\
								\
	  if ((size & 1) == 0)					\
	    {							\
	      lane = (full << 1) | s;				\
	      size = 2;						\
	    }							\
	  else							\
	    {							\
	      if (s)						\
		HALT_UNALLOC;					\
	      lane = full;					\
	      size = 3;						\
	    }							\
	  break;						\
								\
	default:						\
	  HALT_UNALLOC;						\
	}							\
    }								\
  while (0)

/* Load single structure into one lane of N registers.  */
static void
do_vec_LDn_single (sim_cpu *cpu, uint64_t address)
{
  /* instr[31]    = 0
     instr[30]    = element selector 0=>half, 1=>all elements
     instr[29,24] = 00 1101
     instr[23]    = 0=>simple, 1=>post
     instr[22]    = 1
     instr[21]    = width: LD1-or-LD3 (0) / LD2-or-LD4 (1)
     instr[20,16] = 0 0000 (simple), Vinc (reg-post-inc, no SP),
                      11111 (immediate post inc)
     instr[15,13] = opcode
     instr[12]    = S, used for lane number
     instr[11,10] = size, also used for lane number
     instr[9,5]   = address
     instr[4,0]   = Vd  */

  unsigned full = INSTR (30, 30);
  unsigned vd = INSTR (4, 0);
  unsigned size = INSTR (11, 10);
  unsigned s = INSTR (12, 12);
  int nregs = ((INSTR (13, 13) << 1) | INSTR (21, 21)) + 1;
  int lane = 0;
  int i;

  NYI_assert (29, 24, 0x0D);
  NYI_assert (22, 22, 1);

  /* Compute the lane number first (using size), and then compute size.  */
  LDn_STn_SINGLE_LANE_AND_SIZE ();

  for (i = 0; i < nregs; i++)
    switch (size)
      {
      case 0:
	{
	  uint8_t val = aarch64_get_mem_u8 (cpu, address + i);
	  aarch64_set_vec_u8 (cpu, vd + i, lane, val);
	  break;
	}

      case 1:
	{
	  uint16_t val = aarch64_get_mem_u16 (cpu, address + (i * 2));
	  aarch64_set_vec_u16 (cpu, vd + i, lane, val);
	  break;
	}

      case 2:
	{
	  uint32_t val = aarch64_get_mem_u32 (cpu, address + (i * 4));
	  aarch64_set_vec_u32 (cpu, vd + i, lane, val);
	  break;
	}

      case 3:
	{
	  uint64_t val = aarch64_get_mem_u64 (cpu, address + (i * 8));
	  aarch64_set_vec_u64 (cpu, vd + i, lane, val);
	  break;
	}
      }
}

/* Store single structure from one lane from N registers.  */
static void
do_vec_STn_single (sim_cpu *cpu, uint64_t address)
{
  /* instr[31]    = 0
     instr[30]    = element selector 0=>half, 1=>all elements
     instr[29,24] = 00 1101
     instr[23]    = 0=>simple, 1=>post
     instr[22]    = 0
     instr[21]    = width: LD1-or-LD3 (0) / LD2-or-LD4 (1)
     instr[20,16] = 0 0000 (simple), Vinc (reg-post-inc, no SP),
                      11111 (immediate post inc)
     instr[15,13] = opcode
     instr[12]    = S, used for lane number
     instr[11,10] = size, also used for lane number
     instr[9,5]   = address
     instr[4,0]   = Vd  */

  unsigned full = INSTR (30, 30);
  unsigned vd = INSTR (4, 0);
  unsigned size = INSTR (11, 10);
  unsigned s = INSTR (12, 12);
  int nregs = ((INSTR (13, 13) << 1) | INSTR (21, 21)) + 1;
  int lane = 0;
  int i;

  NYI_assert (29, 24, 0x0D);
  NYI_assert (22, 22, 0);

  /* Compute the lane number first (using size), and then compute size.  */
  LDn_STn_SINGLE_LANE_AND_SIZE ();

  for (i = 0; i < nregs; i++)
    switch (size)
      {
      case 0:
	{
	  uint8_t val = aarch64_get_vec_u8 (cpu, vd + i, lane);
	  aarch64_set_mem_u8 (cpu, address + i, val);
	  break;
	}

      case 1:
	{
	  uint16_t val = aarch64_get_vec_u16 (cpu, vd + i, lane);
	  aarch64_set_mem_u16 (cpu, address + (i * 2), val);
	  break;
	}

      case 2:
	{
	  uint32_t val = aarch64_get_vec_u32 (cpu, vd + i, lane);
	  aarch64_set_mem_u32 (cpu, address + (i * 4), val);
	  break;
	}

      case 3:
	{
	  uint64_t val = aarch64_get_vec_u64 (cpu, vd + i, lane);
	  aarch64_set_mem_u64 (cpu, address + (i * 8), val);
	  break;
	}
      }
}

/* Load single structure into all lanes of N registers.  */
static void
do_vec_LDnR (sim_cpu *cpu, uint64_t address)
{
  /* instr[31]    = 0
     instr[30]    = element selector 0=>half, 1=>all elements
     instr[29,24] = 00 1101
     instr[23]    = 0=>simple, 1=>post
     instr[22]    = 1
     instr[21]    = width: LD1R-or-LD3R (0) / LD2R-or-LD4R (1)
     instr[20,16] = 0 0000 (simple), Vinc (reg-post-inc, no SP),
                      11111 (immediate post inc)
     instr[15,14] = 11
     instr[13]    = width: LD1R-or-LD2R (0) / LD3R-or-LD4R (1)
     instr[12]    = 0
     instr[11,10] = element size 00=> byte(b), 01=> half(h),
                                 10=> word(s), 11=> double(d)
     instr[9,5]   = address
     instr[4,0]   = Vd  */

  unsigned full = INSTR (30, 30);
  unsigned vd = INSTR (4, 0);
  unsigned size = INSTR (11, 10);
  int nregs = ((INSTR (13, 13) << 1) | INSTR (21, 21)) + 1;
  int i, n;

  NYI_assert (29, 24, 0x0D);
  NYI_assert (22, 22, 1);
  NYI_assert (15, 14, 3);
  NYI_assert (12, 12, 0);

  for (n = 0; n < nregs; n++)
    switch (size)
      {
      case 0:
	{
	  uint8_t val = aarch64_get_mem_u8 (cpu, address + n);
	  for (i = 0; i < (full ? 16 : 8); i++)
	    aarch64_set_vec_u8 (cpu, vd + n, i, val);
	  break;
	}

      case 1:
	{
	  uint16_t val = aarch64_get_mem_u16 (cpu, address + (n * 2));
	  for (i = 0; i < (full ? 8 : 4); i++)
	    aarch64_set_vec_u16 (cpu, vd + n, i, val);
	  break;
	}

      case 2:
	{
	  uint32_t val = aarch64_get_mem_u32 (cpu, address + (n * 4));
	  for (i = 0; i < (full ? 4 : 2); i++)
	    aarch64_set_vec_u32 (cpu, vd + n, i, val);
	  break;
	}

      case 3:
	{
	  uint64_t val = aarch64_get_mem_u64 (cpu, address + (n * 8));
	  for (i = 0; i < (full ? 2 : 1); i++)
	    aarch64_set_vec_u64 (cpu, vd + n, i, val);
	  break;
	}

      default:
	HALT_UNALLOC;
      }
}

static void
do_vec_load_store (sim_cpu *cpu)
{
  /* {LD|ST}<N>   {Vd..Vd+N}, vaddr

     instr[31]    = 0
     instr[30]    = element selector 0=>half, 1=>all elements
     instr[29,25] = 00110
     instr[24]    = 0=>multiple struct, 1=>single struct
     instr[23]    = 0=>simple, 1=>post
     instr[22]    = 0=>store, 1=>load
     instr[21]    = 0 (LDn) / small(0)-large(1) selector (LDnR)
     instr[20,16] = 00000 (simple), Vinc (reg-post-inc, no SP),
                    11111 (immediate post inc)
     instr[15,12] = elements and destinations.  eg for load:
                     0000=>LD4 => load multiple 4-element to
		     four consecutive registers
                     0100=>LD3 => load multiple 3-element to
		     three consecutive registers
                     1000=>LD2 => load multiple 2-element to
		     two consecutive registers
                     0010=>LD1 => load multiple 1-element to
		     four consecutive registers
                     0110=>LD1 => load multiple 1-element to
		     three consecutive registers
                     1010=>LD1 => load multiple 1-element to
		     two consecutive registers
                     0111=>LD1 => load multiple 1-element to
		     one register
                     1100=>LDR1,LDR2
                     1110=>LDR3,LDR4
     instr[11,10] = element size 00=> byte(b), 01=> half(h),
                                 10=> word(s), 11=> double(d)
     instr[9,5]   = Vn, can be SP
     instr[4,0]   = Vd  */

  int single;
  int post;
  int load;
  unsigned vn;
  uint64_t address;
  int type;

  if (INSTR (31, 31) != 0 || INSTR (29, 25) != 0x06)
    HALT_NYI;

  single = INSTR (24, 24);
  post = INSTR (23, 23);
  load = INSTR (22, 22);
  type = INSTR (15, 12);
  vn = INSTR (9, 5);
  address = aarch64_get_reg_u64 (cpu, vn, SP_OK);

  if (! single && INSTR (21, 21) != 0)
    HALT_UNALLOC;

  if (post)
    {
      unsigned vm = INSTR (20, 16);

      if (vm == R31)
	{
	  unsigned sizeof_operation;

	  if (single)
	    {
	      if ((type >= 0) && (type <= 11))
		{
		  int nregs = ((INSTR (13, 13) << 1) | INSTR (21, 21)) + 1;
		  switch (INSTR (15, 14))
		    {
		    case 0:
		      sizeof_operation = nregs * 1;
		      break;
		    case 1:
		      sizeof_operation = nregs * 2;
		      break;
		    case 2:
		      if (INSTR (10, 10) == 0)
			sizeof_operation = nregs * 4;
		      else
			sizeof_operation = nregs * 8;
		      break;
		    default:
		      HALT_UNALLOC;
		    }
		}
	      else if (type == 0xC)
		{
		  sizeof_operation = INSTR (21, 21) ? 2 : 1;
		  sizeof_operation <<= INSTR (11, 10);
		}
	      else if (type == 0xE)
		{
		  sizeof_operation = INSTR (21, 21) ? 4 : 3;
		  sizeof_operation <<= INSTR (11, 10);
		}
	      else
		HALT_UNALLOC;
	    }
	  else
	    {
	      switch (type)
		{
		case 0: sizeof_operation = 32; break;
		case 4: sizeof_operation = 24; break;
		case 8: sizeof_operation = 16; break;

		case 7:
		  /* One register, immediate offset variant.  */
		  sizeof_operation = 8;
		  break;

		case 10:
		  /* Two registers, immediate offset variant.  */
		  sizeof_operation = 16;
		  break;

		case 6:
		  /* Three registers, immediate offset variant.  */
		  sizeof_operation = 24;
		  break;

		case 2:
		  /* Four registers, immediate offset variant.  */
		  sizeof_operation = 32;
		  break;

		default:
		  HALT_UNALLOC;
		}

	      if (INSTR (30, 30))
		sizeof_operation *= 2;
	    }

	  aarch64_set_reg_u64 (cpu, vn, SP_OK, address + sizeof_operation);
	}
      else
	aarch64_set_reg_u64 (cpu, vn, SP_OK,
			     address + aarch64_get_reg_u64 (cpu, vm, NO_SP));
    }
  else
    {
      NYI_assert (20, 16, 0);
    }

  if (single)
    {
      if (load)
	{
	  if ((type >= 0) && (type <= 11))
	    do_vec_LDn_single (cpu, address);
	  else if ((type == 0xC) || (type == 0xE))
	    do_vec_LDnR (cpu, address);
	  else
	    HALT_UNALLOC;
	  return;
	}

      /* Stores.  */
      if ((type >= 0) && (type <= 11))
	{
	  do_vec_STn_single (cpu, address);
	  return;
	}

      HALT_UNALLOC;
    }

  if (load)
    {
      switch (type)
	{
	case 0:  LD4 (cpu, address); return;
	case 4:  LD3 (cpu, address); return;
	case 8:  LD2 (cpu, address); return;
	case 2:  LD1_4 (cpu, address); return;
	case 6:  LD1_3 (cpu, address); return;
	case 10: LD1_2 (cpu, address); return;
	case 7:  LD1_1 (cpu, address); return;

	default:
	  HALT_UNALLOC;
	}
    }

  /* Stores.  */
  switch (type)
    {
    case 0:  ST4 (cpu, address); return;
    case 4:  ST3 (cpu, address); return;
    case 8:  ST2 (cpu, address); return;
    case 2:  ST1_4 (cpu, address); return;
    case 6:  ST1_3 (cpu, address); return;
    case 10: ST1_2 (cpu, address); return;
    case 7:  ST1_1 (cpu, address); return;
    default:
      HALT_UNALLOC;
    }
}

static void
dexLdSt (sim_cpu *cpu)
{
  /* uint32_t group = dispatchGroup (aarch64_get_instr (cpu));
     assert  group == GROUP_LDST_0100 || group == GROUP_LDST_0110 ||
             group == GROUP_LDST_1100 || group == GROUP_LDST_1110
     bits [29,28:26] of a LS are the secondary dispatch vector.  */
  uint32_t group2 = dispatchLS (aarch64_get_instr (cpu));

  switch (group2)
    {
    case LS_EXCL_000:
      dexLoadExclusive (cpu); return;

    case LS_LIT_010:
    case LS_LIT_011:
      dexLoadLiteral (cpu); return;

    case LS_OTHER_110:
    case LS_OTHER_111:
      dexLoadOther (cpu); return;

    case LS_ADVSIMD_001:
      do_vec_load_store (cpu); return;

    case LS_PAIR_100:
      dex_load_store_pair_gr (cpu); return;

    case LS_PAIR_101:
      dex_load_store_pair_fp (cpu); return;

    default:
      /* Should never reach here.  */
      HALT_NYI;
    }
}

/* Specific decode and execute for group Data Processing Register.  */

static void
dexLogicalShiftedRegister (sim_cpu *cpu)
{
  /* instr[31]    = size : 0 ==> 32 bit, 1 ==> 64 bit
     instr[30,29] = op
     instr[28:24] = 01010
     instr[23,22] = shift : 0 ==> LSL, 1 ==> LSR, 2 ==> ASR, 3 ==> ROR
     instr[21]    = N
     instr[20,16] = Rm
     instr[15,10] = count : must be 0xxxxx for 32 bit
     instr[9,5]   = Rn
     instr[4,0]   = Rd  */

  uint32_t size      = INSTR (31, 31);
  Shift    shiftType = INSTR (23, 22);
  uint32_t count     = INSTR (15, 10);

  /* 32 bit operations must have count[5] = 0.
     or else we have an UNALLOC.  */
  if (size == 0 && uimm (count, 5, 5))
    HALT_UNALLOC;

  /* Dispatch on size:op:N.  */
  switch ((INSTR (31, 29) << 1) | INSTR (21, 21))
    {
    case 0: and32_shift  (cpu, shiftType, count); return;
    case 1: bic32_shift  (cpu, shiftType, count); return;
    case 2: orr32_shift  (cpu, shiftType, count); return;
    case 3: orn32_shift  (cpu, shiftType, count); return;
    case 4: eor32_shift  (cpu, shiftType, count); return;
    case 5: eon32_shift  (cpu, shiftType, count); return;
    case 6: ands32_shift (cpu, shiftType, count); return;
    case 7: bics32_shift (cpu, shiftType, count); return;
    case 8: and64_shift  (cpu, shiftType, count); return;
    case 9: bic64_shift  (cpu, shiftType, count); return;
    case 10:orr64_shift  (cpu, shiftType, count); return;
    case 11:orn64_shift  (cpu, shiftType, count); return;
    case 12:eor64_shift  (cpu, shiftType, count); return;
    case 13:eon64_shift  (cpu, shiftType, count); return;
    case 14:ands64_shift (cpu, shiftType, count); return;
    case 15:bics64_shift (cpu, shiftType, count); return;
    }
}

/* 32 bit conditional select.  */
static void
csel32 (sim_cpu *cpu, CondCode cc)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       testConditionCode (cpu, cc)
		       ? aarch64_get_reg_u32 (cpu, rn, NO_SP)
		       : aarch64_get_reg_u32 (cpu, rm, NO_SP));
}

/* 64 bit conditional select.  */
static void
csel64 (sim_cpu *cpu, CondCode cc)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       testConditionCode (cpu, cc)
		       ? aarch64_get_reg_u64 (cpu, rn, NO_SP)
		       : aarch64_get_reg_u64 (cpu, rm, NO_SP));
}

/* 32 bit conditional increment.  */
static void
csinc32 (sim_cpu *cpu, CondCode cc)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       testConditionCode (cpu, cc)
		       ? aarch64_get_reg_u32 (cpu, rn, NO_SP)
		       : aarch64_get_reg_u32 (cpu, rm, NO_SP) + 1);
}

/* 64 bit conditional increment.  */
static void
csinc64 (sim_cpu *cpu, CondCode cc)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       testConditionCode (cpu, cc)
		       ? aarch64_get_reg_u64 (cpu, rn, NO_SP)
		       : aarch64_get_reg_u64 (cpu, rm, NO_SP) + 1);
}

/* 32 bit conditional invert.  */
static void
csinv32 (sim_cpu *cpu, CondCode cc)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       testConditionCode (cpu, cc)
		       ? aarch64_get_reg_u32 (cpu, rn, NO_SP)
		       : ~ aarch64_get_reg_u32 (cpu, rm, NO_SP));
}

/* 64 bit conditional invert.  */
static void
csinv64 (sim_cpu *cpu, CondCode cc)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       testConditionCode (cpu, cc)
		       ? aarch64_get_reg_u64 (cpu, rn, NO_SP)
		       : ~ aarch64_get_reg_u64 (cpu, rm, NO_SP));
}

/* 32 bit conditional negate.  */
static void
csneg32 (sim_cpu *cpu, CondCode cc)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       testConditionCode (cpu, cc)
		       ? aarch64_get_reg_u32 (cpu, rn, NO_SP)
		       : - aarch64_get_reg_u32 (cpu, rm, NO_SP));
}

/* 64 bit conditional negate.  */
static void
csneg64 (sim_cpu *cpu, CondCode cc)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       testConditionCode (cpu, cc)
		       ? aarch64_get_reg_u64 (cpu, rn, NO_SP)
		       : - aarch64_get_reg_u64 (cpu, rm, NO_SP));
}

static void
dexCondSelect (sim_cpu *cpu)
{
  /* instr[28,21] = 11011011
     instr[31]    = size : 0 ==> 32 bit, 1 ==> 64 bit
     instr[30:11,10] = op : 000 ==> CSEL, 001 ==> CSINC,
                            100 ==> CSINV, 101 ==> CSNEG,
                            _1_ ==> UNALLOC
     instr[29] = S : 0 ==> ok, 1 ==> UNALLOC
     instr[15,12] = cond
     instr[29] = S : 0 ==> ok, 1 ==> UNALLOC  */

  CondCode cc = INSTR (15, 12);
  uint32_t S = INSTR (29, 29);
  uint32_t op2 = INSTR (11, 10);

  if (S == 1)
    HALT_UNALLOC;

  if (op2 & 0x2)
    HALT_UNALLOC;

  switch ((INSTR (31, 30) << 1) | op2)
    {
    case 0: csel32  (cpu, cc); return;
    case 1: csinc32 (cpu, cc); return;
    case 2: csinv32 (cpu, cc); return;
    case 3: csneg32 (cpu, cc); return;
    case 4: csel64  (cpu, cc); return;
    case 5: csinc64 (cpu, cc); return;
    case 6: csinv64 (cpu, cc); return;
    case 7: csneg64 (cpu, cc); return;
    }
}

/* Some helpers for counting leading 1 or 0 bits.  */

/* Counts the number of leading bits which are the same
   in a 32 bit value in the range 1 to 32.  */
static uint32_t
leading32 (uint32_t value)
{
  int32_t mask= 0xffff0000;
  uint32_t count= 16; /* Counts number of bits set in mask.  */
  uint32_t lo = 1;    /* Lower bound for number of sign bits.  */
  uint32_t hi = 32;   /* Upper bound for number of sign bits.  */

  while (lo + 1 < hi)
    {
      int32_t test = (value & mask);

      if (test == 0 || test == mask)
	{
	  lo = count;
	  count = (lo + hi) / 2;
	  mask >>= (count - lo);
	}
      else
	{
	  hi = count;
	  count = (lo + hi) / 2;
	  mask <<= hi - count;
	}
    }

  if (lo != hi)
    {
      int32_t test;

      mask >>= 1;
      test = (value & mask);

      if (test == 0 || test == mask)
	count = hi;
      else
	count = lo;
    }

  return count;
}

/* Counts the number of leading bits which are the same
   in a 64 bit value in the range 1 to 64.  */
static uint64_t
leading64 (uint64_t value)
{
  int64_t mask= 0xffffffff00000000LL;
  uint64_t count = 32; /* Counts number of bits set in mask.  */
  uint64_t lo = 1;     /* Lower bound for number of sign bits.  */
  uint64_t hi = 64;    /* Upper bound for number of sign bits.  */

  while (lo + 1 < hi)
    {
      int64_t test = (value & mask);

      if (test == 0 || test == mask)
	{
	  lo = count;
	  count = (lo + hi) / 2;
	  mask >>= (count - lo);
	}
      else
	{
	  hi = count;
	  count = (lo + hi) / 2;
	  mask <<= hi - count;
	}
    }

  if (lo != hi)
    {
      int64_t test;

      mask >>= 1;
      test = (value & mask);

      if (test == 0 || test == mask)
	count = hi;
      else
	count = lo;
    }

  return count;
}

/* Bit operations.  */
/* N.B register args may not be SP.  */

/* 32 bit count leading sign bits.  */
static void
cls32 (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  /* N.B. the result needs to exclude the leading bit.  */
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, leading32 (aarch64_get_reg_u32 (cpu, rn, NO_SP)) - 1);
}

/* 64 bit count leading sign bits.  */
static void
cls64 (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  /* N.B. the result needs to exclude the leading bit.  */
  aarch64_set_reg_u64
    (cpu, rd, NO_SP, leading64 (aarch64_get_reg_u64 (cpu, rn, NO_SP)) - 1);
}

/* 32 bit count leading zero bits.  */
static void
clz32 (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  uint32_t value = aarch64_get_reg_u32 (cpu, rn, NO_SP);

  /* if the sign (top) bit is set then the count is 0.  */
  if (pick32 (value, 31, 31))
    aarch64_set_reg_u64 (cpu, rd, NO_SP, 0L);
  else
    aarch64_set_reg_u64 (cpu, rd, NO_SP, leading32 (value));
}

/* 64 bit count leading zero bits.  */
static void
clz64 (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  uint64_t value = aarch64_get_reg_u64 (cpu, rn, NO_SP);

  /* if the sign (top) bit is set then the count is 0.  */
  if (pick64 (value, 63, 63))
    aarch64_set_reg_u64 (cpu, rd, NO_SP, 0L);
  else
    aarch64_set_reg_u64 (cpu, rd, NO_SP, leading64 (value));
}

/* 32 bit reverse bits.  */
static void
rbit32 (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  uint32_t value = aarch64_get_reg_u32 (cpu, rn, NO_SP);
  uint32_t result = 0;
  int i;

  for (i = 0; i < 32; i++)
    {
      result <<= 1;
      result |= (value & 1);
      value >>= 1;
    }
  aarch64_set_reg_u64 (cpu, rd, NO_SP, result);
}

/* 64 bit reverse bits.  */
static void
rbit64 (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  uint64_t value = aarch64_get_reg_u64 (cpu, rn, NO_SP);
  uint64_t result = 0;
  int i;

  for (i = 0; i < 64; i++)
    {
      result <<= 1;
      result |= (value & 1UL);
      value >>= 1;
    }
  aarch64_set_reg_u64 (cpu, rd, NO_SP, result);
}

/* 32 bit reverse bytes.  */
static void
rev32 (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  uint32_t value = aarch64_get_reg_u32 (cpu, rn, NO_SP);
  uint32_t result = 0;
  int i;

  for (i = 0; i < 4; i++)
    {
      result <<= 8;
      result |= (value & 0xff);
      value >>= 8;
    }
  aarch64_set_reg_u64 (cpu, rd, NO_SP, result);
}

/* 64 bit reverse bytes.  */
static void
rev64 (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  uint64_t value = aarch64_get_reg_u64 (cpu, rn, NO_SP);
  uint64_t result = 0;
  int i;

  for (i = 0; i < 8; i++)
    {
      result <<= 8;
      result |= (value & 0xffULL);
      value >>= 8;
    }
  aarch64_set_reg_u64 (cpu, rd, NO_SP, result);
}

/* 32 bit reverse shorts.  */
/* N.B.this reverses the order of the bytes in each half word.  */
static void
revh32 (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  uint32_t value = aarch64_get_reg_u32 (cpu, rn, NO_SP);
  uint32_t result = 0;
  int i;

  for (i = 0; i < 2; i++)
    {
      result <<= 8;
      result |= (value & 0x00ff00ff);
      value >>= 8;
    }
  aarch64_set_reg_u64 (cpu, rd, NO_SP, result);
}

/* 64 bit reverse shorts.  */
/* N.B.this reverses the order of the bytes in each half word.  */
static void
revh64 (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  uint64_t value = aarch64_get_reg_u64 (cpu, rn, NO_SP);
  uint64_t result = 0;
  int i;

  for (i = 0; i < 2; i++)
    {
      result <<= 8;
      result |= (value & 0x00ff00ff00ff00ffULL);
      value >>= 8;
    }
  aarch64_set_reg_u64 (cpu, rd, NO_SP, result);
}

static void
dexDataProc1Source (sim_cpu *cpu)
{
  /* instr[30]    = 1
     instr[28,21] = 111010110
     instr[31]    = size : 0 ==> 32 bit, 1 ==> 64 bit
     instr[29]    = S : 0 ==> ok, 1 ==> UNALLOC
     instr[20,16] = opcode2 : 00000 ==> ok, ow ==> UNALLOC
     instr[15,10] = opcode : 000000 ==> RBIT, 000001 ==> REV16,
                             000010 ==> REV, 000011 ==> UNALLOC
                             000100 ==> CLZ, 000101 ==> CLS
                             ow ==> UNALLOC
     instr[9,5]   = rn : may not be SP
     instr[4,0]   = rd : may not be SP.  */

  uint32_t S = INSTR (29, 29);
  uint32_t opcode2 = INSTR (20, 16);
  uint32_t opcode = INSTR (15, 10);
  uint32_t dispatch = ((INSTR (31, 31) << 3) | opcode);

  if (S == 1)
    HALT_UNALLOC;

  if (opcode2 != 0)
    HALT_UNALLOC;

  if (opcode & 0x38)
    HALT_UNALLOC;

  switch (dispatch)
    {
    case 0: rbit32 (cpu); return;
    case 1: revh32 (cpu); return;
    case 2: rev32 (cpu); return;
    case 4: clz32 (cpu); return;
    case 5: cls32 (cpu); return;
    case 8: rbit64 (cpu); return;
    case 9: revh64 (cpu); return;
    case 10:rev32 (cpu); return;
    case 11:rev64 (cpu); return;
    case 12:clz64 (cpu); return;
    case 13:cls64 (cpu); return;
    default: HALT_UNALLOC;
    }
}

/* Variable shift.
   Shifts by count supplied in register.
   N.B register args may not be SP.
   These all use the shifted auxiliary function for
   simplicity and clarity.  Writing the actual shift
   inline would avoid a branch and so be faster but
   would also necessitate getting signs right.  */

/* 32 bit arithmetic shift right.  */
static void
asrv32 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64
    (cpu, rd, NO_SP,
     shifted32 (aarch64_get_reg_u32 (cpu, rn, NO_SP), ASR,
		(aarch64_get_reg_u32 (cpu, rm, NO_SP) & 0x1f)));
}

/* 64 bit arithmetic shift right.  */
static void
asrv64 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64
    (cpu, rd, NO_SP,
     shifted64 (aarch64_get_reg_u64 (cpu, rn, NO_SP), ASR,
		(aarch64_get_reg_u64 (cpu, rm, NO_SP) & 0x3f)));
}

/* 32 bit logical shift left.  */
static void
lslv32 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64
    (cpu, rd, NO_SP,
     shifted32 (aarch64_get_reg_u32 (cpu, rn, NO_SP), LSL,
		(aarch64_get_reg_u32 (cpu, rm, NO_SP) & 0x1f)));
}

/* 64 bit arithmetic shift left.  */
static void
lslv64 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64
    (cpu, rd, NO_SP,
     shifted64 (aarch64_get_reg_u64 (cpu, rn, NO_SP), LSL,
		(aarch64_get_reg_u64 (cpu, rm, NO_SP) & 0x3f)));
}

/* 32 bit logical shift right.  */
static void
lsrv32 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64
    (cpu, rd, NO_SP,
     shifted32 (aarch64_get_reg_u32 (cpu, rn, NO_SP), LSR,
		(aarch64_get_reg_u32 (cpu, rm, NO_SP) & 0x1f)));
}

/* 64 bit logical shift right.  */
static void
lsrv64 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64
    (cpu, rd, NO_SP,
     shifted64 (aarch64_get_reg_u64 (cpu, rn, NO_SP), LSR,
		(aarch64_get_reg_u64 (cpu, rm, NO_SP) & 0x3f)));
}

/* 32 bit rotate right.  */
static void
rorv32 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64
    (cpu, rd, NO_SP,
     shifted32 (aarch64_get_reg_u32 (cpu, rn, NO_SP), ROR,
		(aarch64_get_reg_u32 (cpu, rm, NO_SP) & 0x1f)));
}

/* 64 bit rotate right.  */
static void
rorv64 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  aarch64_set_reg_u64
    (cpu, rd, NO_SP,
     shifted64 (aarch64_get_reg_u64 (cpu, rn, NO_SP), ROR,
		(aarch64_get_reg_u64 (cpu, rm, NO_SP) & 0x3f)));
}


/* divide.  */

/* 32 bit signed divide.  */
static void
cpuiv32 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  /* N.B. the pseudo-code does the divide using 64 bit data.  */
  /* TODO : check that this rounds towards zero as required.  */
  int64_t dividend = aarch64_get_reg_s32 (cpu, rn, NO_SP);
  int64_t divisor = aarch64_get_reg_s32 (cpu, rm, NO_SP);

  aarch64_set_reg_s64 (cpu, rd, NO_SP,
		       divisor ? ((int32_t) (dividend / divisor)) : 0);
}

/* 64 bit signed divide.  */
static void
cpuiv64 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  /* TODO : check that this rounds towards zero as required.  */
  int64_t divisor = aarch64_get_reg_s64 (cpu, rm, NO_SP);

  aarch64_set_reg_s64
    (cpu, rd, NO_SP,
     divisor ? (aarch64_get_reg_s64 (cpu, rn, NO_SP) / divisor) : 0);
}

/* 32 bit unsigned divide.  */
static void
udiv32 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  /* N.B. the pseudo-code does the divide using 64 bit data.  */
  uint64_t dividend = aarch64_get_reg_u32 (cpu, rn, NO_SP);
  uint64_t divisor  = aarch64_get_reg_u32 (cpu, rm, NO_SP);

  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       divisor ? (uint32_t) (dividend / divisor) : 0);
}

/* 64 bit unsigned divide.  */
static void
udiv64 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  /* TODO : check that this rounds towards zero as required.  */
  uint64_t divisor = aarch64_get_reg_u64 (cpu, rm, NO_SP);

  aarch64_set_reg_u64
    (cpu, rd, NO_SP,
     divisor ? (aarch64_get_reg_u64 (cpu, rn, NO_SP) / divisor) : 0);
}

static void
dexDataProc2Source (sim_cpu *cpu)
{
  /* assert instr[30] == 0
     instr[28,21] == 11010110
     instr[31] = size : 0 ==> 32 bit, 1 ==> 64 bit
     instr[29] = S : 0 ==> ok, 1 ==> UNALLOC
     instr[15,10] = opcode : 000010 ==> UDIV, 000011 ==> CPUIV,
                             001000 ==> LSLV, 001001 ==> LSRV
                             001010 ==> ASRV, 001011 ==> RORV
                             ow ==> UNALLOC.  */

  uint32_t dispatch;
  uint32_t S = INSTR (29, 29);
  uint32_t opcode = INSTR (15, 10);

  if (S == 1)
    HALT_UNALLOC;

  if (opcode & 0x34)
    HALT_UNALLOC;

  dispatch = (  (INSTR (31, 31) << 3)
	      | (uimm (opcode, 3, 3) << 2)
	      |  uimm (opcode, 1, 0));
  switch (dispatch)
    {
    case 2:  udiv32 (cpu); return;
    case 3:  cpuiv32 (cpu); return;
    case 4:  lslv32 (cpu); return;
    case 5:  lsrv32 (cpu); return;
    case 6:  asrv32 (cpu); return;
    case 7:  rorv32 (cpu); return;
    case 10: udiv64 (cpu); return;
    case 11: cpuiv64 (cpu); return;
    case 12: lslv64 (cpu); return;
    case 13: lsrv64 (cpu); return;
    case 14: asrv64 (cpu); return;
    case 15: rorv64 (cpu); return;
    default: HALT_UNALLOC;
    }
}


/* Multiply.  */

/* 32 bit multiply and add.  */
static void
madd32 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned ra = INSTR (14, 10);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       aarch64_get_reg_u32 (cpu, ra, NO_SP)
		       + aarch64_get_reg_u32 (cpu, rn, NO_SP)
		       * aarch64_get_reg_u32 (cpu, rm, NO_SP));
}

/* 64 bit multiply and add.  */
static void
madd64 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned ra = INSTR (14, 10);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       aarch64_get_reg_u64 (cpu, ra, NO_SP)
		       + (aarch64_get_reg_u64 (cpu, rn, NO_SP)
			  * aarch64_get_reg_u64 (cpu, rm, NO_SP)));
}

/* 32 bit multiply and sub.  */
static void
msub32 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned ra = INSTR (14, 10);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       aarch64_get_reg_u32 (cpu, ra, NO_SP)
		       - aarch64_get_reg_u32 (cpu, rn, NO_SP)
		       * aarch64_get_reg_u32 (cpu, rm, NO_SP));
}

/* 64 bit multiply and sub.  */
static void
msub64 (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned ra = INSTR (14, 10);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       aarch64_get_reg_u64 (cpu, ra, NO_SP)
		       - aarch64_get_reg_u64 (cpu, rn, NO_SP)
		       * aarch64_get_reg_u64 (cpu, rm, NO_SP));
}

/* Signed multiply add long -- source, source2 : 32 bit, source3 : 64 bit.  */
static void
smaddl (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned ra = INSTR (14, 10);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  /* N.B. we need to multiply the signed 32 bit values in rn, rm to
     obtain a 64 bit product.  */
  aarch64_set_reg_s64
    (cpu, rd, NO_SP,
     aarch64_get_reg_s64 (cpu, ra, NO_SP)
     + ((int64_t) aarch64_get_reg_s32 (cpu, rn, NO_SP))
     * ((int64_t) aarch64_get_reg_s32 (cpu, rm, NO_SP)));
}

/* Signed multiply sub long -- source, source2 : 32 bit, source3 : 64 bit.  */
static void
smsubl (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned ra = INSTR (14, 10);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  /* N.B. we need to multiply the signed 32 bit values in rn, rm to
     obtain a 64 bit product.  */
  aarch64_set_reg_s64
    (cpu, rd, NO_SP,
     aarch64_get_reg_s64 (cpu, ra, NO_SP)
     - ((int64_t) aarch64_get_reg_s32 (cpu, rn, NO_SP))
     * ((int64_t) aarch64_get_reg_s32 (cpu, rm, NO_SP)));
}

/* Integer Multiply/Divide.  */

/* First some macros and a helper function.  */
/* Macros to test or access elements of 64 bit words.  */

/* Mask used to access lo 32 bits of 64 bit unsigned int.  */
#define LOW_WORD_MASK ((1ULL << 32) - 1)
/* Return the lo 32 bit word of a 64 bit unsigned int as a 64 bit unsigned int.  */
#define lowWordToU64(_value_u64) ((_value_u64) & LOW_WORD_MASK)
/* Return the hi 32 bit word of a 64 bit unsigned int as a 64 bit unsigned int.  */
#define highWordToU64(_value_u64) ((_value_u64) >> 32)

/* Offset of sign bit in 64 bit signed integger.  */
#define SIGN_SHIFT_U64 63
/* The sign bit itself -- also identifies the minimum negative int value.  */
#define SIGN_BIT_U64 (1UL << SIGN_SHIFT_U64)
/* Return true if a 64 bit signed int presented as an unsigned int is the
   most negative value.  */
#define isMinimumU64(_value_u64) ((_value_u64) == SIGN_BIT_U64)
/* Return true (non-zero) if a 64 bit signed int presented as an unsigned
   int has its sign bit set to false.  */
#define isSignSetU64(_value_u64) ((_value_u64) & SIGN_BIT_U64)
/* Return 1L or -1L according to whether a 64 bit signed int presented as
   an unsigned int has its sign bit set or not.  */
#define signOfU64(_value_u64) (1L + (((value_u64) >> SIGN_SHIFT_U64) * -2L)
/* Clear the sign bit of a 64 bit signed int presented as an unsigned int.  */
#define clearSignU64(_value_u64) ((_value_u64) &= ~SIGN_BIT_U64)

/* Multiply two 64 bit ints and return.
   the hi 64 bits of the 128 bit product.  */

static uint64_t
mul64hi (uint64_t value1, uint64_t value2)
{
  uint64_t resultmid1;
  uint64_t result;
  uint64_t value1_lo = lowWordToU64 (value1);
  uint64_t value1_hi = highWordToU64 (value1) ;
  uint64_t value2_lo = lowWordToU64 (value2);
  uint64_t value2_hi = highWordToU64 (value2);

  /* Cross-multiply and collect results.  */
  uint64_t xproductlo = value1_lo * value2_lo;
  uint64_t xproductmid1 = value1_lo * value2_hi;
  uint64_t xproductmid2 = value1_hi * value2_lo;
  uint64_t xproducthi = value1_hi * value2_hi;
  uint64_t carry = 0;
  /* Start accumulating 64 bit results.  */
  /* Drop bottom half of lowest cross-product.  */
  uint64_t resultmid = xproductlo >> 32;
  /* Add in middle products.  */
  resultmid = resultmid + xproductmid1;

  /* Check for overflow.  */
  if (resultmid < xproductmid1)
    /* Carry over 1 into top cross-product.  */
    carry++;

  resultmid1  = resultmid + xproductmid2;

  /* Check for overflow.  */
  if (resultmid1 < xproductmid2)
    /* Carry over 1 into top cross-product.  */
    carry++;

  /* Drop lowest 32 bits of middle cross-product.  */
  result = resultmid1 >> 32;
  /* Move carry bit to just above middle cross-product highest bit.  */
  carry = carry << 32;

  /* Add top cross-product plus and any carry.  */
  result += xproducthi + carry;

  return result;
}

/* Signed multiply high, source, source2 :
   64 bit, dest <-- high 64-bit of result.  */
static void
smulh (sim_cpu *cpu)
{
  uint64_t uresult;
  int64_t  result;
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  GReg     ra = INSTR (14, 10);
  int64_t  value1 = aarch64_get_reg_u64 (cpu, rn, NO_SP);
  int64_t  value2 = aarch64_get_reg_u64 (cpu, rm, NO_SP);
  uint64_t uvalue1;
  uint64_t uvalue2;
  int  negate = 0;

  if (ra != R31)
    HALT_UNALLOC;

  /* Convert to unsigned and use the unsigned mul64hi routine
     the fix the sign up afterwards.  */
  if (value1 < 0)
    {
      negate = !negate;
      uvalue1 = -value1;
    }
  else
    {
      uvalue1 = value1;
    }

  if (value2 < 0)
    {
      negate = !negate;
      uvalue2 = -value2;
    }
  else
    {
      uvalue2 = value2;
    }

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);

  uresult = mul64hi (uvalue1, uvalue2);
  result = uresult;

  if (negate)
    {
      /* Multiply 128-bit result by -1, which means highpart gets inverted,
	 and has carry in added only if low part is 0.  */
      result = ~result;
      if ((uvalue1 * uvalue2) == 0)
	result += 1;
    }

  aarch64_set_reg_s64 (cpu, rd, NO_SP, result);
}

/* Unsigned multiply add long -- source, source2 :
   32 bit, source3 : 64 bit.  */
static void
umaddl (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned ra = INSTR (14, 10);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* N.B. we need to multiply the signed 32 bit values in rn, rm to
     obtain a 64 bit product.  */
  aarch64_set_reg_u64
    (cpu, rd, NO_SP,
     aarch64_get_reg_u64 (cpu, ra, NO_SP)
     + ((uint64_t) aarch64_get_reg_u32 (cpu, rn, NO_SP))
     * ((uint64_t) aarch64_get_reg_u32 (cpu, rm, NO_SP)));
}

/* Unsigned multiply sub long -- source, source2 : 32 bit, source3 : 64 bit.  */
static void
umsubl (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned ra = INSTR (14, 10);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  /* N.B. we need to multiply the signed 32 bit values in rn, rm to
     obtain a 64 bit product.  */
  aarch64_set_reg_u64
    (cpu, rd, NO_SP,
     aarch64_get_reg_u64 (cpu, ra, NO_SP)
     - ((uint64_t) aarch64_get_reg_u32 (cpu, rn, NO_SP))
     * ((uint64_t) aarch64_get_reg_u32 (cpu, rm, NO_SP)));
}

/* Unsigned multiply high, source, source2 :
   64 bit, dest <-- high 64-bit of result.  */
static void
umulh (sim_cpu *cpu)
{
  unsigned rm = INSTR (20, 16);
  unsigned rn = INSTR (9, 5);
  unsigned rd = INSTR (4, 0);
  GReg     ra = INSTR (14, 10);

  if (ra != R31)
    HALT_UNALLOC;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rd, NO_SP,
		       mul64hi (aarch64_get_reg_u64 (cpu, rn, NO_SP),
				aarch64_get_reg_u64 (cpu, rm, NO_SP)));
}

static void
dexDataProc3Source (sim_cpu *cpu)
{
  /* assert instr[28,24] == 11011.  */
  /* instr[31] = size : 0 ==> 32 bit, 1 ==> 64 bit (for rd at least)
     instr[30,29] = op54 : 00 ==> ok, ow ==> UNALLOC
     instr[23,21] = op31 : 111 ==> UNALLOC, o2 ==> ok
     instr[15] = o0 : 0/1 ==> ok
     instr[23,21:15] ==> op : 0000 ==> MADD, 0001 ==> MSUB,     (32/64 bit)
                              0010 ==> SMADDL, 0011 ==> SMSUBL, (64 bit only)
                              0100 ==> SMULH,                   (64 bit only)
                              1010 ==> UMADDL, 1011 ==> UNSUBL, (64 bit only)
                              1100 ==> UMULH                    (64 bit only)
                              ow ==> UNALLOC.  */

  uint32_t dispatch;
  uint32_t size = INSTR (31, 31);
  uint32_t op54 = INSTR (30, 29);
  uint32_t op31 = INSTR (23, 21);
  uint32_t o0 = INSTR (15, 15);

  if (op54 != 0)
    HALT_UNALLOC;

  if (size == 0)
    {
      if (op31 != 0)
	HALT_UNALLOC;

      if (o0 == 0)
	madd32 (cpu);
      else
	msub32 (cpu);
      return;
    }

  dispatch = (op31 << 1) | o0;

  switch (dispatch)
    {
    case 0:  madd64 (cpu); return;
    case 1:  msub64 (cpu); return;
    case 2:  smaddl (cpu); return;
    case 3:  smsubl (cpu); return;
    case 4:  smulh (cpu); return;
    case 10: umaddl (cpu); return;
    case 11: umsubl (cpu); return;
    case 12: umulh (cpu); return;
    default: HALT_UNALLOC;
    }
}

static void
dexDPReg (sim_cpu *cpu)
{
  /* uint32_t group = dispatchGroup (aarch64_get_instr (cpu));
     assert  group == GROUP_DPREG_0101 || group == GROUP_DPREG_1101
     bits [28:24:21] of a DPReg are the secondary dispatch vector.  */
  uint32_t group2 = dispatchDPReg (aarch64_get_instr (cpu));

  switch (group2)
    {
    case DPREG_LOG_000:
    case DPREG_LOG_001:
      dexLogicalShiftedRegister (cpu); return;

    case DPREG_ADDSHF_010:
      dexAddSubtractShiftedRegister (cpu); return;

    case DPREG_ADDEXT_011:
      dexAddSubtractExtendedRegister (cpu); return;

    case DPREG_ADDCOND_100:
      {
	/* This set bundles a variety of different operations.  */
	/* Check for.  */
	/* 1) add/sub w carry.  */
	uint32_t mask1 = 0x1FE00000U;
	uint32_t val1  = 0x1A000000U;
	/* 2) cond compare register/immediate.  */
	uint32_t mask2 = 0x1FE00000U;
	uint32_t val2  = 0x1A400000U;
	/* 3) cond select.  */
	uint32_t mask3 = 0x1FE00000U;
	uint32_t val3  = 0x1A800000U;
	/* 4) data proc 1/2 source.  */
	uint32_t mask4 = 0x1FE00000U;
	uint32_t val4  = 0x1AC00000U;

	if ((aarch64_get_instr (cpu) & mask1) == val1)
	  dexAddSubtractWithCarry (cpu);

	else if ((aarch64_get_instr (cpu) & mask2) == val2)
	  CondCompare (cpu);

	else if ((aarch64_get_instr (cpu) & mask3) == val3)
	  dexCondSelect (cpu);

	else if ((aarch64_get_instr (cpu) & mask4) == val4)
	  {
	    /* Bit 30 is clear for data proc 2 source
	       and set for data proc 1 source.  */
	    if (aarch64_get_instr (cpu)  & (1U << 30))
	      dexDataProc1Source (cpu);
	    else
	      dexDataProc2Source (cpu);
	  }

	else
	  /* Should not reach here.  */
	  HALT_NYI;

	return;
      }

    case DPREG_3SRC_110:
      dexDataProc3Source (cpu); return;

    case DPREG_UNALLOC_101:
      HALT_UNALLOC;

    case DPREG_3SRC_111:
      dexDataProc3Source (cpu); return;

    default:
      /* Should never reach here.  */
      HALT_NYI;
    }
}

/* Unconditional Branch immediate.
   Offset is a PC-relative byte offset in the range +/- 128MiB.
   The offset is assumed to be raw from the decode i.e. the
   simulator is expected to scale them from word offsets to byte.  */

/* Unconditional branch.  */
static void
buc (sim_cpu *cpu, int32_t offset)
{
  aarch64_set_next_PC_by_offset (cpu, offset);
}

static unsigned stack_depth = 0;

/* Unconditional branch and link -- writes return PC to LR.  */
static void
bl (sim_cpu *cpu, int32_t offset)
{
  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_save_LR (cpu);
  aarch64_set_next_PC_by_offset (cpu, offset);

  if (TRACE_BRANCH_P (cpu))
    {
      ++ stack_depth;
      TRACE_BRANCH (cpu,
		    " %*scall %" PRIx64 " [%s]"
		    " [args: %" PRIx64 " %" PRIx64 " %" PRIx64 "]",
		    stack_depth, " ", aarch64_get_next_PC (cpu),
		    aarch64_get_func (CPU_STATE (cpu),
				      aarch64_get_next_PC (cpu)),
		    aarch64_get_reg_u64 (cpu, 0, NO_SP),
		    aarch64_get_reg_u64 (cpu, 1, NO_SP),
		    aarch64_get_reg_u64 (cpu, 2, NO_SP)
		    );
    }
}

/* Unconditional Branch register.
   Branch/return address is in source register.  */

/* Unconditional branch.  */
static void
br (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_next_PC (cpu, aarch64_get_reg_u64 (cpu, rn, NO_SP));
}

/* Unconditional branch and link -- writes return PC to LR.  */
static void
blr (sim_cpu *cpu)
{
  /* Ensure we read the destination before we write LR.  */
  uint64_t target = aarch64_get_reg_u64 (cpu, INSTR (9, 5), NO_SP);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_save_LR (cpu);
  aarch64_set_next_PC (cpu, target);

  if (TRACE_BRANCH_P (cpu))
    {
      ++ stack_depth;
      TRACE_BRANCH (cpu,
		    " %*scall %" PRIx64 " [%s]"
		    " [args: %" PRIx64 " %" PRIx64 " %" PRIx64 "]",
		    stack_depth, " ", aarch64_get_next_PC (cpu),
		    aarch64_get_func (CPU_STATE (cpu),
				      aarch64_get_next_PC (cpu)),
		    aarch64_get_reg_u64 (cpu, 0, NO_SP),
		    aarch64_get_reg_u64 (cpu, 1, NO_SP),
		    aarch64_get_reg_u64 (cpu, 2, NO_SP)
		    );
    }
}

/* Return -- assembler will default source to LR this is functionally
   equivalent to br but, presumably, unlike br it side effects the
   branch predictor.  */
static void
ret (sim_cpu *cpu)
{
  unsigned rn = INSTR (9, 5);
  aarch64_set_next_PC (cpu, aarch64_get_reg_u64 (cpu, rn, NO_SP));

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (TRACE_BRANCH_P (cpu))
    {
      TRACE_BRANCH (cpu,
		    " %*sreturn [result: %" PRIx64 "]",
		    stack_depth, " ", aarch64_get_reg_u64 (cpu, 0, NO_SP));
      -- stack_depth;
    }
}

/* NOP -- we implement this and call it from the decode in case we
   want to intercept it later.  */

static void
nop (sim_cpu *cpu)
{
  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
}

/* Data synchronization barrier.  */

static void
dsb (sim_cpu *cpu)
{
  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
}

/* Data memory barrier.  */

static void
dmb (sim_cpu *cpu)
{
  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
}

/* Instruction synchronization barrier.  */

static void
isb (sim_cpu *cpu)
{
  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
}

static void
dexBranchImmediate (sim_cpu *cpu)
{
  /* assert instr[30,26] == 00101
     instr[31] ==> 0 == B, 1 == BL
     instr[25,0] == imm26 branch offset counted in words.  */

  uint32_t top = INSTR (31, 31);
  /* We have a 26 byte signed word offset which we need to pass to the
     execute routine as a signed byte offset.  */
  int32_t offset = simm32 (aarch64_get_instr (cpu), 25, 0) << 2;

  if (top)
    bl (cpu, offset);
  else
    buc (cpu, offset);
}

/* Control Flow.  */

/* Conditional branch

   Offset is a PC-relative byte offset in the range +/- 1MiB pos is
   a bit position in the range 0 .. 63

   cc is a CondCode enum value as pulled out of the decode

   N.B. any offset register (source) can only be Xn or Wn.  */

static void
bcc (sim_cpu *cpu, int32_t offset, CondCode cc)
{
  /* The test returns TRUE if CC is met.  */
  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (testConditionCode (cpu, cc))
    aarch64_set_next_PC_by_offset (cpu, offset);
}

/* 32 bit branch on register non-zero.  */
static void
cbnz32 (sim_cpu *cpu, int32_t offset)
{
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (aarch64_get_reg_u32 (cpu, rt, NO_SP) != 0)
    aarch64_set_next_PC_by_offset (cpu, offset);
}

/* 64 bit branch on register zero.  */
static void
cbnz (sim_cpu *cpu, int32_t offset)
{
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (aarch64_get_reg_u64 (cpu, rt, NO_SP) != 0)
    aarch64_set_next_PC_by_offset (cpu, offset);
}

/* 32 bit branch on register non-zero.  */
static void
cbz32 (sim_cpu *cpu, int32_t offset)
{
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (aarch64_get_reg_u32 (cpu, rt, NO_SP) == 0)
    aarch64_set_next_PC_by_offset (cpu, offset);
}

/* 64 bit branch on register zero.  */
static void
cbz (sim_cpu *cpu, int32_t offset)
{
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (aarch64_get_reg_u64 (cpu, rt, NO_SP) == 0)
    aarch64_set_next_PC_by_offset (cpu, offset);
}

/* Branch on register bit test non-zero -- one size fits all.  */
static void
tbnz (sim_cpu *cpu, uint32_t  pos, int32_t offset)
{
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (aarch64_get_reg_u64 (cpu, rt, NO_SP) & (((uint64_t) 1) << pos))
    aarch64_set_next_PC_by_offset (cpu, offset);
}

/* Branch on register bit test zero -- one size fits all.  */
static void
tbz (sim_cpu *cpu, uint32_t  pos, int32_t offset)
{
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (!(aarch64_get_reg_u64 (cpu, rt, NO_SP) & (((uint64_t) 1) << pos)))
    aarch64_set_next_PC_by_offset (cpu, offset);
}

static void
dexCompareBranchImmediate (sim_cpu *cpu)
{
  /* instr[30,25] = 01 1010
     instr[31]    = size : 0 ==> 32, 1 ==> 64
     instr[24]    = op : 0 ==> CBZ, 1 ==> CBNZ
     instr[23,5]  = simm19 branch offset counted in words
     instr[4,0]   = rt  */

  uint32_t size = INSTR (31, 31);
  uint32_t op   = INSTR (24, 24);
  int32_t offset = simm32 (aarch64_get_instr (cpu), 23, 5) << 2;

  if (size == 0)
    {
      if (op == 0)
	cbz32 (cpu, offset);
      else
	cbnz32 (cpu, offset);
    }
  else
    {
      if (op == 0)
	cbz (cpu, offset);
      else
	cbnz (cpu, offset);
    }
}

static void
dexTestBranchImmediate (sim_cpu *cpu)
{
  /* instr[31]    = b5 : bit 5 of test bit idx
     instr[30,25] = 01 1011
     instr[24]    = op : 0 ==> TBZ, 1 == TBNZ
     instr[23,19] = b40 : bits 4 to 0 of test bit idx
     instr[18,5]  = simm14 : signed offset counted in words
     instr[4,0]   = uimm5  */

  uint32_t pos = ((INSTR (31, 31) << 5) | INSTR (23, 19));
  int32_t offset = simm32 (aarch64_get_instr (cpu), 18, 5) << 2;

  NYI_assert (30, 25, 0x1b);

  if (INSTR (24, 24) == 0)
    tbz (cpu, pos, offset);
  else
    tbnz (cpu, pos, offset);
}

static void
dexCondBranchImmediate (sim_cpu *cpu)
{
  /* instr[31,25] = 010 1010
     instr[24]    = op1; op => 00 ==> B.cond
     instr[23,5]  = simm19 : signed offset counted in words
     instr[4]     = op0
     instr[3,0]   = cond  */

  int32_t offset;
  uint32_t op = ((INSTR (24, 24) << 1) | INSTR (4, 4));

  NYI_assert (31, 25, 0x2a);

  if (op != 0)
    HALT_UNALLOC;

  offset = simm32 (aarch64_get_instr (cpu), 23, 5) << 2;

  bcc (cpu, offset, INSTR (3, 0));
}

static void
dexBranchRegister (sim_cpu *cpu)
{
  /* instr[31,25] = 110 1011
     instr[24,21] = op : 0 ==> BR, 1 => BLR, 2 => RET, 3 => ERET, 4 => DRPS
     instr[20,16] = op2 : must be 11111
     instr[15,10] = op3 : must be 000000
     instr[4,0]   = op2 : must be 11111.  */

  uint32_t op = INSTR (24, 21);
  uint32_t op2 = INSTR (20, 16);
  uint32_t op3 = INSTR (15, 10);
  uint32_t op4 = INSTR (4, 0);

  NYI_assert (31, 25, 0x6b);

  if (op2 != 0x1F || op3 != 0 || op4 != 0)
    HALT_UNALLOC;

  if (op == 0)
    br (cpu);

  else if (op == 1)
    blr (cpu);

  else if (op == 2)
    ret (cpu);

  else
    {
      /* ERET and DRPS accept 0b11111 for rn = instr [4,0].  */
      /* anything else is unallocated.  */
      uint32_t rn = INSTR (4, 0);

      if (rn != 0x1f)
	HALT_UNALLOC;

      if (op == 4 || op == 5)
	HALT_NYI;

      HALT_UNALLOC;
    }
}

/* FIXME: We should get the Angel SWI values from ../../libgloss/aarch64/svc.h
   but this may not be available.  So instead we define the values we need
   here.  */
#define AngelSVC_Reason_Open		0x01
#define AngelSVC_Reason_Close		0x02
#define AngelSVC_Reason_Write		0x05
#define AngelSVC_Reason_Read		0x06
#define AngelSVC_Reason_IsTTY		0x09
#define AngelSVC_Reason_Seek		0x0A
#define AngelSVC_Reason_FLen		0x0C
#define AngelSVC_Reason_Remove		0x0E
#define AngelSVC_Reason_Rename		0x0F
#define AngelSVC_Reason_Clock		0x10
#define AngelSVC_Reason_Time		0x11
#define AngelSVC_Reason_System		0x12
#define AngelSVC_Reason_Errno		0x13
#define AngelSVC_Reason_GetCmdLine	0x15
#define AngelSVC_Reason_HeapInfo	0x16
#define AngelSVC_Reason_ReportException 0x18
#define AngelSVC_Reason_Elapsed         0x30


static void
handle_halt (sim_cpu *cpu, uint32_t val)
{
  uint64_t result = 0;

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  if (val != 0xf000)
    {
      TRACE_SYSCALL (cpu, " HLT [0x%x]", val);
      sim_engine_halt (CPU_STATE (cpu), cpu, NULL, aarch64_get_PC (cpu),
		       sim_stopped, SIM_SIGTRAP);
    }

  /* We have encountered an Angel SVC call.  See if we can process it.  */
  switch (aarch64_get_reg_u32 (cpu, 0, NO_SP))
    {
    case AngelSVC_Reason_HeapInfo:
      {
	/* Get the values.  */
	uint64_t stack_top = aarch64_get_stack_start (cpu);
	uint64_t heap_base = aarch64_get_heap_start (cpu);

	/* Get the pointer  */
	uint64_t ptr = aarch64_get_reg_u64 (cpu, 1, SP_OK);
	ptr = aarch64_get_mem_u64 (cpu, ptr);

	/* Fill in the memory block.  */
	/* Start addr of heap.  */
	aarch64_set_mem_u64 (cpu, ptr +  0, heap_base);
	/* End addr of heap.  */
	aarch64_set_mem_u64 (cpu, ptr +  8, stack_top);
	/* Lowest stack addr.  */
	aarch64_set_mem_u64 (cpu, ptr + 16, heap_base);
	/* Initial stack addr.  */
	aarch64_set_mem_u64 (cpu, ptr + 24, stack_top);

	TRACE_SYSCALL (cpu, " AngelSVC: Get Heap Info");
      }
      break;

    case AngelSVC_Reason_Open:
      {
	/* Get the pointer  */
	/* uint64_t ptr = aarch64_get_reg_u64 (cpu, 1, SP_OK);.  */
	/* FIXME: For now we just assume that we will only be asked
	   to open the standard file descriptors.  */
	static int fd = 0;
	result = fd ++;

	TRACE_SYSCALL (cpu, " AngelSVC: Open file %d", fd - 1);
      }
      break;

    case AngelSVC_Reason_Close:
      {
	uint64_t fh = aarch64_get_reg_u64 (cpu, 1, SP_OK);
	TRACE_SYSCALL (cpu, " AngelSVC: Close file %d", (int) fh);
	result = 0;
      }
      break;

    case AngelSVC_Reason_Errno:
      result = 0;
      TRACE_SYSCALL (cpu, " AngelSVC: Get Errno");
      break;

    case AngelSVC_Reason_Clock:
      result =
#ifdef CLOCKS_PER_SEC
	(CLOCKS_PER_SEC >= 100)
	? (clock () / (CLOCKS_PER_SEC / 100))
	: ((clock () * 100) / CLOCKS_PER_SEC)
#else
	/* Presume unix... clock() returns microseconds.  */
	(clock () / 10000)
#endif
	;
	TRACE_SYSCALL (cpu, " AngelSVC: Get Clock");
      break;

    case AngelSVC_Reason_GetCmdLine:
      {
	/* Get the pointer  */
	uint64_t ptr = aarch64_get_reg_u64 (cpu, 1, SP_OK);
	ptr = aarch64_get_mem_u64 (cpu, ptr);

	/* FIXME: No command line for now.  */
	aarch64_set_mem_u64 (cpu, ptr, 0);
	TRACE_SYSCALL (cpu, " AngelSVC: Get Command Line");
      }
      break;

    case AngelSVC_Reason_IsTTY:
      result = 1;
	TRACE_SYSCALL (cpu, " AngelSVC: IsTTY ?");
      break;

    case AngelSVC_Reason_Write:
      {
	/* Get the pointer  */
	uint64_t ptr = aarch64_get_reg_u64 (cpu, 1, SP_OK);
	/* Get the write control block.  */
	uint64_t fd  = aarch64_get_mem_u64 (cpu, ptr);
	uint64_t buf = aarch64_get_mem_u64 (cpu, ptr + 8);
	uint64_t len = aarch64_get_mem_u64 (cpu, ptr + 16);

	TRACE_SYSCALL (cpu, "write of %" PRIx64 " bytes from %"
		       PRIx64 " on descriptor %" PRIx64,
		       len, buf, fd);

	if (len > 1280)
	  {
	    TRACE_SYSCALL (cpu,
			   " AngelSVC: Write: Suspiciously long write: %ld",
			   (long) len);
	    sim_engine_halt (CPU_STATE (cpu), cpu, NULL, aarch64_get_PC (cpu),
			     sim_stopped, SIM_SIGBUS);
	  }
	else if (fd == 1)
	  {
	    printf ("%.*s", (int) len, aarch64_get_mem_ptr (cpu, buf));
	  }
	else if (fd == 2)
	  {
	    TRACE (cpu, 0, "\n");
	    sim_io_eprintf (CPU_STATE (cpu), "%.*s",
			    (int) len, aarch64_get_mem_ptr (cpu, buf));
	    TRACE (cpu, 0, "\n");
	  }
	else
	  {
	    TRACE_SYSCALL (cpu,
			   " AngelSVC: Write: Unexpected file handle: %d",
			   (int) fd);
	    sim_engine_halt (CPU_STATE (cpu), cpu, NULL, aarch64_get_PC (cpu),
			     sim_stopped, SIM_SIGABRT);
	  }
      }
      break;

    case AngelSVC_Reason_ReportException:
      {
	/* Get the pointer  */
	uint64_t ptr = aarch64_get_reg_u64 (cpu, 1, SP_OK);
	/*ptr = aarch64_get_mem_u64 (cpu, ptr);.  */
	uint64_t type = aarch64_get_mem_u64 (cpu, ptr);
	uint64_t state = aarch64_get_mem_u64 (cpu, ptr + 8);

	TRACE_SYSCALL (cpu,
		       "Angel Exception: type 0x%" PRIx64 " state %" PRIx64,
		       type, state);

	if (type == 0x20026)
	  sim_engine_halt (CPU_STATE (cpu), cpu, NULL, aarch64_get_PC (cpu),
			   sim_exited, state);
	else
	  sim_engine_halt (CPU_STATE (cpu), cpu, NULL, aarch64_get_PC (cpu),
			   sim_stopped, SIM_SIGINT);
      }
      break;

    case AngelSVC_Reason_Read:
    case AngelSVC_Reason_FLen:
    case AngelSVC_Reason_Seek:
    case AngelSVC_Reason_Remove:
    case AngelSVC_Reason_Time:
    case AngelSVC_Reason_System:
    case AngelSVC_Reason_Rename:
    case AngelSVC_Reason_Elapsed:
    default:
      TRACE_SYSCALL (cpu, " HLT [Unknown angel %x]",
		     aarch64_get_reg_u32 (cpu, 0, NO_SP));
      sim_engine_halt (CPU_STATE (cpu), cpu, NULL, aarch64_get_PC (cpu),
		       sim_stopped, SIM_SIGTRAP);
    }

  aarch64_set_reg_u64 (cpu, 0, NO_SP, result);
}

static void
dexExcpnGen (sim_cpu *cpu)
{
  /* instr[31:24] = 11010100
     instr[23,21] = opc : 000 ==> GEN EXCPN, 001 ==> BRK
                          010 ==> HLT,       101 ==> DBG GEN EXCPN
     instr[20,5]  = imm16
     instr[4,2]   = opc2 000 ==> OK, ow ==> UNALLOC
     instr[1,0]   = LL : discriminates opc  */

  uint32_t opc = INSTR (23, 21);
  uint32_t imm16 = INSTR (20, 5);
  uint32_t opc2 = INSTR (4, 2);
  uint32_t LL;

  NYI_assert (31, 24, 0xd4);

  if (opc2 != 0)
    HALT_UNALLOC;

  LL = INSTR (1, 0);

  /* We only implement HLT and BRK for now.  */
  if (opc == 1 && LL == 0)
    {
      TRACE_EVENTS (cpu, " BRK [0x%x]", imm16);
      sim_engine_halt (CPU_STATE (cpu), cpu, NULL, aarch64_get_PC (cpu),
		       sim_exited, aarch64_get_reg_s32 (cpu, R0, SP_OK));
    }

  if (opc == 2 && LL == 0)
    handle_halt (cpu, imm16);

  else if (opc == 0 || opc == 5)
    HALT_NYI;

  else
    HALT_UNALLOC;
}

/* Stub for accessing system registers.  */

static uint64_t
system_get (sim_cpu *cpu, unsigned op0, unsigned op1, unsigned crn,
	    unsigned crm, unsigned op2)
{
  if (crn == 0 && op1 == 3 && crm == 0 && op2 == 7)
    /* DCZID_EL0 - the Data Cache Zero ID register.
       We do not support DC ZVA at the moment, so
       we return a value with the disable bit set.
       We implement support for the DCZID register since
       it is used by the C library's memset function.  */
    return ((uint64_t) 1) << 4;

  if (crn == 0 && op1 == 3 && crm == 0 && op2 == 1)
    /* Cache Type Register.  */
    return 0x80008000UL;

  if (crn == 13 && op1 == 3 && crm == 0 && op2 == 2)
    /* TPIDR_EL0 - thread pointer id.  */
    return aarch64_get_thread_id (cpu);

  if (op1 == 3 && crm == 4 && op2 == 0)
    return aarch64_get_FPCR (cpu);

  if (op1 == 3 && crm == 4 && op2 == 1)
    return aarch64_get_FPSR (cpu);

  else if (op1 == 3 && crm == 2 && op2 == 0)
    return aarch64_get_CPSR (cpu);

  HALT_NYI;
}

static void
system_set (sim_cpu *cpu, unsigned op0, unsigned op1, unsigned crn,
	    unsigned crm, unsigned op2, uint64_t val)
{
  if (op1 == 3 && crm == 4 && op2 == 0)
    aarch64_set_FPCR (cpu, val);

  else if (op1 == 3 && crm == 4 && op2 == 1)
    aarch64_set_FPSR (cpu, val);

  else if (op1 == 3 && crm == 2 && op2 == 0)
    aarch64_set_CPSR (cpu, val);

  else
    HALT_NYI;
}

static void
do_mrs (sim_cpu *cpu)
{
  /* instr[31:20] = 1101 0101 0001 1
     instr[19]    = op0
     instr[18,16] = op1
     instr[15,12] = CRn
     instr[11,8]  = CRm
     instr[7,5]   = op2
     instr[4,0]   = Rt  */
  unsigned sys_op0 = INSTR (19, 19) + 2;
  unsigned sys_op1 = INSTR (18, 16);
  unsigned sys_crn = INSTR (15, 12);
  unsigned sys_crm = INSTR (11, 8);
  unsigned sys_op2 = INSTR (7, 5);
  unsigned rt = INSTR (4, 0);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  aarch64_set_reg_u64 (cpu, rt, NO_SP,
		       system_get (cpu, sys_op0, sys_op1, sys_crn, sys_crm, sys_op2));
}

static void
do_MSR_immediate (sim_cpu *cpu)
{
  /* instr[31:19] = 1101 0101 0000 0
     instr[18,16] = op1
     instr[15,12] = 0100
     instr[11,8]  = CRm
     instr[7,5]   = op2
     instr[4,0]   = 1 1111  */

  unsigned op1 = INSTR (18, 16);
  /*unsigned crm = INSTR (11, 8);*/
  unsigned op2 = INSTR (7, 5);

  NYI_assert (31, 19, 0x1AA0);
  NYI_assert (15, 12, 0x4);
  NYI_assert (4,  0,  0x1F);

  if (op1 == 0)
    {
      if (op2 == 5)
	HALT_NYI; /* set SPSel.  */
      else
	HALT_UNALLOC;
    }
  else if (op1 == 3)
    {
      if (op2 == 6)
	HALT_NYI; /* set DAIFset.  */
      else if (op2 == 7)
	HALT_NYI; /* set DAIFclr.  */
      else
	HALT_UNALLOC;
    }
  else
    HALT_UNALLOC;
}

static void
do_MSR_reg (sim_cpu *cpu)
{
  /* instr[31:20] = 1101 0101 0001
     instr[19]    = op0
     instr[18,16] = op1
     instr[15,12] = CRn
     instr[11,8]  = CRm
     instr[7,5]   = op2
     instr[4,0]   = Rt  */

  unsigned sys_op0 = INSTR (19, 19) + 2;
  unsigned sys_op1 = INSTR (18, 16);
  unsigned sys_crn = INSTR (15, 12);
  unsigned sys_crm = INSTR (11, 8);
  unsigned sys_op2 = INSTR (7, 5);
  unsigned rt = INSTR (4, 0);

  NYI_assert (31, 20, 0xD51);

  TRACE_DECODE (cpu, "emulated at line %d", __LINE__);
  system_set (cpu, sys_op0, sys_op1, sys_crn, sys_crm, sys_op2,
	      aarch64_get_reg_u64 (cpu, rt, NO_SP));
}

static void
do_SYS (sim_cpu *cpu)
{
  /* instr[31,19] = 1101 0101 0000 1
     instr[18,16] = op1
     instr[15,12] = CRn
     instr[11,8]  = CRm
     instr[7,5]   = op2
     instr[4,0]   = Rt  */
  NYI_assert (31, 19, 0x1AA1);

  /* FIXME: For now we just silently accept system ops.  */
}

static void
dexSystem (sim_cpu *cpu)
{
  /* instr[31:22] = 1101 01010 0
     instr[21]    = L
     instr[20,19] = op0
     instr[18,16] = op1
     instr[15,12] = CRn
     instr[11,8]  = CRm
     instr[7,5]   = op2
     instr[4,0]   = uimm5  */

  /* We are interested in HINT, DSB, DMB and ISB

     Hint #0 encodes NOOP (this is the only hint we care about)
     L == 0, op0 == 0, op1 = 011, CRn = 0010, Rt = 11111,
     CRm op2  != 0000 000 OR CRm op2 == 0000 000 || CRm op > 0000 101

     DSB, DMB, ISB are data store barrier, data memory barrier and
     instruction store barrier, respectively, where

     L == 0, op0 == 0, op1 = 011, CRn = 0011, Rt = 11111,
     op2 : DSB ==> 100, DMB ==> 101, ISB ==> 110
     CRm<3:2> ==> domain, CRm<1:0> ==> types,
     domain : 00 ==> OuterShareable, 01 ==> Nonshareable,
              10 ==> InerShareable, 11 ==> FullSystem
     types :  01 ==> Reads, 10 ==> Writes,
              11 ==> All, 00 ==> All (domain == FullSystem).  */

  unsigned rt = INSTR (4, 0);

  NYI_assert (31, 22, 0x354);

  switch (INSTR (21, 12))
    {
    case 0x032:
      if (rt == 0x1F)
	{
	  /* NOP has CRm != 0000 OR.  */
	  /*         (CRm == 0000 AND (op2 == 000 OR op2 > 101)).  */
	  uint32_t crm = INSTR (11, 8);
	  uint32_t op2 = INSTR (7, 5);

	  if (crm != 0 || (op2 == 0 || op2 > 5))
	    {
	      /* Actually call nop method so we can reimplement it later.  */
	      nop (cpu);
	      return;
	    }
	}
      HALT_NYI;

    case 0x033:
      {
	uint32_t op2 =  INSTR (7, 5);

	switch (op2)
	  {
	  case 2: HALT_NYI;
	  case 4: dsb (cpu); return;
	  case 5: dmb (cpu); return;
	  case 6: isb (cpu); return;
	  default: HALT_UNALLOC;
	}
      }

    case 0x3B0:
    case 0x3B4:
    case 0x3BD:
      do_mrs (cpu);
      return;

    case 0x0B7:
      do_SYS (cpu); /* DC is an alias of SYS.  */
      return;

    default:
      if (INSTR (21, 20) == 0x1)
	do_MSR_reg (cpu);
      else if (INSTR (21, 19) == 0 && INSTR (15, 12) == 0x4)
	do_MSR_immediate (cpu);
      else
	HALT_NYI;
      return;
    }
}

static void
dexBr (sim_cpu *cpu)
{
  /* uint32_t group = dispatchGroup (aarch64_get_instr (cpu));
     assert  group == GROUP_BREXSYS_1010 || group == GROUP_BREXSYS_1011
     bits [31,29] of a BrExSys are the secondary dispatch vector.  */
  uint32_t group2 = dispatchBrExSys (aarch64_get_instr (cpu));

  switch (group2)
    {
    case BR_IMM_000:
      return dexBranchImmediate (cpu);

    case BR_IMMCMP_001:
      /* Compare has bit 25 clear while test has it set.  */
      if (!INSTR (25, 25))
	dexCompareBranchImmediate (cpu);
      else
	dexTestBranchImmediate (cpu);
      return;

    case BR_IMMCOND_010:
      /* This is a conditional branch if bit 25 is clear otherwise
         unallocated.  */
      if (!INSTR (25, 25))
	dexCondBranchImmediate (cpu);
      else
	HALT_UNALLOC;
      return;

    case BR_UNALLOC_011:
      HALT_UNALLOC;

    case BR_IMM_100:
      dexBranchImmediate (cpu);
      return;

    case BR_IMMCMP_101:
      /* Compare has bit 25 clear while test has it set.  */
      if (!INSTR (25, 25))
	dexCompareBranchImmediate (cpu);
      else
	dexTestBranchImmediate (cpu);
      return;

    case BR_REG_110:
      /* Unconditional branch reg has bit 25 set.  */
      if (INSTR (25, 25))
	dexBranchRegister (cpu);

      /* This includes both Excpn Gen, System and unalloc operations.
         We need to decode the Excpn Gen operation BRK so we can plant
         debugger entry points.
         Excpn Gen operations have instr [24] = 0.
         we need to decode at least one of the System operations NOP
         which is an alias for HINT #0.
         System operations have instr [24,22] = 100.  */
      else if (INSTR (24, 24) == 0)
	dexExcpnGen (cpu);

      else if (INSTR (24, 22) == 4)
	dexSystem (cpu);

      else
	HALT_UNALLOC;

      return;

    case BR_UNALLOC_111:
      HALT_UNALLOC;

    default:
      /* Should never reach here.  */
      HALT_NYI;
    }
}

static void
aarch64_decode_and_execute (sim_cpu *cpu, uint64_t pc)
{
  /* We need to check if gdb wants an in here.  */
  /* checkBreak (cpu);.  */

  uint64_t group = dispatchGroup (aarch64_get_instr (cpu));

  switch (group)
    {
    case GROUP_PSEUDO_0000:   dexPseudo (cpu); break;
    case GROUP_LDST_0100:     dexLdSt (cpu); break;
    case GROUP_DPREG_0101:    dexDPReg (cpu); break;
    case GROUP_LDST_0110:     dexLdSt (cpu); break;
    case GROUP_ADVSIMD_0111:  dexAdvSIMD0 (cpu); break;
    case GROUP_DPIMM_1000:    dexDPImm (cpu); break;
    case GROUP_DPIMM_1001:    dexDPImm (cpu); break;
    case GROUP_BREXSYS_1010:  dexBr (cpu); break;
    case GROUP_BREXSYS_1011:  dexBr (cpu); break;
    case GROUP_LDST_1100:     dexLdSt (cpu); break;
    case GROUP_DPREG_1101:    dexDPReg (cpu); break;
    case GROUP_LDST_1110:     dexLdSt (cpu); break;
    case GROUP_ADVSIMD_1111:  dexAdvSIMD1 (cpu); break;

    case GROUP_UNALLOC_0001:
    case GROUP_UNALLOC_0010:
    case GROUP_UNALLOC_0011:
      HALT_UNALLOC;

    default:
      /* Should never reach here.  */
      HALT_NYI;
    }
}

static bfd_boolean
aarch64_step (sim_cpu *cpu)
{
  uint64_t pc = aarch64_get_PC (cpu);

  if (pc == TOP_LEVEL_RETURN_PC)
    return FALSE;

  aarch64_set_next_PC (cpu, pc + 4);

  /* Code is always little-endian.  */
  sim_core_read_buffer (CPU_STATE (cpu), cpu, read_map,
			& aarch64_get_instr (cpu), pc, 4);
  aarch64_get_instr (cpu) = endian_le2h_4 (aarch64_get_instr (cpu));

  TRACE_INSN (cpu, " pc = %" PRIx64 " instr = %08x", pc,
	      aarch64_get_instr (cpu));
  TRACE_DISASM (cpu, pc);

  aarch64_decode_and_execute (cpu, pc);

  return TRUE;
}

void
aarch64_run (SIM_DESC sd)
{
  sim_cpu *cpu = STATE_CPU (sd, 0);

  while (aarch64_step (cpu))
    {
      aarch64_update_PC (cpu);

      if (sim_events_tick (sd))
	sim_events_process (sd);
    }

  sim_engine_halt (sd, cpu, NULL, aarch64_get_PC (cpu),
		   sim_exited, aarch64_get_reg_s32 (cpu, R0, NO_SP));
}

void
aarch64_init (sim_cpu *cpu, uint64_t pc)
{
  uint64_t sp = aarch64_get_stack_start (cpu);

  /* Install SP, FP and PC and set LR to -20
     so we can detect a top-level return.  */
  aarch64_set_reg_u64 (cpu, SP, SP_OK, sp);
  aarch64_set_reg_u64 (cpu, FP, SP_OK, sp);
  aarch64_set_reg_u64 (cpu, LR, SP_OK, TOP_LEVEL_RETURN_PC);
  aarch64_set_next_PC (cpu, pc);
  aarch64_update_PC (cpu);
  aarch64_init_LIT_table ();
}
