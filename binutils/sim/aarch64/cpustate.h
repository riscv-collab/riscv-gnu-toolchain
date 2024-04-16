/* cpustate.h -- Prototypes for AArch64 cpu state functions.

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

#ifndef _CPU_STATE_H
#define _CPU_STATE_H

#include <sys/types.h>
#include <stdint.h>
#include <inttypes.h>

#include "sim/sim.h"
#include "sim-main.h"

/* Symbolic names used to identify general registers which also match
   the registers indices in machine code.

   We have 32 general registers which can be read/written as 32 bit or
   64 bit sources/sinks and are appropriately referred to as Wn or Xn
   in the assembly code.  Some instructions mix these access modes
   (e.g. ADD X0, X1, W2) so the implementation of the instruction
   needs to *know* which type of read or write access is required.  */
typedef enum GReg
{
  R0,
  R1,
  R2,
  R3,
  R4,
  R5,
  R6,
  R7,
  R8,
  R9,
  R10,
  R11,
  R12,
  R13,
  R14,
  R15,
  R16,
  R17,
  R18,
  R19,
  R20,
  R21,
  R22,
  R23,
  R24,
  R25,
  R26,
  R27,
  R28,
  R29,
  R30,
  R31,
  FP = R29,
  LR = R30,
  SP = R31,
  ZR = R31
} GReg;

/* Symbolic names used to refer to floating point registers which also
   match the registers indices in machine code.

   We have 32 FP registers which can be read/written as 8, 16, 32, 64
   and 128 bit sources/sinks and are appropriately referred to as Bn,
   Hn, Sn, Dn and Qn in the assembly code. Some instructions mix these
   access modes (e.g. FCVT S0, D0) so the implementation of the
   instruction needs to *know* which type of read or write access is
   required.  */

typedef enum VReg
{
  V0,
  V1,
  V2,
  V3,
  V4,
  V5,
  V6,
  V7,
  V8,
  V9,
  V10,
  V11,
  V12,
  V13,
  V14,
  V15,
  V16,
  V17,
  V18,
  V19,
  V20,
  V21,
  V22,
  V23,
  V24,
  V25,
  V26,
  V27,
  V28,
  V29,
  V30,
  V31,
} VReg;

/* All the different integer bit patterns for the components of a
   general register are overlaid here using a union so as to allow
   all reading and writing of the desired bits.  Note that we have
   to take care when emulating a big-endian AArch64 as we are
   running on a little endian host.  */

typedef union GRegisterValue
{
#if !WORDS_BIGENDIAN
  int8_t   s8;
  int16_t  s16;
  int32_t  s32;
  int64_t  s64;
  uint8_t  u8;
  uint16_t u16;
  uint32_t u32;
  uint64_t u64;
#else
  struct { int64_t :56; int8_t s8; };
  struct { int64_t :48; int16_t s16; };
  struct { int64_t :32; int32_t s32; };
  int64_t s64;
  struct { uint64_t :56; uint8_t u8; };
  struct { uint64_t :48; uint16_t u16; };
  struct { uint64_t :32; uint32_t u32; };
  uint64_t u64;
#endif
} GRegister;

/* Float registers provide for storage of a single, double or quad
   word format float in the same register.  Single floats are not
   paired within each double register as per 32 bit arm.  Instead each
   128 bit register Vn embeds the bits for Sn, and Dn in the lower
   quarter and half, respectively, of the bits for Qn.

   The upper bits can also be accessed as single or double floats by
   the float vector operations using indexing e.g. V1.D[1], V1.S[3]
   etc and, for SIMD operations using a horrible index range notation.

   The spec also talks about accessing float registers as half words
   and bytes with Hn and Bn providing access to the low 16 and 8 bits
   of Vn but it is not really clear what these bits represent.  We can
   probably ignore this for Java anyway.  However, we do need to access
   the raw bits at 32 and 64 bit resolution to load to/from integer
   registers.

   Note - we do not use the long double type.  Aliasing issues between
   integer and float values mean that it is unreliable to use them.  */

typedef union FRegisterValue
{
  float        s;
  double       d;

  uint64_t     v[2];
  uint32_t     w[4];
  uint16_t     h[8];
  uint8_t      b[16];

  int64_t      V[2];
  int32_t      W[4];
  int16_t      H[8];
  int8_t       B[16];

  float        S[4];
  double       D[2];

} FRegister;

/* Condition register bit select values.

   The order of bits here is important because some of
   the flag setting conditional instructions employ a
   bit field to populate the flags when a false condition
   bypasses execution of the operation and we want to
   be able to assign the flags register using the
   supplied value.  */

typedef enum FlagIdx
{
  V_IDX = 0,
  C_IDX = 1,
  Z_IDX = 2,
  N_IDX = 3
} FlagIdx;

typedef enum FlagMask
{
  V = 1 << V_IDX,
  C = 1 << C_IDX,
  Z = 1 << Z_IDX,
  N = 1 << N_IDX
} FlagMask;

#define CPSR_ALL_FLAGS (V | C | Z | N)

typedef uint32_t FlagsRegister;

/* FPSR register -- floating point status register

   This register includes IDC, IXC, UFC, OFC, DZC, IOC and QC bits,
   and the floating point N, Z, C, V bits but the latter are unused in
   aarch64 mode.  The sim ignores QC for now.

   Bit positions are as per the ARMv7 FPSCR register

   IDC :  7 ==> Input Denormal (cumulative exception bit)
   IXC :  4 ==> Inexact
   UFC :  3 ==> Underflow
   OFC :  2 ==> Overflow
   DZC :  1 ==> Division by Zero
   IOC :  0 ==> Invalid Operation

   The rounding mode is held in bits [23,22] defined as follows:

   0b00 Round to Nearest (RN) mode
   0b01 Round towards Plus Infinity (RP) mode
   0b10 Round towards Minus Infinity (RM) mode
   0b11 Round towards Zero (RZ) mode.  */

/* Indices for bits in the FPSR register value.  */
typedef enum FPSRIdx
{
  IO_IDX = 0,
  DZ_IDX = 1,
  OF_IDX = 2,
  UF_IDX = 3,
  IX_IDX = 4,
  ID_IDX = 7
} FPSRIdx;

/* Corresponding bits as numeric values.  */
typedef enum FPSRMask
{
  IO = (1 << IO_IDX),
  DZ = (1 << DZ_IDX),
  OF = (1 << OF_IDX),
  UF = (1 << UF_IDX),
  IX = (1 << IX_IDX),
  ID = (1 << ID_IDX)
} FPSRMask;

#define FPSR_ALL_FPSRS (IO | DZ | OF | UF | IX | ID)

/* General Register access functions.  */
extern uint64_t    aarch64_get_reg_u64 (sim_cpu *, GReg, int);
extern int64_t     aarch64_get_reg_s64 (sim_cpu *, GReg, int);
extern uint32_t    aarch64_get_reg_u32 (sim_cpu *, GReg, int);
extern int32_t     aarch64_get_reg_s32 (sim_cpu *, GReg, int);
extern uint32_t    aarch64_get_reg_u16 (sim_cpu *, GReg, int);
extern int32_t     aarch64_get_reg_s16 (sim_cpu *, GReg, int);
extern uint32_t    aarch64_get_reg_u8  (sim_cpu *, GReg, int);
extern int32_t     aarch64_get_reg_s8  (sim_cpu *, GReg, int);

extern void        aarch64_set_reg_u64 (sim_cpu *, GReg, int, uint64_t);
extern void        aarch64_set_reg_u32 (sim_cpu *, GReg, int, uint32_t);
extern void        aarch64_set_reg_s64 (sim_cpu *, GReg, int, int64_t);
extern void        aarch64_set_reg_s32 (sim_cpu *, GReg, int, int32_t);

/* FP Register access functions.  */
extern float       aarch64_get_FP_half   (sim_cpu *, VReg);
extern float       aarch64_get_FP_float  (sim_cpu *, VReg);
extern double      aarch64_get_FP_double (sim_cpu *, VReg);
extern void        aarch64_get_FP_long_double (sim_cpu *, VReg, FRegister *);

extern void        aarch64_set_FP_half   (sim_cpu *, VReg, float);
extern void        aarch64_set_FP_float  (sim_cpu *, VReg, float);
extern void        aarch64_set_FP_double (sim_cpu *, VReg, double);
extern void        aarch64_set_FP_long_double (sim_cpu *, VReg, FRegister);

/* PC register accessors.  */
extern uint64_t    aarch64_get_PC (sim_cpu *);
extern uint64_t    aarch64_get_next_PC (sim_cpu *);
extern void        aarch64_set_next_PC (sim_cpu *, uint64_t);
extern void        aarch64_set_next_PC_by_offset (sim_cpu *, int64_t);
extern void        aarch64_update_PC (sim_cpu *);
extern void        aarch64_save_LR (sim_cpu *);

/* Instruction accessor - implemented as a
   macro as we do not need to annotate it.  */
#define aarch64_get_instr(cpu)  (AARCH64_SIM_CPU (cpu)->instr)

/* Flag register accessors.  */
extern uint32_t    aarch64_get_CPSR       (sim_cpu *);
extern void        aarch64_set_CPSR       (sim_cpu *, uint32_t);
extern uint32_t    aarch64_get_CPSR_bits  (sim_cpu *, FlagMask);
extern void        aarch64_set_CPSR_bits  (sim_cpu *, uint32_t, uint32_t);
extern uint32_t    aarch64_test_CPSR_bit  (sim_cpu *, FlagMask);
extern void        aarch64_set_CPSR_bit   (sim_cpu *, FlagMask);
extern void        aarch64_clear_CPSR_bit (sim_cpu *, FlagMask);

extern void        aarch64_set_FPSR (sim_cpu *, uint32_t);
extern uint32_t    aarch64_get_FPSR (sim_cpu *);
extern void        aarch64_set_FPSR_bits (sim_cpu *, uint32_t, uint32_t);
extern uint32_t    aarch64_get_FPSR_bits (sim_cpu *, uint32_t);
extern int         aarch64_test_FPSR_bit (sim_cpu *, FPSRMask);

/* Vector register accessors.  */
extern uint64_t    aarch64_get_vec_u64 (sim_cpu *, VReg, unsigned);
extern uint32_t    aarch64_get_vec_u32 (sim_cpu *, VReg, unsigned);
extern uint16_t    aarch64_get_vec_u16 (sim_cpu *, VReg, unsigned);
extern uint8_t     aarch64_get_vec_u8  (sim_cpu *, VReg, unsigned);
extern void        aarch64_set_vec_u64 (sim_cpu *, VReg, unsigned, uint64_t);
extern void        aarch64_set_vec_u32 (sim_cpu *, VReg, unsigned, uint32_t);
extern void        aarch64_set_vec_u16 (sim_cpu *, VReg, unsigned, uint16_t);
extern void        aarch64_set_vec_u8  (sim_cpu *, VReg, unsigned, uint8_t);

extern int64_t     aarch64_get_vec_s64 (sim_cpu *, VReg, unsigned);
extern int32_t     aarch64_get_vec_s32 (sim_cpu *, VReg, unsigned);
extern int16_t     aarch64_get_vec_s16 (sim_cpu *, VReg, unsigned);
extern int8_t      aarch64_get_vec_s8  (sim_cpu *, VReg, unsigned);
extern void        aarch64_set_vec_s64 (sim_cpu *, VReg, unsigned, int64_t);
extern void        aarch64_set_vec_s32 (sim_cpu *, VReg, unsigned, int32_t);
extern void        aarch64_set_vec_s16 (sim_cpu *, VReg, unsigned, int16_t);
extern void        aarch64_set_vec_s8  (sim_cpu *, VReg, unsigned, int8_t);

extern float       aarch64_get_vec_float  (sim_cpu *, VReg, unsigned);
extern double      aarch64_get_vec_double (sim_cpu *, VReg, unsigned);
extern void        aarch64_set_vec_float  (sim_cpu *, VReg, unsigned, float);
extern void        aarch64_set_vec_double (sim_cpu *, VReg, unsigned, double);

/* System register accessors.  */
extern uint64_t	   aarch64_get_thread_id (sim_cpu *);
extern uint32_t	   aarch64_get_FPCR (sim_cpu *);
extern void	   aarch64_set_FPCR (sim_cpu *, uint32_t);

#endif /* _CPU_STATE_H  */
