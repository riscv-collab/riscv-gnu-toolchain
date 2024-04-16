/* Native-dependent code for GNU/Linux x86 (i386 and x86-64).

   Copyright (C) 1999-2024 Free Software Foundation, Inc.

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

#ifndef NAT_X86_LINUX_H
#define NAT_X86_LINUX_H

#include "nat/linux-nat.h"

/* Set whether our local mirror of LWP's debug registers has been
   changed since the values were last written to the thread.  Nonzero
   indicates that a change has been made, zero indicates no change.  */

extern void lwp_set_debug_registers_changed (struct lwp_info *lwp,
					     int value);

/* Return nonzero if our local mirror of LWP's debug registers has
   been changed since the values were last written to the thread,
   zero otherwise.  */

extern int lwp_debug_registers_changed (struct lwp_info *lwp);

/* Function to call when a new thread is detected.  */

extern void x86_linux_new_thread (struct lwp_info *lwp);

/* Function to call when a thread is being deleted.  */

extern void x86_linux_delete_thread (struct arch_lwp_info *arch_lwp);

/* Function to call prior to resuming a thread.  */

extern void x86_linux_prepare_to_resume (struct lwp_info *lwp);

#endif /* NAT_X86_LINUX_H */
