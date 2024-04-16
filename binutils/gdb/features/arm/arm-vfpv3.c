/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: arm-vfpv3.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_arm_arm_vfpv3 (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.arm.vfp");
  tdesc_create_reg (feature, "d0", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d1", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d2", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d3", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d4", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d5", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d6", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d7", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d8", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d9", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d10", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d11", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d12", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d13", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d14", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d15", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d16", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d17", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d18", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d19", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d20", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d21", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d22", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d23", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d24", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d25", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d26", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d27", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d28", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d29", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d30", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d31", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fpscr", regnum++, 1, "float", 32, "int");
  return regnum;
}
