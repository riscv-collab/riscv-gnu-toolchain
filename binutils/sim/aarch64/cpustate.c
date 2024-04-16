/* cpustate.h -- Prototypes for AArch64 simulator functions.

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

#include <stdio.h>
#include <math.h>

#include "sim-main.h"
#include "sim-signal.h"
#include "cpustate.h"
#include "simulator.h"
#include "libiberty.h"

#include "aarch64-sim.h"

/* Some operands are allowed to access the stack pointer (reg 31).
   For others a read from r31 always returns 0, and a write to r31 is ignored.  */
#define reg_num(reg) (((reg) == R31 && !r31_is_sp) ? 32 : (reg))

void
aarch64_set_reg_u64 (sim_cpu *cpu, GReg reg, int r31_is_sp, uint64_t val)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);

  if (reg == R31 && ! r31_is_sp)
    {
      TRACE_REGISTER (cpu, "GR[31] NOT CHANGED!");
      return;
    }

  if (val != aarch64_cpu->gr[reg].u64)
    TRACE_REGISTER (cpu,
		    "GR[%2d] changes from %16" PRIx64 " to %16" PRIx64,
		    reg, aarch64_cpu->gr[reg].u64, val);

  aarch64_cpu->gr[reg].u64 = val;
}

void
aarch64_set_reg_s64 (sim_cpu *cpu, GReg reg, int r31_is_sp, int64_t val)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);

  if (reg == R31 && ! r31_is_sp)
    {
      TRACE_REGISTER (cpu, "GR[31] NOT CHANGED!");
      return;
    }

  if (val != aarch64_cpu->gr[reg].s64)
    TRACE_REGISTER (cpu,
		    "GR[%2d] changes from %16" PRIx64 " to %16" PRIx64,
		    reg, aarch64_cpu->gr[reg].s64, val);

  aarch64_cpu->gr[reg].s64 = val;
}

uint64_t
aarch64_get_reg_u64 (sim_cpu *cpu, GReg reg, int r31_is_sp)
{
  return AARCH64_SIM_CPU (cpu)->gr[reg_num(reg)].u64;
}

int64_t
aarch64_get_reg_s64 (sim_cpu *cpu, GReg reg, int r31_is_sp)
{
  return AARCH64_SIM_CPU (cpu)->gr[reg_num(reg)].s64;
}

uint32_t
aarch64_get_reg_u32 (sim_cpu *cpu, GReg reg, int r31_is_sp)
{
  return AARCH64_SIM_CPU (cpu)->gr[reg_num(reg)].u32;
}

int32_t
aarch64_get_reg_s32 (sim_cpu *cpu, GReg reg, int r31_is_sp)
{
  return AARCH64_SIM_CPU (cpu)->gr[reg_num(reg)].s32;
}

void
aarch64_set_reg_s32 (sim_cpu *cpu, GReg reg, int r31_is_sp, int32_t val)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);

  if (reg == R31 && ! r31_is_sp)
    {
      TRACE_REGISTER (cpu, "GR[31] NOT CHANGED!");
      return;
    }

  if (val != aarch64_cpu->gr[reg].s32)
    TRACE_REGISTER (cpu, "GR[%2d] changes from %8x to %8x",
		    reg, aarch64_cpu->gr[reg].s32, val);

  /* The ARM ARM states that (C1.2.4):
        When the data size is 32 bits, the lower 32 bits of the
	register are used and the upper 32 bits are ignored on
	a read and cleared to zero on a write.
     We simulate this by first clearing the whole 64-bits and
     then writing to the 32-bit value in the GRegister union.  */
  aarch64_cpu->gr[reg].s64 = 0;
  aarch64_cpu->gr[reg].s32 = val;
}

void
aarch64_set_reg_u32 (sim_cpu *cpu, GReg reg, int r31_is_sp, uint32_t val)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);

  if (reg == R31 && ! r31_is_sp)
    {
      TRACE_REGISTER (cpu, "GR[31] NOT CHANGED!");
      return;
    }

  if (val != aarch64_cpu->gr[reg].u32)
    TRACE_REGISTER (cpu, "GR[%2d] changes from %8x to %8x",
		    reg, aarch64_cpu->gr[reg].u32, val);

  aarch64_cpu->gr[reg].u64 = 0;
  aarch64_cpu->gr[reg].u32 = val;
}

uint32_t
aarch64_get_reg_u16 (sim_cpu *cpu, GReg reg, int r31_is_sp)
{
  return AARCH64_SIM_CPU (cpu)->gr[reg_num(reg)].u16;
}

int32_t
aarch64_get_reg_s16 (sim_cpu *cpu, GReg reg, int r31_is_sp)
{
  return AARCH64_SIM_CPU (cpu)->gr[reg_num(reg)].s16;
}

uint32_t
aarch64_get_reg_u8 (sim_cpu *cpu, GReg reg, int r31_is_sp)
{
  return AARCH64_SIM_CPU (cpu)->gr[reg_num(reg)].u8;
}

int32_t
aarch64_get_reg_s8 (sim_cpu *cpu, GReg reg, int r31_is_sp)
{
  return AARCH64_SIM_CPU (cpu)->gr[reg_num(reg)].s8;
}

uint64_t
aarch64_get_PC (sim_cpu *cpu)
{
  return AARCH64_SIM_CPU (cpu)->pc;
}

uint64_t
aarch64_get_next_PC (sim_cpu *cpu)
{
  return AARCH64_SIM_CPU (cpu)->nextpc;
}

void
aarch64_set_next_PC (sim_cpu *cpu, uint64_t next)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);

  if (next != aarch64_cpu->nextpc + 4)
    TRACE_REGISTER (cpu,
		    "NextPC changes from %16" PRIx64 " to %16" PRIx64,
		    aarch64_cpu->nextpc, next);

  aarch64_cpu->nextpc = next;
}

void
aarch64_set_next_PC_by_offset (sim_cpu *cpu, int64_t offset)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);

  if (aarch64_cpu->pc + offset != aarch64_cpu->nextpc + 4)
    TRACE_REGISTER (cpu,
		    "NextPC changes from %16" PRIx64 " to %16" PRIx64,
		    aarch64_cpu->nextpc, aarch64_cpu->pc + offset);

  aarch64_cpu->nextpc = aarch64_cpu->pc + offset;
}

/* Install nextpc as current pc.  */
void
aarch64_update_PC (sim_cpu *cpu)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);

  aarch64_cpu->pc = aarch64_cpu->nextpc;
  /* Rezero the register we hand out when asked for ZR just in case it
     was used as the destination for a write by the previous
     instruction.  */
  aarch64_cpu->gr[32].u64 = 0UL;
}

/* This instruction can be used to save the next PC to LR
   just before installing a branch PC.  */
void
aarch64_save_LR (sim_cpu *cpu)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);

  if (aarch64_cpu->gr[LR].u64 != aarch64_cpu->nextpc)
    TRACE_REGISTER (cpu,
		    "LR    changes from %16" PRIx64 " to %16" PRIx64,
		    aarch64_cpu->gr[LR].u64, aarch64_cpu->nextpc);

  aarch64_cpu->gr[LR].u64 = aarch64_cpu->nextpc;
}

static const char *
decode_cpsr (FlagMask flags)
{
  switch (flags & CPSR_ALL_FLAGS)
    {
    default:
    case 0:  return "----";
    case 1:  return "---V";
    case 2:  return "--C-";
    case 3:  return "--CV";
    case 4:  return "-Z--";
    case 5:  return "-Z-V";
    case 6:  return "-ZC-";
    case 7:  return "-ZCV";
    case 8:  return "N---";
    case 9:  return "N--V";
    case 10: return "N-C-";
    case 11: return "N-CV";
    case 12: return "NZ--";
    case 13: return "NZ-V";
    case 14: return "NZC-";
    case 15: return "NZCV";
    }
}

/* Retrieve the CPSR register as an int.  */
uint32_t
aarch64_get_CPSR (sim_cpu *cpu)
{
  return AARCH64_SIM_CPU (cpu)->CPSR;
}

/* Set the CPSR register as an int.  */
void
aarch64_set_CPSR (sim_cpu *cpu, uint32_t new_flags)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);

  if (TRACE_REGISTER_P (cpu))
    {
      if (aarch64_cpu->CPSR != new_flags)
	TRACE_REGISTER (cpu,
			"CPSR changes from %s to %s",
			decode_cpsr (aarch64_cpu->CPSR), decode_cpsr (new_flags));
      else
	TRACE_REGISTER (cpu,
			"CPSR stays at %s", decode_cpsr (aarch64_cpu->CPSR));
    }

  aarch64_cpu->CPSR = new_flags & CPSR_ALL_FLAGS;
}

/* Read a specific subset of the CPSR as a bit pattern.  */
uint32_t
aarch64_get_CPSR_bits (sim_cpu *cpu, FlagMask mask)
{
  return AARCH64_SIM_CPU (cpu)->CPSR & mask;
}

/* Assign a specific subset of the CPSR as a bit pattern.  */
void
aarch64_set_CPSR_bits (sim_cpu *cpu, uint32_t mask, uint32_t value)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);
  uint32_t old_flags = aarch64_cpu->CPSR;

  mask &= CPSR_ALL_FLAGS;
  aarch64_cpu->CPSR &= ~ mask;
  aarch64_cpu->CPSR |= (value & mask);

  if (old_flags != aarch64_cpu->CPSR)
    TRACE_REGISTER (cpu,
		    "CPSR changes from %s to %s",
		    decode_cpsr (old_flags), decode_cpsr (aarch64_cpu->CPSR));
}

/* Test the value of a single CPSR returned as non-zero or zero.  */
uint32_t
aarch64_test_CPSR_bit (sim_cpu *cpu, FlagMask bit)
{
  return AARCH64_SIM_CPU (cpu)->CPSR & bit;
}

/* Set a single flag in the CPSR.  */
void
aarch64_set_CPSR_bit (sim_cpu *cpu, FlagMask bit)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);
  uint32_t old_flags = aarch64_cpu->CPSR;

  aarch64_cpu->CPSR |= (bit & CPSR_ALL_FLAGS);

  if (old_flags != aarch64_cpu->CPSR)
    TRACE_REGISTER (cpu,
		    "CPSR changes from %s to %s",
		    decode_cpsr (old_flags), decode_cpsr (aarch64_cpu->CPSR));
}

/* Clear a single flag in the CPSR.  */
void
aarch64_clear_CPSR_bit (sim_cpu *cpu, FlagMask bit)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);
  uint32_t old_flags = aarch64_cpu->CPSR;

  aarch64_cpu->CPSR &= ~(bit & CPSR_ALL_FLAGS);

  if (old_flags != aarch64_cpu->CPSR)
    TRACE_REGISTER (cpu,
		    "CPSR changes from %s to %s",
		    decode_cpsr (old_flags), decode_cpsr (aarch64_cpu->CPSR));
}

float
aarch64_get_FP_half (sim_cpu *cpu, VReg reg)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);
  union
  {
    uint16_t h[2];
    float    f;
  } u;

  u.h[0] = 0;
  u.h[1] = aarch64_cpu->fr[reg].h[0];
  return u.f;
}


float
aarch64_get_FP_float (sim_cpu *cpu, VReg reg)
{
  return AARCH64_SIM_CPU (cpu)->fr[reg].s;
}

double
aarch64_get_FP_double (sim_cpu *cpu, VReg reg)
{
  return AARCH64_SIM_CPU (cpu)->fr[reg].d;
}

void
aarch64_get_FP_long_double (sim_cpu *cpu, VReg reg, FRegister *a)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);

  a->v[0] = aarch64_cpu->fr[reg].v[0];
  a->v[1] = aarch64_cpu->fr[reg].v[1];
}

void
aarch64_set_FP_half (sim_cpu *cpu, VReg reg, float val)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);
  union
  {
    uint16_t h[2];
    float    f;
  } u;

  u.f = val;
  aarch64_cpu->fr[reg].h[0] = u.h[1];
  aarch64_cpu->fr[reg].h[1] = 0;
}


void
aarch64_set_FP_float (sim_cpu *cpu, VReg reg, float val)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);

  if (val != aarch64_cpu->fr[reg].s
      /* Handle +/- zero.  */
      || signbit (val) != signbit (aarch64_cpu->fr[reg].s))
    {
      FRegister v;

      v.s = val;
      TRACE_REGISTER (cpu,
		      "FR[%d].s changes from %f to %f [hex: %0" PRIx64 "]",
		      reg, aarch64_cpu->fr[reg].s, val, v.v[0]);
    }

  aarch64_cpu->fr[reg].s = val;
}

void
aarch64_set_FP_double (sim_cpu *cpu, VReg reg, double val)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);

  if (val != aarch64_cpu->fr[reg].d
      /* Handle +/- zero.  */
      || signbit (val) != signbit (aarch64_cpu->fr[reg].d))
    {
      FRegister v;

      v.d = val;
      TRACE_REGISTER (cpu,
		      "FR[%d].d changes from %f to %f [hex: %0" PRIx64 "]",
		      reg, aarch64_cpu->fr[reg].d, val, v.v[0]);
    }
  aarch64_cpu->fr[reg].d = val;
}

void
aarch64_set_FP_long_double (sim_cpu *cpu, VReg reg, FRegister a)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);

  if (aarch64_cpu->fr[reg].v[0] != a.v[0]
      || aarch64_cpu->fr[reg].v[1] != a.v[1])
    TRACE_REGISTER (cpu,
		    "FR[%d].q changes from [%0" PRIx64 " %0" PRIx64 "] to [%0"
		    PRIx64 " %0" PRIx64 "] ",
		    reg,
		    aarch64_cpu->fr[reg].v[0], aarch64_cpu->fr[reg].v[1],
		    a.v[0], a.v[1]);

  aarch64_cpu->fr[reg].v[0] = a.v[0];
  aarch64_cpu->fr[reg].v[1] = a.v[1];
}

#define GET_VEC_ELEMENT(REG, ELEMENT, FIELD)				\
  do									\
    {									\
      struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);	\
									\
      if (ELEMENT >= ARRAY_SIZE (aarch64_cpu->fr[0].FIELD))		\
	{								\
	  TRACE_REGISTER (cpu,						\
			  "Internal SIM error: invalid element number: %d ",\
			  ELEMENT);					\
	  sim_engine_halt (CPU_STATE (cpu), cpu, NULL, aarch64_get_PC (cpu), \
			   sim_stopped, SIM_SIGBUS);			\
	}								\
      return aarch64_cpu->fr[REG].FIELD [ELEMENT];			\
    }									\
  while (0)

uint64_t
aarch64_get_vec_u64 (sim_cpu *cpu, VReg reg, unsigned element)
{
  GET_VEC_ELEMENT (reg, element, v);
}

uint32_t
aarch64_get_vec_u32 (sim_cpu *cpu, VReg reg, unsigned element)
{
  GET_VEC_ELEMENT (reg, element, w);
}

uint16_t
aarch64_get_vec_u16 (sim_cpu *cpu, VReg reg, unsigned element)
{
  GET_VEC_ELEMENT (reg, element, h);
}

uint8_t
aarch64_get_vec_u8 (sim_cpu *cpu, VReg reg, unsigned element)
{
  GET_VEC_ELEMENT (reg, element, b);
}

int64_t
aarch64_get_vec_s64 (sim_cpu *cpu, VReg reg, unsigned element)
{
  GET_VEC_ELEMENT (reg, element, V);
}

int32_t
aarch64_get_vec_s32 (sim_cpu *cpu, VReg reg, unsigned element)
{
  GET_VEC_ELEMENT (reg, element, W);
}

int16_t
aarch64_get_vec_s16 (sim_cpu *cpu, VReg reg, unsigned element)
{
  GET_VEC_ELEMENT (reg, element, H);
}

int8_t
aarch64_get_vec_s8 (sim_cpu *cpu, VReg reg, unsigned element)
{
  GET_VEC_ELEMENT (reg, element, B);
}

float
aarch64_get_vec_float (sim_cpu *cpu, VReg reg, unsigned element)
{
  GET_VEC_ELEMENT (reg, element, S);
}

double
aarch64_get_vec_double (sim_cpu *cpu, VReg reg, unsigned element)
{
  GET_VEC_ELEMENT (reg, element, D);
}


#define SET_VEC_ELEMENT(REG, ELEMENT, VAL, FIELD, PRINTER)		\
  do									\
    {									\
      struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);	\
									\
      if (ELEMENT >= ARRAY_SIZE (aarch64_cpu->fr[0].FIELD))		\
	{								\
	  TRACE_REGISTER (cpu,						\
			  "Internal SIM error: invalid element number: %d ",\
			  ELEMENT);					\
	  sim_engine_halt (CPU_STATE (cpu), cpu, NULL, aarch64_get_PC (cpu), \
			   sim_stopped, SIM_SIGBUS);			\
	}								\
      if (VAL != aarch64_cpu->fr[REG].FIELD [ELEMENT])			\
	TRACE_REGISTER (cpu,						\
			"VR[%2d]." #FIELD " [%d] changes from " PRINTER \
			" to " PRINTER , REG,				\
			ELEMENT, aarch64_cpu->fr[REG].FIELD [ELEMENT], VAL); \
									\
      aarch64_cpu->fr[REG].FIELD [ELEMENT] = VAL;				\
    }									\
  while (0)

void
aarch64_set_vec_u64 (sim_cpu *cpu, VReg reg, unsigned element, uint64_t val)
{
  SET_VEC_ELEMENT (reg, element, val, v, "%16" PRIx64);
}

void
aarch64_set_vec_u32 (sim_cpu *cpu, VReg reg, unsigned element, uint32_t val)
{
  SET_VEC_ELEMENT (reg, element, val, w, "%8x");
}

void
aarch64_set_vec_u16 (sim_cpu *cpu, VReg reg, unsigned element, uint16_t val)
{
  SET_VEC_ELEMENT (reg, element, val, h, "%4x");
}

void
aarch64_set_vec_u8 (sim_cpu *cpu, VReg reg, unsigned element, uint8_t val)
{
  SET_VEC_ELEMENT (reg, element, val, b, "%x");
}

void
aarch64_set_vec_s64 (sim_cpu *cpu, VReg reg, unsigned element, int64_t val)
{
  SET_VEC_ELEMENT (reg, element, val, V, "%16" PRIx64);
}

void
aarch64_set_vec_s32 (sim_cpu *cpu, VReg reg, unsigned element, int32_t val)
{
  SET_VEC_ELEMENT (reg, element, val, W, "%8x");
}

void
aarch64_set_vec_s16 (sim_cpu *cpu, VReg reg, unsigned element, int16_t val)
{
  SET_VEC_ELEMENT (reg, element, val, H, "%4x");
}

void
aarch64_set_vec_s8 (sim_cpu *cpu, VReg reg, unsigned element, int8_t val)
{
  SET_VEC_ELEMENT (reg, element, val, B, "%x");
}

void
aarch64_set_vec_float (sim_cpu *cpu, VReg reg, unsigned element, float val)
{
  SET_VEC_ELEMENT (reg, element, val, S, "%f");
}

void
aarch64_set_vec_double (sim_cpu *cpu, VReg reg, unsigned element, double val)
{
  SET_VEC_ELEMENT (reg, element, val, D, "%f");
}

void
aarch64_set_FPSR (sim_cpu *cpu, uint32_t value)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);

  if (aarch64_cpu->FPSR != value)
    TRACE_REGISTER (cpu,
		    "FPSR changes from %x to %x", aarch64_cpu->FPSR, value);

  aarch64_cpu->FPSR = value & FPSR_ALL_FPSRS;
}

uint32_t
aarch64_get_FPSR (sim_cpu *cpu)
{
  return AARCH64_SIM_CPU (cpu)->FPSR;
}

void
aarch64_set_FPSR_bits (sim_cpu *cpu, uint32_t mask, uint32_t value)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);
  uint32_t old_FPSR = aarch64_cpu->FPSR;

  mask &= FPSR_ALL_FPSRS;
  aarch64_cpu->FPSR &= ~mask;
  aarch64_cpu->FPSR |= (value & mask);

  if (aarch64_cpu->FPSR != old_FPSR)
    TRACE_REGISTER (cpu,
		    "FPSR changes from %x to %x", old_FPSR, aarch64_cpu->FPSR);
}

uint32_t
aarch64_get_FPSR_bits (sim_cpu *cpu, uint32_t mask)
{
  mask &= FPSR_ALL_FPSRS;
  return AARCH64_SIM_CPU (cpu)->FPSR & mask;
}

int
aarch64_test_FPSR_bit (sim_cpu *cpu, FPSRMask flag)
{
  return AARCH64_SIM_CPU (cpu)->FPSR & flag;
}

uint64_t
aarch64_get_thread_id (sim_cpu *cpu)
{
  return AARCH64_SIM_CPU (cpu)->tpidr;
}

uint32_t
aarch64_get_FPCR (sim_cpu *cpu)
{
  return AARCH64_SIM_CPU (cpu)->FPCR;
}

void
aarch64_set_FPCR (sim_cpu *cpu, uint32_t val)
{
  struct aarch64_sim_cpu *aarch64_cpu = AARCH64_SIM_CPU (cpu);

  if (aarch64_cpu->FPCR != val)
    TRACE_REGISTER (cpu,
		    "FPCR changes from %x to %x", aarch64_cpu->FPCR, val);
  aarch64_cpu->FPCR = val;
}
