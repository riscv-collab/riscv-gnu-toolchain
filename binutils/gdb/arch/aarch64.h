/* Common target-dependent functionality for AArch64.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#ifndef ARCH_AARCH64_H
#define ARCH_AARCH64_H

#include "gdbsupport/tdesc.h"

/* Holds information on what architectural features are available.  This is
   used to select register sets.  */
struct aarch64_features
{
  /* A non zero VQ value indicates both the presence of SVE and the
     Vector Quotient - the number of 128-bit chunks in an SVE Z
     register.

     The maximum value for VQ is 16 (5 bits).  */
  uint64_t vq = 0;
  bool pauth = false;
  bool mte = false;

  /* A positive TLS value indicates the number of TLS registers available.  */
  uint8_t tls = 0;
  /* The allowed values for SVQ are the following:

     0 - SME is not supported/available.
     1 - SME is available, SVL is 16 bytes / 128-bit.
     2 - SME is available, SVL is 32 bytes / 256-bit.
     4 - SME is available, SVL is 64 bytes / 512-bit.
     8 - SME is available, SVL is 128 bytes / 1024-bit.
     16 - SME is available, SVL is 256 bytes / 2048-bit.

     These use at most 5 bits to represent.  */
  uint8_t svq = 0;

  /* Whether SME2 is supported.  */
  bool sme2 = false;
};

inline bool operator==(const aarch64_features &lhs, const aarch64_features &rhs)
{
  return lhs.vq == rhs.vq
    && lhs.pauth == rhs.pauth
    && lhs.mte == rhs.mte
    && lhs.tls == rhs.tls
    && lhs.svq == rhs.svq
    && lhs.sme2 == rhs.sme2;
}

namespace std
{
  template<>
  struct hash<aarch64_features>
  {
    std::size_t operator()(const aarch64_features &features) const noexcept
    {
      std::size_t h;

      h = features.vq;
      h = h << 1 | features.pauth;
      h = h << 1 | features.mte;
      /* Shift by two bits for now.  We may need to increase this in the future
	 if more TLS registers get added.  */
      h = h << 2 | features.tls;

      /* Make sure the SVQ values are within the limits.  */
      gdb_assert (features.svq >= 0);
      gdb_assert (features.svq <= 16);
      h = h << 5 | (features.svq & 0x5);

      /* SME2 feature.  */
      h = h << 1 | features.sme2;
      return h;
    }
  };
}

/* Create the aarch64 target description.  */

target_desc *
  aarch64_create_target_description (const aarch64_features &features);

/* Given a pointer value POINTER and a MASK of non-address bits, remove the
   non-address bits from the pointer and sign-extend the result if required.
   The sign-extension is required so we can handle kernel addresses
   correctly.  */
CORE_ADDR aarch64_remove_top_bits (CORE_ADDR pointer, CORE_ADDR mask);

/* Given CMASK and DMASK the two PAC mask registers, return the correct PAC
   mask to use for removing non-address bits from a pointer.  */
CORE_ADDR
aarch64_mask_from_pac_registers (const CORE_ADDR cmask, const CORE_ADDR dmask);

/* Register numbers of various important registers.
   Note that on SVE, the Z registers reuse the V register numbers and the V
   registers become pseudo registers.  */
enum aarch64_regnum
{
  AARCH64_X0_REGNUM,		/* First integer register.  */
  AARCH64_FP_REGNUM = AARCH64_X0_REGNUM + 29,	/* Frame register, if used.  */
  AARCH64_LR_REGNUM = AARCH64_X0_REGNUM + 30,	/* Return address.  */
  AARCH64_SP_REGNUM,		/* Stack pointer.  */
  AARCH64_PC_REGNUM,		/* Program counter.  */
  AARCH64_CPSR_REGNUM,		/* Current Program Status Register.  */
  AARCH64_V0_REGNUM,		/* First fp/vec register.  */
  AARCH64_V31_REGNUM = AARCH64_V0_REGNUM + 31,	/* Last fp/vec register.  */
  AARCH64_SVE_Z0_REGNUM = AARCH64_V0_REGNUM,	/* First SVE Z register.  */
  AARCH64_SVE_Z31_REGNUM = AARCH64_V31_REGNUM,  /* Last SVE Z register.  */
  AARCH64_FPSR_REGNUM,		/* Floating Point Status Register.  */
  AARCH64_FPCR_REGNUM,		/* Floating Point Control Register.  */
  AARCH64_SVE_P0_REGNUM,	/* First SVE predicate register.  */
  AARCH64_SVE_P15_REGNUM = AARCH64_SVE_P0_REGNUM + 15,	/* Last SVE predicate
							   register.  */
  AARCH64_SVE_FFR_REGNUM,	/* SVE First Fault Register.  */
  AARCH64_SVE_VG_REGNUM,	/* SVE Vector Granule.  */

  /* Other useful registers.  */
  AARCH64_LAST_X_ARG_REGNUM = AARCH64_X0_REGNUM + 7,
  AARCH64_STRUCT_RETURN_REGNUM = AARCH64_X0_REGNUM + 8,
  AARCH64_LAST_V_ARG_REGNUM = AARCH64_V0_REGNUM + 7
};

/* Sizes of various AArch64 registers.  */
#define AARCH64_TLS_REGISTER_SIZE 8
#define V_REGISTER_SIZE	  16

/* PAC-related constants.  */
/* Bit 55 is used to select between a kernel-space and user-space address.  */
#define VA_RANGE_SELECT_BIT_MASK  0x80000000000000ULL
/* Mask with 1's in bits 55~63, used to remove the top byte of pointers
   (Top Byte Ignore).  */
#define AARCH64_TOP_BITS_MASK	  0xff80000000000000ULL

/* Pseudo register base numbers.  */
#define AARCH64_Q0_REGNUM 0
#define AARCH64_D0_REGNUM (AARCH64_Q0_REGNUM + AARCH64_D_REGISTER_COUNT)
#define AARCH64_S0_REGNUM (AARCH64_D0_REGNUM + 32)
#define AARCH64_H0_REGNUM (AARCH64_S0_REGNUM + 32)
#define AARCH64_B0_REGNUM (AARCH64_H0_REGNUM + 32)
#define AARCH64_SVE_V0_REGNUM (AARCH64_B0_REGNUM + 32)

#define AARCH64_PAUTH_DMASK_REGNUM(pauth_reg_base) (pauth_reg_base)
#define AARCH64_PAUTH_CMASK_REGNUM(pauth_reg_base) (pauth_reg_base + 1)
/* The high versions of these masks are used for bare metal/kernel-mode pointer
   authentication support.  */
#define AARCH64_PAUTH_DMASK_HIGH_REGNUM(pauth_reg_base) (pauth_reg_base + 2)
#define AARCH64_PAUTH_CMASK_HIGH_REGNUM(pauth_reg_base) (pauth_reg_base + 3)

/* This size is only meant for Linux, not bare metal.  QEMU exposes 4 masks.  */
#define AARCH64_PAUTH_REGS_SIZE (16)

#define AARCH64_X_REGS_NUM 31
#define AARCH64_V_REGS_NUM 32
#define AARCH64_SVE_Z_REGS_NUM AARCH64_V_REGS_NUM
#define AARCH64_SVE_P_REGS_NUM 16
#define AARCH64_NUM_REGS AARCH64_FPCR_REGNUM + 1
#define AARCH64_SVE_NUM_REGS AARCH64_SVE_VG_REGNUM + 1

/* There are a number of ways of expressing the current SVE vector size:

   VL : Vector Length.
	The number of bytes in an SVE Z register.
   VQ : Vector Quotient.
	The number of 128bit chunks in an SVE Z register.
   VG : Vector Granule.
	The number of 64bit chunks in an SVE Z register.  */

#define sve_vg_from_vl(vl)	((vl) / 8)
#define sve_vl_from_vg(vg)	((vg) * 8)
#ifndef sve_vq_from_vl
#define sve_vq_from_vl(vl)	((vl) / 0x10)
#endif
#ifndef sve_vl_from_vq
#define sve_vl_from_vq(vq)	((vq) * 0x10)
#endif
#define sve_vq_from_vg(vg)	(sve_vq_from_vl (sve_vl_from_vg (vg)))
#define sve_vg_from_vq(vq)	(sve_vg_from_vl (sve_vl_from_vq (vq)))


/* Maximum supported VQ value.  Increase if required.  */
#define AARCH64_MAX_SVE_VQ  16

/* SME definitions

   Some of these definitions are not found in the Architecture Reference
   Manual, but we use them so we can keep a similar standard compared to the
   SVE definitions that the Linux Kernel uses.  Otherwise it can get
   confusing.

   SVL : Streaming Vector Length.
	 Although the documentation handles SVL in bits, we do it in
	 bytes to match what we do for SVE.

	 The streaming vector length dictates the size of the ZA register and
	 the size of the SVE registers when in streaming mode.

   SVQ : Streaming Vector Quotient.
	 The number of 128-bit chunks in an SVE Z register or the size of
	 each dimension of the SME ZA matrix.

   SVG : Streaming Vector Granule.
	 The number of 64-bit chunks in an SVE Z register or the size of
	 half a SME ZA matrix dimension.  The SVG definition was added so
	 we keep a familiar definition when dealing with SVE registers in
	 streaming mode.  */

/* The total number of tiles.  This is always fixed regardless of the
   streaming vector length (svl).  */
#define AARCH64_ZA_TILES_NUM 31
/* svl limits for SME.  */
#define AARCH64_SME_MIN_SVL 128
#define AARCH64_SME_MAX_SVL 2048

/* Size of the SME2 ZT0 register in bytes.  */
#define AARCH64_SME2_ZT0_SIZE 64

#endif /* ARCH_AARCH64_H */
