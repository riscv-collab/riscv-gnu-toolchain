/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: v2-aux.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_arc_v2_aux (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.arc.aux");
  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_flags (feature, "status32_type", 4);
  tdesc_add_flag (type_with_fields, 0, "H");
  tdesc_add_bitfield (type_with_fields, "E", 1, 4);
  tdesc_add_flag (type_with_fields, 5, "AE");
  tdesc_add_flag (type_with_fields, 6, "DE");
  tdesc_add_flag (type_with_fields, 7, "U");
  tdesc_add_flag (type_with_fields, 8, "V");
  tdesc_add_flag (type_with_fields, 9, "C");
  tdesc_add_flag (type_with_fields, 10, "N");
  tdesc_add_flag (type_with_fields, 11, "Z");
  tdesc_add_flag (type_with_fields, 12, "L");
  tdesc_add_flag (type_with_fields, 13, "DZ");
  tdesc_add_flag (type_with_fields, 14, "SC");
  tdesc_add_flag (type_with_fields, 15, "ES");
  tdesc_add_bitfield (type_with_fields, "RB", 16, 18);
  tdesc_add_flag (type_with_fields, 19, "AD");
  tdesc_add_flag (type_with_fields, 20, "US");
  tdesc_add_flag (type_with_fields, 31, "IE");

  tdesc_create_reg (feature, "pc", regnum++, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "status32", regnum++, 1, NULL, 32, "status32_type");
  tdesc_create_reg (feature, "lp_start", regnum++, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "lp_end", regnum++, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "bta", regnum++, 1, NULL, 32, "code_ptr");
  return regnum;
}
