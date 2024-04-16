/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: aarch64-pauth.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_aarch64_pauth (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.aarch64.pauth");
  tdesc_create_reg (feature, "pauth_dmask", regnum++, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "pauth_cmask", regnum++, 1, NULL, 64, "int");
  return regnum;
}
