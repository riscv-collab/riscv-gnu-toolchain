/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: sparc64-solaris.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

const struct target_desc *tdesc_sparc64_solaris;
static void
initialize_tdesc_sparc64_solaris (void)
{
  target_desc_up result = allocate_target_description ();
  set_tdesc_architecture (result.get (), bfd_scan_arch ("sparc:v9"));

  set_tdesc_osabi (result.get (), osabi_from_tdesc_string ("Solaris"));

  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.sparc.cpu");
  tdesc_create_reg (feature, "g0", 0, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "g1", 1, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "g2", 2, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "g3", 3, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "g4", 4, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "g5", 5, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "g6", 6, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "g7", 7, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "o0", 8, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "o1", 9, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "o2", 10, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "o3", 11, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "o4", 12, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "o5", 13, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "sp", 14, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "o7", 15, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "l0", 16, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "l1", 17, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "l2", 18, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "l3", 19, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "l4", 20, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "l5", 21, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "l6", 22, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "l7", 23, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "i0", 24, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "i1", 25, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "i2", 26, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "i3", 27, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "i4", 28, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "i5", 29, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "fp", 30, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "i7", 31, 1, NULL, 64, "uint64");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.sparc.cp0");
  tdesc_create_reg (feature, "pc", 80, 1, NULL, 64, "code_ptr");
  tdesc_create_reg (feature, "npc", 81, 1, NULL, 64, "code_ptr");
  tdesc_create_reg (feature, "state", 82, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "fsr", 83, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "fprs", 84, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "y", 85, 1, NULL, 64, "uint64");

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.sparc.fpu");
  tdesc_create_reg (feature, "f0", 32, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f1", 33, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f2", 34, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f3", 35, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f4", 36, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f5", 37, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f6", 38, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f7", 39, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f8", 40, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f9", 41, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f10", 42, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f11", 43, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f12", 44, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f13", 45, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f14", 46, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f15", 47, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f16", 48, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f17", 49, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f18", 50, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f19", 51, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f20", 52, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f21", 53, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f22", 54, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f23", 55, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f24", 56, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f25", 57, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f26", 58, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f27", 59, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f28", 60, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f29", 61, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f30", 62, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f31", 63, 1, NULL, 32, "ieee_single");
  tdesc_create_reg (feature, "f32", 64, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f34", 65, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f36", 66, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f38", 67, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f40", 68, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f42", 69, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f44", 70, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f46", 71, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f48", 72, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f50", 73, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f52", 74, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f54", 75, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f56", 76, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f58", 77, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f60", 78, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f62", 79, 1, NULL, 64, "ieee_double");

  tdesc_sparc64_solaris = result.release ();
}
