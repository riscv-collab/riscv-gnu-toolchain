/* GNU/Linux on AArch64 target support, prototypes.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

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

#ifndef AARCH64_LINUX_TDEP_H
#define AARCH64_LINUX_TDEP_H

#include "regset.h"

/* The general-purpose regset consists of 31 X registers, plus SP, PC,
   and PSTATE registers, as defined in the AArch64 port of the Linux
   kernel.  */
#define AARCH64_LINUX_SIZEOF_GREGSET  (34 * X_REGISTER_SIZE)

/* The fp regset consists of 32 V registers, plus FPCR and FPSR which
   are 4 bytes wide each, and the whole structure is padded to 128 bit
   alignment.  */
#define AARCH64_LINUX_SIZEOF_FPREGSET (33 * V_REGISTER_SIZE)

/* The pauth regset consists of 2 X sized registers.  */
#define AARCH64_LINUX_SIZEOF_PAUTH (2 * X_REGISTER_SIZE)

/* The MTE regset consists of a 64-bit register.  */
#define AARCH64_LINUX_SIZEOF_MTE_REGSET (8)

extern const struct regset aarch64_linux_gregset;
extern const struct regset aarch64_linux_fpregset;

/* Matches HWCAP_PACA in kernel header arch/arm64/include/uapi/asm/hwcap.h.  */
#define AARCH64_HWCAP_PACA (1 << 30)

#endif /* AARCH64_LINUX_TDEP_H */
