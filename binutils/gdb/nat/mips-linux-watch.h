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

#ifndef NAT_MIPS_LINUX_WATCH_H
#define NAT_MIPS_LINUX_WATCH_H

#include <asm/ptrace.h>
#include "gdbsupport/break-common.h"

#define MAX_DEBUG_REGISTER 8

/* If macro PTRACE_GET_WATCH_REGS is not defined, kernel header doesn't
   have hardware watchpoint-related structures.  Define them below.  */

#ifndef PTRACE_GET_WATCH_REGS
#  define PTRACE_GET_WATCH_REGS	0xd0
#  define PTRACE_SET_WATCH_REGS	0xd1

enum pt_watch_style {
  pt_watch_style_mips32,
  pt_watch_style_mips64
};

/* A value of zero in a watchlo indicates that it is available.  */

struct mips32_watch_regs
{
  uint32_t watchlo[MAX_DEBUG_REGISTER];
  /* Lower 16 bits of watchhi.  */
  uint16_t watchhi[MAX_DEBUG_REGISTER];
  /* Valid mask and I R W bits.
   * bit 0 -- 1 if W bit is usable.
   * bit 1 -- 1 if R bit is usable.
   * bit 2 -- 1 if I bit is usable.
   * bits 3 - 11 -- Valid watchhi mask bits.
   */
  uint16_t watch_masks[MAX_DEBUG_REGISTER];
  /* The number of valid watch register pairs.  */
  uint32_t num_valid;
  /* There is confusion across gcc versions about structure alignment,
     so we force 8 byte alignment for these structures so they match
     the kernel even if it was build with a different gcc version.  */
} __attribute__ ((aligned (8)));

struct mips64_watch_regs
{
  uint64_t watchlo[MAX_DEBUG_REGISTER];
  uint16_t watchhi[MAX_DEBUG_REGISTER];
  uint16_t watch_masks[MAX_DEBUG_REGISTER];
  uint32_t num_valid;
} __attribute__ ((aligned (8)));

struct pt_watch_regs
{
  enum pt_watch_style style;
  union
  {
    struct mips32_watch_regs mips32;
    struct mips64_watch_regs mips64;
  };
};

#endif /* !PTRACE_GET_WATCH_REGS */

#define W_BIT 0
#define R_BIT 1
#define I_BIT 2

#define W_MASK (1 << W_BIT)
#define R_MASK (1 << R_BIT)
#define I_MASK (1 << I_BIT)

#define IRW_MASK (I_MASK | R_MASK | W_MASK)

/* We keep list of all watchpoints we should install and calculate the
   watch register values each time the list changes.  This allows for
   easy sharing of watch registers for more than one watchpoint.  */

struct mips_watchpoint
{
  CORE_ADDR addr;
  int len;
  enum target_hw_bp_type type;
  struct mips_watchpoint *next;
};

uint32_t mips_linux_watch_get_num_valid (struct pt_watch_regs *regs);
uint32_t mips_linux_watch_get_irw_mask (struct pt_watch_regs *regs, int n);
CORE_ADDR mips_linux_watch_get_watchlo (struct pt_watch_regs *regs, int n);
void mips_linux_watch_set_watchlo (struct pt_watch_regs *regs, int n,
				   CORE_ADDR value);
uint32_t mips_linux_watch_get_watchhi (struct pt_watch_regs *regs, int n);
void mips_linux_watch_set_watchhi (struct pt_watch_regs *regs, int n,
				   uint16_t value);
int mips_linux_watch_try_one_watch (struct pt_watch_regs *regs,
				    CORE_ADDR addr, int len, uint32_t irw);
void mips_linux_watch_populate_regs (struct mips_watchpoint *current_watches,
				     struct pt_watch_regs *regs);
uint32_t mips_linux_watch_type_to_irw (enum target_hw_bp_type type);

int mips_linux_read_watch_registers (long lwpid,
				     struct pt_watch_regs *watch_readback,
				     int *watch_readback_valid, int force);

#endif /* NAT_MIPS_LINUX_WATCH_H */
