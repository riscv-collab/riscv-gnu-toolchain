/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: arm-m-profile-with-fpa.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_arm_arm_m_profile_with_fpa (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.arm.m-profile");
  tdesc_create_reg (feature, "r0", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r1", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r2", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r3", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r4", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r5", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r6", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r7", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r8", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r9", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r10", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r11", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r12", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sp", regnum++, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "lr", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "pc", regnum++, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", regnum++, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "xpsr", regnum++, 1, NULL, 32, "int");
  return regnum;
}
