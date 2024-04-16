/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: arm-fpa.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_arm_arm_fpa (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.arm.fpa");
  regnum = 16;
  tdesc_create_reg (feature, "f0", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "f1", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "f2", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "f3", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "f4", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "f5", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "f6", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "f7", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "fps", regnum++, 1, NULL, 32, "int");
  return regnum;
}
