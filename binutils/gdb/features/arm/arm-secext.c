/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: arm-secext.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_arm_arm_secext (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.arm.secext");
  tdesc_create_reg (feature, "msp_ns", regnum++, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "psp_ns", regnum++, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "msp_s", regnum++, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "psp_s", regnum++, 1, NULL, 32, "data_ptr");
  return regnum;
}
