/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: tic6x-gp.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_tic6x_gp (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.tic6x.gp");
  tdesc_create_reg (feature, "A16", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A17", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A18", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A19", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A20", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A21", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A22", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A23", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A24", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A25", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A26", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A27", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A28", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A29", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A30", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A31", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B16", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B17", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B18", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B19", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B20", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B21", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B22", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B23", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B24", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B25", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B26", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B27", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B28", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B29", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B30", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B31", regnum++, 1, NULL, 32, "uint32");
  return regnum;
}
