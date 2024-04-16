/* Copyright (C) 2023-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/tdesc.h"

/* This function is NOT auto generated from xml.  Create the AArch64 SME2
   feature into RESULT.

   The ZT0 register is only available when the SME ZA register is
   available.  */

static int
create_feature_aarch64_sme2 (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;
  tdesc_type *element_type;

  feature = tdesc_create_feature (result, "org.gnu.gdb.aarch64.sme2");

  /* Byte type.  */
  element_type = tdesc_named_type (feature, "uint8");

  /* Vector of 64 bytes.  */
  element_type = tdesc_create_vector (feature, "sme2_bv", element_type, 64);

  /* The following is the ZT0 register, with 512 bits (64 bytes).  */
  tdesc_create_reg (feature, "zt0", regnum++, 1, nullptr, 512, "sme2_bv");
  return regnum;
}
