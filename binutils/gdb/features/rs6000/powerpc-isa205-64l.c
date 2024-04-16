/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: powerpc-isa205-64l.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

const struct target_desc *tdesc_powerpc_isa205_64l;
static void
initialize_tdesc_powerpc_isa205_64l (void)
{
  target_desc_up result = allocate_target_description ();
  set_tdesc_architecture (result.get (), bfd_scan_arch ("powerpc:common64"));

  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.power.core");
  tdesc_create_reg (feature, "r0", 0, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r1", 1, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r2", 2, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r3", 3, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r4", 4, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r5", 5, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r6", 6, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r7", 7, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r8", 8, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r9", 9, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r10", 10, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r11", 11, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r12", 12, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r13", 13, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r14", 14, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r15", 15, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r16", 16, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r17", 17, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r18", 18, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r19", 19, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r20", 20, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r21", 21, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r22", 22, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r23", 23, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r24", 24, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r25", 25, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r26", 26, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r27", 27, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r28", 28, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r29", 29, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r30", 30, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "r31", 31, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "pc", 64, 1, NULL, 64, "code_ptr");
  tdesc_create_reg (feature, "msr", 65, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "cr", 66, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "lr", 67, 1, NULL, 64, "code_ptr");
  tdesc_create_reg (feature, "ctr", 68, 1, NULL, 64, "uint64");
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
  tdesc_create_reg (feature, "orig_r3", 71, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "trap", 72, 1, NULL, 64, "int");

  tdesc_powerpc_isa205_64l = result.release ();
}
