/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: aarch64-core.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_aarch64_core (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.aarch64.core");
  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_flags (feature, "cpsr_flags", 4);
  tdesc_add_flag (type_with_fields, 0, "SP");
  tdesc_add_bitfield (type_with_fields, "EL", 2, 3);
  tdesc_add_flag (type_with_fields, 4, "nRW");
  tdesc_add_flag (type_with_fields, 6, "F");
  tdesc_add_flag (type_with_fields, 7, "I");
  tdesc_add_flag (type_with_fields, 8, "A");
  tdesc_add_flag (type_with_fields, 9, "D");
  tdesc_add_bitfield (type_with_fields, "BTYPE", 10, 11);
  tdesc_add_flag (type_with_fields, 12, "SSBS");
  tdesc_add_flag (type_with_fields, 20, "IL");
  tdesc_add_flag (type_with_fields, 21, "SS");
  tdesc_add_flag (type_with_fields, 22, "PAN");
  tdesc_add_flag (type_with_fields, 23, "UAO");
  tdesc_add_flag (type_with_fields, 24, "DIT");
  tdesc_add_flag (type_with_fields, 25, "TCO");
  tdesc_add_flag (type_with_fields, 28, "V");
  tdesc_add_flag (type_with_fields, 29, "C");
  tdesc_add_flag (type_with_fields, 30, "Z");
  tdesc_add_flag (type_with_fields, 31, "N");

  tdesc_create_reg (feature, "x0", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x1", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x2", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x3", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x4", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x5", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x6", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x7", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x8", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x9", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x10", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x11", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x12", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x13", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x14", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x15", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x16", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x17", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x18", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x19", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x20", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x21", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x22", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x23", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x24", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x25", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x26", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x27", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x28", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x29", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x30", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "sp", regnum++, 1, NULL, 64, "data_ptr");
  tdesc_create_reg (feature, "pc", regnum++, 1, NULL, 64, "code_ptr");
  tdesc_create_reg (feature, "cpsr", regnum++, 1, NULL, 32, "cpsr_flags");
  return regnum;
}
