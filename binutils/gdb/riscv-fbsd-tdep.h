/* FreeBSD/riscv target support, prototypes.

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

#ifndef RISCV_FBSD_TDEP_H
#define RISCV_FBSD_TDEP_H

#include "regset.h"

/* The general-purpose regset consists of 31 X registers, EPC, and
   SSTATUS.  */
#define RISCV_FBSD_NUM_GREGS		33

/* The fp regset always consists of 32 128-bit registers, plus a
   64-bit CSR_FCSR.  If 'Q' is not supported, only the low 64-bits of
   each floating point register are valid.  If 'D' is not supported,
   only the low 32-bits of each floating point register are valid.  */
#define RISCV_FBSD_SIZEOF_FPREGSET (32 * 16 + 8)

extern const struct regset riscv_fbsd_gregset;
extern const struct regset riscv_fbsd_fpregset;

#endif /* RISCV_FBSD_TDEP_H */
