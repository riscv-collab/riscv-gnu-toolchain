/* Target-dependent code for the CSKY architecture, for GDB.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.

   Contributed by C-SKY Microsystems and Mentor Graphics.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "gdbsupport/gdb_assert.h"
#include "frame.h"
#include "inferior.h"
#include "symtab.h"
#include "value.h"
#include "gdbcmd.h"
#include "language.h"
#include "gdbcore.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbtypes.h"
#include "target.h"
#include "arch-utils.h"
#include "regcache.h"
#include "osabi.h"
#include "block.h"
#include "reggroups.h"
#include "elf/csky.h"
#include "elf-bfd.h"
#include "symcat.h"
#include "sim-regno.h"
#include "dis-asm.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "trad-frame.h"
#include "infcall.h"
#include "floatformat.h"
#include "remote.h"
#include "target-descriptions.h"
#include "dwarf2/frame.h"
#include "user-regs.h"
#include "valprint.h"
#include "csky-tdep.h"
#include "regset.h"
#include "opcode/csky.h"
#include <algorithm>
#include <vector>

/* Control debugging information emitted in this file.  */

static bool csky_debug = false;

static const reggroup *cr_reggroup;
static const reggroup *fr_reggroup;
static const reggroup *vr_reggroup;
static const reggroup *mmu_reggroup;
static const reggroup *prof_reggroup;

static const char *csky_supported_tdesc_feature_names[] = {
  (const char *)"org.gnu.csky.abiv2.gpr",
  (const char *)"org.gnu.csky.abiv2.fpu",
  (const char *)"org.gnu.csky.abiv2.cr",
  (const char *)"org.gnu.csky.abiv2.fvcr",
  (const char *)"org.gnu.csky.abiv2.mmu",
  (const char *)"org.gnu.csky.abiv2.tee",
  (const char *)"org.gnu.csky.abiv2.fpu2",
  (const char *)"org.gnu.csky.abiv2.bank0",
  (const char *)"org.gnu.csky.abiv2.bank1",
  (const char *)"org.gnu.csky.abiv2.bank2",
  (const char *)"org.gnu.csky.abiv2.bank3",
  (const char *)"org.gnu.csky.abiv2.bank4",
  (const char *)"org.gnu.csky.abiv2.bank5",
  (const char *)"org.gnu.csky.abiv2.bank6",
  (const char *)"org.gnu.csky.abiv2.bank7",
  (const char *)"org.gnu.csky.abiv2.bank8",
  (const char *)"org.gnu.csky.abiv2.bank9",
  (const char *)"org.gnu.csky.abiv2.bank10",
  (const char *)"org.gnu.csky.abiv2.bank11",
  (const char *)"org.gnu.csky.abiv2.bank12",
  (const char *)"org.gnu.csky.abiv2.bank13",
  (const char *)"org.gnu.csky.abiv2.bank14",
  (const char *)"org.gnu.csky.abiv2.bank15",
  (const char *)"org.gnu.csky.abiv2.bank16",
  (const char *)"org.gnu.csky.abiv2.bank17",
  (const char *)"org.gnu.csky.abiv2.bank18",
  (const char *)"org.gnu.csky.abiv2.bank19",
  (const char *)"org.gnu.csky.abiv2.bank20",
  (const char *)"org.gnu.csky.abiv2.bank21",
  (const char *)"org.gnu.csky.abiv2.bank22",
  (const char *)"org.gnu.csky.abiv2.bank23",
  (const char *)"org.gnu.csky.abiv2.bank24",
  (const char *)"org.gnu.csky.abiv2.bank25",
  (const char *)"org.gnu.csky.abiv2.bank26",
  (const char *)"org.gnu.csky.abiv2.bank27",
  (const char *)"org.gnu.csky.abiv2.bank28",
  (const char *)"org.gnu.csky.abiv2.bank29",
  (const char *)"org.gnu.csky.abiv2.bank30",
  (const char *)"org.gnu.csky.abiv2.bank31"
};

struct csky_supported_tdesc_register
{
  char name[16];
  int num;
};

static const struct csky_supported_tdesc_register csky_supported_gpr_regs[] = {
  {"r0", 0},
  {"r1", 1},
  {"r2", 2},
  {"r3", 3},
  {"r4", 4},
  {"r5", 5},
  {"r6", 6},
  {"r7", 7},
  {"r8", 8},
  {"r9", 9},
  {"r10", 10},
  {"r11", 11},
  {"r12", 12},
  {"r13", 13},
  {"r14", 14},
  {"r15", 15},
  {"r16", 16},
  {"r17", 17},
  {"r18", 18},
  {"r19", 19},
  {"r20", 20},
  {"r21", 21},
  {"r22", 22},
  {"r23", 23},
  {"r24", 24},
  {"r25", 25},
  {"r26", 26},
  {"r27", 27},
  {"r28", 28},
  {"r28", 28},
  {"r29", 29},
  {"r30", 30},
  {"r31", 31},
  {"hi", CSKY_HI_REGNUM},
  {"lo", CSKY_LO_REGNUM},
  {"pc", CSKY_PC_REGNUM}
};

static const struct csky_supported_tdesc_register csky_supported_fpu_regs[] = {
  /* fr0~fr15.  */
  {"fr0",  CSKY_FR0_REGNUM + 0},
  {"fr1",  CSKY_FR0_REGNUM + 1},
  {"fr2",  CSKY_FR0_REGNUM + 2},
  {"fr3",  CSKY_FR0_REGNUM + 3},
  {"fr4",  CSKY_FR0_REGNUM + 4},
  {"fr5",  CSKY_FR0_REGNUM + 5},
  {"fr6",  CSKY_FR0_REGNUM + 6},
  {"fr7",  CSKY_FR0_REGNUM + 7},
  {"fr8",  CSKY_FR0_REGNUM + 8},
  {"fr9",  CSKY_FR0_REGNUM + 9},
  {"fr10", CSKY_FR0_REGNUM + 10},
  {"fr11", CSKY_FR0_REGNUM + 11},
  {"fr12", CSKY_FR0_REGNUM + 12},
  {"fr13", CSKY_FR0_REGNUM + 13},
  {"fr14", CSKY_FR0_REGNUM + 14},
  {"fr15", CSKY_FR0_REGNUM + 15},
  /* fr16~fr31.  */
  {"fr16", CSKY_FR16_REGNUM + 0},
  {"fr17", CSKY_FR16_REGNUM + 1},
  {"fr18", CSKY_FR16_REGNUM + 2},
  {"fr19", CSKY_FR16_REGNUM + 3},
  {"fr20", CSKY_FR16_REGNUM + 4},
  {"fr21", CSKY_FR16_REGNUM + 5},
  {"fr22", CSKY_FR16_REGNUM + 6},
  {"fr23", CSKY_FR16_REGNUM + 7},
  {"fr24", CSKY_FR16_REGNUM + 8},
  {"fr25", CSKY_FR16_REGNUM + 9},
  {"fr26", CSKY_FR16_REGNUM + 10},
  {"fr27", CSKY_FR16_REGNUM + 11},
  {"fr28", CSKY_FR16_REGNUM + 12},
  {"fr29", CSKY_FR16_REGNUM + 13},
  {"fr30", CSKY_FR16_REGNUM + 14},
  {"fr31", CSKY_FR16_REGNUM + 15},
  /* vr0~vr15.  */
  {"vr0",  CSKY_VR0_REGNUM + 0},
  {"vr1",  CSKY_VR0_REGNUM + 1},
  {"vr2",  CSKY_VR0_REGNUM + 2},
  {"vr3",  CSKY_VR0_REGNUM + 3},
  {"vr4",  CSKY_VR0_REGNUM + 4},
  {"vr5",  CSKY_VR0_REGNUM + 5},
  {"vr6",  CSKY_VR0_REGNUM + 6},
  {"vr7",  CSKY_VR0_REGNUM + 7},
  {"vr8",  CSKY_VR0_REGNUM + 8},
  {"vr9",  CSKY_VR0_REGNUM + 9},
  {"vr10", CSKY_VR0_REGNUM + 10},
  {"vr11", CSKY_VR0_REGNUM + 11},
  {"vr12", CSKY_VR0_REGNUM + 12},
  {"vr13", CSKY_VR0_REGNUM + 13},
  {"vr14", CSKY_VR0_REGNUM + 14},
  {"vr15", CSKY_VR0_REGNUM + 15},
  /* fpu control registers.  */
  {"fcr",  CSKY_FCR_REGNUM + 0},
  {"fid",  CSKY_FCR_REGNUM + 1},
  {"fesr", CSKY_FCR_REGNUM + 2},
};

static const struct csky_supported_tdesc_register csky_supported_ar_regs[] = {
  {"ar0",  CSKY_AR0_REGNUM + 0},
  {"ar1",  CSKY_AR0_REGNUM + 1},
  {"ar2",  CSKY_AR0_REGNUM + 2},
  {"ar3",  CSKY_AR0_REGNUM + 3},
  {"ar4",  CSKY_AR0_REGNUM + 4},
  {"ar5",  CSKY_AR0_REGNUM + 5},
  {"ar6",  CSKY_AR0_REGNUM + 6},
  {"ar7",  CSKY_AR0_REGNUM + 7},
  {"ar8",  CSKY_AR0_REGNUM + 8},
  {"ar9",  CSKY_AR0_REGNUM + 9},
  {"ar10", CSKY_AR0_REGNUM + 10},
  {"ar11", CSKY_AR0_REGNUM + 11},
  {"ar12", CSKY_AR0_REGNUM + 12},
  {"ar13", CSKY_AR0_REGNUM + 13},
  {"ar14", CSKY_AR0_REGNUM + 14},
  {"ar15", CSKY_AR0_REGNUM + 15},
};

static const struct csky_supported_tdesc_register csky_supported_bank0_regs[] = {
  {"cr0",  CSKY_CR0_REGNUM + 0},
  {"cr1",  CSKY_CR0_REGNUM + 1},
  {"cr2",  CSKY_CR0_REGNUM + 2},
  {"cr3",  CSKY_CR0_REGNUM + 3},
  {"cr4",  CSKY_CR0_REGNUM + 4},
  {"cr5",  CSKY_CR0_REGNUM + 5},
  {"cr6",  CSKY_CR0_REGNUM + 6},
  {"cr7",  CSKY_CR0_REGNUM + 7},
  {"cr8",  CSKY_CR0_REGNUM + 8},
  {"cr9",  CSKY_CR0_REGNUM + 9},
  {"cr10", CSKY_CR0_REGNUM + 10},
  {"cr11", CSKY_CR0_REGNUM + 11},
  {"cr12", CSKY_CR0_REGNUM + 12},
  {"cr13", CSKY_CR0_REGNUM + 13},
  {"cr14", CSKY_CR0_REGNUM + 14},
  {"cr15", CSKY_CR0_REGNUM + 15},
  {"cr16", CSKY_CR0_REGNUM + 16},
  {"cr17", CSKY_CR0_REGNUM + 17},
  {"cr18", CSKY_CR0_REGNUM + 18},
  {"cr19", CSKY_CR0_REGNUM + 19},
  {"cr20", CSKY_CR0_REGNUM + 20},
  {"cr21", CSKY_CR0_REGNUM + 21},
  {"cr22", CSKY_CR0_REGNUM + 22},
  {"cr23", CSKY_CR0_REGNUM + 23},
  {"cr24", CSKY_CR0_REGNUM + 24},
  {"cr25", CSKY_CR0_REGNUM + 25},
  {"cr26", CSKY_CR0_REGNUM + 26},
  {"cr27", CSKY_CR0_REGNUM + 27},
  {"cr28", CSKY_CR0_REGNUM + 28},
  {"cr29", CSKY_CR0_REGNUM + 29},
  {"cr30", CSKY_CR0_REGNUM + 30},
  {"cr31", CSKY_CR0_REGNUM + 31}
};

static const struct csky_supported_tdesc_register csky_supported_mmu_regs[] = {
  {"mcr0",  128},
  {"mcr2",  129},
  {"mcr3",  130},
  {"mcr4",  131},
  {"mcr6",  132},
  {"mcr8",  133},
  {"mcr29", 134},
  {"mcr30", 135},
  {"mcr31", 136}
};

static const struct csky_supported_tdesc_register csky_supported_bank15_regs[] = {
  {"cp15cp1",   253},
  {"cp15cp5",   254},
  {"cp15cp7",   255},
  {"cp15cp9",   256},
  {"cp15cp10",  257},
  {"cp15cp11",  258},
  {"cp15cp12",  259},
  {"cp15cp13",  260},
  {"cp15cp14",  261},
  {"cp15cp15",  262},
  {"cp15cp16",  263},
  {"cp15cp17",  264},
  {"cp15cp18",  265},
  {"cp15cp19",  266},
  {"cp15cp20",  267},
  {"cp15cp21",  268},
  {"cp15cp22",  269},
  {"cp15cp23",  270},
  {"cp15cp24",  271},
  {"cp15cp25",  272},
  {"cp15cp26",  273},
  {"cp15cp27",  274},
  {"cp15cp28",  275},
};

static const struct csky_supported_tdesc_register csky_supported_alias_regs[] = {
  /* Alias register names for Bank0.  */
  {"psr",   CSKY_CR0_REGNUM + 0},
  {"vbr",   CSKY_CR0_REGNUM + 1},
  {"epsr",  CSKY_CR0_REGNUM + 2},
  {"fpsr",  CSKY_CR0_REGNUM + 3},
  {"epc",   CSKY_CR0_REGNUM + 4},
  {"fpc",   CSKY_CR0_REGNUM + 5},
  {"ss0",   CSKY_CR0_REGNUM + 6},
  {"ss1",   CSKY_CR0_REGNUM + 7},
  {"ss2",   CSKY_CR0_REGNUM + 8},
  {"ss3",   CSKY_CR0_REGNUM + 9},
  {"ss4",   CSKY_CR0_REGNUM + 10},
  {"gcr",   CSKY_CR0_REGNUM + 11},
  {"gsr",   CSKY_CR0_REGNUM + 12},
  {"cpuid", CSKY_CR0_REGNUM + 13},
  {"ccr",   CSKY_CR0_REGNUM + 18},
  {"capr",  CSKY_CR0_REGNUM + 19},
  {"pacr",  CSKY_CR0_REGNUM + 20},
  {"prsr",  CSKY_CR0_REGNUM + 21},
  {"chr",   CSKY_CR0_REGNUM + 31},
  /* Alias register names for MMU.  */
  {"mir",   128},
  {"mel0",  129},
  {"mel1",  130},
  {"meh",   131},
  {"mpr",   132},
  {"mcir",  133},
  {"mpgd",  134},
  {"msa0",  135},
  {"msa1",  136},
  /* Alias register names for Bank1.  */
  {"ebr",     190},
  {"errlc",   195},
  {"erraddr", 196},
  {"errsts",  197},
  {"errinj",  198},
  {"usp",     203},
  {"int_sp",  204},
  {"itcmcr",  211},
  {"dtcmcr",  212},
  {"cindex",  215},
  {"cdata0",  216},
  {"cdata1",  217},
  {"cdata2",  218},
  {"cins",    220},
  /* Alias register names for Bank3.  */
  {"sepsr",   221},
  {"t_wssr",  221},
  {"sevbr",   222},
  {"t_wrcr",  222},
  {"seepsr",  223},
  {"seepc",   225},
  {"nsssp",   227},
  {"t_usp",   228},
  {"dcr",     229},
  {"t_pcr",   230},
};

/* Functions declaration.  */

static const char *
csky_pseudo_register_name (struct gdbarch *gdbarch, int regno);

/* Get csky supported registers's count for tdesc xml.  */

static int
csky_get_supported_tdesc_registers_count()
{
  int count = 0;
  count += ARRAY_SIZE (csky_supported_gpr_regs);
  count += ARRAY_SIZE (csky_supported_fpu_regs);
  count += ARRAY_SIZE (csky_supported_ar_regs);
  count += ARRAY_SIZE (csky_supported_bank0_regs);
  count += ARRAY_SIZE (csky_supported_mmu_regs);
  count += ARRAY_SIZE (csky_supported_bank15_regs);
  count += ARRAY_SIZE (csky_supported_alias_regs);
  /* Bank1~Bank14, Bank16~Bank31. */
  count += 32 * (14 + 16);
  return count;
}

/* Return a supported register according to index.  */

static const struct csky_supported_tdesc_register *
csky_get_supported_register_by_index (int index)
{
  static struct csky_supported_tdesc_register tdesc_reg;
  int count = 0;
  int multi, remain;
  int count_gpr = ARRAY_SIZE (csky_supported_gpr_regs);
  int count_fpu = ARRAY_SIZE (csky_supported_fpu_regs);
  int count_ar = ARRAY_SIZE (csky_supported_ar_regs);
  int count_bank0 = ARRAY_SIZE (csky_supported_bank0_regs);
  int count_mmu = ARRAY_SIZE (csky_supported_mmu_regs);
  int count_bank15 = ARRAY_SIZE (csky_supported_bank15_regs);
  int count_alias = ARRAY_SIZE (csky_supported_alias_regs);

  count = count_gpr;
  if (index < count)
    return &csky_supported_gpr_regs[index];
  if (index < (count + count_fpu))
    return &csky_supported_fpu_regs[index - count];
  count += count_fpu;
  if (index < (count + count_ar))
    return &csky_supported_ar_regs[index - count];
  count += count_ar;
  if (index < (count + count_bank0))
    return &csky_supported_bank0_regs[index - count];
  count += count_bank0;
  if (index < (count + count_mmu))
    return &csky_supported_mmu_regs[index - count];
  count += count_mmu;
  if (index < (count + count_bank15))
    return &csky_supported_bank15_regs[index - count];
  count += count_bank15;
  if (index < (count + count_alias))
    return &csky_supported_alias_regs[index - count];
  count += count_alias;
  index -= count;
  multi = index / 32;
  remain = index % 32;
  switch (multi)
    {
      case 0: /* Bank1.  */
	{
	  sprintf (tdesc_reg.name, "cp1cr%d", remain);
	  tdesc_reg.num = 189 + remain;
	}
	break;
      case 1: /* Bank2.  */
	{
	  sprintf (tdesc_reg.name, "cp2cr%d", remain);
	  tdesc_reg.num = 276 + remain;
	}
	break;
      case 2: /* Bank3.  */
	{
	  sprintf (tdesc_reg.name, "cp3cr%d", remain);
	  tdesc_reg.num = 221 + remain;
	}
	break;
      case 3:  /* Bank4.  */
      case 4:  /* Bank5.  */
      case 5:  /* Bank6.  */
      case 6:  /* Bank7.  */
      case 7:  /* Bank8.  */
      case 8:  /* Bank9.  */
      case 9:  /* Bank10.  */
      case 10: /* Bank11.  */
      case 11: /* Bank12.  */
      case 12: /* Bank13.  */
      case 13: /* Bank14.  */
	{
	  /* Regitsers in Bank4~14 have continuous regno with start 308.  */
	  sprintf (tdesc_reg.name, "cp%dcr%d", (multi + 1), remain);
	  tdesc_reg.num = 308 + ((multi - 3) * 32) + remain;
	}
	break;
      case 14: /* Bank16.  */
      case 15: /* Bank17.  */
      case 16: /* Bank18.  */
      case 17: /* Bank19.  */
      case 18: /* Bank20.  */
      case 19: /* Bank21.  */
      case 20: /* Bank22.  */
      case 21: /* Bank23.  */
      case 22: /* Bank24.  */
      case 23: /* Bank25.  */
      case 24: /* Bank26.  */
      case 25: /* Bank27.  */
      case 26: /* Bank28.  */
      case 27: /* Bank29.  */
      case 28: /* Bank30.  */
      case 29: /* Bank31.  */
	{
	  /* Regitsers in Bank16~31 have continuous regno with start 660.  */
	  sprintf (tdesc_reg.name, "cp%dcr%d", (multi + 2), remain);
	  tdesc_reg.num = 660 + ((multi - 14) * 32) + remain;
	}
	break;
      default:
	return NULL;
    }
  return &tdesc_reg;
}

/* Convenience function to print debug messages in prologue analysis.  */

static void
print_savedreg_msg (int regno, int offsets[], bool print_continuing)
{
  gdb_printf (gdb_stdlog, "csky: r%d saved at offset 0x%x\n",
	      regno, offsets[regno]);
  if (print_continuing)
    gdb_printf (gdb_stdlog, "csky: continuing\n");
}

/*  Check whether the instruction at ADDR is 16-bit or not.  */

static int
csky_pc_is_csky16 (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  gdb_byte target_mem[2];
  int status;
  unsigned int insn;
  int ret = 1;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  status = target_read_memory (addr, target_mem, 2);
  /* Assume a 16-bit instruction if we can't read memory.  */
  if (status)
    return 1;

  /* Get instruction from memory.  */
  insn = extract_unsigned_integer (target_mem, 2, byte_order);
  if ((insn & CSKY_32_INSN_MASK) == CSKY_32_INSN_MASK)
    ret = 0;
  else if (insn == CSKY_BKPT_INSN)
    {
      /* Check for 32-bit bkpt instruction which is all 0.  */
      status = target_read_memory (addr + 2, target_mem, 2);
      if (status)
	return 1;

      insn = extract_unsigned_integer (target_mem, 2, byte_order);
      if (insn == CSKY_BKPT_INSN)
	ret = 0;
    }
  return ret;
}

/* Get one instruction at ADDR and store it in INSN.  Return 2 for
   a 16-bit instruction or 4 for a 32-bit instruction.  */

static int
csky_get_insn (struct gdbarch *gdbarch, CORE_ADDR addr, unsigned int *insn)
{
  gdb_byte target_mem[2];
  unsigned int insn_type;
  int status;
  int insn_len = 2;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  status = target_read_memory (addr, target_mem, 2);
  if (status)
    memory_error (TARGET_XFER_E_IO, addr);

  insn_type = extract_unsigned_integer (target_mem, 2, byte_order);
  if (CSKY_32_INSN_MASK == (insn_type & CSKY_32_INSN_MASK))
    {
      status = target_read_memory (addr + 2, target_mem, 2);
      if (status)
	memory_error (TARGET_XFER_E_IO, addr);
      insn_type = ((insn_type << 16)
		   | extract_unsigned_integer (target_mem, 2, byte_order));
      insn_len = 4;
    }
  *insn = insn_type;
  return insn_len;
}

/* Implement the read_pc gdbarch method.  */

static CORE_ADDR
csky_read_pc (readable_regcache *regcache)
{
  ULONGEST pc;
  regcache->cooked_read (CSKY_PC_REGNUM, &pc);
  return pc;
}

/* Implement the write_pc gdbarch method.  */

static void
csky_write_pc (regcache *regcache, CORE_ADDR val)
{
  regcache_cooked_write_unsigned (regcache, CSKY_PC_REGNUM, val);
}

/* C-Sky ABI register names.  */

static const char * const csky_register_names[] =
{
  /* General registers 0 - 31.  */
  "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
  "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
  "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
  "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",

  /* DSP hilo registers 36 and 37.  */
  "",      "",    "",     "",     "hi",    "lo",   "",    "",

  /* FPU/VPU general registers 40 - 71.  */
  "fr0", "fr1", "fr2",  "fr3",  "fr4",  "fr5",  "fr6",  "fr7",
  "fr8", "fr9", "fr10", "fr11", "fr12", "fr13", "fr14", "fr15",
  "vr0", "vr1", "vr2",  "vr3",  "vr4",  "vr5",  "vr6",  "vr7",
  "vr8", "vr9", "vr10", "vr11", "vr12", "vr13", "vr14", "vr15",

  /* Program counter 72.  */
  "pc",

  /* Optional registers (ar) 73 - 88.  */
  "ar0", "ar1", "ar2",  "ar3",  "ar4",  "ar5",  "ar6",  "ar7",
  "ar8", "ar9", "ar10", "ar11", "ar12", "ar13", "ar14", "ar15",

  /* Control registers (cr) 89 - 119.  */
  "psr",  "vbr",  "epsr", "fpsr", "epc",  "fpc",  "ss0",  "ss1",
  "ss2",  "ss3",  "ss4",  "gcr",  "gsr",  "cr13", "cr14", "cr15",
  "cr16", "cr17", "cr18", "cr19", "cr20", "cr21", "cr22", "cr23",
  "cr24", "cr25", "cr26", "cr27", "cr28", "cr29", "cr30", "cr31",

  /* FPU/VPU control registers 121 ~ 123.  */
  /* User sp 127.  */
  "fid", "fcr", "fesr", "", "", "", "usp",

  /* MMU control registers: 128 - 136.  */
  "mcr0", "mcr2", "mcr3", "mcr4", "mcr6", "mcr8", "mcr29", "mcr30",
  "mcr31", "", "", "",

  /* Profiling control registers 140 - 143.  */
  /* Profiling software general registers 144 - 157.  */
  "profcr0",  "profcr1",  "profcr2",  "profcr3",  "profsgr0",  "profsgr1",
  "profsgr2", "profsgr3", "profsgr4", "profsgr5", "profsgr6",  "profsgr7",
  "profsgr8", "profsgr9", "profsgr10","profsgr11","profsgr12", "profsgr13",
  "",	 "",

  /* Profiling architecture general registers 160 - 174.  */
  "profagr0", "profagr1", "profagr2", "profagr3", "profagr4", "profagr5",
  "profagr6", "profagr7", "profagr8", "profagr9", "profagr10","profagr11",
  "profagr12","profagr13","profagr14", "",

  /* Profiling extension general registers 176 - 188.  */
  "profxgr0", "profxgr1", "profxgr2", "profxgr3", "profxgr4", "profxgr5",
  "profxgr6", "profxgr7", "profxgr8", "profxgr9", "profxgr10","profxgr11",
  "profxgr12",

  /* Control registers in bank1.  */
  "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "",
  "cp1cr16", "cp1cr17", "cp1cr18", "cp1cr19", "cp1cr20", "", "", "",
  "", "", "", "", "", "", "", "",

  /* Control registers in bank3 (ICE).  */
  "sepsr", "sevbr", "seepsr", "", "seepc", "", "nsssp", "seusp",
  "sedcr", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", "",
  "", "", "", "", "", "", "", ""
};

/* Implement the register_name gdbarch method.  */

static const char *
csky_register_name (struct gdbarch *gdbarch, int reg_nr)
{
  if (reg_nr >= gdbarch_num_regs (gdbarch))
    return csky_pseudo_register_name (gdbarch, reg_nr);

  if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    return tdesc_register_name (gdbarch, reg_nr);

  return csky_register_names[reg_nr];
}

/* Construct vector type for vrx registers.  */

static struct type *
csky_vector_type (struct gdbarch *gdbarch)
{
  const struct builtin_type *bt = builtin_type (gdbarch);

  struct type *t;

  t = arch_composite_type (gdbarch, "__gdb_builtin_type_vec128i",
			   TYPE_CODE_UNION);

  append_composite_type_field (t, "u32",
			       init_vector_type (bt->builtin_int32, 4));
  append_composite_type_field (t, "u16",
			       init_vector_type (bt->builtin_int16, 8));
  append_composite_type_field (t, "u8",
			       init_vector_type (bt->builtin_int8, 16));

  t->set_is_vector (true);
  t->set_name ("builtin_type_vec128i");

  return t;
}

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

static struct type *
csky_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  int num_regs = gdbarch_num_regs (gdbarch);
  csky_gdbarch_tdep *tdep
    = gdbarch_tdep<csky_gdbarch_tdep> (gdbarch);

  if (tdep->fv_pseudo_registers_count)
    {
      if ((reg_nr >= num_regs)
	  && (reg_nr < (num_regs + tdep->fv_pseudo_registers_count)))
	return builtin_type (gdbarch)->builtin_int32;
    }

  /* Vector register has 128 bits, and only in ck810. Just return
     csky_vector_type(), not check tdesc_has_registers(), is in case
     of some GDB stub does not describe type for Vector registers
     in the target-description-xml.  */
  if ((reg_nr >= CSKY_VR0_REGNUM) && (reg_nr <= CSKY_VR0_REGNUM + 15))
    return csky_vector_type (gdbarch);

  /* If type has been described in tdesc-xml, use it.  */
  if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    {
      struct type *tdesc_t = tdesc_register_type (gdbarch, reg_nr);
      if (tdesc_t)
	return tdesc_t;
    }

  /* PC, EPC, FPC is a text pointer.  */
  if ((reg_nr == CSKY_PC_REGNUM)  || (reg_nr == CSKY_EPC_REGNUM)
      || (reg_nr == CSKY_FPC_REGNUM))
    return builtin_type (gdbarch)->builtin_func_ptr;

  /* VBR is a data pointer.  */
  if (reg_nr == CSKY_VBR_REGNUM)
    return builtin_type (gdbarch)->builtin_data_ptr;

  /* Float register has 64 bits, and only in ck810.  */
  if ((reg_nr >=CSKY_FR0_REGNUM) && (reg_nr <= CSKY_FR0_REGNUM + 15))
    {
      type_allocator alloc (gdbarch);
      return init_float_type (alloc, 64, "builtin_type_csky_ext",
			      floatformats_ieee_double);
    }

  /* Profiling general register has 48 bits, we use 64bit.  */
  if ((reg_nr >= CSKY_PROFGR_REGNUM) && (reg_nr <= CSKY_PROFGR_REGNUM + 44))
    return builtin_type (gdbarch)->builtin_uint64;

  if (reg_nr == CSKY_SP_REGNUM)
    return builtin_type (gdbarch)->builtin_data_ptr;

  /* Others are 32 bits.  */
  return builtin_type (gdbarch)->builtin_int32;
}

/* Data structure to marshall items in a dummy stack frame when
   calling a function in the inferior.  */

struct csky_stack_item
{
  csky_stack_item (int len_, const gdb_byte *data_)
  : len (len_), data (data_)
  {}

  int len;
  const gdb_byte *data;
};

/* Implement the push_dummy_call gdbarch method.  */

static CORE_ADDR
csky_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		      struct regcache *regcache, CORE_ADDR bp_addr,
		      int nargs, struct value **args, CORE_ADDR sp,
		      function_call_return_method return_method,
		      CORE_ADDR struct_addr)
{
  int argnum;
  int argreg = CSKY_ABI_A0_REGNUM;
  int last_arg_regnum = CSKY_ABI_LAST_ARG_REGNUM;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  std::vector<csky_stack_item> stack_items;

  /* Set the return address.  For CSKY, the return breakpoint is
     always at BP_ADDR.  */
  regcache_cooked_write_unsigned (regcache, CSKY_LR_REGNUM, bp_addr);

  /* The struct_return pointer occupies the first parameter
     passing register.  */
  if (return_method == return_method_struct)
    {
      if (csky_debug)
	{
	  gdb_printf (gdb_stdlog,
		      "csky: struct return in %s = %s\n",
		      gdbarch_register_name (gdbarch, argreg),
		      paddress (gdbarch, struct_addr));
	}
      regcache_cooked_write_unsigned (regcache, argreg, struct_addr);
      argreg++;
    }

  /* Put parameters into argument registers in REGCACHE.
     In ABI argument registers are r0 through r3.  */
  for (argnum = 0; argnum < nargs; argnum++)
    {
      int len;
      struct type *arg_type;
      const gdb_byte *val;

      arg_type = check_typedef (args[argnum]->type ());
      len = arg_type->length ();
      val = args[argnum]->contents ().data ();

      /* Copy the argument to argument registers or the dummy stack.
	 Large arguments are split between registers and stack.

	 If len < 4, there is no need to worry about endianness since
	 the arguments will always be stored in the low address.  */
      if (len < 4)
	{
	  CORE_ADDR regval
	    = extract_unsigned_integer (val, len, byte_order);
	  regcache_cooked_write_unsigned (regcache, argreg, regval);
	  argreg++;
	}
      else
	{
	  while (len > 0)
	    {
	      int partial_len = len < 4 ? len : 4;
	      if (argreg <= last_arg_regnum)
		{
		  /* The argument is passed in an argument register.  */
		  CORE_ADDR regval
		    = extract_unsigned_integer (val, partial_len,
						byte_order);
		  if (byte_order == BFD_ENDIAN_BIG)
		    regval <<= (4 - partial_len) * 8;

		  /* Put regval into register in REGCACHE.  */
		  regcache_cooked_write_unsigned (regcache, argreg,
						  regval);
		  argreg++;
		}
	      else
		{
		  /* The argument should be pushed onto the dummy stack.  */
		  stack_items.emplace_back (4, val);
		}
	      len -= partial_len;
	      val += partial_len;
	    }
	}
    }

  /* Transfer the dummy stack frame to the target.  */
  std::vector<csky_stack_item>::reverse_iterator iter;
  for (iter = stack_items.rbegin (); iter != stack_items.rend (); ++iter)
    {
      sp -= iter->len;
      write_memory (sp, iter->data, iter->len);
    }

  /* Finally, update the SP register.  */
  regcache_cooked_write_unsigned (regcache, CSKY_SP_REGNUM, sp);
  return sp;
}

/* Implement the return_value gdbarch method.  */

static enum return_value_convention
csky_return_value (struct gdbarch *gdbarch, struct value *function,
		   struct type *valtype, struct regcache *regcache,
		   gdb_byte *readbuf, const gdb_byte *writebuf)
{
  CORE_ADDR regval;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int len = valtype->length ();
  unsigned int ret_regnum = CSKY_RET_REGNUM;

  /* Csky abi specifies that return values larger than 8 bytes
     are put on the stack.  */
  if (len > 8)
    return RETURN_VALUE_STRUCT_CONVENTION;
  else
    {
      if (readbuf != NULL)
	{
	  ULONGEST tmp;
	  /* By using store_unsigned_integer we avoid having to do
	     anything special for small big-endian values.  */
	  regcache->cooked_read (ret_regnum, &tmp);
	  store_unsigned_integer (readbuf, (len > 4 ? 4 : len),
				  byte_order, tmp);
	  if (len > 4)
	    {
	      regcache->cooked_read (ret_regnum + 1, &tmp);
	      store_unsigned_integer (readbuf + 4,  4, byte_order, tmp);
	    }
	}
      if (writebuf != NULL)
	{
	  regval = extract_unsigned_integer (writebuf, len > 4 ? 4 : len,
					     byte_order);
	  regcache_cooked_write_unsigned (regcache, ret_regnum, regval);
	  if (len > 4)
	    {
	      regval = extract_unsigned_integer ((gdb_byte *) writebuf + 4,
						 4, byte_order);
	      regcache_cooked_write_unsigned (regcache, ret_regnum + 1,
					      regval);
	    }

	}
      return RETURN_VALUE_REGISTER_CONVENTION;
    }
}

/* Implement the frame_align gdbarch method.

   Adjust the address downward (direction of stack growth) so that it
   is correctly aligned for a new stack frame.  */

static CORE_ADDR
csky_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return align_down (addr, 4);
}

/* Unwind cache used for gdbarch fallback unwinder.  */

struct csky_unwind_cache
{
  /* The stack pointer at the time this frame was created; i.e. the
     caller's stack pointer when this function was called.  It is used
     to identify this frame.  */
  CORE_ADDR prev_sp;

  /* The frame base for this frame is just prev_sp - frame size.
     FRAMESIZE is the distance from the frame pointer to the
     initial stack pointer.  */
  int framesize;

  /* The register used to hold the frame pointer for this frame.  */
  int framereg;

  /* Saved register offsets.  */
  trad_frame_saved_reg *saved_regs;
};

/* Do prologue analysis, returning the PC of the first instruction
   after the function prologue.  */

static CORE_ADDR
csky_analyze_prologue (struct gdbarch *gdbarch,
		       CORE_ADDR start_pc,
		       CORE_ADDR limit_pc,
		       CORE_ADDR end_pc,
		       frame_info_ptr this_frame,
		       struct csky_unwind_cache *this_cache,
		       lr_type_t lr_type)
{
  CORE_ADDR addr;
  unsigned int insn, rn;
  int framesize = 0;
  int stacksize = 0;
  int register_offsets[CSKY_NUM_GREGS_SAVED_GREGS];
  int insn_len;
  /* For adjusting fp.  */
  int is_fp_saved = 0;
  int adjust_fp = 0;

  /* REGISTER_OFFSETS will contain offsets from the top of the frame
     (NOT the frame pointer) for the various saved registers, or -1
     if the register is not saved.  */
  for (rn = 0; rn < CSKY_NUM_GREGS_SAVED_GREGS; rn++)
    register_offsets[rn] = -1;

  /* Analyze the prologue.  Things we determine from analyzing the
     prologue include the size of the frame and which registers are
     saved (and where).  */
  if (csky_debug)
    {
      gdb_printf (gdb_stdlog,
		  "csky: Scanning prologue: start_pc = 0x%x,"
		  "limit_pc = 0x%x\n", (unsigned int) start_pc,
		  (unsigned int) limit_pc);
    }

  /* Default to 16 bit instruction.  */
  insn_len = 2;
  stacksize = 0;
  for (addr = start_pc; addr < limit_pc; addr += insn_len)
    {
      /* Get next insn.  */
      insn_len = csky_get_insn (gdbarch, addr, &insn);

      /* Check if 32 bit.  */
      if (insn_len == 4)
	{
	  /* subi32 sp,sp oimm12.  */
	  if (CSKY_32_IS_SUBI0 (insn))
	    {
	      /* Got oimm12.  */
	      int offset = CSKY_32_SUBI_IMM (insn);
	      if (csky_debug)
		{
		  gdb_printf (gdb_stdlog,
			      "csky: got subi sp,%d; continuing\n",
			      offset);
		}
	      stacksize += offset;
	      continue;
	    }
	  /* stm32 ry-rz,(sp).  */
	  else if (CSKY_32_IS_STMx0 (insn))
	    {
	      /* Spill register(s).  */
	      int start_register;
	      int reg_count;
	      int offset;

	      /* BIG WARNING! The CKCore ABI does not restrict functions
		 to taking only one stack allocation.  Therefore, when
		 we save a register, we record the offset of where it was
		 saved relative to the current stacksize.  This will
		 then give an offset from the SP upon entry to our
		 function.  Remember, stacksize is NOT constant until
		 we're done scanning the prologue.  */
	      start_register = CSKY_32_STM_VAL_REGNUM (insn);
	      reg_count = CSKY_32_STM_SIZE (insn);
	      if (csky_debug)
		{
		  gdb_printf (gdb_stdlog,
			      "csky: got stm r%d-r%d,(sp)\n",
			      start_register,
			      start_register + reg_count);
		}

	      for (rn = start_register, offset = 0;
		   rn <= start_register + reg_count;
		   rn++, offset += 4)
		{
		  register_offsets[rn] = stacksize - offset;
		  if (csky_debug)
		    {
		      gdb_printf (gdb_stdlog,
				  "csky: r%d saved at 0x%x"
				  " (offset %d)\n",
				  rn, register_offsets[rn],
				  offset);
		    }
		}
	      if (csky_debug)
		gdb_printf (gdb_stdlog, "csky: continuing\n");
	      continue;
	    }
	  /* stw ry,(sp,disp).  */
	  else if (CSKY_32_IS_STWx0 (insn))
	    {
	      /* Spill register: see note for IS_STM above.  */
	      int disp;

	      rn = CSKY_32_ST_VAL_REGNUM (insn);
	      disp = CSKY_32_ST_OFFSET (insn);
	      register_offsets[rn] = stacksize - disp;
	      if (csky_debug)
		print_savedreg_msg (rn, register_offsets, true);
	      continue;
	    }
	  else if (CSKY_32_IS_MOV_FP_SP (insn))
	    {
	      /* SP is saved to FP reg, means code afer prologue may
		 modify SP.  */
	      is_fp_saved = 1;
	      adjust_fp = stacksize;
	      continue;
	    }
	  else if (CSKY_32_IS_MFCR_EPSR (insn))
	    {
	      unsigned int insn2;
	      addr += 4;
	      int mfcr_regnum = insn & 0x1f;
	      insn_len = csky_get_insn (gdbarch, addr, &insn2);
	      if (insn_len == 2)
		{
		  int stw_regnum = (insn2 >> 5) & 0x7;
		  if (CSKY_16_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
		    {
		      int offset;

		      /* CSKY_EPSR_REGNUM.  */
		      rn  = CSKY_NUM_GREGS;
		      offset = CSKY_16_STWx0_OFFSET (insn2);
		      register_offsets[rn] = stacksize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	      else
		{
		  /* INSN_LEN == 4.  */
		  int stw_regnum = (insn2 >> 21) & 0x1f;
		  if (CSKY_32_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
		    {
		      int offset;

		      /* CSKY_EPSR_REGNUM.  */
		      rn  = CSKY_NUM_GREGS;
		      offset = CSKY_32_ST_OFFSET (insn2);
		      register_offsets[rn] = framesize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	    }
	  else if (CSKY_32_IS_MFCR_FPSR (insn))
	    {
	      unsigned int insn2;
	      addr += 4;
	      int mfcr_regnum = insn & 0x1f;
	      insn_len = csky_get_insn (gdbarch, addr, &insn2);
	      if (insn_len == 2)
		{
		  int stw_regnum = (insn2 >> 5) & 0x7;
		  if (CSKY_16_IS_STWx0 (insn2) && (mfcr_regnum
						 == stw_regnum))
		    {
		      int offset;

		      /* CSKY_FPSR_REGNUM.  */
		      rn  = CSKY_NUM_GREGS + 1;
		      offset = CSKY_16_STWx0_OFFSET (insn2);
		      register_offsets[rn] = stacksize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	      else
		{
		  /* INSN_LEN == 4.  */
		  int stw_regnum = (insn2 >> 21) & 0x1f;
		  if (CSKY_32_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
		    {
		      int offset;

		      /* CSKY_FPSR_REGNUM.  */
		      rn  = CSKY_NUM_GREGS + 1;
		      offset = CSKY_32_ST_OFFSET (insn2);
		      register_offsets[rn] = framesize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	    }
	  else if (CSKY_32_IS_MFCR_EPC (insn))
	    {
	      unsigned int insn2;
	      addr += 4;
	      int mfcr_regnum = insn & 0x1f;
	      insn_len = csky_get_insn (gdbarch, addr, &insn2);
	      if (insn_len == 2)
		{
		  int stw_regnum = (insn2 >> 5) & 0x7;
		  if (CSKY_16_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
		    {
		      int offset;

		      /* CSKY_EPC_REGNUM.  */
		      rn  = CSKY_NUM_GREGS + 2;
		      offset = CSKY_16_STWx0_OFFSET (insn2);
		      register_offsets[rn] = stacksize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	      else
		{
		  /* INSN_LEN == 4.  */
		  int stw_regnum = (insn2 >> 21) & 0x1f;
		  if (CSKY_32_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
		    {
		      int offset;

		      /* CSKY_EPC_REGNUM.  */
		      rn  = CSKY_NUM_GREGS + 2;
		      offset = CSKY_32_ST_OFFSET (insn2);
		      register_offsets[rn] = framesize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	    }
	  else if (CSKY_32_IS_MFCR_FPC (insn))
	    {
	      unsigned int insn2;
	      addr += 4;
	      int mfcr_regnum = insn & 0x1f;
	      insn_len = csky_get_insn (gdbarch, addr, &insn2);
	      if (insn_len == 2)
		{
		  int stw_regnum = (insn2 >> 5) & 0x7;
		  if (CSKY_16_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
		    {
		      int offset;

		      /* CSKY_FPC_REGNUM.  */
		      rn  = CSKY_NUM_GREGS + 3;
		      offset = CSKY_16_STWx0_OFFSET (insn2);
		      register_offsets[rn] = stacksize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	      else
		{
		  /* INSN_LEN == 4.  */
		  int stw_regnum = (insn2 >> 21) & 0x1f;
		  if (CSKY_32_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
		    {
		      int offset;

		      /* CSKY_FPC_REGNUM.  */
		      rn  = CSKY_NUM_GREGS + 3;
		      offset = CSKY_32_ST_OFFSET (insn2);
		      register_offsets[rn] = framesize - offset;
		      if (csky_debug)
			print_savedreg_msg (rn, register_offsets, true);
		      continue;
		    }
		  break;
		}
	    }
	  else if (CSKY_32_IS_PUSH (insn))
	    {
	      /* Push for 32_bit.  */
	      if (CSKY_32_IS_PUSH_R29 (insn))
		{
		  stacksize += 4;
		  register_offsets[29] = stacksize;
		  if (csky_debug)
		    print_savedreg_msg (29, register_offsets, false);
		}
	      if (CSKY_32_PUSH_LIST2 (insn))
		{
		  int num = CSKY_32_PUSH_LIST2 (insn);
		  int tmp = 0;
		  stacksize += num * 4;
		  if (csky_debug)
		    {
		      gdb_printf (gdb_stdlog,
				  "csky: push regs_array: r16-r%d\n",
				  16 + num - 1);
		    }
		  for (rn = 16; rn <= 16 + num - 1; rn++)
		    {
		       register_offsets[rn] = stacksize - tmp;
		       if (csky_debug)
			 {
			   gdb_printf (gdb_stdlog,
				       "csky: r%d saved at 0x%x"
				       " (offset %d)\n", rn,
				       register_offsets[rn], tmp);
			 }
		       tmp += 4;
		    }
		}
	      if (CSKY_32_IS_PUSH_R15 (insn))
		{
		  stacksize += 4;
		  register_offsets[15] = stacksize;
		  if (csky_debug)
		    print_savedreg_msg (15, register_offsets, false);
		}
	      if (CSKY_32_PUSH_LIST1 (insn))
		{
		  int num = CSKY_32_PUSH_LIST1 (insn);
		  int tmp = 0;
		  stacksize += num * 4;
		  if (csky_debug)
		    {
		      gdb_printf (gdb_stdlog,
				  "csky: push regs_array: r4-r%d\n",
				  4 + num - 1);
		    }
		  for (rn = 4; rn <= 4 + num - 1; rn++)
		    {
		       register_offsets[rn] = stacksize - tmp;
		       if (csky_debug)
			 {
			   gdb_printf (gdb_stdlog,
				       "csky: r%d saved at 0x%x"
				       " (offset %d)\n", rn,
				       register_offsets[rn], tmp);
			 }
			tmp += 4;
		    }
		}

	      framesize = stacksize;
	      if (csky_debug)
		gdb_printf (gdb_stdlog, "csky: continuing\n");
	      continue;
	    }
	  else if (CSKY_32_IS_LRW4 (insn) || CSKY_32_IS_MOVI4 (insn)
		   || CSKY_32_IS_MOVIH4 (insn) || CSKY_32_IS_BMASKI4 (insn))
	    {
	      int adjust = 0;
	      int offset = 0;
	      unsigned int insn2;

	      if (csky_debug)
		{
		  gdb_printf (gdb_stdlog,
			      "csky: looking at large frame\n");
		}
	      if (CSKY_32_IS_LRW4 (insn))
		{
		  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
		  int literal_addr = (addr + ((insn & 0xffff) << 2))
				     & 0xfffffffc;
		  adjust = read_memory_unsigned_integer (literal_addr, 4,
							 byte_order);
		}
	      else if (CSKY_32_IS_MOVI4 (insn))
		adjust = (insn  & 0xffff);
	      else if (CSKY_32_IS_MOVIH4 (insn))
		adjust = (insn & 0xffff) << 16;
	      else
		{
		  /* CSKY_32_IS_BMASKI4 (insn).  */
		  adjust = (1 << (((insn & 0x3e00000) >> 21) + 1)) - 1;
		}

	      if (csky_debug)
		{
		  gdb_printf (gdb_stdlog,
			      "csky: base stacksize=0x%x\n", adjust);

		  /* May have zero or more insns which modify r4.  */
		  gdb_printf (gdb_stdlog,
			      "csky: looking for r4 adjusters...\n");
		}

	      offset = 4;
	      insn_len = csky_get_insn (gdbarch, addr + offset, &insn2);
	      while (CSKY_IS_R4_ADJUSTER (insn2))
		{
		  if (CSKY_32_IS_ADDI4 (insn2))
		    {
		      int imm = (insn2 & 0xfff) + 1;
		      adjust += imm;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: addi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_SUBI4 (insn2))
		    {
		      int imm = (insn2 & 0xfff) + 1;
		      adjust -= imm;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: subi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_NOR4 (insn2))
		    {
		      adjust = ~adjust;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: nor r4,r4,r4\n");
			}
		    }
		  else if (CSKY_32_IS_ROTLI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      int temp = adjust >> (32 - imm);
		      adjust <<= imm;
		      adjust |= temp;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: rotli r4,r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_LISI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      adjust <<= imm;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: lsli r4,r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_BSETI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      adjust |= (1 << imm);
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: bseti r4,r4 %d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_BCLRI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      adjust &= ~(1 << imm);
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: bclri r4,r4 %d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_IXH4 (insn2))
		    {
		      adjust *= 3;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: ixh r4,r4,r4\n");
			}
		    }
		  else if (CSKY_32_IS_IXW4 (insn2))
		    {
		      adjust *= 5;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: ixw r4,r4,r4\n");
			}
		    }
		  else if (CSKY_16_IS_ADDI4 (insn2))
		    {
		      int imm = (insn2 & 0xff) + 1;
		      adjust += imm;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: addi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_SUBI4 (insn2))
		    {
		      int imm = (insn2 & 0xff) + 1;
		      adjust -= imm;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: subi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_NOR4 (insn2))
		    {
		      adjust = ~adjust;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: nor r4,r4\n");
			}
		    }
		  else if (CSKY_16_IS_BSETI4 (insn2))
		    {
		      int imm = (insn2 & 0x1f);
		      adjust |= (1 << imm);
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: bseti r4, %d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_BCLRI4 (insn2))
		    {
		      int imm = (insn2 & 0x1f);
		      adjust &= ~(1 << imm);
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: bclri r4, %d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_LSLI4 (insn2))
		    {
		      int imm = (insn2 & 0x1f);
		      adjust <<= imm;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: lsli r4,r4, %d\n", imm);
			}
		    }

		  offset += insn_len;
		  insn_len =  csky_get_insn (gdbarch, addr + offset, &insn2);
		};

	      if (csky_debug)
		{
		  gdb_printf (gdb_stdlog, "csky: done looking for"
			      " r4 adjusters\n");
		}

	      /* If the next insn adjusts the stack pointer, we keep
		 everything; if not, we scrap it and we've found the
		 end of the prologue.  */
	      if (CSKY_IS_SUBU4 (insn2))
		{
		  addr += offset;
		  stacksize += adjust;
		  if (csky_debug)
		    {
		      gdb_printf (gdb_stdlog,
				  "csky: found stack adjustment of"
				  " 0x%x bytes.\n", adjust);
		      gdb_printf (gdb_stdlog,
				  "csky: skipping to new address %s\n",
				  core_addr_to_string_nz (addr));
		      gdb_printf (gdb_stdlog,
				  "csky: continuing\n");
		    }
		  continue;
		}

	      /* None of these instructions are prologue, so don't touch
		 anything.  */
	      if (csky_debug)
		{
		  gdb_printf (gdb_stdlog,
			      "csky: no subu sp,sp,r4; NOT altering"
			      " stacksize.\n");
		}
	      break;
	    }
	}
      else
	{
	  /* insn_len != 4.  */

	  /* subi.sp sp,disp.  */
	  if (CSKY_16_IS_SUBI0 (insn))
	    {
	      int offset = CSKY_16_SUBI_IMM (insn);
	      if (csky_debug)
		{
		  gdb_printf (gdb_stdlog,
			      "csky: got subi r0,%d; continuing\n",
			      offset);
		}
	      stacksize += offset;
	      continue;
	    }
	  /* stw.16 rz,(sp,disp).  */
	  else if (CSKY_16_IS_STWx0 (insn))
	    {
	      /* Spill register: see note for IS_STM above.  */
	      int disp;

	      rn = CSKY_16_ST_VAL_REGNUM (insn);
	      disp = CSKY_16_ST_OFFSET (insn);
	      register_offsets[rn] = stacksize - disp;
	      if (csky_debug)
		print_savedreg_msg (rn, register_offsets, true);
	      continue;
	    }
	  else if (CSKY_16_IS_MOV_FP_SP (insn))
	    {
	      /* SP is saved to FP reg, means prologue may modify SP.  */
	      is_fp_saved = 1;
	      adjust_fp = stacksize;
	      continue;
	    }
	  else if (CSKY_16_IS_PUSH (insn))
	    {
	      /* Push for 16_bit.  */
	      int offset = 0;
	      if (CSKY_16_IS_PUSH_R15 (insn))
		{
		  stacksize += 4;
		  register_offsets[15] = stacksize;
		  if (csky_debug)
		    print_savedreg_msg (15, register_offsets, false);
		  offset += 4;
		 }
	      if (CSKY_16_PUSH_LIST1 (insn))
		{
		  int num = CSKY_16_PUSH_LIST1 (insn);
		  int tmp = 0;
		  stacksize += num * 4;
		  offset += num * 4;
		  if (csky_debug)
		    {
		      gdb_printf (gdb_stdlog,
				  "csky: push regs_array: r4-r%d\n",
				  4 + num - 1);
		    }
		  for (rn = 4; rn <= 4 + num - 1; rn++)
		    {
		       register_offsets[rn] = stacksize - tmp;
		       if (csky_debug)
			 {
			   gdb_printf (gdb_stdlog,
				       "csky: r%d saved at 0x%x"
				       " (offset %d)\n", rn,
				       register_offsets[rn], offset);
			 }
		       tmp += 4;
		    }
		}

	      framesize = stacksize;
	      if (csky_debug)
		gdb_printf (gdb_stdlog, "csky: continuing\n");
	      continue;
	    }
	  else if (CSKY_16_IS_LRW4 (insn) || CSKY_16_IS_MOVI4 (insn))
	    {
	      int adjust = 0;
	      unsigned int insn2;

	      if (csky_debug)
		{
		  gdb_printf (gdb_stdlog,
			      "csky: looking at large frame\n");
		}
	      if (CSKY_16_IS_LRW4 (insn))
		{
		  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
		  int offset = ((insn & 0x300) >> 3) | (insn & 0x1f);
		  int literal_addr = (addr + ( offset << 2)) & 0xfffffffc;
		  adjust = read_memory_unsigned_integer (literal_addr, 4,
							 byte_order);
		}
	      else
		{
		  /* CSKY_16_IS_MOVI4 (insn).  */
		  adjust = (insn  & 0xff);
		}

	      if (csky_debug)
		{
		  gdb_printf (gdb_stdlog,
			      "csky: base stacksize=0x%x\n", adjust);
		}

	      /* May have zero or more instructions which modify r4.  */
	      if (csky_debug)
		{
		  gdb_printf (gdb_stdlog,
			      "csky: looking for r4 adjusters...\n");
		}
	      int offset = 2;
	      insn_len = csky_get_insn (gdbarch, addr + offset, &insn2);
	      while (CSKY_IS_R4_ADJUSTER (insn2))
		{
		  if (CSKY_32_IS_ADDI4 (insn2))
		    {
		      int imm = (insn2 & 0xfff) + 1;
		      adjust += imm;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: addi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_SUBI4 (insn2))
		    {
		      int imm = (insn2 & 0xfff) + 1;
		      adjust -= imm;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: subi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_NOR4 (insn2))
		    {
		      adjust = ~adjust;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: nor r4,r4,r4\n");
			}
		    }
		  else if (CSKY_32_IS_ROTLI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      int temp = adjust >> (32 - imm);
		      adjust <<= imm;
		      adjust |= temp;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: rotli r4,r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_LISI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      adjust <<= imm;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: lsli r4,r4,%d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_BSETI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      adjust |= (1 << imm);
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: bseti r4,r4 %d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_BCLRI4 (insn2))
		    {
		      int imm = ((insn2 >> 21) & 0x1f);
		      adjust &= ~(1 << imm);
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: bclri r4,r4 %d\n", imm);
			}
		    }
		  else if (CSKY_32_IS_IXH4 (insn2))
		    {
		      adjust *= 3;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: ixh r4,r4,r4\n");
			}
		    }
		  else if (CSKY_32_IS_IXW4 (insn2))
		    {
		      adjust *= 5;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: ixw r4,r4,r4\n");
			}
		    }
		  else if (CSKY_16_IS_ADDI4 (insn2))
		    {
		      int imm = (insn2 & 0xff) + 1;
		      adjust += imm;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: addi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_SUBI4 (insn2))
		    {
		      int imm = (insn2 & 0xff) + 1;
		      adjust -= imm;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: subi r4,%d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_NOR4 (insn2))
		    {
		      adjust = ~adjust;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: nor r4,r4\n");
			}
		    }
		  else if (CSKY_16_IS_BSETI4 (insn2))
		    {
		      int imm = (insn2 & 0x1f);
		      adjust |= (1 << imm);
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: bseti r4, %d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_BCLRI4 (insn2))
		    {
		      int imm = (insn2 & 0x1f);
		      adjust &= ~(1 << imm);
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: bclri r4, %d\n", imm);
			}
		    }
		  else if (CSKY_16_IS_LSLI4 (insn2))
		    {
		      int imm = (insn2 & 0x1f);
		      adjust <<= imm;
		      if (csky_debug)
			{
			  gdb_printf (gdb_stdlog,
				      "csky: lsli r4,r4, %d\n", imm);
			}
		    }

		  offset += insn_len;
		  insn_len = csky_get_insn (gdbarch, addr + offset, &insn2);
		};

	      if (csky_debug)
		{
		  gdb_printf (gdb_stdlog, "csky: "
			      "done looking for r4 adjusters\n");
		}

	      /* If the next instruction adjusts the stack pointer, we keep
		 everything; if not, we scrap it and we've found the end
		 of the prologue.  */
	      if (CSKY_IS_SUBU4 (insn2))
		{
		  addr += offset;
		  stacksize += adjust;
		  if (csky_debug)
		    {
		      gdb_printf (gdb_stdlog, "csky: "
				  "found stack adjustment of 0x%x"
				  " bytes.\n", adjust);
		      gdb_printf (gdb_stdlog, "csky: "
				  "skipping to new address %s\n",
				  core_addr_to_string_nz (addr));
		      gdb_printf (gdb_stdlog, "csky: continuing\n");
		    }
		  continue;
		}

	      /* None of these instructions are prologue, so don't touch
		 anything.  */
	      if (csky_debug)
		{
		  gdb_printf (gdb_stdlog, "csky: no subu sp,r4; "
			      "NOT altering stacksize.\n");
		}
	      break;
	    }
	}

      /* This is not a prologue instruction, so stop here.  */
      if (csky_debug)
	{
	  gdb_printf (gdb_stdlog, "csky: insn is not a prologue"
		      " insn -- ending scan\n");
	}
      break;
    }

  if (this_cache)
    {
      CORE_ADDR unwound_fp;
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      this_cache->framesize = framesize;

      if (is_fp_saved)
	{
	  this_cache->framereg = CSKY_FP_REGNUM;
	  unwound_fp = get_frame_register_unsigned (this_frame,
						    this_cache->framereg);
	  this_cache->prev_sp = unwound_fp + adjust_fp;
	}
      else
	{
	  this_cache->framereg = CSKY_SP_REGNUM;
	  unwound_fp = get_frame_register_unsigned (this_frame,
						    this_cache->framereg);
	  this_cache->prev_sp = unwound_fp + stacksize;
	}

      /* Note where saved registers are stored.  The offsets in
	 REGISTER_OFFSETS are computed relative to the top of the frame.  */
      for (rn = 0; rn < CSKY_NUM_GREGS; rn++)
	{
	  if (register_offsets[rn] >= 0)
	    {
	      this_cache->saved_regs[rn].set_addr (this_cache->prev_sp
						   - register_offsets[rn]);
	      if (csky_debug)
		{
		  CORE_ADDR rn_value = read_memory_unsigned_integer (
		    this_cache->saved_regs[rn].addr (), 4, byte_order);
		  gdb_printf (gdb_stdlog, "Saved register %s "
			      "stored at 0x%08lx, value=0x%08lx\n",
			      csky_register_names[rn],
			      (unsigned long)
			      this_cache->saved_regs[rn].addr (),
			      (unsigned long) rn_value);
		}
	    }
	}
      if (lr_type == LR_TYPE_EPC)
	{
	  /* rte || epc .  */
	  this_cache->saved_regs[CSKY_PC_REGNUM]
	    = this_cache->saved_regs[CSKY_EPC_REGNUM];
	}
      else if (lr_type == LR_TYPE_FPC)
	{
	  /* rfi || fpc .  */
	  this_cache->saved_regs[CSKY_PC_REGNUM]
	    = this_cache->saved_regs[CSKY_FPC_REGNUM];
	}
      else
	{
	  this_cache->saved_regs[CSKY_PC_REGNUM]
	    = this_cache->saved_regs[CSKY_LR_REGNUM];
	}
    }

  return addr;
}

/* Detect whether PC is at a point where the stack frame has been
   destroyed.  */

static int
csky_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  unsigned int insn;
  CORE_ADDR addr;
  CORE_ADDR func_start, func_end;

  if (!find_pc_partial_function (pc, NULL, &func_start, &func_end))
    return 0;

  bool fp_saved = false;
  int insn_len;
  for (addr = func_start; addr < func_end; addr += insn_len)
    {
      /* Get next insn.  */
      insn_len = csky_get_insn (gdbarch, addr, &insn);

      if (insn_len == 2)
	{
	  /* Is sp is saved to fp.  */
	  if (CSKY_16_IS_MOV_FP_SP (insn))
	    fp_saved = true;
	  /* If sp was saved to fp and now being restored from
	     fp then it indicates the start of epilog.  */
	  else if (fp_saved && CSKY_16_IS_MOV_SP_FP (insn))
	    return pc >= addr;
	}
    }
  return 0;
}

/* Implement the skip_prologue gdbarch hook.  */

static CORE_ADDR
csky_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr, func_end;
  const int default_search_limit = 128;

  /* See if we can find the end of the prologue using the symbol table.  */
  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);

      if (post_prologue_pc != 0)
	return std::max (pc, post_prologue_pc);
    }
  else
    func_end = pc + default_search_limit;

  /* Find the end of prologue.  Default lr_type.  */
  return csky_analyze_prologue (gdbarch, pc, func_end, func_end,
				NULL, NULL, LR_TYPE_R15);
}

/* Implement the breakpoint_kind_from_pc gdbarch method.  */

static int
csky_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  if (csky_pc_is_csky16 (gdbarch, *pcptr))
    return CSKY_INSN_SIZE16;
  else
    return CSKY_INSN_SIZE32;
}

/* Implement the sw_breakpoint_from_kind gdbarch method.  */

static const gdb_byte *
csky_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
  *size = kind;
  if (kind == CSKY_INSN_SIZE16)
    {
      static gdb_byte csky_16_breakpoint[] = { 0, 0 };
      return csky_16_breakpoint;
    }
  else
    {
      static gdb_byte csky_32_breakpoint[] = { 0, 0, 0, 0 };
      return csky_32_breakpoint;
    }
}

/* Determine link register type.  */

static lr_type_t
csky_analyze_lr_type (struct gdbarch *gdbarch,
		      CORE_ADDR start_pc, CORE_ADDR end_pc)
{
  CORE_ADDR addr;
  unsigned int insn, insn_len;
  insn_len = 2;

  for (addr = start_pc; addr < end_pc; addr += insn_len)
    {
      insn_len = csky_get_insn (gdbarch, addr, &insn);
      if (insn_len == 4)
	{
	  if (CSKY_32_IS_MFCR_EPSR (insn) || CSKY_32_IS_MFCR_EPC (insn)
	      || CSKY_32_IS_RTE (insn))
	    return LR_TYPE_EPC;
	}
      else if (CSKY_32_IS_MFCR_FPSR (insn) || CSKY_32_IS_MFCR_FPC (insn)
	       || CSKY_32_IS_RFI (insn))
	return LR_TYPE_FPC;
      else if (CSKY_32_IS_JMP (insn) || CSKY_32_IS_BR (insn)
	       || CSKY_32_IS_JMPIX (insn) || CSKY_32_IS_JMPI (insn))
	return LR_TYPE_R15;
      else
	{
	  /* 16 bit instruction.  */
	  if (CSKY_16_IS_JMP (insn) || CSKY_16_IS_BR (insn)
	      || CSKY_16_IS_JMPIX (insn))
	    return LR_TYPE_R15;
	}
    }
    return LR_TYPE_R15;
}

/* Heuristic unwinder.  */

static struct csky_unwind_cache *
csky_frame_unwind_cache (frame_info_ptr this_frame)
{
  CORE_ADDR prologue_start, prologue_end, func_end, prev_pc, block_addr;
  struct csky_unwind_cache *cache;
  const struct block *bl;
  unsigned long func_size = 0;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  unsigned int sp_regnum = CSKY_SP_REGNUM;

  /* Default lr type is r15.  */
  lr_type_t lr_type = LR_TYPE_R15;

  cache = FRAME_OBSTACK_ZALLOC (struct csky_unwind_cache);
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);

  /* Assume there is no frame until proven otherwise.  */
  cache->framereg = sp_regnum;

  cache->framesize = 0;

  prev_pc = get_frame_pc (this_frame);
  block_addr = get_frame_address_in_block (this_frame);
  if (find_pc_partial_function (block_addr, NULL, &prologue_start,
				&func_end) == 0)
    /* We couldn't find a function containing block_addr, so bail out
       and hope for the best.  */
    return cache;

  /* Get the (function) symbol matching prologue_start.  */
  bl = block_for_pc (prologue_start);
  if (bl != NULL)
    func_size = bl->end () - bl->start ();
  else
    {
      struct bound_minimal_symbol msymbol
	= lookup_minimal_symbol_by_pc (prologue_start);
      if (msymbol.minsym != NULL)
	func_size = msymbol.minsym->size ();
    }

  /* If FUNC_SIZE is 0 we may have a special-case use of lr
     e.g. exception or interrupt.  */
  if (func_size == 0)
    lr_type = csky_analyze_lr_type (gdbarch, prologue_start, func_end);

  prologue_end = std::min (func_end, prev_pc);

  /* Analyze the function prologue.  */
  csky_analyze_prologue (gdbarch, prologue_start, prologue_end,
			    func_end, this_frame, cache, lr_type);

  /* gdbarch_sp_regnum contains the value and not the address.  */
  cache->saved_regs[sp_regnum].set_value (cache->prev_sp);
  return cache;
}

/* Implement the this_id function for the normal unwinder.  */

static void
csky_frame_this_id (frame_info_ptr this_frame,
		    void **this_prologue_cache, struct frame_id *this_id)
{
  struct csky_unwind_cache *cache;
  struct frame_id id;

  if (*this_prologue_cache == NULL)
    *this_prologue_cache = csky_frame_unwind_cache (this_frame);
  cache = (struct csky_unwind_cache *) *this_prologue_cache;

  /* This marks the outermost frame.  */
  if (cache->prev_sp == 0)
    return;

  id = frame_id_build (cache->prev_sp, get_frame_func (this_frame));
  *this_id = id;
}

/* Implement the prev_register function for the normal unwinder.  */

static struct value *
csky_frame_prev_register (frame_info_ptr this_frame,
			  void **this_prologue_cache, int regnum)
{
  struct csky_unwind_cache *cache;

  if (*this_prologue_cache == NULL)
    *this_prologue_cache = csky_frame_unwind_cache (this_frame);
  cache = (struct csky_unwind_cache *) *this_prologue_cache;

  return trad_frame_get_prev_register (this_frame, cache->saved_regs,
				       regnum);
}

/* Data structures for the normal prologue-analysis-based
   unwinder.  */

static const struct frame_unwind csky_unwind_cache = {
  "cski prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  csky_frame_this_id,
  csky_frame_prev_register,
  NULL,
  default_frame_sniffer,
  NULL,
  NULL
};

static CORE_ADDR
csky_check_long_branch (frame_info_ptr frame, CORE_ADDR pc)
{
  gdb_byte buf[8];
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order_for_code
	= gdbarch_byte_order_for_code (gdbarch);

  if (target_read_memory (pc, buf, 8) == 0)
    {
      unsigned int data0
	= extract_unsigned_integer (buf, 4, byte_order_for_code);
      unsigned int data1
	= extract_unsigned_integer (buf + 4, 4, byte_order_for_code);

      /* Case: jmpi [pc+4] : 0xeac00001
	 .long addr   */
      if (data0 == CSKY_JMPI_PC_4)
	return data1;

      /* Case: lrw t1, [pc+8] : 0xea8d0002
	       jmp t1         : 0x7834
	       nop            : 0x6c03
	       .long addr  */
      if ((data0 == CSKY_LRW_T1_PC_8) && (data1 == CSKY_JMP_T1_VS_NOP))
	{
	  if (target_read_memory (pc + 8, buf, 4) == 0)
	    return  extract_unsigned_integer (buf, 4, byte_order_for_code);
	}

      return 0;
    }

  return 0;
}

static int
csky_stub_unwind_sniffer (const struct frame_unwind *self,
			  frame_info_ptr this_frame,
			  void **this_prologue_cache)
{
  CORE_ADDR addr_in_block, pc;
  gdb_byte dummy[4];
  const char *name;
  CORE_ADDR start_addr;

  /* Get pc */
  addr_in_block = get_frame_address_in_block (this_frame);
  pc = get_frame_pc (this_frame);

  if (in_plt_section (addr_in_block)
      || target_read_memory (pc, dummy, 4) != 0)
    return 1;

  /* Find the starting address and name of the function containing the PC.  */
  if (find_pc_partial_function (pc, &name, &start_addr, NULL) == 0)
    {
      start_addr = csky_check_long_branch (this_frame, pc);
      /* if not long branch, return 0.  */
      if (start_addr != 0)
	return 1;

      return 0;
    }

  return 0;
}

static struct csky_unwind_cache *
csky_make_stub_cache (frame_info_ptr this_frame)
{
  struct csky_unwind_cache *cache;

  cache = FRAME_OBSTACK_ZALLOC (struct csky_unwind_cache);
  cache->saved_regs = trad_frame_alloc_saved_regs (this_frame);
  cache->prev_sp = get_frame_register_unsigned (this_frame, CSKY_SP_REGNUM);

  return cache;
}

static void
csky_stub_this_id (frame_info_ptr this_frame,
		  void **this_cache,
		  struct frame_id *this_id)
{
  struct csky_unwind_cache *cache;

  if (*this_cache == NULL)
    *this_cache = csky_make_stub_cache (this_frame);
  cache = (struct csky_unwind_cache *) *this_cache;

  /* Our frame ID for a stub frame is the current SP and LR.  */
  *this_id = frame_id_build (cache->prev_sp, get_frame_pc (this_frame));
}

static struct value *
csky_stub_prev_register (frame_info_ptr this_frame,
			    void **this_cache,
			    int prev_regnum)
{
  struct csky_unwind_cache *cache;

  if (*this_cache == NULL)
    *this_cache = csky_make_stub_cache (this_frame);
  cache = (struct csky_unwind_cache *) *this_cache;

  /* If we are asked to unwind the PC, then return the LR.  */
  if (prev_regnum == CSKY_PC_REGNUM)
    {
      CORE_ADDR lr;

      lr = frame_unwind_register_unsigned (this_frame, CSKY_LR_REGNUM);
      return frame_unwind_got_constant (this_frame, prev_regnum, lr);
    }

  if (prev_regnum == CSKY_SP_REGNUM)
    return frame_unwind_got_constant (this_frame, prev_regnum, cache->prev_sp);

  return trad_frame_get_prev_register (this_frame, cache->saved_regs,
				       prev_regnum);
}

static frame_unwind csky_stub_unwind = {
  "csky stub",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  csky_stub_this_id,
  csky_stub_prev_register,
  NULL,
  csky_stub_unwind_sniffer
};

/* Implement the this_base, this_locals, and this_args hooks
   for the normal unwinder.  */

static CORE_ADDR
csky_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct csky_unwind_cache *cache;

  if (*this_cache == NULL)
    *this_cache = csky_frame_unwind_cache (this_frame);
  cache = (struct csky_unwind_cache *) *this_cache;

  return cache->prev_sp - cache->framesize;
}

static const struct frame_base csky_frame_base = {
  &csky_unwind_cache,
  csky_frame_base_address,
  csky_frame_base_address,
  csky_frame_base_address
};

/* Initialize register access method.  */

static void
csky_dwarf2_frame_init_reg (struct gdbarch *gdbarch, int regnum,
			    struct dwarf2_frame_state_reg *reg,
			    frame_info_ptr this_frame)
{
  if (regnum == gdbarch_pc_regnum (gdbarch))
    reg->how = DWARF2_FRAME_REG_RA;
  else if (regnum == gdbarch_sp_regnum (gdbarch))
    reg->how = DWARF2_FRAME_REG_CFA;
}

/* Create csky register groups.  */

static void
csky_init_reggroup ()
{
  cr_reggroup = reggroup_new ("cr", USER_REGGROUP);
  fr_reggroup = reggroup_new ("fr", USER_REGGROUP);
  vr_reggroup = reggroup_new ("vr", USER_REGGROUP);
  mmu_reggroup = reggroup_new ("mmu", USER_REGGROUP);
  prof_reggroup = reggroup_new ("profiling", USER_REGGROUP);
}

/* Add register groups into reggroup list.  */

static void
csky_add_reggroups (struct gdbarch *gdbarch)
{
  reggroup_add (gdbarch, cr_reggroup);
  reggroup_add (gdbarch, fr_reggroup);
  reggroup_add (gdbarch, vr_reggroup);
  reggroup_add (gdbarch, mmu_reggroup);
  reggroup_add (gdbarch, prof_reggroup);
}

/* Return the groups that a CSKY register can be categorised into.  */

static int
csky_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			  const struct reggroup *reggroup)
{
  int raw_p;

  if (gdbarch_register_name (gdbarch, regnum)[0] == '\0')
    return 0;

  if (reggroup == all_reggroup)
    return 1;

  raw_p = regnum < gdbarch_num_regs (gdbarch);
  if (reggroup == save_reggroup || reggroup == restore_reggroup)
    return raw_p;

  if ((((regnum >= CSKY_R0_REGNUM) && (regnum <= CSKY_R0_REGNUM + 31))
       || (regnum == CSKY_PC_REGNUM)
       || (regnum == CSKY_EPC_REGNUM)
       || (regnum == CSKY_CR0_REGNUM)
       || (regnum == CSKY_EPSR_REGNUM))
      && (reggroup == general_reggroup))
    return 1;

  if (((regnum == CSKY_PC_REGNUM)
       || ((regnum >= CSKY_CR0_REGNUM)
	   && (regnum <= CSKY_CR0_REGNUM + 30)))
      && (reggroup == cr_reggroup))
    return 2;

  if ((((regnum >= CSKY_VR0_REGNUM) && (regnum <= CSKY_VR0_REGNUM + 15))
       || ((regnum >= CSKY_FCR_REGNUM)
	   && (regnum <= CSKY_FCR_REGNUM + 2)))
      && (reggroup == vr_reggroup))
    return 3;

  if (((regnum >= CSKY_MMU_REGNUM) && (regnum <= CSKY_MMU_REGNUM + 8))
      && (reggroup == mmu_reggroup))
    return 4;

  if (((regnum >= CSKY_PROFCR_REGNUM)
       && (regnum <= CSKY_PROFCR_REGNUM + 48))
      && (reggroup == prof_reggroup))
    return 5;

  if ((((regnum >= CSKY_FR0_REGNUM) && (regnum <= CSKY_FR0_REGNUM + 15))
       || ((regnum >= CSKY_FCR_REGNUM) && (regnum <= CSKY_FCR_REGNUM + 2)))
      && (reggroup == fr_reggroup))
    return 6;

  if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    {
      if (tdesc_register_in_reggroup_p (gdbarch, regnum, reggroup) > 0)
	return 7;
    }

  return 0;
}

/* Implement the dwarf2_reg_to_regnum gdbarch method.  */

static int
csky_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int dw_reg)
{
  /* For GPRs.  */
  if (dw_reg >= CSKY_R0_REGNUM && dw_reg <= CSKY_R0_REGNUM + 31)
    return dw_reg;

  /* For Hi, Lo, PC.  */
  if (dw_reg == CSKY_HI_REGNUM || dw_reg == CSKY_LO_REGNUM
      || dw_reg == CSKY_PC_REGNUM)
    return dw_reg;

  /* For Float and Vector pseudo registers.  */
  if (dw_reg >= FV_PSEUDO_REGNO_FIRST && dw_reg <= FV_PSEUDO_REGNO_LAST)
    {
      char name_buf[4];

      xsnprintf (name_buf, sizeof (name_buf), "s%d",
		 dw_reg - FV_PSEUDO_REGNO_FIRST);
      return user_reg_map_name_to_regnum (gdbarch, name_buf,
					  strlen (name_buf));
    }

  /* Others, unknown.  */
  return -1;
}

/* Check whether xml has discribled the essential regs.  */

static int
csky_essential_reg_check (const struct csky_supported_tdesc_register *reg)
{
  if ((strcmp (reg->name , "pc") == 0)
      && (reg->num == CSKY_PC_REGNUM))
    return CSKY_TDESC_REGS_PC_NUMBERED;
  else if ((strcmp (reg->name , "r14") == 0)
      && (reg->num == CSKY_SP_REGNUM))
    return CSKY_TDESC_REGS_SP_NUMBERED;
  else if ((strcmp (reg->name , "r15") == 0)
      && (reg->num == CSKY_LR_REGNUM))
    return CSKY_TDESC_REGS_LR_NUMBERED;
  else
    return 0;
}

/* Check whether xml has discribled the fr0~fr15 regs.  */

static int
csky_fr0_fr15_reg_check (const struct csky_supported_tdesc_register *reg) {
  int i = 0;
  for (i = 0; i < 16; i++)
    {
      if ((strcmp (reg->name, csky_supported_fpu_regs[i].name) == 0)
	  && (csky_supported_fpu_regs[i].num == reg->num))
	return (1 << i);
    }

  return 0;
};

/* Check whether xml has discribled the fr16~fr31 regs.  */

static int
csky_fr16_fr31_reg_check (const struct csky_supported_tdesc_register *reg) {
  int i = 0;
  for (i = 0; i < 16; i++)
    {
      if ((strcmp (reg->name, csky_supported_fpu_regs[i + 16].name) == 0)
	  && (csky_supported_fpu_regs[i + 16].num == reg->num))
	return (1 << i);
    }

  return 0;
};

/* Check whether xml has discribled the vr0~vr15 regs.  */

static int
csky_vr0_vr15_reg_check (const struct csky_supported_tdesc_register *reg) {
  int i = 0;
  for (i = 0; i < 16; i++)
    {
      if ((strcmp (reg->name, csky_supported_fpu_regs[i + 32].name) == 0)
	  && (csky_supported_fpu_regs[i + 32].num == reg->num))
	return (1 << i);
    }

  return 0;
};

/* Return pseudo reg's name.  */

static const char *
csky_pseudo_register_name (struct gdbarch *gdbarch, int regno)
{
  int num_regs = gdbarch_num_regs (gdbarch);
  csky_gdbarch_tdep *tdep
    = gdbarch_tdep<csky_gdbarch_tdep> (gdbarch);

  regno -= num_regs;

  if (tdep->fv_pseudo_registers_count)
    {
      static const char *const fv_pseudo_names[] = {
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15",
	"s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
	"s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31",
	"s32", "s33", "s34", "s35", "s36", "s37", "s38", "s39",
	"s40", "s41", "s42", "s43", "s44", "s45", "s46", "s47",
	"s48", "s49", "s50", "s51", "s52", "s53", "s54", "s55",
	"s56", "s57", "s58", "s59", "s60", "s61", "s62", "s63",
	"s64", "s65", "s66", "s67", "s68", "s69", "s70", "s71",
	"s72", "s73", "s74", "s75", "s76", "s77", "s78", "s79",
	"s80", "s81", "s82", "s83", "s84", "s85", "s86", "s87",
	"s88", "s89", "s90", "s91", "s92", "s93", "s94", "s95",
	"s96", "s97", "s98", "s99", "s100", "s101", "s102", "s103",
	"s104", "s105", "s106", "s107", "s108", "s109", "s110", "s111",
	"s112", "s113", "s114", "s115", "s116", "s117", "s118", "s119",
	"s120", "s121", "s122", "s123", "s124", "s125", "s126", "s127",
      };

      if (regno < tdep->fv_pseudo_registers_count)
	{
	  if ((regno < 64) && ((regno % 4) >= 2) && !tdep->has_vr0)
	    return "";
	  else if ((regno >= 64) && ((regno % 4) >= 2))
	    return "";
	  else
	    return fv_pseudo_names[regno];
	}
    }

  return "";
}

/* Read for csky pseudo regs.  */

static enum register_status
csky_pseudo_register_read (struct gdbarch *gdbarch,
			   struct readable_regcache *regcache,
			   int regnum, gdb_byte *buf)
{
  int num_regs = gdbarch_num_regs (gdbarch);
  csky_gdbarch_tdep *tdep
    = gdbarch_tdep<csky_gdbarch_tdep> (gdbarch);

  regnum -= num_regs;

  if (regnum < tdep->fv_pseudo_registers_count)
    {
      enum register_status status;
      int gdb_regnum = 0;
      int offset = 0;
      gdb_byte reg_buf[16];

      /* Ensure getting s0~s63 from vrx if tdep->has_vr0 is true.  */
      if (tdep->has_vr0)
	{
	  if (regnum < 64)
	    {
	      gdb_regnum = CSKY_VR0_REGNUM + (regnum / 4);
	      offset = (regnum % 4) * 4;
	    }
	  else
	    {
	      gdb_regnum = CSKY_FR16_REGNUM + ((regnum - 64) / 4);
	      if ((regnum % 4) >= 2)
		return REG_UNAVAILABLE;
	      offset = (regnum % 2) * 4;
	    }
	}
      else
	{
	  gdb_regnum = CSKY_FR0_REGNUM + (regnum / 4);
	  if ((regnum % 4) >= 2)
	    return REG_UNAVAILABLE;
	  offset = (regnum % 2) * 4;
	}

      status = regcache->raw_read (gdb_regnum, reg_buf);
      if (status == REG_VALID)
	memcpy (buf, reg_buf + offset, 4);
      return status;
    }

  return REG_UNKNOWN;
}

/* Write for csky pseudo regs.  */

static void
csky_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache,
			    int regnum, const gdb_byte *buf)
{
  int num_regs = gdbarch_num_regs (gdbarch);
  csky_gdbarch_tdep *tdep
    = gdbarch_tdep<csky_gdbarch_tdep> (gdbarch);

  regnum -= num_regs;

  if (regnum < tdep->fv_pseudo_registers_count)
    {
      gdb_byte reg_buf[16];
      int gdb_regnum = 0;
      int offset = 0;

      if (tdep->has_vr0)
	{
	  if (regnum < 64)
	    {
	      gdb_regnum = CSKY_VR0_REGNUM + (regnum / 4);
	      offset = (regnum % 4) * 4;
	    }
	  else
	    {
	      gdb_regnum = CSKY_FR16_REGNUM + ((regnum - 64) / 4);
	      if ((regnum % 4) >= 2)
		return;
	      offset = (regnum % 2) * 4;
	    }
	}
      else
	{
	  gdb_regnum = CSKY_FR0_REGNUM + (regnum / 4);
	  if ((regnum % 4) >= 2)
	    return;
	  offset = (regnum % 2) * 4;
	}

      regcache->raw_read (gdb_regnum, reg_buf);
      memcpy (reg_buf + offset, buf, 4);
      regcache->raw_write (gdb_regnum, reg_buf);
      return;
    }

  return;
}

/* Initialize the current architecture based on INFO.  If possible,
   re-use an architecture from ARCHES, which is a list of
   architectures already created during this debugging session.

   Called at program startup, when reading a core file, and when
   reading a binary file.  */

static struct gdbarch *
csky_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  /* Analyze info.abfd.  */
  unsigned int fpu_abi = 0;
  unsigned int vdsp_version = 0;
  unsigned int fpu_hardfp = 0;
  /* Analyze info.target_desc */
  int num_regs = 0;
  int has_fr0 = 0;
  int has_fr16 = 0;
  int has_vr0 = 0;
  tdesc_arch_data_up tdesc_data;

  if (tdesc_has_registers (info.target_desc))
    {
      int valid_p = 0;
      int numbered = 0;
      int index = 0;
      int i = 0;
      int feature_names_count = ARRAY_SIZE (csky_supported_tdesc_feature_names);
      int support_tdesc_regs_count
	= csky_get_supported_tdesc_registers_count();
      const struct csky_supported_tdesc_register *tdesc_reg;
      const struct tdesc_feature *feature;

      tdesc_data = tdesc_data_alloc ();
      for (index = 0; index < feature_names_count; index ++)
	{
	  feature = tdesc_find_feature (info.target_desc,
					csky_supported_tdesc_feature_names[index]);
	  if (feature != NULL)
	    {
	      for (i = 0; i < support_tdesc_regs_count; i++)
		{
		  tdesc_reg = csky_get_supported_register_by_index (i);
		  if (!tdesc_reg)
		    break;
		  numbered = tdesc_numbered_register (feature, tdesc_data.get(),
						      tdesc_reg->num,
						      tdesc_reg->name);
		  if (numbered) {
		      valid_p |= csky_essential_reg_check (tdesc_reg);
		      has_fr0 |= csky_fr0_fr15_reg_check (tdesc_reg);
		      has_fr16 |= csky_fr16_fr31_reg_check (tdesc_reg);
		      has_vr0 |= csky_vr0_vr15_reg_check (tdesc_reg);
		      if (num_regs < tdesc_reg->num)
			num_regs = tdesc_reg->num;
		  }
		}
	    }
	}
      if (valid_p != CSKY_TDESC_REGS_ESSENTIAL_VALUE)
	return NULL;
    }

  /* When the type of bfd file is srec(or any files are not elf),
     the E_FLAGS will be not credible.  */
  if (info.abfd != NULL && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour)
    {
      /* Get FPU, VDSP build options.  */
      fpu_abi = bfd_elf_get_obj_attr_int (info.abfd,
					  OBJ_ATTR_PROC,
					  Tag_CSKY_FPU_ABI);
      vdsp_version = bfd_elf_get_obj_attr_int (info.abfd,
					       OBJ_ATTR_PROC,
					       Tag_CSKY_VDSP_VERSION);
      fpu_hardfp = bfd_elf_get_obj_attr_int (info.abfd,
					     OBJ_ATTR_PROC,
					     Tag_CSKY_FPU_HARDFP);
    }

  /* Find a candidate among the list of pre-declared architectures.  */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      csky_gdbarch_tdep *tdep
	= gdbarch_tdep<csky_gdbarch_tdep> (arches->gdbarch);
      if (fpu_abi != tdep->fpu_abi)
	continue;
      if (vdsp_version != tdep->vdsp_version)
	continue;
      if (fpu_hardfp != tdep->fpu_hardfp)
	continue;

      /* Found a match.  */
      return arches->gdbarch;
    }

  /* None found, create a new architecture from the information
     provided.  */
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new csky_gdbarch_tdep));
  csky_gdbarch_tdep *tdep = gdbarch_tdep<csky_gdbarch_tdep> (gdbarch);

  tdep->fpu_abi = fpu_abi;
  tdep->vdsp_version = vdsp_version;
  tdep->fpu_hardfp = fpu_hardfp;

  if (tdesc_data != NULL)
    {
      if ((has_vr0 == CSKY_FULL16_ONEHOT_VALUE)
	  && (has_fr16 == CSKY_FULL16_ONEHOT_VALUE))
	{
	  tdep->has_vr0 = 1;
	  tdep->fv_pseudo_registers_count = 128;
	}
      else if ((has_vr0 == CSKY_FULL16_ONEHOT_VALUE)
	       && (has_fr16 != CSKY_FULL16_ONEHOT_VALUE))
	{
	  tdep->has_vr0 = 1;
	  tdep->fv_pseudo_registers_count = 64;
	}
      else if ((has_fr0 == CSKY_FULL16_ONEHOT_VALUE)
	       && (has_vr0 != CSKY_FULL16_ONEHOT_VALUE))
	{
	  tdep->has_vr0 = 0;
	  tdep->fv_pseudo_registers_count = 64;
	}
      else
	{
	  tdep->has_vr0 = 0;
	  tdep->fv_pseudo_registers_count = 0;
	}
    }
  else
    {
      tdep->has_vr0 = 1;
      tdep->fv_pseudo_registers_count = 64;
    }

  /* Target data types.  */
  set_gdbarch_ptr_bit (gdbarch, 32);
  set_gdbarch_addr_bit (gdbarch, 32);
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_float_format (gdbarch, floatformats_ieee_single);
  set_gdbarch_double_format (gdbarch, floatformats_ieee_double);

  /* Information about the target architecture.  */
  set_gdbarch_return_value (gdbarch, csky_return_value);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, csky_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, csky_sw_breakpoint_from_kind);

  /* Register architecture.  */
  set_gdbarch_num_regs (gdbarch, CSKY_NUM_REGS);
  set_gdbarch_pc_regnum (gdbarch, CSKY_PC_REGNUM);
  set_gdbarch_sp_regnum (gdbarch, CSKY_SP_REGNUM);
  set_gdbarch_register_name (gdbarch, csky_register_name);
  set_gdbarch_register_type (gdbarch, csky_register_type);
  set_gdbarch_read_pc (gdbarch, csky_read_pc);
  set_gdbarch_write_pc (gdbarch, csky_write_pc);
  csky_add_reggroups (gdbarch);
  set_gdbarch_register_reggroup_p (gdbarch, csky_register_reggroup_p);
  set_gdbarch_stab_reg_to_regnum (gdbarch, csky_dwarf_reg_to_regnum);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, csky_dwarf_reg_to_regnum);
  dwarf2_frame_set_init_reg (gdbarch, csky_dwarf2_frame_init_reg);

  /* Functions to analyze frames.  */
  frame_base_set_default (gdbarch, &csky_frame_base);
  set_gdbarch_skip_prologue (gdbarch, csky_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_frame_align (gdbarch, csky_frame_align);
  set_gdbarch_stack_frame_destroyed_p (gdbarch, csky_stack_frame_destroyed_p);

  /* Functions handling dummy frames.  */
  set_gdbarch_push_dummy_call (gdbarch, csky_push_dummy_call);

  /* Frame unwinders.  Use DWARF debug info if available,
     otherwise use our own unwinder.  */
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &csky_stub_unwind);
  frame_unwind_append_unwinder (gdbarch, &csky_unwind_cache);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  /* Support simple overlay manager.  */
  set_gdbarch_overlay_update (gdbarch, simple_overlay_update);
  set_gdbarch_char_signed (gdbarch, 0);

  if (tdesc_data != nullptr)
    {
      set_gdbarch_num_regs (gdbarch, (num_regs + 1));
      tdesc_use_registers (gdbarch, info.target_desc, std::move (tdesc_data));
      set_gdbarch_register_type (gdbarch, csky_register_type);
      set_gdbarch_register_reggroup_p (gdbarch,
				       csky_register_reggroup_p);
    }

  if (tdep->fv_pseudo_registers_count)
    {
      set_gdbarch_num_pseudo_regs (gdbarch,
				   tdep->fv_pseudo_registers_count);
      set_gdbarch_pseudo_register_read (gdbarch,
					csky_pseudo_register_read);
      set_gdbarch_deprecated_pseudo_register_write
	(gdbarch, csky_pseudo_register_write);
      set_tdesc_pseudo_register_name (gdbarch, csky_pseudo_register_name);
    }

  return gdbarch;
}

void _initialize_csky_tdep ();
void
_initialize_csky_tdep ()
{

  gdbarch_register (bfd_arch_csky, csky_gdbarch_init);

  csky_init_reggroup ();

  /* Allow debugging this file's internals.  */
  add_setshow_boolean_cmd ("csky", class_maintenance, &csky_debug,
			   _("Set C-Sky debugging."),
			   _("Show C-Sky debugging."),
			   _("When on, C-Sky specific debugging is enabled."),
			   NULL,
			   NULL,
			   &setdebuglist, &showdebuglist);
}
