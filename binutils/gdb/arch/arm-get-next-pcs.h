/* Common code for ARM software single stepping support.

   Copyright (C) 1988-2024 Free Software Foundation, Inc.

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

#ifndef ARCH_ARM_GET_NEXT_PCS_H
#define ARCH_ARM_GET_NEXT_PCS_H

#include <vector>

/* Forward declaration.  */
struct arm_get_next_pcs;
struct reg_buffer_common;

/* get_next_pcs operations.  */
struct arm_get_next_pcs_ops
{
  ULONGEST (*read_mem_uint) (CORE_ADDR memaddr, int len, int byte_order);
  CORE_ADDR (*syscall_next_pc) (struct arm_get_next_pcs *self);
  CORE_ADDR (*addr_bits_remove) (struct arm_get_next_pcs *self, CORE_ADDR val);
  int (*is_thumb) (struct arm_get_next_pcs *self);

  /* Fix up PC if needed.  */
  CORE_ADDR (*fixup) (struct arm_get_next_pcs *self, CORE_ADDR pc);
};

/* Context for a get_next_pcs call on ARM.  */
struct arm_get_next_pcs
{
  /* Operations implementations.  */
  struct arm_get_next_pcs_ops *ops;
  /* Byte order for data.  */
  int byte_order;
  /* Byte order for code.  */
  int byte_order_for_code;
  /* Whether the target has 32-bit thumb-2 breakpoint defined or
     not.  */
  int has_thumb2_breakpoint;
  /* Registry cache.  */
  reg_buffer_common *regcache;
};

/* Initialize arm_get_next_pcs.  */
void arm_get_next_pcs_ctor (struct arm_get_next_pcs *self,
			    struct arm_get_next_pcs_ops *ops,
			    int byte_order,
			    int byte_order_for_code,
			    int has_thumb2_breakpoint,
			    reg_buffer_common *regcache);

/* Find the next possible PCs after the current instruction executes.  */
std::vector<CORE_ADDR> arm_get_next_pcs (struct arm_get_next_pcs *self);

#endif /* ARCH_ARM_GET_NEXT_PCS_H */
