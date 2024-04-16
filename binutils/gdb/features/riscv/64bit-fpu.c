/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: 64bit-fpu.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_riscv_64bit_fpu (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.riscv.fpu");
  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_union (feature, "riscv_double");
  tdesc_type *field_type;
  field_type = tdesc_named_type (feature, "ieee_single");
  tdesc_add_field (type_with_fields, "float", field_type);
  field_type = tdesc_named_type (feature, "ieee_double");
  tdesc_add_field (type_with_fields, "double", field_type);

  regnum = 33;
  tdesc_create_reg (feature, "ft0", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "ft1", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "ft2", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "ft3", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "ft4", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "ft5", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "ft6", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "ft7", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fs0", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fs1", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fa0", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fa1", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fa2", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fa3", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fa4", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fa5", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fa6", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fa7", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fs2", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fs3", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fs4", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fs5", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fs6", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fs7", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fs8", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fs9", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fs10", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "fs11", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "ft8", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "ft9", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "ft10", regnum++, 1, NULL, 64, "riscv_double");
  tdesc_create_reg (feature, "ft11", regnum++, 1, NULL, 64, "riscv_double");
  regnum = 68;
  tdesc_create_reg (feature, "fcsr", regnum++, 1, NULL, 32, "int");
  return regnum;
}
