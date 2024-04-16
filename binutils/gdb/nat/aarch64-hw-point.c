/* Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "gdbsupport/break-common.h"
#include "gdbsupport/common-regcache.h"
#include "aarch64-hw-point.h"

#ifdef __linux__
/* For kernel_supports_any_contiguous_range.  */
#include "aarch64-linux-hw-point.h"
#else
#define	kernel_supports_any_contiguous_range	true
#endif

/* Number of hardware breakpoints/watchpoints the target supports.
   They are initialized with values obtained via ptrace.  */

int aarch64_num_bp_regs;
int aarch64_num_wp_regs;

/* Return starting byte 0..7 incl. of a watchpoint encoded by CTRL.  */

unsigned int
aarch64_watchpoint_offset (unsigned int ctrl)
{
  uint8_t mask = DR_CONTROL_MASK (ctrl);
  unsigned retval;

  /* Shift out bottom zeros.  */
  for (retval = 0; mask && (mask & 1) == 0; ++retval)
    mask >>= 1;

  return retval;
}

/* Utility function that returns the length in bytes of a watchpoint
   according to the content of a hardware debug control register CTRL.
   Any contiguous range of bytes in CTRL is supported.  The returned
   value can be between 0..8 (inclusive).  */

unsigned int
aarch64_watchpoint_length (unsigned int ctrl)
{
  uint8_t mask = DR_CONTROL_MASK (ctrl);
  unsigned retval;

  /* Shift out bottom zeros.  */
  mask >>= aarch64_watchpoint_offset (ctrl);

  /* Count bottom ones.  */
  for (retval = 0; (mask & 1) != 0; ++retval)
    mask >>= 1;

  if (mask != 0)
    error (_("Unexpected hardware watchpoint length register value 0x%x"),
	   DR_CONTROL_MASK (ctrl));

  return retval;
}

/* Given the hardware breakpoint or watchpoint type TYPE and its
   length LEN, return the expected encoding for a hardware
   breakpoint/watchpoint control register.  */

static unsigned int
aarch64_point_encode_ctrl_reg (enum target_hw_bp_type type, int offset, int len)
{
  unsigned int ctrl, ttype;

  gdb_assert (offset == 0 || kernel_supports_any_contiguous_range);
  gdb_assert (offset + len <= AARCH64_HWP_MAX_LEN_PER_REG);

  /* type */
  switch (type)
    {
    case hw_write:
      ttype = 2;
      break;
    case hw_read:
      ttype = 1;
      break;
    case hw_access:
      ttype = 3;
      break;
    case hw_execute:
      ttype = 0;
      break;
    default:
      perror_with_name (_("Unrecognized breakpoint/watchpoint type"));
    }

  ctrl = ttype << 3;

  /* offset and length bitmask */
  ctrl |= ((1 << len) - 1) << (5 + offset);
  /* enabled at el0 */
  ctrl |= (2 << 1) | 1;

  return ctrl;
}

/* Addresses to be written to the hardware breakpoint and watchpoint
   value registers need to be aligned; the alignment is 4-byte and
   8-type respectively.  Linux kernel rejects any non-aligned address
   it receives from the related ptrace call.  Furthermore, the kernel
   currently only supports the following Byte Address Select (BAS)
   values: 0x1, 0x3, 0xf and 0xff, which means that for a hardware
   watchpoint to be accepted by the kernel (via ptrace call), its
   valid length can only be 1 byte, 2 bytes, 4 bytes or 8 bytes.
   Despite these limitations, the unaligned watchpoint is supported in
   this port.

   Return 0 for any non-compliant ADDR and/or LEN; return 1 otherwise.  */

static int
aarch64_point_is_aligned (ptid_t ptid, int is_watchpoint, CORE_ADDR addr,
			  int len)
{
  unsigned int alignment = 0;

  if (is_watchpoint)
    alignment = AARCH64_HWP_ALIGNMENT;
  else
    {
      reg_buffer_common *regcache = get_thread_regcache_for_ptid (ptid);

      /* Set alignment to 2 only if the current process is 32-bit,
	 since thumb instruction can be 2-byte aligned.  Otherwise, set
	 alignment to AARCH64_HBP_ALIGNMENT.  */
      if (regcache_register_size (regcache, 0) == 8)
	alignment = AARCH64_HBP_ALIGNMENT;
      else
	alignment = 2;
    }

  if (addr & (alignment - 1))
    return 0;

  if ((!kernel_supports_any_contiguous_range
       && len != 8 && len != 4 && len != 2 && len != 1)
      || (kernel_supports_any_contiguous_range
	  && (len < 1 || len > 8)))
    return 0;

  return 1;
}

/* Given the (potentially unaligned) watchpoint address in ADDR and
   length in LEN, return the aligned address, offset from that base
   address, and aligned length in *ALIGNED_ADDR_P, *ALIGNED_OFFSET_P
   and *ALIGNED_LEN_P, respectively.  The returned values will be
   valid values to write to the hardware watchpoint value and control
   registers.

   The given watchpoint may get truncated if more than one hardware
   register is needed to cover the watched region.  *NEXT_ADDR_P
   and *NEXT_LEN_P, if non-NULL, will return the address and length
   of the remaining part of the watchpoint (which can be processed
   by calling this routine again to generate another aligned address,
   offset and length tuple.

   Essentially, unaligned watchpoint is achieved by minimally
   enlarging the watched area to meet the alignment requirement, and
   if necessary, splitting the watchpoint over several hardware
   watchpoint registers.

   On kernels that predate the support for Byte Address Select (BAS)
   in the hardware watchpoint control register, the offset from the
   base address is always zero, and so in that case the trade-off is
   that there will be false-positive hits for the read-type or the
   access-type hardware watchpoints; for the write type, which is more
   commonly used, there will be no such issues, as the higher-level
   breakpoint management in gdb always examines the exact watched
   region for any content change, and transparently resumes a thread
   from a watchpoint trap if there is no change to the watched region.

   Another limitation is that because the watched region is enlarged,
   the watchpoint fault address discovered by
   aarch64_stopped_data_address may be outside of the original watched
   region, especially when the triggering instruction is accessing a
   larger region.  When the fault address is not within any known
   range, watchpoints_triggered in gdb will get confused, as the
   higher-level watchpoint management is only aware of original
   watched regions, and will think that some unknown watchpoint has
   been triggered.  To prevent such a case,
   aarch64_stopped_data_address implementations in gdb and gdbserver
   try to match the trapped address with a watched region, and return
   an address within the latter. */

static void
aarch64_align_watchpoint (CORE_ADDR addr, int len, CORE_ADDR *aligned_addr_p,
			  int *aligned_offset_p, int *aligned_len_p,
			  CORE_ADDR *next_addr_p, int *next_len_p,
			  CORE_ADDR *next_addr_orig_p)
{
  int aligned_len;
  unsigned int offset, aligned_offset;
  CORE_ADDR aligned_addr;
  const unsigned int alignment = AARCH64_HWP_ALIGNMENT;
  const unsigned int max_wp_len = AARCH64_HWP_MAX_LEN_PER_REG;

  /* As assumed by the algorithm.  */
  gdb_assert (alignment == max_wp_len);

  if (len <= 0)
    return;

  /* The address put into the hardware watchpoint value register must
     be aligned.  */
  offset = addr & (alignment - 1);
  aligned_addr = addr - offset;
  aligned_offset
    = kernel_supports_any_contiguous_range ? addr & (alignment - 1) : 0;

  gdb_assert (offset >= 0 && offset < alignment);
  gdb_assert (aligned_addr >= 0 && aligned_addr <= addr);
  gdb_assert (offset + len > 0);

  if (offset + len >= max_wp_len)
    {
      /* Need more than one watchpoint register; truncate at the
	 alignment boundary.  */
      aligned_len
	= max_wp_len - (kernel_supports_any_contiguous_range ? offset : 0);
      len -= (max_wp_len - offset);
      addr += (max_wp_len - offset);
      gdb_assert ((addr & (alignment - 1)) == 0);
    }
  else
    {
      /* Find the smallest valid length that is large enough to
	 accommodate this watchpoint.  */
      static const unsigned char
	aligned_len_array[AARCH64_HWP_MAX_LEN_PER_REG] =
	{ 1, 2, 4, 4, 8, 8, 8, 8 };

      aligned_len = (kernel_supports_any_contiguous_range
		     ? len : aligned_len_array[offset + len - 1]);
      addr += len;
      len = 0;
    }

  if (aligned_addr_p)
    *aligned_addr_p = aligned_addr;
  if (aligned_offset_p)
    *aligned_offset_p = aligned_offset;
  if (aligned_len_p)
    *aligned_len_p = aligned_len;
  if (next_addr_p)
    *next_addr_p = addr;
  if (next_len_p)
    *next_len_p = len;
  if (next_addr_orig_p)
    *next_addr_orig_p = align_down (*next_addr_orig_p + alignment, alignment);
}

/* Record the insertion of one breakpoint/watchpoint, as represented
   by ADDR and CTRL, in the process' arch-specific data area *STATE.  */

static int
aarch64_dr_state_insert_one_point (ptid_t ptid,
				   struct aarch64_debug_reg_state *state,
				   enum target_hw_bp_type type,
				   CORE_ADDR addr, int offset, int len,
				   CORE_ADDR addr_orig)
{
  int i, idx, num_regs, is_watchpoint;
  unsigned int ctrl, *dr_ctrl_p, *dr_ref_count;
  CORE_ADDR *dr_addr_p, *dr_addr_orig_p;

  /* Set up state pointers.  */
  is_watchpoint = (type != hw_execute);
  gdb_assert (aarch64_point_is_aligned (ptid, is_watchpoint, addr, len));
  if (is_watchpoint)
    {
      num_regs = aarch64_num_wp_regs;
      dr_addr_p = state->dr_addr_wp;
      dr_addr_orig_p = state->dr_addr_orig_wp;
      dr_ctrl_p = state->dr_ctrl_wp;
      dr_ref_count = state->dr_ref_count_wp;
    }
  else
    {
      num_regs = aarch64_num_bp_regs;
      dr_addr_p = state->dr_addr_bp;
      dr_addr_orig_p = nullptr;
      dr_ctrl_p = state->dr_ctrl_bp;
      dr_ref_count = state->dr_ref_count_bp;
    }

  ctrl = aarch64_point_encode_ctrl_reg (type, offset, len);

  /* Find an existing or free register in our cache.  */
  idx = -1;
  for (i = 0; i < num_regs; ++i)
    {
      if ((dr_ctrl_p[i] & 1) == 0)
	{
	  gdb_assert (dr_ref_count[i] == 0);
	  idx = i;
	  /* no break; continue hunting for an existing one.  */
	}
      else if (dr_addr_p[i] == addr
	       && (dr_addr_orig_p == nullptr || dr_addr_orig_p[i] == addr_orig)
	       && dr_ctrl_p[i] == ctrl)
	{
	  gdb_assert (dr_ref_count[i] != 0);
	  idx = i;
	  break;
	}
    }

  /* No space.  */
  if (idx == -1)
    return -1;

  /* Update our cache.  */
  if ((dr_ctrl_p[idx] & 1) == 0)
    {
      /* new entry */
      dr_addr_p[idx] = addr;
      if (dr_addr_orig_p != nullptr)
	dr_addr_orig_p[idx] = addr_orig;
      dr_ctrl_p[idx] = ctrl;
      dr_ref_count[idx] = 1;
      /* Notify the change.  */
      aarch64_notify_debug_reg_change (ptid, is_watchpoint, idx);
    }
  else
    {
      /* existing entry */
      dr_ref_count[idx]++;
    }

  return 0;
}

/* Record the removal of one breakpoint/watchpoint, as represented by
   ADDR and CTRL, in the process' arch-specific data area *STATE.  */

static int
aarch64_dr_state_remove_one_point (ptid_t ptid,
				   struct aarch64_debug_reg_state *state,
				   enum target_hw_bp_type type,
				   CORE_ADDR addr, int offset, int len,
				   CORE_ADDR addr_orig)
{
  int i, num_regs, is_watchpoint;
  unsigned int ctrl, *dr_ctrl_p, *dr_ref_count;
  CORE_ADDR *dr_addr_p, *dr_addr_orig_p;

  /* Set up state pointers.  */
  is_watchpoint = (type != hw_execute);
  if (is_watchpoint)
    {
      num_regs = aarch64_num_wp_regs;
      dr_addr_p = state->dr_addr_wp;
      dr_addr_orig_p = state->dr_addr_orig_wp;
      dr_ctrl_p = state->dr_ctrl_wp;
      dr_ref_count = state->dr_ref_count_wp;
    }
  else
    {
      num_regs = aarch64_num_bp_regs;
      dr_addr_p = state->dr_addr_bp;
      dr_addr_orig_p = nullptr;
      dr_ctrl_p = state->dr_ctrl_bp;
      dr_ref_count = state->dr_ref_count_bp;
    }

  ctrl = aarch64_point_encode_ctrl_reg (type, offset, len);

  /* Find the entry that matches the ADDR and CTRL.  */
  for (i = 0; i < num_regs; ++i)
    if (dr_addr_p[i] == addr
	&& (dr_addr_orig_p == nullptr || dr_addr_orig_p[i] == addr_orig)
	&& dr_ctrl_p[i] == ctrl)
      {
	gdb_assert (dr_ref_count[i] != 0);
	break;
      }

  /* Not found.  */
  if (i == num_regs)
    return -1;

  /* Clear our cache.  */
  if (--dr_ref_count[i] == 0)
    {
      /* Clear the enable bit.  */
      ctrl &= ~1;
      dr_addr_p[i] = 0;
      if (dr_addr_orig_p != nullptr)
	dr_addr_orig_p[i] = 0;
      dr_ctrl_p[i] = ctrl;
      /* Notify the change.  */
      aarch64_notify_debug_reg_change (ptid, is_watchpoint, i);
    }

  return 0;
}

int
aarch64_handle_breakpoint (enum target_hw_bp_type type, CORE_ADDR addr,
			   int len, int is_insert, ptid_t ptid,
			   struct aarch64_debug_reg_state *state)
{
  if (is_insert)
    {
      /* The hardware breakpoint on AArch64 should always be 4-byte
	 aligned, but on AArch32, it can be 2-byte aligned.  Note that
	 we only check the alignment on inserting breakpoint because
	 aarch64_point_is_aligned needs the inferior_ptid inferior's
	 regcache to decide whether the inferior is 32-bit or 64-bit.
	 However when GDB follows the parent process and detach breakpoints
	 from child process, inferior_ptid is the child ptid, but the
	 child inferior doesn't exist in GDB's view yet.  */
      if (!aarch64_point_is_aligned (ptid, 0 /* is_watchpoint */ , addr, len))
	return -1;

      return aarch64_dr_state_insert_one_point (ptid, state, type, addr, 0, len,
						-1);
    }
  else
    return aarch64_dr_state_remove_one_point (ptid, state, type, addr, 0, len,
					      -1);
}

/* This is essentially the same as aarch64_handle_breakpoint, apart
   from that it is an aligned watchpoint to be handled.  */

static int
aarch64_handle_aligned_watchpoint (enum target_hw_bp_type type,
				   CORE_ADDR addr, int len, int is_insert,
				   ptid_t ptid,
				   struct aarch64_debug_reg_state *state)
{
  if (is_insert)
    return aarch64_dr_state_insert_one_point (ptid, state, type, addr, 0, len,
					      addr);
  else
    return aarch64_dr_state_remove_one_point (ptid, state, type, addr, 0, len,
					      addr);
}

/* Insert/remove unaligned watchpoint by calling
   aarch64_align_watchpoint repeatedly until the whole watched region,
   as represented by ADDR and LEN, has been properly aligned and ready
   to be written to one or more hardware watchpoint registers.
   IS_INSERT indicates whether this is an insertion or a deletion.
   Return 0 if succeed.  */

static int
aarch64_handle_unaligned_watchpoint (enum target_hw_bp_type type,
				     CORE_ADDR addr, int len, int is_insert,
				     ptid_t ptid,
				     struct aarch64_debug_reg_state *state)
{
  CORE_ADDR addr_orig = addr;

  while (len > 0)
    {
      CORE_ADDR aligned_addr;
      int aligned_offset, aligned_len, ret;
      CORE_ADDR addr_orig_next = addr_orig;

      aarch64_align_watchpoint (addr, len, &aligned_addr, &aligned_offset,
				&aligned_len, &addr, &len, &addr_orig_next);

      if (is_insert)
	ret = aarch64_dr_state_insert_one_point (ptid, state, type,
						 aligned_addr, aligned_offset,
						 aligned_len, addr_orig);
      else
	ret = aarch64_dr_state_remove_one_point (ptid, state, type,
						 aligned_addr, aligned_offset,
						 aligned_len, addr_orig);

      if (show_debug_regs)
	debug_printf ("handle_unaligned_watchpoint: is_insert: %d\n"
		      "                             "
		      "aligned_addr: %s, aligned_len: %d\n"
		      "                                "
		      "addr_orig: %s\n"
		      "                                "
		      "next_addr: %s,    next_len: %d\n"
		      "                           "
		      "addr_orig_next: %s\n",
		      is_insert, core_addr_to_string_nz (aligned_addr),
		      aligned_len, core_addr_to_string_nz (addr_orig),
		      core_addr_to_string_nz (addr), len,
		      core_addr_to_string_nz (addr_orig_next));

      addr_orig = addr_orig_next;

      if (ret != 0)
	return ret;
    }

  return 0;
}

int
aarch64_handle_watchpoint (enum target_hw_bp_type type, CORE_ADDR addr,
			   int len, int is_insert, ptid_t ptid,
			   struct aarch64_debug_reg_state *state)
{
  if (aarch64_point_is_aligned (ptid, 1 /* is_watchpoint */ , addr, len))
    return aarch64_handle_aligned_watchpoint (type, addr, len, is_insert, ptid,
					      state);
  else
    return aarch64_handle_unaligned_watchpoint (type, addr, len, is_insert,
						ptid, state);
}

/* See nat/aarch64-hw-point.h.  */

bool
aarch64_any_set_debug_regs_state (aarch64_debug_reg_state *state,
				  bool watchpoint)
{
  int count = watchpoint ? aarch64_num_wp_regs : aarch64_num_bp_regs;
  if (count == 0)
    return false;

  const CORE_ADDR *addr = watchpoint ? state->dr_addr_wp : state->dr_addr_bp;
  const unsigned int *ctrl = watchpoint ? state->dr_ctrl_wp : state->dr_ctrl_bp;

  for (int i = 0; i < count; i++)
    if (addr[i] != 0 || ctrl[i] != 0)
      return true;

  return false;
}

/* Print the values of the cached breakpoint/watchpoint registers.  */

void
aarch64_show_debug_reg_state (struct aarch64_debug_reg_state *state,
			      const char *func, CORE_ADDR addr,
			      int len, enum target_hw_bp_type type)
{
  int i;

  debug_printf ("%s", func);
  if (addr || len)
    debug_printf (" (addr=0x%08lx, len=%d, type=%s)",
		  (unsigned long) addr, len,
		  type == hw_write ? "hw-write-watchpoint"
		  : (type == hw_read ? "hw-read-watchpoint"
		     : (type == hw_access ? "hw-access-watchpoint"
			: (type == hw_execute ? "hw-breakpoint"
			   : "??unknown??"))));
  debug_printf (":\n");

  debug_printf ("\tBREAKPOINTs:\n");
  for (i = 0; i < aarch64_num_bp_regs; i++)
    debug_printf ("\tBP%d: addr=%s, ctrl=0x%08x, ref.count=%d\n",
		  i, core_addr_to_string_nz (state->dr_addr_bp[i]),
		  state->dr_ctrl_bp[i], state->dr_ref_count_bp[i]);

  debug_printf ("\tWATCHPOINTs:\n");
  for (i = 0; i < aarch64_num_wp_regs; i++)
    debug_printf ("\tWP%d: addr=%s (orig=%s), ctrl=0x%08x, ref.count=%d\n",
		  i, core_addr_to_string_nz (state->dr_addr_wp[i]),
		  core_addr_to_string_nz (state->dr_addr_orig_wp[i]),
		  state->dr_ctrl_wp[i], state->dr_ref_count_wp[i]);
}

/* Return true if we can watch a memory region that starts address
   ADDR and whose length is LEN in bytes.  */

int
aarch64_region_ok_for_watchpoint (CORE_ADDR addr, int len)
{
  CORE_ADDR aligned_addr;

  /* Can not set watchpoints for zero or negative lengths.  */
  if (len <= 0)
    return 0;

  /* Must have hardware watchpoint debug register(s).  */
  if (aarch64_num_wp_regs == 0)
    return 0;

  /* We support unaligned watchpoint address and arbitrary length,
     as long as the size of the whole watched area after alignment
     doesn't exceed size of the total area that all watchpoint debug
     registers can watch cooperatively.

     This is a very relaxed rule, but unfortunately there are
     limitations, e.g. false-positive hits, due to limited support of
     hardware debug registers in the kernel.  See comment above
     aarch64_align_watchpoint for more information.  */

  aligned_addr = addr & ~(AARCH64_HWP_MAX_LEN_PER_REG - 1);
  if (aligned_addr + aarch64_num_wp_regs * AARCH64_HWP_MAX_LEN_PER_REG
      < addr + len)
    return 0;

  /* All tests passed so we are likely to be able to set the watchpoint.
     The reason that it is 'likely' rather than 'must' is because
     we don't check the current usage of the watchpoint registers, and
     there may not be enough registers available for this watchpoint.
     Ideally we should check the cached debug register state, however
     the checking is costly.  */
  return 1;
}
