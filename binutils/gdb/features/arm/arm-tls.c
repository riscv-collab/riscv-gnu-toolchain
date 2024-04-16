/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: arm-tls.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_arm_arm_tls (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.arm.tls");
  tdesc_create_reg (feature, "tpidruro", regnum++, 1, NULL, 32, "data_ptr");
  return regnum;
}
