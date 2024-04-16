/* Native-dependent code for x86 (i386 and x86-64).

   Low level functions to implement Operating System specific
   code to manipulate x86 debug registers.

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

#ifndef X86_NAT_H
#define X86_NAT_H 1

#include "breakpoint.h"
#include "nat/x86-dregs.h"
#include "target.h"

/* Hardware-assisted breakpoints and watchpoints.  */

/* Use this function to set x86_dr_low debug_register_length field
   rather than setting it directly to check that the length is only
   set once.  It also enables the 'maint set/show show-debug-regs' 
   command.  */

extern void x86_set_debug_register_length (int len);

/* Use this function to reset the x86-nat.c debug register state.  */

extern void x86_cleanup_dregs (void);

/* Return the debug register state for process PID.  If no existing
   state is found for this process, return nullptr.  */

struct x86_debug_reg_state *x86_lookup_debug_reg_state (pid_t pid);

/* Called whenever GDB is no longer debugging process PID.  It deletes
   data structures that keep track of debug register state.  */

extern void x86_forget_process (pid_t pid);

/* Helper functions used by x86_nat_target below.  See their
   definitions.  */

extern int x86_can_use_hw_breakpoint (enum bptype type, int cnt, int othertype);
extern int x86_region_ok_for_hw_watchpoint (CORE_ADDR addr, int len);
extern int x86_stopped_by_watchpoint ();
extern int x86_stopped_data_address (CORE_ADDR *addr_p);
extern int x86_insert_watchpoint (CORE_ADDR addr, int len,
			   enum target_hw_bp_type type,
			   struct expression *cond);
extern int x86_remove_watchpoint (CORE_ADDR addr, int len,
			   enum target_hw_bp_type type,
			   struct expression *cond);
extern int x86_insert_hw_breakpoint (struct gdbarch *gdbarch,
			      struct bp_target_info *bp_tgt);
extern int x86_remove_hw_breakpoint (struct gdbarch *gdbarch,
				     struct bp_target_info *bp_tgt);
extern int x86_stopped_by_hw_breakpoint ();

/* Convenience template mixin used to add x86 watchpoints support to a
   target.  */

template <typename BaseTarget>
struct x86_nat_target : public BaseTarget
{
  /* Hook in the x86 hardware watchpoints/breakpoints support.  */

  int can_use_hw_breakpoint (enum bptype type, int cnt, int othertype) override
  { return x86_can_use_hw_breakpoint (type, cnt, othertype); }

  int region_ok_for_hw_watchpoint (CORE_ADDR addr, int len) override
  { return x86_region_ok_for_hw_watchpoint (addr, len); }

  int insert_watchpoint (CORE_ADDR addr, int len,
			 enum target_hw_bp_type type,
			 struct expression *cond) override
  { return x86_insert_watchpoint (addr, len, type, cond); }

  int remove_watchpoint (CORE_ADDR addr, int len,
			 enum target_hw_bp_type type,
			 struct expression *cond) override
  { return x86_remove_watchpoint (addr, len, type, cond); }

  int insert_hw_breakpoint (struct gdbarch *gdbarch,
			    struct bp_target_info *bp_tgt) override
  { return x86_insert_hw_breakpoint (gdbarch, bp_tgt); }

  int remove_hw_breakpoint (struct gdbarch *gdbarch,
			    struct bp_target_info *bp_tgt) override
  { return x86_remove_hw_breakpoint (gdbarch, bp_tgt); }

  bool stopped_by_watchpoint () override
  { return x86_stopped_by_watchpoint (); }

  bool stopped_data_address (CORE_ADDR *addr_p) override
  { return x86_stopped_data_address (addr_p); }

  /* A target must provide an implementation of the
     "supports_stopped_by_hw_breakpoint" target method before this
     callback will be used.  */
  bool stopped_by_hw_breakpoint () override
  { return x86_stopped_by_hw_breakpoint (); }
};

#endif /* X86_NAT_H */
