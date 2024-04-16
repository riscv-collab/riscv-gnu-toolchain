/* Target-dependent code for Moxie
 
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

#ifndef MOXIE_TDEP_H
#define MOXIE_TDEP_H

#include "gdbarch.h"

struct moxie_gdbarch_tdep : gdbarch_tdep_base
{
  /* gdbarch target dependent data here.  Currently unused for MOXIE.  */
};

enum moxie_regnum
{
  MOXIE_FP_REGNUM = 0,
  MOXIE_SP_REGNUM = 1,
  R0_REGNUM = 2,
  R1_REGNUM = 3,
  MOXIE_PC_REGNUM = 16,
  MOXIE_CC_REGNUM = 17,
  RET1_REGNUM = R0_REGNUM,
  ARG1_REGNUM = R0_REGNUM,
  ARGN_REGNUM = R1_REGNUM,
};

#define MOXIE_NUM_REGS 18

#endif /* moxie-tdep.h */
