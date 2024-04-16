/* Copyright (C) 2009-2024 Free Software Foundation, Inc.
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

#ifndef NAT_AARCH64_LINUX_HW_POINT_H
#define NAT_AARCH64_LINUX_HW_POINT_H

#include "gdbsupport/break-common.h"

#include "nat/aarch64-hw-point.h"

/* ptrace hardware breakpoint resource info is formatted as follows:

   31             24             16               8              0
   +---------------+--------------+---------------+---------------+
   |   RESERVED    |   RESERVED   |   DEBUG_ARCH  |  NUM_SLOTS    |
   +---------------+--------------+---------------+---------------+  */


/* Macros to extract fields from the hardware debug information word.  */
#define AARCH64_DEBUG_NUM_SLOTS(x) ((x) & 0xff)
#define AARCH64_DEBUG_ARCH(x) (((x) >> 8) & 0xff)

/* Each bit of a variable of this type is used to indicate whether a
   hardware breakpoint or watchpoint setting has been changed since
   the last update.

   Bit N corresponds to the Nth hardware breakpoint or watchpoint
   setting which is managed in aarch64_debug_reg_state, where N is
   valid between 0 and the total number of the hardware breakpoint or
   watchpoint debug registers minus 1.

   When bit N is 1, the corresponding breakpoint or watchpoint setting
   has changed, and therefore the corresponding hardware debug
   register needs to be updated via the ptrace interface.

   In the per-thread arch-specific data area, we define two such
   variables for per-thread hardware breakpoint and watchpoint
   settings respectively.

   This type is part of the mechanism which helps reduce the number of
   ptrace calls to the kernel, i.e. avoid asking the kernel to write
   to the debug registers with unchanged values.  */

typedef ULONGEST dr_changed_t;

/* Set each of the lower M bits of X to 1; assert X is wide enough.  */

#define DR_MARK_ALL_CHANGED(x, m)					\
  do									\
    {									\
      gdb_assert (sizeof ((x)) * 8 >= (m));				\
      (x) = (((dr_changed_t)1 << (m)) - 1);				\
    } while (0)

#define DR_MARK_N_CHANGED(x, n)						\
  do									\
    {									\
      (x) |= ((dr_changed_t)1 << (n));					\
    } while (0)

#define DR_CLEAR_CHANGED(x)						\
  do									\
    {									\
      (x) = 0;								\
    } while (0)

#define DR_HAS_CHANGED(x) ((x) != 0)
#define DR_N_HAS_CHANGED(x, n) ((x) & ((dr_changed_t)1 << (n)))

/* Per-thread arch-specific data we want to keep.  */

struct arch_lwp_info
{
  /* When bit N is 1, it indicates the Nth hardware breakpoint or
     watchpoint register pair needs to be updated when the thread is
     resumed; see aarch64_linux_prepare_to_resume.  */
  dr_changed_t dr_changed_bp;
  dr_changed_t dr_changed_wp;
};

/* True if this kernel does not have the bug described by PR
   external/20207 (Linux >= 4.10).  A fixed kernel supports any
   contiguous range of bits in 8-bit byte DR_CONTROL_MASK.  A buggy
   kernel supports only 0x01, 0x03, 0x0f and 0xff.  We start by
   assuming the bug is fixed, and then detect the bug at
   PTRACE_SETREGSET time.  */

extern bool kernel_supports_any_contiguous_range;

void aarch64_linux_set_debug_regs (struct aarch64_debug_reg_state *state,
				   int tid, int watchpoint);

void aarch64_linux_get_debug_reg_capacity (int tid);

struct aarch64_debug_reg_state *aarch64_get_debug_reg_state (pid_t pid);

#endif /* NAT_AARCH64_LINUX_HW_POINT_H */
