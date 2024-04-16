/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: nios2.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

const struct target_desc *tdesc_nios2;
static void
initialize_tdesc_nios2 (void)
{
  target_desc_up result = allocate_target_description ();
  set_tdesc_architecture (result.get (), bfd_scan_arch ("nios2"));

  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.nios2.cpu");
  tdesc_create_reg (feature, "zero", 0, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "at", 1, 1, NULL, 32, "uint32");
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
  tdesc_create_reg (feature, "et", 24, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "bt", 25, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "gp", 26, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "sp", 27, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "fp", 28, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "ea", 29, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "sstatus", 30, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "ra", 31, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "pc", 32, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "status", 33, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "estatus", 34, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "bstatus", 35, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "ienable", 36, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "ipending", 37, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cpuid", 38, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "ctl6", 39, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "exception", 40, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "pteaddr", 41, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "tlbacc", 42, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "tlbmisc", 43, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "eccinj", 44, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "badaddr", 45, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "config", 46, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "mpubase", 47, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "mpuacc", 48, 1, NULL, 32, "uint32");

  tdesc_nios2 = result.release ();
}
