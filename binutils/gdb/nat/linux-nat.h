/* Code for native debugging support for GNU/Linux (LWP layer).

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#ifndef NAT_LINUX_NAT_H
#define NAT_LINUX_NAT_H

#include "gdbsupport/function-view.h"
#include "target/waitstatus.h"

struct lwp_info;
struct arch_lwp_info;

/* This is the kernel's hard limit.  Not to be confused with SIGRTMIN.  */
#ifndef __SIGRTMIN
#define __SIGRTMIN 32
#endif

/* Unlike other extended result codes, WSTOPSIG (status) on
   PTRACE_O_TRACESYSGOOD syscall events doesn't return SIGTRAP, but
   instead SIGTRAP with bit 7 set.  */
#define SYSCALL_SIGTRAP (SIGTRAP | 0x80)

/* Return the ptid of the current lightweight process.  With NPTL
   threads and LWPs map 1:1, so this is equivalent to returning the
   ptid of the current thread.  This function must be provided by
   the client. */

extern ptid_t current_lwp_ptid (void);

/* Function type for the CALLBACK argument of iterate_over_lwps.  */
typedef int (iterate_over_lwps_ftype) (struct lwp_info *lwp);

/* Iterate over all LWPs.  Calls CALLBACK with its second argument set
   to DATA for every LWP in the list.  If CALLBACK returns nonzero for
   a particular LWP, return a pointer to the structure describing that
   LWP immediately.  Otherwise return NULL.  This function must be
   provided by the client.  */

extern struct lwp_info *iterate_over_lwps
    (ptid_t filter,
     gdb::function_view<iterate_over_lwps_ftype> callback);

/* Return the ptid of LWP.  */

extern ptid_t ptid_of_lwp (struct lwp_info *lwp);

/* Set the architecture-specific data of LWP.  This function must be
   provided by the client. */

extern void lwp_set_arch_private_info (struct lwp_info *lwp,
				       struct arch_lwp_info *info);

/* Return the architecture-specific data of LWP.  This function must
   be provided by the client. */

extern struct arch_lwp_info *lwp_arch_private_info (struct lwp_info *lwp);

/* Return nonzero if LWP is stopped, zero otherwise.  This function
   must be provided by the client.  */

extern int lwp_is_stopped (struct lwp_info *lwp);

/* Return the reason the LWP last stopped.  This function must be
   provided by the client.  */

extern enum target_stop_reason lwp_stop_reason (struct lwp_info *lwp);

/* Cause LWP to stop.  This function must be provided by the
   client.  */

extern void linux_stop_lwp (struct lwp_info *lwp);

/* Return nonzero if we are single-stepping this LWP at the ptrace
   level.  */

extern int lwp_is_stepping (struct lwp_info *lwp);

#endif /* NAT_LINUX_NAT_H */
