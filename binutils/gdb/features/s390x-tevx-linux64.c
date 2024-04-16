/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: s390x-tevx-linux64.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

const struct target_desc *tdesc_s390x_tevx_linux64;
static void
initialize_tdesc_s390x_tevx_linux64 (void)
{
  target_desc_up result = allocate_target_description ();
  set_tdesc_architecture (result.get (), bfd_scan_arch ("s390:64-bit"));

  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.s390.core");
  tdesc_create_reg (feature, "pswm", 0, 1, "psw", 64, "uint64");
  tdesc_create_reg (feature, "pswa", 1, 1, "psw", 64, "uint64");
  tdesc_create_reg (feature, "r0", 2, 1, "general", 64, "uint64");
  tdesc_create_reg (feature, "r1", 3, 1, "general", 64, "uint64");
  tdesc_create_reg (feature, "r2", 4, 1, "general", 64, "uint64");
  tdesc_create_reg (feature, "r3", 5, 1, "general", 64, "uint64");
  tdesc_create_reg (feature, "r4", 6, 1, "general", 64, "uint64");
  tdesc_create_reg (feature, "r5", 7, 1, "general", 64, "uint64");
  tdesc_create_reg (feature, "r6", 8, 1, "general", 64, "uint64");
  tdesc_create_reg (feature, "r7", 9, 1, "general", 64, "uint64");
  tdesc_create_reg (feature, "r8", 10, 1, "general", 64, "uint64");
  tdesc_create_reg (feature, "r9", 11, 1, "general", 64, "uint64");
  tdesc_create_reg (feature, "r10", 12, 1, "general", 64, "uint64");
  tdesc_create_reg (feature, "r11", 13, 1, "general", 64, "uint64");
  tdesc_create_reg (feature, "r12", 14, 1, "general", 64, "uint64");
  tdesc_create_reg (feature, "r13", 15, 1, "general", 64, "uint64");
  tdesc_create_reg (feature, "r14", 16, 1, "general", 64, "uint64");
  tdesc_create_reg (feature, "r15", 17, 1, "general", 64, "uint64");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.s390.acr");
  tdesc_create_reg (feature, "acr0", 18, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr1", 19, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr2", 20, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr3", 21, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr4", 22, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr5", 23, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr6", 24, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr7", 25, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr8", 26, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr9", 27, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr10", 28, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr11", 29, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr12", 30, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr13", 31, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr14", 32, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr15", 33, 1, "access", 32, "uint32");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.s390.fpr");
  tdesc_create_reg (feature, "fpc", 34, 1, "float", 32, "uint32");
  tdesc_create_reg (feature, "f0", 35, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f1", 36, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f2", 37, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f3", 38, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f4", 39, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f5", 40, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f6", 41, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f7", 42, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f8", 43, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f9", 44, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f10", 45, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f11", 46, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f12", 47, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f13", 48, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f14", 49, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f15", 50, 1, "float", 64, "ieee_double");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.s390.linux");
  tdesc_create_reg (feature, "orig_r2", 51, 1, "system", 64, "uint64");
  tdesc_create_reg (feature, "last_break", 52, 0, "system", 64, "code_ptr");
  tdesc_create_reg (feature, "system_call", 53, 1, "system", 32, "uint32");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.s390.tdb");
  tdesc_create_reg (feature, "tdb0", 54, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tac", 55, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tct", 56, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "atia", 57, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr0", 58, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr1", 59, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr2", 60, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr3", 61, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr4", 62, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr5", 63, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr6", 64, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr7", 65, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr8", 66, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr9", 67, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr10", 68, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr11", 69, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr12", 70, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr13", 71, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr14", 72, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr15", 73, 1, "tdb", 64, "uint64");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.s390.vx");
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

  tdesc_create_reg (feature, "v0l", 74, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v1l", 75, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v2l", 76, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v3l", 77, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v4l", 78, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v5l", 79, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v6l", 80, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v7l", 81, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v8l", 82, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v9l", 83, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v10l", 84, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v11l", 85, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v12l", 86, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v13l", 87, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v14l", 88, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v15l", 89, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "v16", 90, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "v17", 91, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "v18", 92, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "v19", 93, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "v20", 94, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "v21", 95, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "v22", 96, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "v23", 97, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "v24", 98, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "v25", 99, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "v26", 100, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "v27", 101, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "v28", 102, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "v29", 103, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "v30", 104, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "v31", 105, 1, NULL, 128, "vec128");

  tdesc_s390x_tevx_linux64 = result.release ();
}
