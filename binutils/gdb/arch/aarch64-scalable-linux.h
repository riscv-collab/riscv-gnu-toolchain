/* Common AArch64 Linux arch-specific definitions for the scalable
   extensions: SVE and SME.

   Copyright (C) 2023-2024 Free Software Foundation, Inc.

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

#ifndef ARCH_AARCH64_SCALABLE_LINUX_H
#define ARCH_AARCH64_SCALABLE_LINUX_H

#include "gdbsupport/common-defs.h"
#include "gdbsupport/common-regcache.h"

/* Feature check for Scalable Matrix Extension.  */
#ifndef HWCAP2_SME
#define HWCAP2_SME (1 << 23)
#endif

/* Feature check for Scalable Matrix Extension 2.  */
#ifndef HWCAP2_SME2
#define HWCAP2_SME2   (1UL << 37)
#define HWCAP2_SME2P1 (1UL << 38)
#endif

/* Streaming mode enabled/disabled bit.  */
#define SVCR_SM_BIT (1 << 0)
/* ZA enabled/disabled bit.  */
#define SVCR_ZA_BIT (1 << 1)
/* Mask including all valid SVCR bits.  */
#define SVCR_BIT_MASK (SVCR_SM_BIT | SVCR_ZA_BIT)

/* SVE/SSVE-related constants used for an empty SVE/SSVE register set
   dumped to a core file.  When SME is supported, either the SVE state or
   the SSVE state will be empty when it is dumped to a core file.  */
#define SVE_CORE_DUMMY_SIZE 0x220
#define SVE_CORE_DUMMY_MAX_SIZE 0x2240
#define SVE_CORE_DUMMY_VL 0x10
#define SVE_CORE_DUMMY_MAX_VL 0x100
#define SVE_CORE_DUMMY_FLAGS 0x0
#define SVE_CORE_DUMMY_RESERVED 0x0

/* Return TRUE if the SVE state in the register cache REGCACHE
   is empty (zero).  Return FALSE otherwise.  */
extern bool sve_state_is_empty (const struct reg_buffer_common *reg_buf);

#endif /* ARCH_AARCH64_SCALABLE_LINUX_H */
