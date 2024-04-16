/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: microblaze.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

const struct target_desc *tdesc_microblaze;
static void
initialize_tdesc_microblaze (void)
{
  target_desc_up result = allocate_target_description ();
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.microblaze.core");
  tdesc_create_reg (feature, "r0", 0, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r1", 1, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "r2", 2, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r3", 3, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r4", 4, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r5", 5, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r6", 6, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r7", 7, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r8", 8, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r9", 9, 1, NULL, 32, "int");
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
  tdesc_create_reg (feature, "rpc", 32, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "rmsr", 33, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rear", 34, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "resr", 35, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rfsr", 36, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rbtr", 37, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rpvr0", 38, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rpvr1", 39, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rpvr2", 40, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rpvr3", 41, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rpvr4", 42, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rpvr5", 43, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rpvr6", 44, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rpvr7", 45, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rpvr8", 46, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rpvr9", 47, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rpvr10", 48, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rpvr11", 49, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "redr", 50, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rpid", 51, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rzpr", 52, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rtlbx", 53, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rtlbsx", 54, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rtlblo", 55, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "rtlbhi", 56, 1, NULL, 32, "int");

  tdesc_microblaze = result.release ();
}
