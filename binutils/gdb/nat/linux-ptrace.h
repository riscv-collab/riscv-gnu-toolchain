/* Copyright (C) 2011-2024 Free Software Foundation, Inc.

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

#ifndef NAT_LINUX_PTRACE_H
#define NAT_LINUX_PTRACE_H

#include "nat/gdb_ptrace.h"
#include "gdbsupport/gdb_wait.h"

#ifdef __UCLIBC__
#if !(defined(__UCLIBC_HAS_MMU__) || defined(__ARCH_HAS_MMU__))
/* PTRACE_TEXT_ADDR and friends.  */
#include <asm/ptrace.h>
#define HAS_NOMMU
#endif
#endif

#if !defined(PTRACE_TYPE_ARG3)
#define PTRACE_TYPE_ARG3 void *
#endif

#if !defined(PTRACE_TYPE_ARG4)
#define PTRACE_TYPE_ARG4 void *
#endif

#ifndef PTRACE_GETSIGINFO
# define PTRACE_GETSIGINFO 0x4202
# define PTRACE_SETSIGINFO 0x4203
#endif /* PTRACE_GETSIGINF */

#ifndef PTRACE_GETREGSET
#define PTRACE_GETREGSET	0x4204
#endif

#ifndef PTRACE_SETREGSET
#define PTRACE_SETREGSET	0x4205
#endif

/* If the system headers did not provide the constants, hard-code the normal
   values.  */
#ifndef PTRACE_EVENT_FORK

#define PTRACE_SETOPTIONS	0x4200
#define PTRACE_GETEVENTMSG	0x4201

/* options set using PTRACE_SETOPTIONS */
#define PTRACE_O_TRACESYSGOOD	0x00000001
#define PTRACE_O_TRACEFORK	0x00000002
#define PTRACE_O_TRACEVFORK	0x00000004
#define PTRACE_O_TRACECLONE	0x00000008
#define PTRACE_O_TRACEEXEC	0x00000010
#define PTRACE_O_TRACEVFORKDONE	0x00000020
#define PTRACE_O_TRACEEXIT	0x00000040

/* Wait extended result codes for the above trace options.  */
#define PTRACE_EVENT_FORK	1
#define PTRACE_EVENT_VFORK	2
#define PTRACE_EVENT_CLONE	3
#define PTRACE_EVENT_EXEC	4
#define PTRACE_EVENT_VFORK_DONE	5
#define PTRACE_EVENT_EXIT	6

#endif /* PTRACE_EVENT_FORK */

#ifndef PTRACE_O_EXITKILL
/* Only defined in Linux Kernel 3.8 or later.  */
#define PTRACE_O_EXITKILL	0x00100000
#endif

#if (defined __bfin__ || defined __frv__ || defined __sh__) \
    && !defined PTRACE_GETFDPIC
#define PTRACE_GETFDPIC		31
#define PTRACE_GETFDPIC_EXEC	0
#define PTRACE_GETFDPIC_INTERP	1
#endif

/* We can't always assume that this flag is available, but all systems
   with the ptrace event handlers also have __WALL, so it's safe to use
   in some contexts.  */
#ifndef __WALL
#define __WALL          0x40000000 /* Wait for any child.  */
#endif

/* True if whether a breakpoint/watchpoint triggered can be determined
   from the si_code of SIGTRAP's siginfo_t (TRAP_BRKPT/TRAP_HWBKPT).
   That is, if the kernel can tell us whether the thread executed a
   software breakpoint, we trust it.  The kernel will be determining
   that from the hardware (e.g., from which exception was raised in
   the CPU).  Relying on whether a breakpoint is planted in memory at
   the time the SIGTRAP is processed to determine whether the thread
   stopped for a software breakpoint can be too late.  E.g., the
   breakpoint could have been removed since.  Or the thread could have
   stepped an instruction the size of a breakpoint instruction, and
   before the stop is processed a breakpoint is inserted at its
   address.  Getting these wrong is disastrous on decr_pc_after_break
   architectures.  The moribund location mechanism helps with that
   somewhat but it is an heuristic, and can well fail.  Getting that
   information out of the kernel and ultimately out of the CPU is the
   way to go.  That said, some architecture may get the si_code wrong,
   and as such we're leaving fallback code in place.  We'll remove
   this after a while if no problem is reported.  */
#define USE_SIGTRAP_SIGINFO 1

/* The x86 kernel gets some of the si_code values backwards, like
   this:

   | what                                     | si_code     |
   |------------------------------------------+-------------|
   | software breakpoints (int3)              | SI_KERNEL   |
   | single-steps                             | TRAP_TRACE  |
   | single-stepping a syscall                | TRAP_BRKPT  |
   | user sent SIGTRAP                        | 0           |
   | exec SIGTRAP (when no PTRACE_EVENT_EXEC) | 0           |
   | hardware breakpoints/watchpoints         | TRAP_HWBKPT |

   That is, it reports SI_KERNEL for software breakpoints (and only
   for those), and TRAP_BRKPT for single-stepping a syscall...  If the
   kernel is ever fixed, we'll just have to detect it like we detect
   optional ptrace features: by forking and debugging ourselves,
   running to a breakpoint and checking what comes out of
   siginfo->si_code.

   The ppc kernel does use TRAP_BRKPT for software breakpoints
   in PowerPC code, but it uses SI_KERNEL for software breakpoints
   in SPU code on a Cell/B.E.  However, SI_KERNEL is never seen
   on a SIGTRAP for any other reason.

   The MIPS kernel up until 4.5 used SI_KERNEL for all kernel
   generated traps.  Since:

     - MIPS doesn't do hardware single-step.
     - We don't need to care about exec SIGTRAPs --- we assume
       PTRACE_EVENT_EXEC is available.
     - The MIPS kernel doesn't support hardware breakpoints.

   on MIPS, all we need to care about is distinguishing between
   software breakpoints and hardware watchpoints, which can be done by
   peeking the debug registers.

   Beginning with Linux 4.6, the MIPS port reports proper TRAP_BRKPT and
   TRAP_HWBKPT codes, so we also match them.

   The generic Linux target code should use GDB_ARCH_IS_TRAP_* instead
   of TRAP_* to abstract out these peculiarities.  */
#if defined __i386__ || defined __x86_64__
# define GDB_ARCH_IS_TRAP_BRKPT(X) ((X) == SI_KERNEL)
# define GDB_ARCH_IS_TRAP_HWBKPT(X) ((X) == TRAP_HWBKPT)
#elif defined __powerpc__
# define GDB_ARCH_IS_TRAP_BRKPT(X) ((X) == SI_KERNEL || (X) == TRAP_BRKPT)
# define GDB_ARCH_IS_TRAP_HWBKPT(X) ((X) == TRAP_HWBKPT)
#elif defined __mips__
# define GDB_ARCH_IS_TRAP_BRKPT(X) ((X) == SI_KERNEL || (X) == TRAP_BRKPT)
# define GDB_ARCH_IS_TRAP_HWBKPT(X) ((X) == SI_KERNEL || (X) == TRAP_HWBKPT)
#else
# define GDB_ARCH_IS_TRAP_BRKPT(X) ((X) == TRAP_BRKPT)
# define GDB_ARCH_IS_TRAP_HWBKPT(X) ((X) == TRAP_HWBKPT)
#endif

#ifndef TRAP_HWBKPT
# define TRAP_HWBKPT 4
#endif

extern std::string linux_ptrace_attach_fail_reason (pid_t pid);

/* Find all possible reasons we could have failed to attach to PTID
   and return them as a string.  ERR is the error PTRACE_ATTACH failed
   with (an errno).  */
extern std::string linux_ptrace_attach_fail_reason_string (ptid_t ptid, int err);

extern void linux_ptrace_init_warnings (void);
extern void linux_check_ptrace_features (void);
extern void linux_enable_event_reporting (pid_t pid, int attached);
extern void linux_disable_event_reporting (pid_t pid);
extern int linux_ptrace_get_extended_event (int wstat);
extern int linux_is_extended_waitstatus (int wstat);
extern int linux_wstatus_maybe_breakpoint (int wstat);

#endif /* NAT_LINUX_PTRACE_H */
