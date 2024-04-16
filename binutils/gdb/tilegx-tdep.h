/* Target-dependent code for the Tilera TILE-Gx processor.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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

#ifndef TILEGX_TDEP_H
#define TILEGX_TDEP_H

/* TILE-Gx has 56 general purpose registers (R0 - R52, TP, SP, LR),
   plus 8 special general purpose registers (network and ZERO),
   plus 1 magic register (PC).

   TP (aka R53) is the thread specific data pointer.
   SP (aka R54) is the stack pointer.
   LR (aka R55) is the link register.  */

enum tilegx_regnum
  {
    TILEGX_R0_REGNUM,
    TILEGX_FIRST_EASY_REGNUM = TILEGX_R0_REGNUM,
    TILEGX_R1_REGNUM,
    TILEGX_R2_REGNUM,
    TILEGX_R3_REGNUM,
    TILEGX_R4_REGNUM,
    TILEGX_R5_REGNUM,
    TILEGX_R6_REGNUM,
    TILEGX_R7_REGNUM,
    TILEGX_R8_REGNUM,
    TILEGX_R9_REGNUM,
    TILEGX_R10_REGNUM,
    TILEGX_R11_REGNUM,
    TILEGX_R12_REGNUM,
    TILEGX_R13_REGNUM,
    TILEGX_R14_REGNUM,
    TILEGX_R15_REGNUM,
    TILEGX_R16_REGNUM,
    TILEGX_R17_REGNUM,
    TILEGX_R18_REGNUM,
    TILEGX_R19_REGNUM,
    TILEGX_R20_REGNUM,
    TILEGX_R21_REGNUM,
    TILEGX_R22_REGNUM,
    TILEGX_R23_REGNUM,
    TILEGX_R24_REGNUM,
    TILEGX_R25_REGNUM,
    TILEGX_R26_REGNUM,
    TILEGX_R27_REGNUM,
    TILEGX_R28_REGNUM,
    TILEGX_R29_REGNUM,
    TILEGX_R30_REGNUM,
    TILEGX_R31_REGNUM,
    TILEGX_R32_REGNUM,
    TILEGX_R33_REGNUM,
    TILEGX_R34_REGNUM,
    TILEGX_R35_REGNUM,
    TILEGX_R36_REGNUM,
    TILEGX_R37_REGNUM,
    TILEGX_R38_REGNUM,
    TILEGX_R39_REGNUM,
    TILEGX_R40_REGNUM,
    TILEGX_R41_REGNUM,
    TILEGX_R42_REGNUM,
    TILEGX_R43_REGNUM,
    TILEGX_R44_REGNUM,
    TILEGX_R45_REGNUM,
    TILEGX_R46_REGNUM,
    TILEGX_R47_REGNUM,
    TILEGX_R48_REGNUM,
    TILEGX_R49_REGNUM,
    TILEGX_R50_REGNUM,
    TILEGX_R51_REGNUM,
    TILEGX_R52_REGNUM,
    TILEGX_TP_REGNUM,
    TILEGX_SP_REGNUM,
    TILEGX_LR_REGNUM,

    TILEGX_SN_REGNUM,
    TILEGX_NUM_EASY_REGS = TILEGX_SN_REGNUM, /* 56 */

    TILEGX_IO0_REGNUM,
    TILEGX_IO1_REGNUM,
    TILEGX_US0_REGNUM,
    TILEGX_US1_REGNUM,
    TILEGX_US2_REGNUM,
    TILEGX_US3_REGNUM,
    TILEGX_ZERO_REGNUM,

    TILEGX_PC_REGNUM,
    TILEGX_NUM_PHYS_REGS = TILEGX_PC_REGNUM, /* 64 */
    TILEGX_FAULTNUM_REGNUM,
    TILEGX_NUM_REGS, /* 66 */
  };

enum { tilegx_reg_size = 8 };

#endif /* tilegx-tdep.h */
