/* Target-dependent code for GDB, the GNU debugger.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

#ifndef PPC_LINUX_TDEP_H
#define PPC_LINUX_TDEP_H

#include "ppc-tdep.h"

struct regset;

/* From ppc-linux-tdep.c ...  */
const struct regset *ppc_linux_gregset (int);
const struct regset *ppc_linux_fpregset (void);

/* Get the vector regset that matches the target byte order.  */
const struct regset *ppc_linux_vrregset (struct gdbarch *gdbarch);
const struct regset *ppc_linux_vsxregset (void);

/* Get the checkpointed GPR regset that matches the target wordsize
   and byteorder of GDBARCH.  */
const struct regset *ppc_linux_cgprregset (struct gdbarch *gdbarch);

/* Get the checkpointed vector regset that matches the target byte
   order.  */
const struct regset* ppc_linux_cvmxregset (struct gdbarch *gdbarch);

/* Extra register number constants.  The Linux kernel stores a
   "trap" code and the original value of r3 into special "registers";
   these need to be saved and restored when performing an inferior
   call while the inferior was interrupted within a system call.  */
enum {
  PPC_ORIG_R3_REGNUM = PPC_NUM_REGS,
  PPC_TRAP_REGNUM,
};

/* Return 1 if PPC_ORIG_R3_REGNUM and PPC_TRAP_REGNUM are usable.  */
int ppc_linux_trap_reg_p (struct gdbarch *gdbarch);

/* Additional register sets, defined in ppc-linux-tdep.c.  */
extern const struct regset ppc32_linux_pprregset;
extern const struct regset ppc32_linux_dscrregset;
extern const struct regset ppc32_linux_tarregset;
extern const struct regset ppc32_linux_ebbregset;
extern const struct regset ppc32_linux_pmuregset;
extern const struct regset ppc32_linux_tm_sprregset;
extern const struct regset ppc32_linux_cfprregset;
extern const struct regset ppc32_linux_cvsxregset;
extern const struct regset ppc32_linux_cpprregset;
extern const struct regset ppc32_linux_cdscrregset;
extern const struct regset ppc32_linux_ctarregset;

#endif /* PPC_LINUX_TDEP_H */
