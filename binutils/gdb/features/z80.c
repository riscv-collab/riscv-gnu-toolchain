/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: z80.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

const struct target_desc *tdesc_z80;
static void
initialize_tdesc_z80 (void)
{
  target_desc_up result = allocate_target_description ();
  set_tdesc_architecture (result.get (), bfd_scan_arch ("z80"));

  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.z80.cpu");
  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_flags (feature, "af_flags", 2);
  tdesc_add_flag (type_with_fields, 0, "C");
  tdesc_add_flag (type_with_fields, 1, "N");
  tdesc_add_flag (type_with_fields, 2, "P/V");
  tdesc_add_flag (type_with_fields, 3, "F3");
  tdesc_add_flag (type_with_fields, 4, "H");
  tdesc_add_flag (type_with_fields, 5, "F5");
  tdesc_add_flag (type_with_fields, 6, "Z");
  tdesc_add_flag (type_with_fields, 7, "S");

  tdesc_create_reg (feature, "af", 0, 1, NULL, 16, "af_flags");
  tdesc_create_reg (feature, "bc", 1, 1, NULL, 16, "uint16");
  tdesc_create_reg (feature, "de", 2, 1, NULL, 16, "data_ptr");
  tdesc_create_reg (feature, "hl", 3, 1, NULL, 16, "data_ptr");
  tdesc_create_reg (feature, "sp", 4, 1, NULL, 16, "data_ptr");
  tdesc_create_reg (feature, "pc", 5, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "ix", 6, 1, NULL, 16, "data_ptr");
  tdesc_create_reg (feature, "iy", 7, 1, NULL, 16, "data_ptr");
  tdesc_create_reg (feature, "af'", 8, 1, NULL, 16, "af_flags");
  tdesc_create_reg (feature, "bc'", 9, 1, NULL, 16, "uint16");
  tdesc_create_reg (feature, "de'", 10, 1, NULL, 16, "data_ptr");
  tdesc_create_reg (feature, "hl'", 11, 1, NULL, 16, "data_ptr");
  tdesc_create_reg (feature, "ir", 12, 1, NULL, 16, "uint16");

  tdesc_z80 = result.release ();
}
