/* Definitions to target GDB to OpenRISC 1000 32-bit targets.
   Copyright (C) 2008-2024 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along
   With this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef OR1K_TDEP_H
#define OR1K_TDEP_H

#ifndef TARGET_OR1K
#define TARGET_OR1K
#endif

/* Make cgen names unique to prevent ODR conflicts with other targets.  */
#define GDB_CGEN_REMAP_PREFIX or1k
#include "cgen-remap.h"
#include "opcodes/or1k-desc.h"
#include "opcodes/or1k-opc.h"

/* General Purpose Registers */
#define OR1K_ZERO_REGNUM          0
#define OR1K_SP_REGNUM            1
#define OR1K_FP_REGNUM            2
#define OR1K_FIRST_ARG_REGNUM     3
#define OR1K_LAST_ARG_REGNUM      8
#define OR1K_LR_REGNUM            9
#define OR1K_FIRST_SAVED_REGNUM  10
#define OR1K_RV_REGNUM           11
#define OR1K_PPC_REGNUM          (OR1K_MAX_GPR_REGS + 0)
#define OR1K_NPC_REGNUM          (OR1K_MAX_GPR_REGS + 1)
#define OR1K_SR_REGNUM           (OR1K_MAX_GPR_REGS + 2)

/* Properties of the architecture. GDB mapping of registers is all the GPRs
   and SPRs followed by the PPC, NPC and SR at the end. Red zone is the area
   past the end of the stack reserved for exception handlers etc.  */

#define OR1K_MAX_GPR_REGS            32
#define OR1K_NUM_PSEUDO_REGS         0
#define OR1K_NUM_REGS               (OR1K_MAX_GPR_REGS + 3)
#define OR1K_STACK_ALIGN             4
#define OR1K_INSTLEN                 4
#define OR1K_INSTBITLEN             (OR1K_INSTLEN * 8)
#define OR1K_NUM_TAP_RECORDS         8
#define OR1K_FRAME_RED_ZONE_SIZE     2536

/* Single step based on where the current instruction will take us.  */
extern std::vector<CORE_ADDR> or1k_software_single_step
  (struct regcache *regcache);


#endif /* OR1K_TDEP_H */
