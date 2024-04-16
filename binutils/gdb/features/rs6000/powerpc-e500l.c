/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: powerpc-e500l.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

const struct target_desc *tdesc_powerpc_e500l;
static void
initialize_tdesc_powerpc_e500l (void)
{
  target_desc_up result = allocate_target_description ();
  set_tdesc_architecture (result.get (), bfd_scan_arch ("powerpc:e500"));

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

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.spe");
  tdesc_create_reg (feature, "ev0h", 32, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev1h", 33, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev2h", 34, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev3h", 35, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev4h", 36, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev5h", 37, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev6h", 38, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev7h", 39, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev8h", 40, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev9h", 41, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev10h", 42, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev11h", 43, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev12h", 44, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev13h", 45, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev14h", 46, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev15h", 47, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev16h", 48, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev17h", 49, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev18h", 50, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev19h", 51, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev20h", 52, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev21h", 53, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev22h", 54, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev23h", 55, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev24h", 56, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev25h", 57, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev26h", 58, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev27h", 59, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev28h", 60, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev29h", 61, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev30h", 62, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ev31h", 63, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "acc", 73, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "spefscr", 74, 1, NULL, 32, "int");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.linux");
  tdesc_create_reg (feature, "orig_r3", 71, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "trap", 72, 1, NULL, 32, "int");

  tdesc_powerpc_e500l = result.release ();
}
