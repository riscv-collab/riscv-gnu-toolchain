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

#ifndef NAT_AARCH64_HW_POINT_H
#define NAT_AARCH64_HW_POINT_H

/* Macro definitions, data structures, and code for the hardware
   breakpoint and hardware watchpoint support follow.  We use the
   following abbreviations throughout the code:

   hw - hardware
   bp - breakpoint
   wp - watchpoint  */

/* Maximum number of hardware breakpoint and watchpoint registers.
   Neither of these values may exceed the width of dr_changed_t
   measured in bits.  */

#define AARCH64_HBP_MAX_NUM 16
#define AARCH64_HWP_MAX_NUM 16

/* Alignment requirement in bytes for addresses written to
   hardware breakpoint and watchpoint value registers.

   A ptrace call attempting to set an address that does not meet the
   alignment criteria will fail.  Limited support has been provided in
   this port for unaligned watchpoints, such that from a GDB user
   perspective, an unaligned watchpoint may be requested.

   This is achieved by minimally enlarging the watched area to meet the
   alignment requirement, and if necessary, splitting the watchpoint
   over several hardware watchpoint registers.  */

#define AARCH64_HBP_ALIGNMENT 4
#define AARCH64_HWP_ALIGNMENT 8

/* The maximum length of a memory region that can be watched by one
   hardware watchpoint register.  */

#define AARCH64_HWP_MAX_LEN_PER_REG 8

/* Macro for the expected version of the ARMv8-A debug architecture.  */
#define AARCH64_DEBUG_ARCH_V8 0x6
#define AARCH64_DEBUG_ARCH_V8_1 0x7
#define AARCH64_DEBUG_ARCH_V8_2 0x8
#define AARCH64_DEBUG_ARCH_V8_4 0x9
#define AARCH64_DEBUG_ARCH_V8_8 0xa
/* Armv8.9 debug architecture.  */
#define AARCH64_DEBUG_ARCH_V8_9 0xb

/* ptrace expects control registers to be formatted as follows:

   31                             13          5      3      1     0
   +--------------------------------+----------+------+------+----+
   |         RESERVED (SBZ)         |   MASK   | TYPE | PRIV | EN |
   +--------------------------------+----------+------+------+----+

   The TYPE field is ignored for breakpoints.  */

#define DR_CONTROL_ENABLED(ctrl)	(((ctrl) & 0x1) == 1)
#define DR_CONTROL_MASK(ctrl)		(((ctrl) >> 5) & 0xff)

/* Structure for managing the hardware breakpoint/watchpoint resources.
   DR_ADDR_* stores the address, DR_CTRL_* stores the control register
   content, and DR_REF_COUNT_* counts the numbers of references to the
   corresponding bp/wp, by which way the limited hardware resources
   are not wasted on duplicated bp/wp settings (though so far gdb has
   done a good job by not sending duplicated bp/wp requests).  */

struct aarch64_debug_reg_state
{
  /* hardware breakpoint */
  CORE_ADDR dr_addr_bp[AARCH64_HBP_MAX_NUM];
  unsigned int dr_ctrl_bp[AARCH64_HBP_MAX_NUM];
  unsigned int dr_ref_count_bp[AARCH64_HBP_MAX_NUM];

  /* hardware watchpoint */
  /* Address aligned down to AARCH64_HWP_ALIGNMENT.  */
  CORE_ADDR dr_addr_wp[AARCH64_HWP_MAX_NUM];
  /* Address as entered by user without any forced alignment.  */
  CORE_ADDR dr_addr_orig_wp[AARCH64_HWP_MAX_NUM];
  unsigned int dr_ctrl_wp[AARCH64_HWP_MAX_NUM];
  unsigned int dr_ref_count_wp[AARCH64_HWP_MAX_NUM];
};

extern int aarch64_num_bp_regs;
extern int aarch64_num_wp_regs;

/* Invoked when IDXth breakpoint/watchpoint register pair needs to be
   updated.  */
void aarch64_notify_debug_reg_change (ptid_t ptid, int is_watchpoint,
				      unsigned int idx);

unsigned int aarch64_watchpoint_offset (unsigned int ctrl);
unsigned int aarch64_watchpoint_length (unsigned int ctrl);

int aarch64_handle_breakpoint (enum target_hw_bp_type type, CORE_ADDR addr,
			       int len, int is_insert, ptid_t ptid,
			       struct aarch64_debug_reg_state *state);
int aarch64_handle_watchpoint (enum target_hw_bp_type type, CORE_ADDR addr,
			       int len, int is_insert, ptid_t ptid,
			       struct aarch64_debug_reg_state *state);

/* Return TRUE if there are any hardware breakpoints.  If WATCHPOINT is TRUE,
   check hardware watchpoints instead.  */
bool aarch64_any_set_debug_regs_state (aarch64_debug_reg_state *state,
				       bool watchpoint);

void aarch64_show_debug_reg_state (struct aarch64_debug_reg_state *state,
				   const char *func, CORE_ADDR addr,
				   int len, enum target_hw_bp_type type);

int aarch64_region_ok_for_watchpoint (CORE_ADDR addr, int len);

#endif /* NAT_AARCH64_HW_POINT_H */
