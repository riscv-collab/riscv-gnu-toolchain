/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: 32bit-avx512.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_i386_32bit_avx512 (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.i386.avx512");
  tdesc_type *element_type;
  element_type = tdesc_named_type (feature, "uint128");
  tdesc_create_vector (feature, "v2ui128", element_type, 2);

  tdesc_create_reg (feature, "k0", regnum++, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "k1", regnum++, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "k2", regnum++, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "k3", regnum++, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "k4", regnum++, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "k5", regnum++, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "k6", regnum++, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "k7", regnum++, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "zmm0h", regnum++, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm1h", regnum++, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm2h", regnum++, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm3h", regnum++, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm4h", regnum++, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm5h", regnum++, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm6h", regnum++, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm7h", regnum++, 1, NULL, 256, "v2ui128");
  return regnum;
}
