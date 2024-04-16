/* Target-dependent code for the NDS32 architecture, for GDB.

   Copyright (C) 2013-2024 Free Software Foundation, Inc.
   Contributed by Andes Technology Corporation.

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
#include "frame.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcore.h"
#include "value.h"
#include "reggroups.h"
#include "inferior.h"
#include "osabi.h"
#include "arch-utils.h"
#include "regcache.h"
#include "dis-asm.h"
#include "user-regs.h"
#include "elf-bfd.h"
#include "dwarf2/frame.h"
#include "remote.h"
#include "target-descriptions.h"

#include "nds32-tdep.h"
#include "elf/nds32.h"
#include "opcode/nds32.h"
#include <algorithm>

#include "features/nds32.c"

/* Simple macros for instruction analysis.  */
#define CHOP_BITS(insn, n)	(insn & ~__MASK (n))
#define N32_LSMW_ENABLE4(insn)	(((insn) >> 6) & 0xf)
#define N32_SMW_ADM \
	N32_TYPE4 (LSMW, 0, 0, 0, 1, (N32_LSMW_ADM << 2) | N32_LSMW_LSMW)
#define N32_LMW_BIM \
	N32_TYPE4 (LSMW, 0, 0, 0, 0, (N32_LSMW_BIM << 2) | N32_LSMW_LSMW)
#define N32_FLDI_SP \
	N32_TYPE2 (LDC, 0, REG_SP, 0)

/* Use an invalid address value as 'not available' marker.  */
enum { REG_UNAVAIL = (CORE_ADDR) -1 };

/* Use an impossible value as invalid offset.  */
enum { INVALID_OFFSET = (CORE_ADDR) -1 };

/* Instruction groups for NDS32 epilogue analysis.  */
enum
{
  /* Instructions used everywhere, not only in epilogue.  */
  INSN_NORMAL,
  /* Instructions used to reset sp for local vars, arguments, etc.  */
  INSN_RESET_SP,
  /* Instructions used to recover saved regs and to recover padding.  */
  INSN_RECOVER,
  /* Instructions used to return to the caller.  */
  INSN_RETURN,
  /* Instructions used to recover saved regs and to return to the caller.  */
  INSN_RECOVER_RETURN,
};

static const char *const nds32_register_names[] =
{
  /* 32 GPRs.  */
  "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
  "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
  "r24", "r25", "r26", "r27", "fp", "gp", "lp", "sp",
  /* PC.  */
  "pc",
};

static const char *const nds32_fdr_register_names[] =
{
  "fd0", "fd1", "fd2", "fd3", "fd4", "fd5", "fd6", "fd7",
  "fd8", "fd9", "fd10", "fd11", "fd12", "fd13", "fd14", "fd15",
  "fd16", "fd17", "fd18", "fd19", "fd20", "fd21", "fd22", "fd23",
  "fd24", "fd25", "fd26", "fd27", "fd28", "fd29", "fd30", "fd31"
};

static const char *const nds32_fsr_register_names[] =
{
  "fs0", "fs1", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7",
  "fs8", "fs9", "fs10", "fs11", "fs12", "fs13", "fs14", "fs15",
  "fs16", "fs17", "fs18", "fs19", "fs20", "fs21", "fs22", "fs23",
  "fs24", "fs25", "fs26", "fs27", "fs28", "fs29", "fs30", "fs31"
};

/* The number of registers for four FPU configuration options.  */
const int num_fdr_map[] = { 4, 8, 16, 32 };
const int num_fsr_map[] = { 8, 16, 32, 32 };

/* Aliases for registers.  */
static const struct
{
  const char *name;
  const char *alias;
} nds32_register_aliases[] =
{
  {"r15", "ta"},
  {"r26", "p0"},
  {"r27", "p1"},
  {"fp", "r28"},
  {"gp", "r29"},
  {"lp", "r30"},
  {"sp", "r31"},

  {"cr0", "cpu_ver"},
  {"cr1", "icm_cfg"},
  {"cr2", "dcm_cfg"},
  {"cr3", "mmu_cfg"},
  {"cr4", "msc_cfg"},
  {"cr5", "core_id"},
  {"cr6", "fucop_exist"},
  {"cr7", "msc_cfg2"},

  {"ir0", "psw"},
  {"ir1", "ipsw"},
  {"ir2", "p_psw"},
  {"ir3", "ivb"},
  {"ir4", "eva"},
  {"ir5", "p_eva"},
  {"ir6", "itype"},
  {"ir7", "p_itype"},
  {"ir8", "merr"},
  {"ir9", "ipc"},
  {"ir10", "p_ipc"},
  {"ir11", "oipc"},
  {"ir12", "p_p0"},
  {"ir13", "p_p1"},
  {"ir14", "int_mask"},
  {"ir15", "int_pend"},
  {"ir16", "sp_usr"},
  {"ir17", "sp_priv"},
  {"ir18", "int_pri"},
  {"ir19", "int_ctrl"},
  {"ir20", "sp_usr1"},
  {"ir21", "sp_priv1"},
  {"ir22", "sp_usr2"},
  {"ir23", "sp_priv2"},
  {"ir24", "sp_usr3"},
  {"ir25", "sp_priv3"},
  {"ir26", "int_mask2"},
  {"ir27", "int_pend2"},
  {"ir28", "int_pri2"},
  {"ir29", "int_trigger"},

  {"mr0", "mmu_ctl"},
  {"mr1", "l1_pptb"},
  {"mr2", "tlb_vpn"},
  {"mr3", "tlb_data"},
  {"mr4", "tlb_misc"},
  {"mr5", "vlpt_idx"},
  {"mr6", "ilmb"},
  {"mr7", "dlmb"},
  {"mr8", "cache_ctl"},
  {"mr9", "hsmp_saddr"},
  {"mr10", "hsmp_eaddr"},
  {"mr11", "bg_region"},

  {"dr0", "bpc0"},
  {"dr1", "bpc1"},
  {"dr2", "bpc2"},
  {"dr3", "bpc3"},
  {"dr4", "bpc4"},
  {"dr5", "bpc5"},
  {"dr6", "bpc6"},
  {"dr7", "bpc7"},
  {"dr8", "bpa0"},
  {"dr9", "bpa1"},
  {"dr10", "bpa2"},
  {"dr11", "bpa3"},
  {"dr12", "bpa4"},
  {"dr13", "bpa5"},
  {"dr14", "bpa6"},
  {"dr15", "bpa7"},
  {"dr16", "bpam0"},
  {"dr17", "bpam1"},
  {"dr18", "bpam2"},
  {"dr19", "bpam3"},
  {"dr20", "bpam4"},
  {"dr21", "bpam5"},
  {"dr22", "bpam6"},
  {"dr23", "bpam7"},
  {"dr24", "bpv0"},
  {"dr25", "bpv1"},
  {"dr26", "bpv2"},
  {"dr27", "bpv3"},
  {"dr28", "bpv4"},
  {"dr29", "bpv5"},
  {"dr30", "bpv6"},
  {"dr31", "bpv7"},
  {"dr32", "bpcid0"},
  {"dr33", "bpcid1"},
  {"dr34", "bpcid2"},
  {"dr35", "bpcid3"},
  {"dr36", "bpcid4"},
  {"dr37", "bpcid5"},
  {"dr38", "bpcid6"},
  {"dr39", "bpcid7"},
  {"dr40", "edm_cfg"},
  {"dr41", "edmsw"},
  {"dr42", "edm_ctl"},
  {"dr43", "edm_dtr"},
  {"dr44", "bpmtc"},
  {"dr45", "dimbr"},
  {"dr46", "tecr0"},
  {"dr47", "tecr1"},

  {"hspr0", "hsp_ctl"},
  {"hspr1", "sp_bound"},
  {"hspr2", "sp_bound_priv"},

  {"pfr0", "pfmc0"},
  {"pfr1", "pfmc1"},
  {"pfr2", "pfmc2"},
  {"pfr3", "pfm_ctl"},
  {"pfr4", "pft_ctl"},

  {"dmar0", "dma_cfg"},
  {"dmar1", "dma_gcsw"},
  {"dmar2", "dma_chnsel"},
  {"dmar3", "dma_act"},
  {"dmar4", "dma_setup"},
  {"dmar5", "dma_isaddr"},
  {"dmar6", "dma_esaddr"},
  {"dmar7", "dma_tcnt"},
  {"dmar8", "dma_status"},
  {"dmar9", "dma_2dset"},
  {"dmar10", "dma_2dsctl"},
  {"dmar11", "dma_rcnt"},
  {"dmar12", "dma_hstatus"},

  {"racr0", "prusr_acc_ctl"},
  {"fucpr", "fucop_ctl"},

  {"idr0", "sdz_ctl"},
  {"idr1", "misc_ctl"},
  {"idr2", "ecc_misc"},

  {"secur0", "sfcr"},
  {"secur1", "sign"},
  {"secur2", "isign"},
  {"secur3", "p_isign"},
};

/* Value of a register alias.  BATON is the regnum of the corresponding
   register.  */

static struct value *
value_of_nds32_reg (frame_info_ptr frame, const void *baton)
{
  return value_of_register ((int) (intptr_t) baton,
			    get_next_frame_sentinel_okay (frame));
}

/* Implement the "frame_align" gdbarch method.  */

static CORE_ADDR
nds32_frame_align (struct gdbarch *gdbarch, CORE_ADDR sp)
{
  /* 8-byte aligned.  */
  return align_down (sp, 8);
}

/* The same insn machine code is used for little-endian and big-endian.  */
constexpr gdb_byte nds32_break_insn[] = { 0xEA, 0x00 };

typedef BP_MANIPULATION (nds32_break_insn) nds32_breakpoint;

/* Implement the "dwarf2_reg_to_regnum" gdbarch method.  */

static int
nds32_dwarf2_reg_to_regnum (struct gdbarch *gdbarch, int num)
{
  nds32_gdbarch_tdep *tdep = gdbarch_tdep<nds32_gdbarch_tdep> (gdbarch);
  const int FSR = 38;
  const int FDR = FSR + 32;

  if (num >= 0 && num < 32)
    {
      /* General-purpose registers (R0 - R31).  */
      return num;
    }
  else if (num >= FSR && num < FSR + 32)
    {
      /* Single precision floating-point registers (FS0 - FS31).  */
      return num - FSR + tdep->fs0_regnum;
    }
  else if (num >= FDR && num < FDR + 32)
    {
      /* Double precision floating-point registers (FD0 - FD31).  */
      return num - FDR + NDS32_FD0_REGNUM;
    }

  /* No match, return a inaccessible register number.  */
  return -1;
}

/* NDS32 register groups.  */
static const reggroup *nds32_cr_reggroup;
static const reggroup *nds32_ir_reggroup;
static const reggroup *nds32_mr_reggroup;
static const reggroup *nds32_dr_reggroup;
static const reggroup *nds32_pfr_reggroup;
static const reggroup *nds32_hspr_reggroup;
static const reggroup *nds32_dmar_reggroup;
static const reggroup *nds32_racr_reggroup;
static const reggroup *nds32_idr_reggroup;
static const reggroup *nds32_secur_reggroup;

static void
nds32_init_reggroups (void)
{
  nds32_cr_reggroup = reggroup_new ("cr", USER_REGGROUP);
  nds32_ir_reggroup = reggroup_new ("ir", USER_REGGROUP);
  nds32_mr_reggroup = reggroup_new ("mr", USER_REGGROUP);
  nds32_dr_reggroup = reggroup_new ("dr", USER_REGGROUP);
  nds32_pfr_reggroup = reggroup_new ("pfr", USER_REGGROUP);
  nds32_hspr_reggroup = reggroup_new ("hspr", USER_REGGROUP);
  nds32_dmar_reggroup = reggroup_new ("dmar", USER_REGGROUP);
  nds32_racr_reggroup = reggroup_new ("racr", USER_REGGROUP);
  nds32_idr_reggroup = reggroup_new ("idr", USER_REGGROUP);
  nds32_secur_reggroup = reggroup_new ("secur", USER_REGGROUP);
}

static void
nds32_add_reggroups (struct gdbarch *gdbarch)
{
  /* Add NDS32 register groups.  */
  reggroup_add (gdbarch, nds32_cr_reggroup);
  reggroup_add (gdbarch, nds32_ir_reggroup);
  reggroup_add (gdbarch, nds32_mr_reggroup);
  reggroup_add (gdbarch, nds32_dr_reggroup);
  reggroup_add (gdbarch, nds32_pfr_reggroup);
  reggroup_add (gdbarch, nds32_hspr_reggroup);
  reggroup_add (gdbarch, nds32_dmar_reggroup);
  reggroup_add (gdbarch, nds32_racr_reggroup);
  reggroup_add (gdbarch, nds32_idr_reggroup);
  reggroup_add (gdbarch, nds32_secur_reggroup);
}

/* Implement the "register_reggroup_p" gdbarch method.  */

static int
nds32_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			   const struct reggroup *reggroup)
{
  const char *reg_name;
  const char *group_name;
  int ret;

  if (reggroup == all_reggroup)
    return 1;

  /* General reggroup contains only GPRs and PC.  */
  if (reggroup == general_reggroup)
    return regnum <= NDS32_PC_REGNUM;

  if (reggroup == float_reggroup || reggroup == save_reggroup
      || reggroup == restore_reggroup)
    {
      ret = tdesc_register_in_reggroup_p (gdbarch, regnum, reggroup);
      if (ret != -1)
	return ret;

      return default_register_reggroup_p (gdbarch, regnum, reggroup);
    }

  if (reggroup == system_reggroup)
    return (regnum > NDS32_PC_REGNUM)
	    && !nds32_register_reggroup_p (gdbarch, regnum, float_reggroup);

  /* The NDS32 reggroup contains registers whose name is prefixed
     by reggroup name.  */
  reg_name = gdbarch_register_name (gdbarch, regnum);
  group_name = reggroup->name ();
  return !strncmp (reg_name, group_name, strlen (group_name));
}

/* Implement the "pseudo_register_type" tdesc_arch_data method.  */

static struct type *
nds32_pseudo_register_type (struct gdbarch *gdbarch, int regnum)
{
  regnum -= gdbarch_num_regs (gdbarch);

  /* Currently, only FSRs could be defined as pseudo registers.  */
  if (regnum < gdbarch_num_pseudo_regs (gdbarch))
    {
      type_allocator alloc (gdbarch);
      return init_float_type (alloc, -1, "builtin_type_ieee_single",
			      floatformats_ieee_single);
    }

  warning (_("Unknown nds32 pseudo register %d."), regnum);
  return NULL;
}

/* Implement the "pseudo_register_name" tdesc_arch_data method.  */

static const char *
nds32_pseudo_register_name (struct gdbarch *gdbarch, int regnum)
{
  regnum -= gdbarch_num_regs (gdbarch);

  /* Currently, only FSRs could be defined as pseudo registers.  */
  gdb_assert (regnum < gdbarch_num_pseudo_regs (gdbarch));
  return nds32_fsr_register_names[regnum];
}

/* Implement the "pseudo_register_read" gdbarch method.  */

static enum register_status
nds32_pseudo_register_read (struct gdbarch *gdbarch,
			    readable_regcache *regcache, int regnum,
			    gdb_byte *buf)
{
  nds32_gdbarch_tdep *tdep = gdbarch_tdep<nds32_gdbarch_tdep> (gdbarch);
  gdb_byte reg_buf[8];
  int offset, fdr_regnum;
  enum register_status status;

  /* This function is registered in nds32_gdbarch_init only after these are
     set.  */
  gdb_assert (tdep->fpu_freg != -1);
  gdb_assert (tdep->use_pseudo_fsrs != 0);

  regnum -= gdbarch_num_regs (gdbarch);

  /* Currently, only FSRs could be defined as pseudo registers.  */
  if (regnum < gdbarch_num_pseudo_regs (gdbarch))
    {
      /* fs0 is always the most significant half of fd0.  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	offset = (regnum & 1) ? 4 : 0;
      else
	offset = (regnum & 1) ? 0 : 4;

      fdr_regnum = NDS32_FD0_REGNUM + (regnum >> 1);
      status = regcache->raw_read (fdr_regnum, reg_buf);
      if (status == REG_VALID)
	memcpy (buf, reg_buf + offset, 4);

      return status;
    }

  gdb_assert_not_reached ("invalid pseudo register number");
}

/* Implement the "pseudo_register_write" gdbarch method.  */

static void
nds32_pseudo_register_write (struct gdbarch *gdbarch,
			     struct regcache *regcache, int regnum,
			     const gdb_byte *buf)
{
  nds32_gdbarch_tdep *tdep = gdbarch_tdep<nds32_gdbarch_tdep> (gdbarch);
  gdb_byte reg_buf[8];
  int offset, fdr_regnum;

  /* This function is registered in nds32_gdbarch_init only after these are
     set.  */
  gdb_assert (tdep->fpu_freg != -1);
  gdb_assert (tdep->use_pseudo_fsrs != 0);

  regnum -= gdbarch_num_regs (gdbarch);

  /* Currently, only FSRs could be defined as pseudo registers.  */
  if (regnum < gdbarch_num_pseudo_regs (gdbarch))
    {
      /* fs0 is always the most significant half of fd0.  */
      if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
	offset = (regnum & 1) ? 4 : 0;
      else
	offset = (regnum & 1) ? 0 : 4;

      fdr_regnum = NDS32_FD0_REGNUM + (regnum >> 1);
      regcache->raw_read (fdr_regnum, reg_buf);
      memcpy (reg_buf + offset, buf, 4);
      regcache->raw_write (fdr_regnum, reg_buf);
      return;
    }

  gdb_assert_not_reached ("invalid pseudo register number");
}

/* Helper function for NDS32 ABI.  Return true if FPRs can be used
   to pass function arguments and return value.  */

static int
nds32_abi_use_fpr (int elf_abi)
{
  return elf_abi == E_NDS_ABI_V2FP_PLUS;
}

/* Helper function for NDS32 ABI.  Return true if GPRs and stack
   can be used together to pass an argument.  */

static int
nds32_abi_split (int elf_abi)
{
  return elf_abi == E_NDS_ABI_AABI;
}

#define NDS32_NUM_SAVED_REGS (NDS32_LP_REGNUM + 1)

struct nds32_frame_cache
{
  /* The previous frame's inner most stack address.  Used as this
     frame ID's stack_addr.  */
  CORE_ADDR prev_sp;

  /* The frame's base, optionally used by the high-level debug info.  */
  CORE_ADDR base;

  /* During prologue analysis, keep how far the SP and FP have been offset
     from the start of the stack frame (as defined by the previous frame's
     stack pointer).
     During epilogue analysis, keep how far the SP has been offset from the
     current stack pointer.  */
  CORE_ADDR sp_offset;
  CORE_ADDR fp_offset;

  /* The address of the first instruction in this function.  */
  CORE_ADDR pc;

  /* Saved registers.  */
  CORE_ADDR saved_regs[NDS32_NUM_SAVED_REGS];
};

/* Allocate and initialize a frame cache.  */

static struct nds32_frame_cache *
nds32_alloc_frame_cache (void)
{
  struct nds32_frame_cache *cache;
  int i;

  cache = FRAME_OBSTACK_ZALLOC (struct nds32_frame_cache);

  /* Initialize fp_offset to check if FP is set in prologue.  */
  cache->fp_offset = INVALID_OFFSET;

  /* Saved registers.  We initialize these to -1 since zero is a valid
     offset.  */
  for (i = 0; i < NDS32_NUM_SAVED_REGS; i++)
    cache->saved_regs[i] = REG_UNAVAIL;

  return cache;
}

/* Helper function for instructions used to push multiple words.  */

static void
nds32_push_multiple_words (struct nds32_frame_cache *cache, int rb, int re,
			   int enable4)
{
  CORE_ADDR sp_offset = cache->sp_offset;
  int i;

  /* Check LP, GP, FP in enable4.  */
  for (i = 1; i <= 3; i++)
    {
      if ((enable4 >> i) & 0x1)
	{
	  sp_offset += 4;
	  cache->saved_regs[NDS32_SP_REGNUM - i] = sp_offset;
	}
    }

  /* Skip case where re == rb == sp.  */
  if ((rb < REG_FP) && (re < REG_FP))
    {
      for (i = re; i >= rb; i--)
	{
	  sp_offset += 4;
	  cache->saved_regs[i] = sp_offset;
	}
    }

  /* For sp, update the offset.  */
  cache->sp_offset = sp_offset;
}

/* Analyze the instructions within the given address range.  If CACHE
   is non-NULL, fill it in.  Return the first address beyond the given
   address range.  If CACHE is NULL, return the first address not
   recognized as a prologue instruction.  */

static CORE_ADDR
nds32_analyze_prologue (struct gdbarch *gdbarch, CORE_ADDR pc,
			CORE_ADDR limit_pc, struct nds32_frame_cache *cache)
{
  nds32_gdbarch_tdep *tdep = gdbarch_tdep<nds32_gdbarch_tdep> (gdbarch);
  int abi_use_fpr = nds32_abi_use_fpr (tdep->elf_abi);
  /* Current scanning status.  */
  int in_prologue_bb = 0;
  int val_ta = 0;
  uint32_t insn, insn_len;

  for (; pc < limit_pc; pc += insn_len)
    {
      insn = read_memory_unsigned_integer (pc, 4, BFD_ENDIAN_BIG);

      if ((insn & 0x80000000) == 0)
	{
	  /* 32-bit instruction */
	  insn_len = 4;

	  if (CHOP_BITS (insn, 15) == N32_TYPE2 (ADDI, REG_SP, REG_SP, 0))
	    {
	      /* addi $sp, $sp, imm15s */
	      int imm15s = N32_IMM15S (insn);

	      if (imm15s < 0)
		{
		  if (cache != NULL)
		    cache->sp_offset += -imm15s;

		  in_prologue_bb = 1;
		  continue;
		}
	    }
	  else if (CHOP_BITS (insn, 15) == N32_TYPE2 (ADDI, REG_FP, REG_SP, 0))
	    {
	      /* addi $fp, $sp, imm15s */
	      int imm15s = N32_IMM15S (insn);

	      if (imm15s > 0)
		{
		  if (cache != NULL)
		    cache->fp_offset = cache->sp_offset - imm15s;

		  in_prologue_bb = 1;
		  continue;
		}
	    }
	  else if ((insn & ~(__MASK (19) << 6)) == N32_SMW_ADM
		   && N32_RA5 (insn) == REG_SP)
	    {
	      /* smw.adm Rb, [$sp], Re, enable4 */
	      if (cache != NULL)
		nds32_push_multiple_words (cache, N32_RT5 (insn),
					   N32_RB5 (insn),
					   N32_LSMW_ENABLE4 (insn));
	      in_prologue_bb = 1;
	      continue;
	    }
	  else if (insn == N32_ALU1 (ADD, REG_SP, REG_SP, REG_TA)
		   || insn == N32_ALU1 (ADD, REG_SP, REG_TA, REG_SP))
	    {
	      /* add $sp, $sp, $ta */
	      /* add $sp, $ta, $sp */
	      if (val_ta < 0)
		{
		  if (cache != NULL)
		    cache->sp_offset += -val_ta;

		  in_prologue_bb = 1;
		  continue;
		}
	    }
	  else if (CHOP_BITS (insn, 20) == N32_TYPE1 (MOVI, REG_TA, 0))
	    {
	      /* movi $ta, imm20s */
	      if (cache != NULL)
		val_ta = N32_IMM20S (insn);

	      continue;
	    }
	  else if (CHOP_BITS (insn, 20) == N32_TYPE1 (SETHI, REG_TA, 0))
	    {
	      /* sethi $ta, imm20u */
	      if (cache != NULL)
		val_ta = N32_IMM20U (insn) << 12;

	      continue;
	    }
	  else if (CHOP_BITS (insn, 15) == N32_TYPE2 (ORI, REG_TA, REG_TA, 0))
	    {
	      /* ori $ta, $ta, imm15u */
	      if (cache != NULL)
		val_ta |= N32_IMM15U (insn);

	      continue;
	    }
	  else if (CHOP_BITS (insn, 15) == N32_TYPE2 (ADDI, REG_TA, REG_TA, 0))
	    {
	      /* addi $ta, $ta, imm15s */
	      if (cache != NULL)
		val_ta += N32_IMM15S (insn);

	      continue;
	    }
	  if (insn == N32_ALU1 (ADD, REG_GP, REG_TA, REG_GP)
	      || insn == N32_ALU1 (ADD, REG_GP, REG_GP, REG_TA))
	    {
	      /* add $gp, $ta, $gp */
	      /* add $gp, $gp, $ta */
	      in_prologue_bb = 1;
	      continue;
	    }
	  else if (CHOP_BITS (insn, 20) == N32_TYPE1 (MOVI, REG_GP, 0))
	    {
	      /* movi $gp, imm20s */
	      in_prologue_bb = 1;
	      continue;
	    }
	  else if (CHOP_BITS (insn, 20) == N32_TYPE1 (SETHI, REG_GP, 0))
	    {
	      /* sethi $gp, imm20u */
	      in_prologue_bb = 1;
	      continue;
	    }
	  else if (CHOP_BITS (insn, 15) == N32_TYPE2 (ORI, REG_GP, REG_GP, 0))
	    {
	      /* ori $gp, $gp, imm15u */
	      in_prologue_bb = 1;
	      continue;
	    }
	  else
	    {
	      /* Jump/Branch insns never appear in prologue basic block.
		 The loop can be escaped early when these insns are met.  */
	      if (in_prologue_bb == 1)
		{
		  int op = N32_OP6 (insn);

		  if (op == N32_OP6_JI
		      || op == N32_OP6_JREG
		      || op == N32_OP6_BR1
		      || op == N32_OP6_BR2
		      || op == N32_OP6_BR3)
		    break;
		}
	    }

	  if (abi_use_fpr && N32_OP6 (insn) == N32_OP6_SDC
	      && __GF (insn, 12, 3) == 0)
	    {
	      /* For FPU insns, CP (bit [13:14]) should be CP0,  and only
		 normal form (bit [12] == 0) is used.  */

	      /* fsdi FDt, [$sp + (imm12s << 2)] */
	      if (N32_RA5 (insn) == REG_SP)
		continue;
	    }

	  /* The optimizer might shove anything into the prologue, if
	     we build up cache (cache != NULL) from analyzing prologue,
	     we just skip what we don't recognize and analyze further to
	     make cache as complete as possible.  However, if we skip
	     prologue, we'll stop immediately on unrecognized
	     instruction.  */
	  if (cache == NULL)
	    break;
	}
      else
	{
	  /* 16-bit instruction */
	  insn_len = 2;

	  insn >>= 16;

	  if (CHOP_BITS (insn, 10) == N16_TYPE10 (ADDI10S, 0))
	    {
	      /* addi10s.sp */
	      int imm10s = N16_IMM10S (insn);

	      if (imm10s < 0)
		{
		  if (cache != NULL)
		    cache->sp_offset += -imm10s;

		  in_prologue_bb = 1;
		  continue;
		}
	    }
	  else if (__GF (insn, 7, 8) == N16_T25_PUSH25)
	    {
	      /* push25 */
	      if (cache != NULL)
		{
		  int imm8u = (insn & 0x1f) << 3;
		  int re = (insn >> 5) & 0x3;
		  const int reg_map[] = { 6, 8, 10, 14 };

		  /* Operation 1 -- smw.adm R6, [$sp], Re, #0xe */
		  nds32_push_multiple_words (cache, 6, reg_map[re], 0xe);

		  /* Operation 2 -- sp = sp - (imm5u << 3) */
		  cache->sp_offset += imm8u;
		}

	      in_prologue_bb = 1;
	      continue;
	    }
	  else if (insn == N16_TYPE5 (ADD5PC, REG_GP))
	    {
	      /* add5.pc $gp */
	      in_prologue_bb = 1;
	      continue;
	    }
	  else if (CHOP_BITS (insn, 5) == N16_TYPE55 (MOVI55, REG_GP, 0))
	    {
	      /* movi55 $gp, imm5s */
	      in_prologue_bb = 1;
	      continue;
	    }
	  else
	    {
	      /* Jump/Branch insns never appear in prologue basic block.
		 The loop can be escaped early when these insns are met.  */
	      if (in_prologue_bb == 1)
		{
		  uint32_t insn5 = CHOP_BITS (insn, 5);
		  uint32_t insn8 = CHOP_BITS (insn, 8);
		  uint32_t insn38 = CHOP_BITS (insn, 11);

		  if (insn5 == N16_TYPE5 (JR5, 0)
		      || insn5 == N16_TYPE5 (JRAL5, 0)
		      || insn5 == N16_TYPE5 (RET5, 0)
		      || insn8 == N16_TYPE8 (J8, 0)
		      || insn8 == N16_TYPE8 (BEQZS8, 0)
		      || insn8 == N16_TYPE8 (BNEZS8, 0)
		      || insn38 == N16_TYPE38 (BEQZ38, 0, 0)
		      || insn38 == N16_TYPE38 (BNEZ38, 0, 0)
		      || insn38 == N16_TYPE38 (BEQS38, 0, 0)
		      || insn38 == N16_TYPE38 (BNES38, 0, 0))
		    break;
		}
	    }

	  /* The optimizer might shove anything into the prologue, if
	     we build up cache (cache != NULL) from analyzing prologue,
	     we just skip what we don't recognize and analyze further to
	     make cache as complete as possible.  However, if we skip
	     prologue, we'll stop immediately on unrecognized
	     instruction.  */
	  if (cache == NULL)
	    break;
	}
    }

  return pc;
}

/* Implement the "skip_prologue" gdbarch method.

   Find the end of function prologue.  */

static CORE_ADDR
nds32_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr, limit_pc;

  /* See if we can determine the end of the prologue via the symbol table.
     If so, then return either PC, or the PC after the prologue, whichever
     is greater.  */
  if (find_pc_partial_function (pc, NULL, &func_addr, NULL))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);
      if (post_prologue_pc != 0)
	return std::max (pc, post_prologue_pc);
    }

  /* Can't determine prologue from the symbol table, need to examine
     instructions.  */

  /* Find an upper limit on the function prologue using the debug
     information.  If the debug information could not be used to provide
     that bound, then use an arbitrary large number as the upper bound.  */
  limit_pc = skip_prologue_using_sal (gdbarch, pc);
  if (limit_pc == 0)
    limit_pc = pc + 128;	/* Magic.  */

  /* Find the end of prologue.  */
  return nds32_analyze_prologue (gdbarch, pc, limit_pc, NULL);
}

/* Allocate and fill in *THIS_CACHE with information about the prologue of
   *THIS_FRAME.  Do not do this if *THIS_CACHE was already allocated.  Return
   a pointer to the current nds32_frame_cache in *THIS_CACHE.  */

static struct nds32_frame_cache *
nds32_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct nds32_frame_cache *cache;
  CORE_ADDR current_pc;
  ULONGEST prev_sp;
  ULONGEST this_base;
  int i;

  if (*this_cache)
    return (struct nds32_frame_cache *) *this_cache;

  cache = nds32_alloc_frame_cache ();
  *this_cache = cache;

  cache->pc = get_frame_func (this_frame);
  current_pc = get_frame_pc (this_frame);
  nds32_analyze_prologue (gdbarch, cache->pc, current_pc, cache);

  /* Compute the previous frame's stack pointer (which is also the
     frame's ID's stack address), and this frame's base pointer.  */
  if (cache->fp_offset != INVALID_OFFSET)
    {
      /* FP is set in prologue, so it can be used to calculate other info.  */
      this_base = get_frame_register_unsigned (this_frame, NDS32_FP_REGNUM);
      prev_sp = this_base + cache->fp_offset;
    }
  else
    {
      this_base = get_frame_register_unsigned (this_frame, NDS32_SP_REGNUM);
      prev_sp = this_base + cache->sp_offset;
    }

  cache->prev_sp = prev_sp;
  cache->base = this_base;

  /* Adjust all the saved registers such that they contain addresses
     instead of offsets.  */
  for (i = 0; i < NDS32_NUM_SAVED_REGS; i++)
    if (cache->saved_regs[i] != REG_UNAVAIL)
      cache->saved_regs[i] = cache->prev_sp - cache->saved_regs[i];

  return cache;
}

/* Implement the "this_id" frame_unwind method.

   Our frame ID for a normal frame is the current function's starting
   PC and the caller's SP when we were called.  */

static void
nds32_frame_this_id (frame_info_ptr this_frame,
		     void **this_cache, struct frame_id *this_id)
{
  struct nds32_frame_cache *cache = nds32_frame_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->prev_sp == 0)
    return;

  *this_id = frame_id_build (cache->prev_sp, cache->pc);
}

/* Implement the "prev_register" frame_unwind method.  */

static struct value *
nds32_frame_prev_register (frame_info_ptr this_frame, void **this_cache,
			   int regnum)
{
  struct nds32_frame_cache *cache = nds32_frame_cache (this_frame, this_cache);

  if (regnum == NDS32_SP_REGNUM)
    return frame_unwind_got_constant (this_frame, regnum, cache->prev_sp);

  /* The PC of the previous frame is stored in the LP register of
     the current frame.  */
  if (regnum == NDS32_PC_REGNUM)
    regnum = NDS32_LP_REGNUM;

  if (regnum < NDS32_NUM_SAVED_REGS && cache->saved_regs[regnum] != REG_UNAVAIL)
    return frame_unwind_got_memory (this_frame, regnum,
				    cache->saved_regs[regnum]);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind nds32_frame_unwind =
{
  "nds32 prologue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  nds32_frame_this_id,
  nds32_frame_prev_register,
  NULL,
  default_frame_sniffer,
};

/* Return the frame base address of *THIS_FRAME.  */

static CORE_ADDR
nds32_frame_base_address (frame_info_ptr this_frame, void **this_cache)
{
  struct nds32_frame_cache *cache = nds32_frame_cache (this_frame, this_cache);

  return cache->base;
}

static const struct frame_base nds32_frame_base =
{
  &nds32_frame_unwind,
  nds32_frame_base_address,
  nds32_frame_base_address,
  nds32_frame_base_address
};

/* Helper function for instructions used to pop multiple words.  */

static void
nds32_pop_multiple_words (struct nds32_frame_cache *cache, int rb, int re,
			  int enable4)
{
  CORE_ADDR sp_offset = cache->sp_offset;
  int i;

  /* Skip case where re == rb == sp.  */
  if ((rb < REG_FP) && (re < REG_FP))
    {
      for (i = rb; i <= re; i++)
	{
	  cache->saved_regs[i] = sp_offset;
	  sp_offset += 4;
	}
    }

  /* Check FP, GP, LP in enable4.  */
  for (i = 3; i >= 1; i--)
    {
      if ((enable4 >> i) & 0x1)
	{
	  cache->saved_regs[NDS32_SP_REGNUM - i] = sp_offset;
	  sp_offset += 4;
	}
    }

  /* For sp, update the offset.  */
  cache->sp_offset = sp_offset;
}

/* The instruction sequences in NDS32 epilogue are

   INSN_RESET_SP  (optional)
		  (If exists, this must be the first instruction in epilogue
		   and the stack has not been destroyed.).
   INSN_RECOVER  (optional).
   INSN_RETURN/INSN_RECOVER_RETURN  (required).  */

/* Helper function for analyzing the given 32-bit INSN.  If CACHE is non-NULL,
   the necessary information will be recorded.  */

static inline int
nds32_analyze_epilogue_insn32 (int abi_use_fpr, uint32_t insn,
			       struct nds32_frame_cache *cache)
{
  if (CHOP_BITS (insn, 15) == N32_TYPE2 (ADDI, REG_SP, REG_SP, 0)
      && N32_IMM15S (insn) > 0)
    /* addi $sp, $sp, imm15s */
    return INSN_RESET_SP;
  else if (CHOP_BITS (insn, 15) == N32_TYPE2 (ADDI, REG_SP, REG_FP, 0)
	   && N32_IMM15S (insn) < 0)
    /* addi $sp, $fp, imm15s */
    return INSN_RESET_SP;
  else if ((insn & ~(__MASK (19) << 6)) == N32_LMW_BIM
	   && N32_RA5 (insn) == REG_SP)
    {
      /* lmw.bim Rb, [$sp], Re, enable4 */
      if (cache != NULL)
	nds32_pop_multiple_words (cache, N32_RT5 (insn),
				  N32_RB5 (insn), N32_LSMW_ENABLE4 (insn));

      return INSN_RECOVER;
    }
  else if (insn == N32_JREG (JR, 0, REG_LP, 0, 1))
    /* ret $lp */
    return INSN_RETURN;
  else if (insn == N32_ALU1 (ADD, REG_SP, REG_SP, REG_TA)
	   || insn == N32_ALU1 (ADD, REG_SP, REG_TA, REG_SP))
    /* add $sp, $sp, $ta */
    /* add $sp, $ta, $sp */
    return INSN_RESET_SP;
  else if (abi_use_fpr
	   && (insn & ~(__MASK (5) << 20 | __MASK (13))) == N32_FLDI_SP)
    {
      if (__GF (insn, 12, 1) == 0)
	/* fldi FDt, [$sp + (imm12s << 2)] */
	return INSN_RECOVER;
      else
	{
	  /* fldi.bi FDt, [$sp], (imm12s << 2) */
	  int offset = N32_IMM12S (insn) << 2;

	  if (offset == 8 || offset == 12)
	    {
	      if (cache != NULL)
		cache->sp_offset += offset;

	      return INSN_RECOVER;
	    }
	}
    }

  return INSN_NORMAL;
}

/* Helper function for analyzing the given 16-bit INSN.  If CACHE is non-NULL,
   the necessary information will be recorded.  */

static inline int
nds32_analyze_epilogue_insn16 (uint32_t insn, struct nds32_frame_cache *cache)
{
  if (insn == N16_TYPE5 (RET5, REG_LP))
    /* ret5 $lp */
    return INSN_RETURN;
  else if (CHOP_BITS (insn, 10) == N16_TYPE10 (ADDI10S, 0))
    {
      /* addi10s.sp */
      int imm10s = N16_IMM10S (insn);

      if (imm10s > 0)
	{
	  if (cache != NULL)
	    cache->sp_offset += imm10s;

	  return INSN_RECOVER;
	}
    }
  else if (__GF (insn, 7, 8) == N16_T25_POP25)
    {
      /* pop25 */
      if (cache != NULL)
	{
	  int imm8u = (insn & 0x1f) << 3;
	  int re = (insn >> 5) & 0x3;
	  const int reg_map[] = { 6, 8, 10, 14 };

	  /* Operation 1 -- sp = sp + (imm5u << 3) */
	  cache->sp_offset += imm8u;

	  /* Operation 2 -- lmw.bim R6, [$sp], Re, #0xe */
	  nds32_pop_multiple_words (cache, 6, reg_map[re], 0xe);
	}

      /* Operation 3 -- ret $lp */
      return INSN_RECOVER_RETURN;
    }

  return INSN_NORMAL;
}

/* Analyze a reasonable amount of instructions from the given PC to find
   the instruction used to return to the caller.  Return 1 if the 'return'
   instruction could be found, 0 otherwise.

   If CACHE is non-NULL, fill it in.  */

static int
nds32_analyze_epilogue (struct gdbarch *gdbarch, CORE_ADDR pc,
			struct nds32_frame_cache *cache)
{
  nds32_gdbarch_tdep *tdep = gdbarch_tdep<nds32_gdbarch_tdep> (gdbarch);
  int abi_use_fpr = nds32_abi_use_fpr (tdep->elf_abi);
  CORE_ADDR limit_pc;
  uint32_t insn, insn_len;
  int insn_type = INSN_NORMAL;

  if (abi_use_fpr)
    limit_pc = pc + 48;
  else
    limit_pc = pc + 16;

  for (; pc < limit_pc; pc += insn_len)
    {
      insn = read_memory_unsigned_integer (pc, 4, BFD_ENDIAN_BIG);

      if ((insn & 0x80000000) == 0)
	{
	  /* 32-bit instruction */
	  insn_len = 4;

	  insn_type = nds32_analyze_epilogue_insn32 (abi_use_fpr, insn, cache);
	  if (insn_type == INSN_RETURN)
	    return 1;
	  else if (insn_type == INSN_RECOVER)
	    continue;
	}
      else
	{
	  /* 16-bit instruction */
	  insn_len = 2;

	  insn >>= 16;
	  insn_type = nds32_analyze_epilogue_insn16 (insn, cache);
	  if (insn_type == INSN_RETURN || insn_type == INSN_RECOVER_RETURN)
	    return 1;
	  else if (insn_type == INSN_RECOVER)
	    continue;
	}

      /* Stop the scan if this is an unexpected instruction.  */
      break;
    }

  return 0;
}

/* Implement the "stack_frame_destroyed_p" gdbarch method.  */

static int
nds32_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  nds32_gdbarch_tdep *tdep = gdbarch_tdep<nds32_gdbarch_tdep> (gdbarch);
  int abi_use_fpr = nds32_abi_use_fpr (tdep->elf_abi);
  int insn_type = INSN_NORMAL;
  int ret_found = 0;
  uint32_t insn;

  insn = read_memory_unsigned_integer (addr, 4, BFD_ENDIAN_BIG);

  if ((insn & 0x80000000) == 0)
    {
      /* 32-bit instruction */

      insn_type = nds32_analyze_epilogue_insn32 (abi_use_fpr, insn, NULL);
    }
  else
    {
      /* 16-bit instruction */

      insn >>= 16;
      insn_type = nds32_analyze_epilogue_insn16 (insn, NULL);
    }

  if (insn_type == INSN_NORMAL || insn_type == INSN_RESET_SP)
    return 0;

  /* Search the required 'return' instruction within the following reasonable
     instructions.  */
  ret_found = nds32_analyze_epilogue (gdbarch, addr, NULL);
  if (ret_found == 0)
    return 0;

  /* Scan backwards to make sure that the last instruction has adjusted
     stack.  Both a 16-bit and a 32-bit instruction will be tried.  This is
     just a heuristic, so the false positives will be acceptable.  */
  insn = read_memory_unsigned_integer (addr - 2, 4, BFD_ENDIAN_BIG);

  /* Only 16-bit instructions are possible at addr - 2.  */
  if ((insn & 0x80000000) != 0)
    {
      /* This may be a 16-bit instruction or part of a 32-bit instruction.  */

      insn_type = nds32_analyze_epilogue_insn16 (insn >> 16, NULL);
      if (insn_type == INSN_RECOVER)
	return 1;
    }

  insn = read_memory_unsigned_integer (addr - 4, 4, BFD_ENDIAN_BIG);

  /* If this is a 16-bit instruction at addr - 4, then there must be another
     16-bit instruction at addr - 2, so only 32-bit instructions need to
     be analyzed here.  */
  if ((insn & 0x80000000) == 0)
    {
      /* This may be a 32-bit instruction or part of a 32-bit instruction.  */

      insn_type = nds32_analyze_epilogue_insn32 (abi_use_fpr, insn, NULL);
      if (insn_type == INSN_RECOVER || insn_type == INSN_RESET_SP)
	return 1;
    }

  return 0;
}

/* Implement the "sniffer" frame_unwind method.  */

static int
nds32_epilogue_frame_sniffer (const struct frame_unwind *self,
			      frame_info_ptr this_frame, void **this_cache)
{
  if (frame_relative_level (this_frame) == 0)
    return nds32_stack_frame_destroyed_p (get_frame_arch (this_frame),
					  get_frame_pc (this_frame));
  else
    return 0;
}

/* Allocate and fill in *THIS_CACHE with information needed to unwind
   *THIS_FRAME within epilogue.  Do not do this if *THIS_CACHE was already
   allocated.  Return a pointer to the current nds32_frame_cache in
   *THIS_CACHE.  */

static struct nds32_frame_cache *
nds32_epilogue_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct nds32_frame_cache *cache;
  CORE_ADDR current_pc, current_sp;
  int i;

  if (*this_cache)
    return (struct nds32_frame_cache *) *this_cache;

  cache = nds32_alloc_frame_cache ();
  *this_cache = cache;

  cache->pc = get_frame_func (this_frame);
  current_pc = get_frame_pc (this_frame);
  nds32_analyze_epilogue (gdbarch, current_pc, cache);

  current_sp = get_frame_register_unsigned (this_frame, NDS32_SP_REGNUM);
  cache->prev_sp = current_sp + cache->sp_offset;

  /* Adjust all the saved registers such that they contain addresses
     instead of offsets.  */
  for (i = 0; i < NDS32_NUM_SAVED_REGS; i++)
    if (cache->saved_regs[i] != REG_UNAVAIL)
      cache->saved_regs[i] = current_sp + cache->saved_regs[i];

  return cache;
}

/* Implement the "this_id" frame_unwind method.  */

static void
nds32_epilogue_frame_this_id (frame_info_ptr this_frame,
			      void **this_cache, struct frame_id *this_id)
{
  struct nds32_frame_cache *cache
    = nds32_epilogue_frame_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->prev_sp == 0)
    return;

  *this_id = frame_id_build (cache->prev_sp, cache->pc);
}

/* Implement the "prev_register" frame_unwind method.  */

static struct value *
nds32_epilogue_frame_prev_register (frame_info_ptr this_frame,
				    void **this_cache, int regnum)
{
  struct nds32_frame_cache *cache
    = nds32_epilogue_frame_cache (this_frame, this_cache);

  if (regnum == NDS32_SP_REGNUM)
    return frame_unwind_got_constant (this_frame, regnum, cache->prev_sp);

  /* The PC of the previous frame is stored in the LP register of
     the current frame.  */
  if (regnum == NDS32_PC_REGNUM)
    regnum = NDS32_LP_REGNUM;

  if (regnum < NDS32_NUM_SAVED_REGS && cache->saved_regs[regnum] != REG_UNAVAIL)
    return frame_unwind_got_memory (this_frame, regnum,
				    cache->saved_regs[regnum]);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

static const struct frame_unwind nds32_epilogue_frame_unwind =
{
  "nds32 epilogue",
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  nds32_epilogue_frame_this_id,
  nds32_epilogue_frame_prev_register,
  NULL,
  nds32_epilogue_frame_sniffer
};


/* Floating type and struct type that has only one floating type member
   can pass value using FPU registers (when FPU ABI is used).  */

static int
nds32_check_calling_use_fpr (struct type *type)
{
  struct type *t;
  enum type_code typecode;

  t = type;
  while (1)
    {
      t = check_typedef (t);
      typecode = t->code ();
      if (typecode != TYPE_CODE_STRUCT)
	break;
      else if (t->num_fields () != 1)
	return 0;
      else
	t = t->field (0).type ();
    }

  return typecode == TYPE_CODE_FLT;
}

/* Implement the "push_dummy_call" gdbarch method.  */

static CORE_ADDR
nds32_push_dummy_call (struct gdbarch *gdbarch, struct value *function,
		       struct regcache *regcache, CORE_ADDR bp_addr,
		       int nargs, struct value **args, CORE_ADDR sp,
		       function_call_return_method return_method,
		       CORE_ADDR struct_addr)
{
  const int REND = 6;		/* End for register offset.  */
  int goff = 0;			/* Current gpr offset for argument.  */
  int foff = 0;			/* Current fpr offset for argument.  */
  int soff = 0;			/* Current stack offset for argument.  */
  int i;
  ULONGEST regval;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  nds32_gdbarch_tdep *tdep = gdbarch_tdep<nds32_gdbarch_tdep> (gdbarch);
  struct type *func_type = function->type ();
  int abi_use_fpr = nds32_abi_use_fpr (tdep->elf_abi);
  int abi_split = nds32_abi_split (tdep->elf_abi);

  /* Set the return address.  For the NDS32, the return breakpoint is
     always at BP_ADDR.  */
  regcache_cooked_write_unsigned (regcache, NDS32_LP_REGNUM, bp_addr);

  /* If STRUCT_RETURN is true, then the struct return address (in
     STRUCT_ADDR) will consume the first argument-passing register.
     Both adjust the register count and store that value.  */
  if (return_method == return_method_struct)
    {
      regcache_cooked_write_unsigned (regcache, NDS32_R0_REGNUM, struct_addr);
      goff++;
    }

  /* Now make sure there's space on the stack */
  for (i = 0; i < nargs; i++)
    {
      struct type *type = args[i]->type ();
      int align = type_align (type);

      /* If align is zero, it may be an empty struct.
	 Just ignore the argument of empty struct.  */
      if (align == 0)
	continue;

      sp -= type->length ();
      sp = align_down (sp, align);
    }

  /* Stack must be 8-byte aligned.  */
  sp = align_down (sp, 8);

  soff = 0;
  for (i = 0; i < nargs; i++)
    {
      const gdb_byte *val;
      int align, len;
      struct type *type;
      int calling_use_fpr;
      int use_fpr = 0;

      type = args[i]->type ();
      calling_use_fpr = nds32_check_calling_use_fpr (type);
      len = type->length ();
      align = type_align (type);
      val = args[i]->contents ().data ();

      /* The size of a composite type larger than 4 bytes will be rounded
	 up to the nearest multiple of 4.  */
      if (len > 4)
	len = align_up (len, 4);

      /* Variadic functions are handled differently between AABI and ABI2FP+.

	 For AABI, the caller pushes arguments in registers, callee stores
	 unnamed arguments in stack, and then va_arg fetch arguments in stack.
	 Therefore, we don't have to handle variadic functions specially.

	 For ABI2FP+, the caller pushes only named arguments in registers
	 and pushes all unnamed arguments in stack.  */

      if (abi_use_fpr && func_type->has_varargs ()
	  && i >= func_type->num_fields ())
	goto use_stack;

      /* Try to use FPRs to pass arguments only when
	 1. The program is built using toolchain with FPU support.
	 2. The type of this argument can use FPR to pass value.  */
      use_fpr = abi_use_fpr && calling_use_fpr;

      if (use_fpr)
	{
	  if (tdep->fpu_freg == -1)
	    goto error_no_fpr;

	  /* Adjust alignment.  */
	  if ((align >> 2) > 0)
	    foff = align_up (foff, align >> 2);

	  if (foff < REND)
	    {
	      switch (len)
		{
		case 4:
		  regcache->cooked_write (tdep->fs0_regnum + foff, val);
		  foff++;
		  break;
		case 8:
		  regcache->cooked_write (NDS32_FD0_REGNUM + (foff >> 1), val);
		  foff += 2;
		  break;
		default:
		  /* Long double?  */
		  internal_error ("Do not know how to handle %d-byte double.\n",
				  len);
		  break;
		}
	      continue;
	    }
	}
      else
	{
	  /*
	     When passing arguments using GPRs,

	     * A composite type not larger than 4 bytes is passed in $rN.
	       The format is as if the value is loaded with load instruction
	       of corresponding size (e.g., LB, LH, LW).

	       For example,

		       r0
		       31      0
	       LITTLE: [x x b a]
		  BIG: [x x a b]

	     * Otherwise, a composite type is passed in consecutive registers.
	       The size is rounded up to the nearest multiple of 4.
	       The successive registers hold the parts of the argument as if
	       were loaded using lmw instructions.

	       For example,

		       r0	 r1
		       31      0 31      0
	       LITTLE: [d c b a] [x x x e]
		  BIG: [a b c d] [e x x x]
	   */

	  /* Adjust alignment.  */
	  if ((align >> 2) > 0)
	    goff = align_up (goff, align >> 2);

	  if (len <= (REND - goff) * 4)
	    {
	      /* This argument can be passed wholly via GPRs.  */
	      while (len > 0)
		{
		  regval = extract_unsigned_integer (val, (len > 4) ? 4 : len,
						     byte_order);
		  regcache_cooked_write_unsigned (regcache,
						  NDS32_R0_REGNUM + goff,
						  regval);
		  len -= 4;
		  val += 4;
		  goff++;
		}
	      continue;
	    }
	  else if (abi_split)
	    {
	      /* Some parts of this argument can be passed via GPRs.  */
	      while (goff < REND)
		{
		  regval = extract_unsigned_integer (val, (len > 4) ? 4 : len,
						     byte_order);
		  regcache_cooked_write_unsigned (regcache,
						  NDS32_R0_REGNUM + goff,
						  regval);
		  len -= 4;
		  val += 4;
		  goff++;
		}
	    }
	}

use_stack:
      /*
	 When pushing (split parts of) an argument into stack,

	 * A composite type not larger than 4 bytes is copied to different
	   base address.
	   In little-endian, the first byte of this argument is aligned
	   at the low address of the next free word.
	   In big-endian, the last byte of this argument is aligned
	   at the high address of the next free word.

	   For example,

	   sp [ - ]  [ c ] hi
	      [ c ]  [ b ]
	      [ b ]  [ a ]
	      [ a ]  [ - ] lo
	     LITTLE   BIG
       */

      /* Adjust alignment.  */
      soff = align_up (soff, align);

      while (len > 0)
	{
	  int rlen = (len > 4) ? 4 : len;

	  if (byte_order == BFD_ENDIAN_BIG)
	    write_memory (sp + soff + 4 - rlen, val, rlen);
	  else
	    write_memory (sp + soff, val, rlen);

	  len -= 4;
	  val += 4;
	  soff += 4;
	}
    }

  /* Finally, update the SP register.  */
  regcache_cooked_write_unsigned (regcache, NDS32_SP_REGNUM, sp);

  return sp;

error_no_fpr:
  /* If use_fpr, but no floating-point register exists,
     then it is an error.  */
  error (_("Fail to call. FPU registers are required."));
}

/* Read, for architecture GDBARCH, a function return value of TYPE
   from REGCACHE, and copy that into VALBUF.  */

static void
nds32_extract_return_value (struct gdbarch *gdbarch, struct type *type,
			    struct regcache *regcache, gdb_byte *valbuf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  nds32_gdbarch_tdep *tdep = gdbarch_tdep<nds32_gdbarch_tdep> (gdbarch);
  int abi_use_fpr = nds32_abi_use_fpr (tdep->elf_abi);
  int calling_use_fpr;
  int len;

  calling_use_fpr = nds32_check_calling_use_fpr (type);
  len = type->length ();

  if (abi_use_fpr && calling_use_fpr)
    {
      if (len == 4)
	regcache->cooked_read (tdep->fs0_regnum, valbuf);
      else if (len == 8)
	regcache->cooked_read (NDS32_FD0_REGNUM, valbuf);
      else
	internal_error (_("Cannot extract return value of %d bytes "
			  "long floating-point."), len);
    }
  else
    {
      /*
	 When returning result,

	 * A composite type not larger than 4 bytes is returned in $r0.
	   The format is as if the result is loaded with load instruction
	   of corresponding size (e.g., LB, LH, LW).

	   For example,

		   r0
		   31      0
	   LITTLE: [x x b a]
	      BIG: [x x a b]

	 * Otherwise, a composite type not larger than 8 bytes is returned
	   in $r0 and $r1.
	   In little-endian, the first word is loaded in $r0.
	   In big-endian, the last word is loaded in $r1.

	   For example,

		   r0	     r1
		   31      0 31      0
	   LITTLE: [d c b a] [x x x e]
	      BIG: [x x x a] [b c d e]
       */

      ULONGEST tmp;

      if (len < 4)
	{
	  /* By using store_unsigned_integer we avoid having to do
	     anything special for small big-endian values.  */
	  regcache_cooked_read_unsigned (regcache, NDS32_R0_REGNUM, &tmp);
	  store_unsigned_integer (valbuf, len, byte_order, tmp);
	}
      else if (len == 4)
	{
	  regcache->cooked_read (NDS32_R0_REGNUM, valbuf);
	}
      else if (len < 8)
	{
	  int len1, len2;

	  len1 = byte_order == BFD_ENDIAN_BIG ? len - 4 : 4;
	  len2 = len - len1;

	  regcache_cooked_read_unsigned (regcache, NDS32_R0_REGNUM, &tmp);
	  store_unsigned_integer (valbuf, len1, byte_order, tmp);

	  regcache_cooked_read_unsigned (regcache, NDS32_R0_REGNUM + 1, &tmp);
	  store_unsigned_integer (valbuf + len1, len2, byte_order, tmp);
	}
      else
	{
	  regcache->cooked_read (NDS32_R0_REGNUM, valbuf);
	  regcache->cooked_read (NDS32_R0_REGNUM + 1, valbuf + 4);
	}
    }
}

/* Write, for architecture GDBARCH, a function return value of TYPE
   from VALBUF into REGCACHE.  */

static void
nds32_store_return_value (struct gdbarch *gdbarch, struct type *type,
			  struct regcache *regcache, const gdb_byte *valbuf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  nds32_gdbarch_tdep *tdep = gdbarch_tdep<nds32_gdbarch_tdep> (gdbarch);
  int abi_use_fpr = nds32_abi_use_fpr (tdep->elf_abi);
  int calling_use_fpr;
  int len;

  calling_use_fpr = nds32_check_calling_use_fpr (type);
  len = type->length ();

  if (abi_use_fpr && calling_use_fpr)
    {
      if (len == 4)
	regcache->cooked_write (tdep->fs0_regnum, valbuf);
      else if (len == 8)
	regcache->cooked_write (NDS32_FD0_REGNUM, valbuf);
      else
	internal_error (_("Cannot store return value of %d bytes "
			  "long floating-point."), len);
    }
  else
    {
      ULONGEST regval;

      if (len < 4)
	{
	  regval = extract_unsigned_integer (valbuf, len, byte_order);
	  regcache_cooked_write_unsigned (regcache, NDS32_R0_REGNUM, regval);
	}
      else if (len == 4)
	{
	  regcache->cooked_write (NDS32_R0_REGNUM, valbuf);
	}
      else if (len < 8)
	{
	  int len1, len2;

	  len1 = byte_order == BFD_ENDIAN_BIG ? len - 4 : 4;
	  len2 = len - len1;

	  regval = extract_unsigned_integer (valbuf, len1, byte_order);
	  regcache_cooked_write_unsigned (regcache, NDS32_R0_REGNUM, regval);

	  regval = extract_unsigned_integer (valbuf + len1, len2, byte_order);
	  regcache_cooked_write_unsigned (regcache, NDS32_R0_REGNUM + 1,
					  regval);
	}
      else
	{
	  regcache->cooked_write (NDS32_R0_REGNUM, valbuf);
	  regcache->cooked_write (NDS32_R0_REGNUM + 1, valbuf + 4);
	}
    }
}

/* Implement the "return_value" gdbarch method.

   Determine, for architecture GDBARCH, how a return value of TYPE
   should be returned.  If it is supposed to be returned in registers,
   and READBUF is non-zero, read the appropriate value from REGCACHE,
   and copy it into READBUF.  If WRITEBUF is non-zero, write the value
   from WRITEBUF into REGCACHE.  */

static enum return_value_convention
nds32_return_value (struct gdbarch *gdbarch, struct value *func_type,
		    struct type *type, struct regcache *regcache,
		    gdb_byte *readbuf, const gdb_byte *writebuf)
{
  if (type->length () > 8)
    {
      return RETURN_VALUE_STRUCT_CONVENTION;
    }
  else
    {
      if (readbuf != NULL)
	nds32_extract_return_value (gdbarch, type, regcache, readbuf);
      if (writebuf != NULL)
	nds32_store_return_value (gdbarch, type, regcache, writebuf);

      return RETURN_VALUE_REGISTER_CONVENTION;
    }
}

/* Implement the "get_longjmp_target" gdbarch method.  */

static int
nds32_get_longjmp_target (frame_info_ptr frame, CORE_ADDR *pc)
{
  gdb_byte buf[4];
  CORE_ADDR jb_addr;
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  jb_addr = get_frame_register_unsigned (frame, NDS32_R0_REGNUM);

  if (target_read_memory (jb_addr + 11 * 4, buf, 4))
    return 0;

  *pc = extract_unsigned_integer (buf, 4, byte_order);
  return 1;
}

/* Validate the given TDESC, and fixed-number some registers in it.
   Return 0 if the given TDESC does not contain the required feature
   or not contain required registers.  */

static int
nds32_validate_tdesc_p (const struct target_desc *tdesc,
			struct tdesc_arch_data *tdesc_data,
			int *fpu_freg, int *use_pseudo_fsrs)
{
  const struct tdesc_feature *feature;
  int i, valid_p;

  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.nds32.core");
  if (feature == NULL)
    return 0;

  valid_p = 1;
  /* Validate and fixed-number R0-R10.  */
  for (i = NDS32_R0_REGNUM; i <= NDS32_R0_REGNUM + 10; i++)
    valid_p &= tdesc_numbered_register (feature, tdesc_data, i,
					nds32_register_names[i]);

  /* Validate R15.  */
  valid_p &= tdesc_unnumbered_register (feature,
					nds32_register_names[NDS32_TA_REGNUM]);

  /* Validate and fixed-number FP, GP, LP, SP, PC.  */
  for (i = NDS32_FP_REGNUM; i <= NDS32_PC_REGNUM; i++)
    valid_p &= tdesc_numbered_register (feature, tdesc_data, i,
					nds32_register_names[i]);

  if (!valid_p)
    return 0;

  /* Fixed-number R11-R27.  */
  for (i = NDS32_R0_REGNUM + 11; i <= NDS32_R0_REGNUM + 27; i++)
    tdesc_numbered_register (feature, tdesc_data, i, nds32_register_names[i]);

  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.nds32.fpu");
  if (feature != NULL)
    {
      int num_fdr_regs, num_fsr_regs, fs0_regnum, num_listed_fsr;
      int freg = -1;

      /* Guess FPU configuration via listed registers.  */
      if (tdesc_unnumbered_register (feature, "fd31"))
	freg = 3;
      else if (tdesc_unnumbered_register (feature, "fd15"))
	freg = 2;
      else if (tdesc_unnumbered_register (feature, "fd7"))
	freg = 1;
      else if (tdesc_unnumbered_register (feature, "fd3"))
	freg = 0;

      if (freg == -1)
	/* Required FDR is not found.  */
	return 0;
      else
	*fpu_freg = freg;

      /* Validate and fixed-number required FDRs.  */
      num_fdr_regs = num_fdr_map[freg];
      for (i = 0; i < num_fdr_regs; i++)
	valid_p &= tdesc_numbered_register (feature, tdesc_data,
					    NDS32_FD0_REGNUM + i,
					    nds32_fdr_register_names[i]);
      if (!valid_p)
	return 0;

      /* Count the number of listed FSRs, and fixed-number them if present.  */
      num_fsr_regs = num_fsr_map[freg];
      fs0_regnum = NDS32_FD0_REGNUM + num_fdr_regs;
      num_listed_fsr = 0;
      for (i = 0; i < num_fsr_regs; i++)
	num_listed_fsr += tdesc_numbered_register (feature, tdesc_data,
						   fs0_regnum + i,
						   nds32_fsr_register_names[i]);

      if (num_listed_fsr == 0)
	/* No required FSRs are listed explicitly,  make them pseudo registers
	   of FDRs.  */
	*use_pseudo_fsrs = 1;
      else if (num_listed_fsr == num_fsr_regs)
	/* All required FSRs are listed explicitly.  */
	*use_pseudo_fsrs = 0;
      else
	/* Some required FSRs are missing.  */
	return 0;
    }

  return 1;
}

/* Initialize the current architecture based on INFO.  If possible,
   re-use an architecture from ARCHES, which is a list of
   architectures already created during this debugging session.

   Called e.g. at program startup, when reading a core file, and when
   reading a binary file.  */

static struct gdbarch *
nds32_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch_list *best_arch;
  tdesc_arch_data_up tdesc_data;
  const struct target_desc *tdesc = info.target_desc;
  int elf_abi = E_NDS_ABI_AABI;
  int fpu_freg = -1;
  int use_pseudo_fsrs = 0;
  int i, num_regs, maxregs;

  /* Extract the elf_flags if available.  */
  if (info.abfd && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour)
    elf_abi = elf_elfheader (info.abfd)->e_flags & EF_NDS_ABI;

  /* If there is already a candidate, use it.  */
  for (best_arch = gdbarch_list_lookup_by_info (arches, &info);
       best_arch != NULL;
       best_arch = gdbarch_list_lookup_by_info (best_arch->next, &info))
    {
      nds32_gdbarch_tdep *idep
	= gdbarch_tdep<nds32_gdbarch_tdep> (best_arch->gdbarch);

      if (idep->elf_abi != elf_abi)
	continue;

      /* Found a match.  */
      break;
    }

  if (best_arch != NULL)
    return best_arch->gdbarch;

  if (!tdesc_has_registers (tdesc))
    tdesc = tdesc_nds32;

  tdesc_data = tdesc_data_alloc ();

  if (!nds32_validate_tdesc_p (tdesc, tdesc_data.get (), &fpu_freg,
			       &use_pseudo_fsrs))
    return NULL;

  /* Allocate space for the new architecture.  */
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new nds32_gdbarch_tdep));
  nds32_gdbarch_tdep *tdep = gdbarch_tdep<nds32_gdbarch_tdep> (gdbarch);

  tdep->fpu_freg = fpu_freg;
  tdep->use_pseudo_fsrs = use_pseudo_fsrs;
  tdep->fs0_regnum = -1;
  tdep->elf_abi = elf_abi;

  set_gdbarch_wchar_bit (gdbarch, 16);
  set_gdbarch_wchar_signed (gdbarch, 0);

  if (fpu_freg == -1)
    num_regs = NDS32_NUM_REGS;
  else if (use_pseudo_fsrs == 1)
    {
      set_gdbarch_pseudo_register_read (gdbarch, nds32_pseudo_register_read);
      set_gdbarch_deprecated_pseudo_register_write
	(gdbarch, nds32_pseudo_register_write);
      set_tdesc_pseudo_register_name (gdbarch, nds32_pseudo_register_name);
      set_tdesc_pseudo_register_type (gdbarch, nds32_pseudo_register_type);
      set_gdbarch_num_pseudo_regs (gdbarch, num_fsr_map[fpu_freg]);

      num_regs = NDS32_NUM_REGS + num_fdr_map[fpu_freg];
    }
  else
    num_regs = NDS32_NUM_REGS + num_fdr_map[fpu_freg] + num_fsr_map[fpu_freg];

  set_gdbarch_num_regs (gdbarch, num_regs);
  tdesc_use_registers (gdbarch, tdesc, std::move (tdesc_data));

  /* Cache the register number of fs0.  */
  if (fpu_freg != -1)
    tdep->fs0_regnum = user_reg_map_name_to_regnum (gdbarch, "fs0", -1);

  /* Add NDS32 register aliases.  To avoid search in user register name space,
     user_reg_map_name_to_regnum is not used.  */
  maxregs = gdbarch_num_cooked_regs (gdbarch);
  for (i = 0; i < ARRAY_SIZE (nds32_register_aliases); i++)
    {
      int regnum, j;

      regnum = -1;
      /* Search register name space.  */
      for (j = 0; j < maxregs; j++)
	{
	  const char *regname = gdbarch_register_name (gdbarch, j);

	  if (strcmp (regname, nds32_register_aliases[i].name) == 0)
	    {
	      regnum = j;
	      break;
	    }
	}

      /* Try next alias entry if the given name can not be found in register
	 name space.  */
      if (regnum == -1)
	continue;

      user_reg_add (gdbarch, nds32_register_aliases[i].alias,
		    value_of_nds32_reg, (const void *) (intptr_t) regnum);
    }

  nds32_add_reggroups (gdbarch);

  /* Hook in ABI-specific overrides, if they have been registered.  */
  info.tdesc_data = tdesc_data.get ();
  gdbarch_init_osabi (info, gdbarch);

  /* Override tdesc_register callbacks for system registers.  */
  set_gdbarch_register_reggroup_p (gdbarch, nds32_register_reggroup_p);

  set_gdbarch_sp_regnum (gdbarch, NDS32_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, NDS32_PC_REGNUM);
  set_gdbarch_stack_frame_destroyed_p (gdbarch, nds32_stack_frame_destroyed_p);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, nds32_dwarf2_reg_to_regnum);

  set_gdbarch_push_dummy_call (gdbarch, nds32_push_dummy_call);
  set_gdbarch_return_value (gdbarch, nds32_return_value);

  set_gdbarch_skip_prologue (gdbarch, nds32_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch,
				       nds32_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch,
				       nds32_breakpoint::bp_from_kind);

  set_gdbarch_frame_align (gdbarch, nds32_frame_align);
  frame_base_set_default (gdbarch, &nds32_frame_base);

  /* Handle longjmp.  */
  set_gdbarch_get_longjmp_target (gdbarch, nds32_get_longjmp_target);

  /* The order of appending is the order it check frame.  */
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &nds32_epilogue_frame_unwind);
  frame_unwind_append_unwinder (gdbarch, &nds32_frame_unwind);

  return gdbarch;
}

void _initialize_nds32_tdep ();
void
_initialize_nds32_tdep ()
{
  /* Initialize gdbarch.  */
  gdbarch_register (bfd_arch_nds32, nds32_gdbarch_init);

  initialize_tdesc_nds32 ();
  nds32_init_reggroups ();
}
