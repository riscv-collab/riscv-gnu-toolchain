/* Low-level debug register code for GNU/Linux x86 (i386 and x86-64).

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

#ifndef NAT_X86_LINUX_DREGS_H
#define NAT_X86_LINUX_DREGS_H

/* Return the address stored in the current inferior's debug register
   REGNUM.  */

extern CORE_ADDR x86_linux_dr_get_addr (int regnum);

/* Store ADDR in debug register REGNUM of all LWPs of the current
   inferior.  */

extern void x86_linux_dr_set_addr (int regnum, CORE_ADDR addr);

/* Return the value stored in the current inferior's debug control
   register.  */

extern unsigned long x86_linux_dr_get_control (void);

/* Store CONTROL in the debug control registers of all LWPs of the
   current inferior.  */

extern void x86_linux_dr_set_control (unsigned long control);

/* Return the value stored in the current inferior's debug status
   register.  */

extern unsigned long x86_linux_dr_get_status (void);

/* Update the thread's debug registers if the values in our local
   mirror have been changed.  */

extern void x86_linux_update_debug_registers (struct lwp_info *lwp);

#endif /* NAT_X86_LINUX_DREGS_H */
