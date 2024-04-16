/* FreeBSD/aarch64 target support, prototypes.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#ifndef AARCH64_FBSD_TDEP_H
#define AARCH64_FBSD_TDEP_H

#include "regset.h"

/* The general-purpose regset consists of 30 X registers, plus LR, SP,
   ELR, and SPSR registers.  SPSR is 32 bits but the structure is
   padded to 64 bit alignment.  */
#define AARCH64_FBSD_SIZEOF_GREGSET  (34 * X_REGISTER_SIZE)

/* The fp regset consists of 32 V registers, plus FPSR and FPCR which
   are 4 bytes wide each, and the whole structure is padded to 128 bit
   alignment.  */
#define AARCH64_FBSD_SIZEOF_FPREGSET (33 * V_REGISTER_SIZE)

/* The TLS regset consists of a single register.  */
#define	AARCH64_FBSD_SIZEOF_TLSREGSET (X_REGISTER_SIZE)

extern const struct regset aarch64_fbsd_gregset;
extern const struct regset aarch64_fbsd_fpregset;
extern const struct regset aarch64_fbsd_tls_regset;

#endif /* AARCH64_FBSD_TDEP_H */
