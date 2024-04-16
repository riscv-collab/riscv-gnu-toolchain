/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: tic6x-c6xp.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_tic6x_c6xp (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.tic6x.c6xp");
  tdesc_create_reg (feature, "TSR", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "ILC", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "RILC", regnum++, 1, NULL, 32, "uint32");
  return regnum;
}
