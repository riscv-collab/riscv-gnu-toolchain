/* Common Linux arch-specific functionality for AArch64 scalable
   extensions: SVE and SME.

   Copyright (C) 2023-2024 Free Software Foundation, Inc.

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

#include "arch/aarch64-scalable-linux.h"
#include "arch/aarch64.h"
#include "gdbsupport/byte-vector.h"
#include "gdbsupport/common-regcache.h"

/* See arch/aarch64-scalable-linux.h  */

bool
sve_state_is_empty (const struct reg_buffer_common *reg_buf)
{
  /* Instead of allocating a buffer with the size of the current vector
     length, just use a buffer that is big enough for all cases.  */
  gdb_byte zero_buffer[256];

  /* Zero it out.  */
  memset (zero_buffer, 0, 256);

  /* Are any of the Z registers set (non-zero) after the first 128 bits?  */
  for (int i = 0; i < AARCH64_SVE_Z_REGS_NUM; i++)
    {
      if (!reg_buf->raw_compare (AARCH64_SVE_Z0_REGNUM + i, zero_buffer,
				 V_REGISTER_SIZE))
	return false;
    }

  /* Are any of the P registers set (non-zero)?  */
  for (int i = 0; i < AARCH64_SVE_P_REGS_NUM; i++)
    {
      if (!reg_buf->raw_compare (AARCH64_SVE_P0_REGNUM + i, zero_buffer, 0))
	return false;
    }

  /* Is the FFR register set (non-zero)?  */
  return reg_buf->raw_compare (AARCH64_SVE_FFR_REGNUM, zero_buffer, 0);
}
