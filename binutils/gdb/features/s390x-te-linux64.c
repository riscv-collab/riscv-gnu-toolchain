/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: s390x-te-linux64.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

const struct target_desc *tdesc_s390x_te_linux64;
static void
initialize_tdesc_s390x_te_linux64 (void)
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

  tdesc_s390x_te_linux64 = result.release ();
}
