/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: arm-m-system.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_arm_arm_m_system (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.arm.m-system");
  tdesc_create_reg (feature, "msp", regnum++, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "psp", regnum++, 1, NULL, 32, "data_ptr");
  return regnum;
}
