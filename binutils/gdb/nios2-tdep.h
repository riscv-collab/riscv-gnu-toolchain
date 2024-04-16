/* Target-dependent header for the Nios II architecture, for GDB.
   Copyright (C) 2012-2024 Free Software Foundation, Inc.
   Contributed by Mentor Graphics, Inc.

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

#ifndef NIOS2_TDEP_H
#define NIOS2_TDEP_H

#include "gdbarch.h"

/* Nios II ISA specific encodings and macros.  */
#include "opcode/nios2.h"

/* Registers.  */
#define NIOS2_Z_REGNUM 0     /* Zero */
#define NIOS2_R2_REGNUM 2    /* used for return value */
#define NIOS2_R3_REGNUM 3    /* used for return value */
/* Used for hidden zero argument to store ptr to struct return value.  */
#define NIOS2_R4_REGNUM 4
#define NIOS2_R7_REGNUM 7
#define NIOS2_GP_REGNUM 26  /* Global Pointer */
#define NIOS2_SP_REGNUM 27  /* Stack Pointer */
#define NIOS2_FP_REGNUM 28  /* Frame Pointer */
#define NIOS2_EA_REGNUM 29  /* Exception address */
#define NIOS2_BA_REGNUM 30  /* Breakpoint return address */
#define NIOS2_RA_REGNUM 31  /* Return address */
#define NIOS2_PC_REGNUM 32

/* Control registers.  */
#define NIOS2_STATUS_REGNUM 33
#define NIOS2_ESTATUS_REGNUM 34
#define NIOS2_BSTATUS_REGNUM 35
#define NIOS2_IENABLE_REGNUM 36
#define NIOS2_IPENDING_REGNUM 37
#define NIOS2_CPUID_REGNUM 38
#define NIOS2_EXCEPTION_REGNUM 40
#define NIOS2_PTEADDR_REGNUM 41
#define NIOS2_TLBACC_REGNUM 42
#define NIOS2_TLBMISC_REGNUM 43
#define NIOS2_ECCINJ_REGNUM 44
#define NIOS2_BADADDR_REGNUM 45
#define NIOS2_CONFIG_REGNUM 46
#define NIOS2_MPUBASE_REGNUM 47
#define NIOS2_MPUACC_REGNUM 48

/* R4-R7 are used for argument passing.  */
#define NIOS2_FIRST_ARGREG NIOS2_R4_REGNUM
#define NIOS2_LAST_ARGREG NIOS2_R7_REGNUM

/* Number of all registers.  */
#define NIOS2_NUM_REGS 49

/* Size of an instruction, in bytes.  */
#define NIOS2_OPCODE_SIZE 4
#define NIOS2_CDX_OPCODE_SIZE 2

/* Target-dependent structure in gdbarch.  */
struct nios2_gdbarch_tdep : gdbarch_tdep_base
{
  /* Assumes FRAME is stopped at a syscall (trap) instruction; returns
     the expected next PC.  */
  CORE_ADDR (*syscall_next_pc) (frame_info_ptr frame,
				const struct nios2_opcode *op) = nullptr;

  /* Returns true if PC points to a kernel helper function.  */
  bool (*is_kernel_helper) (CORE_ADDR pc) = nullptr;

  /* Offset to PC value in jump buffer.
     If this is negative, longjmp support will be disabled.  */
  int jb_pc = 0;
};

extern const struct target_desc *tdesc_nios2_linux;
extern const struct target_desc *tdesc_nios2;

#endif /* NIOS2_TDEP_H */
