/* Target-dependent code for the NDS32 architecture, for GDB.

   Copyright (C) 2013-2024 Free Software Foundation, Inc.
   Contributed by Andes Technology Corporation.

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

#ifndef NDS32_TDEP_H
#define NDS32_TDEP_H

#include "gdbarch.h"

enum nds32_regnum
{
  /* General purpose registers.  */
  NDS32_R0_REGNUM = 0,
  NDS32_R5_REGNUM = 5,
  NDS32_TA_REGNUM = 15,		/* Temporary register.  */
  NDS32_FP_REGNUM = 28,		/* Frame pointer.  */
  NDS32_GP_REGNUM = 29,		/* Global pointer.  */
  NDS32_LP_REGNUM = 30,		/* Link pointer.  */
  NDS32_SP_REGNUM = 31,		/* Stack pointer.  */

  NDS32_PC_REGNUM = 32,		/* Program counter.  */

  NDS32_NUM_REGS,

  /* The first double precision floating-point register.  */
  NDS32_FD0_REGNUM = NDS32_NUM_REGS,
};

struct nds32_gdbarch_tdep : gdbarch_tdep_base
{
  /* The guessed FPU configuration.  */
  int fpu_freg = 0;
  /* FSRs are defined as pseudo registers.  */
  int use_pseudo_fsrs = 0;
  /* Cached regnum of the first FSR (FS0).  */
  int fs0_regnum = 0;
  /* ELF ABI info.  */
  int elf_abi = 0;
};
#endif /* NDS32_TDEP_H */
