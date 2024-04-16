/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: arm-m-profile-mve.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_arm_arm_m_profile_mve (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.arm.m-profile-mve");
  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_flags (feature, "vpr_reg", 4);
  tdesc_add_bitfield (type_with_fields, "P0", 0, 15);
  tdesc_add_bitfield (type_with_fields, "MASK01", 16, 19);
  tdesc_add_bitfield (type_with_fields, "MASK23", 20, 23);

  tdesc_create_reg (feature, "vpr", regnum++, 1, NULL, 32, "vpr_reg");
  return regnum;
}
