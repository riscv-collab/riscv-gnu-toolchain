/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: 32bit-linux.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_i386_32bit_linux (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.i386.linux");
  regnum = 41;
  tdesc_create_reg (feature, "orig_eax", regnum++, 1, NULL, 32, "int");
  return regnum;
}
