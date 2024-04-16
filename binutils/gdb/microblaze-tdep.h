/* Target-dependent code for Xilinx MicroBlaze.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#ifndef MICROBLAZE_TDEP_H
#define MICROBLAZE_TDEP_H 1

#include "gdbarch.h"

/* Microblaze architecture-specific information.  */
struct microblaze_gdbarch_tdep : gdbarch_tdep_base
{
};

/* Register numbers.  */
enum microblaze_regnum
{
  MICROBLAZE_R0_REGNUM,
  MICROBLAZE_R1_REGNUM, MICROBLAZE_SP_REGNUM = MICROBLAZE_R1_REGNUM,
  MICROBLAZE_R2_REGNUM,
  MICROBLAZE_R3_REGNUM, MICROBLAZE_RETVAL_REGNUM = MICROBLAZE_R3_REGNUM,
  MICROBLAZE_R4_REGNUM,
  MICROBLAZE_R5_REGNUM, MICROBLAZE_FIRST_ARGREG = MICROBLAZE_R5_REGNUM,
  MICROBLAZE_R6_REGNUM,
  MICROBLAZE_R7_REGNUM,
  MICROBLAZE_R8_REGNUM,
  MICROBLAZE_R9_REGNUM,
  MICROBLAZE_R10_REGNUM, MICROBLAZE_LAST_ARGREG = MICROBLAZE_R10_REGNUM,
  MICROBLAZE_R11_REGNUM,
  MICROBLAZE_R12_REGNUM,
  MICROBLAZE_R13_REGNUM,
  MICROBLAZE_R14_REGNUM,
  MICROBLAZE_R15_REGNUM,
  MICROBLAZE_R16_REGNUM,
  MICROBLAZE_R17_REGNUM,
  MICROBLAZE_R18_REGNUM,
  MICROBLAZE_R19_REGNUM,
  MICROBLAZE_R20_REGNUM,
  MICROBLAZE_R21_REGNUM,
  MICROBLAZE_R22_REGNUM,
  MICROBLAZE_R23_REGNUM,
  MICROBLAZE_R24_REGNUM,
  MICROBLAZE_R25_REGNUM,
  MICROBLAZE_R26_REGNUM,
  MICROBLAZE_R27_REGNUM,
  MICROBLAZE_R28_REGNUM,
  MICROBLAZE_R29_REGNUM,
  MICROBLAZE_R30_REGNUM,
  MICROBLAZE_R31_REGNUM,
  MICROBLAZE_PC_REGNUM,
  MICROBLAZE_MSR_REGNUM,
  MICROBLAZE_EAR_REGNUM,
  MICROBLAZE_ESR_REGNUM,
  MICROBLAZE_FSR_REGNUM,
  MICROBLAZE_BTR_REGNUM,
  MICROBLAZE_PVR0_REGNUM,
  MICROBLAZE_PVR1_REGNUM,
  MICROBLAZE_PVR2_REGNUM,
  MICROBLAZE_PVR3_REGNUM,
  MICROBLAZE_PVR4_REGNUM,
  MICROBLAZE_PVR5_REGNUM,
  MICROBLAZE_PVR6_REGNUM,
  MICROBLAZE_PVR7_REGNUM,
  MICROBLAZE_PVR8_REGNUM,
  MICROBLAZE_PVR9_REGNUM,
  MICROBLAZE_PVR10_REGNUM,
  MICROBLAZE_PVR11_REGNUM,
  MICROBLAZE_REDR_REGNUM,
  MICROBLAZE_RPID_REGNUM,
  MICROBLAZE_RZPR_REGNUM,
  MICROBLAZE_RTLBX_REGNUM,
  MICROBLAZE_RTLBSX_REGNUM,
  MICROBLAZE_RTLBLO_REGNUM,
  MICROBLAZE_RTLBHI_REGNUM,
  MICROBLAZE_SLR_REGNUM, MICROBLAZE_NUM_CORE_REGS = MICROBLAZE_SLR_REGNUM,
  MICROBLAZE_SHR_REGNUM,
  MICROBLAZE_NUM_REGS
};

struct microblaze_frame_cache
{
  /* Base address.  */
  CORE_ADDR base;
  CORE_ADDR pc;

  /* Do we have a frame?  */
  int frameless_p;

  /* Frame size.  */
  int framesize;

  /* Frame register.  */
  int fp_regnum;

  /* Offsets to saved registers.  */
  int register_offsets[MICROBLAZE_NUM_REGS];

  /* Table of saved registers.  */
  struct trad_frame_saved_reg *saved_regs;
};
/* All registers are 32 bits.  */
#define MICROBLAZE_REGISTER_SIZE 4

/* MICROBLAZE_BREAKPOINT defines the breakpoint that should be used.
   Only used for native debugging.  */
#define MICROBLAZE_BREAKPOINT {0xb9, 0xcc, 0x00, 0x60}

#endif /* microblaze-tdep.h */
