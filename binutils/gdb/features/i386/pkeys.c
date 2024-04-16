/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: pkeys.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_i386_pkeys (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.i386.pkeys");
  tdesc_create_reg (feature, "pkru", regnum++, 1, NULL, 32, "uint32");
  return regnum;
}
