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

#include "server.h"

#include "linux-arm-tdesc.h"

#include "tdesc.h"
#include "arch/arm.h"
#include <inttypes.h>

/* All possible Arm target descriptors.  */
static struct target_desc *tdesc_arm_list[ARM_FP_TYPE_INVALID];

/* See linux-arm-tdesc.h.  */

const target_desc *
arm_linux_read_description (arm_fp_type fp_type)
{
  struct target_desc *tdesc = tdesc_arm_list[fp_type];

  if (tdesc == nullptr)
    {
      tdesc = arm_create_target_description (fp_type, false);

      static const char *expedite_regs[] = { "r11", "sp", "pc", 0 };
      init_target_desc (tdesc, expedite_regs);

      tdesc_arm_list[fp_type] = tdesc;
    }

  return tdesc;
}

/* See linux-arm-tdesc.h.  */

arm_fp_type
arm_linux_get_tdesc_fp_type (const target_desc *tdesc)
{
  gdb_assert (tdesc != nullptr);

  /* Many of the tdesc_arm_list entries may not have been initialised yet.  This
     is ok, because tdesc must be one of the initialised ones.  */
  for (int i = ARM_FP_TYPE_NONE; i < ARM_FP_TYPE_INVALID; i++)
    {
      if (tdesc == tdesc_arm_list[i])
	return (arm_fp_type) i;
    }

  return ARM_FP_TYPE_INVALID;
}
