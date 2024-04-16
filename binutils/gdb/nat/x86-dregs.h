/* Debug register code for x86 (i386 and x86-64).

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#ifndef NAT_X86_DREGS_H
#define NAT_X86_DREGS_H

/* Support for hardware watchpoints and breakpoints using the x86
   debug registers.

   This provides several functions for inserting and removing
   hardware-assisted breakpoints and watchpoints, testing if one or
   more of the watchpoints triggered and at what address, checking
   whether a given region can be watched, etc.

   The functions below implement debug registers sharing by reference
   counts, and allow to watch regions up to 16 bytes long
   (32 bytes on 64 bit hosts).  */


#include "gdbsupport/break-common.h"

/* Low-level function vector.  */

struct x86_dr_low_type
  {
    /* Set the debug control (DR7) register to a given value for
       all LWPs.  May be NULL if the debug control register cannot
       be set.  */
    void (*set_control) (unsigned long);

    /* Put an address into one debug register for all LWPs.  May
       be NULL if debug registers cannot be set*/
    void (*set_addr) (int, CORE_ADDR);

    /* Return the address in a given debug register of the current
       LWP.  */
    CORE_ADDR (*get_addr) (int);

    /* Return the value of the debug status (DR6) register for
       current LWP.  */
    unsigned long (*get_status) (void);

    /* Return the value of the debug control (DR7) register for
       current LWP.  */
    unsigned long (*get_control) (void);

    /* Number of bytes used for debug registers (4 or 8).  */
    int debug_register_length;
  };

extern struct x86_dr_low_type x86_dr_low;

/* Debug registers' indices.  */
#define DR_FIRSTADDR 0
#define DR_LASTADDR  3
#define DR_NADDR     4	/* The number of debug address registers.  */
#define DR_STATUS    6	/* Index of debug status register (DR6).  */
#define DR_CONTROL   7	/* Index of debug control register (DR7).  */

/* Global state needed to track h/w watchpoints.  */

struct x86_debug_reg_state
{
  /* Mirror the inferior's DRi registers.  We keep the status and
     control registers separated because they don't hold addresses.
     Note that since we can change these mirrors while threads are
     running, we never trust them to explain a cause of a trap.
     For that, we need to peek directly in the inferior registers.  */
  CORE_ADDR dr_mirror[DR_NADDR];
  unsigned dr_status_mirror, dr_control_mirror;

  /* Reference counts for each debug address register.  */
  int dr_ref_count[DR_NADDR];
};

/* A macro to loop over all debug address registers.  */
#define ALL_DEBUG_ADDRESS_REGISTERS(i) \
  for (i = DR_FIRSTADDR; i <= DR_LASTADDR; i++)

/* Return a pointer to the local mirror of the debug registers of
   process PID.  This function must be provided by the client
   if required.  */
extern struct x86_debug_reg_state *x86_debug_reg_state (pid_t pid);

/* Insert a watchpoint to watch a memory region which starts at
   address ADDR and whose length is LEN bytes.  Watch memory accesses
   of the type TYPE.  Return 0 on success, -1 on failure.  */
extern int x86_dr_insert_watchpoint (struct x86_debug_reg_state *state,
				     enum target_hw_bp_type type,
				     CORE_ADDR addr,
				     int len);

/* Remove a watchpoint that watched the memory region which starts at
   address ADDR, whose length is LEN bytes, and for accesses of the
   type TYPE.  Return 0 on success, -1 on failure.  */
extern int x86_dr_remove_watchpoint (struct x86_debug_reg_state *state,
				     enum target_hw_bp_type type,
				     CORE_ADDR addr,
				     int len);

/* Return non-zero if we can watch a memory region that starts at
   address ADDR and whose length is LEN bytes.  */
extern int x86_dr_region_ok_for_watchpoint (struct x86_debug_reg_state *state,
					    CORE_ADDR addr, int len);

/* If the inferior has some break/watchpoint that triggered, set the
   address associated with that break/watchpoint and return true.
   Otherwise, return false.  */
extern int x86_dr_stopped_data_address (struct x86_debug_reg_state *state,
					CORE_ADDR *addr_p);

/* Return true if the inferior has some watchpoint that triggered.
   Otherwise return false.  */
extern int x86_dr_stopped_by_watchpoint (struct x86_debug_reg_state *state);

/* Return true if the inferior has some hardware breakpoint that
   triggered.  Otherwise return false.  */
extern int x86_dr_stopped_by_hw_breakpoint (struct x86_debug_reg_state *state);

#endif /* NAT_X86_DREGS_H */
