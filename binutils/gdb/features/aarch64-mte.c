/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: aarch64-mte.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_aarch64_mte (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.aarch64.mte");
  tdesc_create_reg (feature, "tag_ctl", regnum++, 0, "system", 64, "uint64");
  return regnum;
}
