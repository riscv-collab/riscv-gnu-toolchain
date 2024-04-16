/* Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

/* This function is NOT auto generated from xml.  Create the aarch64 with SVE
   feature into RESULT, where SCALE is the number of 128 bit chunks in a Z
   register.  */

static int
create_feature_aarch64_sve (struct target_desc *result, long regnum,
			    uint64_t scale)
{
  struct tdesc_feature *feature;
  tdesc_type *element_type, *field_type;
  tdesc_type_with_fields *type_with_fields;

  feature = tdesc_create_feature (result, "org.gnu.gdb.aarch64.sve");

  element_type = tdesc_named_type (feature, "uint128");
  tdesc_create_vector (feature, "svevqu", element_type, scale);

  element_type = tdesc_named_type (feature, "int128");
  tdesc_create_vector (feature, "svevqs", element_type, scale);

  element_type = tdesc_named_type (feature, "ieee_double");
  tdesc_create_vector (feature, "svevdf", element_type, 2 * scale);

  element_type = tdesc_named_type (feature, "uint64");
  tdesc_create_vector (feature, "svevdu", element_type, 2 * scale);

  element_type = tdesc_named_type (feature, "int64");
  tdesc_create_vector (feature, "svevds", element_type, 2 * scale);

  element_type = tdesc_named_type (feature, "ieee_single");
  tdesc_create_vector (feature, "svevsf", element_type, 4 * scale);

  element_type = tdesc_named_type (feature, "uint32");
  tdesc_create_vector (feature, "svevsu", element_type, 4 * scale);

  element_type = tdesc_named_type (feature, "int32");
  tdesc_create_vector (feature, "svevss", element_type, 4 * scale);

  element_type = tdesc_named_type (feature, "ieee_half");
  tdesc_create_vector (feature, "svevhf", element_type, 8 * scale);

  element_type = tdesc_named_type (feature, "uint16");
  tdesc_create_vector (feature, "svevhu", element_type, 8 * scale);

  element_type = tdesc_named_type (feature, "int16");
  tdesc_create_vector (feature, "svevhs", element_type, 8 * scale);

  element_type = tdesc_named_type (feature, "uint8");
  tdesc_create_vector (feature, "svevbu", element_type, 16 * scale);

  element_type = tdesc_named_type (feature, "int8");
  tdesc_create_vector (feature, "svevbs", element_type, 16 * scale);

  type_with_fields = tdesc_create_union (feature, "svevnq");
  field_type = tdesc_named_type (feature, "svevqu");
  tdesc_add_field (type_with_fields, "u", field_type);
  field_type = tdesc_named_type (feature, "svevqs");
  tdesc_add_field (type_with_fields, "s", field_type);

  type_with_fields = tdesc_create_union (feature, "svevnd");
  field_type = tdesc_named_type (feature, "svevdf");
  tdesc_add_field (type_with_fields, "f", field_type);
  field_type = tdesc_named_type (feature, "svevdu");
  tdesc_add_field (type_with_fields, "u", field_type);
  field_type = tdesc_named_type (feature, "svevds");
  tdesc_add_field (type_with_fields, "s", field_type);

  type_with_fields = tdesc_create_union (feature, "svevns");
  field_type = tdesc_named_type (feature, "svevsf");
  tdesc_add_field (type_with_fields, "f", field_type);
  field_type = tdesc_named_type (feature, "svevsu");
  tdesc_add_field (type_with_fields, "u", field_type);
  field_type = tdesc_named_type (feature, "svevss");
  tdesc_add_field (type_with_fields, "s", field_type);

  type_with_fields = tdesc_create_union (feature, "svevnh");
  field_type = tdesc_named_type (feature, "svevhf");
  tdesc_add_field (type_with_fields, "f", field_type);
  field_type = tdesc_named_type (feature, "svevhu");
  tdesc_add_field (type_with_fields, "u", field_type);
  field_type = tdesc_named_type (feature, "svevhs");
  tdesc_add_field (type_with_fields, "s", field_type);

  type_with_fields = tdesc_create_union (feature, "svevnb");
  field_type = tdesc_named_type (feature, "svevbu");
  tdesc_add_field (type_with_fields, "u", field_type);
  field_type = tdesc_named_type (feature, "svevbs");
  tdesc_add_field (type_with_fields, "s", field_type);

  type_with_fields = tdesc_create_union (feature, "svev");
  field_type = tdesc_named_type (feature, "svevnq");
  tdesc_add_field (type_with_fields, "q", field_type);
  field_type = tdesc_named_type (feature, "svevnd");
  tdesc_add_field (type_with_fields, "d", field_type);
  field_type = tdesc_named_type (feature, "svevns");
  tdesc_add_field (type_with_fields, "s", field_type);
  field_type = tdesc_named_type (feature, "svevnh");
  tdesc_add_field (type_with_fields, "h", field_type);
  field_type = tdesc_named_type (feature, "svevnb");
  tdesc_add_field (type_with_fields, "b", field_type);

  field_type = tdesc_named_type (feature, "uint8");
  tdesc_create_vector (feature, "svep", field_type, 2 * scale);

  /* FPSR register type */
  type_with_fields = tdesc_create_flags (feature, "fpsr_flags", 4);
  tdesc_add_flag (type_with_fields, 0, "IOC");
  tdesc_add_flag (type_with_fields, 1, "DZC");
  tdesc_add_flag (type_with_fields, 2, "OFC");
  tdesc_add_flag (type_with_fields, 3, "UFC");
  tdesc_add_flag (type_with_fields, 4, "IXC");
  tdesc_add_flag (type_with_fields, 7, "IDC");
  tdesc_add_flag (type_with_fields, 27, "QC");
  tdesc_add_flag (type_with_fields, 28, "V");
  tdesc_add_flag (type_with_fields, 29, "C");
  tdesc_add_flag (type_with_fields, 30, "Z");
  tdesc_add_flag (type_with_fields, 31, "N");

  /* FPCR register type */
  type_with_fields = tdesc_create_flags (feature, "fpcr_flags", 4);
  tdesc_add_flag (type_with_fields, 0, "FIZ");
  tdesc_add_flag (type_with_fields, 1, "AH");
  tdesc_add_flag (type_with_fields, 2, "NEP");
  tdesc_add_flag (type_with_fields, 8, "IOE");
  tdesc_add_flag (type_with_fields, 9, "DZE");
  tdesc_add_flag (type_with_fields, 10, "OFE");
  tdesc_add_flag (type_with_fields, 11, "UFE");
  tdesc_add_flag (type_with_fields, 12, "IXE");
  tdesc_add_flag (type_with_fields, 13, "EBF");
  tdesc_add_flag (type_with_fields, 15, "IDE");
  tdesc_add_bitfield (type_with_fields, "Len", 16, 18);
  tdesc_add_flag (type_with_fields, 19, "FZ16");
  tdesc_add_bitfield (type_with_fields, "Stride", 20, 21);
  tdesc_add_bitfield (type_with_fields, "RMode", 22, 23);
  tdesc_add_flag (type_with_fields, 24, "FZ");
  tdesc_add_flag (type_with_fields, 25, "DN");
  tdesc_add_flag (type_with_fields, 26, "AHP");

  tdesc_create_reg (feature, "z0", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z1", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z2", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z3", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z4", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z5", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z6", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z7", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z8", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z9", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z10", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z11", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z12", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z13", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z14", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z15", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z16", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z17", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z18", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z19", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z20", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z21", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z22", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z23", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z24", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z25", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z26", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z27", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z28", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z29", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z30", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "z31", regnum++, 1, NULL, 128 * scale, "svev");
  tdesc_create_reg (feature, "fpsr", regnum++, 1, NULL, 32, "fpsr_flags");
  tdesc_create_reg (feature, "fpcr", regnum++, 1, NULL, 32, "fpcr_flags");
  tdesc_create_reg (feature, "p0", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "p1", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "p2", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "p3", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "p4", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "p5", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "p6", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "p7", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "p8", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "p9", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "p10", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "p11", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "p12", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "p13", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "p14", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "p15", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "ffr", regnum++, 1, NULL, 16 * scale, "svep");
  tdesc_create_reg (feature, "vg", regnum++, 1, NULL, 64, "int");
  return regnum;
}
