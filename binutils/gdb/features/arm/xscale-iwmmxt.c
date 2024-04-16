/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: xscale-iwmmxt.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_arm_xscale_iwmmxt (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.xscale.iwmmxt");
  tdesc_type *element_type;
  element_type = tdesc_named_type (feature, "uint8");
  tdesc_create_vector (feature, "iwmmxt_v8u8", element_type, 8);

  element_type = tdesc_named_type (feature, "uint16");
  tdesc_create_vector (feature, "iwmmxt_v4u16", element_type, 4);

  element_type = tdesc_named_type (feature, "uint32");
  tdesc_create_vector (feature, "iwmmxt_v2u32", element_type, 2);

  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_union (feature, "iwmmxt_vec64i");
  tdesc_type *field_type;
  field_type = tdesc_named_type (feature, "iwmmxt_v8u8");
  tdesc_add_field (type_with_fields, "u8", field_type);
  field_type = tdesc_named_type (feature, "iwmmxt_v4u16");
  tdesc_add_field (type_with_fields, "u16", field_type);
  field_type = tdesc_named_type (feature, "iwmmxt_v2u32");
  tdesc_add_field (type_with_fields, "u32", field_type);
  field_type = tdesc_named_type (feature, "uint64");
  tdesc_add_field (type_with_fields, "u64", field_type);

  tdesc_create_reg (feature, "wR0", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR1", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR2", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR3", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR4", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR5", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR6", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR7", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR8", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR9", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR10", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR11", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR12", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR13", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR14", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR15", regnum++, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wCSSF", regnum++, 1, "vector", 32, "int");
  tdesc_create_reg (feature, "wCASF", regnum++, 1, "vector", 32, "int");
  tdesc_create_reg (feature, "wCGR0", regnum++, 1, "vector", 32, "int");
  tdesc_create_reg (feature, "wCGR1", regnum++, 1, "vector", 32, "int");
  tdesc_create_reg (feature, "wCGR2", regnum++, 1, "vector", 32, "int");
  tdesc_create_reg (feature, "wCGR3", regnum++, 1, "vector", 32, "int");
  return regnum;
}
