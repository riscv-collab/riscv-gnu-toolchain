/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: fpu.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_loongarch_fpu (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.loongarch.fpu");
  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_union (feature, "fputype");
  tdesc_type *field_type;
  field_type = tdesc_named_type (feature, "ieee_single");
  tdesc_add_field (type_with_fields, "f", field_type);
  field_type = tdesc_named_type (feature, "ieee_double");
  tdesc_add_field (type_with_fields, "d", field_type);

  tdesc_create_reg (feature, "f0", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f1", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f2", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f3", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f4", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f5", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f6", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f7", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f8", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f9", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f10", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f11", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f12", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f13", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f14", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f15", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f16", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f17", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f18", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f19", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f20", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f21", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f22", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f23", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f24", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f25", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f26", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f27", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f28", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f29", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f30", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "f31", regnum++, 1, "float", 64, "fputype");
  tdesc_create_reg (feature, "fcc0", regnum++, 1, "float", 8, "uint8");
  tdesc_create_reg (feature, "fcc1", regnum++, 1, "float", 8, "uint8");
  tdesc_create_reg (feature, "fcc2", regnum++, 1, "float", 8, "uint8");
  tdesc_create_reg (feature, "fcc3", regnum++, 1, "float", 8, "uint8");
  tdesc_create_reg (feature, "fcc4", regnum++, 1, "float", 8, "uint8");
  tdesc_create_reg (feature, "fcc5", regnum++, 1, "float", 8, "uint8");
  tdesc_create_reg (feature, "fcc6", regnum++, 1, "float", 8, "uint8");
  tdesc_create_reg (feature, "fcc7", regnum++, 1, "float", 8, "uint8");
  tdesc_create_reg (feature, "fcsr", regnum++, 1, "float", 32, "uint32");
  return regnum;
}
