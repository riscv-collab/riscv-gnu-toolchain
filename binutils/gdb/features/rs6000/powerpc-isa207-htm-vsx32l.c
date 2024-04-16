/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: powerpc-isa207-htm-vsx32l.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

const struct target_desc *tdesc_powerpc_isa207_htm_vsx32l;
static void
initialize_tdesc_powerpc_isa207_htm_vsx32l (void)
{
  target_desc_up result = allocate_target_description ();
  set_tdesc_architecture (result.get (), bfd_scan_arch ("powerpc:common"));

  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.core");
  tdesc_create_reg (feature, "r0", 0, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r1", 1, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r2", 2, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r3", 3, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r4", 4, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r5", 5, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r6", 6, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r7", 7, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r8", 8, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r9", 9, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r10", 10, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r11", 11, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r12", 12, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r13", 13, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r14", 14, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r15", 15, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r16", 16, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r17", 17, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r18", 18, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r19", 19, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r20", 20, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r21", 21, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r22", 22, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r23", 23, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r24", 24, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r25", 25, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r26", 26, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r27", 27, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r28", 28, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r29", 29, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r30", 30, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r31", 31, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "pc", 64, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "msr", 65, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr", 66, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "lr", 67, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "ctr", 68, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "xer", 69, 1, NULL, 32, "uint32");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.fpu");
  tdesc_create_reg (feature, "f0", 32, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f1", 33, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f2", 34, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f3", 35, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f4", 36, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f5", 37, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f6", 38, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f7", 39, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f8", 40, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f9", 41, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f10", 42, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f11", 43, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f12", 44, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f13", 45, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f14", 46, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f15", 47, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f16", 48, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f17", 49, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f18", 50, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f19", 51, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f20", 52, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f21", 53, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f22", 54, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f23", 55, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f24", 56, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f25", 57, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f26", 58, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f27", 59, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f28", 60, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f29", 61, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f30", 62, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f31", 63, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fpscr", 70, 1, "float", 64, "int");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.linux");
  tdesc_create_reg (feature, "orig_r3", 71, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "trap", 72, 1, NULL, 32, "int");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.altivec");
  tdesc_type *element_type;
  element_type = tdesc_named_type (feature, "ieee_single");
  tdesc_create_vector (feature, "v4f", element_type, 4);

  element_type = tdesc_named_type (feature, "int32");
  tdesc_create_vector (feature, "v4i32", element_type, 4);

  element_type = tdesc_named_type (feature, "int16");
  tdesc_create_vector (feature, "v8i16", element_type, 8);

  element_type = tdesc_named_type (feature, "int8");
  tdesc_create_vector (feature, "v16i8", element_type, 16);

  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_union (feature, "vec128");
  tdesc_type *field_type;
  field_type = tdesc_named_type (feature, "uint128");
  tdesc_add_field (type_with_fields, "uint128", field_type);
  field_type = tdesc_named_type (feature, "v4f");
  tdesc_add_field (type_with_fields, "v4_float", field_type);
  field_type = tdesc_named_type (feature, "v4i32");
  tdesc_add_field (type_with_fields, "v4_int32", field_type);
  field_type = tdesc_named_type (feature, "v8i16");
  tdesc_add_field (type_with_fields, "v8_int16", field_type);
  field_type = tdesc_named_type (feature, "v16i8");
  tdesc_add_field (type_with_fields, "v16_int8", field_type);

  tdesc_create_reg (feature, "vr0", 73, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr1", 74, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr2", 75, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr3", 76, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr4", 77, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr5", 78, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr6", 79, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr7", 80, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr8", 81, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr9", 82, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr10", 83, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr11", 84, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr12", 85, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr13", 86, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr14", 87, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr15", 88, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr16", 89, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr17", 90, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr18", 91, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr19", 92, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr20", 93, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr21", 94, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr22", 95, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr23", 96, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr24", 97, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr25", 98, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr26", 99, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr27", 100, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr28", 101, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr29", 102, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr30", 103, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr31", 104, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vscr", 105, 1, "vector", 32, "int");
  tdesc_create_reg (feature, "vrsave", 106, 1, "vector", 32, "int");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.vsx");
  tdesc_create_reg (feature, "vs0h", 107, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs1h", 108, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs2h", 109, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs3h", 110, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs4h", 111, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs5h", 112, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs6h", 113, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs7h", 114, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs8h", 115, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs9h", 116, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs10h", 117, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs11h", 118, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs12h", 119, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs13h", 120, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs14h", 121, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs15h", 122, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs16h", 123, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs17h", 124, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs18h", 125, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs19h", 126, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs20h", 127, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs21h", 128, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs22h", 129, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs23h", 130, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs24h", 131, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs25h", 132, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs26h", 133, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs27h", 134, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs28h", 135, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs29h", 136, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs30h", 137, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs31h", 138, 1, NULL, 64, "uint64");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.ppr");
  tdesc_create_reg (feature, "ppr", 139, 1, NULL, 64, "uint64");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.dscr");
  tdesc_create_reg (feature, "dscr", 140, 1, NULL, 64, "uint64");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.tar");
  tdesc_create_reg (feature, "tar", 141, 1, NULL, 64, "uint64");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.ebb");
  tdesc_create_reg (feature, "bescr", 142, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "ebbhr", 143, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "ebbrr", 144, 0, NULL, 64, "uint64");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.linux.pmu");
  tdesc_create_reg (feature, "mmcr0", 145, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "mmcr2", 146, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "siar", 147, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "sdar", 148, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "sier", 149, 0, NULL, 64, "uint64");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.htm.spr");
  tdesc_create_reg (feature, "tfhar", 150, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "texasr", 151, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "tfiar", 152, 0, NULL, 64, "uint64");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.htm.core");
  tdesc_create_reg (feature, "cr0", 153, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr1", 154, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr2", 155, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr3", 156, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr4", 157, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr5", 158, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr6", 159, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr7", 160, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr8", 161, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr9", 162, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr10", 163, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr11", 164, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr12", 165, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr13", 166, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr14", 167, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr15", 168, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr16", 169, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr17", 170, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr18", 171, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr19", 172, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr20", 173, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr21", 174, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr22", 175, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr23", 176, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr24", 177, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr25", 178, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr26", 179, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr27", 180, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr28", 181, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr29", 182, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr30", 183, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr31", 184, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "ccr", 185, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cxer", 186, 0, NULL, 32, "uint32");
  tdesc_create_reg (feature, "clr", 187, 0, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "cctr", 188, 0, NULL, 32, "uint32");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.htm.fpu");
  tdesc_create_reg (feature, "cf0", 189, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf1", 190, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf2", 191, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf3", 192, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf4", 193, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf5", 194, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf6", 195, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf7", 196, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf8", 197, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf9", 198, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf10", 199, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf11", 200, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf12", 201, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf13", 202, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf14", 203, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf15", 204, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf16", 205, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf17", 206, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf18", 207, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf19", 208, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf20", 209, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf21", 210, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf22", 211, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf23", 212, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf24", 213, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf25", 214, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf26", 215, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf27", 216, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf28", 217, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf29", 218, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf30", 219, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cf31", 220, 0, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "cfpscr", 221, 0, "float", 64, "uint64");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.htm.altivec");
  element_type = tdesc_named_type (feature, "ieee_single");
  tdesc_create_vector (feature, "v4f", element_type, 4);

  element_type = tdesc_named_type (feature, "int32");
  tdesc_create_vector (feature, "v4i32", element_type, 4);

  element_type = tdesc_named_type (feature, "int16");
  tdesc_create_vector (feature, "v8i16", element_type, 8);

  element_type = tdesc_named_type (feature, "int8");
  tdesc_create_vector (feature, "v16i8", element_type, 16);

  type_with_fields = tdesc_create_union (feature, "vec128");
  field_type = tdesc_named_type (feature, "uint128");
  tdesc_add_field (type_with_fields, "uint128", field_type);
  field_type = tdesc_named_type (feature, "v4f");
  tdesc_add_field (type_with_fields, "v4_float", field_type);
  field_type = tdesc_named_type (feature, "v4i32");
  tdesc_add_field (type_with_fields, "v4_int32", field_type);
  field_type = tdesc_named_type (feature, "v8i16");
  tdesc_add_field (type_with_fields, "v8_int16", field_type);
  field_type = tdesc_named_type (feature, "v16i8");
  tdesc_add_field (type_with_fields, "v16_int8", field_type);

  tdesc_create_reg (feature, "cvr0", 222, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr1", 223, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr2", 224, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr3", 225, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr4", 226, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr5", 227, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr6", 228, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr7", 229, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr8", 230, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr9", 231, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr10", 232, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr11", 233, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr12", 234, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr13", 235, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr14", 236, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr15", 237, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr16", 238, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr17", 239, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr18", 240, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr19", 241, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr20", 242, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr21", 243, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr22", 244, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr23", 245, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr24", 246, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr25", 247, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr26", 248, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr27", 249, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr28", 250, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr29", 251, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr30", 252, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvr31", 253, 0, NULL, 128, "vec128");
  tdesc_create_reg (feature, "cvscr", 254, 0, "vector", 32, "int");
  tdesc_create_reg (feature, "cvrsave", 255, 0, "vector", 32, "int");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.htm.vsx");
  tdesc_create_reg (feature, "cvs0h", 256, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs1h", 257, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs2h", 258, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs3h", 259, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs4h", 260, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs5h", 261, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs6h", 262, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs7h", 263, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs8h", 264, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs9h", 265, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs10h", 266, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs11h", 267, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs12h", 268, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs13h", 269, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs14h", 270, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs15h", 271, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs16h", 272, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs17h", 273, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs18h", 274, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs19h", 275, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs20h", 276, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs21h", 277, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs22h", 278, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs23h", 279, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs24h", 280, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs25h", 281, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs26h", 282, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs27h", 283, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs28h", 284, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs29h", 285, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs30h", 286, 0, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cvs31h", 287, 0, NULL, 64, "uint64");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.htm.ppr");
  tdesc_create_reg (feature, "cppr", 288, 0, NULL, 64, "uint64");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.htm.dscr");
  tdesc_create_reg (feature, "cdscr", 289, 0, NULL, 64, "uint64");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.htm.tar");
  tdesc_create_reg (feature, "ctar", 290, 0, NULL, 64, "uint64");

  tdesc_powerpc_isa207_htm_vsx32l = result.release ();
}
