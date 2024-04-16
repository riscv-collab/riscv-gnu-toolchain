/* Common target dependent code for GDB on AArch64 systems.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

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


#ifndef AARCH64_TDEP_H
#define AARCH64_TDEP_H

#include "arch/aarch64.h"
#include "displaced-stepping.h"
#include "infrun.h"
#include "gdbarch.h"

/* Forward declarations.  */
struct gdbarch;
struct regset;

/* AArch64 Dwarf register numbering.  */
#define AARCH64_DWARF_X0   0
#define AARCH64_DWARF_SP  31
#define AARCH64_DWARF_PC  32
#define AARCH64_DWARF_RA_SIGN_STATE  34
#define AARCH64_DWARF_V0  64
#define AARCH64_DWARF_SVE_VG   46
#define AARCH64_DWARF_SVE_FFR  47
#define AARCH64_DWARF_SVE_P0   48
#define AARCH64_DWARF_SVE_Z0   96

/* Size of integer registers.  */
#define X_REGISTER_SIZE  8
#define B_REGISTER_SIZE  1
#define H_REGISTER_SIZE  2
#define S_REGISTER_SIZE  4
#define D_REGISTER_SIZE  8
#define Q_REGISTER_SIZE 16

/* Total number of general (X) registers.  */
#define AARCH64_X_REGISTER_COUNT 32
/* Total number of D registers.  */
#define AARCH64_D_REGISTER_COUNT 32

/* The maximum number of modified instructions generated for one
   single-stepped instruction.  */
#define AARCH64_DISPLACED_MODIFIED_INSNS 1

/* Target-dependent structure in gdbarch.  */
struct aarch64_gdbarch_tdep : gdbarch_tdep_base
{
  /* Lowest address at which instructions will appear.  */
  CORE_ADDR lowest_pc = 0;

  /* Offset to PC value in jump buffer.  If this is negative, longjmp
     support will be disabled.  */
  int jb_pc = 0;

  /* And the size of each entry in the buf.  */
  size_t jb_elt_size = 0;

  /* Types for AdvSISD registers.  */
  struct type *vnq_type = nullptr;
  struct type *vnd_type = nullptr;
  struct type *vns_type = nullptr;
  struct type *vnh_type = nullptr;
  struct type *vnb_type = nullptr;
  struct type *vnv_type = nullptr;

  /* Types for SME ZA tiles and tile slices pseudo-registers.  */
  struct type *sme_tile_type_q = nullptr;
  struct type *sme_tile_type_d = nullptr;
  struct type *sme_tile_type_s = nullptr;
  struct type *sme_tile_type_h = nullptr;
  struct type *sme_tile_type_b = nullptr;
  struct type *sme_tile_slice_type_q = nullptr;
  struct type *sme_tile_slice_type_d = nullptr;
  struct type *sme_tile_slice_type_s = nullptr;
  struct type *sme_tile_slice_type_h = nullptr;
  struct type *sme_tile_slice_type_b = nullptr;

  /* Vector of names for SME pseudo-registers.  The number of elements is
     different for each distinct svl value.  */
  std::vector<std::string> sme_pseudo_names;

  /* syscall record.  */
  int (*aarch64_syscall_record) (struct regcache *regcache,
				 unsigned long svc_number) = nullptr;

  /* The VQ value for SVE targets, or zero if SVE is not supported.  */
  uint64_t vq = 0;

  /* Returns true if the target supports SVE.  */
  bool has_sve () const
  {
    return vq != 0;
  }

  int pauth_reg_base = 0;
  /* Number of pauth masks.  */
  int pauth_reg_count = 0;
  int ra_sign_state_regnum = 0;

  /* Returns true if the target supports pauth.  */
  bool has_pauth () const
  {
    return pauth_reg_base != -1;
  }

  /* First MTE register.  This is -1 if no MTE registers are available.  */
  int mte_reg_base = 0;

  /* Returns true if the target supports MTE.  */
  bool has_mte () const
  {
    return mte_reg_base != -1;
  }

  /* TLS registers.  This is -1 if the TLS registers are not available.  */
  int tls_regnum_base = 0;
  int tls_register_count = 0;

  bool has_tls() const
  {
    return tls_regnum_base != -1;
  }

  /* The W pseudo-registers.  */
  int w_pseudo_base = 0;
  int w_pseudo_count = 0;

  /* SME feature fields.  */

  /* Index of the first SME register.  This is -1 if SME is not supported.  */
  int sme_reg_base = 0;
  /* svg register index.  */
  int sme_svg_regnum = 0;
  /* svcr register index.  */
  int sme_svcr_regnum = 0;
  /* ZA register index.  */
  int sme_za_regnum = 0;
  /* Index of the first SME pseudo-register.  This is -1 if SME is not
     supported.  */
  int sme_pseudo_base = 0;
  /* Total number of SME pseudo-registers.  */
  int sme_pseudo_count = 0;
  /* First tile slice pseudo-register index.  */
  int sme_tile_slice_pseudo_base = 0;
  /* Total number of tile slice pseudo-registers.  */
  int sme_tile_slice_pseudo_count = 0;
  /* First tile pseudo-register index.  */
  int sme_tile_pseudo_base = 0;
  /* The streaming vector quotient (svq) for SME, or zero if SME is not
     supported.  */
  size_t sme_svq = 0;

  /* Return true if the target supports SME, and false otherwise.  */
  bool has_sme () const
  {
    return sme_svq != 0;
  }

  /* Index of the SME2 ZT0 register.  This is -1 if SME2 is not
     supported.  */
  int sme2_zt0_regnum = -1;

  /* Return true if the target supports SME2, and false otherwise.  */
  bool has_sme2 () const
  {
    return sme2_zt0_regnum > 0;
  }
};

const target_desc *aarch64_read_description (const aarch64_features &features);
aarch64_features
aarch64_features_from_target_desc (const struct target_desc *tdesc);

extern int aarch64_process_record (struct gdbarch *gdbarch,
			       struct regcache *regcache, CORE_ADDR addr);

displaced_step_copy_insn_closure_up
  aarch64_displaced_step_copy_insn (struct gdbarch *gdbarch,
				    CORE_ADDR from, CORE_ADDR to,
				    struct regcache *regs);

void aarch64_displaced_step_fixup (struct gdbarch *gdbarch,
				   displaced_step_copy_insn_closure *dsc,
				   CORE_ADDR from, CORE_ADDR to,
				   struct regcache *regs, bool completed_p);

bool aarch64_displaced_step_hw_singlestep (struct gdbarch *gdbarch);

#endif /* aarch64-tdep.h */
