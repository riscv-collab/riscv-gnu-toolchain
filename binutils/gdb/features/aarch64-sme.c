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
#include <cmath>

/* This function is NOT auto generated from xml.  Create the AArch64 SME
   feature into RESULT.  SVL is the streaming vector length in bytes.

   The ZA register has a total size of SVL x SVL.

   When in Streaming SVE mode, the effective SVE vector length, VL, is equal
   to SVL.  */

static int
create_feature_aarch64_sme (struct target_desc *result, long regnum,
			    size_t svl)
{
  struct tdesc_feature *feature;
  tdesc_type *element_type;

  feature = tdesc_create_feature (result, "org.gnu.gdb.aarch64.sme");

  /* The SVG register.  */
  tdesc_create_reg (feature, "svg", regnum++, 1, nullptr, 64, "int");

  /* SVCR flags type.  */
  tdesc_type_with_fields *type_with_fields
    = tdesc_create_flags (feature, "svcr_flags", 8);
  tdesc_add_flag (type_with_fields, 0, "SM");
  tdesc_add_flag (type_with_fields, 1, "ZA");

  /* The SVCR register.  */
  tdesc_create_reg (feature, "svcr", regnum++, 1, nullptr, 64, "svcr_flags");

  /* Byte type.  */
  element_type = tdesc_named_type (feature, "uint8");
  /* Vector of bytes.  */
  element_type = tdesc_create_vector (feature, "sme_bv", element_type,
				      svl);
  /* Vector of vector of bytes (Matrix).  */
  element_type = tdesc_create_vector (feature, "sme_bvv", element_type,
				      svl);

  /* The following is the ZA register set.  */
  tdesc_create_reg (feature, "za", regnum++, 1, nullptr,
		    std::pow (svl, 2) * 8, "sme_bvv");
  return regnum;
}
