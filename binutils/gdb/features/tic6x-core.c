/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: tic6x-core.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_tic6x_core (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.tic6x.core");
  tdesc_create_reg (feature, "A0", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A1", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A2", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A3", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A4", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A5", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A6", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A7", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A8", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A9", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A10", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A11", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A12", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A13", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A14", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A15", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B0", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B1", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B2", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B3", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B4", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B5", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B6", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B7", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B8", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B9", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B10", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B11", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B12", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B13", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B14", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B15", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "CSR", regnum++, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "PC", regnum++, 1, NULL, 32, "code_ptr");
  return regnum;
}
