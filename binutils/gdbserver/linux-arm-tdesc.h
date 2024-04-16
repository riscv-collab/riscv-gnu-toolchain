/* Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_LINUX_ARM_TDESC_H
#define GDBSERVER_LINUX_ARM_TDESC_H

#include "arch/arm.h"

/* Return the Arm target description with fp registers FP_TYPE.  */

const target_desc * arm_linux_read_description (arm_fp_type fp_type);

/* For a target description TDESC, return its fp type.  */

arm_fp_type arm_linux_get_tdesc_fp_type (const target_desc *tdesc);

#endif /* linux-arm-tdesc.h.  */
