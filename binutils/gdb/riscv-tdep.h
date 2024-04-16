/* Target-dependent header for the RISC-V architecture, for GDB, the
   GNU Debugger.

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

#ifndef RISCV_TDEP_H
#define RISCV_TDEP_H

#include "arch/riscv.h"
#include "gdbarch.h"

/* RiscV register numbers.  */
enum
{
  RISCV_ZERO_REGNUM = 0,	/* Read-only register, always 0.  */
  RISCV_RA_REGNUM = 1,		/* Return Address.  */
  RISCV_SP_REGNUM = 2,		/* Stack Pointer.  */
  RISCV_GP_REGNUM = 3,		/* Global Pointer.  */
  RISCV_TP_REGNUM = 4,		/* Thread Pointer.  */
  RISCV_FP_REGNUM = 8,		/* Frame Pointer.  */
  RISCV_A0_REGNUM = 10,		/* First argument.  */
  RISCV_A1_REGNUM = 11,		/* Second argument.  */
  RISCV_A7_REGNUM = 17,		/* Seventh argument.  */
  RISCV_PC_REGNUM = 32,		/* Program Counter.  */

  RISCV_NUM_INTEGER_REGS = 32,

  RISCV_FIRST_FP_REGNUM = 33,	/* First Floating Point Register */
  RISCV_FA0_REGNUM = 43,
  RISCV_FA1_REGNUM = RISCV_FA0_REGNUM + 1,
  RISCV_LAST_FP_REGNUM = 64,	/* Last Floating Point Register */

  RISCV_FIRST_CSR_REGNUM = 65,  /* First CSR */
#define DECLARE_CSR(name, num, class, define_version, abort_version) \
  RISCV_ ## num ## _REGNUM = RISCV_FIRST_CSR_REGNUM + num,
#include "opcode/riscv-opc.h"
#undef DECLARE_CSR
  RISCV_LAST_CSR_REGNUM = 4160,
  RISCV_CSR_LEGACY_MISA_REGNUM = 0xf10 + RISCV_FIRST_CSR_REGNUM,

  RISCV_PRIV_REGNUM = 4161,

  RISCV_V0_REGNUM,

  RISCV_V31_REGNUM = RISCV_V0_REGNUM + 31,

  RISCV_LAST_REGNUM = RISCV_V31_REGNUM
};

/* RiscV DWARF register numbers.  */
enum
{
  RISCV_DWARF_REGNUM_X0 = 0,
  RISCV_DWARF_REGNUM_X31 = 31,
  RISCV_DWARF_REGNUM_F0 = 32,
  RISCV_DWARF_REGNUM_F31 = 63,
  RISCV_DWARF_REGNUM_V0 = 96,
  RISCV_DWARF_REGNUM_V31 = 127,
  RISCV_DWARF_FIRST_CSR = 4096,
  RISCV_DWARF_LAST_CSR = 8191,
};

/* RISC-V specific per-architecture information.  */
struct riscv_gdbarch_tdep : gdbarch_tdep_base
{
  /* Features about the target hardware that impact how the gdbarch is
     configured.  Two gdbarch instances are compatible only if this field
     matches.  */
  struct riscv_gdbarch_features isa_features;

  /* Features about the abi that impact how the gdbarch is configured.  Two
     gdbarch instances are compatible only if this field matches.  */
  struct riscv_gdbarch_features abi_features;

  /* ISA-specific data types.  */
  struct type *riscv_fpreg_d_type = nullptr;

  /* The location of these registers, set to -2 by default so we don't
     match against -1 which is frequently used to mean "all registers",
     e.g. in the regcache supply/collect code.  */
  int fflags_regnum = -2;
  int frm_regnum = -2;

  /* Use for tracking unknown CSRs in the target description.
     UNKNOWN_CSRS_FIRST_REGNUM is the number assigned to the first unknown
     CSR.  All other unknown CSRs will be assigned sequential numbers after
     this, with UNKNOWN_CSRS_COUNT being the total number of unknown CSRs.  */
  int unknown_csrs_first_regnum = -1;
  int unknown_csrs_count = 0;

  /* Some targets (QEMU) are reporting three registers twice in the target
     description they send.  These three register numbers, when not set to
     -1, are for the duplicate copies of these registers.  */
  int duplicate_fflags_regnum = -1;
  int duplicate_frm_regnum = -1;
  int duplicate_fcsr_regnum = -1;

  /* Return the expected next PC assuming FRAME is stopped at a syscall
     instruction.  */
  CORE_ADDR (*syscall_next_pc) (frame_info_ptr frame) = nullptr;
};


/* Return the width in bytes  of the general purpose registers for GDBARCH.
   Possible return values are 4, 8, or 16 for RiscV variants RV32, RV64, or
   RV128.  */
extern int riscv_isa_xlen (struct gdbarch *gdbarch);

/* Return the width in bytes of the hardware floating point registers for
   GDBARCH.  If this architecture has no floating point registers, then
   return 0.  Possible values are 4, 8, or 16 for depending on which of
   single, double or quad floating point support is available.  */
extern int riscv_isa_flen (struct gdbarch *gdbarch);

/* Return the width in bytes of the general purpose register abi for
   GDBARCH.  This can be equal to, or less than RISCV_ISA_XLEN and reflects
   how the binary was compiled rather than the hardware that is available.
   It is possible that a binary compiled for RV32 is being run on an RV64
   target, in which case the isa xlen is 8-bytes, and the abi xlen is
   4-bytes.  This will impact how inferior functions are called.  */
extern int riscv_abi_xlen (struct gdbarch *gdbarch);

/* Return the width in bytes of the floating point register abi for
   GDBARCH.  This reflects how the binary was compiled rather than the
   hardware that is available.  It is possible that a binary is compiled
   for single precision floating point, and then run on a target with
   double precision floating point.  A return value of 0 indicates that no
   floating point abi is in use (floating point arguments will be passed
   in integer registers) other possible return value are 4, 8, or 16 as
   with RISCV_ISA_FLEN.  */
extern int riscv_abi_flen (struct gdbarch *gdbarch);

/* Return true if GDBARCH is using the embedded x-regs abi, that is the
   target only has 16 x-registers, which includes a reduced number of
   argument registers.  */
extern bool riscv_abi_embedded (struct gdbarch *gdbarch);

/* Single step based on where the current instruction will take us.  */
extern std::vector<CORE_ADDR> riscv_software_single_step
  (struct regcache *regcache);

/* Supply register REGNUM from the buffer REGS (length LEN) into
   REGCACHE.  REGSET describes the layout of the buffer.  If REGNUM is -1
   then all registers described by REGSET are supplied.

   The register RISCV_ZERO_REGNUM should not be described by REGSET,
   however, this register (which always has the value 0) will be supplied
   by this function if requested.

   The registers RISCV_CSR_FFLAGS_REGNUM and RISCV_CSR_FRM_REGNUM should
   not be described by REGSET, however, these register will be provided if
   requested assuming either:
   (a) REGCACHE already contains the value of RISCV_CSR_FCSR_REGNUM, or
   (b) REGSET describes the location of RISCV_CSR_FCSR_REGNUM in the REGS
       buffer.

   This function can be used as the supply function for either x-regs or
   f-regs when loading corefiles, and doesn't care which abi is currently
   in use.  */

extern void riscv_supply_regset (const struct regset *regset,
				  struct regcache *regcache, int regnum,
				  const void *regs, size_t len);

/* The names of the RISC-V target description features.  */
extern const char *riscv_feature_name_csr;

#endif /* RISCV_TDEP_H */
