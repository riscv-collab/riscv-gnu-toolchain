/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: rv32e-xregs.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_riscv_rv32e_xregs (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.riscv.cpu");
  tdesc_create_reg (feature, "zero", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ra", regnum++, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "sp", regnum++, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "gp", regnum++, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "tp", regnum++, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "t0", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "t1", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "t2", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "fp", regnum++, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "s1", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "a0", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "a1", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "a2", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "a3", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "a4", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "a5", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "pc", regnum++, 1, NULL, 32, "code_ptr");
  return regnum;
}
