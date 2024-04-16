/* Common native Linux code for the AArch64 scalable extensions: SVE and SME.

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

#include <sys/utsname.h>
#include <sys/uio.h>
#include "gdbsupport/common-defs.h"
#include "elf/external.h"
#include "elf/common.h"
#include "aarch64-scalable-linux-ptrace.h"
#include "arch/aarch64.h"
#include "gdbsupport/common-regcache.h"
#include "gdbsupport/byte-vector.h"
#include <endian.h>
#include "arch/aarch64-scalable-linux.h"

/* See nat/aarch64-scalable-linux-ptrace.h.  */

bool
aarch64_has_sve_state (int tid)
{
  struct user_sve_header header;

  if (!read_sve_header (tid, header))
    return false;

  if ((header.flags & SVE_PT_REGS_SVE) == 0)
    return false;

  if (sizeof (header) == header.size)
    return false;

  return true;
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

bool
aarch64_has_ssve_state (int tid)
{
  struct user_sve_header header;

  if (!read_ssve_header (tid, header))
    return false;

  if ((header.flags & SVE_PT_REGS_SVE) == 0)
    return false;

  if (sizeof (header) == header.size)
    return false;

  return true;
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

bool
aarch64_has_za_state (int tid)
{
  struct user_za_header header;

  if (!read_za_header (tid, header))
    return false;

  if (sizeof (header) == header.size)
    return false;

  return true;
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

bool
read_sve_header (int tid, struct user_sve_header &header)
{
  struct iovec iovec;

  iovec.iov_len = sizeof (header);
  iovec.iov_base = &header;

  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_SVE, &iovec) < 0)
    {
      /* SVE is not supported.  */
      return false;
    }
  return true;
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

bool
write_sve_header (int tid, const struct user_sve_header &header)
{
  struct iovec iovec;

  iovec.iov_len = sizeof (header);
  iovec.iov_base = (void *) &header;

  if (ptrace (PTRACE_SETREGSET, tid, NT_ARM_SVE, &iovec) < 0)
    {
      /* SVE is not supported.  */
      return false;
    }
  return true;
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

bool
read_ssve_header (int tid, struct user_sve_header &header)
{
  struct iovec iovec;

  iovec.iov_len = sizeof (header);
  iovec.iov_base = &header;

  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_SSVE, &iovec) < 0)
    {
      /* SSVE is not supported.  */
      return false;
    }
  return true;
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

bool
write_ssve_header (int tid, const struct user_sve_header &header)
{
  struct iovec iovec;

  iovec.iov_len = sizeof (header);
  iovec.iov_base = (void *) &header;

  if (ptrace (PTRACE_SETREGSET, tid, NT_ARM_SSVE, &iovec) < 0)
    {
      /* SSVE is not supported.  */
      return false;
    }
  return true;
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

bool
read_za_header (int tid, struct user_za_header &header)
{
  struct iovec iovec;

  iovec.iov_len = sizeof (header);
  iovec.iov_base = &header;

  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_ZA, &iovec) < 0)
    {
      /* ZA is not supported.  */
      return false;
    }
  return true;
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

bool
write_za_header (int tid, const struct user_za_header &header)
{
  struct iovec iovec;

  iovec.iov_len = sizeof (header);
  iovec.iov_base = (void *) &header;

  if (ptrace (PTRACE_SETREGSET, tid, NT_ARM_ZA, &iovec) < 0)
    {
      /* ZA is not supported.  */
      return false;
    }
  return true;
}

/* Given VL, the streaming vector length for SME, return true if it is valid
   and false otherwise.  */

static bool
aarch64_sme_vl_valid (size_t vl)
{
  return (vl == 16 || vl == 32 || vl == 64 || vl == 128 || vl == 256);
}

/* Given VL, the vector length for SVE, return true if it is valid and false
   otherwise.  SVE_state is true when the check is for the SVE register set.
   Otherwise the check is for the SSVE register set.  */

static bool
aarch64_sve_vl_valid (const bool sve_state, size_t vl)
{
  if (sve_state)
    return sve_vl_valid (vl);

  /* We have an active SSVE state, where the valid vector length values are
     more restrictive.  */
  return aarch64_sme_vl_valid (vl);
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

uint64_t
aarch64_sve_get_vq (int tid)
{
  struct iovec iovec;
  struct user_sve_header header;
  iovec.iov_len = sizeof (header);
  iovec.iov_base = &header;

  /* Figure out which register set to use for the request.  The vector length
     for SVE can be different from the vector length for SSVE.  */
  bool has_sve_state = !aarch64_has_ssve_state (tid);
  if (ptrace (PTRACE_GETREGSET, tid, has_sve_state? NT_ARM_SVE : NT_ARM_SSVE,
	      &iovec) < 0)
    {
      /* SVE is not supported.  */
      return 0;
    }

  /* Ptrace gives the vector length in bytes.  Convert it to VQ, the number of
     128bit chunks in a Z register.  We use VQ because 128 bits is the minimum
     a Z register can increase in size.  */
  uint64_t vq = sve_vq_from_vl (header.vl);

  if (!aarch64_sve_vl_valid (has_sve_state, header.vl))
    {
      warning (_("Invalid SVE state from kernel; SVE disabled."));
      return 0;
    }

  return vq;
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

bool
aarch64_sve_set_vq (int tid, uint64_t vq)
{
  struct iovec iovec;
  struct user_sve_header header;

  iovec.iov_len = sizeof (header);
  iovec.iov_base = &header;

  /* Figure out which register set to use for the request.  The vector length
     for SVE can be different from the vector length for SSVE.  */
  bool has_sve_state = !aarch64_has_ssve_state (tid);
  if (ptrace (PTRACE_GETREGSET, tid, has_sve_state? NT_ARM_SVE : NT_ARM_SSVE,
	      &iovec) < 0)
    {
      /* SVE/SSVE is not supported.  */
      return false;
    }

  header.vl = sve_vl_from_vq (vq);

  if (ptrace (PTRACE_SETREGSET, tid, has_sve_state? NT_ARM_SVE : NT_ARM_SSVE,
	      &iovec) < 0)
    {
      /* Vector length change failed.  */
      return false;
    }

  return true;
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

bool
aarch64_sve_set_vq (int tid, struct reg_buffer_common *reg_buf)
{
  uint64_t reg_vg = 0;

  /* The VG register may not be valid if we've not collected any value yet.
     This can happen, for example,  if we're restoring the regcache after an
     inferior function call, and the VG register comes after the Z
     registers.  */
  if (reg_buf->get_register_status (AARCH64_SVE_VG_REGNUM) != REG_VALID)
    {
      /* If vg is not available yet, fetch it from ptrace.  The VG value from
	 ptrace is likely the correct one.  */
      uint64_t vq = aarch64_sve_get_vq (tid);

      /* If something went wrong, just bail out.  */
      if (vq == 0)
	return false;

      reg_vg = sve_vg_from_vq (vq);
    }
  else
    reg_buf->raw_collect (AARCH64_SVE_VG_REGNUM, &reg_vg);

  return aarch64_sve_set_vq (tid, sve_vq_from_vg (reg_vg));
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

uint64_t
aarch64_za_get_svq (int tid)
{
  struct user_za_header header;
  if (!read_za_header (tid, header))
    return 0;

  uint64_t vq = sve_vq_from_vl (header.vl);

  if (!aarch64_sve_vl_valid (false, header.vl))
    {
      warning (_("Invalid ZA state from kernel; ZA disabled."));
      return 0;
    }

  return vq;
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

bool
aarch64_za_set_svq (int tid, uint64_t vq)
{
  struct iovec iovec;

  /* Read the NT_ARM_ZA header.  */
  struct user_za_header header;
  if (!read_za_header (tid, header))
    {
      /* ZA is not supported.  */
      return false;
    }

  /* If the size is the correct one already, don't update it.  If we do
     update the streaming vector length, we will invalidate the register
     state for ZA, and we do not want that.  */
  if (header.vl == sve_vl_from_vq (vq))
    return true;

  /* The streaming vector length is about to get updated.  Set the new value
     in the NT_ARM_ZA header and adjust the size as well.  */

  header.vl = sve_vl_from_vq (vq);
  header.size = sizeof (struct user_za_header);

  /* Update the NT_ARM_ZA register set with the new streaming vector
     length.  */
  iovec.iov_len = sizeof (header);
  iovec.iov_base = &header;

  if (ptrace (PTRACE_SETREGSET, tid, NT_ARM_ZA, &iovec) < 0)
    {
      /* Streaming vector length change failed.  */
      return false;
    }

  /* At this point we have successfully adjusted the streaming vector length
     for the NT_ARM_ZA register set, and it should have no payload
     (no ZA state).  */

  return true;
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

bool
aarch64_za_set_svq (int tid, const struct reg_buffer_common *reg_buf,
		    int svg_regnum)
{
  uint64_t reg_svg = 0;

  /* The svg register may not be valid if we've not collected any value yet.
     This can happen, for example,  if we're restoring the regcache after an
     inferior function call, and the svg register comes after the Z
     registers.  */
  if (reg_buf->get_register_status (svg_regnum) != REG_VALID)
    {
      /* If svg is not available yet, fetch it from ptrace.  The svg value from
	 ptrace is likely the correct one.  */
      uint64_t svq = aarch64_za_get_svq (tid);

      /* If something went wrong, just bail out.  */
      if (svq == 0)
	return false;

      reg_svg = sve_vg_from_vq (svq);
    }
  else
    reg_buf->raw_collect (svg_regnum, &reg_svg);

  return aarch64_za_set_svq (tid, sve_vq_from_vg (reg_svg));
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

gdb::byte_vector
aarch64_fetch_sve_regset (int tid)
{
  uint64_t vq = aarch64_sve_get_vq (tid);

  if (vq == 0)
    perror_with_name (_("Unable to fetch SVE/SSVE vector length"));

  /* A ptrace call with NT_ARM_SVE will return a header followed by either a
     dump of all the SVE and FP registers, or an fpsimd structure (identical to
     the one returned by NT_FPREGSET) if the kernel has not yet executed any
     SVE code.  Make sure we allocate enough space for a full SVE dump.  */

  gdb::byte_vector sve_state (SVE_PT_SIZE (vq, SVE_PT_REGS_SVE), 0);

  struct iovec iovec;
  iovec.iov_base = sve_state.data ();
  iovec.iov_len = sve_state.size ();

  bool has_sve_state = !aarch64_has_ssve_state (tid);
  if (ptrace (PTRACE_GETREGSET, tid, has_sve_state? NT_ARM_SVE : NT_ARM_SSVE,
	      &iovec) < 0)
    perror_with_name (_("Unable to fetch SVE/SSVE registers"));

  return sve_state;
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

void
aarch64_store_sve_regset (int tid, const gdb::byte_vector &sve_state)
{
  struct iovec iovec;
  /* We need to cast from (const void *) here.  */
  iovec.iov_base = (void *) sve_state.data ();
  iovec.iov_len = sve_state.size ();

  bool has_sve_state = !aarch64_has_ssve_state (tid);
  if (ptrace (PTRACE_SETREGSET, tid, has_sve_state? NT_ARM_SVE : NT_ARM_SSVE,
	      &iovec) < 0)
    perror_with_name (_("Unable to store SVE/SSVE registers"));
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

gdb::byte_vector
aarch64_fetch_za_regset (int tid)
{
  struct user_za_header header;
  if (!read_za_header (tid, header))
    error (_("Failed to read NT_ARM_ZA header."));

  if (!aarch64_sme_vl_valid (header.vl))
    error (_("Found invalid vector length for NT_ARM_ZA."));

  struct iovec iovec;
  iovec.iov_len = header.size;
  gdb::byte_vector za_state (header.size);
  iovec.iov_base = za_state.data ();

  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_ZA, &iovec) < 0)
    perror_with_name (_("Failed to fetch NT_ARM_ZA register set."));

  return za_state;
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

void
aarch64_store_za_regset (int tid, const gdb::byte_vector &za_state)
{
  struct iovec iovec;
  /* We need to cast from (const void *) here.  */
  iovec.iov_base = (void *) za_state.data ();
  iovec.iov_len = za_state.size ();

  if (ptrace (PTRACE_SETREGSET, tid, NT_ARM_ZA, &iovec) < 0)
    perror_with_name (_("Failed to write to the NT_ARM_ZA register set."));
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

void
aarch64_initialize_za_regset (int tid)
{
  /* First fetch the NT_ARM_ZA header so we can fetch the streaming vector
     length.  */
  struct user_za_header header;
  if (!read_za_header (tid, header))
    error (_("Failed to read NT_ARM_ZA header."));

  /* The vector should be default-initialized to zero, and we should account
     for the payload as well.  */
  std::vector<gdb_byte> za_new_state (ZA_PT_SIZE (sve_vq_from_vl (header.vl)));

  /* Adjust the header size since we are adding the initialized ZA
     payload.  */
  header.size = ZA_PT_SIZE (sve_vq_from_vl (header.vl));

  /* Overlay the modified header onto the new ZA state.  */
  const gdb_byte *base = (gdb_byte *) &header;
  memcpy (za_new_state.data (), base, sizeof (user_za_header));

  /* Set the ptrace request up and update the NT_ARM_ZA register set.  */
  struct iovec iovec;
  iovec.iov_len = za_new_state.size ();
  iovec.iov_base = za_new_state.data ();

  if (ptrace (PTRACE_SETREGSET, tid, NT_ARM_ZA, &iovec) < 0)
    perror_with_name (_("Failed to initialize the NT_ARM_ZA register set."));

  if (supports_zt_registers (tid))
    {
      /* If this target supports SME2, upon initializing ZA, we also need to
	 initialize the ZT registers with 0 values.  Do so now.  */
      gdb::byte_vector zt_new_state (AARCH64_SME2_ZT0_SIZE, 0);
      aarch64_store_zt_regset (tid, zt_new_state);
    }

  /* The NT_ARM_ZA register set should now contain a zero-initialized ZA
     payload.  */
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

gdb::byte_vector
aarch64_fetch_zt_regset (int tid)
{
  /* Read NT_ARM_ZT.  This register set is only available if
     the ZA bit is non-zero.  */
  gdb::byte_vector zt_state (AARCH64_SME2_ZT0_SIZE);

  struct iovec iovec;
  iovec.iov_len = AARCH64_SME2_ZT0_SIZE;
  iovec.iov_base = zt_state.data ();

  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_ZT, &iovec) < 0)
    perror_with_name (_("Failed to fetch NT_ARM_ZT register set."));

  return zt_state;
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

void
aarch64_store_zt_regset (int tid, const gdb::byte_vector &zt_state)
{
  gdb_assert (zt_state.size () == AARCH64_SME2_ZT0_SIZE
	      || zt_state.size () == 0);

  /* We need to be mindful of writing data to NT_ARM_ZT.  If the ZA bit
     is 0 and we write something to ZT, it will flip the ZA bit.

     Right now this is taken care of by callers of this function.  */
  struct iovec iovec;
  iovec.iov_len = zt_state.size ();
  iovec.iov_base = (void *) zt_state.data ();

  /* Write the contents of ZT_STATE to the NT_ARM_ZT register set.  */
  if (ptrace (PTRACE_SETREGSET, tid, NT_ARM_ZT, &iovec) < 0)
    perror_with_name (_("Failed to write to the NT_ARM_ZT register set."));
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

bool
supports_zt_registers (int tid)
{
  gdb_byte zt_state[AARCH64_SME2_ZT0_SIZE];

  struct iovec iovec;
  iovec.iov_len = AARCH64_SME2_ZT0_SIZE;
  iovec.iov_base = (void *) zt_state;

  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_ZT, &iovec) < 0)
    return false;

  return true;
}

/* If we are running in BE mode, byteswap the contents
   of SRC to DST for SIZE bytes.  Other, just copy the contents
   from SRC to DST.  */

static void
aarch64_maybe_swab128 (gdb_byte *dst, const gdb_byte *src, size_t size)
{
  gdb_assert (src != nullptr && dst != nullptr);
  gdb_assert (size > 1);

#if (__BYTE_ORDER == __BIG_ENDIAN)
  for (int i = 0; i < size - 1; i++)
    dst[i] = src[size - i];
#else
  memcpy (dst, src, size);
#endif
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

void
aarch64_sve_regs_copy_to_reg_buf (int tid, struct reg_buffer_common *reg_buf)
{
  gdb::byte_vector sve_state = aarch64_fetch_sve_regset (tid);

  gdb_byte *base = sve_state.data ();
  struct user_sve_header *header
    = (struct user_sve_header *) sve_state.data ();

  uint64_t vq = sve_vq_from_vl (header->vl);
  uint64_t vg = sve_vg_from_vl (header->vl);

  /* Sanity check the data in the header.  */
  if (!sve_vl_valid (header->vl)
      || SVE_PT_SIZE (vq, header->flags) != header->size)
    error (_("Invalid SVE header from kernel."));

  /* Update VG.  Note, the registers in the regcache will already be of the
     correct length.  */
  reg_buf->raw_supply (AARCH64_SVE_VG_REGNUM, &vg);

  if (HAS_SVE_STATE (*header))
    {
      /* The register dump contains a set of SVE registers.  */

      for (int i = 0; i < AARCH64_SVE_Z_REGS_NUM; i++)
	reg_buf->raw_supply (AARCH64_SVE_Z0_REGNUM + i,
			     base + SVE_PT_SVE_ZREG_OFFSET (vq, i));

      for (int i = 0; i < AARCH64_SVE_P_REGS_NUM; i++)
	reg_buf->raw_supply (AARCH64_SVE_P0_REGNUM + i,
			     base + SVE_PT_SVE_PREG_OFFSET (vq, i));

      reg_buf->raw_supply (AARCH64_SVE_FFR_REGNUM,
			   base + SVE_PT_SVE_FFR_OFFSET (vq));
      reg_buf->raw_supply (AARCH64_FPSR_REGNUM,
			   base + SVE_PT_SVE_FPSR_OFFSET (vq));
      reg_buf->raw_supply (AARCH64_FPCR_REGNUM,
			   base + SVE_PT_SVE_FPCR_OFFSET (vq));
    }
  else
    {
      /* WARNING: SIMD state is laid out in memory in target-endian format,
	 while SVE state is laid out in an endianness-independent format (LE).

	 So we have a couple cases to consider:

	 1 - If the target is big endian, then SIMD state is big endian,
	 requiring a byteswap.

	 2 - If the target is little endian, then SIMD state is little endian,
	 which matches the SVE format, so no byteswap is needed. */

      /* There is no SVE state yet - the register dump contains a fpsimd
	 structure instead.  These registers still exist in the hardware, but
	 the kernel has not yet initialised them, and so they will be null.  */

      gdb_byte *reg = (gdb_byte *) alloca (SVE_PT_SVE_ZREG_SIZE (vq));
      struct user_fpsimd_state *fpsimd
	= (struct user_fpsimd_state *)(base + SVE_PT_FPSIMD_OFFSET);

      /* Make sure we have a zeroed register buffer.  We will need the zero
	 padding below.  */
      memset (reg, 0, SVE_PT_SVE_ZREG_SIZE (vq));

      /* Copy across the V registers from fpsimd structure to the Z registers,
	 ensuring the non overlapping state is set to null.  */

      for (int i = 0; i < AARCH64_SVE_Z_REGS_NUM; i++)
	{
	  /* Handle big endian/little endian SIMD/SVE conversion.  */
	  aarch64_maybe_swab128 (reg, (const gdb_byte *) &fpsimd->vregs[i],
				 V_REGISTER_SIZE);
	  reg_buf->raw_supply (AARCH64_SVE_Z0_REGNUM + i, reg);
	}

      reg_buf->raw_supply (AARCH64_FPSR_REGNUM,
			   (const gdb_byte *) &fpsimd->fpsr);
      reg_buf->raw_supply (AARCH64_FPCR_REGNUM,
			   (const gdb_byte *) &fpsimd->fpcr);

      /* Clear the SVE only registers.  */
      memset (reg, 0, SVE_PT_SVE_ZREG_SIZE (vq));

      for (int i = 0; i < AARCH64_SVE_P_REGS_NUM; i++)
	reg_buf->raw_supply (AARCH64_SVE_P0_REGNUM + i, reg);

      reg_buf->raw_supply (AARCH64_SVE_FFR_REGNUM, reg);
    }

  /* At this point we have updated the register cache with the contents of
     the NT_ARM_SVE register set.  */
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

void
aarch64_sve_regs_copy_from_reg_buf (int tid,
				    struct reg_buffer_common *reg_buf)
{
  /* First store the vector length to the thread.  This is done first to
     ensure the ptrace buffers read from the kernel are the correct size.  */
  if (!aarch64_sve_set_vq (tid, reg_buf))
    perror_with_name (_("Unable to set VG register"));

  /* Obtain a dump of SVE registers from ptrace.  */
  gdb::byte_vector sve_state = aarch64_fetch_sve_regset (tid);

  struct user_sve_header *header = (struct user_sve_header *) sve_state.data ();
  uint64_t vq = sve_vq_from_vl (header->vl);

  gdb::byte_vector new_state (SVE_PT_SIZE (32, SVE_PT_REGS_SVE), 0);
  memcpy (new_state.data (), sve_state.data (), sve_state.size ());
  header = (struct user_sve_header *) new_state.data ();
  gdb_byte *base = new_state.data ();

  /* Sanity check the data in the header.  */
  if (!sve_vl_valid (header->vl)
      || SVE_PT_SIZE (vq, header->flags) != header->size)
    error (_("Invalid SVE header from kernel."));

  if (!HAS_SVE_STATE (*header))
    {
      /* There is no SVE state yet - the register dump contains a fpsimd
	 structure instead.  Where possible we want to write the reg_buf data
	 back to the kernel using the fpsimd structure.  However, if we cannot
	 then we'll need to reformat the fpsimd into a full SVE structure,
	 resulting in the initialization of SVE state written back to the
	 kernel, which is why we try to avoid it.  */

      /* Buffer (using the maximum size a Z register) used to look for zeroed
	 out sve state.  */
      gdb_byte reg[256];
      memset (reg, 0, sizeof (reg));

      /* Check in the reg_buf if any of the Z registers are set after the
	 first 128 bits, or if any of the other SVE registers are set.  */
      bool has_sve_state = false;
      for (int i = 0; i < AARCH64_SVE_Z_REGS_NUM; i++)
	{
	  if (!reg_buf->raw_compare (AARCH64_SVE_Z0_REGNUM + i, reg,
				     V_REGISTER_SIZE))
	    {
	      has_sve_state = true;
	      break;
	    }
	}

      if (!has_sve_state)
	for (int i = 0; i < AARCH64_SVE_P_REGS_NUM; i++)
	  {
	    if (!reg_buf->raw_compare (AARCH64_SVE_P0_REGNUM + i, reg, 0))
	      {
		has_sve_state = true;
		break;
	      }
	  }

      if (!has_sve_state)
	  has_sve_state
	    = !reg_buf->raw_compare (AARCH64_SVE_FFR_REGNUM, reg, 0);

      struct user_fpsimd_state *fpsimd
	= (struct user_fpsimd_state *)(base + SVE_PT_FPSIMD_OFFSET);

      /* If no SVE state exists, then use the existing fpsimd structure to
	 write out state and return.  */
      if (!has_sve_state)
	{
	  /* WARNING: SIMD state is laid out in memory in target-endian format,
	     while SVE state is laid out in an endianness-independent format
	     (LE).

	     So we have a couple cases to consider:

	     1 - If the target is big endian, then SIMD state is big endian,
	     requiring a byteswap.

	     2 - If the target is little endian, then SIMD state is little
	     endian, which matches the SVE format, so no byteswap is needed. */

	  /* The collects of the Z registers will overflow the size of a vreg.
	     There is enough space in the structure to allow for this, but we
	     cannot overflow into the next register as we might not be
	     collecting every register.  */

	  for (int i = 0; i < AARCH64_SVE_Z_REGS_NUM; i++)
	    {
	      if (REG_VALID
		  == reg_buf->get_register_status (AARCH64_SVE_Z0_REGNUM + i))
		{
		  reg_buf->raw_collect (AARCH64_SVE_Z0_REGNUM + i, reg);
		  /* Handle big endian/little endian SIMD/SVE conversion.  */
		  aarch64_maybe_swab128 ((gdb_byte *) &fpsimd->vregs[i], reg,
					 V_REGISTER_SIZE);
		}
	    }

	  if (REG_VALID == reg_buf->get_register_status (AARCH64_FPSR_REGNUM))
	    reg_buf->raw_collect (AARCH64_FPSR_REGNUM,
				  (gdb_byte *) &fpsimd->fpsr);
	  if (REG_VALID == reg_buf->get_register_status (AARCH64_FPCR_REGNUM))
	    reg_buf->raw_collect (AARCH64_FPCR_REGNUM,
				  (gdb_byte *) &fpsimd->fpcr);

	  /* At this point we have collected all the data from the register
	     cache and we are ready to update the FPSIMD register content
	     of the thread.  */

	  /* Fall through so we can update the thread's contents with the
	     FPSIMD register cache values.  */
	}
      else
	{
	  /* Otherwise, reformat the fpsimd structure into a full SVE set, by
	     expanding the V registers (working backwards so we don't splat
	     registers before they are copied) and using zero for everything
	     else.
	     Note that enough space for a full SVE dump was originally allocated
	     for base.  */

	  header->flags |= SVE_PT_REGS_SVE;
	  header->size = SVE_PT_SIZE (vq, SVE_PT_REGS_SVE);

	  memcpy (base + SVE_PT_SVE_FPSR_OFFSET (vq), &fpsimd->fpsr,
		  sizeof (uint32_t));
	  memcpy (base + SVE_PT_SVE_FPCR_OFFSET (vq), &fpsimd->fpcr,
		  sizeof (uint32_t));

	  for (int i = AARCH64_SVE_Z_REGS_NUM - 1; i >= 0 ; i--)
	    {
	      memcpy (base + SVE_PT_SVE_ZREG_OFFSET (vq, i), &fpsimd->vregs[i],
		      sizeof (__int128_t));
	    }

	  /* At this point we have converted the FPSIMD layout to an SVE
	     layout and copied the register data.

	     Fall through so we can update the thread's contents with the SVE
	     register cache values.  */
	}
    }
  else
    {
      /* We already have SVE state for this thread, so we just need to update
	 the values of the registers.  */
      for (int i = 0; i < AARCH64_SVE_Z_REGS_NUM; i++)
	if (REG_VALID == reg_buf->get_register_status (AARCH64_SVE_Z0_REGNUM
						       + i))
	  reg_buf->raw_collect (AARCH64_SVE_Z0_REGNUM + i,
				base + SVE_PT_SVE_ZREG_OFFSET (vq, i));

      for (int i = 0; i < AARCH64_SVE_P_REGS_NUM; i++)
	if (REG_VALID == reg_buf->get_register_status (AARCH64_SVE_P0_REGNUM
						       + i))
	  reg_buf->raw_collect (AARCH64_SVE_P0_REGNUM + i,
				base + SVE_PT_SVE_PREG_OFFSET (vq, i));

      if (REG_VALID == reg_buf->get_register_status (AARCH64_SVE_FFR_REGNUM))
	reg_buf->raw_collect (AARCH64_SVE_FFR_REGNUM,
			      base + SVE_PT_SVE_FFR_OFFSET (vq));
      if (REG_VALID == reg_buf->get_register_status (AARCH64_FPSR_REGNUM))
	reg_buf->raw_collect (AARCH64_FPSR_REGNUM,
			      base + SVE_PT_SVE_FPSR_OFFSET (vq));
      if (REG_VALID == reg_buf->get_register_status (AARCH64_FPCR_REGNUM))
	reg_buf->raw_collect (AARCH64_FPCR_REGNUM,
			      base + SVE_PT_SVE_FPCR_OFFSET (vq));
    }

  /* At this point we have collected all the data from the register cache and
     we are ready to update the SVE/FPSIMD register contents of the thread.

     sve_state should contain all the data in the correct format, ready to be
     passed on to ptrace.  */
  aarch64_store_sve_regset (tid, new_state);
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

void
aarch64_za_regs_copy_to_reg_buf (int tid, struct reg_buffer_common *reg_buf,
				 int za_regnum, int svg_regnum,
				 int svcr_regnum)
{
  /* Fetch the current ZA state from the thread.  */
  gdb::byte_vector za_state = aarch64_fetch_za_regset (tid);

  /* Sanity check.  */
  gdb_assert (!za_state.empty ());

  gdb_byte *base = za_state.data ();
  struct user_za_header *header = (struct user_za_header *) base;

  /* If we have ZA state, read it.  Otherwise, make the contents of ZA
     in the register cache all zeroes.  This is how we present the ZA
     state when it is not initialized.  */
  uint64_t svcr_value = 0;
  if (aarch64_has_za_state (tid))
    {
      /* Sanity check the data in the header.  */
      if (!sve_vl_valid (header->vl)
	  || ZA_PT_SIZE (sve_vq_from_vl (header->vl)) != header->size)
	{
	  error (_("Found invalid streaming vector length in NT_ARM_ZA"
		   " register set"));
	}

      reg_buf->raw_supply (za_regnum, base + ZA_PT_ZA_OFFSET);
      svcr_value |= SVCR_ZA_BIT;
    }
  else
    {
      size_t za_bytes = header->vl * header->vl;
      gdb_byte za_zeroed[za_bytes];
      memset (za_zeroed, 0, za_bytes);
      reg_buf->raw_supply (za_regnum, za_zeroed);
    }

  /* Handle the svg and svcr registers separately.  We need to calculate
     their values manually, as the Linux Kernel doesn't expose those
     explicitly.  */
  svcr_value |= aarch64_has_ssve_state (tid)? SVCR_SM_BIT : 0;
  uint64_t svg_value = sve_vg_from_vl (header->vl);

  /* Update the contents of svg and svcr registers.  */
  reg_buf->raw_supply (svg_regnum, &svg_value);
  reg_buf->raw_supply (svcr_regnum, &svcr_value);

  /* The register buffer should now contain the updated copy of the NT_ARM_ZA
     state.  */
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

void
aarch64_za_regs_copy_from_reg_buf (int tid,
				   struct reg_buffer_common *reg_buf,
				   int za_regnum, int svg_regnum,
				   int svcr_regnum)
{
  /* REG_BUF contains the updated ZA state.  We need to extract that state
     and write it to the thread TID.  */


  /* First check if there is a change to the streaming vector length.  Two
     outcomes are possible here:

     1 - The streaming vector length in the register cache differs from the
	 one currently on the thread state.  This means that we will need to
	 update the NT_ARM_ZA register set to reflect the new streaming vector
	 length.

     2 - The streaming vector length in the register cache is the same as in
	 the thread state.  This means we do not need to update the NT_ARM_ZA
	 register set for a new streaming vector length, and we only need to
	 deal with changes to za, svg and svcr.

     None of the two possibilities above imply that the ZA state actually
     exists.  They only determine what needs to be done with any ZA content
     based on the state of the streaming vector length.  */

  /* First fetch the NT_ARM_ZA header so we can fetch the streaming vector
     length.  */
  struct user_za_header header;
  if (!read_za_header (tid, header))
    error (_("Failed to read NT_ARM_ZA header."));

  /* Fetch the current streaming vector length.  */
  uint64_t old_svg = sve_vg_from_vl (header.vl);

  /* Fetch the (potentially) new streaming vector length.  */
  uint64_t new_svg;
  reg_buf->raw_collect (svg_regnum, &new_svg);

  /* Did the streaming vector length change?  */
  bool svg_changed = new_svg != old_svg;

  /* First store the streaming vector length to the thread.  This is done
     first to ensure the ptrace buffers read from the kernel are the correct
     size.  If the streaming vector length is the same as the current one, it
     won't be updated.  */
  if (!aarch64_za_set_svq (tid, reg_buf, svg_regnum))
    error (_("Unable to set svg register"));

  bool has_za_state = aarch64_has_za_state (tid);

  size_t za_bytes = sve_vl_from_vg (old_svg) * sve_vl_from_vg (old_svg);
  gdb_byte za_zeroed[za_bytes];
  memset (za_zeroed, 0, za_bytes);

  /* If the streaming vector length changed, zero out the contents of ZA in
     the register cache.  Otherwise, we will need to update the ZA contents
     in the thread with the ZA contents from the register cache, and they will
     differ in size.  */
  if (svg_changed)
    reg_buf->raw_supply (za_regnum, za_zeroed);

  /* When we update svg, we don't automatically initialize the ZA buffer.  If
     we have no ZA state and the ZA register contents in the register cache are
     zero, just return and leave the ZA register cache contents as zero.  */
  if (!has_za_state
      && reg_buf->raw_compare (za_regnum, za_zeroed, 0))
    {
      /* No ZA state in the thread or in the register cache.  This was likely
	 just an adjustment of the streaming vector length.  Let this fall
	 through and update svcr and svg in the register cache.  */
    }
  else
    {
      /* If there is no ZA state but the register cache contains ZA data, we
	 need to initialize the ZA data through ptrace.  First we initialize
	 all the bytes of ZA to zero.  */
      if (!has_za_state
	  && !reg_buf->raw_compare (za_regnum, za_zeroed, 0))
	aarch64_initialize_za_regset (tid);

      /* From this point onwards, it is assumed we have a ZA payload in
	 the NT_ARM_ZA register set for this thread, and we need to update
	 such state based on the contents of the register cache.  */

      /* Fetch the current ZA state from the thread.  */
      gdb::byte_vector za_state = aarch64_fetch_za_regset (tid);

      gdb_byte *base = za_state.data ();
      struct user_za_header *za_header = (struct user_za_header *) base;
      uint64_t svq = sve_vq_from_vl (za_header->vl);

      /* Sanity check the data in the header.  */
      if (!sve_vl_valid (za_header->vl)
	  || ZA_PT_SIZE (svq) != za_header->size)
	error (_("Invalid vector length or payload size when reading ZA."));

      /* Overwrite the ZA state contained in the thread with the ZA state from
	 the register cache.  */
      if (REG_VALID == reg_buf->get_register_status (za_regnum))
	reg_buf->raw_collect (za_regnum, base + ZA_PT_ZA_OFFSET);

      /* Write back the ZA state to the thread's NT_ARM_ZA register set.  */
      aarch64_store_za_regset (tid, za_state);
    }

  /* Update svcr and svg accordingly.  */
  uint64_t svcr_value = 0;
  svcr_value |= aarch64_has_ssve_state (tid)? SVCR_SM_BIT : 0;
  svcr_value |= aarch64_has_za_state (tid)? SVCR_ZA_BIT : 0;
  reg_buf->raw_supply (svcr_regnum, &svcr_value);

  /* At this point we have written the data contained in the register cache to
     the thread's NT_ARM_ZA register set.  */
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

void
aarch64_zt_regs_copy_to_reg_buf (int tid, struct reg_buffer_common *reg_buf,
				 int zt_regnum)
{
  /* If we have ZA state, read the ZT state.  Otherwise, make the contents of
     ZT in the register cache all zeroes.  This is how we present the ZT
     state when it is not initialized (ZA not active).  */
  if (aarch64_has_za_state (tid))
    {
      /* Fetch the current ZT state from the thread.  */
      gdb::byte_vector zt_state = aarch64_fetch_zt_regset (tid);

      /* Sanity check.  */
      gdb_assert (!zt_state.empty ());

      /* Copy the ZT data to the register buffer.  */
      reg_buf->raw_supply (zt_regnum, zt_state.data ());
    }
  else
    {
      /* Zero out ZT.  */
      gdb::byte_vector zt_zeroed (AARCH64_SME2_ZT0_SIZE, 0);
      reg_buf->raw_supply (zt_regnum, zt_zeroed.data ());
    }

  /* The register buffer should now contain the updated copy of the NT_ARM_ZT
     state.  */
}

/* See nat/aarch64-scalable-linux-ptrace.h.  */

void
aarch64_zt_regs_copy_from_reg_buf (int tid,
				   struct reg_buffer_common *reg_buf,
				   int zt_regnum)
{
  /* Do we have a valid ZA state?  */
  bool valid_za = aarch64_has_za_state (tid);

  /* Is the register buffer contents for ZT all zeroes?  */
  gdb::byte_vector zt_bytes (AARCH64_SME2_ZT0_SIZE, 0);
  bool zt_is_all_zeroes
    = reg_buf->raw_compare (zt_regnum, zt_bytes.data (), 0);

  /* If ZA state is valid or if we have non-zero data for ZT in the register
     buffer, we will invoke ptrace to write the ZT state.  Otherwise we don't
     have to do anything here.  */
  if (valid_za || !zt_is_all_zeroes)
    {
      if (!valid_za)
	{
	  /* ZA state is not valid.  That means we need to initialize the ZA
	     state prior to writing the ZT state.  */
	  aarch64_initialize_za_regset (tid);
	}

      /* Extract the ZT data from the register buffer.  */
      reg_buf->raw_collect (zt_regnum, zt_bytes.data ());

      /* Write the ZT data to thread TID.  */
      aarch64_store_zt_regset (tid, zt_bytes);
    }

  /* At this point we have (potentially) written the data contained in the
     register cache to the thread's NT_ARM_ZT register set.  */
}
