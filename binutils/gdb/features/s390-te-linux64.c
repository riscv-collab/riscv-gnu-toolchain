/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: s390-te-linux64.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

const struct target_desc *tdesc_s390_te_linux64;
static void
initialize_tdesc_s390_te_linux64 (void)
{
  target_desc_up result = allocate_target_description ();
  set_tdesc_architecture (result.get (), bfd_scan_arch ("s390:31-bit"));

  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.s390.core");
  tdesc_create_reg (feature, "pswm", 0, 1, "psw", 32, "uint32");
  tdesc_create_reg (feature, "pswa", 1, 1, "psw", 32, "uint32");
  tdesc_create_reg (feature, "r0h", 2, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r0l", 3, 1, "lower", 32, "uint32");
  tdesc_create_reg (feature, "r1h", 4, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r1l", 5, 1, "lower", 32, "uint32");
  tdesc_create_reg (feature, "r2h", 6, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r2l", 7, 1, "lower", 32, "uint32");
  tdesc_create_reg (feature, "r3h", 8, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r3l", 9, 1, "lower", 32, "uint32");
  tdesc_create_reg (feature, "r4h", 10, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r4l", 11, 1, "lower", 32, "uint32");
  tdesc_create_reg (feature, "r5h", 12, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r5l", 13, 1, "lower", 32, "uint32");
  tdesc_create_reg (feature, "r6h", 14, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r6l", 15, 1, "lower", 32, "uint32");
  tdesc_create_reg (feature, "r7h", 16, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r7l", 17, 1, "lower", 32, "uint32");
  tdesc_create_reg (feature, "r8h", 18, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r8l", 19, 1, "lower", 32, "uint32");
  tdesc_create_reg (feature, "r9h", 20, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r9l", 21, 1, "lower", 32, "uint32");
  tdesc_create_reg (feature, "r10h", 22, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r10l", 23, 1, "lower", 32, "uint32");
  tdesc_create_reg (feature, "r11h", 24, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r11l", 25, 1, "lower", 32, "uint32");
  tdesc_create_reg (feature, "r12h", 26, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r12l", 27, 1, "lower", 32, "uint32");
  tdesc_create_reg (feature, "r13h", 28, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r13l", 29, 1, "lower", 32, "uint32");
  tdesc_create_reg (feature, "r14h", 30, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r14l", 31, 1, "lower", 32, "uint32");
  tdesc_create_reg (feature, "r15h", 32, 1, "upper", 32, "uint32");
  tdesc_create_reg (feature, "r15l", 33, 1, "lower", 32, "uint32");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.s390.acr");
  tdesc_create_reg (feature, "acr0", 34, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr1", 35, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr2", 36, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr3", 37, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr4", 38, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr5", 39, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr6", 40, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr7", 41, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr8", 42, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr9", 43, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr10", 44, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr11", 45, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr12", 46, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr13", 47, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr14", 48, 1, "access", 32, "uint32");
  tdesc_create_reg (feature, "acr15", 49, 1, "access", 32, "uint32");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.s390.fpr");
  tdesc_create_reg (feature, "fpc", 50, 1, "float", 32, "uint32");
  tdesc_create_reg (feature, "f0", 51, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f1", 52, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f2", 53, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f3", 54, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f4", 55, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f5", 56, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f6", 57, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f7", 58, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f8", 59, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f9", 60, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f10", 61, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f11", 62, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f12", 63, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f13", 64, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f14", 65, 1, "float", 64, "ieee_double");
  tdesc_create_reg (feature, "f15", 66, 1, "float", 64, "ieee_double");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.s390.linux");
  tdesc_create_reg (feature, "orig_r2", 67, 1, "system", 32, "uint32");
  tdesc_create_reg (feature, "last_break", 68, 0, "system", 32, "code_ptr");
  tdesc_create_reg (feature, "system_call", 69, 1, "system", 32, "uint32");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.s390.tdb");
  tdesc_create_reg (feature, "tdb0", 70, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tac", 71, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tct", 72, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "atia", 73, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr0", 74, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr1", 75, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr2", 76, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr3", 77, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr4", 78, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr5", 79, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr6", 80, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr7", 81, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr8", 82, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr9", 83, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr10", 84, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr11", 85, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr12", 86, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr13", 87, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr14", 88, 1, "tdb", 64, "uint64");
  tdesc_create_reg (feature, "tr15", 89, 1, "tdb", 64, "uint64");

  tdesc_s390_te_linux64 = result.release ();
}
