/* GNU/Linux on  TI C6x target support.
   Copyright (C) 2011-2024 Free Software Foundation, Inc.
   Contributed by Yao Qi <yao@codesourcery.com>

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

#ifndef TIC6X_TDEP_H
#define TIC6X_TDEP_H

#include "gdbarch.h"

enum
{
  TIC6X_A4_REGNUM = 4,
  TIC6X_A5_REGNUM = 5,
  TIC6X_FP_REGNUM = 15,  /* Frame Pointer: A15 */
  TIC6X_B0_REGNUM = 16,
  TIC6X_RA_REGNUM = 19,  /* Return address: B3 */
  TIC6X_B4_REGNUM = 20,
  TIC6X_B5_REGNUM = 21,
  TIC6X_DP_REGNUM = 30,  /* Data Page Pointer: B14 */
  TIC6X_SP_REGNUM = 31,  /* Stack Pointer: B15 */
  TIC6X_CSR_REGNUM = 32,
  TIC6X_PC_REGNUM = 33,
  TIC6X_NUM_CORE_REGS = 33, /* The number of core registers */
  TIC6X_RILC_REGNUM = 68,
  TIC6X_NUM_REGS /* The number of registers */
};

#define TIC6X_INST_SWE 0x10000000

extern const gdb_byte tic6x_bkpt_illegal_opcode_be[];
extern const gdb_byte tic6x_bkpt_illegal_opcode_le[];

/* Target-dependent structure in gdbarch.  */
struct tic6x_gdbarch_tdep : gdbarch_tdep_base
{
  /* Return the expected next PC if FRAME is stopped at a syscall
     instruction.  */
  CORE_ADDR (*syscall_next_pc) (frame_info_ptr frame) = nullptr;

  const gdb_byte *breakpoint = nullptr; /* Breakpoint instruction.  */

  int has_gp = 0; /* Has general purpose registers A16 - A31 and B16 - B31.  */
};

#endif /* TIC6X_TDEP_H */
