/* Target-dependent code for GNU/Linux AArch64.

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

#include "defs.h"

#include "gdbarch.h"
#include "glibc-tdep.h"
#include "linux-tdep.h"
#include "aarch64-tdep.h"
#include "aarch64-linux-tdep.h"
#include "osabi.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "tramp-frame.h"
#include "trad-frame.h"
#include "target.h"
#include "target/target.h"
#include "expop.h"
#include "auxv.h"

#include "regcache.h"
#include "regset.h"

#include "stap-probe.h"
#include "parser-defs.h"
#include "user-regs.h"
#include "xml-syscall.h"
#include <ctype.h>

#include "record-full.h"
#include "linux-record.h"

#include "arch/aarch64-mte-linux.h"
#include "arch/aarch64-scalable-linux.h"

#include "arch-utils.h"
#include "value.h"

#include "gdbsupport/selftest.h"

#include "elf/common.h"
#include "elf/aarch64.h"
#include "arch/aarch64-insn.h"

/* For std::pow */
#include <cmath>

/* Signal frame handling.

      +------------+  ^
      | saved lr   |  |
   +->| saved fp   |--+
   |  |            |
   |  |            |
   |  +------------+
   |  | saved lr   |
   +--| saved fp   |
   ^  |            |
   |  |            |
   |  +------------+
   ^  |            |
   |  | signal     |
   |  |            |        SIGTRAMP_FRAME (struct rt_sigframe)
   |  | saved regs |
   +--| saved sp   |--> interrupted_sp
   |  | saved pc   |--> interrupted_pc
   |  |            |
   |  +------------+
   |  | saved lr   |--> default_restorer (movz x8, NR_sys_rt_sigreturn; svc 0)
   +--| saved fp   |<- FP
      |            |         NORMAL_FRAME
      |            |<- SP
      +------------+

  On signal delivery, the kernel will create a signal handler stack
  frame and setup the return address in LR to point at restorer stub.
  The signal stack frame is defined by:

  struct rt_sigframe
  {
    siginfo_t info;
    struct ucontext uc;
  };

  The ucontext has the following form:
  struct ucontext
  {
    unsigned long uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    sigset_t uc_sigmask;
    struct sigcontext uc_mcontext;
  };

  struct sigcontext
  {
    unsigned long fault_address;
    unsigned long regs[31];
    unsigned long sp;		/ * 31 * /
    unsigned long pc;		/ * 32 * /
    unsigned long pstate;	/ * 33 * /
    __u8 __reserved[4096]
  };

  The reserved space in sigcontext contains additional structures, each starting
  with a aarch64_ctx, which specifies a unique identifier and the total size of
  the structure.  The final structure in reserved will start will a null
  aarch64_ctx.  The penultimate entry in reserved may be a extra_context which
  then points to a further block of reserved space.

  struct aarch64_ctx {
	u32 magic;
	u32 size;
  };

  The restorer stub will always have the form:

  d28015a8        movz    x8, #0xad
  d4000001        svc     #0x0

  This is a system call sys_rt_sigreturn.

  We detect signal frames by snooping the return code for the restorer
  instruction sequence.

  The handler then needs to recover the saved register set from
  ucontext.uc_mcontext.  */

/* These magic numbers need to reflect the layout of the kernel
   defined struct rt_sigframe and ucontext.  */
#define AARCH64_SIGCONTEXT_REG_SIZE             8
#define AARCH64_RT_SIGFRAME_UCONTEXT_OFFSET     128
#define AARCH64_UCONTEXT_SIGCONTEXT_OFFSET      176
#define AARCH64_SIGCONTEXT_XO_OFFSET            8
#define AARCH64_SIGCONTEXT_RESERVED_OFFSET      288

#define AARCH64_SIGCONTEXT_RESERVED_SIZE	4096

/* Unique identifiers that may be used for aarch64_ctx.magic.  */
#define AARCH64_EXTRA_MAGIC			0x45585401
#define AARCH64_FPSIMD_MAGIC			0x46508001
#define AARCH64_SVE_MAGIC			0x53564501
#define AARCH64_ZA_MAGIC			0x54366345
#define AARCH64_TPIDR2_MAGIC			0x54504902
#define AARCH64_ZT_MAGIC			0x5a544e01

/* Defines for the extra_context that follows an AARCH64_EXTRA_MAGIC.  */
#define AARCH64_EXTRA_DATAP_OFFSET		8

/* Defines for the fpsimd that follows an AARCH64_FPSIMD_MAGIC.  */
#define AARCH64_FPSIMD_FPSR_OFFSET		8
#define AARCH64_FPSIMD_FPCR_OFFSET		12
#define AARCH64_FPSIMD_V0_OFFSET		16
#define AARCH64_FPSIMD_VREG_SIZE		16

/* Defines for the sve structure that follows an AARCH64_SVE_MAGIC.  */
#define AARCH64_SVE_CONTEXT_VL_OFFSET		8
#define AARCH64_SVE_CONTEXT_FLAGS_OFFSET	10
#define AARCH64_SVE_CONTEXT_REGS_OFFSET		16
#define AARCH64_SVE_CONTEXT_P_REGS_OFFSET(vq) (32 * vq * 16)
#define AARCH64_SVE_CONTEXT_FFR_OFFSET(vq) \
  (AARCH64_SVE_CONTEXT_P_REGS_OFFSET (vq) + (16 * vq * 2))
#define AARCH64_SVE_CONTEXT_SIZE(vq) \
  (AARCH64_SVE_CONTEXT_FFR_OFFSET (vq) + (vq * 2))
/* Flag indicating the SVE Context describes streaming mode.  */
#define SVE_SIG_FLAG_SM				0x1

/* SME constants.  */
#define AARCH64_SME_CONTEXT_SVL_OFFSET		8
#define AARCH64_SME_CONTEXT_REGS_OFFSET		16
#define AARCH64_SME_CONTEXT_ZA_SIZE(svq) \
  ((sve_vl_from_vq (svq) * sve_vl_from_vq (svq)))
#define AARCH64_SME_CONTEXT_SIZE(svq) \
  (AARCH64_SME_CONTEXT_REGS_OFFSET + AARCH64_SME_CONTEXT_ZA_SIZE (svq))

/* TPIDR2 register value offset in the TPIDR2 signal frame context.  */
#define AARCH64_TPIDR2_CONTEXT_TPIDR2_OFFSET	8

/* SME2 (ZT) constants.  */
/* Offset of the field containing the number of registers in the SME2 signal
   context state.  */
#define AARCH64_SME2_CONTEXT_NREGS_OFFSET	8
/* Offset of the beginning of the register data for the first ZT register in
   the signal context state.  */
#define AARCH64_SME2_CONTEXT_REGS_OFFSET	16

/* Holds information about the signal frame.  */
struct aarch64_linux_sigframe
{
  /* The stack pointer value.  */
  CORE_ADDR sp = 0;
  /* The sigcontext address.  */
  CORE_ADDR sigcontext_address = 0;
  /* The start/end signal frame section addresses.  */
  CORE_ADDR section = 0;
  CORE_ADDR section_end = 0;

  /* Starting address of the section containing the general purpose
     registers.  */
  CORE_ADDR gpr_section = 0;
  /* Starting address of the section containing the FPSIMD registers.  */
  CORE_ADDR fpsimd_section = 0;
  /* Starting address of the section containing the SVE registers.  */
  CORE_ADDR sve_section = 0;
  /* Starting address of the section containing the ZA register.  */
  CORE_ADDR za_section = 0;
  /* Starting address of the section containing the TPIDR2 register.  */
  CORE_ADDR tpidr2_section = 0;
  /* Starting address of the section containing the ZT registers.  */
  CORE_ADDR zt_section = 0;
  /* Starting address of the section containing extra information.  */
  CORE_ADDR extra_section = 0;

  /* The vector length (SVE or SSVE).  */
  ULONGEST vl = 0;
  /* The streaming vector length (SSVE/ZA).  */
  ULONGEST svl = 0;
  /* Number of ZT registers in this context.  */
  unsigned int zt_register_count = 0;

  /* True if we are in streaming mode, false otherwise.  */
  bool streaming_mode = false;
  /* True if we have a ZA payload, false otherwise.  */
  bool za_payload = false;
  /* True if we have a ZT entry in the signal context, false otherwise.  */
  bool zt_available = false;
};

/* Read an aarch64_ctx, returning the magic value, and setting *SIZE to the
   size, or return 0 on error.  */

static uint32_t
read_aarch64_ctx (CORE_ADDR ctx_addr, enum bfd_endian byte_order,
		  uint32_t *size)
{
  uint32_t magic = 0;
  gdb_byte buf[4];

  if (target_read_memory (ctx_addr, buf, 4) != 0)
    return 0;
  magic = extract_unsigned_integer (buf, 4, byte_order);

  if (target_read_memory (ctx_addr + 4, buf, 4) != 0)
    return 0;
  *size = extract_unsigned_integer (buf, 4, byte_order);

  return magic;
}

/* Given CACHE, use the trad_frame* functions to restore the FPSIMD
   registers from a signal frame.

   FPSIMD_CONTEXT is the address of the signal frame context containing FPSIMD
   data.  */

static void
aarch64_linux_restore_vregs (struct gdbarch *gdbarch,
			     struct trad_frame_cache *cache,
			     CORE_ADDR fpsimd_context)
{
  /* WARNING: SIMD state is laid out in memory in target-endian format.

     So we have a couple cases to consider:

     1 - If the target is big endian, then SIMD state is big endian,
     requiring a byteswap.

     2 - If the target is little endian, then SIMD state is little endian, so
     no byteswap is needed. */

  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int num_regs = gdbarch_num_regs (gdbarch);
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  for (int i = 0; i < 32; i++)
    {
      CORE_ADDR offset = (fpsimd_context + AARCH64_FPSIMD_V0_OFFSET
			  + (i * AARCH64_FPSIMD_VREG_SIZE));

      gdb_byte buf[V_REGISTER_SIZE];

      /* Read the contents of the V register.  */
      if (target_read_memory (offset, buf, V_REGISTER_SIZE))
	error (_("Failed to read fpsimd register from signal context."));

      if (byte_order == BFD_ENDIAN_BIG)
	{
	  size_t size = V_REGISTER_SIZE/2;

	  /* Read the two halves of the V register in reverse byte order.  */
	  CORE_ADDR u64 = extract_unsigned_integer (buf, size,
						    byte_order);
	  CORE_ADDR l64 = extract_unsigned_integer (buf + size, size,
						    byte_order);

	  /* Copy the reversed bytes to the buffer.  */
	  store_unsigned_integer (buf, size, BFD_ENDIAN_LITTLE, l64);
	  store_unsigned_integer (buf + size , size, BFD_ENDIAN_LITTLE, u64);

	  /* Now we can store the correct bytes for the V register.  */
	  trad_frame_set_reg_value_bytes (cache, AARCH64_V0_REGNUM + i,
					  {buf, V_REGISTER_SIZE});
	  trad_frame_set_reg_value_bytes (cache,
					  num_regs + AARCH64_Q0_REGNUM
					  + i, {buf, Q_REGISTER_SIZE});
	  trad_frame_set_reg_value_bytes (cache,
					  num_regs + AARCH64_D0_REGNUM
					  + i, {buf, D_REGISTER_SIZE});
	  trad_frame_set_reg_value_bytes (cache,
					  num_regs + AARCH64_S0_REGNUM
					  + i, {buf, S_REGISTER_SIZE});
	  trad_frame_set_reg_value_bytes (cache,
					  num_regs + AARCH64_H0_REGNUM
					  + i, {buf, H_REGISTER_SIZE});
	  trad_frame_set_reg_value_bytes (cache,
					  num_regs + AARCH64_B0_REGNUM
					  + i, {buf, B_REGISTER_SIZE});

	  if (tdep->has_sve ())
	    trad_frame_set_reg_value_bytes (cache,
					    num_regs + AARCH64_SVE_V0_REGNUM
					    + i, {buf, V_REGISTER_SIZE});
	}
      else
	{
	  /* Little endian, just point at the address containing the register
	     value.  */
	  trad_frame_set_reg_addr (cache, AARCH64_V0_REGNUM + i, offset);
	  trad_frame_set_reg_addr (cache, num_regs + AARCH64_Q0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (cache, num_regs + AARCH64_D0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (cache, num_regs + AARCH64_S0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (cache, num_regs + AARCH64_H0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (cache, num_regs + AARCH64_B0_REGNUM + i,
				   offset);

	  if (tdep->has_sve ())
	    trad_frame_set_reg_addr (cache, num_regs + AARCH64_SVE_V0_REGNUM
				     + i, offset);
	}

      if (tdep->has_sve ())
	{
	  /* If SVE is supported for this target, zero out the Z
	     registers then copy the first 16 bytes of each of the V
	     registers to the associated Z register.  Otherwise the Z
	     registers will contain uninitialized data.  */
	  std::vector<gdb_byte> z_buffer (tdep->vq * 16);

	  /* We have already handled the endianness swap above, so we don't need
	     to worry about it here.  */
	  memcpy (z_buffer.data (), buf, V_REGISTER_SIZE);
	  trad_frame_set_reg_value_bytes (cache,
					  AARCH64_SVE_Z0_REGNUM + i,
					  z_buffer);
	}
    }
}

/* Given a signal frame THIS_FRAME, read the signal frame information into
   SIGNAL_FRAME.  */

static void
aarch64_linux_read_signal_frame_info (frame_info_ptr this_frame,
				  struct aarch64_linux_sigframe &signal_frame)
{
  signal_frame.sp = get_frame_register_unsigned (this_frame, AARCH64_SP_REGNUM);
  signal_frame.sigcontext_address
    = signal_frame.sp + AARCH64_RT_SIGFRAME_UCONTEXT_OFFSET
      + AARCH64_UCONTEXT_SIGCONTEXT_OFFSET;
  signal_frame.section
    = signal_frame.sigcontext_address + AARCH64_SIGCONTEXT_RESERVED_OFFSET;
  signal_frame.section_end
    = signal_frame.section + AARCH64_SIGCONTEXT_RESERVED_SIZE;

  signal_frame.gpr_section
    = signal_frame.sigcontext_address + AARCH64_SIGCONTEXT_XO_OFFSET;

  /* Search for all the other sections, stopping at null.  */
  CORE_ADDR section = signal_frame.section;
  CORE_ADDR section_end = signal_frame.section_end;
  uint32_t size, magic;
  bool extra_found = false;
  enum bfd_endian byte_order
    = gdbarch_byte_order (get_frame_arch (this_frame));

  while ((magic = read_aarch64_ctx (section, byte_order, &size)) != 0
	 && size != 0)
    {
      switch (magic)
	{
	case AARCH64_FPSIMD_MAGIC:
	  {
	    signal_frame.fpsimd_section = section;
	    section += size;
	    break;
	  }

	case AARCH64_SVE_MAGIC:
	  {
	    /* Check if the section is followed by a full SVE dump, and set
	       sve_regs if it is.  */
	    gdb_byte buf[4];

	    /* Extract the vector length.  */
	    if (target_read_memory (section + AARCH64_SVE_CONTEXT_VL_OFFSET,
				    buf, 2) != 0)
	      {
		warning (_("Failed to read the vector length from the SVE "
			   "signal frame context."));
		section += size;
		break;
	      }

	    signal_frame.vl = extract_unsigned_integer (buf, 2, byte_order);

	    /* Extract the flags to check if we are in streaming mode.  */
	    if (target_read_memory (section
				    + AARCH64_SVE_CONTEXT_FLAGS_OFFSET,
				    buf, 2) != 0)
	      {
		warning (_("Failed to read the flags from the SVE signal frame"
			   " context."));
		section += size;
		break;
	      }

	    uint16_t flags = extract_unsigned_integer (buf, 2, byte_order);

	    /* Is this SSVE data? If so, we are in streaming mode.  */
	    signal_frame.streaming_mode
	      = (flags & SVE_SIG_FLAG_SM) ? true : false;

	    ULONGEST vq = sve_vq_from_vl (signal_frame.vl);
	    if (size >= AARCH64_SVE_CONTEXT_SIZE (vq))
	      {
		signal_frame.sve_section
		  = section + AARCH64_SVE_CONTEXT_REGS_OFFSET;
	      }
	    section += size;
	    break;
	  }

	case AARCH64_ZA_MAGIC:
	  {
	    /* Check if the section is followed by a full ZA dump, and set
	       za_state if it is.  */
	    gdb_byte buf[2];

	    /* Extract the streaming vector length.  */
	    if (target_read_memory (section + AARCH64_SME_CONTEXT_SVL_OFFSET,
				    buf, 2) != 0)
	      {
		warning (_("Failed to read the streaming vector length from "
			   "ZA signal frame context."));
		section += size;
		break;
	      }

	    signal_frame.svl = extract_unsigned_integer (buf, 2, byte_order);
	    ULONGEST svq = sve_vq_from_vl (signal_frame.svl);

	    if (size >= AARCH64_SME_CONTEXT_SIZE (svq))
	      {
		signal_frame.za_section
		  = section + AARCH64_SME_CONTEXT_REGS_OFFSET;
		signal_frame.za_payload = true;
	      }
	    section += size;
	    break;
	  }

	case AARCH64_TPIDR2_MAGIC:
	  {
	    /* This is context containing the tpidr2 register.  */
	    signal_frame.tpidr2_section = section;
	    section += size;
	    break;
	  }
	case AARCH64_ZT_MAGIC:
	  {
	    gdb_byte buf[2];

	    /* Extract the number of ZT registers available in this
	       context.  */
	    if (target_read_memory (section + AARCH64_SME2_CONTEXT_NREGS_OFFSET,
				    buf, 2) != 0)
	      {
		warning (_("Failed to read the number of ZT registers from the "
			   "ZT signal frame context."));
		section += size;
		break;
	      }

	    signal_frame.zt_register_count
	      = extract_unsigned_integer (buf, 2, byte_order);

	    /* This is a context containing the ZT registers.  This should only
	       exist if we also have the ZA context.  The presence of the ZT
	       context without the ZA context is invalid.  */
	    signal_frame.zt_section = section;
	    signal_frame.zt_available = true;

	    section += size;
	    break;
	  }
	case AARCH64_EXTRA_MAGIC:
	  {
	    /* Extra is always the last valid section in reserved and points to
	       an additional block of memory filled with more sections. Reset
	       the address to the extra section and continue looking for more
	       structures.  */
	    gdb_byte buf[8];

	    if (target_read_memory (section + AARCH64_EXTRA_DATAP_OFFSET,
				    buf, 8) != 0)
	      {
		warning (_("Failed to read the extra section address from the"
			   " signal frame context."));
		section += size;
		break;
	      }

	    section = extract_unsigned_integer (buf, 8, byte_order);
	    signal_frame.extra_section = section;
	    extra_found = true;
	    break;
	  }

	default:
	  section += size;
	  break;
	}

      /* Prevent searching past the end of the reserved section.  The extra
	 section does not have a hard coded limit - we have to rely on it ending
	 with nulls.  */
      if (!extra_found && section > section_end)
	break;
    }

    /* Sanity check that if the ZT entry exists, the ZA entry must also
       exist.  */
    if (signal_frame.zt_available && !signal_frame.za_payload)
      error (_("While reading signal context information, found a ZT context "
	       "without a ZA context, which is invalid."));
}

/* Implement the "init" method of struct tramp_frame.  */

static void
aarch64_linux_sigframe_init (const struct tramp_frame *self,
			     frame_info_ptr this_frame,
			     struct trad_frame_cache *this_cache,
			     CORE_ADDR func)
{
  /* Read the signal context information.  */
  struct aarch64_linux_sigframe signal_frame;
  aarch64_linux_read_signal_frame_info (this_frame, signal_frame);

  /* Now we have all the data required to restore the registers from the
     signal frame.  */

  /* Restore the general purpose registers.  */
  CORE_ADDR offset = signal_frame.gpr_section;
  for (int i = 0; i < 31; i++)
    {
      trad_frame_set_reg_addr (this_cache, AARCH64_X0_REGNUM + i, offset);
      offset += AARCH64_SIGCONTEXT_REG_SIZE;
    }
  trad_frame_set_reg_addr (this_cache, AARCH64_SP_REGNUM, offset);
  offset += AARCH64_SIGCONTEXT_REG_SIZE;
  trad_frame_set_reg_addr (this_cache, AARCH64_PC_REGNUM, offset);

  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  /* Restore the SVE / FPSIMD registers.  */
  if (tdep->has_sve () && signal_frame.sve_section != 0)
    {
      ULONGEST vq = sve_vq_from_vl (signal_frame.vl);
      CORE_ADDR sve_regs = signal_frame.sve_section;

      /* Restore VG.  */
      trad_frame_set_reg_value (this_cache, AARCH64_SVE_VG_REGNUM,
				sve_vg_from_vl (signal_frame.vl));

      int num_regs = gdbarch_num_regs (gdbarch);
      for (int i = 0; i < 32; i++)
	{
	  offset = sve_regs + (i * vq * 16);
	  trad_frame_set_reg_addr (this_cache, AARCH64_SVE_Z0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (this_cache,
				   num_regs + AARCH64_SVE_V0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (this_cache, num_regs + AARCH64_Q0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (this_cache, num_regs + AARCH64_D0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (this_cache, num_regs + AARCH64_S0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (this_cache, num_regs + AARCH64_H0_REGNUM + i,
				   offset);
	  trad_frame_set_reg_addr (this_cache, num_regs + AARCH64_B0_REGNUM + i,
				   offset);
	}

      offset = sve_regs + AARCH64_SVE_CONTEXT_P_REGS_OFFSET (vq);
      for (int i = 0; i < 16; i++)
	trad_frame_set_reg_addr (this_cache, AARCH64_SVE_P0_REGNUM + i,
				 offset + (i * vq * 2));

      offset = sve_regs + AARCH64_SVE_CONTEXT_FFR_OFFSET (vq);
      trad_frame_set_reg_addr (this_cache, AARCH64_SVE_FFR_REGNUM, offset);
    }

  /* Restore the FPSIMD registers.  */
  if (signal_frame.fpsimd_section != 0)
    {
      CORE_ADDR fpsimd = signal_frame.fpsimd_section;

      trad_frame_set_reg_addr (this_cache, AARCH64_FPSR_REGNUM,
			       fpsimd + AARCH64_FPSIMD_FPSR_OFFSET);
      trad_frame_set_reg_addr (this_cache, AARCH64_FPCR_REGNUM,
			       fpsimd + AARCH64_FPSIMD_FPCR_OFFSET);

      /* If there was no SVE section then set up the V registers.  */
      if (!tdep->has_sve () || signal_frame.sve_section == 0)
	aarch64_linux_restore_vregs (gdbarch, this_cache, fpsimd);
    }

  /* Restore the SME registers.  */
  if (tdep->has_sme ())
    {
      if (signal_frame.za_section != 0)
	{
	  /* Restore the ZA state.  */
	  trad_frame_set_reg_addr (this_cache, tdep->sme_za_regnum,
				   signal_frame.za_section);
	}

      /* Restore/Reconstruct SVCR.  */
      ULONGEST svcr = 0;
      svcr |= signal_frame.za_payload ? SVCR_ZA_BIT : 0;
      svcr |= signal_frame.streaming_mode ? SVCR_SM_BIT : 0;
      trad_frame_set_reg_value (this_cache, tdep->sme_svcr_regnum, svcr);

      /* Restore SVG.  */
      trad_frame_set_reg_value (this_cache, tdep->sme_svg_regnum,
				sve_vg_from_vl (signal_frame.svl));

      /* Handle SME2 (ZT).  */
      if (tdep->has_sme2 ()
	  && signal_frame.za_section != 0
	  && signal_frame.zt_register_count > 0)
	{
	  /* Is ZA state available?  */
	  gdb_assert (svcr & SVCR_ZA_BIT);

	  /* Restore the ZT state.  For now we assume that we only have
	     a single ZT register.  If/When more ZT registers appear, we
	     should update the code to handle that case accordingly.  */
	  trad_frame_set_reg_addr (this_cache, tdep->sme2_zt0_regnum,
				   signal_frame.zt_section
				   + AARCH64_SME2_CONTEXT_REGS_OFFSET);
	}
    }

  /* Restore the tpidr2 register, if the target supports it and if there is
     an entry for it.  */
  if (signal_frame.tpidr2_section != 0 && tdep->has_tls ()
      && tdep->tls_register_count >= 2)
    {
      /* Restore tpidr2.  */
      trad_frame_set_reg_addr (this_cache, tdep->tls_regnum_base + 1,
			       signal_frame.tpidr2_section
			       + AARCH64_TPIDR2_CONTEXT_TPIDR2_OFFSET);
    }

  trad_frame_set_id (this_cache, frame_id_build (signal_frame.sp, func));
}

/* Implements the "prev_arch" method of struct tramp_frame.  */

static struct gdbarch *
aarch64_linux_sigframe_prev_arch (frame_info_ptr this_frame,
				  void **frame_cache)
{
  struct trad_frame_cache *cache
    = (struct trad_frame_cache *) *frame_cache;

  gdb_assert (cache != nullptr);

  struct aarch64_linux_sigframe signal_frame;
  aarch64_linux_read_signal_frame_info (this_frame, signal_frame);

  /* The SVE vector length and the SME vector length may change from frame to
     frame.  Make sure we report the correct architecture to the previous
     frame.

     We can reuse the next frame's architecture here, as it should be mostly
     the same, except for potential different vg and svg values.  */
  const struct target_desc *tdesc
    = gdbarch_target_desc (get_frame_arch (this_frame));
  aarch64_features features = aarch64_features_from_target_desc (tdesc);
  features.vq = sve_vq_from_vl (signal_frame.vl);
  features.svq = (uint8_t) sve_vq_from_vl (signal_frame.svl);

  struct gdbarch_info info;
  info.bfd_arch_info = bfd_lookup_arch (bfd_arch_aarch64, bfd_mach_aarch64);
  info.target_desc = aarch64_read_description (features);
  return gdbarch_find_by_info (info);
}

static const struct tramp_frame aarch64_linux_rt_sigframe =
{
  SIGTRAMP_FRAME,
  4,
  {
    /* movz x8, 0x8b (S=1,o=10,h=0,i=0x8b,r=8)
       Soo1 0010 1hhi iiii iiii iiii iiir rrrr  */
    {0xd2801168, ULONGEST_MAX},

    /* svc  0x0      (o=0, l=1)
       1101 0100 oooi iiii iiii iiii iii0 00ll  */
    {0xd4000001, ULONGEST_MAX},
    {TRAMP_SENTINEL_INSN, ULONGEST_MAX}
  },
  aarch64_linux_sigframe_init,
  nullptr, /* validate */
  aarch64_linux_sigframe_prev_arch, /* prev_arch */
};

/* Register maps.  */

static const struct regcache_map_entry aarch64_linux_gregmap[] =
  {
    { 31, AARCH64_X0_REGNUM, 8 }, /* x0 ... x30 */
    { 1, AARCH64_SP_REGNUM, 8 },
    { 1, AARCH64_PC_REGNUM, 8 },
    { 1, AARCH64_CPSR_REGNUM, 8 },
    { 0 }
  };

static const struct regcache_map_entry aarch64_linux_fpregmap[] =
  {
    { 32, AARCH64_V0_REGNUM, 16 }, /* v0 ... v31 */
    { 1, AARCH64_FPSR_REGNUM, 4 },
    { 1, AARCH64_FPCR_REGNUM, 4 },
    { 0 }
  };

/* Register set definitions.  */

const struct regset aarch64_linux_gregset =
  {
    aarch64_linux_gregmap,
    regcache_supply_regset, regcache_collect_regset
  };

const struct regset aarch64_linux_fpregset =
  {
    aarch64_linux_fpregmap,
    regcache_supply_regset, regcache_collect_regset
  };

/* The fields in an SVE header at the start of a SVE regset.  */

#define SVE_HEADER_SIZE_LENGTH		4
#define SVE_HEADER_MAX_SIZE_LENGTH	4
#define SVE_HEADER_VL_LENGTH		2
#define SVE_HEADER_MAX_VL_LENGTH	2
#define SVE_HEADER_FLAGS_LENGTH		2
#define SVE_HEADER_RESERVED_LENGTH	2

#define SVE_HEADER_SIZE_OFFSET		0
#define SVE_HEADER_MAX_SIZE_OFFSET	\
  (SVE_HEADER_SIZE_OFFSET + SVE_HEADER_SIZE_LENGTH)
#define SVE_HEADER_VL_OFFSET		\
  (SVE_HEADER_MAX_SIZE_OFFSET + SVE_HEADER_MAX_SIZE_LENGTH)
#define SVE_HEADER_MAX_VL_OFFSET	\
  (SVE_HEADER_VL_OFFSET + SVE_HEADER_VL_LENGTH)
#define SVE_HEADER_FLAGS_OFFSET		\
  (SVE_HEADER_MAX_VL_OFFSET + SVE_HEADER_MAX_VL_LENGTH)
#define SVE_HEADER_RESERVED_OFFSET	\
  (SVE_HEADER_FLAGS_OFFSET + SVE_HEADER_FLAGS_LENGTH)
#define SVE_HEADER_SIZE			\
  (SVE_HEADER_RESERVED_OFFSET + SVE_HEADER_RESERVED_LENGTH)

#define SVE_HEADER_FLAG_SVE		1

/* Get the vector quotient (VQ) or streaming vector quotient (SVQ) value
   from the section named SECTION_NAME.

   Return non-zero if successful and 0 otherwise.  */

static uint64_t
aarch64_linux_core_read_vq (struct gdbarch *gdbarch, bfd *abfd,
			    const char *section_name)
{
  gdb_assert (section_name != nullptr);

  asection *section = bfd_get_section_by_name (abfd, section_name);

  if (section == nullptr)
    {
      /* No SVE state.  */
      return 0;
    }

  size_t size = bfd_section_size (section);

  /* Check extended state size.  */
  if (size < SVE_HEADER_SIZE)
    {
      warning (_("'%s' core file section is too small. "
		 "Expected %s bytes, got %s bytes"), section_name,
		 pulongest (SVE_HEADER_SIZE), pulongest (size));
      return 0;
    }

  gdb_byte header[SVE_HEADER_SIZE];

  if (!bfd_get_section_contents (abfd, section, header, 0, SVE_HEADER_SIZE))
    {
      warning (_("Couldn't read sve header from "
		 "'%s' core file section."), section_name);
      return 0;
    }

  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  uint64_t vq
    = sve_vq_from_vl (extract_unsigned_integer (header + SVE_HEADER_VL_OFFSET,
						SVE_HEADER_VL_LENGTH,
						byte_order));

  if (vq > AARCH64_MAX_SVE_VQ || vq == 0)
    {
      warning (_("SVE/SSVE vector length in core file is invalid."
		 " (max vq=%d) (detected vq=%s)"), AARCH64_MAX_SVE_VQ,
	       pulongest (vq));
      return 0;
    }

  return vq;
}

/* Get the vector quotient (VQ) value from CORE_BFD's sections.

   Return non-zero if successful and 0 otherwise.  */

static uint64_t
aarch64_linux_core_read_vq_from_sections (struct gdbarch *gdbarch,
					  bfd *core_bfd)
{
  /* First check if we have a SSVE section.  If so, check if it is active.  */
  asection *section = bfd_get_section_by_name (core_bfd, ".reg-aarch-ssve");

  if (section != nullptr)
    {
      /* We've found a SSVE section, so now fetch its data.  */
      gdb_byte header[SVE_HEADER_SIZE];

      if (bfd_get_section_contents (core_bfd, section, header, 0,
				    SVE_HEADER_SIZE))
	{
	  /* Check if the SSVE section has SVE contents.  */
	  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
	  uint16_t flags
	    = extract_unsigned_integer (header + SVE_HEADER_FLAGS_OFFSET,
					SVE_HEADER_FLAGS_LENGTH, byte_order);

	  if (flags & SVE_HEADER_FLAG_SVE)
	    {
	      /* The SSVE state is active, so return the vector length from the
		 the SSVE section.  */
	      return aarch64_linux_core_read_vq (gdbarch, core_bfd,
						 ".reg-aarch-ssve");
	    }
	}
    }

  /* No valid SSVE section.  Return the vq from the SVE section (if any).  */
  return aarch64_linux_core_read_vq (gdbarch, core_bfd, ".reg-aarch-sve");
}

/* Supply register REGNUM from BUF to REGCACHE, using the register map
   in REGSET.  If REGNUM is -1, do this for all registers in REGSET.
   If BUF is nullptr, set the registers to "unavailable" status.  */

static void
supply_sve_regset (const struct regset *regset,
		   struct regcache *regcache,
		   int regnum, const void *buf, size_t size)
{
  gdb_byte *header = (gdb_byte *) buf;
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  if (buf == nullptr)
    return regcache->supply_regset (regset, regnum, nullptr, size);
  gdb_assert (size > SVE_HEADER_SIZE);

  /* BUF contains an SVE header followed by a register dump of either the
     passed in SVE regset or a NEON fpregset.  */

  /* Extract required fields from the header.  */
  ULONGEST vl = extract_unsigned_integer (header + SVE_HEADER_VL_OFFSET,
					  SVE_HEADER_VL_LENGTH, byte_order);
  uint16_t flags = extract_unsigned_integer (header + SVE_HEADER_FLAGS_OFFSET,
					     SVE_HEADER_FLAGS_LENGTH,
					     byte_order);

  if (regnum == -1 || regnum == AARCH64_SVE_VG_REGNUM)
    {
      gdb_byte vg_target[8];
      store_integer ((gdb_byte *)&vg_target, sizeof (uint64_t), byte_order,
		     sve_vg_from_vl (vl));
      regcache->raw_supply (AARCH64_SVE_VG_REGNUM, &vg_target);
    }

  if (flags & SVE_HEADER_FLAG_SVE)
    {
      /* Register dump is a SVE structure.  */
      regcache->supply_regset (regset, regnum,
			       (gdb_byte *) buf + SVE_HEADER_SIZE,
			       size - SVE_HEADER_SIZE);
    }
  else
    {
      /* Register dump is a fpsimd structure.  First clear the SVE
	 registers.  */
      for (int i = 0; i < AARCH64_SVE_Z_REGS_NUM; i++)
	regcache->raw_supply_zeroed (AARCH64_SVE_Z0_REGNUM + i);
      for (int i = 0; i < AARCH64_SVE_P_REGS_NUM; i++)
	regcache->raw_supply_zeroed (AARCH64_SVE_P0_REGNUM + i);
      regcache->raw_supply_zeroed (AARCH64_SVE_FFR_REGNUM);

      /* Then supply the fpsimd registers.  */
      regcache->supply_regset (&aarch64_linux_fpregset, regnum,
			       (gdb_byte *) buf + SVE_HEADER_SIZE,
			       size - SVE_HEADER_SIZE);
    }
}

/* Collect an inactive SVE register set state.  This is equivalent to a
   fpsimd layout.

   Collect the data from REGCACHE to BUF, using the register
   map in REGSET.  */

static void
collect_inactive_sve_regset (const struct regcache *regcache,
			     void *buf, size_t size, int vg_regnum)
{
  gdb_byte *header = (gdb_byte *) buf;
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  gdb_assert (buf != nullptr);
  gdb_assert (size >= SVE_CORE_DUMMY_SIZE);

  /* Zero out everything first.  */
  memset ((gdb_byte *) buf, 0, SVE_CORE_DUMMY_SIZE);

  /* BUF starts with a SVE header prior to the register dump.  */

  /* Dump the default size of an empty SVE payload.  */
  uint32_t real_size = SVE_CORE_DUMMY_SIZE;
  store_unsigned_integer (header + SVE_HEADER_SIZE_OFFSET,
			  SVE_HEADER_SIZE_LENGTH, byte_order, real_size);

  /* Dump a dummy max size.  */
  uint32_t max_size = SVE_CORE_DUMMY_MAX_SIZE;
  store_unsigned_integer (header + SVE_HEADER_MAX_SIZE_OFFSET,
			  SVE_HEADER_MAX_SIZE_LENGTH, byte_order, max_size);

  /* Dump the vector length.  */
  ULONGEST vg = 0;
  regcache->raw_collect (vg_regnum, &vg);
  uint16_t vl = sve_vl_from_vg (vg);
  store_unsigned_integer (header + SVE_HEADER_VL_OFFSET, SVE_HEADER_VL_LENGTH,
			  byte_order, vl);

  /* Dump the standard maximum vector length.  */
  uint16_t max_vl = SVE_CORE_DUMMY_MAX_VL;
  store_unsigned_integer (header + SVE_HEADER_MAX_VL_OFFSET,
			  SVE_HEADER_MAX_VL_LENGTH, byte_order,
			  max_vl);

  /* The rest of the fields are zero.  */
  uint16_t flags = SVE_CORE_DUMMY_FLAGS;
  store_unsigned_integer (header + SVE_HEADER_FLAGS_OFFSET,
			  SVE_HEADER_FLAGS_LENGTH, byte_order,
			  flags);
  uint16_t reserved = SVE_CORE_DUMMY_RESERVED;
  store_unsigned_integer (header + SVE_HEADER_RESERVED_OFFSET,
			  SVE_HEADER_RESERVED_LENGTH, byte_order, reserved);

  /* We are done with the header part of it.  Now dump the register state
     in the FPSIMD format.  */

  /* Dump the first 128 bits of each of the Z registers.  */
  header += AARCH64_SVE_CONTEXT_REGS_OFFSET;
  for (int i = 0; i < AARCH64_SVE_Z_REGS_NUM; i++)
    regcache->raw_collect_part (AARCH64_SVE_Z0_REGNUM + i, 0, V_REGISTER_SIZE,
				header + V_REGISTER_SIZE * i);

  /* Dump FPSR and FPCR.  */
  header += 32 * V_REGISTER_SIZE;
  regcache->raw_collect (AARCH64_FPSR_REGNUM, header);
  regcache->raw_collect (AARCH64_FPCR_REGNUM, header + 4);

  /* Dump two reserved empty fields of 4 bytes.  */
  header += 8;
  memset (header, 0, 8);

  /* We should have a FPSIMD-formatted register dump now.  */
}

/* Collect register REGNUM from REGCACHE to BUF, using the register
   map in REGSET.  If REGNUM is -1, do this for all registers in
   REGSET.  */

static void
collect_sve_regset (const struct regset *regset,
		    const struct regcache *regcache,
		    int regnum, void *buf, size_t size)
{
  gdb_byte *header = (gdb_byte *) buf;
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);
  uint64_t vq = tdep->vq;

  gdb_assert (buf != NULL);
  gdb_assert (size > SVE_HEADER_SIZE);

  /* BUF starts with a SVE header prior to the register dump.  */

  store_unsigned_integer (header + SVE_HEADER_SIZE_OFFSET,
			  SVE_HEADER_SIZE_LENGTH, byte_order, size);
  uint32_t max_size = SVE_CORE_DUMMY_MAX_SIZE;
  store_unsigned_integer (header + SVE_HEADER_MAX_SIZE_OFFSET,
			  SVE_HEADER_MAX_SIZE_LENGTH, byte_order, max_size);
  store_unsigned_integer (header + SVE_HEADER_VL_OFFSET, SVE_HEADER_VL_LENGTH,
			  byte_order, sve_vl_from_vq (vq));
  uint16_t max_vl = SVE_CORE_DUMMY_MAX_VL;
  store_unsigned_integer (header + SVE_HEADER_MAX_VL_OFFSET,
			  SVE_HEADER_MAX_VL_LENGTH, byte_order,
			  max_vl);
  uint16_t flags = SVE_HEADER_FLAG_SVE;
  store_unsigned_integer (header + SVE_HEADER_FLAGS_OFFSET,
			  SVE_HEADER_FLAGS_LENGTH, byte_order,
			  flags);
  uint16_t reserved = SVE_CORE_DUMMY_RESERVED;
  store_unsigned_integer (header + SVE_HEADER_RESERVED_OFFSET,
			  SVE_HEADER_RESERVED_LENGTH, byte_order, reserved);

  /* The SVE register dump follows.  */
  regcache->collect_regset (regset, regnum, (gdb_byte *) buf + SVE_HEADER_SIZE,
			    size - SVE_HEADER_SIZE);
}

/* Supply register REGNUM from BUF to REGCACHE, using the register map
   in REGSET.  If REGNUM is -1, do this for all registers in REGSET.
   If BUF is NULL, set the registers to "unavailable" status.  */

static void
aarch64_linux_supply_sve_regset (const struct regset *regset,
				 struct regcache *regcache,
				 int regnum, const void *buf, size_t size)
{
  struct gdbarch *gdbarch = regcache->arch ();
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  if (tdep->has_sme ())
    {
      ULONGEST svcr = 0;
      regcache->raw_collect (tdep->sme_svcr_regnum, &svcr);

      /* Is streaming mode enabled?  */
      if (svcr & SVCR_SM_BIT)
	/* If so, don't load SVE data from the SVE section.  The data to be
	   used is in the SSVE section.  */
	return;
    }
  /* If streaming mode is not enabled, load the SVE regcache data from the SVE
     section.  */
  supply_sve_regset (regset, regcache, regnum, buf, size);
}

/* Collect register REGNUM from REGCACHE to BUF, using the register
   map in REGSET.  If REGNUM is -1, do this for all registers in
   REGSET.  */

static void
aarch64_linux_collect_sve_regset (const struct regset *regset,
				  const struct regcache *regcache,
				  int regnum, void *buf, size_t size)
{
  struct gdbarch *gdbarch = regcache->arch ();
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);
  bool streaming_mode = false;

  if (tdep->has_sme ())
    {
      ULONGEST svcr = 0;
      regcache->raw_collect (tdep->sme_svcr_regnum, &svcr);

      /* Is streaming mode enabled?  */
      if (svcr & SVCR_SM_BIT)
	{
	  /* If so, don't dump SVE regcache data to the SVE section.  The SVE
	     data should be dumped to the SSVE section.  Dump an empty SVE
	     block instead.  */
	  streaming_mode = true;
	}
    }

  /* If streaming mode is not enabled or there is no SME support, dump the
     SVE regcache data to the SVE section.  */

  /* Check if we have an active SVE state (non-zero Z/P/FFR registers).
     If so, then we need to dump registers in the SVE format.

     Otherwise we should dump the registers in the FPSIMD format.  */
  if (sve_state_is_empty (regcache) || streaming_mode)
    collect_inactive_sve_regset (regcache, buf, size, AARCH64_SVE_VG_REGNUM);
  else
    collect_sve_regset (regset, regcache, regnum, buf, size);
}

/* Supply register REGNUM from BUF to REGCACHE, using the register map
   in REGSET.  If REGNUM is -1, do this for all registers in REGSET.
   If BUF is NULL, set the registers to "unavailable" status.  */

static void
aarch64_linux_supply_ssve_regset (const struct regset *regset,
				  struct regcache *regcache,
				  int regnum, const void *buf, size_t size)
{
  gdb_byte *header = (gdb_byte *) buf;
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  uint16_t flags = extract_unsigned_integer (header + SVE_HEADER_FLAGS_OFFSET,
					     SVE_HEADER_FLAGS_LENGTH,
					     byte_order);

  /* Since SVCR's bits are inferred from the data we have in the header of the
     SSVE section, we need to initialize it to zero first, so that it doesn't
     carry garbage data.  */
  ULONGEST svcr = 0;
  regcache->raw_supply (tdep->sme_svcr_regnum, &svcr);

  /* Is streaming mode enabled?  */
  if (flags & SVE_HEADER_FLAG_SVE)
    {
      /* Streaming mode is active, so flip the SM bit.  */
      svcr = SVCR_SM_BIT;
      regcache->raw_supply (tdep->sme_svcr_regnum, &svcr);

      /* Fetch the SVE data from the SSVE section.  */
      supply_sve_regset (regset, regcache, regnum, buf, size);
    }
}

/* Collect register REGNUM from REGCACHE to BUF, using the register
   map in REGSET.  If REGNUM is -1, do this for all registers in
   REGSET.  */

static void
aarch64_linux_collect_ssve_regset (const struct regset *regset,
				   const struct regcache *regcache,
				   int regnum, void *buf, size_t size)
{
  struct gdbarch *gdbarch = regcache->arch ();
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);
  ULONGEST svcr = 0;
  regcache->raw_collect (tdep->sme_svcr_regnum, &svcr);

  /* Is streaming mode enabled?  */
  if (svcr & SVCR_SM_BIT)
    {
      /* If so, dump SVE regcache data to the SSVE section.  */
      collect_sve_regset (regset, regcache, regnum, buf, size);
    }
  else
    {
      /* Otherwise dump an empty SVE block to the SSVE section with the
	 streaming vector length.  */
      collect_inactive_sve_regset (regcache, buf, size, tdep->sme_svg_regnum);
    }
}

/* Supply register REGNUM from BUF to REGCACHE, using the register map
   in REGSET.  If REGNUM is -1, do this for all registers in REGSET.
   If BUF is NULL, set the registers to "unavailable" status.  */

static void
aarch64_linux_supply_za_regset (const struct regset *regset,
				struct regcache *regcache,
				int regnum, const void *buf, size_t size)
{
  gdb_byte *header = (gdb_byte *) buf;
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  /* Handle an empty buffer.  */
  if (buf == nullptr)
    return regcache->supply_regset (regset, regnum, nullptr, size);

  if (size < SVE_HEADER_SIZE)
    error (_("ZA state header size (%s) invalid.  Should be at least %s."),
	   pulongest (size), pulongest (SVE_HEADER_SIZE));

  /* The ZA register note in a core file can have a couple of states:

     1 - Just the header without the payload.  This means that there is no
	 ZA data, and we should populate only SVCR and SVG registers on GDB's
	 side.  The ZA data should be marked as unavailable.

     2 - The header with an additional data payload.  This means there is
	 actual ZA data, and we should populate ZA, SVCR and SVG.  */

  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  /* Populate SVG.  */
  ULONGEST svg
    = sve_vg_from_vl (extract_unsigned_integer (header + SVE_HEADER_VL_OFFSET,
						SVE_HEADER_VL_LENGTH,
						byte_order));
  regcache->raw_supply (tdep->sme_svg_regnum, &svg);

  size_t data_size
    = extract_unsigned_integer (header + SVE_HEADER_SIZE_OFFSET,
				SVE_HEADER_SIZE_LENGTH, byte_order)
      - SVE_HEADER_SIZE;

  /* Populate SVCR.  */
  bool has_za_payload = (data_size > 0);
  ULONGEST svcr;
  regcache->raw_collect (tdep->sme_svcr_regnum, &svcr);

  /* If we have a ZA payload, enable bit 2 of SVCR, otherwise clear it.  This
     register gets updated by the SVE/SSVE-handling functions as well, as they
     report the SM bit 1.  */
  if (has_za_payload)
    svcr |= SVCR_ZA_BIT;
  else
    svcr &= ~SVCR_ZA_BIT;

  /* Update SVCR in the register buffer.  */
  regcache->raw_supply (tdep->sme_svcr_regnum, &svcr);

  /* Populate the register cache with ZA register contents, if we have any.  */
  buf = has_za_payload ? (gdb_byte *) buf + SVE_HEADER_SIZE : nullptr;

  size_t za_bytes = std::pow (sve_vl_from_vg (svg), 2);

  /* Update ZA in the register buffer.  */
  if (has_za_payload)
    {
      /* Check that the payload size is sane.  */
      if (size < SVE_HEADER_SIZE + za_bytes)
	{
	  error (_("ZA header + payload size (%s) invalid.  Should be at "
		   "least %s."),
		 pulongest (size), pulongest (SVE_HEADER_SIZE + za_bytes));
	}

      regcache->raw_supply (tdep->sme_za_regnum, buf);
    }
  else
    {
      gdb_byte za_zeroed[za_bytes];
      memset (za_zeroed, 0, za_bytes);
      regcache->raw_supply (tdep->sme_za_regnum, za_zeroed);
    }
}

/* Collect register REGNUM from REGCACHE to BUF, using the register
   map in REGSET.  If REGNUM is -1, do this for all registers in
   REGSET.  */

static void
aarch64_linux_collect_za_regset (const struct regset *regset,
				 const struct regcache *regcache,
				 int regnum, void *buf, size_t size)
{
  gdb_assert (buf != nullptr);

  /* Sanity check the dump size.  */
  gdb_assert (size >= SVE_HEADER_SIZE);

  /* The ZA register note in a core file can have a couple of states:

     1 - Just the header without the payload.  This means that there is no
	 ZA data, and we should dump just the header.

     2 - The header with an additional data payload.  This means there is
	 actual ZA data, and we should dump both the header and the ZA data
	 payload.  */

  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (regcache->arch ());

  /* Determine if we have ZA state from the SVCR register ZA bit.  */
  ULONGEST svcr;
  regcache->raw_collect (tdep->sme_svcr_regnum, &svcr);

  /* Check the ZA payload.  */
  bool has_za_payload = (svcr & SVCR_ZA_BIT) != 0;
  size = has_za_payload ? size : SVE_HEADER_SIZE;

  /* Write the size and max_size fields.  */
  gdb_byte *header = (gdb_byte *) buf;
  enum bfd_endian byte_order = gdbarch_byte_order (regcache->arch ());
  store_unsigned_integer (header + SVE_HEADER_SIZE_OFFSET,
			  SVE_HEADER_SIZE_LENGTH, byte_order, size);

  uint32_t max_size
    = SVE_HEADER_SIZE + std::pow (sve_vl_from_vq (tdep->sme_svq), 2);
  store_unsigned_integer (header + SVE_HEADER_MAX_SIZE_OFFSET,
			  SVE_HEADER_MAX_SIZE_LENGTH, byte_order, max_size);

  /* Output the other fields of the ZA header (vl, max_vl, flags and
     reserved).  */
  uint64_t svq = tdep->sme_svq;
  store_unsigned_integer (header + SVE_HEADER_VL_OFFSET, SVE_HEADER_VL_LENGTH,
			  byte_order, sve_vl_from_vq (svq));

  uint16_t max_vl = SVE_CORE_DUMMY_MAX_VL;
  store_unsigned_integer (header + SVE_HEADER_MAX_VL_OFFSET,
			  SVE_HEADER_MAX_VL_LENGTH, byte_order,
			  max_vl);

  uint16_t flags = SVE_CORE_DUMMY_FLAGS;
  store_unsigned_integer (header + SVE_HEADER_FLAGS_OFFSET,
			  SVE_HEADER_FLAGS_LENGTH, byte_order, flags);

  uint16_t reserved = SVE_CORE_DUMMY_RESERVED;
  store_unsigned_integer (header + SVE_HEADER_RESERVED_OFFSET,
			  SVE_HEADER_RESERVED_LENGTH, byte_order, reserved);

  buf = has_za_payload ? (gdb_byte *) buf + SVE_HEADER_SIZE : nullptr;

  /* Dump the register cache contents for the ZA register to the buffer.  */
  regcache->collect_regset (regset, regnum, (gdb_byte *) buf,
			    size - SVE_HEADER_SIZE);
}

/* Supply register REGNUM from BUF to REGCACHE, using the register map
   in REGSET.  If REGNUM is -1, do this for all registers in REGSET.
   If BUF is NULL, set the registers to "unavailable" status.  */

static void
aarch64_linux_supply_zt_regset (const struct regset *regset,
				struct regcache *regcache,
				int regnum, const void *buf, size_t size)
{
  /* Read the ZT register note from a core file into the register buffer.  */

  /* Make sure the buffer contains at least the expected amount of data we are
     supposed to get.  */
  gdb_assert (size >= AARCH64_SME2_ZT0_SIZE);

  /* Handle an empty buffer.  */
  if (buf == nullptr)
    return regcache->supply_regset (regset, regnum, nullptr, size);

  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (regcache->arch ());

  /* Supply the ZT0 register contents.  */
  regcache->raw_supply (tdep->sme2_zt0_regnum, buf);
}

/* Collect register REGNUM from REGCACHE to BUF, using the register
   map in REGSET.  If REGNUM is -1, do this for all registers in
   REGSET.  */

static void
aarch64_linux_collect_zt_regset (const struct regset *regset,
				 const struct regcache *regcache,
				 int regnum, void *buf, size_t size)
{
  /* Read the ZT register contents from the register buffer into the core
     file section.  */

  /* Make sure the buffer can hold the data we need to return.  */
  gdb_assert (size >= AARCH64_SME2_ZT0_SIZE);
  gdb_assert (buf != nullptr);

  aarch64_gdbarch_tdep *tdep
    = gdbarch_tdep<aarch64_gdbarch_tdep> (regcache->arch ());

  /* Dump the register cache contents for the ZT register to the buffer.  */
  regcache->collect_regset (regset, tdep->sme2_zt0_regnum, buf,
			    AARCH64_SME2_ZT0_SIZE);
}

/* Implement the "iterate_over_regset_sections" gdbarch method.  */

static void
aarch64_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					    iterate_over_regset_sections_cb *cb,
					    void *cb_data,
					    const struct regcache *regcache)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  cb (".reg", AARCH64_LINUX_SIZEOF_GREGSET, AARCH64_LINUX_SIZEOF_GREGSET,
      &aarch64_linux_gregset, NULL, cb_data);

  if (tdep->has_sve ())
    {
      /* Create this on the fly in order to handle vector register sizes.  */
      const struct regcache_map_entry sve_regmap[] =
	{
	  { 32, AARCH64_SVE_Z0_REGNUM, (int) (tdep->vq * 16) },
	  { 16, AARCH64_SVE_P0_REGNUM, (int) (tdep->vq * 16 / 8) },
	  { 1, AARCH64_SVE_FFR_REGNUM, (int) (tdep->vq * 16 / 8) },
	  { 1, AARCH64_FPSR_REGNUM, 4 },
	  { 1, AARCH64_FPCR_REGNUM, 4 },
	  { 0 }
	};

      const struct regset aarch64_linux_ssve_regset =
	{
	  sve_regmap,
	  aarch64_linux_supply_ssve_regset, aarch64_linux_collect_ssve_regset,
	  REGSET_VARIABLE_SIZE
	};

      /* If SME is supported in the core file, process the SSVE section first,
	 and the SVE section last.  This is because we need information from
	 the SSVE set to determine if streaming mode is active.  If streaming
	 mode is active, we need to extract the data from the SSVE section.

	 Otherwise, if streaming mode is not active, we fetch the data from the
	 SVE section.  */
      if (tdep->has_sme ())
	{
	  cb (".reg-aarch-ssve",
	      SVE_HEADER_SIZE
	      + regcache_map_entry_size (aarch64_linux_fpregmap),
	      SVE_HEADER_SIZE + regcache_map_entry_size (sve_regmap),
	      &aarch64_linux_ssve_regset, "SSVE registers", cb_data);
	}

      /* Handle the SVE register set.  */
      const struct regset aarch64_linux_sve_regset =
	{
	  sve_regmap,
	  aarch64_linux_supply_sve_regset, aarch64_linux_collect_sve_regset,
	  REGSET_VARIABLE_SIZE
	};

      cb (".reg-aarch-sve",
	  SVE_HEADER_SIZE + regcache_map_entry_size (aarch64_linux_fpregmap),
	  SVE_HEADER_SIZE + regcache_map_entry_size (sve_regmap),
	  &aarch64_linux_sve_regset, "SVE registers", cb_data);
    }
  else
    cb (".reg2", AARCH64_LINUX_SIZEOF_FPREGSET, AARCH64_LINUX_SIZEOF_FPREGSET,
	&aarch64_linux_fpregset, NULL, cb_data);

  if (tdep->has_sme ())
    {
      /* Setup the register set information for a ZA register set core
	 dump.  */

      /* Create this on the fly in order to handle the ZA register size.  */
      const struct regcache_map_entry za_regmap[] =
	{
	  { 1, tdep->sme_za_regnum,
	    (int) std::pow (sve_vl_from_vq (tdep->sme_svq), 2) },
	  { 0 }
	};

      const struct regset aarch64_linux_za_regset =
	{
	  za_regmap,
	  aarch64_linux_supply_za_regset, aarch64_linux_collect_za_regset,
	  REGSET_VARIABLE_SIZE
	};

      cb (".reg-aarch-za",
	  SVE_HEADER_SIZE,
	  SVE_HEADER_SIZE + std::pow (sve_vl_from_vq (tdep->sme_svq), 2),
	  &aarch64_linux_za_regset, "ZA register", cb_data);

      /* Handle SME2 (ZT) as well, which is only available if SME is
	 available.  */
      if (tdep->has_sme2 ())
	{
	  const struct regcache_map_entry zt_regmap[] =
	    {
	      { 1, tdep->sme2_zt0_regnum, AARCH64_SME2_ZT0_SIZE },
	      { 0 }
	    };

	  /* We set the register set size to REGSET_VARIABLE_SIZE here because
	     in the future there might be more ZT registers.  */
	  const struct regset aarch64_linux_zt_regset =
	    {
	      zt_regmap,
	      aarch64_linux_supply_zt_regset, aarch64_linux_collect_zt_regset,
	      REGSET_VARIABLE_SIZE
	    };

	  cb (".reg-aarch-zt",
	      AARCH64_SME2_ZT0_SIZE,
	      AARCH64_SME2_ZT0_SIZE,
	      &aarch64_linux_zt_regset, "ZT registers", cb_data);
	}
    }

  if (tdep->has_pauth ())
    {
      /* Create this on the fly in order to handle the variable location.  */
      const struct regcache_map_entry pauth_regmap[] =
	{
	  { 2, AARCH64_PAUTH_DMASK_REGNUM (tdep->pauth_reg_base), 8},
	  { 0 }
	};

      const struct regset aarch64_linux_pauth_regset =
	{
	  pauth_regmap, regcache_supply_regset, regcache_collect_regset
	};

      cb (".reg-aarch-pauth", AARCH64_LINUX_SIZEOF_PAUTH,
	  AARCH64_LINUX_SIZEOF_PAUTH, &aarch64_linux_pauth_regset,
	  "pauth registers", cb_data);
    }

  /* Handle MTE registers.  */
  if (tdep->has_mte ())
    {
      /* Create this on the fly in order to handle the variable location.  */
      const struct regcache_map_entry mte_regmap[] =
	{
	  { 1, tdep->mte_reg_base, 8},
	  { 0 }
	};

      const struct regset aarch64_linux_mte_regset =
	{
	  mte_regmap, regcache_supply_regset, regcache_collect_regset
	};

      cb (".reg-aarch-mte", AARCH64_LINUX_SIZEOF_MTE_REGSET,
	  AARCH64_LINUX_SIZEOF_MTE_REGSET, &aarch64_linux_mte_regset,
	  "MTE registers", cb_data);
    }

  /* Handle the TLS registers.  */
  if (tdep->has_tls ())
    {
      gdb_assert (tdep->tls_regnum_base != -1);
      gdb_assert (tdep->tls_register_count > 0);

      int sizeof_tls_regset
	= AARCH64_TLS_REGISTER_SIZE * tdep->tls_register_count;

      const struct regcache_map_entry tls_regmap[] =
	{
	  { tdep->tls_register_count, tdep->tls_regnum_base,
	    AARCH64_TLS_REGISTER_SIZE },
	  { 0 }
	};

      const struct regset aarch64_linux_tls_regset =
	{
	  tls_regmap, regcache_supply_regset, regcache_collect_regset,
	  REGSET_VARIABLE_SIZE
	};

      cb (".reg-aarch-tls", sizeof_tls_regset, sizeof_tls_regset,
	  &aarch64_linux_tls_regset, "TLS register", cb_data);
    }
}

/* Implement the "core_read_description" gdbarch method.  */

static const struct target_desc *
aarch64_linux_core_read_description (struct gdbarch *gdbarch,
				     struct target_ops *target, bfd *abfd)
{
  std::optional<gdb::byte_vector> auxv = target_read_auxv_raw (target);
  CORE_ADDR hwcap = linux_get_hwcap (auxv, target, gdbarch);
  CORE_ADDR hwcap2 = linux_get_hwcap2 (auxv, target, gdbarch);

  aarch64_features features;

  /* We need to extract the SVE data from the .reg-aarch-sve section or the
     .reg-aarch-ssve section depending on which one was active when the core
     file was generated.

     If the SSVE section contains SVE data, then it is considered active.
     Otherwise the SVE section is considered active.  This guarantees we will
     have the correct target description with the correct SVE vector
     length.  */
  features.vq = aarch64_linux_core_read_vq_from_sections (gdbarch, abfd);
  features.pauth = hwcap & AARCH64_HWCAP_PACA;
  features.mte = hwcap2 & HWCAP2_MTE;

  /* Handle the TLS section.  */
  asection *tls = bfd_get_section_by_name (abfd, ".reg-aarch-tls");
  if (tls != nullptr)
    {
      size_t size = bfd_section_size (tls);
      /* Convert the size to the number of actual registers, by
	 dividing by 8.  */
      features.tls = size / AARCH64_TLS_REGISTER_SIZE;
    }

  features.svq
    = aarch64_linux_core_read_vq (gdbarch, abfd, ".reg-aarch-za");

  /* Are the ZT registers available?  */
  if (bfd_get_section_by_name (abfd, ".reg-aarch-zt") != nullptr)
    {
      /* Check if ZA is also available, otherwise this is an invalid
	 combination.  */
      if (bfd_get_section_by_name (abfd, ".reg-aarch-za") != nullptr)
	features.sme2 = true;
      else
	warning (_("While reading core file sections, found ZT registers entry "
		   "but no ZA register entry.  The ZT contents will be "
		   "ignored"));
    }

  return aarch64_read_description (features);
}

/* Implementation of `gdbarch_stap_is_single_operand', as defined in
   gdbarch.h.  */

static int
aarch64_stap_is_single_operand (struct gdbarch *gdbarch, const char *s)
{
  return (*s == '#' || isdigit (*s) /* Literal number.  */
	  || *s == '[' /* Register indirection.  */
	  || isalpha (*s)); /* Register value.  */
}

/* This routine is used to parse a special token in AArch64's assembly.

   The special tokens parsed by it are:

      - Register displacement (e.g, [fp, #-8])

   It returns one if the special token has been parsed successfully,
   or zero if the current token is not considered special.  */

static expr::operation_up
aarch64_stap_parse_special_token (struct gdbarch *gdbarch,
				  struct stap_parse_info *p)
{
  if (*p->arg == '[')
    {
      /* Temporary holder for lookahead.  */
      const char *tmp = p->arg;
      char *endp;
      /* Used to save the register name.  */
      const char *start;
      int len;
      int got_minus = 0;
      long displacement;

      ++tmp;
      start = tmp;

      /* Register name.  */
      while (isalnum (*tmp))
	++tmp;

      if (*tmp != ',')
	return {};

      len = tmp - start;
      std::string regname (start, len);

      if (user_reg_map_name_to_regnum (gdbarch, regname.c_str (), len) == -1)
	error (_("Invalid register name `%s' on expression `%s'."),
	       regname.c_str (), p->saved_arg);

      ++tmp;
      tmp = skip_spaces (tmp);
      /* Now we expect a number.  It can begin with '#' or simply
	 a digit.  */
      if (*tmp == '#')
	++tmp;

      if (*tmp == '-')
	{
	  ++tmp;
	  got_minus = 1;
	}
      else if (*tmp == '+')
	++tmp;

      if (!isdigit (*tmp))
	return {};

      displacement = strtol (tmp, &endp, 10);
      tmp = endp;

      /* Skipping last `]'.  */
      if (*tmp++ != ']')
	return {};
      p->arg = tmp;

      using namespace expr;

      /* The displacement.  */
      struct type *long_type = builtin_type (gdbarch)->builtin_long;
      if (got_minus)
	displacement = -displacement;
      operation_up disp = make_operation<long_const_operation> (long_type,
								displacement);

      /* The register name.  */
      operation_up reg
	= make_operation<register_operation> (std::move (regname));

      operation_up sum
	= make_operation<add_operation> (std::move (reg), std::move (disp));

      /* Casting to the expected type.  */
      struct type *arg_ptr_type = lookup_pointer_type (p->arg_type);
      sum = make_operation<unop_cast_operation> (std::move (sum),
						 arg_ptr_type);
      return make_operation<unop_ind_operation> (std::move (sum));
    }
  return {};
}

/* AArch64 process record-replay constructs: syscall, signal etc.  */

static linux_record_tdep aarch64_linux_record_tdep;

/* Enum that defines the AArch64 linux specific syscall identifiers used for
   process record/replay.  */

enum aarch64_syscall {
  aarch64_sys_io_setup = 0,
  aarch64_sys_io_destroy = 1,
  aarch64_sys_io_submit = 2,
  aarch64_sys_io_cancel = 3,
  aarch64_sys_io_getevents = 4,
  aarch64_sys_setxattr = 5,
  aarch64_sys_lsetxattr = 6,
  aarch64_sys_fsetxattr = 7,
  aarch64_sys_getxattr = 8,
  aarch64_sys_lgetxattr = 9,
  aarch64_sys_fgetxattr = 10,
  aarch64_sys_listxattr = 11,
  aarch64_sys_llistxattr = 12,
  aarch64_sys_flistxattr = 13,
  aarch64_sys_removexattr = 14,
  aarch64_sys_lremovexattr = 15,
  aarch64_sys_fremovexattr = 16,
  aarch64_sys_getcwd = 17,
  aarch64_sys_lookup_dcookie = 18,
  aarch64_sys_eventfd2 = 19,
  aarch64_sys_epoll_create1 = 20,
  aarch64_sys_epoll_ctl = 21,
  aarch64_sys_epoll_pwait = 22,
  aarch64_sys_dup = 23,
  aarch64_sys_dup3 = 24,
  aarch64_sys_fcntl = 25,
  aarch64_sys_inotify_init1 = 26,
  aarch64_sys_inotify_add_watch = 27,
  aarch64_sys_inotify_rm_watch = 28,
  aarch64_sys_ioctl = 29,
  aarch64_sys_ioprio_set = 30,
  aarch64_sys_ioprio_get = 31,
  aarch64_sys_flock = 32,
  aarch64_sys_mknodat = 33,
  aarch64_sys_mkdirat = 34,
  aarch64_sys_unlinkat = 35,
  aarch64_sys_symlinkat = 36,
  aarch64_sys_linkat = 37,
  aarch64_sys_renameat = 38,
  aarch64_sys_umount2 = 39,
  aarch64_sys_mount = 40,
  aarch64_sys_pivot_root = 41,
  aarch64_sys_nfsservctl = 42,
  aarch64_sys_statfs = 43,
  aarch64_sys_fstatfs = 44,
  aarch64_sys_truncate = 45,
  aarch64_sys_ftruncate = 46,
  aarch64_sys_fallocate = 47,
  aarch64_sys_faccessat = 48,
  aarch64_sys_chdir = 49,
  aarch64_sys_fchdir = 50,
  aarch64_sys_chroot = 51,
  aarch64_sys_fchmod = 52,
  aarch64_sys_fchmodat = 53,
  aarch64_sys_fchownat = 54,
  aarch64_sys_fchown = 55,
  aarch64_sys_openat = 56,
  aarch64_sys_close = 57,
  aarch64_sys_vhangup = 58,
  aarch64_sys_pipe2 = 59,
  aarch64_sys_quotactl = 60,
  aarch64_sys_getdents64 = 61,
  aarch64_sys_lseek = 62,
  aarch64_sys_read = 63,
  aarch64_sys_write = 64,
  aarch64_sys_readv = 65,
  aarch64_sys_writev = 66,
  aarch64_sys_pread64 = 67,
  aarch64_sys_pwrite64 = 68,
  aarch64_sys_preadv = 69,
  aarch64_sys_pwritev = 70,
  aarch64_sys_sendfile = 71,
  aarch64_sys_pselect6 = 72,
  aarch64_sys_ppoll = 73,
  aarch64_sys_signalfd4 = 74,
  aarch64_sys_vmsplice = 75,
  aarch64_sys_splice = 76,
  aarch64_sys_tee = 77,
  aarch64_sys_readlinkat = 78,
  aarch64_sys_newfstatat = 79,
  aarch64_sys_fstat = 80,
  aarch64_sys_sync = 81,
  aarch64_sys_fsync = 82,
  aarch64_sys_fdatasync = 83,
  aarch64_sys_sync_file_range2 = 84,
  aarch64_sys_sync_file_range = 84,
  aarch64_sys_timerfd_create = 85,
  aarch64_sys_timerfd_settime = 86,
  aarch64_sys_timerfd_gettime = 87,
  aarch64_sys_utimensat = 88,
  aarch64_sys_acct = 89,
  aarch64_sys_capget = 90,
  aarch64_sys_capset = 91,
  aarch64_sys_personality = 92,
  aarch64_sys_exit = 93,
  aarch64_sys_exit_group = 94,
  aarch64_sys_waitid = 95,
  aarch64_sys_set_tid_address = 96,
  aarch64_sys_unshare = 97,
  aarch64_sys_futex = 98,
  aarch64_sys_set_robust_list = 99,
  aarch64_sys_get_robust_list = 100,
  aarch64_sys_nanosleep = 101,
  aarch64_sys_getitimer = 102,
  aarch64_sys_setitimer = 103,
  aarch64_sys_kexec_load = 104,
  aarch64_sys_init_module = 105,
  aarch64_sys_delete_module = 106,
  aarch64_sys_timer_create = 107,
  aarch64_sys_timer_gettime = 108,
  aarch64_sys_timer_getoverrun = 109,
  aarch64_sys_timer_settime = 110,
  aarch64_sys_timer_delete = 111,
  aarch64_sys_clock_settime = 112,
  aarch64_sys_clock_gettime = 113,
  aarch64_sys_clock_getres = 114,
  aarch64_sys_clock_nanosleep = 115,
  aarch64_sys_syslog = 116,
  aarch64_sys_ptrace = 117,
  aarch64_sys_sched_setparam = 118,
  aarch64_sys_sched_setscheduler = 119,
  aarch64_sys_sched_getscheduler = 120,
  aarch64_sys_sched_getparam = 121,
  aarch64_sys_sched_setaffinity = 122,
  aarch64_sys_sched_getaffinity = 123,
  aarch64_sys_sched_yield = 124,
  aarch64_sys_sched_get_priority_max = 125,
  aarch64_sys_sched_get_priority_min = 126,
  aarch64_sys_sched_rr_get_interval = 127,
  aarch64_sys_kill = 129,
  aarch64_sys_tkill = 130,
  aarch64_sys_tgkill = 131,
  aarch64_sys_sigaltstack = 132,
  aarch64_sys_rt_sigsuspend = 133,
  aarch64_sys_rt_sigaction = 134,
  aarch64_sys_rt_sigprocmask = 135,
  aarch64_sys_rt_sigpending = 136,
  aarch64_sys_rt_sigtimedwait = 137,
  aarch64_sys_rt_sigqueueinfo = 138,
  aarch64_sys_rt_sigreturn = 139,
  aarch64_sys_setpriority = 140,
  aarch64_sys_getpriority = 141,
  aarch64_sys_reboot = 142,
  aarch64_sys_setregid = 143,
  aarch64_sys_setgid = 144,
  aarch64_sys_setreuid = 145,
  aarch64_sys_setuid = 146,
  aarch64_sys_setresuid = 147,
  aarch64_sys_getresuid = 148,
  aarch64_sys_setresgid = 149,
  aarch64_sys_getresgid = 150,
  aarch64_sys_setfsuid = 151,
  aarch64_sys_setfsgid = 152,
  aarch64_sys_times = 153,
  aarch64_sys_setpgid = 154,
  aarch64_sys_getpgid = 155,
  aarch64_sys_getsid = 156,
  aarch64_sys_setsid = 157,
  aarch64_sys_getgroups = 158,
  aarch64_sys_setgroups = 159,
  aarch64_sys_uname = 160,
  aarch64_sys_sethostname = 161,
  aarch64_sys_setdomainname = 162,
  aarch64_sys_getrlimit = 163,
  aarch64_sys_setrlimit = 164,
  aarch64_sys_getrusage = 165,
  aarch64_sys_umask = 166,
  aarch64_sys_prctl = 167,
  aarch64_sys_getcpu = 168,
  aarch64_sys_gettimeofday = 169,
  aarch64_sys_settimeofday = 170,
  aarch64_sys_adjtimex = 171,
  aarch64_sys_getpid = 172,
  aarch64_sys_getppid = 173,
  aarch64_sys_getuid = 174,
  aarch64_sys_geteuid = 175,
  aarch64_sys_getgid = 176,
  aarch64_sys_getegid = 177,
  aarch64_sys_gettid = 178,
  aarch64_sys_sysinfo = 179,
  aarch64_sys_mq_open = 180,
  aarch64_sys_mq_unlink = 181,
  aarch64_sys_mq_timedsend = 182,
  aarch64_sys_mq_timedreceive = 183,
  aarch64_sys_mq_notify = 184,
  aarch64_sys_mq_getsetattr = 185,
  aarch64_sys_msgget = 186,
  aarch64_sys_msgctl = 187,
  aarch64_sys_msgrcv = 188,
  aarch64_sys_msgsnd = 189,
  aarch64_sys_semget = 190,
  aarch64_sys_semctl = 191,
  aarch64_sys_semtimedop = 192,
  aarch64_sys_semop = 193,
  aarch64_sys_shmget = 194,
  aarch64_sys_shmctl = 195,
  aarch64_sys_shmat = 196,
  aarch64_sys_shmdt = 197,
  aarch64_sys_socket = 198,
  aarch64_sys_socketpair = 199,
  aarch64_sys_bind = 200,
  aarch64_sys_listen = 201,
  aarch64_sys_accept = 202,
  aarch64_sys_connect = 203,
  aarch64_sys_getsockname = 204,
  aarch64_sys_getpeername = 205,
  aarch64_sys_sendto = 206,
  aarch64_sys_recvfrom = 207,
  aarch64_sys_setsockopt = 208,
  aarch64_sys_getsockopt = 209,
  aarch64_sys_shutdown = 210,
  aarch64_sys_sendmsg = 211,
  aarch64_sys_recvmsg = 212,
  aarch64_sys_readahead = 213,
  aarch64_sys_brk = 214,
  aarch64_sys_munmap = 215,
  aarch64_sys_mremap = 216,
  aarch64_sys_add_key = 217,
  aarch64_sys_request_key = 218,
  aarch64_sys_keyctl = 219,
  aarch64_sys_clone = 220,
  aarch64_sys_execve = 221,
  aarch64_sys_mmap = 222,
  aarch64_sys_fadvise64 = 223,
  aarch64_sys_swapon = 224,
  aarch64_sys_swapoff = 225,
  aarch64_sys_mprotect = 226,
  aarch64_sys_msync = 227,
  aarch64_sys_mlock = 228,
  aarch64_sys_munlock = 229,
  aarch64_sys_mlockall = 230,
  aarch64_sys_munlockall = 231,
  aarch64_sys_mincore = 232,
  aarch64_sys_madvise = 233,
  aarch64_sys_remap_file_pages = 234,
  aarch64_sys_mbind = 235,
  aarch64_sys_get_mempolicy = 236,
  aarch64_sys_set_mempolicy = 237,
  aarch64_sys_migrate_pages = 238,
  aarch64_sys_move_pages = 239,
  aarch64_sys_rt_tgsigqueueinfo = 240,
  aarch64_sys_perf_event_open = 241,
  aarch64_sys_accept4 = 242,
  aarch64_sys_recvmmsg = 243,
  aarch64_sys_wait4 = 260,
  aarch64_sys_prlimit64 = 261,
  aarch64_sys_fanotify_init = 262,
  aarch64_sys_fanotify_mark = 263,
  aarch64_sys_name_to_handle_at = 264,
  aarch64_sys_open_by_handle_at = 265,
  aarch64_sys_clock_adjtime = 266,
  aarch64_sys_syncfs = 267,
  aarch64_sys_setns = 268,
  aarch64_sys_sendmmsg = 269,
  aarch64_sys_process_vm_readv = 270,
  aarch64_sys_process_vm_writev = 271,
  aarch64_sys_kcmp = 272,
  aarch64_sys_finit_module = 273,
  aarch64_sys_sched_setattr = 274,
  aarch64_sys_sched_getattr = 275,
  aarch64_sys_getrandom = 278
};

/* aarch64_canonicalize_syscall maps syscall ids from the native AArch64
   linux set of syscall ids into a canonical set of syscall ids used by
   process record.  */

static enum gdb_syscall
aarch64_canonicalize_syscall (enum aarch64_syscall syscall_number)
{
#define SYSCALL_MAP(SYSCALL) case aarch64_sys_##SYSCALL: \
  return gdb_sys_##SYSCALL

#define UNSUPPORTED_SYSCALL_MAP(SYSCALL) case aarch64_sys_##SYSCALL: \
  return gdb_sys_no_syscall

  switch (syscall_number)
    {
      SYSCALL_MAP (io_setup);
      SYSCALL_MAP (io_destroy);
      SYSCALL_MAP (io_submit);
      SYSCALL_MAP (io_cancel);
      SYSCALL_MAP (io_getevents);

      SYSCALL_MAP (setxattr);
      SYSCALL_MAP (lsetxattr);
      SYSCALL_MAP (fsetxattr);
      SYSCALL_MAP (getxattr);
      SYSCALL_MAP (lgetxattr);
      SYSCALL_MAP (fgetxattr);
      SYSCALL_MAP (listxattr);
      SYSCALL_MAP (llistxattr);
      SYSCALL_MAP (flistxattr);
      SYSCALL_MAP (removexattr);
      SYSCALL_MAP (lremovexattr);
      SYSCALL_MAP (fremovexattr);
      SYSCALL_MAP (getcwd);
      SYSCALL_MAP (lookup_dcookie);
      SYSCALL_MAP (eventfd2);
      SYSCALL_MAP (epoll_create1);
      SYSCALL_MAP (epoll_ctl);
      SYSCALL_MAP (epoll_pwait);
      SYSCALL_MAP (dup);
      SYSCALL_MAP (dup3);
      SYSCALL_MAP (fcntl);
      SYSCALL_MAP (inotify_init1);
      SYSCALL_MAP (inotify_add_watch);
      SYSCALL_MAP (inotify_rm_watch);
      SYSCALL_MAP (ioctl);
      SYSCALL_MAP (ioprio_set);
      SYSCALL_MAP (ioprio_get);
      SYSCALL_MAP (flock);
      SYSCALL_MAP (mknodat);
      SYSCALL_MAP (mkdirat);
      SYSCALL_MAP (unlinkat);
      SYSCALL_MAP (symlinkat);
      SYSCALL_MAP (linkat);
      SYSCALL_MAP (renameat);
      UNSUPPORTED_SYSCALL_MAP (umount2);
      SYSCALL_MAP (mount);
      SYSCALL_MAP (pivot_root);
      SYSCALL_MAP (nfsservctl);
      SYSCALL_MAP (statfs);
      SYSCALL_MAP (truncate);
      SYSCALL_MAP (ftruncate);
      SYSCALL_MAP (fallocate);
      SYSCALL_MAP (faccessat);
      SYSCALL_MAP (fchdir);
      SYSCALL_MAP (chroot);
      SYSCALL_MAP (fchmod);
      SYSCALL_MAP (fchmodat);
      SYSCALL_MAP (fchownat);
      SYSCALL_MAP (fchown);
      SYSCALL_MAP (openat);
      SYSCALL_MAP (close);
      SYSCALL_MAP (vhangup);
      SYSCALL_MAP (pipe2);
      SYSCALL_MAP (quotactl);
      SYSCALL_MAP (getdents64);
      SYSCALL_MAP (lseek);
      SYSCALL_MAP (read);
      SYSCALL_MAP (write);
      SYSCALL_MAP (readv);
      SYSCALL_MAP (writev);
      SYSCALL_MAP (pread64);
      SYSCALL_MAP (pwrite64);
      UNSUPPORTED_SYSCALL_MAP (preadv);
      UNSUPPORTED_SYSCALL_MAP (pwritev);
      SYSCALL_MAP (sendfile);
      SYSCALL_MAP (pselect6);
      SYSCALL_MAP (ppoll);
      UNSUPPORTED_SYSCALL_MAP (signalfd4);
      SYSCALL_MAP (vmsplice);
      SYSCALL_MAP (splice);
      SYSCALL_MAP (tee);
      SYSCALL_MAP (readlinkat);
      SYSCALL_MAP (newfstatat);

      SYSCALL_MAP (fstat);
      SYSCALL_MAP (sync);
      SYSCALL_MAP (fsync);
      SYSCALL_MAP (fdatasync);
      SYSCALL_MAP (sync_file_range);
      UNSUPPORTED_SYSCALL_MAP (timerfd_create);
      UNSUPPORTED_SYSCALL_MAP (timerfd_settime);
      UNSUPPORTED_SYSCALL_MAP (timerfd_gettime);
      UNSUPPORTED_SYSCALL_MAP (utimensat);
      SYSCALL_MAP (acct);
      SYSCALL_MAP (capget);
      SYSCALL_MAP (capset);
      SYSCALL_MAP (personality);
      SYSCALL_MAP (exit);
      SYSCALL_MAP (exit_group);
      SYSCALL_MAP (waitid);
      SYSCALL_MAP (set_tid_address);
      SYSCALL_MAP (unshare);
      SYSCALL_MAP (futex);
      SYSCALL_MAP (set_robust_list);
      SYSCALL_MAP (get_robust_list);
      SYSCALL_MAP (nanosleep);

      SYSCALL_MAP (getitimer);
      SYSCALL_MAP (setitimer);
      SYSCALL_MAP (kexec_load);
      SYSCALL_MAP (init_module);
      SYSCALL_MAP (delete_module);
      SYSCALL_MAP (timer_create);
      SYSCALL_MAP (timer_settime);
      SYSCALL_MAP (timer_gettime);
      SYSCALL_MAP (timer_getoverrun);
      SYSCALL_MAP (timer_delete);
      SYSCALL_MAP (clock_settime);
      SYSCALL_MAP (clock_gettime);
      SYSCALL_MAP (clock_getres);
      SYSCALL_MAP (clock_nanosleep);
      SYSCALL_MAP (syslog);
      SYSCALL_MAP (ptrace);
      SYSCALL_MAP (sched_setparam);
      SYSCALL_MAP (sched_setscheduler);
      SYSCALL_MAP (sched_getscheduler);
      SYSCALL_MAP (sched_getparam);
      SYSCALL_MAP (sched_setaffinity);
      SYSCALL_MAP (sched_getaffinity);
      SYSCALL_MAP (sched_yield);
      SYSCALL_MAP (sched_get_priority_max);
      SYSCALL_MAP (sched_get_priority_min);
      SYSCALL_MAP (sched_rr_get_interval);
      SYSCALL_MAP (kill);
      SYSCALL_MAP (tkill);
      SYSCALL_MAP (tgkill);
      SYSCALL_MAP (sigaltstack);
      SYSCALL_MAP (rt_sigsuspend);
      SYSCALL_MAP (rt_sigaction);
      SYSCALL_MAP (rt_sigprocmask);
      SYSCALL_MAP (rt_sigpending);
      SYSCALL_MAP (rt_sigtimedwait);
      SYSCALL_MAP (rt_sigqueueinfo);
      SYSCALL_MAP (rt_sigreturn);
      SYSCALL_MAP (setpriority);
      SYSCALL_MAP (getpriority);
      SYSCALL_MAP (reboot);
      SYSCALL_MAP (setregid);
      SYSCALL_MAP (setgid);
      SYSCALL_MAP (setreuid);
      SYSCALL_MAP (setuid);
      SYSCALL_MAP (setresuid);
      SYSCALL_MAP (getresuid);
      SYSCALL_MAP (setresgid);
      SYSCALL_MAP (getresgid);
      SYSCALL_MAP (setfsuid);
      SYSCALL_MAP (setfsgid);
      SYSCALL_MAP (times);
      SYSCALL_MAP (setpgid);
      SYSCALL_MAP (getpgid);
      SYSCALL_MAP (getsid);
      SYSCALL_MAP (setsid);
      SYSCALL_MAP (getgroups);
      SYSCALL_MAP (setgroups);
      SYSCALL_MAP (uname);
      SYSCALL_MAP (sethostname);
      SYSCALL_MAP (setdomainname);
      SYSCALL_MAP (getrlimit);
      SYSCALL_MAP (setrlimit);
      SYSCALL_MAP (getrusage);
      SYSCALL_MAP (umask);
      SYSCALL_MAP (prctl);
      SYSCALL_MAP (getcpu);
      SYSCALL_MAP (gettimeofday);
      SYSCALL_MAP (settimeofday);
      SYSCALL_MAP (adjtimex);
      SYSCALL_MAP (getpid);
      SYSCALL_MAP (getppid);
      SYSCALL_MAP (getuid);
      SYSCALL_MAP (geteuid);
      SYSCALL_MAP (getgid);
      SYSCALL_MAP (getegid);
      SYSCALL_MAP (gettid);
      SYSCALL_MAP (sysinfo);
      SYSCALL_MAP (mq_open);
      SYSCALL_MAP (mq_unlink);
      SYSCALL_MAP (mq_timedsend);
      SYSCALL_MAP (mq_timedreceive);
      SYSCALL_MAP (mq_notify);
      SYSCALL_MAP (mq_getsetattr);
      SYSCALL_MAP (msgget);
      SYSCALL_MAP (msgctl);
      SYSCALL_MAP (msgrcv);
      SYSCALL_MAP (msgsnd);
      SYSCALL_MAP (semget);
      SYSCALL_MAP (semctl);
      SYSCALL_MAP (semtimedop);
      SYSCALL_MAP (semop);
      SYSCALL_MAP (shmget);
      SYSCALL_MAP (shmctl);
      SYSCALL_MAP (shmat);
      SYSCALL_MAP (shmdt);
      SYSCALL_MAP (socket);
      SYSCALL_MAP (socketpair);
      SYSCALL_MAP (bind);
      SYSCALL_MAP (listen);
      SYSCALL_MAP (accept);
      SYSCALL_MAP (connect);
      SYSCALL_MAP (getsockname);
      SYSCALL_MAP (getpeername);
      SYSCALL_MAP (sendto);
      SYSCALL_MAP (recvfrom);
      SYSCALL_MAP (setsockopt);
      SYSCALL_MAP (getsockopt);
      SYSCALL_MAP (shutdown);
      SYSCALL_MAP (sendmsg);
      SYSCALL_MAP (recvmsg);
      SYSCALL_MAP (readahead);
      SYSCALL_MAP (brk);
      SYSCALL_MAP (munmap);
      SYSCALL_MAP (mremap);
      SYSCALL_MAP (add_key);
      SYSCALL_MAP (request_key);
      SYSCALL_MAP (keyctl);
      SYSCALL_MAP (clone);
      SYSCALL_MAP (execve);

    case aarch64_sys_mmap:
      return gdb_sys_mmap2;

      SYSCALL_MAP (fadvise64);
      SYSCALL_MAP (swapon);
      SYSCALL_MAP (swapoff);
      SYSCALL_MAP (mprotect);
      SYSCALL_MAP (msync);
      SYSCALL_MAP (mlock);
      SYSCALL_MAP (munlock);
      SYSCALL_MAP (mlockall);
      SYSCALL_MAP (munlockall);
      SYSCALL_MAP (mincore);
      SYSCALL_MAP (madvise);
      SYSCALL_MAP (remap_file_pages);
      SYSCALL_MAP (mbind);
      SYSCALL_MAP (get_mempolicy);
      SYSCALL_MAP (set_mempolicy);
      SYSCALL_MAP (migrate_pages);
      SYSCALL_MAP (move_pages);
      UNSUPPORTED_SYSCALL_MAP (rt_tgsigqueueinfo);
      UNSUPPORTED_SYSCALL_MAP (perf_event_open);
      UNSUPPORTED_SYSCALL_MAP (accept4);
      UNSUPPORTED_SYSCALL_MAP (recvmmsg);

      SYSCALL_MAP (wait4);

      UNSUPPORTED_SYSCALL_MAP (prlimit64);
      UNSUPPORTED_SYSCALL_MAP (fanotify_init);
      UNSUPPORTED_SYSCALL_MAP (fanotify_mark);
      UNSUPPORTED_SYSCALL_MAP (name_to_handle_at);
      UNSUPPORTED_SYSCALL_MAP (open_by_handle_at);
      UNSUPPORTED_SYSCALL_MAP (clock_adjtime);
      UNSUPPORTED_SYSCALL_MAP (syncfs);
      UNSUPPORTED_SYSCALL_MAP (setns);
      UNSUPPORTED_SYSCALL_MAP (sendmmsg);
      UNSUPPORTED_SYSCALL_MAP (process_vm_readv);
      UNSUPPORTED_SYSCALL_MAP (process_vm_writev);
      UNSUPPORTED_SYSCALL_MAP (kcmp);
      UNSUPPORTED_SYSCALL_MAP (finit_module);
      UNSUPPORTED_SYSCALL_MAP (sched_setattr);
      UNSUPPORTED_SYSCALL_MAP (sched_getattr);
      SYSCALL_MAP (getrandom);
  default:
    return gdb_sys_no_syscall;
  }
}

/* Retrieve the syscall number at a ptrace syscall-stop, either on syscall entry
   or exit.  Return -1 upon error.  */

static LONGEST
aarch64_linux_get_syscall_number (struct gdbarch *gdbarch, thread_info *thread)
{
  struct regcache *regs = get_thread_regcache (thread);
  LONGEST ret;

  /* Get the system call number from register x8.  */
  regs->cooked_read (AARCH64_X0_REGNUM + 8, &ret);

  /* On exit from a successful execve, we will be in a new process and all the
     registers will be cleared - x0 to x30 will be 0, except for a 1 in x7.
     This function will only ever get called when stopped at the entry or exit
     of a syscall, so by checking for 0 in x0 (arg0/retval), x1 (arg1), x8
     (syscall), x29 (FP) and x30 (LR) we can infer:
     1) Either inferior is at exit from successful execve.
     2) Or inferior is at entry to a call to io_setup with invalid arguments and
	a corrupted FP and LR.
     It should be safe enough to assume case 1.  */
  if (ret == 0)
    {
      LONGEST x1 = -1, fp = -1, lr = -1;
      regs->cooked_read (AARCH64_X0_REGNUM + 1, &x1);
      regs->cooked_read (AARCH64_FP_REGNUM, &fp);
      regs->cooked_read (AARCH64_LR_REGNUM, &lr);
      if (x1 == 0 && fp ==0 && lr == 0)
	return aarch64_sys_execve;
    }

  return ret;
}

/* Record all registers but PC register for process-record.  */

static int
aarch64_all_but_pc_registers_record (struct regcache *regcache)
{
  int i;

  for (i = AARCH64_X0_REGNUM; i < AARCH64_PC_REGNUM; i++)
    if (record_full_arch_list_add_reg (regcache, i))
      return -1;

  if (record_full_arch_list_add_reg (regcache, AARCH64_CPSR_REGNUM))
    return -1;

  return 0;
}

/* Handler for aarch64 system call instruction recording.  */

static int
aarch64_linux_syscall_record (struct regcache *regcache,
			      unsigned long svc_number)
{
  int ret = 0;
  enum gdb_syscall syscall_gdb;

  syscall_gdb =
    aarch64_canonicalize_syscall ((enum aarch64_syscall) svc_number);

  if (syscall_gdb < 0)
    {
      gdb_printf (gdb_stderr,
		  _("Process record and replay target doesn't "
		    "support syscall number %s\n"),
		  plongest (svc_number));
      return -1;
    }

  if (syscall_gdb == gdb_sys_sigreturn
      || syscall_gdb == gdb_sys_rt_sigreturn)
   {
     if (aarch64_all_but_pc_registers_record (regcache))
       return -1;
     return 0;
   }

  ret = record_linux_system_call (syscall_gdb, regcache,
				  &aarch64_linux_record_tdep);
  if (ret != 0)
    return ret;

  /* Record the return value of the system call.  */
  if (record_full_arch_list_add_reg (regcache, AARCH64_X0_REGNUM))
    return -1;
  /* Record LR.  */
  if (record_full_arch_list_add_reg (regcache, AARCH64_LR_REGNUM))
    return -1;
  /* Record CPSR.  */
  if (record_full_arch_list_add_reg (regcache, AARCH64_CPSR_REGNUM))
    return -1;

  return 0;
}

/* Implement the "gcc_target_options" gdbarch method.  */

static std::string
aarch64_linux_gcc_target_options (struct gdbarch *gdbarch)
{
  /* GCC doesn't know "-m64".  */
  return {};
}

/* Helper to get the allocation tag from a 64-bit ADDRESS.

   Return the allocation tag if successful and nullopt otherwise.  */

static std::optional<CORE_ADDR>
aarch64_mte_get_atag (CORE_ADDR address)
{
  gdb::byte_vector tags;

  /* Attempt to fetch the allocation tag.  */
  if (!target_fetch_memtags (address, 1, tags,
			     static_cast<int> (memtag_type::allocation)))
    return {};

  /* Only one tag should've been returned.  Make sure we got exactly that.  */
  if (tags.size () != 1)
    error (_("Target returned an unexpected number of tags."));

  /* Although our tags are 4 bits in size, they are stored in a
     byte.  */
  return tags[0];
}

/* Implement the tagged_address_p gdbarch method.  */

static bool
aarch64_linux_tagged_address_p (struct gdbarch *gdbarch, struct value *address)
{
  gdb_assert (address != nullptr);

  CORE_ADDR addr = value_as_address (address);

  /* Remove the top byte for the memory range check.  */
  addr = gdbarch_remove_non_address_bits (gdbarch, addr);

  /* Check if the page that contains ADDRESS is mapped with PROT_MTE.  */
  if (!linux_address_in_memtag_page (addr))
    return false;

  /* We have a valid tag in the top byte of the 64-bit address.  */
  return true;
}

/* Implement the memtag_matches_p gdbarch method.  */

static bool
aarch64_linux_memtag_matches_p (struct gdbarch *gdbarch,
				struct value *address)
{
  gdb_assert (address != nullptr);

  /* Make sure we are dealing with a tagged address to begin with.  */
  if (!aarch64_linux_tagged_address_p (gdbarch, address))
    return true;

  CORE_ADDR addr = value_as_address (address);

  /* Fetch the allocation tag for ADDRESS.  */
  std::optional<CORE_ADDR> atag
    = aarch64_mte_get_atag (gdbarch_remove_non_address_bits (gdbarch, addr));

  if (!atag.has_value ())
    return true;

  /* Fetch the logical tag for ADDRESS.  */
  gdb_byte ltag = aarch64_mte_get_ltag (addr);

  /* Are the tags the same?  */
  return ltag == *atag;
}

/* Implement the set_memtags gdbarch method.  */

static bool
aarch64_linux_set_memtags (struct gdbarch *gdbarch, struct value *address,
			   size_t length, const gdb::byte_vector &tags,
			   memtag_type tag_type)
{
  gdb_assert (!tags.empty ());
  gdb_assert (address != nullptr);

  CORE_ADDR addr = value_as_address (address);

  /* Set the logical tag or the allocation tag.  */
  if (tag_type == memtag_type::logical)
    {
      /* When setting logical tags, we don't care about the length, since
	 we are only setting a single logical tag.  */
      addr = aarch64_mte_set_ltag (addr, tags[0]);

      /* Update the value's content with the tag.  */
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      gdb_byte *srcbuf = address->contents_raw ().data ();
      store_unsigned_integer (srcbuf, sizeof (addr), byte_order, addr);
    }
  else
    {
      /* Remove the top byte.  */
      addr = gdbarch_remove_non_address_bits (gdbarch, addr);

      /* Make sure we are dealing with a tagged address to begin with.  */
      if (!aarch64_linux_tagged_address_p (gdbarch, address))
	return false;

      /* With G being the number of tag granules and N the number of tags
	 passed in, we can have the following cases:

	 1 - G == N: Store all the N tags to memory.

	 2 - G < N : Warn about having more tags than granules, but write G
		     tags.

	 3 - G > N : This is a "fill tags" operation.  We should use the tags
		     as a pattern to fill the granules repeatedly until we have
		     written G tags to memory.
      */

      size_t g = aarch64_mte_get_tag_granules (addr, length,
					       AARCH64_MTE_GRANULE_SIZE);
      size_t n = tags.size ();

      if (g < n)
	warning (_("Got more tags than memory granules.  Tags will be "
		   "truncated."));
      else if (g > n)
	warning (_("Using tag pattern to fill memory range."));

      if (!target_store_memtags (addr, length, tags,
				 static_cast<int> (memtag_type::allocation)))
	return false;
    }
  return true;
}

/* Implement the get_memtag gdbarch method.  */

static struct value *
aarch64_linux_get_memtag (struct gdbarch *gdbarch, struct value *address,
			  memtag_type tag_type)
{
  gdb_assert (address != nullptr);

  CORE_ADDR addr = value_as_address (address);
  CORE_ADDR tag = 0;

  /* Get the logical tag or the allocation tag.  */
  if (tag_type == memtag_type::logical)
    tag = aarch64_mte_get_ltag (addr);
  else
    {
      /* Make sure we are dealing with a tagged address to begin with.  */
      if (!aarch64_linux_tagged_address_p (gdbarch, address))
	return nullptr;

      /* Remove the top byte.  */
      addr = gdbarch_remove_non_address_bits (gdbarch, addr);
      std::optional<CORE_ADDR> atag = aarch64_mte_get_atag (addr);

      if (!atag.has_value ())
	return nullptr;

      tag = *atag;
    }

  /* Convert the tag to a value.  */
  return value_from_ulongest (builtin_type (gdbarch)->builtin_unsigned_int,
			      tag);
}

/* Implement the memtag_to_string gdbarch method.  */

static std::string
aarch64_linux_memtag_to_string (struct gdbarch *gdbarch, struct value *tag_value)
{
  if (tag_value == nullptr)
    return "";

  CORE_ADDR tag = value_as_address (tag_value);

  return string_printf ("0x%s", phex_nz (tag, sizeof (tag)));
}

/* AArch64 Linux implementation of the report_signal_info gdbarch
   hook.  Displays information about possible memory tag violations.  */

static void
aarch64_linux_report_signal_info (struct gdbarch *gdbarch,
				  struct ui_out *uiout,
				  enum gdb_signal siggnal)
{
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  if (!tdep->has_mte () || siggnal != GDB_SIGNAL_SEGV)
    return;

  CORE_ADDR fault_addr = 0;
  long si_code = 0;

  try
    {
      /* Sigcode tells us if the segfault is actually a memory tag
	 violation.  */
      si_code = parse_and_eval_long ("$_siginfo.si_code");

      fault_addr
	= parse_and_eval_long ("$_siginfo._sifields._sigfault.si_addr");
    }
  catch (const gdb_exception_error &exception)
    {
      exception_print (gdb_stderr, exception);
      return;
    }

  /* If this is not a memory tag violation, just return.  */
  if (si_code != SEGV_MTEAERR && si_code != SEGV_MTESERR)
    return;

  uiout->text ("\n");

  uiout->field_string ("sigcode-meaning", _("Memory tag violation"));

  /* For synchronous faults, show additional information.  */
  if (si_code == SEGV_MTESERR)
    {
      uiout->text (_(" while accessing address "));
      uiout->field_core_addr ("fault-addr", gdbarch, fault_addr);
      uiout->text ("\n");

      std::optional<CORE_ADDR> atag
	= aarch64_mte_get_atag (gdbarch_remove_non_address_bits (gdbarch,
								 fault_addr));
      gdb_byte ltag = aarch64_mte_get_ltag (fault_addr);

      if (!atag.has_value ())
	uiout->text (_("Allocation tag unavailable"));
      else
	{
	  uiout->text (_("Allocation tag "));
	  uiout->field_string ("allocation-tag", hex_string (*atag));
	  uiout->text ("\n");
	  uiout->text (_("Logical tag "));
	  uiout->field_string ("logical-tag", hex_string (ltag));
	}
    }
  else
    {
      uiout->text ("\n");
      uiout->text (_("Fault address unavailable"));
    }
}

/* AArch64 Linux implementation of the gdbarch_create_memtag_section hook.  */

static asection *
aarch64_linux_create_memtag_section (struct gdbarch *gdbarch, bfd *obfd,
				     CORE_ADDR address, size_t size)
{
  gdb_assert (obfd != nullptr);
  gdb_assert (size > 0);

  /* Create the section and associated program header.

     Make sure the section's flags has SEC_HAS_CONTENTS, otherwise BFD will
     refuse to write data to this section.  */
  asection *mte_section
    = bfd_make_section_anyway_with_flags (obfd, "memtag", SEC_HAS_CONTENTS);

  if (mte_section == nullptr)
    return nullptr;

  bfd_set_section_vma (mte_section, address);
  /* The size of the memory range covered by the memory tags.  We reuse the
     section's rawsize field for this purpose.  */
  mte_section->rawsize = size;

  /* Fetch the number of tags we need to save.  */
  size_t tags_count
    = aarch64_mte_get_tag_granules (address, size, AARCH64_MTE_GRANULE_SIZE);
  /* Tags are stored packed as 2 tags per byte.  */
  bfd_set_section_size (mte_section, (tags_count + 1) >> 1);
  /* Store program header information.  */
  bfd_record_phdr (obfd, PT_AARCH64_MEMTAG_MTE, 1, 0, 0, 0, 0, 0, 1,
		   &mte_section);

  return mte_section;
}

/* Maximum number of tags to request.  */
#define MAX_TAGS_TO_TRANSFER 1024

/* AArch64 Linux implementation of the gdbarch_fill_memtag_section hook.  */

static bool
aarch64_linux_fill_memtag_section (struct gdbarch *gdbarch, asection *osec)
{
  /* We only handle MTE tags for now.  */

  size_t segment_size = osec->rawsize;
  CORE_ADDR start_address = bfd_section_vma (osec);
  CORE_ADDR end_address = start_address + segment_size;

  /* Figure out how many tags we need to store in this memory range.  */
  size_t granules = aarch64_mte_get_tag_granules (start_address, segment_size,
						  AARCH64_MTE_GRANULE_SIZE);

  /* If there are no tag granules to fetch, just return.  */
  if (granules == 0)
    return true;

  CORE_ADDR address = start_address;

  /* Vector of tags.  */
  gdb::byte_vector tags;

  while (granules > 0)
    {
      /* Transfer tags in chunks.  */
      gdb::byte_vector tags_read;
      size_t xfer_len
	= ((granules >= MAX_TAGS_TO_TRANSFER)
	  ? MAX_TAGS_TO_TRANSFER * AARCH64_MTE_GRANULE_SIZE
	  : granules * AARCH64_MTE_GRANULE_SIZE);

      if (!target_fetch_memtags (address, xfer_len, tags_read,
				 static_cast<int> (memtag_type::allocation)))
	{
	  warning (_("Failed to read MTE tags from memory range [%s,%s)."),
		     phex_nz (start_address, sizeof (start_address)),
		     phex_nz (end_address, sizeof (end_address)));
	  return false;
	}

      /* Transfer over the tags that have been read.  */
      tags.insert (tags.end (), tags_read.begin (), tags_read.end ());

      /* Adjust the remaining granules and starting address.  */
      granules -= tags_read.size ();
      address += tags_read.size () * AARCH64_MTE_GRANULE_SIZE;
    }

  /* Pack the MTE tag bits.  */
  aarch64_mte_pack_tags (tags);

  if (!bfd_set_section_contents (osec->owner, osec, tags.data (),
				 0, tags.size ()))
    {
      warning (_("Failed to write %s bytes of corefile memory "
		 "tag content (%s)."),
	       pulongest (tags.size ()),
	       bfd_errmsg (bfd_get_error ()));
    }
  return true;
}

/* AArch64 Linux implementation of the gdbarch_decode_memtag_section
   hook.  Decode a memory tag section and return the requested tags.

   The section is guaranteed to cover the [ADDRESS, ADDRESS + length)
   range.  */

static gdb::byte_vector
aarch64_linux_decode_memtag_section (struct gdbarch *gdbarch,
				     bfd_section *section,
				     int type,
				     CORE_ADDR address, size_t length)
{
  gdb_assert (section != nullptr);

  /* The requested address must not be less than section->vma.  */
  gdb_assert (section->vma <= address);

  /* Figure out how many tags we need to fetch in this memory range.  */
  size_t granules = aarch64_mte_get_tag_granules (address, length,
						  AARCH64_MTE_GRANULE_SIZE);
  /* Sanity check.  */
  gdb_assert (granules > 0);

  /* Fetch the total number of tags in the range [VMA, address + length).  */
  size_t granules_from_vma
    = aarch64_mte_get_tag_granules (section->vma,
				    address - section->vma + length,
				    AARCH64_MTE_GRANULE_SIZE);

  /* Adjust the tags vector to contain the exact number of packed bytes.  */
  gdb::byte_vector tags (((granules - 1) >> 1) + 1);

  /* Figure out the starting offset into the packed tags data.  */
  file_ptr offset = ((granules_from_vma - granules) >> 1);

  if (!bfd_get_section_contents (section->owner, section, tags.data (),
				 offset, tags.size ()))
    error (_("Couldn't read contents from memtag section."));

  /* At this point, the tags are packed 2 per byte.  Unpack them before
     returning.  */
  bool skip_first = ((granules_from_vma - granules) % 2) != 0;
  aarch64_mte_unpack_tags (tags, skip_first);

  /* Resize to the exact number of tags that was requested.  */
  tags.resize (granules);

  return tags;
}

/* AArch64 Linux implementation of the
   gdbarch_use_target_description_from_corefile_notes hook.  */

static bool
aarch64_use_target_description_from_corefile_notes (gdbarch *gdbarch,
						    bfd *obfd)
{
  /* Sanity check.  */
  gdb_assert (obfd != nullptr);

  /* If the corefile contains any SVE or SME register data, we don't want to
     use the target description note, as it may be incorrect.

     Currently the target description note contains a potentially incorrect
     target description if the originating program changed the SVE or SME
     vector lengths mid-execution.

     Once we support per-thread target description notes in the corefiles, we
     can always trust those notes whenever they are available.  */
  if (bfd_get_section_by_name (obfd, ".reg-aarch-sve") != nullptr
      || bfd_get_section_by_name (obfd, ".reg-aarch-za") != nullptr
      || bfd_get_section_by_name (obfd, ".reg-aarch-zt") != nullptr)
    return false;

  return true;
}

static void
aarch64_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  static const char *const stap_integer_prefixes[] = { "#", "", NULL };
  static const char *const stap_register_prefixes[] = { "", NULL };
  static const char *const stap_register_indirection_prefixes[] = { "[",
								    NULL };
  static const char *const stap_register_indirection_suffixes[] = { "]",
								    NULL };
  aarch64_gdbarch_tdep *tdep = gdbarch_tdep<aarch64_gdbarch_tdep> (gdbarch);

  tdep->lowest_pc = 0x8000;

  linux_init_abi (info, gdbarch, 1);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
					 linux_lp64_fetch_link_map_offsets);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  /* Shared library handling.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  tramp_frame_prepend_unwinder (gdbarch, &aarch64_linux_rt_sigframe);

  /* Enable longjmp.  */
  tdep->jb_pc = 11;

  set_gdbarch_iterate_over_regset_sections
    (gdbarch, aarch64_linux_iterate_over_regset_sections);
  set_gdbarch_core_read_description
    (gdbarch, aarch64_linux_core_read_description);

  /* SystemTap related.  */
  set_gdbarch_stap_integer_prefixes (gdbarch, stap_integer_prefixes);
  set_gdbarch_stap_register_prefixes (gdbarch, stap_register_prefixes);
  set_gdbarch_stap_register_indirection_prefixes (gdbarch,
					    stap_register_indirection_prefixes);
  set_gdbarch_stap_register_indirection_suffixes (gdbarch,
					    stap_register_indirection_suffixes);
  set_gdbarch_stap_is_single_operand (gdbarch, aarch64_stap_is_single_operand);
  set_gdbarch_stap_parse_special_token (gdbarch,
					aarch64_stap_parse_special_token);

  /* Reversible debugging, process record.  */
  set_gdbarch_process_record (gdbarch, aarch64_process_record);
  /* Syscall record.  */
  tdep->aarch64_syscall_record = aarch64_linux_syscall_record;

  /* MTE-specific settings and hooks.  */
  if (tdep->has_mte ())
    {
      /* Register a hook for checking if an address is tagged or not.  */
      set_gdbarch_tagged_address_p (gdbarch, aarch64_linux_tagged_address_p);

      /* Register a hook for checking if there is a memory tag match.  */
      set_gdbarch_memtag_matches_p (gdbarch,
				    aarch64_linux_memtag_matches_p);

      /* Register a hook for setting the logical/allocation tags for
	 a range of addresses.  */
      set_gdbarch_set_memtags (gdbarch, aarch64_linux_set_memtags);

      /* Register a hook for extracting the logical/allocation tag from an
	 address.  */
      set_gdbarch_get_memtag (gdbarch, aarch64_linux_get_memtag);

      /* Set the allocation tag granule size to 16 bytes.  */
      set_gdbarch_memtag_granule_size (gdbarch, AARCH64_MTE_GRANULE_SIZE);

      /* Register a hook for converting a memory tag to a string.  */
      set_gdbarch_memtag_to_string (gdbarch, aarch64_linux_memtag_to_string);

      set_gdbarch_report_signal_info (gdbarch,
				      aarch64_linux_report_signal_info);

      /* Core file helpers.  */

      /* Core file helper to create a memory tag section for a particular
	 PT_LOAD segment.  */
      set_gdbarch_create_memtag_section
	(gdbarch, aarch64_linux_create_memtag_section);

      /* Core file helper to fill a memory tag section with tag data.  */
      set_gdbarch_fill_memtag_section
	(gdbarch, aarch64_linux_fill_memtag_section);

      /* Core file helper to decode a memory tag section.  */
      set_gdbarch_decode_memtag_section (gdbarch,
					 aarch64_linux_decode_memtag_section);
    }

  /* Initialize the aarch64_linux_record_tdep.  */
  /* These values are the size of the type that will be used in a system
     call.  They are obtained from Linux Kernel source.  */
  aarch64_linux_record_tdep.size_pointer
    = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
  aarch64_linux_record_tdep.size__old_kernel_stat = 32;
  aarch64_linux_record_tdep.size_tms = 32;
  aarch64_linux_record_tdep.size_loff_t = 8;
  aarch64_linux_record_tdep.size_flock = 32;
  aarch64_linux_record_tdep.size_oldold_utsname = 45;
  aarch64_linux_record_tdep.size_ustat = 32;
  aarch64_linux_record_tdep.size_old_sigaction = 32;
  aarch64_linux_record_tdep.size_old_sigset_t = 8;
  aarch64_linux_record_tdep.size_rlimit = 16;
  aarch64_linux_record_tdep.size_rusage = 144;
  aarch64_linux_record_tdep.size_timeval = 16;
  aarch64_linux_record_tdep.size_timezone = 8;
  aarch64_linux_record_tdep.size_old_gid_t = 2;
  aarch64_linux_record_tdep.size_old_uid_t = 2;
  aarch64_linux_record_tdep.size_fd_set = 128;
  aarch64_linux_record_tdep.size_old_dirent = 280;
  aarch64_linux_record_tdep.size_statfs = 120;
  aarch64_linux_record_tdep.size_statfs64 = 120;
  aarch64_linux_record_tdep.size_sockaddr = 16;
  aarch64_linux_record_tdep.size_int
    = gdbarch_int_bit (gdbarch) / TARGET_CHAR_BIT;
  aarch64_linux_record_tdep.size_long
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  aarch64_linux_record_tdep.size_ulong
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  aarch64_linux_record_tdep.size_msghdr = 56;
  aarch64_linux_record_tdep.size_itimerval = 32;
  aarch64_linux_record_tdep.size_stat = 144;
  aarch64_linux_record_tdep.size_old_utsname = 325;
  aarch64_linux_record_tdep.size_sysinfo = 112;
  aarch64_linux_record_tdep.size_msqid_ds = 120;
  aarch64_linux_record_tdep.size_shmid_ds = 112;
  aarch64_linux_record_tdep.size_new_utsname = 390;
  aarch64_linux_record_tdep.size_timex = 208;
  aarch64_linux_record_tdep.size_mem_dqinfo = 24;
  aarch64_linux_record_tdep.size_if_dqblk = 72;
  aarch64_linux_record_tdep.size_fs_quota_stat = 80;
  aarch64_linux_record_tdep.size_timespec = 16;
  aarch64_linux_record_tdep.size_pollfd = 8;
  aarch64_linux_record_tdep.size_NFS_FHSIZE = 32;
  aarch64_linux_record_tdep.size_knfsd_fh = 132;
  aarch64_linux_record_tdep.size_TASK_COMM_LEN = 16;
  aarch64_linux_record_tdep.size_sigaction = 32;
  aarch64_linux_record_tdep.size_sigset_t = 8;
  aarch64_linux_record_tdep.size_siginfo_t = 128;
  aarch64_linux_record_tdep.size_cap_user_data_t = 8;
  aarch64_linux_record_tdep.size_stack_t = 24;
  aarch64_linux_record_tdep.size_off_t = 8;
  aarch64_linux_record_tdep.size_stat64 = 144;
  aarch64_linux_record_tdep.size_gid_t = 4;
  aarch64_linux_record_tdep.size_uid_t = 4;
  aarch64_linux_record_tdep.size_PAGE_SIZE = 4096;
  aarch64_linux_record_tdep.size_flock64 = 32;
  aarch64_linux_record_tdep.size_user_desc = 16;
  aarch64_linux_record_tdep.size_io_event = 32;
  aarch64_linux_record_tdep.size_iocb = 64;
  aarch64_linux_record_tdep.size_epoll_event = 12;
  aarch64_linux_record_tdep.size_itimerspec = 32;
  aarch64_linux_record_tdep.size_mq_attr = 64;
  aarch64_linux_record_tdep.size_termios = 36;
  aarch64_linux_record_tdep.size_termios2 = 44;
  aarch64_linux_record_tdep.size_pid_t = 4;
  aarch64_linux_record_tdep.size_winsize = 8;
  aarch64_linux_record_tdep.size_serial_struct = 72;
  aarch64_linux_record_tdep.size_serial_icounter_struct = 80;
  aarch64_linux_record_tdep.size_hayes_esp_config = 12;
  aarch64_linux_record_tdep.size_size_t = 8;
  aarch64_linux_record_tdep.size_iovec = 16;
  aarch64_linux_record_tdep.size_time_t = 8;

  /* These values are the second argument of system call "sys_ioctl".
     They are obtained from Linux Kernel source.  */
  aarch64_linux_record_tdep.ioctl_TCGETS = 0x5401;
  aarch64_linux_record_tdep.ioctl_TCSETS = 0x5402;
  aarch64_linux_record_tdep.ioctl_TCSETSW = 0x5403;
  aarch64_linux_record_tdep.ioctl_TCSETSF = 0x5404;
  aarch64_linux_record_tdep.ioctl_TCGETA = 0x5405;
  aarch64_linux_record_tdep.ioctl_TCSETA = 0x5406;
  aarch64_linux_record_tdep.ioctl_TCSETAW = 0x5407;
  aarch64_linux_record_tdep.ioctl_TCSETAF = 0x5408;
  aarch64_linux_record_tdep.ioctl_TCSBRK = 0x5409;
  aarch64_linux_record_tdep.ioctl_TCXONC = 0x540a;
  aarch64_linux_record_tdep.ioctl_TCFLSH = 0x540b;
  aarch64_linux_record_tdep.ioctl_TIOCEXCL = 0x540c;
  aarch64_linux_record_tdep.ioctl_TIOCNXCL = 0x540d;
  aarch64_linux_record_tdep.ioctl_TIOCSCTTY = 0x540e;
  aarch64_linux_record_tdep.ioctl_TIOCGPGRP = 0x540f;
  aarch64_linux_record_tdep.ioctl_TIOCSPGRP = 0x5410;
  aarch64_linux_record_tdep.ioctl_TIOCOUTQ = 0x5411;
  aarch64_linux_record_tdep.ioctl_TIOCSTI = 0x5412;
  aarch64_linux_record_tdep.ioctl_TIOCGWINSZ = 0x5413;
  aarch64_linux_record_tdep.ioctl_TIOCSWINSZ = 0x5414;
  aarch64_linux_record_tdep.ioctl_TIOCMGET = 0x5415;
  aarch64_linux_record_tdep.ioctl_TIOCMBIS = 0x5416;
  aarch64_linux_record_tdep.ioctl_TIOCMBIC = 0x5417;
  aarch64_linux_record_tdep.ioctl_TIOCMSET = 0x5418;
  aarch64_linux_record_tdep.ioctl_TIOCGSOFTCAR = 0x5419;
  aarch64_linux_record_tdep.ioctl_TIOCSSOFTCAR = 0x541a;
  aarch64_linux_record_tdep.ioctl_FIONREAD = 0x541b;
  aarch64_linux_record_tdep.ioctl_TIOCINQ = 0x541b;
  aarch64_linux_record_tdep.ioctl_TIOCLINUX = 0x541c;
  aarch64_linux_record_tdep.ioctl_TIOCCONS = 0x541d;
  aarch64_linux_record_tdep.ioctl_TIOCGSERIAL = 0x541e;
  aarch64_linux_record_tdep.ioctl_TIOCSSERIAL = 0x541f;
  aarch64_linux_record_tdep.ioctl_TIOCPKT = 0x5420;
  aarch64_linux_record_tdep.ioctl_FIONBIO = 0x5421;
  aarch64_linux_record_tdep.ioctl_TIOCNOTTY = 0x5422;
  aarch64_linux_record_tdep.ioctl_TIOCSETD = 0x5423;
  aarch64_linux_record_tdep.ioctl_TIOCGETD = 0x5424;
  aarch64_linux_record_tdep.ioctl_TCSBRKP = 0x5425;
  aarch64_linux_record_tdep.ioctl_TIOCTTYGSTRUCT = 0x5426;
  aarch64_linux_record_tdep.ioctl_TIOCSBRK = 0x5427;
  aarch64_linux_record_tdep.ioctl_TIOCCBRK = 0x5428;
  aarch64_linux_record_tdep.ioctl_TIOCGSID = 0x5429;
  aarch64_linux_record_tdep.ioctl_TCGETS2 = 0x802c542a;
  aarch64_linux_record_tdep.ioctl_TCSETS2 = 0x402c542b;
  aarch64_linux_record_tdep.ioctl_TCSETSW2 = 0x402c542c;
  aarch64_linux_record_tdep.ioctl_TCSETSF2 = 0x402c542d;
  aarch64_linux_record_tdep.ioctl_TIOCGPTN = 0x80045430;
  aarch64_linux_record_tdep.ioctl_TIOCSPTLCK = 0x40045431;
  aarch64_linux_record_tdep.ioctl_FIONCLEX = 0x5450;
  aarch64_linux_record_tdep.ioctl_FIOCLEX = 0x5451;
  aarch64_linux_record_tdep.ioctl_FIOASYNC = 0x5452;
  aarch64_linux_record_tdep.ioctl_TIOCSERCONFIG = 0x5453;
  aarch64_linux_record_tdep.ioctl_TIOCSERGWILD = 0x5454;
  aarch64_linux_record_tdep.ioctl_TIOCSERSWILD = 0x5455;
  aarch64_linux_record_tdep.ioctl_TIOCGLCKTRMIOS = 0x5456;
  aarch64_linux_record_tdep.ioctl_TIOCSLCKTRMIOS = 0x5457;
  aarch64_linux_record_tdep.ioctl_TIOCSERGSTRUCT = 0x5458;
  aarch64_linux_record_tdep.ioctl_TIOCSERGETLSR = 0x5459;
  aarch64_linux_record_tdep.ioctl_TIOCSERGETMULTI = 0x545a;
  aarch64_linux_record_tdep.ioctl_TIOCSERSETMULTI = 0x545b;
  aarch64_linux_record_tdep.ioctl_TIOCMIWAIT = 0x545c;
  aarch64_linux_record_tdep.ioctl_TIOCGICOUNT = 0x545d;
  aarch64_linux_record_tdep.ioctl_TIOCGHAYESESP = 0x545e;
  aarch64_linux_record_tdep.ioctl_TIOCSHAYESESP = 0x545f;
  aarch64_linux_record_tdep.ioctl_FIOQSIZE = 0x5460;

  /* These values are the second argument of system call "sys_fcntl"
     and "sys_fcntl64".  They are obtained from Linux Kernel source.  */
  aarch64_linux_record_tdep.fcntl_F_GETLK = 5;
  aarch64_linux_record_tdep.fcntl_F_GETLK64 = 12;
  aarch64_linux_record_tdep.fcntl_F_SETLK64 = 13;
  aarch64_linux_record_tdep.fcntl_F_SETLKW64 = 14;

  /* The AArch64 syscall calling convention: reg x0-x6 for arguments,
     reg x8 for syscall number and return value in reg x0.  */
  aarch64_linux_record_tdep.arg1 = AARCH64_X0_REGNUM + 0;
  aarch64_linux_record_tdep.arg2 = AARCH64_X0_REGNUM + 1;
  aarch64_linux_record_tdep.arg3 = AARCH64_X0_REGNUM + 2;
  aarch64_linux_record_tdep.arg4 = AARCH64_X0_REGNUM + 3;
  aarch64_linux_record_tdep.arg5 = AARCH64_X0_REGNUM + 4;
  aarch64_linux_record_tdep.arg6 = AARCH64_X0_REGNUM + 5;
  aarch64_linux_record_tdep.arg7 = AARCH64_X0_REGNUM + 6;

  /* `catch syscall' */
  set_xml_syscall_file_name (gdbarch, "syscalls/aarch64-linux.xml");
  set_gdbarch_get_syscall_number (gdbarch, aarch64_linux_get_syscall_number);

  /* Displaced stepping.  */
  set_gdbarch_max_insn_length (gdbarch, 4);
  set_gdbarch_displaced_step_buffer_length
    (gdbarch, 4 * AARCH64_DISPLACED_MODIFIED_INSNS);
  set_gdbarch_displaced_step_copy_insn (gdbarch,
					aarch64_displaced_step_copy_insn);
  set_gdbarch_displaced_step_fixup (gdbarch, aarch64_displaced_step_fixup);
  set_gdbarch_displaced_step_hw_singlestep (gdbarch,
					    aarch64_displaced_step_hw_singlestep);

  set_gdbarch_gcc_target_options (gdbarch, aarch64_linux_gcc_target_options);

  /* Hook to decide if the target description should be obtained from
     corefile target description note(s) or inferred from the corefile
     sections.  */
  set_gdbarch_use_target_description_from_corefile_notes (gdbarch,
			    aarch64_use_target_description_from_corefile_notes);
}

#if GDB_SELF_TEST

namespace selftests {

/* Verify functions to read and write logical tags.  */

static void
aarch64_linux_ltag_tests (void)
{
  /* We have 4 bits of tags, but we test writing all the bits of the top
     byte of address.  */
  for (int i = 0; i < 1 << 8; i++)
    {
      CORE_ADDR addr = ((CORE_ADDR) i << 56) | 0xdeadbeef;
      SELF_CHECK (aarch64_mte_get_ltag (addr) == (i & 0xf));

      addr = aarch64_mte_set_ltag (0xdeadbeef, i);
      SELF_CHECK (addr = ((CORE_ADDR) (i & 0xf) << 56) | 0xdeadbeef);
    }
}

} // namespace selftests
#endif /* GDB_SELF_TEST */

void _initialize_aarch64_linux_tdep ();
void
_initialize_aarch64_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_aarch64, 0, GDB_OSABI_LINUX,
			  aarch64_linux_init_abi);

#if GDB_SELF_TEST
  selftests::register_test ("aarch64-linux-tagged-address",
			    selftests::aarch64_linux_ltag_tests);
#endif
}
