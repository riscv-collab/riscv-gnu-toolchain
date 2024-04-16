/* Target-dependent code for SPARC.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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

#ifndef SPARC_TDEP_H
#define SPARC_TDEP_H 1

#include "gdbarch.h"

#define SPARC_CORE_REGISTERS                      \
  "g0", "g1", "g2", "g3", "g4", "g5", "g6", "g7", \
  "o0", "o1", "o2", "o3", "o4", "o5", "sp", "o7", \
  "l0", "l1", "l2", "l3", "l4", "l5", "l6", "l7", \
  "i0", "i1", "i2", "i3", "i4", "i5", "fp", "i7"

class frame_info_ptr;
struct gdbarch;
struct regcache;
struct regset;
struct trad_frame_saved_reg;

/* Register offsets for the general-purpose register set.  */

struct sparc_gregmap
{
  int r_psr_offset;
  int r_pc_offset;
  int r_npc_offset;
  int r_y_offset;
  int r_wim_offset;
  int r_tbr_offset;
  int r_g1_offset;
  int r_l0_offset;
  int r_y_size;
};

struct sparc_fpregmap
{
  int r_f0_offset;
  int r_fsr_offset;
};

/* SPARC architecture-specific information.  */

struct sparc_gdbarch_tdep : gdbarch_tdep_base
{
  /* Register numbers for the PN and nPC registers.  The definitions
     for (64-bit) UltraSPARC differ from the (32-bit) SPARC
     definitions.  */
  int pc_regnum = 0;
  int npc_regnum = 0;

  /* Register names specific for architecture (sparc32 vs. sparc64) */
  const char * const *fpu_register_names = nullptr;
  size_t fpu_registers_num = 0;
  const char * const *cp0_register_names = nullptr;
  size_t cp0_registers_num = 0;

  /* Register sets.  */
  const struct regset *gregset = nullptr;
  size_t sizeof_gregset = 0;
  const struct regset *fpregset = nullptr;
  size_t sizeof_fpregset = 0;

  /* Offset of saved PC in jmp_buf.  */
  int jb_pc_offset = 0;

  /* Size of an Procedure Linkage Table (PLT) entry, 0 if we shouldn't
     treat the PLT special when doing prologue analysis.  */
  size_t plt_entry_size = 0;

  /* Alternative location for trap return.  Used for single-stepping.  */
  CORE_ADDR (*step_trap) (frame_info_ptr frame, unsigned long insn)
    = nullptr;

  /* ISA-specific data types.  */
  struct type *sparc_psr_type = nullptr;
  struct type *sparc_fsr_type = nullptr;
  struct type *sparc64_ccr_type = nullptr;
  struct type *sparc64_pstate_type = nullptr;
  struct type *sparc64_fsr_type = nullptr;
  struct type *sparc64_fprs_type = nullptr;
};

/* Register numbers of various important registers.  */

enum sparc_regnum
{
  SPARC_G0_REGNUM = 0,		/* %g0 */
  SPARC_G1_REGNUM,
  SPARC_G2_REGNUM,
  SPARC_G3_REGNUM,
  SPARC_G4_REGNUM,
  SPARC_G5_REGNUM,
  SPARC_G6_REGNUM,
  SPARC_G7_REGNUM,		/* %g7 */
  SPARC_O0_REGNUM,		/* %o0 */
  SPARC_O1_REGNUM,
  SPARC_O2_REGNUM,
  SPARC_O3_REGNUM,
  SPARC_O4_REGNUM,
  SPARC_O5_REGNUM,
  SPARC_SP_REGNUM,		/* %sp (%o6) */
  SPARC_O7_REGNUM,		/* %o7 */
  SPARC_L0_REGNUM,		/* %l0 */
  SPARC_L1_REGNUM,
  SPARC_L2_REGNUM,
  SPARC_L3_REGNUM,
  SPARC_L4_REGNUM,
  SPARC_L5_REGNUM,
  SPARC_L6_REGNUM,
  SPARC_L7_REGNUM,		/* %l7 */
  SPARC_I0_REGNUM,		/* %i0 */
  SPARC_I1_REGNUM,
  SPARC_I2_REGNUM,
  SPARC_I3_REGNUM,
  SPARC_I4_REGNUM,
  SPARC_I5_REGNUM,
  SPARC_FP_REGNUM,		/* %fp (%i6) */
  SPARC_I7_REGNUM,		/* %i7 */
  SPARC_F0_REGNUM,		/* %f0 */
  SPARC_F1_REGNUM,
  SPARC_F2_REGNUM,
  SPARC_F3_REGNUM,
  SPARC_F4_REGNUM,
  SPARC_F5_REGNUM,
  SPARC_F6_REGNUM,
  SPARC_F7_REGNUM,
  SPARC_F31_REGNUM		/* %f31 */
  = SPARC_F0_REGNUM + 31
};

enum sparc32_regnum
{
  SPARC32_Y_REGNUM		/* %y */
  = SPARC_F31_REGNUM + 1,
  SPARC32_PSR_REGNUM,		/* %psr */
  SPARC32_WIM_REGNUM,		/* %wim */
  SPARC32_TBR_REGNUM,		/* %tbr */
  SPARC32_PC_REGNUM,		/* %pc */
  SPARC32_NPC_REGNUM,		/* %npc */
  SPARC32_FSR_REGNUM,		/* %fsr */
  SPARC32_CSR_REGNUM,		/* %csr */
};

/* Pseudo registers.  */
enum sparc32_pseudo_regnum
{
  SPARC32_D0_REGNUM = 0,	/* %d0 */
  SPARC32_D30_REGNUM		/* %d30 */
  = SPARC32_D0_REGNUM + 15
};


struct sparc_frame_cache
{
  /* Base address.  */
  CORE_ADDR base;
  CORE_ADDR pc;

  /* Do we have a frame?  */
  int frameless_p;

  /* The offset from the base register to the CFA.  */
  int frame_offset;

  /* Mask of `local' and `in' registers saved in the register save area.  */
  unsigned short int saved_regs_mask;

  /* Mask of `out' registers copied or renamed to their `in' sibling.  */
  unsigned char copied_regs_mask;

  /* Do we have a Structure, Union or Quad-Precision return value?  */
  int struct_return_p;

  /* Table of saved registers.  */
  struct trad_frame_saved_reg *saved_regs;
};

/* Fetch the instruction at PC.  */
extern unsigned long sparc_fetch_instruction (CORE_ADDR pc);

/* Fetch StackGhost Per-Process XOR cookie.  */
extern ULONGEST sparc_fetch_wcookie (struct gdbarch *gdbarch);

/* Record the effect of a SAVE instruction on CACHE.  */
extern void sparc_record_save_insn (struct sparc_frame_cache *cache);

/* Do a full analysis of the prologue at PC and update CACHE accordingly.  */
extern CORE_ADDR sparc_analyze_prologue (struct gdbarch *gdbarch,
					 CORE_ADDR pc, CORE_ADDR current_pc,
					 struct sparc_frame_cache *cache);

extern struct sparc_frame_cache *
  sparc_frame_cache (frame_info_ptr this_frame, void **this_cache);

extern struct sparc_frame_cache *
  sparc32_frame_cache (frame_info_ptr this_frame, void **this_cache);

extern int
  sparc_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc);



extern void sparc_supply_rwindow (struct regcache *regcache,
				  CORE_ADDR sp, int regnum);
extern void sparc_collect_rwindow (const struct regcache *regcache,
				   CORE_ADDR sp, int regnum);

/* Register offsets for SunOS 4.  */
extern const struct sparc_gregmap sparc32_sunos4_gregmap;
extern const struct sparc_fpregmap sparc32_sunos4_fpregmap;
extern const struct sparc_fpregmap sparc32_bsd_fpregmap;

extern void sparc32_supply_gregset (const struct sparc_gregmap *gregmap,
				    struct regcache *regcache,
				    int regnum, const void *gregs);
extern void sparc32_collect_gregset (const struct sparc_gregmap *gregmap,
				     const struct regcache *regcache,
				     int regnum, void *gregs);
extern void sparc32_supply_fpregset (const struct sparc_fpregmap *fpregmap,
				     struct regcache *regcache,
				     int regnum, const void *fpregs);
extern void sparc32_collect_fpregset (const struct sparc_fpregmap *fpregmap,
				      const struct regcache *regcache,
				      int regnum, void *fpregs);

extern int sparc_is_annulled_branch_insn (CORE_ADDR pc);

/* Functions and variables exported from sparc-sol2-tdep.c.  */

/* Register offsets for Solaris 2.  */
extern const struct sparc_gregmap sparc32_sol2_gregmap;
extern const struct sparc_fpregmap sparc32_sol2_fpregmap;

/* Functions and variables exported from sparc-netbsd-tdep.c.  */

/* Register offsets for NetBSD.  */
extern const struct sparc_gregmap sparc32nbsd_gregmap;

/* Return the address of a system call's alternative return
   address.  */
extern CORE_ADDR sparcnbsd_step_trap (frame_info_ptr frame,
				      unsigned long insn);

extern void sparc32nbsd_init_abi (struct gdbarch_info info,
				  struct gdbarch *gdbarch);

extern struct trad_frame_saved_reg *
  sparc32nbsd_sigcontext_saved_regs (frame_info_ptr next_frame);

#endif /* sparc-tdep.h */
