/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: 32bit-cpu.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_riscv_32bit_cpu (struct target_desc *result, long regnum)
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
  tdesc_create_reg (feature, "a6", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "a7", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "s2", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "s3", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "s4", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "s5", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "s6", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "s7", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "s8", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "s9", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "s10", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "s11", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "t3", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "t4", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "t5", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "t6", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "pc", regnum++, 1, NULL, 32, "code_ptr");
  return regnum;
}
