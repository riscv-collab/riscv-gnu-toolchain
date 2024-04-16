/* Common target dependent code for GNU/Linux on ARM systems.

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

#ifndef ARCH_ARM_LINUX_H
#define ARCH_ARM_LINUX_H

/* The index to access CSPR in user_regs defined in GLIBC.  */
#define ARM_CPSR_GREGNUM 16

/* There are a couple of different possible stack layouts that
   we need to support.

   Before version 2.6.18, the kernel used completely independent
   layouts for non-RT and RT signals.  For non-RT signals the stack
   began directly with a struct sigcontext.  For RT signals the stack
   began with two redundant pointers (to the siginfo and ucontext),
   and then the siginfo and ucontext.

   As of version 2.6.18, the non-RT signal frame layout starts with
   a ucontext and the RT signal frame starts with a siginfo and then
   a ucontext.  Also, the ucontext now has a designated save area
   for coprocessor registers.

   For RT signals, it's easy to tell the difference: we look for
   pinfo, the pointer to the siginfo.  If it has the expected
   value, we have an old layout.  If it doesn't, we have the new
   layout.

   For non-RT signals, it's a bit harder.  We need something in one
   layout or the other with a recognizable offset and value.  We can't
   use the return trampoline, because ARM usually uses SA_RESTORER,
   in which case the stack return trampoline is not filled in.
   We can't use the saved stack pointer, because sigaltstack might
   be in use.  So for now we guess the new layout...  */

/* There are three words (trap_no, error_code, oldmask) in
   struct sigcontext before r0.  */
#define ARM_SIGCONTEXT_R0 0xc

/* There are five words (uc_flags, uc_link, and three for uc_stack)
   in the ucontext_t before the sigcontext.  */
#define ARM_UCONTEXT_SIGCONTEXT 0x14

/* There are three elements in an rt_sigframe before the ucontext:
   pinfo, puc, and info.  The first two are pointers and the third
   is a struct siginfo, with size 128 bytes.  We could follow puc
   to the ucontext, but it's simpler to skip the whole thing.  */
#define ARM_OLD_RT_SIGFRAME_SIGINFO 0x8
#define ARM_OLD_RT_SIGFRAME_UCONTEXT 0x88

#define ARM_NEW_RT_SIGFRAME_UCONTEXT 0x80

#define ARM_NEW_SIGFRAME_MAGIC 0x5ac3c35a

int
arm_linux_sigreturn_next_pc_offset (unsigned long sp,
				    unsigned long sp_data,
				    unsigned long svc_number,
				    int is_sigreturn);

struct arm_get_next_pcs;

CORE_ADDR arm_linux_get_next_pcs_fixup (struct arm_get_next_pcs *self,
					CORE_ADDR pc);

#endif /* ARCH_ARM_LINUX_H */
