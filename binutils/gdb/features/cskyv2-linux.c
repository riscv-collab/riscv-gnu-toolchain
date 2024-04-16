/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: cskyv2-linux.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_cskyv2_linux (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.csky.abiv2");
  tdesc_type *element_type;
  element_type = tdesc_named_type (feature, "ieee_single");
  tdesc_create_vector (feature, "v4f", element_type, 4);

  element_type = tdesc_named_type (feature, "ieee_double");
  tdesc_create_vector (feature, "v2d", element_type, 2);

  element_type = tdesc_named_type (feature, "int8");
  tdesc_create_vector (feature, "v16i8", element_type, 16);

  element_type = tdesc_named_type (feature, "int16");
  tdesc_create_vector (feature, "v8i16", element_type, 8);

  element_type = tdesc_named_type (feature, "int32");
  tdesc_create_vector (feature, "v4i32", element_type, 4);

  element_type = tdesc_named_type (feature, "int64");
  tdesc_create_vector (feature, "v2i64", element_type, 2);

  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_union (feature, "vec128");
  tdesc_type *field_type;
  field_type = tdesc_named_type (feature, "v4f");
  tdesc_add_field (type_with_fields, "v4_float", field_type);
  field_type = tdesc_named_type (feature, "v2d");
  tdesc_add_field (type_with_fields, "v2_double", field_type);
  field_type = tdesc_named_type (feature, "v16i8");
  tdesc_add_field (type_with_fields, "v16_int8", field_type);
  field_type = tdesc_named_type (feature, "v8i16");
  tdesc_add_field (type_with_fields, "v8_int16", field_type);
  field_type = tdesc_named_type (feature, "v4i32");
  tdesc_add_field (type_with_fields, "v4_int32", field_type);
  field_type = tdesc_named_type (feature, "v2i64");
  tdesc_add_field (type_with_fields, "v2_int64", field_type);
  field_type = tdesc_named_type (feature, "uint128");
  tdesc_add_field (type_with_fields, "uint128", field_type);

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
  tdesc_create_reg (feature, "r13", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r14", regnum++, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "r15", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r16", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r17", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r18", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r19", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r20", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r21", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r22", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r23", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r24", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r25", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r26", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r27", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r28", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r29", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r30", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r31", regnum++, 1, NULL, 32, "int");
  regnum = 36;
  tdesc_create_reg (feature, "hi", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "lo", regnum++, 1, NULL, 32, "int");
  regnum = 40;
  tdesc_create_reg (feature, "fr0", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fr1", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fr2", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fr3", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fr4", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fr5", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fr6", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fr7", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fr8", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fr9", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fr10", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fr11", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fr12", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fr13", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fr14", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fr15", regnum++, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "vr0", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr1", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr2", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr3", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr4", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr5", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr6", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr7", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr8", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr9", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr10", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr11", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr12", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr13", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr14", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr15", regnum++, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "pc", regnum++, 1, NULL, 32, "code_ptr");
  regnum = 89;
  tdesc_create_reg (feature, "psr", regnum++, 1, NULL, 32, "int");
  regnum = 121;
  tdesc_create_reg (feature, "fid", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "fcr", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "fesr", regnum++, 1, NULL, 32, "int");
  regnum = 127;
  tdesc_create_reg (feature, "usp", regnum++, 1, NULL, 32, "int");
  return regnum;
}
