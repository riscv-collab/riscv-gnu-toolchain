/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: rx.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

const struct target_desc *tdesc_rx;
static void
initialize_tdesc_rx (void)
{
  target_desc_up result = allocate_target_description ();
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.rx.core");
  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_flags (feature, "psw_flags", 4);
  tdesc_add_flag (type_with_fields, 0, "C");
  tdesc_add_flag (type_with_fields, 1, "Z");
  tdesc_add_flag (type_with_fields, 2, "S");
  tdesc_add_flag (type_with_fields, 3, "O");
  tdesc_add_flag (type_with_fields, 16, "I");
  tdesc_add_flag (type_with_fields, 17, "U");
  tdesc_add_flag (type_with_fields, 20, "PM");
  tdesc_add_flag (type_with_fields, 24, "IPL0");
  tdesc_add_flag (type_with_fields, 25, "IPL1");
  tdesc_add_flag (type_with_fields, 26, "IPL2");
  tdesc_add_flag (type_with_fields, 27, "IPL3");

  type_with_fields = tdesc_create_flags (feature, "fpsw_flags", 4);
  tdesc_add_flag (type_with_fields, 0, "RM0");
  tdesc_add_flag (type_with_fields, 1, "RM1");
  tdesc_add_flag (type_with_fields, 2, "CV");
  tdesc_add_flag (type_with_fields, 3, "CO");
  tdesc_add_flag (type_with_fields, 4, "CZ");
  tdesc_add_flag (type_with_fields, 5, "CU");
  tdesc_add_flag (type_with_fields, 6, "CX");
  tdesc_add_flag (type_with_fields, 7, "CE");
  tdesc_add_flag (type_with_fields, 8, "DN");
  tdesc_add_flag (type_with_fields, 10, "EV");
  tdesc_add_flag (type_with_fields, 11, "EO");
  tdesc_add_flag (type_with_fields, 12, "EZ");
  tdesc_add_flag (type_with_fields, 13, "EU");
  tdesc_add_flag (type_with_fields, 14, "EX");
  tdesc_add_flag (type_with_fields, 26, "FV");
  tdesc_add_flag (type_with_fields, 27, "FO");
  tdesc_add_flag (type_with_fields, 28, "FZ");
  tdesc_add_flag (type_with_fields, 29, "FU");
  tdesc_add_flag (type_with_fields, 30, "FX");
  tdesc_add_flag (type_with_fields, 31, "FS");

  tdesc_create_reg (feature, "r0", 0, 1, NULL, 32, "data_ptr");
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
  tdesc_create_reg (feature, "usp", 16, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "isp", 17, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "psw", 18, 1, NULL, 32, "psw_flags");
  tdesc_create_reg (feature, "pc", 19, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "intb", 20, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "bpsw", 21, 1, NULL, 32, "psw_flags");
  tdesc_create_reg (feature, "bpc", 22, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "fintv", 23, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "fpsw", 24, 1, NULL, 32, "fpsw_flags");
  tdesc_create_reg (feature, "acc", 25, 1, NULL, 64, "uint64");

  tdesc_rx = result.release ();
}
