/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: or1k-linux.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

const struct target_desc *tdesc_or1k_linux;
static void
initialize_tdesc_or1k_linux (void)
{
  target_desc_up result = allocate_target_description ();
  set_tdesc_architecture (result.get (), bfd_scan_arch ("or1k"));

  set_tdesc_osabi (result.get (), osabi_from_tdesc_string ("GNU/Linux"));

  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.or1k.group0");
  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_flags (feature, "sr_flags", 4);
  tdesc_add_flag (type_with_fields, 0, "SM");
  tdesc_add_flag (type_with_fields, 1, "TEE");
  tdesc_add_flag (type_with_fields, 2, "IEE");
  tdesc_add_flag (type_with_fields, 3, "DCE");
  tdesc_add_flag (type_with_fields, 4, "ICE");
  tdesc_add_flag (type_with_fields, 5, "DME");
  tdesc_add_flag (type_with_fields, 6, "IME");
  tdesc_add_flag (type_with_fields, 7, "LEE");
  tdesc_add_flag (type_with_fields, 8, "CE");
  tdesc_add_flag (type_with_fields, 9, "F");
  tdesc_add_flag (type_with_fields, 10, "CY");
  tdesc_add_flag (type_with_fields, 11, "OV");
  tdesc_add_flag (type_with_fields, 12, "OVE");
  tdesc_add_flag (type_with_fields, 13, "DSX");
  tdesc_add_flag (type_with_fields, 14, "EPH");
  tdesc_add_flag (type_with_fields, 15, "FO");
  tdesc_add_flag (type_with_fields, 16, "SUMRA");
  tdesc_add_bitfield (type_with_fields, "CID", 28, 31);

  tdesc_create_reg (feature, "r0", 0, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r1", 1, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "r2", 2, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "r3", 3, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r4", 4, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r5", 5, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r6", 6, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r7", 7, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r8", 8, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r9", 9, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "r10", 10, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r11", 11, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r12", 12, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r13", 13, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r14", 14, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r15", 15, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r16", 16, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r17", 17, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r18", 18, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r19", 19, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r20", 20, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r21", 21, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r22", 22, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r23", 23, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r24", 24, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r25", 25, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r26", 26, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r27", 27, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r28", 28, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r29", 29, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r30", 30, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r31", 31, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ppc", 32, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "npc", 33, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "sr", 34, 1, NULL, 32, "sr_flags");

  tdesc_or1k_linux = result.release ();
}
