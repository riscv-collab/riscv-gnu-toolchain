/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: powerpc-505.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

const struct target_desc *tdesc_powerpc_505;
static void
initialize_tdesc_powerpc_505 (void)
{
  target_desc_up result = allocate_target_description ();
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
  tdesc_create_reg (feature, "fpscr", 70, 1, "float", 32, "int");

  feature = tdesc_create_feature (result.get (), "OEA");
  tdesc_create_reg (feature, "sr0", 71, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sr1", 72, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sr2", 73, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sr3", 74, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sr4", 75, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sr5", 76, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sr6", 77, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sr7", 78, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sr8", 79, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sr9", 80, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sr10", 81, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sr11", 82, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sr12", 83, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sr13", 84, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sr14", 85, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sr15", 86, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "pvr", 87, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ibat0u", 88, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ibat0l", 89, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ibat1u", 90, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ibat1l", 91, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ibat2u", 92, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ibat2l", 93, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ibat3u", 94, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ibat3l", 95, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "dbat0u", 96, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "dbat0l", 97, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "dbat1u", 98, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "dbat1l", 99, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "dbat2u", 100, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "dbat2l", 101, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "dbat3u", 102, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "dbat3l", 103, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sdr1", 104, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "asr", 105, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "dar", 106, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "dsisr", 107, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sprg0", 108, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sprg1", 109, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sprg2", 110, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sprg3", 111, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "srr0", 112, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "srr1", 113, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "tbl", 114, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "tbu", 115, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "dec", 116, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "dabr", 117, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "ear", 118, 1, NULL, 32, "int");

  feature = tdesc_create_feature (result.get (), "505");
  tdesc_create_reg (feature, "eie", 119, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "eid", 120, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "nri", 121, 1, NULL, 32, "int");

  tdesc_powerpc_505 = result.release ();
}
